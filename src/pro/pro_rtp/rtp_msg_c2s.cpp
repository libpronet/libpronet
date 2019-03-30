/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (http://www.libpro.org)
 */

#include "rtp_msg_c2s.h"
#include "rtp_foundation.h"
#include "rtp_framework.h"
#include "rtp_msg_client.h"
#include "rtp_msg_command.h"
#include "rtp_msg_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_functor_command.h"
#include "../pro_util/pro_functor_command_task.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_PENDING_COUNT     5000
#define DEFAULT_REDLINE_BYTES (1024 * 1024 * 8)
#define HEARTBEAT_INTERVAL    1
#define RECONNECT_INTERVAL    10
#define DEFAULT_TIMEOUT       20

static const RTP_MSG_USER  ROOT_ID_C2S(1, 1, 65535);                             /* 1-1-65535 */
static const unsigned char SERVER_CID   = 1;                                     /* 1-... */
static const PRO_UINT64    NODE_UID_MIN = 1;                                     /* 1 ~ 0xEFFFFFFFFF */
static const PRO_UINT64    NODE_UID_MAX = ((PRO_UINT64)0xEF << 32) | 0xFFFFFFFF; /* 1 ~ 0xEFFFFFFFFF */

typedef void (CRtpMsgC2s::* ACTION)(PRO_INT64*);

/////////////////////////////////////////////////////////////////////////////
////

CRtpMsgC2s*
CRtpMsgC2s::CreateInstance(RTP_MM_TYPE                  mmType,
                           const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,      /* = NULL */
                           const char*                  uplinkSslServiceName, /* = NULL */
                           const PRO_SSL_SERVER_CONFIG* localSslConfig,       /* = NULL */
                           bool                         localSslForced)       /* = false */
{
    assert(mmType != 0);
    if (mmType == 0)
    {
        return (NULL);
    }

    CRtpMsgC2s* const msgC2s = new CRtpMsgC2s(
        mmType, uplinkSslConfig, uplinkSslServiceName, localSslConfig, localSslForced);

    return (msgC2s);
}

CRtpMsgC2s::CRtpMsgC2s(RTP_MM_TYPE                  mmType,
                       const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,      /* = NULL */
                       const char*                  uplinkSslServiceName, /* = NULL */
                       const PRO_SSL_SERVER_CONFIG* localSslConfig,       /* = NULL */
                       bool                         localSslForced)       /* = false */
                       :
m_mmType(mmType),
m_uplinkSslConfig(uplinkSslConfig),
m_uplinkSslServiceName(uplinkSslServiceName != NULL ? uplinkSslServiceName : ""),
m_localSslConfig(localSslConfig),
m_localSslForced(localSslForced)
{
    m_observer               = NULL;
    m_reactor                = NULL;
    m_service                = NULL;
    m_task                   = NULL;
    m_msgClient              = NULL;
    m_timerId                = 0;
    m_connectTick            = 0;
    m_uplinkIp               = "";
    m_uplinkPort             = 0;
    m_uplinkPassword         = "";
    m_uplinkLocalIp          = "";
    m_uplinkTimeoutInSeconds = DEFAULT_TIMEOUT;
    m_localTimeoutInSeconds  = DEFAULT_TIMEOUT;
    m_redlineBytes           = DEFAULT_REDLINE_BYTES;
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
                 unsigned long       uplinkTimeoutInSeconds, /* = 0 */
                 unsigned short      localServiceHubPort,
                 unsigned long       localTimeoutInSeconds)  /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(uplinkIp != NULL);
    assert(uplinkIp[0] != '\0');
    assert(uplinkPort > 0);
    assert(uplinkUser != NULL);
    assert(uplinkUser->classId == SERVER_CID);
    assert(
        uplinkUser->UserId() == 0 ||
        uplinkUser->UserId() >= NODE_UID_MIN && uplinkUser->UserId() <= NODE_UID_MAX
        );
    assert(!uplinkUser->IsRoot());
    assert(localServiceHubPort > 0);
    if (observer == NULL || reactor == NULL ||
        uplinkIp == NULL || uplinkIp[0] == '\0' || uplinkPort == 0 ||
        uplinkUser == NULL || uplinkUser->classId != SERVER_CID
        ||
        uplinkUser->UserId() != 0 &&
        (uplinkUser->UserId() < NODE_UID_MIN || uplinkUser->UserId() > NODE_UID_MAX)
        ||
        uplinkUser->IsRoot() || localServiceHubPort == 0)
    {
        return (false);
    }

    char uplinkIpByDNS[64] = "";

    /*
     * DNS, for reconnecting
     */
    {
        const PRO_UINT32 uplinkIp2 = pbsd_inet_aton(uplinkIp);
        if (uplinkIp2 == (PRO_UINT32)-1 || uplinkIp2 == 0)
        {
            return (false);
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

    IRtpService*            service   = NULL;
    CProFunctorCommandTask* task      = NULL;
    CRtpMsgClient*          msgClient = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_service == NULL);
        assert(m_task == NULL);
        assert(m_msgClient == NULL);
        if (m_observer != NULL || m_reactor != NULL ||
            m_service != NULL || m_task != NULL || m_msgClient != NULL)
        {
            return (false);
        }

        task = new CProFunctorCommandTask;
        if (!task->Start())
        {
            goto EXIT;
        }

        service = CreateRtpService(m_localSslConfig, this, reactor,
            m_mmType, localServiceHubPort, localTimeoutInSeconds);
        if (service == NULL)
        {
            goto EXIT;
        }

        msgClient = CRtpMsgClient::CreateInstance(
            true, m_mmType, m_uplinkSslConfig, m_uplinkSslServiceName.c_str());
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

        observer->AddRef();
        m_observer               = observer;
        m_reactor                = reactor;
        m_service                = service;
        m_task                   = task;
        m_msgClient              = msgClient;
        m_timerId                = reactor->ScheduleTimer(this, HEARTBEAT_INTERVAL * 1000, true);
        m_connectTick            = ProGetTickCount64();
        m_uplinkIp               = uplinkIpByDNS;
        m_uplinkPort             = uplinkPort;
        m_uplinkUser             = *uplinkUser;
        m_uplinkPassword         = uplinkPassword != NULL ? uplinkPassword : "";
        m_uplinkLocalIp          = uplinkLocalIp  != NULL ? uplinkLocalIp  : "";
        m_uplinkTimeoutInSeconds = uplinkTimeoutInSeconds;
        m_localTimeoutInSeconds  = localTimeoutInSeconds;
        m_c2sUserBak             = *uplinkUser;
    }

    return (true);

EXIT:

    if (task != NULL)
    {
        task->Stop();
        delete task;
    }

    if (msgClient != NULL)
    {
        msgClient->Fini();
        msgClient->Release();
    }

    DeleteRtpService(service);

    return (false);
}

