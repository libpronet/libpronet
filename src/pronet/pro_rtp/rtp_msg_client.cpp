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

#include "rtp_msg_client.h"
#include "rtp_base.h"
#include "rtp_msg.h"
#include "rtp_msg_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_REDLINE_BYTES (1024 * 1024)
#define DEFAULT_TIMEOUT       20

static const PRO_UINT64 NODE_UID_MIN  = 1;                                     /* 1 ~ 0xFFFFFFFFFF */
static const PRO_UINT64 NODE_UID_MAXX = ((PRO_UINT64)0xFF << 32) | 0xFFFFFFFF; /* 1 ~ 0xFFFFFFFFFF */

/////////////////////////////////////////////////////////////////////////////
////

CRtpMsgClient*
CRtpMsgClient::CreateInstance(bool                         enableTransfer,
                              RTP_MM_TYPE                  mmType,
                              const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
                              const char*                  sslSni)    /* = NULL */
{
    assert(mmType >= RTP_MMT_MSG_MIN);
    assert(mmType <= RTP_MMT_MSG_MAX);
    if (mmType < RTP_MMT_MSG_MIN || mmType > RTP_MMT_MSG_MAX)
    {
        return (NULL);
    }

    CRtpMsgClient* const msgClient =
        new CRtpMsgClient(enableTransfer, mmType, sslConfig, sslSni);

    return (msgClient);
}

CRtpMsgClient::CRtpMsgClient(bool                         enableTransfer,
                             RTP_MM_TYPE                  mmType,
                             const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
                             const char*                  sslSni)    /* = NULL */
                             :
m_enableTransfer(enableTransfer),
m_mmType(mmType),
m_sslConfig(sslConfig),
m_sslSni(sslSni != NULL ? sslSni : "")
{
    m_observer  = NULL;
    m_reactor   = NULL;
    m_session   = NULL;
    m_timerId   = 0;

    m_canUpcall = true;
}

CRtpMsgClient::~CRtpMsgClient()
{
    Fini();
}

bool
CRtpMsgClient::Init(IRtpMsgClientObserver* observer,
                    IProReactor*           reactor,
                    const char*            remoteIp,
                    unsigned short         remotePort,
                    const RTP_MSG_USER*    user,
                    const char*            password,         /* = NULL */
                    const char*            localIp,          /* = NULL */
                    unsigned long          timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    assert(user != NULL);
    assert(user->classId > 0);
    assert(
        user->UserId() == 0
        ||
        (user->UserId() >= NODE_UID_MIN && user->UserId() <= NODE_UID_MAXX)
       );
    assert(!user->IsRoot());
    if (
        observer == NULL || reactor == NULL ||
        remoteIp == NULL || remoteIp[0] == '\0' || remotePort == 0 ||
        user == NULL || user->classId == 0
        ||
        (user->UserId() != 0 &&
        (user->UserId() < NODE_UID_MIN || user->UserId() > NODE_UID_MAXX))
        ||
        user->IsRoot()
       )
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_session == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_session != NULL)
        {
            return (false);
        }

        RTP_MSG_HEADER0 hdr0;
        memset(&hdr0, 0, sizeof(RTP_MSG_HEADER0));
        hdr0.version     = pbsd_hton16(RTP_MSG_PROTOCOL_VERSION);
        hdr0.user        = *user;
        hdr0.user.instId = pbsd_hton16(hdr0.user.instId);

        RTP_SESSION_INFO localInfo;
        memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
        localInfo.mmType   = m_mmType;
        localInfo.packMode = RTP_MSG_PACK_MODE;
        memcpy(localInfo.userData, &hdr0, sizeof(RTP_MSG_HEADER0));

        RTP_INIT_ARGS initArgs;
        memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));

        if (m_sslConfig != NULL)
        {
            initArgs.sslclientEx.observer         = this;
            initArgs.sslclientEx.reactor          = reactor;
            initArgs.sslclientEx.sslConfig        = m_sslConfig;
            initArgs.sslclientEx.remotePort       = remotePort;
            initArgs.sslclientEx.timeoutInSeconds = timeoutInSeconds;
            strncpy_pro(initArgs.sslclientEx.sslSni,
                sizeof(initArgs.sslclientEx.sslSni), m_sslSni.c_str());
            strncpy_pro(initArgs.sslclientEx.remoteIp,
                sizeof(initArgs.sslclientEx.remoteIp), remoteIp);
            if (password != NULL)
            {
                strncpy_pro(initArgs.sslclientEx.password,
                    sizeof(initArgs.sslclientEx.password), password);
            }
            if (localIp != NULL)
            {
                strncpy_pro(initArgs.sslclientEx.localIp,
                    sizeof(initArgs.sslclientEx.localIp), localIp);
            }

            m_session = CreateRtpSessionWrapper(
                RTP_ST_SSLCLIENT_EX, &initArgs, &localInfo);
            ProZeroMemory(
                initArgs.sslclientEx.password,
                sizeof(initArgs.sslclientEx.password)
                );
        }
        else
        {
            initArgs.tcpclientEx.observer         = this;
            initArgs.tcpclientEx.reactor          = reactor;
            initArgs.tcpclientEx.remotePort       = remotePort;
            initArgs.tcpclientEx.timeoutInSeconds = timeoutInSeconds;
            strncpy_pro(initArgs.tcpclientEx.remoteIp,
                sizeof(initArgs.tcpclientEx.remoteIp), remoteIp);
            if (password != NULL)
            {
                strncpy_pro(initArgs.tcpclientEx.password,
                    sizeof(initArgs.tcpclientEx.password), password);
            }
            if (localIp != NULL)
            {
                strncpy_pro(initArgs.tcpclientEx.localIp,
                    sizeof(initArgs.tcpclientEx.localIp), localIp);
            }

            m_session = CreateRtpSessionWrapper(
                RTP_ST_TCPCLIENT_EX, &initArgs, &localInfo);
            ProZeroMemory(
                initArgs.tcpclientEx.password,
                sizeof(initArgs.tcpclientEx.password)
                );
        }

        if (m_session == NULL)
        {
            return (false);
        }
        else
        {
            m_session->SetOutputRedline(DEFAULT_REDLINE_BYTES, 0, 0);
        }

        observer->AddRef();
        m_observer = observer;
        m_reactor  = reactor;
        m_userBak  = *user;
        m_timerId  = reactor->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false);
    }

    return (true);
}

