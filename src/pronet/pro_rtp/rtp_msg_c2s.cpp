/*
 * Copyright (C) 2018-2019 Eric Tung <libpronet@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of LibProNet (https://github.com/libpronet/libpronet)
 */

#include "rtp_msg_c2s.h"
#include "rtp_base.h"
#include "rtp_msg.h"
#include "rtp_msg_client.h"
#include "rtp_msg_command.h"
#include "rtp_msg_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_functor_command.h"
#include "../pro_util/pro_functor_command_task.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_PENDING_COUNT         (PRO_ACCEPTOR_LENGTH + PRO_SERVICER_LENGTH)
#define DEFAULT_REDLINE_BYTES_SRV (1024 * 1024 * 8)
#define DEFAULT_REDLINE_BYTES_USR (1024 * 1024)
#define HEARTBEAT_INTERVAL        1
#define RECONNECT_INTERVAL        5
#define DEFAULT_TIMEOUT           20

static const RTP_MSG_USER  ROOT_ID_C2S(1, 1, 65535);        /* 1-1-65535 */
static const unsigned char SERVER_CID    = 1;               /* 1-... */
static const uint64_t      NODE_UID_MIN  = 1;               /* 1 ~ 0xFFFFFFFFFF */
static const uint64_t      NODE_UID_MAXX = 0xFFFFFFFFFFULL; /* 1 ~ 0xFFFFFFFFFF */

typedef void (CRtpMsgC2s::* ACTION)(int64_t*);

/////////////////////////////////////////////////////////////////////////////
////

CRtpMsgC2s*
CRtpMsgC2s::CreateInstance(RTP_MM_TYPE                  mmType,
                           const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig, /* = NULL */
                           const char*                  uplinkSslSni,    /* = NULL */
                           const PRO_SSL_SERVER_CONFIG* localSslConfig,  /* = NULL */
                           bool                         localSslForced)  /* = false */
{
    assert(mmType >= RTP_MMT_MSG_MIN);
    assert(mmType <= RTP_MMT_MSG_MAX);
    if (mmType < RTP_MMT_MSG_MIN || mmType > RTP_MMT_MSG_MAX)
    {
        return NULL;
    }

    return new CRtpMsgC2s(mmType, uplinkSslConfig, uplinkSslSni, localSslConfig, localSslForced);
}

CRtpMsgC2s::CRtpMsgC2s(RTP_MM_TYPE                  mmType,
                       const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig, /* = NULL */
                       const char*                  uplinkSslSni,    /* = NULL */
                       const PRO_SSL_SERVER_CONFIG* localSslConfig,  /* = NULL */
                       bool                         localSslForced)  /* = false */
:
m_mmType(mmType),
m_uplinkSslConfig(uplinkSslConfig),
m_uplinkSslSni(uplinkSslSni != NULL ? uplinkSslSni : ""),
m_localSslConfig(localSslConfig),
m_localSslForced(localSslForced)
{
    m_observer               = NULL;
    m_reactor                = NULL;
    m_task                   = NULL;
    m_msgClient              = NULL;
    m_service                = NULL;
    m_serviceHubPort         = 0;
    m_timerId                = 0;
    m_connectTick            = 0;
    m_uplinkIp               = "";
    m_uplinkPort             = 0;
    m_uplinkPassword         = "";
    m_uplinkLocalIp          = "";
    m_uplinkTimeoutInSeconds = DEFAULT_TIMEOUT;
    m_uplinkRedlineBytes     = DEFAULT_REDLINE_BYTES_SRV;
    m_localTimeoutInSeconds  = DEFAULT_TIMEOUT;
    m_localRedlineBytes      = DEFAULT_REDLINE_BYTES_USR;
}

CRtpMsgC2s::~CRtpMsgC2s()
{
    Fini();
}