void
CRtpMsgC2s::Fini()
{
    IRtpMsgC2sObserver*                    observer  = NULL;
    IRtpService*                           service   = NULL;
    CProFunctorCommandTask*                task      = NULL;
    CRtpMsgClient*                         msgClient = NULL;
    CProStlMap<IRtpSession*, RTP_MSG_USER> session2User;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
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

        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::const_iterator       itr = m_timerId2Info.begin();
        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::const_iterator const end = m_timerId2Info.end();

        for (; itr != end; ++itr)
        {
            const unsigned long                 timerId = itr->first;
            const RTP_MSG_AsyncOnAcceptSession& info    = itr->second;

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }

        m_timerId2Info.clear();
        session2User = m_session2User;
        m_session2User.clear();
        m_user2Session.clear();

        msgClient = m_msgClient;
        m_msgClient = NULL;
        task = m_task;
        m_task = NULL;
        service = m_service;
        m_service = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    task->Stop();
    delete task;

    CProStlMap<IRtpSession*, RTP_MSG_USER>::const_iterator       itr = session2User.begin();
    CProStlMap<IRtpSession*, RTP_MSG_USER>::const_iterator const end = session2User.end();

    for (; itr != end; ++itr)
    {
        DeleteRtpSessionWrapper(itr->first);
    }

    if (msgClient != NULL)
    {
        msgClient->Fini();
        msgClient->Release();
    }

    DeleteRtpService(service);
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CRtpMsgC2s::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpMsgC2s::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpMsgC2s::GetC2sUser(RTP_MSG_USER* c2sUser) const
{
    assert(c2sUser != NULL);
    if (c2sUser == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *c2sUser = m_c2sUserBak;
    }
}

void
PRO_CALLTYPE
CRtpMsgC2s::GetUserCount(unsigned long* pendingUserCount, /* = NULL */
                         unsigned long* userCount) const  /* = NULL */
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (pendingUserCount != NULL)
        {
            *pendingUserCount = (unsigned long)m_timerId2Info.size();
        }
        if (userCount != NULL)
        {
            *userCount        = (unsigned long)m_user2Session.size();
        }
    }
}