void
CRtpMsgClient::Fini()
{
    IRtpMsgClientObserver* observer = NULL;
    IRtpSession*           session  = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        session = m_session;
        m_session = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    DeleteRtpSessionWrapper(session);
    observer->Release();
}

unsigned long
CRtpMsgClient::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
CRtpMsgClient::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CRtpMsgClient::GetUser(RTP_MSG_USER* myUser) const
{
    assert(myUser != NULL);
    if (myUser == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *myUser = m_userBak;
    }
}

PRO_SSL_SUITE_ID
CRtpMsgClient::GetSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            suiteId = m_session->GetSslSuite(suiteName);
        }
    }

    return (suiteId);
}

const char*
CRtpMsgClient::GetLocalIp(char localIp[64]) const
{
    strcpy(localIp, "0.0.0.0");

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetLocalIp(localIp);
        }
    }

    return (localIp);
}

unsigned short
CRtpMsgClient::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            localPort = m_session->GetLocalPort();
        }
    }

    return (localPort);
}

const char*
CRtpMsgClient::GetRemoteIp(char remoteIp[64]) const
{
    strcpy(remoteIp, "0.0.0.0");

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetRemoteIp(remoteIp);
        }
    }

    return (remoteIp);
}

unsigned short
CRtpMsgClient::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            remotePort = m_session->GetRemotePort();
        }
    }

    return (remotePort);
}

bool
CRtpMsgClient::SendMsg(const void*         buf,
                       unsigned long       size,
                       PRO_UINT16          charset,
                       const RTP_MSG_USER* dstUsers,
                       unsigned char       dstUserCount)
{
    const bool ret = SendMsg2(
        buf, size, NULL, 0, charset, dstUsers, dstUserCount);

    return (ret);
}