bool
CRtpMsgC2s::Init(IRtpMsgC2sObserver* observer,
                 IProReactor*        reactor,
                 const char*         uplinkIp,
                 unsigned short      uplinkPort,
                 const RTP_MSG_USER* uplinkUser,
                 const char*         uplinkPassword,
                 const char*         uplinkLocalIp,          /* = NULL */
                 unsigned int        uplinkTimeoutInSeconds, /* = 0 */
                 unsigned short      localServiceHubPort,
                 unsigned int        localTimeoutInSeconds)  /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(uplinkIp != NULL);
    assert(uplinkIp[0] != '\0');
    assert(uplinkPort > 0);
    assert(uplinkUser != NULL);
    assert(uplinkUser->classId > 0);
    assert(uplinkUser->UserId() <= NODE_UID_MAXX);
    assert(!uplinkUser->IsRoot());
    assert(localServiceHubPort > 0);
    if (observer == NULL || reactor == NULL || uplinkIp == NULL || uplinkIp[0] == '\0' ||
        uplinkPort == 0 || uplinkUser == NULL || uplinkUser->classId == 0 ||
        uplinkUser->UserId() > NODE_UID_MAXX || uplinkUser->IsRoot() || localServiceHubPort == 0)
    {
        return false;
    }

    char uplinkIpByDNS[64] = "";

    /*
     * DNS, for reconnecting
     */
    {
        uint32_t uplinkIp2 = pbsd_inet_aton(uplinkIp);
        if (uplinkIp2 == (uint32_t)-1 || uplinkIp2 == 0)
        {
            return false;
        }

        pbsd_inet_ntoa(uplinkIp2, uplinkIpByDNS);
    }

    if (uplinkTimeoutInSeconds == 0)
    {
        uplinkTimeoutInSeconds = DEFAULT_TIMEOUT;
    }
    if (localTimeoutInSeconds == 0)
    {
        localTimeoutInSeconds  = DEFAULT_TIMEOUT;
    }

    CProFunctorCommandTask* task      = NULL;
    CRtpMsgClient*          msgClient = NULL;
    IRtpService*            service   = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_task == NULL);
        assert(m_msgClient == NULL);
        assert(m_service == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_task != NULL || m_msgClient != NULL ||
            m_service != NULL)
        {
            return false;
        }

        task = new CProFunctorCommandTask;
        if (!task->Start())
        {
            goto EXIT;
        }

        msgClient = CRtpMsgClient::CreateInstance(
            true, m_mmType, m_uplinkSslConfig, m_uplinkSslSni.c_str());
        if (msgClient == NULL)
        {
            goto EXIT;
        }
        if (!msgClient->Init(
            this,
            reactor,
            uplinkIpByDNS,
            uplinkPort,
            uplinkUser,
            uplinkPassword,
            uplinkLocalIp,
            uplinkTimeoutInSeconds
            ))
        {
            goto EXIT;
        }
        else
        {
            msgClient->SetOutputRedline(m_uplinkRedlineBytes);
        }

        service = CreateRtpService(m_localSslConfig, this, reactor, m_mmType,
            localServiceHubPort, localTimeoutInSeconds);
        if (service == NULL)
        {
            goto EXIT;
        }

        observer->AddRef();
        m_observer               = observer;
        m_reactor                = reactor;
        m_task                   = task;
        m_msgClient              = msgClient;
        m_service                = service;
        m_serviceHubPort         = localServiceHubPort;
        m_timerId                = reactor->ScheduleTimer(this, HEARTBEAT_INTERVAL * 1000, true);
        m_connectTick            = ProGetTickCount64();
        m_uplinkIp               = uplinkIpByDNS;
        m_uplinkPort             = uplinkPort;
        m_uplinkUser             = *uplinkUser;
        m_uplinkPassword         = uplinkPassword != NULL ? uplinkPassword : "";
        m_uplinkLocalIp          = uplinkLocalIp  != NULL ? uplinkLocalIp  : "";
        m_uplinkTimeoutInSeconds = uplinkTimeoutInSeconds;
        m_localTimeoutInSeconds  = localTimeoutInSeconds;
        m_myUserBak              = *uplinkUser;
    }

    return true;

EXIT:

    DeleteRtpService(service);

    if (msgClient != NULL)
    {
        msgClient->Fini();
        msgClient->Release();
    }

    delete task;

    return false;
}

void
CRtpMsgC2s::Fini()
{
    IRtpMsgC2sObserver*                    observer  = NULL;
    CProFunctorCommandTask*                task      = NULL;
    CRtpMsgClient*                         msgClient = NULL;
    IRtpService*                           service   = NULL;
    CProStlMap<IRtpSession*, RTP_MSG_USER> session2User;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        if (!m_uplinkPassword.empty())
        {
            ProZeroMemory(&m_uplinkPassword[0], m_uplinkPassword.length());
            m_uplinkPassword = "";
        }

        auto itr = m_timerId2Info.begin();
        auto end = m_timerId2Info.end();

        for (; itr != end; ++itr)
        {
            uint64_t                            timerId = itr->first;
            const RTP_MSG_AsyncOnAcceptSession& info    = itr->second;

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }

        m_timerId2Info.clear();
        session2User = m_session2User;
        m_session2User.clear();
        m_user2Session.clear();

        service = m_service;
        m_service = NULL;
        msgClient = m_msgClient;
        m_msgClient = NULL;
        task = m_task;
        m_task = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    auto itr = session2User.begin();
    auto end = session2User.end();

    for (; itr != end; ++itr)
    {
        DeleteRtpSessionWrapper(itr->first);
    }

    DeleteRtpService(service);

    if (msgClient != NULL)
    {
        msgClient->Fini();
        msgClient->Release();
    }

    delete task;
    observer->Release();
}

unsigned long
CRtpMsgC2s::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CRtpMsgC2s::Release()
{
    return CProRefCount::Release();
}