void
PRO_CALLTYPE
CRtpMsgC2s::KickoutUser(const RTP_MSG_USER* user)
{
    assert(user != NULL);
    if (user == NULL || user->classId == 0 || user->UserId() == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        IProFunctorCommand* const command =
            CProFunctorCommand_cpp<CRtpMsgC2s, ACTION>::CreateInstance(
            *this,
            &CRtpMsgC2s::AsyncKickoutUser,
            (PRO_INT64)user->classId,
            (PRO_INT64)user->UserId(),
            (PRO_INT64)user->instId
            );
        m_task->Put(command);
    }
}

void
CRtpMsgC2s::AsyncKickoutUser(PRO_INT64* args)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    const RTP_MSG_USER user((unsigned char)args[0], args[1], (PRO_UINT16)args[2]);
    assert(user.classId > 0);
    assert(user.UserId() > 0);

    IRtpMsgC2sObserver* observer   = NULL;
    IRtpSession*        oldSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        CProStlMap<RTP_MSG_USER, IRtpSession*>::iterator const itr = m_user2Session.find(user);
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
}}

void
PRO_CALLTYPE
CRtpMsgC2s::SetOutputRedline(unsigned long redlineBytes)
{
    assert(redlineBytes > 0);
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        m_redlineBytes = redlineBytes;
        if (m_msgClient != NULL)
        {
            m_msgClient->SetOutputRedline(redlineBytes);
        }
    }
}

unsigned long
PRO_CALLTYPE
CRtpMsgC2s::GetOutputRedline() const
{
    unsigned long redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        redlineBytes = m_redlineBytes;
    }

    return (redlineBytes);
}