bool
CRtpMsgClient::SendMsg2(const void*         buf1,
                        unsigned long       size1,
                        const void*         buf2,  /* = NULL */
                        unsigned long       size2, /* = 0 */
                        PRO_UINT16          charset,
                        const RTP_MSG_USER* dstUsers,
                        unsigned char       dstUserCount)
{
    assert(buf1 != NULL);
    assert(size1 > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    if (buf1 == NULL || size1 == 0 || dstUsers == NULL || dstUserCount == 0)
    {
        return (false);
    }

    const bool ret = PushData(
        buf1, size1, buf2, size2, charset, dstUsers, dstUserCount, NULL);

    return (ret);
}

bool
CRtpMsgClient::TransferMsg(const void*         buf,
                           unsigned long       size,
                           PRO_UINT16          charset,
                           const RTP_MSG_USER* dstUsers,
                           unsigned char       dstUserCount,
                           const RTP_MSG_USER& srcUser)
{
    assert(buf != NULL);
    assert(size > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (buf == NULL || size == 0 || dstUsers == NULL || dstUserCount == 0 ||
        srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return (false);
    }

    const bool ret = PushData(
        buf, size, NULL, 0, charset, dstUsers, dstUserCount, &srcUser);

    return (ret);
}

bool
CRtpMsgClient::PushData(const void*         buf1,
                        unsigned long       size1,
                        const void*         buf2,    /* = NULL */
                        unsigned long       size2,   /* = 0 */
                        PRO_UINT16          charset,
                        const RTP_MSG_USER* dstUsers,
                        unsigned char       dstUserCount,
                        const RTP_MSG_USER* srcUser) /* = NULL */
{
    assert(buf1 != NULL);
    assert(size1 > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);

    if (buf2 == NULL || size2 == 0)
    {
        buf2  = NULL;
        size2 = 0;
    }

    if (srcUser == NULL)
    {
        srcUser = &m_userBak;
    }

    bool ret = true;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return (false);
        }

        const unsigned long msgHeaderSize = sizeof(RTP_MSG_HEADER) +
            sizeof(RTP_MSG_USER) * (dstUserCount - 1);

        IRtpPacket* const packet = CreateRtpPacketSpace(
            msgHeaderSize + size1 + size2, RTP_MSG_PACK_MODE);
        if (packet == NULL)
        {
            return (false);
        }

        RTP_MSG_HEADER* const msgHeaderPtr =
            (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
        memset(msgHeaderPtr, 0, msgHeaderSize);

        msgHeaderPtr->charset        = pbsd_hton16(charset);
        msgHeaderPtr->srcUser        = *srcUser;
        msgHeaderPtr->srcUser.instId = pbsd_hton16(msgHeaderPtr->srcUser.instId);
        msgHeaderPtr->dstUserCount   = dstUserCount;

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            if (dstUsers[i].classId == 0 || dstUsers[i].UserId() == 0)
            {
                ret = false;
                break;
            }

            msgHeaderPtr->dstUsers[i]        = dstUsers[i];
            msgHeaderPtr->dstUsers[i].instId = pbsd_hton16(msgHeaderPtr->dstUsers[i].instId);
        }

        if (!ret)
        {
            packet->Release();

            return (false);
        }

        memcpy((char*)msgHeaderPtr + msgHeaderSize, buf1, size1);
        if (buf2 != NULL && size2 > 0)
        {
            memcpy((char*)msgHeaderPtr + msgHeaderSize + size1, buf2, size2);
        }

        packet->SetMmType(m_mmType);
        ret = m_session->SendPacket(packet);
        packet->Release();
    }

    return (ret);
}

void
CRtpMsgClient::SetOutputRedline(unsigned long redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        m_session->SetOutputRedline(redlineBytes, 0, 0);
    }
}

unsigned long
CRtpMsgClient::GetOutputRedline() const
{
    unsigned long redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetOutputRedline(&redlineBytes, NULL, NULL);
        }
    }

    return (redlineBytes);
}

unsigned long
CRtpMsgClient::GetSendingBytes() const
{
    unsigned long sendingBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetFlowctrlInfo(
                NULL, NULL, NULL, NULL, &sendingBytes, NULL);
        }
    }

    return (sendingBytes);
}

void
CRtpMsgClient::OnOkSession(IRtpSession* session)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    RTP_SESSION_ACK ack;
    session->GetAck(&ack);

    RTP_MSG_HEADER0 hdr0;
    memcpy(&hdr0, ack.userData, sizeof(RTP_MSG_HEADER0));
    hdr0.version = pbsd_ntoh16(hdr0.version);

    RTP_MSG_USER user = hdr0.user;
    user.instId       = pbsd_ntoh16(user.instId);

    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    IRtpMsgClientObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_userBak = user; /* login */

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        char publicIp[64] = "";
        pbsd_inet_ntoa(hdr0.publicIp, publicIp);

        observer->OnOkMsg(this, &user, publicIp);
    }

    observer->Release();
}