void
CRtpMsgC2s::GetUplinkUser(RTP_MSG_USER* myUser) const
{
    assert(myUser != NULL);
    if (myUser == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *myUser = m_myUserBak;
    }
}

PRO_SSL_SUITE_ID
CRtpMsgC2s::GetUplinkSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_msgClient != NULL)
        {
            suiteId = m_msgClient->GetSslSuite(suiteName);
        }
    }

    return suiteId;
}

const char*
CRtpMsgC2s::GetUplinkLocalIp(char localIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_msgClient != NULL)
        {
            m_msgClient->GetLocalIp(localIp);
        }
        else
        {
            strncpy_pro(localIp, 64, m_uplinkLocalIp.c_str());
        }
    }

    return localIp;
}

unsigned short
CRtpMsgC2s::GetUplinkLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_msgClient != NULL)
        {
            localPort = m_msgClient->GetLocalPort();
        }
    }

    return localPort;
}

const char*
CRtpMsgC2s::GetUplinkRemoteIp(char remoteIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        strncpy_pro(remoteIp, 64, m_uplinkIp.c_str());
    }

    return remoteIp;
}

unsigned short
CRtpMsgC2s::GetUplinkRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        remotePort = m_uplinkPort;
    }

    return remotePort;
}

void
CRtpMsgC2s::SetUplinkOutputRedline(size_t redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_uplinkRedlineBytes = redlineBytes;
        if (m_msgClient != NULL)
        {
            m_msgClient->SetOutputRedline(redlineBytes);
        }
    }
}

size_t
CRtpMsgC2s::GetUplinkOutputRedline() const
{
    size_t redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        redlineBytes = m_uplinkRedlineBytes;
    }

    return redlineBytes;
}

size_t
CRtpMsgC2s::GetUplinkSendingBytes() const
{
    size_t sendingBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_msgClient != NULL)
        {
            sendingBytes = m_msgClient->GetSendingBytes();
        }
    }

    return sendingBytes;
}

unsigned short
CRtpMsgC2s::GetLocalServicePort() const
{
    unsigned short servicePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        servicePort = m_serviceHubPort;
    }

    return servicePort;
}

PRO_SSL_SUITE_ID
CRtpMsgC2s::GetLocalSslSuite(const RTP_MSG_USER* user,
                             char                suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    assert(user != NULL);
    if (user == NULL)
    {
        return suiteId;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        auto itr = m_user2Session.find(*user);
        if (itr != m_user2Session.end())
        {
            suiteId = itr->second->GetSslSuite(suiteName);
        }
    }

    return suiteId;
}

void
CRtpMsgC2s::GetLocalUserCount(size_t* pendingUserCount, /* = NULL */
                              size_t* userCount) const  /* = NULL */
{
    CProThreadMutexGuard mon(m_lock);

    if (pendingUserCount != NULL)
    {
        *pendingUserCount = m_timerId2Info.size();
    }
    if (userCount != NULL)
    {
        *userCount        = m_user2Session.size();
    }
}

void
CRtpMsgC2s::KickoutLocalUser(const RTP_MSG_USER* user)
{
    assert(user != NULL);
    if (user == NULL || user->classId == 0 || user->UserId() == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        IProFunctorCommand* command =
            CProFunctorCommand_cpp<CRtpMsgC2s, ACTION>::CreateInstance(
                *this,
                &CRtpMsgC2s::AsyncKickoutLocalUser,
                (int64_t)user->classId,
                (int64_t)user->UserId(),
                (int64_t)user->instId
                );
        m_task->Put(command);
    }
}

void
CRtpMsgC2s::AsyncKickoutLocalUser(int64_t* args)
{
    RTP_MSG_USER user((unsigned char)args[0], args[1], (uint16_t)args[2]);
    assert(user.classId > 0);
    assert(user.UserId() > 0);

    IRtpMsgC2sObserver* observer   = NULL;
    IRtpSession*        oldSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_user2Session.find(user);
        if (itr == m_user2Session.end())
        {
            return;
        }

        oldSession = itr->second;
        m_session2User.erase(oldSession);
        m_user2Session.erase(itr);

        if (m_msgClient != NULL)
        {
            ReportLogout(m_msgClient, user);
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnCloseUser(this, &user, -1, 0);
    observer->Release();
    DeleteRtpSessionWrapper(oldSession);
}

void
CRtpMsgC2s::SetLocalOutputRedline(size_t redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_localRedlineBytes = redlineBytes;
    }
}

size_t
CRtpMsgC2s::GetLocalOutputRedline() const
{
    size_t redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        redlineBytes = m_localRedlineBytes;
    }

    return redlineBytes;
}