void
PRO_CALLTYPE
CRtpMsgC2s::OnAcceptSession(IRtpService*            service,
                            PRO_INT64               sockId,
                            bool                    unixSocket,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const RTP_SESSION_INFO* remoteInfo,
                            PRO_UINT64              nonce)
{
    assert(service != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    if (service == NULL || sockId == -1 || remoteIp == NULL || remoteInfo == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_TCPCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    if (remoteInfo->sessionType != RTP_ST_TCPCLIENT_EX || remoteInfo->mmType != m_mmType)
    {
        ProCloseSockId(sockId);

        return;
    }

    AcceptSession(service, NULL, sockId, unixSocket,
        remoteIp, remotePort, remoteInfo, nonce);
}

void
PRO_CALLTYPE
CRtpMsgC2s::OnAcceptSession(IRtpService*            service,
                            PRO_SSL_CTX*            sslCtx,
                            PRO_INT64               sockId,
                            bool                    unixSocket,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const RTP_SESSION_INFO* remoteInfo,
                            PRO_UINT64              nonce)
{
    assert(service != NULL);
    assert(sslCtx != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    if (service == NULL || sslCtx == NULL || sockId == -1 ||
        remoteIp == NULL || remoteInfo == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_SSLCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    if (remoteInfo->sessionType != RTP_ST_SSLCLIENT_EX || remoteInfo->mmType != m_mmType)
    {
        ProSslCtx_Delete(sslCtx);
        ProCloseSockId(sockId);

        return;
    }

    AcceptSession(service, sslCtx, sockId, unixSocket,
        remoteIp, remotePort, remoteInfo, nonce);
}

void
CRtpMsgC2s::AcceptSession(IRtpService*            service,
                          PRO_SSL_CTX*            sslCtx,
                          PRO_INT64               sockId,
                          bool                    unixSocket,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const RTP_SESSION_INFO* remoteInfo,
                          PRO_UINT64              nonce)
{
    assert(service != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    assert(
        remoteInfo->sessionType == RTP_ST_TCPCLIENT_EX ||
        remoteInfo->sessionType == RTP_ST_SSLCLIENT_EX
        );
    assert(remoteInfo->mmType == m_mmType);

    RTP_MSG_HEADER0 msgHeader;
    RTP_MSG_USER    user;

    if (m_localSslConfig == NULL)
    {
        if (remoteInfo->sessionType != RTP_ST_TCPCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else if (m_localSslForced)
    {
        if (remoteInfo->sessionType != RTP_ST_SSLCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else
    {
    }

    memcpy(&msgHeader, remoteInfo->userData, sizeof(RTP_MSG_HEADER0));
    user        = msgHeader.user;
    user.instId = pbsd_ntoh16(msgHeader.user.instId);

    assert(user.classId > 0);
    assert(
        user.UserId() == 0 ||
        user.UserId() >= NODE_UID_MIN && user.UserId() <= NODE_UID_MAX
        );
    assert(!user.IsRoot());
    if (user.classId == 0
        ||
        user.UserId() != 0 &&
        (user.UserId() < NODE_UID_MIN || user.UserId() > NODE_UID_MAX)
        ||
        user.IsRoot())
    {
        goto EXIT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
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

        if (m_msgClient == NULL || m_c2sUser.classId == 0 || m_c2sUser.UserId() == 0 ||
            user == m_c2sUser)
        {
            goto EXIT;
        }

        char hashString[64 + 1] = "";
        hashString[64] = '\0';

        {
            const char* const p = remoteInfo->passwordHash;

            for (int i = 0; i < 32; ++i)
            {
                snprintf_pro(
                    hashString + i * 2,
                    2 + 1,
                    "%02x",
                    (unsigned int)(unsigned char)p[i] /* unsigned */
                    );
            }
        }

        const unsigned long timerId = m_reactor->ScheduleTimer(
            this, (PRO_UINT64)m_localTimeoutInSeconds * 1000, false);

        char idString[64] = "";
        RtpMsgUser2String(&user, idString);

        CProConfigStream msgStream;
        msgStream.Add      (TAG_msg_name          , MSG_client_login);
        msgStream.AddUint  (TAG_client_index      , timerId);
        msgStream.Add      (TAG_client_id         , idString);
        msgStream.Add      (TAG_client_public_ip  , remoteIp);
        msgStream.Add      (TAG_client_hash_string, hashString);
        msgStream.AddUint64(TAG_client_nonce      , nonce);

        CProStlString theString = "";
        msgStream.ToString(theString);

        if (!m_msgClient->SendMsg(
            theString.c_str(), (PRO_UINT16)theString.length(), 0, &ROOT_ID_C2S, 1))
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
        info.nonce      = nonce;
        info.remoteInfo = *remoteInfo;

        m_timerId2Info[timerId] = info;
    }

    return;

EXIT:

    ProSslCtx_Delete(sslCtx);
    ProCloseSockId(sockId);
}

void
PRO_CALLTYPE
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

    RTP_MSG_HEADER* const msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    if (msgHeaderPtr->dstUserCount == 0)
    {
        msgHeaderPtr->dstUserCount = 1;
    }

    const unsigned long msgHeaderSize =
        sizeof(RTP_MSG_HEADER) + sizeof(RTP_MSG_USER) * (msgHeaderPtr->dstUserCount - 1);

    const void* const   msgBodyPtr  = (char*)msgHeaderPtr + msgHeaderSize;
    const unsigned long msgBodySize = packet->GetPayloadSize() - msgHeaderSize;
    const PRO_UINT16    charset     = pbsd_ntoh16(msgHeaderPtr->charset);

    RTP_MSG_USER srcUser = msgHeaderPtr->srcUser;
    srcUser.instId       = pbsd_ntoh16(msgHeaderPtr->srcUser.instId);

    assert(msgBodySize > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (msgBodySize == 0 || srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        CProStlMap<IRtpSession*, RTP_MSG_USER>::const_iterator const itr =
            m_session2User.find(session);
        if (itr == m_session2User.end() || srcUser != itr->second)
        {
            return;
        }

        unsigned char sessionCount    = 0;
        IRtpSession*  sessions[255];
        unsigned char uplinkUserCount = 0;
        RTP_MSG_USER  uplinkUsers[255];

        for (int i = 0; i < (int)msgHeaderPtr->dstUserCount; ++i)
        {
            RTP_MSG_USER dstUser = msgHeaderPtr->dstUsers[i];
            dstUser.instId       = pbsd_ntoh16(msgHeaderPtr->dstUsers[i].instId);

            if (dstUser.classId == 0 || dstUser.UserId() == 0)
            {
                continue;
            }

            CProStlMap<RTP_MSG_USER, IRtpSession*>::const_iterator const itr2 =
                m_user2Session.find(dstUser);
            if (itr2 != m_user2Session.end())
            {
                sessions[sessionCount]       = itr2->second;
                ++sessionCount;
            }
            else
            {
                uplinkUsers[uplinkUserCount] = dstUser;
                ++uplinkUserCount;
            }
        }

        if (sessionCount > 0)
        {
            SendMsgToDownlink(sessions, sessionCount, msgBodyPtr, (PRO_UINT16)msgBodySize,
                charset, &srcUser);
        }

        if (uplinkUserCount > 0 && m_msgClient != NULL)
        {
            m_msgClient->TransferMsg(msgBodyPtr, (PRO_UINT16)msgBodySize,
                charset, uplinkUsers, uplinkUserCount, &srcUser);
        }
    }
}

void
PRO_CALLTYPE
CRtpMsgC2s::OnCloseSession(IRtpSession* session,
                           long         errorCode,
                           long         sslCode,
                           bool         tcpConnected)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;
    RTP_MSG_USER        user;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        CProStlMap<IRtpSession*, RTP_MSG_USER>::iterator const itr = m_session2User.find(session);
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
}}

void
PRO_CALLTYPE
CRtpMsgC2s::OnOkMsg(IRtpMsgClient*      msgClient,
                    const RTP_MSG_USER* myUser,
                    const char*         myPublicIp)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(msgClient != NULL);
    assert(myUser != NULL);
    assert(myUser->classId == SERVER_CID);
    assert(myUser->UserId() > 0);
    assert(myPublicIp != NULL);
    assert(myPublicIp[0] != '\0');
    if (msgClient == NULL || myUser == NULL || myUser->classId != SERVER_CID ||
        myUser->UserId() == 0 || myPublicIp == NULL || myPublicIp[0] == '\0')
    {
        return;
    }

    IRtpMsgC2sObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        m_c2sUser    = *myUser; /* login */
        m_c2sUserBak = *myUser;

        m_msgClient->SetOutputRedline(m_redlineBytes);

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnOkC2s(this, myUser, myPublicIp);
    observer->Release();
}}

void
PRO_CALLTYPE
CRtpMsgC2s::OnRecvMsg(IRtpMsgClient*      msgClient,
                      const void*         buf,
                      PRO_UINT16          size,
                      PRO_UINT16          charset,
                      const RTP_MSG_USER* srcUser)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(msgClient != NULL);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser != NULL);
    assert(srcUser->classId > 0);
    assert(srcUser->UserId() > 0);
    if (msgClient == NULL || buf == NULL || size == 0 || srcUser == NULL ||
        srcUser->classId == 0 || srcUser->UserId() == 0)
    {
        return;
    }

    if (*srcUser != ROOT_ID_C2S)
    {
        return;
    }

    const CProStlString            theString((char*)buf, size);
    CProStlVector<PRO_CONFIG_ITEM> theConfigs;

    if (!CProConfigStream::StringToConfigs(theString, theConfigs))
    {
        return;
    }

    CProConfigStream msgStream;
    msgStream.Add(theConfigs);

    CProStlString msgName = "";
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
}}

void
CRtpMsgC2s::ProcessMsg_client_login_ok(IRtpMsgClient*          msgClient,
                                       const CProConfigStream& msgStream)
{{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    unsigned int  client_index = 0;
    CProStlString client_id    = "";

    msgStream.GetUint(TAG_client_index, client_index);
    msgStream.Get    (TAG_client_id   , client_id);

    RTP_MSG_USER user;
    RtpMsgString2User(client_id.c_str(), &user);

    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    bool                ret          = true;
    IRtpMsgC2sObserver* observer     = NULL;
    IRtpSession*        oldSession   = NULL;
    IRtpSession*        newSession   = NULL;
    char                publicIp[64] = "";

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::iterator const itr =
            m_timerId2Info.find(client_index);
        if (itr == m_timerId2Info.end())
        {
            ret = false;
        }
        else
        {
            const RTP_MSG_AsyncOnAcceptSession info = itr->second;
            m_timerId2Info.erase(itr);

            m_reactor->CancelTimer(client_index);

            RTP_SESSION_INFO localInfo;
            memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
            localInfo.remoteVersion = info.remoteInfo.localVersion;
            localInfo.mmType        = m_mmType;

            RTP_INIT_ARGS initArgs;
            memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));

            if (info.remoteInfo.sessionType == RTP_ST_SSLCLIENT_EX)
            {
                initArgs.sslserverEx.observer   = this;
                initArgs.sslserverEx.reactor    = m_reactor;
                initArgs.sslserverEx.sslCtx     = info.sslCtx;
                initArgs.sslserverEx.sockId     = info.sockId;
                initArgs.sslserverEx.unixSocket = info.unixSocket;

                newSession = CreateRtpSessionWrapper(RTP_ST_SSLSERVER_EX, &initArgs, &localInfo);
            }
            else
            {
                initArgs.tcpserverEx.observer   = this;
                initArgs.tcpserverEx.reactor    = m_reactor;
                initArgs.tcpserverEx.sockId     = info.sockId;
                initArgs.tcpserverEx.unixSocket = info.unixSocket;

                newSession = CreateRtpSessionWrapper(RTP_ST_TCPSERVER_EX, &initArgs, &localInfo);
            }

            if (newSession == NULL)
            {
                ProSslCtx_Delete(info.sslCtx);
                ProCloseSockId(info.sockId);
                ret = false;
            }
            else
            {
                newSession->GetRemoteIp(publicIp);

                CProStlMap<RTP_MSG_USER, IRtpSession*>::iterator const itr2 =
                    m_user2Session.find(user);
                if (itr2 != m_user2Session.end())
                {
                    oldSession = itr2->second;
                    m_session2User.erase(oldSession);
                    m_user2Session.erase(itr2);
                }

                ret = SendAckToDownlink(newSession, &user, publicIp);
                if (ret)
                {
                    m_session2User[newSession] = user;
                    m_user2Session[user]       = newSession;
                    newSession = NULL;
                }
            }
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (oldSession != NULL)
    {
        observer->OnCloseUser(this, &user, -1, 0);
    }

    if (ret)
    {
        observer->OnOkUser(this, &user, publicIp);
    }
    else
    {
        ReportLogout(msgClient, user);
    }

    observer->Release();
    DeleteRtpSessionWrapper(newSession);
    DeleteRtpSessionWrapper(oldSession);
}}

void
CRtpMsgC2s::ProcessMsg_client_login_error(IRtpMsgClient*          msgClient,
                                          const CProConfigStream& msgStream)
{{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    unsigned int client_index = 0;
    msgStream.GetUint(TAG_client_index, client_index);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::iterator const itr =
            m_timerId2Info.find(client_index);
        if (itr == m_timerId2Info.end())
        {
            return;
        }

        const RTP_MSG_AsyncOnAcceptSession info = itr->second;
        m_timerId2Info.erase(itr);

        m_reactor->CancelTimer(client_index);
        ProSslCtx_Delete(info.sslCtx);
        ProCloseSockId(info.sockId);
    }
}}