void
CRtpMsgClient::OnRecvSession(IRtpSession* session,
                             IRtpPacket*  packet)
{
    assert(session != NULL);
    assert(packet != NULL);
    assert(packet->GetPayloadSize() > sizeof(RTP_MSG_HEADER));
    if (session == NULL || packet == NULL ||
        packet->GetPayloadSize() <= sizeof(RTP_MSG_HEADER))
    {
        return;
    }

    RTP_MSG_HEADER* const msgHeaderPtr =
        (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    if (msgHeaderPtr->dstUserCount == 0)
    {
        msgHeaderPtr->dstUserCount = 1;
    }

    const unsigned long msgHeaderSize = sizeof(RTP_MSG_HEADER) +
        sizeof(RTP_MSG_USER) * (msgHeaderPtr->dstUserCount - 1);

    const void* const   msgBodyPtr  = (char*)msgHeaderPtr + msgHeaderSize;
    const unsigned long msgBodySize = packet->GetPayloadSize() - msgHeaderSize;
    const PRO_UINT16    charset     = pbsd_ntoh16(msgHeaderPtr->charset);

    RTP_MSG_USER srcUser = msgHeaderPtr->srcUser;
    srcUser.instId       = pbsd_ntoh16(srcUser.instId);

    assert(msgBodySize > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (msgBodySize == 0 || srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return;
    }

    IRtpMsgClientObserver* observer     = NULL;
    unsigned char          dstUserCount = 0;
    RTP_MSG_USER           dstUsers[255];

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        if (!m_enableTransfer) /* for client */
        {
        }
        else                   /* for c2s */
        {
            unsigned char badUserCount = 0;

            for (int i = 0; i < (int)msgHeaderPtr->dstUserCount; ++i)
            {
                dstUsers[dstUserCount]        = msgHeaderPtr->dstUsers[i];
                dstUsers[dstUserCount].instId = pbsd_ntoh16(dstUsers[dstUserCount].instId);

                if (dstUsers[dstUserCount].classId > 0 &&
                    dstUsers[dstUserCount].UserId() == 0)
                {
                    ++badUserCount;
                    continue;
                }

                if (dstUsers[dstUserCount].classId > 0 &&
                    dstUsers[dstUserCount] != m_userBak)
                {
                    ++dstUserCount;
                }
            }

            msgHeaderPtr->dstUserCount -= badUserCount;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (!m_enableTransfer) /* for client */
        {
            observer->OnRecvMsg(
                this, msgBodyPtr, msgBodySize, charset, &srcUser);
        }
        else                   /* for c2s */
        {
            if (dstUserCount != msgHeaderPtr->dstUserCount)
            {
                observer->OnRecvMsg(
                    this, msgBodyPtr, msgBodySize, charset, &srcUser);
            }

            if (dstUserCount > 0)
            {
                ((IRtpMsgClientObserverEx*)observer)->OnTransferMsg(
                    this,
                    msgBodyPtr,
                    msgBodySize,
                    charset,
                    srcUser,
                    dstUsers,
                    dstUserCount
                    );
            }
        }
    }

    observer->Release();
}

void
CRtpMsgClient::OnCloseSession(IRtpSession* session,
                              long         errorCode,
                              long         sslCode,
                              bool         tcpConnected)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgClientObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseMsg(this, errorCode, sslCode, tcpConnected);
    }

    observer->Release();
}

void
CRtpMsgClient::OnHeartbeatSession(IRtpSession* session,
                                  PRO_INT64    peerAliveTick)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgClientObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        observer->OnHeartbeatMsg(this, peerAliveTick);
    }

    observer->Release();
}

void
CRtpMsgClient::OnTimer(void*      factory,
                       PRO_UINT64 timerId,
                       PRO_INT64  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IRtpMsgClientObserver* observer     = NULL;
    bool                   tcpConnected = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        tcpConnected = m_session->IsTcpConnected();

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseMsg(this, PBSD_ETIMEDOUT, 0, tcpConnected);
    }

    observer->Release();

    Fini();
}