size_t
CRtpMsgC2s::GetLocalSendingBytes(const RTP_MSG_USER* user) const
{
    assert(user != NULL);
    if (user == NULL)
    {
        return 0;
    }

    size_t sendingBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        auto itr = m_user2Session.find(*user);
        if (itr != m_user2Session.end())
        {
            itr->second->GetFlowctrlInfo(NULL, NULL, NULL, NULL, &sendingBytes, NULL);
        }
    }

    return sendingBytes;
}

void
CRtpMsgC2s::OnAcceptSession(IRtpService*            service,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const RTP_SESSION_INFO* remoteInfo,
                            const PRO_NONCE*        nonce)
{
    assert(service != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    assert(nonce != NULL);
    if (service == NULL || sockId == -1 || remoteIp == NULL || remoteInfo == NULL || nonce == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_TCPCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    assert(remoteInfo->packMode == RTP_MSG_PACK_MODE);
    if (remoteInfo->sessionType != RTP_ST_TCPCLIENT_EX || remoteInfo->mmType != m_mmType ||
        remoteInfo->packMode != RTP_MSG_PACK_MODE)
    {
        ProCloseSockId(sockId);

        return;
    }

    AcceptSession(service, NULL, sockId, unixSocket, remoteIp, remotePort, *remoteInfo, *nonce);
}

void
CRtpMsgC2s::OnAcceptSession(IRtpService*            service,
                            PRO_SSL_CTX*            sslCtx,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const RTP_SESSION_INFO* remoteInfo,
                            const PRO_NONCE*        nonce)
{
    assert(service != NULL);
    assert(sslCtx != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    assert(nonce != NULL);
    if (service == NULL || sslCtx == NULL || sockId == -1 || remoteIp == NULL ||
        remoteInfo == NULL || nonce == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_SSLCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    assert(remoteInfo->packMode == RTP_MSG_PACK_MODE);
    if (remoteInfo->sessionType != RTP_ST_SSLCLIENT_EX || remoteInfo->mmType != m_mmType ||
        remoteInfo->packMode != RTP_MSG_PACK_MODE)
    {
        ProSslCtx_Delete(sslCtx);
        ProCloseSockId(sockId);

        return;
    }

    AcceptSession(service, sslCtx, sockId, unixSocket, remoteIp, remotePort, *remoteInfo, *nonce);
}

void
CRtpMsgC2s::AcceptSession(IRtpService*            service,
                          PRO_SSL_CTX*            sslCtx, /* = NULL */
                          int64_t                 sockId,
                          bool                    unixSocket,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const RTP_SESSION_INFO& remoteInfo,
                          const PRO_NONCE&        nonce)
{
    assert(service != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(
        remoteInfo.sessionType == RTP_ST_TCPCLIENT_EX ||
        remoteInfo.sessionType == RTP_ST_SSLCLIENT_EX
        );
    assert(remoteInfo.mmType == m_mmType);
    assert(remoteInfo.packMode == RTP_MSG_PACK_MODE);

    RTP_MSG_HEADER0 hdr0;
    RTP_MSG_USER    user;

    if (m_localSslConfig == NULL)
    {
        if (remoteInfo.sessionType != RTP_ST_TCPCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else if (m_localSslForced)
    {
        if (remoteInfo.sessionType != RTP_ST_SSLCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else
    {
    }

    memcpy(&hdr0, remoteInfo.userData, sizeof(RTP_MSG_HEADER0));
    hdr0.version = pbsd_ntoh16(hdr0.version);

    user        = hdr0.user;
    user.instId = pbsd_ntoh16(user.instId);

    assert(user.classId > 0);
    assert(user.UserId() <= NODE_UID_MAXX);
    assert(!user.IsRoot());
    if (user.classId == 0 || user.UserId() > NODE_UID_MAXX || user.IsRoot())
    {
        goto EXIT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            goto EXIT;
        }

        if (service != m_service)
        {
            goto EXIT;
        }

        if (m_timerId2Info.size() >= MAX_PENDING_COUNT)
        {
            goto EXIT;
        }

        if (m_msgClient == NULL ||
            m_myUserNow.classId == 0 || m_myUserNow.UserId() == 0 || user == m_myUserNow)
        {
            goto EXIT;
        }

        char idString[64] = "";
        RtpMsgUser2String(&user, idString);

        char hashString[64 + 1] = "";
        hashString[64] = '\0';

        for (int i = 0; i < 32; ++i)
        {
            sprintf(hashString + i * 2, "%02x", (unsigned int)remoteInfo.passwordHash[i]);
        }

        char nonceString[64 + 1] = "";
        nonceString[64] = '\0';

        for (int j = 0; j < 32; ++j)
        {
            sprintf(nonceString + j * 2, "%02x", (unsigned int)nonce.nonce[j]);
        }

        uint64_t timerId = m_reactor->ScheduleTimer(
            this, (uint64_t)m_localTimeoutInSeconds * 1000, false);

        CProConfigStream msgStream;
        msgStream.Add      (TAG_msg_name           , MSG_client_login);
        msgStream.AddUint64(TAG_client_index       , timerId);
        msgStream.Add      (TAG_client_id          , idString);
        msgStream.Add      (TAG_client_public_ip   , remoteIp);
        msgStream.Add      (TAG_client_hash_string , hashString);
        msgStream.Add      (TAG_client_nonce_string, nonceString);

        CProStlString theString;
        msgStream.ToString(theString);

        if (!m_msgClient->SendMsg(theString.c_str(), theString.length(), 0, &ROOT_ID_C2S, 1))
        {
            m_reactor->CancelTimer(timerId);

            goto EXIT;
        }

        RTP_MSG_AsyncOnAcceptSession info;
        info.sslCtx     = sslCtx;
        info.sockId     = sockId;
        info.unixSocket = unixSocket;
        info.remoteIp   = remoteIp;
        info.remotePort = remotePort;
        info.remoteInfo = remoteInfo;
        info.nonce      = nonce;

        m_timerId2Info[timerId] = info;
    }

    return;

EXIT:

    ProSslCtx_Delete(sslCtx);
    ProCloseSockId(sockId);
}

void
CRtpMsgC2s::OnRecvSession(IRtpSession* session,
                          IRtpPacket*  packet)
{
    assert(session != NULL);
    assert(packet != NULL);
    assert(packet->GetPayloadSize() > sizeof(RTP_MSG_HEADER));
    if (session == NULL || packet == NULL || packet->GetPayloadSize() <= sizeof(RTP_MSG_HEADER))
    {
        return;
    }

    RTP_MSG_HEADER* msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    if (msgHeaderPtr->dstUserCount == 0)
    {
        msgHeaderPtr->dstUserCount = 1;
    }

    size_t msgHeaderSize =
        sizeof(RTP_MSG_HEADER) + sizeof(RTP_MSG_USER) * (msgHeaderPtr->dstUserCount - 1);

    const void* msgBodyPtr  = (char*)msgHeaderPtr + msgHeaderSize;
    int         msgBodySize = (int)(packet->GetPayloadSize() - msgHeaderSize);
    uint16_t    charset     = pbsd_ntoh16(msgHeaderPtr->charset);

    RTP_MSG_USER srcUser = msgHeaderPtr->srcUser;
    srcUser.instId       = pbsd_ntoh16(srcUser.instId);

    assert(msgBodySize > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (msgBodySize == 0 || srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return;
    }

    CRtpMsgClient* msgClient       = NULL;
    unsigned char  sessionCount    = 0;
    IRtpSession*   sessions[255];
    unsigned char  uplinkUserCount = 0;
    RTP_MSG_USER   uplinkUsers[255];

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_session2User.find(session);
        if (itr == m_session2User.end() || srcUser != itr->second)
        {
            return;
        }

        for (int i = 0; i < (int)msgHeaderPtr->dstUserCount; ++i)
        {
            RTP_MSG_USER dstUser = msgHeaderPtr->dstUsers[i];
            dstUser.instId       = pbsd_ntoh16(dstUser.instId);

            if (dstUser.classId == 0 || dstUser.UserId() == 0)
            {
                continue;
            }

            auto itr2 = m_user2Session.find(dstUser);
            if (itr2 != m_user2Session.end())
            {
                itr2->second->AddRef();
                sessions[sessionCount]       = itr2->second;
                ++sessionCount;
            }
            else if (dstUser != m_myUserNow)
            {
                uplinkUsers[uplinkUserCount] = dstUser;
                ++uplinkUserCount;
            }
            else
            {
            }
        }

        if (uplinkUserCount > 0 && m_msgClient != NULL)
        {
            m_msgClient->AddRef();
            msgClient = m_msgClient;
        }
    }

    for (int i = 0; i < (int)sessionCount; ++i)
    {
        SendMsgToDownlink(m_mmType, sessions + i, 1, msgBodyPtr, msgBodySize, charset, srcUser);
        sessions[i]->Release();
    }

    if (msgClient != NULL)
    {
        msgClient->TransferMsg(msgBodyPtr, msgBodySize, charset,
            uplinkUsers, uplinkUserCount, srcUser);
        msgClient->Release();
    }
}

void
CRtpMsgC2s::OnCloseSession(IRtpSession* session,
                           int          errorCode,
                           int          sslCode,
                           bool         tcpConnected)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;
    RTP_MSG_USER        user;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_session2User.find(session);
        if (itr == m_session2User.end())
        {
            return;
        }

        user = itr->second;
        m_session2User.erase(itr);
        m_user2Session.erase(user);

        if (m_msgClient != NULL)
        {
            ReportLogout(m_msgClient, user);
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnCloseUser(this, &user, errorCode, sslCode);
    observer->Release();
    DeleteRtpSessionWrapper(session);
}

void
CRtpMsgC2s::OnHeartbeatSession(IRtpSession* session,
                               int64_t      peerAliveTick)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;
    RTP_MSG_USER        user;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_session2User.find(session);
        if (itr == m_session2User.end())
        {
            return;
        }

        user = itr->second;

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnHeartbeatUser(this, &user, peerAliveTick);
    observer->Release();
}

void
CRtpMsgC2s::OnOkMsg(IRtpMsgClient*      msgClient,
                    const RTP_MSG_USER* myUser,
                    const char*         myPublicIp)
{
    assert(msgClient != NULL);
    assert(myUser != NULL);
    assert(myUser->classId > 0);
    assert(myUser->UserId() > 0);
    assert(myPublicIp != NULL);
    assert(myPublicIp[0] != '\0');
    if (msgClient == NULL || myUser == NULL || myUser->classId == 0 || myUser->UserId() == 0 ||
        myPublicIp == NULL || myPublicIp[0] == '\0')
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        m_myUserNow = *myUser; /* login */
        m_myUserBak = *myUser; /* login */

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnOkC2s(this, myUser, myPublicIp);
    observer->Release();
}

void
CRtpMsgC2s::OnRecvMsg(IRtpMsgClient*      msgClient,
                      const void*         buf,
                      size_t              size,
                      uint16_t            charset,
                      const RTP_MSG_USER* srcUser)
{
    assert(msgClient != NULL);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser != NULL);
    assert(srcUser->classId > 0);
    assert(srcUser->UserId() > 0);
    if (msgClient == NULL || buf == NULL || size == 0 ||
        srcUser == NULL || srcUser->classId == 0 || srcUser->UserId() == 0)
    {
        return;
    }

    if (*srcUser != ROOT_ID_C2S)
    {
        return;
    }

    CProStlString                  theString((char*)buf, size);
    CProStlVector<PRO_CONFIG_ITEM> theConfigs;

    if (!CProConfigStream::StringToConfigs(theString, theConfigs))
    {
        return;
    }

    CProConfigStream msgStream;
    msgStream.Add(theConfigs);

    CProStlString msgName;
    msgStream.Get(TAG_msg_name, msgName);

    if (stricmp(msgName.c_str(), MSG_client_login_ok) == 0)
    {
        ProcessMsg_client_login_ok(msgClient, msgStream);
    }
    else if (stricmp(msgName.c_str(), MSG_client_login_error) == 0)
    {
        ProcessMsg_client_login_error(msgClient, msgStream);
    }
    else if (stricmp(msgName.c_str(), MSG_client_kickout) == 0)
    {
        ProcessMsg_client_kickout(msgClient, msgStream);
    }
    else
    {
    }
}

void
CRtpMsgC2s::ProcessMsg_client_login_ok(IRtpMsgClient*          msgClient,
                                       const CProConfigStream& msgStream)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    uint64_t      client_index;
    CProStlString client_id;

    msgStream.GetUint64(TAG_client_index, client_index);
    msgStream.Get      (TAG_client_id   , client_id);

    RTP_MSG_USER user;
    RtpMsgString2User(client_id.c_str(), &user);

    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    IRtpMsgC2sObserver*          observer   = NULL;
    IRtpSession*                 newSession = NULL;
    IRtpSession*                 oldSession = NULL;
    RTP_MSG_AsyncOnAcceptSession acceptedInfo;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        auto itr = m_timerId2Info.find(client_index);
        if (itr != m_timerId2Info.end())
        {
            acceptedInfo = itr->second;
            m_timerId2Info.erase(itr);

            m_reactor->CancelTimer(client_index);

            RTP_MSG_HEADER0 hdr0;
            memset(&hdr0, 0, sizeof(RTP_MSG_HEADER0));
            hdr0.version     = pbsd_hton16(RTP_MSG_PROTOCOL_VERSION);
            hdr0.user        = user;
            hdr0.user.instId = pbsd_hton16(hdr0.user.instId);
            hdr0.publicIp    = pbsd_inet_aton(acceptedInfo.remoteIp.c_str());

            RTP_SESSION_INFO localInfo;
            memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
            localInfo.remoteVersion = acceptedInfo.remoteInfo.localVersion;
            localInfo.mmType        = m_mmType;
            localInfo.packMode      = acceptedInfo.remoteInfo.packMode;

            RTP_INIT_ARGS initArgs;
            memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));

            if (acceptedInfo.remoteInfo.sessionType == RTP_ST_SSLCLIENT_EX)
            {
                initArgs.sslserverEx.observer    = this;
                initArgs.sslserverEx.reactor     = m_reactor;
                initArgs.sslserverEx.sslCtx      = acceptedInfo.sslCtx;
                initArgs.sslserverEx.sockId      = acceptedInfo.sockId;
                initArgs.sslserverEx.unixSocket  = acceptedInfo.unixSocket;
                initArgs.sslserverEx.suspendRecv = true; /* !!! */
                initArgs.sslserverEx.useAckData  = true;
                memcpy(initArgs.sslserverEx.ackData, &hdr0, sizeof(RTP_MSG_HEADER0));

                newSession = CreateRtpSessionWrapper(RTP_ST_SSLSERVER_EX, &initArgs, &localInfo);
            }
            else
            {
                initArgs.tcpserverEx.observer    = this;
                initArgs.tcpserverEx.reactor     = m_reactor;
                initArgs.tcpserverEx.sockId      = acceptedInfo.sockId;
                initArgs.tcpserverEx.unixSocket  = acceptedInfo.unixSocket;
                initArgs.tcpserverEx.suspendRecv = true; /* !!! */
                initArgs.tcpserverEx.useAckData  = true;
                memcpy(initArgs.tcpserverEx.ackData, &hdr0, sizeof(RTP_MSG_HEADER0));

                newSession = CreateRtpSessionWrapper(RTP_ST_TCPSERVER_EX, &initArgs, &localInfo);
            }

            if (newSession == NULL)
            {
                ProSslCtx_Delete(acceptedInfo.sslCtx);
                ProCloseSockId(acceptedInfo.sockId);
            }
            else
            {
                newSession->SetOutputRedline(m_localRedlineBytes, 0, 0);

                auto itr2 = m_user2Session.find(user);
                if (itr2 != m_user2Session.end())
                {
                    oldSession = itr2->second;
                    m_session2User.erase(oldSession);
                    m_user2Session.erase(itr2);
                }

                m_session2User[newSession] = user;
                m_user2Session[user]       = newSession;
            }
        }

        if (newSession != NULL)
        {
            newSession->AddRef();
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (oldSession != NULL)
    {
        observer->OnCloseUser(this, &user, -1, 0);
    }

    if (newSession != NULL)
    {
        observer->OnOkUser(this, &user, acceptedInfo.remoteIp.c_str());

        newSession->ResumeRecv(); /* !!! */
        newSession->Release();
    }
    else
    {
        ReportLogout(msgClient, user);
    }

    observer->Release();
    DeleteRtpSessionWrapper(oldSession);
}

void
CRtpMsgC2s::ProcessMsg_client_login_error(IRtpMsgClient*          msgClient,
                                          const CProConfigStream& msgStream)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    uint64_t client_index = 0;
    msgStream.GetUint64(TAG_client_index, client_index);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        auto itr = m_timerId2Info.find(client_index);
        if (itr == m_timerId2Info.end())
        {
            return;
        }

        RTP_MSG_AsyncOnAcceptSession info = itr->second;
        m_timerId2Info.erase(itr);

        m_reactor->CancelTimer(client_index);
        ProSslCtx_Delete(info.sslCtx);
        ProCloseSockId(info.sockId);
    }
}

void
CRtpMsgC2s::ProcessMsg_client_kickout(IRtpMsgClient*          msgClient,
                                      const CProConfigStream& msgStream)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    CProStlString client_id;
    msgStream.Get(TAG_client_id, client_id);

    RTP_MSG_USER user;
    RtpMsgString2User(client_id.c_str(), &user);

    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    IRtpMsgC2sObserver* observer   = NULL;
    IRtpSession*        oldSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        auto itr = m_user2Session.find(user);
        if (itr == m_user2Session.end())
        {
            return;
        }

        oldSession = itr->second;
        m_session2User.erase(oldSession);
        m_user2Session.erase(itr);

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnCloseUser(this, &user, -1, 0);
    observer->Release();
    DeleteRtpSessionWrapper(oldSession);
}

void
CRtpMsgC2s::OnTransferMsg(IRtpMsgClient*      msgClient,
                          const void*         buf,
                          size_t              size,
                          uint16_t            charset,
                          const RTP_MSG_USER& srcUser,
                          const RTP_MSG_USER* dstUsers,
                          unsigned char       dstUserCount)
{
    assert(msgClient != NULL);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    if (msgClient == NULL || buf == NULL || size == 0 || srcUser.classId == 0 ||
        srcUser.UserId() == 0 || dstUsers == NULL || dstUserCount == 0)
    {
        return;
    }

    unsigned char sessionCount = 0;
    IRtpSession*  sessions[255];

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            auto itr = m_user2Session.find(dstUsers[i]);
            if (itr != m_user2Session.end())
            {
                itr->second->AddRef();
                sessions[sessionCount] = itr->second;
                ++sessionCount;
            }
        }
    }

    for (int i = 0; i < (int)sessionCount; ++i)
    {
        SendMsgToDownlink(m_mmType, sessions + i, 1, buf, size, charset, srcUser);
        sessions[i]->Release();
    }
}