void
CRtpMsgC2s::ProcessMsg_client_kickout(IRtpMsgClient*          msgClient,
                                      const CProConfigStream& msgStream)
{{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    CProStlString client_id = "";
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

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        CProStlMap<RTP_MSG_USER, IRtpSession*>::iterator const itr = m_user2Session.find(user);
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
}}

void
PRO_CALLTYPE
CRtpMsgC2s::OnTransferMsg(IRtpMsgClient*      msgClient,
                          const void*         buf,
                          PRO_UINT16          size,
                          PRO_UINT16          charset,
                          const RTP_MSG_USER* srcUser,
                          const RTP_MSG_USER* dstUsers,
                          unsigned char       dstUserCount)
{
    assert(msgClient != NULL);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser != NULL);
    assert(srcUser->classId > 0);
    assert(srcUser->UserId() > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    if (msgClient == NULL || buf == NULL || size == 0 || srcUser == NULL ||
        srcUser->classId == 0 || srcUser->UserId() == 0 || dstUsers == NULL || dstUserCount == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        unsigned char sessionCount = 0;
        IRtpSession*  sessions[255];

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            CProStlMap<RTP_MSG_USER, IRtpSession*>::const_iterator const itr =
                m_user2Session.find(dstUsers[i]);
            if (itr != m_user2Session.end())
            {
                sessions[sessionCount] = itr->second;
                ++sessionCount;
            }
        }

        if (sessionCount > 0)
        {
            SendMsgToDownlink(sessions, sessionCount, buf, size, charset, srcUser);
        }
    }
}

