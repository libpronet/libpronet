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

#include "rtp_msg_client.h"
#include "rtp_foundation.h"
#include "rtp_framework.h"
#include "rtp_msg_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

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
    assert(mmType != 0);
    if (mmType == 0)
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
    m_observer   = NULL;
    m_reactor    = NULL;
    m_session    = NULL;
    m_bucket     = NULL;
    m_timerId    = 0;
    m_onOkCalled = false;

    m_canUpcall  = true;
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
        user->UserId() == 0 ||
        user->UserId() >= NODE_UID_MIN && user->UserId() <= NODE_UID_MAXX
        );
    assert(!user->IsRoot());
    if (observer == NULL || reactor == NULL ||
        remoteIp == NULL || remoteIp[0] == '\0' || remotePort == 0 ||
        user == NULL || user->classId == 0
        ||
        user->UserId() != 0 &&
        (user->UserId() < NODE_UID_MIN || user->UserId() > NODE_UID_MAXX)
        ||
        user->IsRoot())
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    IRtpSession* session = NULL;
    IRtpBucket*  bucket  = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_session == NULL);
        assert(m_bucket == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_session != NULL || m_bucket != NULL)
        {
            return (false);
        }

        RTP_MSG_HEADER0 msgHeader;
        memset(&msgHeader, 0, sizeof(RTP_MSG_HEADER0));
        msgHeader.version     = pbsd_hton16(RTP_MSG_PROTOCOL_VERSION);
        msgHeader.user        = *user;
        msgHeader.user.instId = pbsd_hton16(user->instId);

        RTP_SESSION_INFO localInfo;
        memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
        localInfo.mmType   = m_mmType;
        localInfo.packMode = RTP_MSG_PACK_MODE;
        memcpy(localInfo.userData, &msgHeader, sizeof(RTP_MSG_HEADER0));

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

            session = CreateRtpSessionWrapper(RTP_ST_SSLCLIENT_EX, &initArgs, &localInfo);
            ProZeroMemory(initArgs.sslclientEx.password, sizeof(initArgs.sslclientEx.password));
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

            session = CreateRtpSessionWrapper(RTP_ST_TCPCLIENT_EX, &initArgs, &localInfo);
            ProZeroMemory(initArgs.tcpclientEx.password, sizeof(initArgs.tcpclientEx.password));
        }

        if (session == NULL)
        {
            goto EXIT;
        }

        bucket = CreateRtpBaseBucket();
        if (bucket == NULL)
        {
            goto EXIT;
        }

        observer->AddRef();
        m_observer = observer;
        m_reactor  = reactor;
        m_session  = session;
        m_bucket   = bucket;
        m_user     = *user;
        m_timerId  = reactor->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false);
    }

    return (true);

EXIT:

    if (bucket != NULL)
    {
        bucket->Destroy();
    }

    DeleteRtpSessionWrapper(session);

    return (false);
}