void
CRtpMsgC2s::OnCloseMsg(IRtpMsgClient* msgClient,
                       int            errorCode,
                       int            sslCode,
                       bool           tcpConnected)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver*                    observer = NULL;
    CProStlMap<IRtpSession*, RTP_MSG_USER> session2User;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        auto itr = m_timerId2Info.begin();
        auto end = m_timerId2Info.end();

        for (; itr != end; ++itr)
        {
            uint64_t                            timerId = itr->first;
            const RTP_MSG_AsyncOnAcceptSession& info    = itr->second;

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }

        m_timerId2Info.clear();
        session2User = m_session2User;
        m_session2User.clear();
        m_user2Session.clear();

        m_myUserNow.Zero();         /* logout */
        m_myUserBak = m_uplinkUser; /* logout */
        m_msgClient = NULL;         /* logout */

        m_observer->AddRef();
        observer = m_observer;
    }

    auto itr = session2User.begin();
    auto end = session2User.end();

    for (; itr != end; ++itr)
    {
        DeleteRtpSessionWrapper(itr->first);
    }

    observer->OnCloseC2s(this, errorCode, sslCode, tcpConnected);
    observer->Release();

    CRtpMsgClient* msgClient2 = (CRtpMsgClient*)msgClient;
    msgClient2->Fini();
    msgClient2->Release();
}