void
PRO_CALLTYPE
CRtpMsgC2s::OnCloseMsg(IRtpMsgClient* msgClient,
                       long           errorCode,
                       long           sslCode,
                       bool           tcpConnected)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    IRtpMsgC2sObserver*                    observer = NULL;
    CProStlMap<IRtpSession*, RTP_MSG_USER> session2User;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }

        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::const_iterator       itr = m_timerId2Info.begin();
        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::const_iterator const end = m_timerId2Info.end();

        for (; itr != end; ++itr)
        {
            const unsigned long                 timerId = itr->first;
            const RTP_MSG_AsyncOnAcceptSession& info    = itr->second;

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }

        m_timerId2Info.clear();
        session2User = m_session2User;
        m_session2User.clear();
        m_user2Session.clear();

        m_c2sUser.Zero();   /* reset */
        m_msgClient = NULL; /* reset */

        m_observer->AddRef();
        observer = m_observer;
    }

    CProStlMap<IRtpSession*, RTP_MSG_USER>::const_iterator       itr = session2User.begin();
    CProStlMap<IRtpSession*, RTP_MSG_USER>::const_iterator const end = session2User.end();

    for (; itr != end; ++itr)
    {
        DeleteRtpSessionWrapper(itr->first);
    }

    observer->OnCloseC2s(this, errorCode, sslCode, tcpConnected);
    observer->Release();

    CRtpMsgClient* const msgClient2 = (CRtpMsgClient*)msgClient;
    msgClient2->Fini();
    msgClient2->Release();
}}