void
CRtpMsgClient::Fini()
{
    IRtpMsgClientObserver* observer = NULL;
    IRtpSession*           session  = NULL;
    IRtpBucket*            bucket   = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_bucket == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        bucket = m_bucket;
        m_bucket = NULL;
        session = m_session;
        m_session = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    bucket->Destroy();
    DeleteRtpSessionWrapper(session);
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CRtpMsgClient::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpMsgClient::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpMsgClient::GetUser(RTP_MSG_USER* user) const
{
    assert(user != NULL);
    if (user == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *user = m_user;
    }
}

PRO_SSL_SUITE_ID
PRO_CALLTYPE
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

bool
PRO_CALLTYPE
CRtpMsgClient::SendMsg(const void*         buf,
                       unsigned long       size,
                       PRO_UINT16          charset,
                       const RTP_MSG_USER* dstUsers,
                       unsigned char       dstUserCount)
{
    assert(buf != NULL);
    assert(size > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    if (buf == NULL || size == 0 || dstUsers == NULL || dstUserCount == 0)
    {
        return (false);
    }

    const bool ret = PushData(buf, size, charset, dstUsers, dstUserCount, NULL);

    return (ret);
}

bool
CRtpMsgClient::TransferMsg(const void*         buf,
                           unsigned long       size,
                           PRO_UINT16          charset,
                           const RTP_MSG_USER* dstUsers,
                           unsigned char       dstUserCount,
                           const RTP_MSG_USER* srcUser)
{
    assert(buf != NULL);
    assert(size > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    assert(srcUser != NULL);
    assert(srcUser->classId > 0);
    assert(srcUser->UserId() > 0);
    if (buf == NULL || size == 0 || dstUsers == NULL || dstUserCount == 0 ||
        srcUser == NULL || srcUser->classId == 0 || srcUser->UserId() == 0)
    {
        return (false);
    }

    const bool ret = PushData(buf, size, charset, dstUsers, dstUserCount, srcUser);

    return (ret);
}

bool
CRtpMsgClient::PushData(const void*         buf,
                        unsigned long       size,
                        PRO_UINT16          charset,
                        const RTP_MSG_USER* dstUsers,
                        unsigned char       dstUserCount,
                        const RTP_MSG_USER* srcUser) /* = NULL */
{
    assert(buf != NULL);
    assert(size > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);

    bool ret = true;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return (false);
        }

        unsigned long cachedBytes  = 0;
        unsigned long redlineBytes = 0;
        m_session->GetFlowctrlInfo(NULL, NULL, NULL, NULL, &cachedBytes, NULL);
        cachedBytes += m_bucket->GetTotalBytes();
        m_bucket->GetRedline(&redlineBytes, NULL);

        const unsigned long msgHeaderSize =
            sizeof(RTP_MSG_HEADER) + sizeof(RTP_MSG_USER) * (dstUserCount - 1);
        if (cachedBytes + msgHeaderSize + size > redlineBytes && /* check redline */
            cachedBytes > 0)
        {
            return (false);
        }

        IRtpPacket* const packet = CreateRtpPacketSpace(msgHeaderSize + size, RTP_MSG_PACK_MODE);
        if (packet == NULL)
        {
            return (false);
        }

        RTP_MSG_HEADER* const msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
        memset(msgHeaderPtr, 0, msgHeaderSize);

        msgHeaderPtr->charset            = pbsd_hton16(charset);
        if (srcUser != NULL)
        {
            msgHeaderPtr->srcUser        = *srcUser;
            msgHeaderPtr->srcUser.instId = pbsd_hton16(srcUser->instId);
        }
        msgHeaderPtr->dstUserCount       = dstUserCount;

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            if (dstUsers[i].classId == 0 || dstUsers[i].UserId() == 0)
            {
                ret = false;
                break;
            }

            msgHeaderPtr->dstUsers[i]        = dstUsers[i];
            msgHeaderPtr->dstUsers[i].instId = pbsd_hton16(dstUsers[i].instId);
        }

        if (!ret)
        {
            packet->Release();

            return (false);
        }

        memcpy((char*)msgHeaderPtr + msgHeaderSize, buf, size);

        packet->SetMmType(m_mmType);
        ret = m_bucket->PushBackAddRef(packet);
        packet->Release();

        SendData(m_onOkCalled);
    }

    return (ret);
}

void
CRtpMsgClient::SendData(bool onOkCalled)
{
    assert(m_session != NULL);
    assert(m_bucket != NULL);
    if (m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    if (!onOkCalled)
    {
        return;
    }

    IRtpPacket* const packet = m_bucket->GetFront();
    if (packet == NULL)
    {
        return;
    }

    RTP_MSG_HEADER* const msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();

    if (msgHeaderPtr->srcUser.classId == 0)
    {
        msgHeaderPtr->srcUser        = m_user;
        msgHeaderPtr->srcUser.instId = pbsd_hton16(m_user.instId);
    }

    if (m_session->SendPacket(packet))
    {
        m_bucket->PopFrontRelease(packet);
    }
}

void
PRO_CALLTYPE
CRtpMsgClient::SetOutputRedline(unsigned long redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        m_session->SetOutputRedline(redlineBytes, 0);
        m_bucket->SetRedline(redlineBytes, 0);
    }
}