void
CRtpMsgC2s::OnHeartbeatMsg(IRtpMsgClient* msgClient,
                           int64_t        peerAliveTick)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnHeartbeatC2s(this, peerAliveTick);
    observer->Release();
}

void
CRtpMsgC2s::OnTimer(void*    factory,
                    uint64_t timerId,
                    int64_t  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        if (timerId == m_timerId)
        {
            const int64_t tick = ProGetTickCount64();

            if (m_msgClient == NULL && tick - m_connectTick >= RECONNECT_INTERVAL * 1000)
            {
                m_connectTick = tick;

                m_msgClient = CRtpMsgClient::CreateInstance(
                    true, m_mmType, m_uplinkSslConfig, m_uplinkSslSni.c_str());
                if (m_msgClient != NULL)
                {
                    if (!m_msgClient->Init(
                        this,
                        m_reactor,
                        m_uplinkIp.c_str(),
                        m_uplinkPort,
                        &m_uplinkUser,
                        m_uplinkPassword.c_str(),
                        m_uplinkLocalIp.c_str(),
                        m_uplinkTimeoutInSeconds
                        ))
                    {
                        m_msgClient->Release();
                        m_msgClient = NULL;
                    }
                    else
                    {
                        m_msgClient->SetOutputRedline(m_uplinkRedlineBytes);
                    }
                }
            }

            return;
        }

        auto itr = m_timerId2Info.find(timerId);
        if (itr != m_timerId2Info.end())
        {
            RTP_MSG_AsyncOnAcceptSession info = itr->second;
            m_timerId2Info.erase(itr);

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }
    }
}