void
PRO_CALLTYPE
CRtpMsgC2s::OnTimer(unsigned long timerId,
                    PRO_INT64     userData)
{
    assert(timerId > 0);
    if (timerId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_service == NULL || m_task == NULL)
        {
            return;
        }

        if (timerId == m_timerId)
        {
            const PRO_INT64 tick = ProGetTickCount64();

            if (m_msgClient == NULL &&
                tick - m_connectTick >= RECONNECT_INTERVAL * 1000)
            {
                m_connectTick = tick;

                m_msgClient = CRtpMsgClient::CreateInstance(
                    true, m_mmType, m_uplinkSslConfig, m_uplinkSslServiceName.c_str());
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
                }
            }

            return;
        }

        CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession>::iterator const itr =
            m_timerId2Info.find(timerId);
        if (itr != m_timerId2Info.end())
        {
            const RTP_MSG_AsyncOnAcceptSession info = itr->second;
            m_timerId2Info.erase(itr);

            m_reactor->CancelTimer(timerId);
            ProSslCtx_Delete(info.sslCtx);
            ProCloseSockId(info.sockId);
        }
    }
}

bool
CRtpMsgC2s::SendAckToDownlink(IRtpSession*        session,
                              const RTP_MSG_USER* user,
                              const char*         publicIp)
{
    assert(session != NULL);
    assert(user != NULL);
    assert(user->classId > 0);
    assert(user->UserId() > 0);
    assert(publicIp != NULL);
    assert(publicIp[0] != '\0');
    if (session == NULL || user == NULL || user->classId == 0 || user->UserId() == 0 ||
        publicIp == NULL || publicIp[0] == '\0')
    {
        return (false);
    }

    const unsigned long msgHeaderSize = sizeof(RTP_MSG_HEADER0);

    IRtpPacket* const packet = CreateRtpPacketSpace(msgHeaderSize);
    if (packet == NULL)
    {
        return (false);
    }

    RTP_MSG_HEADER0* const msgHeaderPtr = (RTP_MSG_HEADER0*)packet->GetPayloadBuffer();
    memset(msgHeaderPtr, 0, msgHeaderSize);

    msgHeaderPtr->version     = pbsd_hton16(RTP_MSG_PROTOCOL_VERSION);
    msgHeaderPtr->user        = *user;
    msgHeaderPtr->user.instId = pbsd_hton16(user->instId);
    msgHeaderPtr->publicIp    = pbsd_inet_aton(publicIp);

    packet->SetMmType(m_mmType);
    const bool ret = session->SendPacket(packet);
    packet->Release();

    return (ret);
}