unsigned long
PRO_CALLTYPE
CRtpMsgClient::GetOutputRedline() const
{
    unsigned long redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_bucket != NULL)
        {
            m_bucket->GetRedline(&redlineBytes, NULL);
        }
    }

    return (redlineBytes);
}

void
PRO_CALLTYPE
CRtpMsgClient::OnRecvSession(IRtpSession* session,
                             IRtpPacket*  packet)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    if (!m_onOkCalled)
    {
        RecvAck(session, packet);
    }
    else
    {
        RecvData(session, packet);
    }
}}

void
CRtpMsgClient::RecvAck(IRtpSession* session,
                       IRtpPacket*  packet)
{{
    assert(session != NULL);
    assert(packet != NULL);
    assert(packet->GetPayloadSize() >= sizeof(RTP_MSG_HEADER0));
    if (session == NULL || packet == NULL || packet->GetPayloadSize() < sizeof(RTP_MSG_HEADER0))
    {
        return;
    }

    RTP_MSG_HEADER0* const msgHeaderPtr = (RTP_MSG_HEADER0*)packet->GetPayloadBuffer();

    RTP_MSG_USER user = msgHeaderPtr->user;
    user.instId       = pbsd_ntoh16(msgHeaderPtr->user.instId);

    assert(user.classId > 0);
    assert(user.UserId() > 0);
    if (user.classId == 0 || user.UserId() == 0)
    {
        return;
    }

    IRtpMsgClientObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_user = user;
        SendData(true);

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        char publicIp[64] = "";
        pbsd_inet_ntoa(msgHeaderPtr->publicIp, publicIp);

        m_onOkCalled = true;
        observer->OnOkMsg(this, &user, publicIp);
    }

    observer->Release();
}}

void
CRtpMsgClient::RecvData(IRtpSession* session,
                        IRtpPacket*  packet)
{{
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

    IRtpMsgClientObserver* observer     = NULL;
    unsigned char          dstUserCount = 0;
    RTP_MSG_USER           dstUsers[255];

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
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
                dstUsers[dstUserCount].instId = pbsd_ntoh16(msgHeaderPtr->dstUsers[i].instId);

                if (dstUsers[dstUserCount].classId > 0 && dstUsers[dstUserCount].UserId() == 0)
                {
                    ++badUserCount;
                    continue;
                }

                if (dstUsers[dstUserCount].classId > 0 && dstUsers[dstUserCount] != m_user)
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
            observer->OnRecvMsg(this, msgBodyPtr, msgBodySize, charset, &srcUser);
        }
        else                   /* for c2s */
        {
            if (dstUserCount != msgHeaderPtr->dstUserCount)
            {
                observer->OnRecvMsg(this, msgBodyPtr, msgBodySize, charset, &srcUser);
            }

            if (dstUserCount > 0)
            {
                ((IRtpMsgClientObserverEx*)observer)->OnTransferMsg(this, msgBodyPtr,
                    msgBodySize, charset, &srcUser, dstUsers, dstUserCount);
            }
        }
    }

    observer->Release();
}}

void
PRO_CALLTYPE
CRtpMsgClient::OnSendSession(IRtpSession* session,
                             bool         packetErased)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        SendData(m_onOkCalled);
    }
}

void
PRO_CALLTYPE
CRtpMsgClient::OnCloseSession(IRtpSession* session,
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

    IRtpMsgClientObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_session = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseMsg(this, errorCode, sslCode, tcpConnected);
    }

    observer->Release();
    DeleteRtpSessionWrapper(session);
}}

void
PRO_CALLTYPE
CRtpMsgClient::OnTimer(unsigned long timerId,
                       PRO_INT64     userData)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(timerId > 0);
    if (timerId == 0)
    {
        return;
    }

    IRtpMsgClientObserver* observer     = NULL;
    bool                   tcpConnected = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
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
}}