bool
CRtpMsgC2s::SendMsgToDownlink(RTP_MM_TYPE         mmType,
                              IRtpSession**       sessions,
                              unsigned char       sessionCount,
                              const void*         buf,
                              size_t              size,
                              uint16_t            charset,
                              const RTP_MSG_USER& srcUser)
{
    assert(sessions != NULL);
    assert(sessionCount > 0);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (sessions == NULL || sessionCount == 0 || buf == NULL || size == 0 ||
        srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return false;
    }

    size_t msgHeaderSize = sizeof(RTP_MSG_HEADER);

    IRtpPacket* packet = CreateRtpPacketSpace(msgHeaderSize + size, RTP_MSG_PACK_MODE);
    if (packet == NULL)
    {
        return false;
    }

    RTP_MSG_HEADER* msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    memset(msgHeaderPtr, 0, msgHeaderSize);

    msgHeaderPtr->charset        = pbsd_hton16(charset);
    msgHeaderPtr->srcUser        = srcUser;
    msgHeaderPtr->srcUser.instId = pbsd_hton16(msgHeaderPtr->srcUser.instId);

    memcpy((char*)msgHeaderPtr + msgHeaderSize, buf, size);

    packet->SetMmType(mmType);

    bool ret = true;

    for (int i = 0; i < (int)sessionCount; ++i)
    {
        assert(sessions[i] != NULL);
        if (!sessions[i]->SendPacket(packet))
        {
            ret = false;
        }
    }

    packet->Release();

    return ret;
}

void
CRtpMsgC2s::ReportLogout(IRtpMsgClient*      msgClient,
                         const RTP_MSG_USER& user)
{
    assert(msgClient != NULL);
    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (msgClient == NULL || user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    char idString[64] = "";
    RtpMsgUser2String(&user, idString);

    CProConfigStream msgStream;
    msgStream.Add(TAG_msg_name , MSG_client_logout);
    msgStream.Add(TAG_client_id, idString);

    CProStlString theString;
    msgStream.ToString(theString);

    msgClient->SendMsg(theString.c_str(), theString.length(), 0, &ROOT_ID_C2SPORT, 1);
}