bool
CRtpMsgC2s::SendMsgToDownlink(IRtpSession**       sessions,
                              unsigned char       sessionCount,
                              const void*         buf,
                              PRO_UINT16          size,
                              PRO_UINT16          charset,
                              const RTP_MSG_USER* srcUser)
{
    assert(sessions != NULL);
    assert(sessionCount > 0);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser != NULL);
    assert(srcUser->classId > 0);
    assert(srcUser->UserId() > 0);
    if (sessions == NULL || sessionCount == 0 || buf == NULL || size == 0 ||
        srcUser == NULL || srcUser->classId == 0 || srcUser->UserId() == 0)
    {
        return (false);
    }

    const unsigned long msgHeaderSize = sizeof(RTP_MSG_HEADER);

    IRtpPacket* const packet = CreateRtpPacketSpace(msgHeaderSize + size);
    if (packet == NULL)
    {
        return (false);
    }

    RTP_MSG_HEADER* const msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    memset(msgHeaderPtr, 0, msgHeaderSize);

    msgHeaderPtr->charset        = pbsd_hton16(charset);
    msgHeaderPtr->srcUser        = *srcUser;
    msgHeaderPtr->srcUser.instId = pbsd_hton16(srcUser->instId);

    memcpy((char*)msgHeaderPtr + msgHeaderSize, buf, size);

    packet->SetMmType(m_mmType);

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

    return (ret);
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

    CProStlString theString = "";
    msgStream.ToString(theString);

    msgClient->SendMsg(theString.c_str(), (PRO_UINT16)theString.length(), 0, &ROOT_ID_C2S, 1);
}
