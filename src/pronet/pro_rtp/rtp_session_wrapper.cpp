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

#include "rtp_session_wrapper.h"
#include "rtp_base.h"
#include "rtp_bucket.h"
#include "rtp_session_a.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_file_monitor.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stat.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define TRACE_INTERVAL     20
#define HEARTBEAT_INTERVAL 1

#if defined(__cplusplus)
extern "C" {
#endif

extern CProFileMonitor g_fileMonitor;

#if defined(__cplusplus)
} /* extern "C" */
#endif

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionWrapper*
CRtpSessionWrapper::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return NULL;
    }

    return new CRtpSessionWrapper(*localInfo);
}

CRtpSessionWrapper::CRtpSessionWrapper(const RTP_SESSION_INFO& localInfo)
{
    m_info             = localInfo;
    m_magic            = 0;
    m_observer         = NULL;
    m_reactor          = NULL;
    m_session          = NULL;
    m_bucket           = NULL;
    m_pushToBucketRet1 = true; /* !!! */
    m_pushToBucketRet2 = true; /* !!! */
    m_packetErased     = false;
    m_enableInput      = true;
    m_enableOutput     = true;
    m_outputPending    = false;
    m_timerId          = 0;
    m_onOkCalled       = false;
    m_traceTick        = 0;

    m_sendTimerId      = 0;
    m_sendDurationMs   = 0;
    m_pushTick         = 0;
}

CRtpSessionWrapper::~CRtpSessionWrapper()
{
    Fini();
}

bool
CRtpSessionWrapper::Init(RTP_SESSION_TYPE     sessionType,
                         const RTP_INIT_ARGS* initArgs)
{
    assert(initArgs != NULL);
    assert(initArgs->comm.observer != NULL);
    assert(initArgs->comm.reactor != NULL);
    if (initArgs == NULL || initArgs->comm.observer == NULL || initArgs->comm.reactor == NULL)
    {
        return false;
    }

    RTP_INIT_ARGS initArgs2 = *initArgs;

    switch (sessionType)
    {
    case RTP_ST_UDPCLIENT:
        initArgs2.udpclient.localIp[sizeof(initArgs2.udpclient.localIp) - 1] = '\0';
        break;
    case RTP_ST_UDPSERVER:
        initArgs2.udpserver.localIp[sizeof(initArgs2.udpserver.localIp) - 1] = '\0';
        break;
    case RTP_ST_TCPCLIENT:
        initArgs2.tcpclient.remoteIp[sizeof(initArgs2.tcpclient.remoteIp) - 1] = '\0';
        initArgs2.tcpclient.localIp[sizeof(initArgs2.tcpclient.localIp) - 1]   = '\0';
        break;
    case RTP_ST_TCPSERVER:
        initArgs2.tcpserver.localIp[sizeof(initArgs2.tcpserver.localIp) - 1] = '\0';
        break;
    case RTP_ST_UDPCLIENT_EX:
        initArgs2.udpclientEx.remoteIp[sizeof(initArgs2.udpclientEx.remoteIp) - 1] = '\0';
        initArgs2.udpclientEx.localIp[sizeof(initArgs2.udpclientEx.localIp) - 1]   = '\0';
        break;
    case RTP_ST_UDPSERVER_EX:
        initArgs2.udpserverEx.localIp[sizeof(initArgs2.udpserverEx.localIp) - 1] = '\0';
        break;
    case RTP_ST_TCPCLIENT_EX:
        initArgs2.tcpclientEx.remoteIp[sizeof(initArgs2.tcpclientEx.remoteIp) - 1] = '\0';
        initArgs2.tcpclientEx.password[sizeof(initArgs2.tcpclientEx.password) - 1] = '\0';
        initArgs2.tcpclientEx.localIp[sizeof(initArgs2.tcpclientEx.localIp) - 1]   = '\0';
        break;
    case RTP_ST_TCPSERVER_EX:
        break;
    case RTP_ST_SSLCLIENT_EX:
        initArgs2.sslclientEx.sslSni[sizeof(initArgs2.sslclientEx.sslSni) - 1]     = '\0';
        initArgs2.sslclientEx.remoteIp[sizeof(initArgs2.sslclientEx.remoteIp) - 1] = '\0';
        initArgs2.sslclientEx.password[sizeof(initArgs2.sslclientEx.password) - 1] = '\0';
        initArgs2.sslclientEx.localIp[sizeof(initArgs2.sslclientEx.localIp) - 1]   = '\0';
        break;
    case RTP_ST_SSLSERVER_EX:
        break;
    case RTP_ST_MCAST:
        initArgs2.mcast.mcastIp[sizeof(initArgs2.mcast.mcastIp) - 1] = '\0';
        initArgs2.mcast.localIp[sizeof(initArgs2.mcast.localIp) - 1] = '\0';
        break;
    case RTP_ST_MCAST_EX:
        initArgs2.mcastEx.mcastIp[sizeof(initArgs2.mcastEx.mcastIp) - 1] = '\0';
        initArgs2.mcastEx.localIp[sizeof(initArgs2.mcastEx.localIp) - 1] = '\0';
        break;
    default:
    {
        assert(0);

        return false;
    }
    } /* end of switch () */

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_session == NULL);
        assert(m_bucket == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_session != NULL || m_bucket != NULL)
        {
            return false;
        }

        IRtpBucket* sysBucket = CreateRtpBucket(m_info.mmType, sessionType);
        if (sysBucket == NULL)
        {
            return false;
        }

        switch (sessionType)
        {
        case RTP_ST_UDPCLIENT:
        {
            m_session = CreateRtpSessionUdpclient(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.udpclient.localIp,
                initArgs2.udpclient.localPort
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.udpclient.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.udpclient.bucket;
            }
            break;
        }
        case RTP_ST_UDPSERVER:
        {
            m_session = CreateRtpSessionUdpserver(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.udpserver.localIp,
                initArgs2.udpserver.localPort
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.udpserver.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.udpserver.bucket;
            }
            break;
        }
        case RTP_ST_TCPCLIENT:
        {
            m_session = CreateRtpSessionTcpclient(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.tcpclient.remoteIp,
                initArgs2.tcpclient.remotePort,
                initArgs2.tcpclient.localIp,
                initArgs2.tcpclient.timeoutInSeconds,
                initArgs2.tcpclient.suspendRecv
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.tcpclient.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.tcpclient.bucket;
            }
            break;
        }
        case RTP_ST_TCPSERVER:
        {
            m_session = CreateRtpSessionTcpserver(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.tcpserver.localIp,
                initArgs2.tcpserver.localPort,
                initArgs2.tcpserver.timeoutInSeconds,
                initArgs2.tcpserver.suspendRecv
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.tcpserver.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.tcpserver.bucket;
            }
            break;
        }
        case RTP_ST_UDPCLIENT_EX:
        {
            m_session = CreateRtpSessionUdpclientEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.udpclientEx.remoteIp,
                initArgs2.udpclientEx.remotePort,
                initArgs2.udpclientEx.localIp,
                initArgs2.udpclientEx.timeoutInSeconds
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.udpclientEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.udpclientEx.bucket;
            }
            break;
        }
        case RTP_ST_UDPSERVER_EX:
        {
            m_session = CreateRtpSessionUdpserverEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.udpserverEx.localIp,
                initArgs2.udpserverEx.localPort,
                initArgs2.udpserverEx.timeoutInSeconds
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.udpserverEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.udpserverEx.bucket;
            }
            break;
        }
        case RTP_ST_TCPCLIENT_EX:
        {
            m_session = CreateRtpSessionTcpclientEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.tcpclientEx.remoteIp,
                initArgs2.tcpclientEx.remotePort,
                initArgs2.tcpclientEx.password,
                initArgs2.tcpclientEx.localIp,
                initArgs2.tcpclientEx.timeoutInSeconds,
                initArgs2.tcpclientEx.suspendRecv
                );
            ProZeroMemory(initArgs2.tcpclientEx.password, sizeof(initArgs2.tcpclientEx.password));
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.tcpclientEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.tcpclientEx.bucket;
            }
            break;
        }
        case RTP_ST_TCPSERVER_EX:
        {
            m_session = CreateRtpSessionTcpserverEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.tcpserverEx.sockId,
                initArgs2.tcpserverEx.unixSocket,
                initArgs2.tcpserverEx.useAckData,
                initArgs2.tcpserverEx.ackData,
                initArgs2.tcpserverEx.suspendRecv
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.tcpserverEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.tcpserverEx.bucket;
            }
            break;
        }
        case RTP_ST_SSLCLIENT_EX:
        {
            m_session = CreateRtpSessionSslclientEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.sslclientEx.sslConfig,
                initArgs2.sslclientEx.sslSni,
                initArgs2.sslclientEx.remoteIp,
                initArgs2.sslclientEx.remotePort,
                initArgs2.sslclientEx.password,
                initArgs2.sslclientEx.localIp,
                initArgs2.sslclientEx.timeoutInSeconds,
                initArgs2.sslclientEx.suspendRecv
                );
            ProZeroMemory(initArgs2.sslclientEx.password, sizeof(initArgs2.sslclientEx.password));
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.sslclientEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.sslclientEx.bucket;
            }
            break;
        }
        case RTP_ST_SSLSERVER_EX:
        {
            m_session = CreateRtpSessionSslserverEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.sslserverEx.sslCtx,
                initArgs2.sslserverEx.sockId,
                initArgs2.sslserverEx.unixSocket,
                initArgs2.sslserverEx.useAckData,
                initArgs2.sslserverEx.ackData,
                initArgs2.sslserverEx.suspendRecv
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.sslserverEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.sslserverEx.bucket;
            }
            break;
        }
        case RTP_ST_MCAST:
        {
            m_session = CreateRtpSessionMcast(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.mcast.mcastIp,
                initArgs2.mcast.mcastPort,
                initArgs2.mcast.localIp
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.mcast.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.mcast.bucket;
            }
            break;
        }
        case RTP_ST_MCAST_EX:
        {
            m_session = CreateRtpSessionMcastEx(
                this,
                initArgs2.comm.reactor,
                &m_info,
                initArgs2.mcastEx.mcastIp,
                initArgs2.mcastEx.mcastPort,
                initArgs2.mcastEx.localIp
                );
            if (m_session == NULL)
            {
                break;
            }

            if (initArgs2.mcastEx.bucket == NULL)
            {
                m_bucket  = sysBucket;
                sysBucket = NULL;
            }
            else
            {
                m_bucket  = initArgs2.mcastEx.bucket;
            }
            break;
        }
        } /* end of switch () */

        if (sysBucket != NULL)
        {
            sysBucket->Destroy();
        }

        if (m_session == NULL)
        {
            return false;
        }

        unsigned int statInSeconds = GetRtpStatTimeSpan();

        switch (sessionType)
        {
        case RTP_ST_TCPCLIENT_EX:
        case RTP_ST_TCPSERVER_EX:
        case RTP_ST_SSLCLIENT_EX:
        case RTP_ST_SSLSERVER_EX:
            statInSeconds *= 2; /* double span */
            break;
        }

        m_statFrameRateInput.SetTimeSpan(statInSeconds);
        m_statFrameRateOutput.SetTimeSpan(statInSeconds);
        m_statBitRateInput.SetTimeSpan(statInSeconds);
        m_statBitRateOutput.SetTimeSpan(statInSeconds);
        m_statLossRateInput.SetTimeSpan(statInSeconds);
        m_statLossRateOutput.SetTimeSpan(statInSeconds);

        initArgs2.comm.observer->AddRef();
        m_observer = initArgs2.comm.observer;
        m_reactor  = initArgs2.comm.reactor;
        m_session->GetInfo(&m_info); /* retrieve the real info */

        bool enableTrace = false;
        if (
            (m_info.mmType >= RTP_MMT_MSG_MIN && m_info.mmType <= RTP_MMT_MSG_MAX)
            ||
            (m_info.mmType >= RTP_MMT_AUDIO_MIN && m_info.mmType <= RTP_MMT_AUDIO_MAX)
            ||
            (m_info.mmType >= RTP_MMT_VIDEO_MIN && m_info.mmType <= RTP_MMT_VIDEO_MAX)
           )
        {
            enableTrace = g_fileMonitor.QueryFileExist();
        }
        if (enableTrace)
        {
            m_timerId = m_reactor->SetupTimer(
                this, HEARTBEAT_INTERVAL * 1000, HEARTBEAT_INTERVAL * 1000);
        }
    }

    return true;
}

void
CRtpSessionWrapper::Fini()
{
    IRtpSessionObserver*      observer = NULL;
    IRtpSession*              session  = NULL;
    IRtpBucket*               bucket   = NULL;
    CProStlDeque<IRtpPacket*> pushPackets;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_reactor->CancelMmTimer(m_sendTimerId);
        m_timerId     = 0;
        m_sendTimerId = 0;

        pushPackets = m_pushPackets;
        m_pushPackets.clear();
        bucket = m_bucket;
        m_bucket = NULL;
        session = m_session;
        m_session = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    int i = 0;
    int c = (int)pushPackets.size();

    for (; i < c; ++i)
    {
        pushPackets[i]->Release();
    }

    bucket->Destroy();
    DeleteRtpSession(session);
    observer->Release();
}

unsigned long
CRtpSessionWrapper::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CRtpSessionWrapper::Release()
{
    return CProRefCount::Release();
}

void
CRtpSessionWrapper::GetInfo(RTP_SESSION_INFO* info) const
{
    assert(info != NULL);
    if (info == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *info = m_info;
    }
}

void
CRtpSessionWrapper::GetAck(RTP_SESSION_ACK* ack) const
{
    assert(ack != NULL);
    if (ack == NULL)
    {
        return;
    }

    memset(ack, 0, sizeof(RTP_SESSION_ACK));

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetAck(ack);
        }
    }
}

void
CRtpSessionWrapper::GetSyncId(unsigned char syncId[14]) const
{
    memset(syncId, 0, 14);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetSyncId(syncId);
        }
    }
}

PRO_SSL_SUITE_ID
CRtpSessionWrapper::GetSslSuite(char suiteName[64]) const
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

    return suiteId;
}

int64_t
CRtpSessionWrapper::GetSockId() const
{
    int64_t sockId = -1;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            sockId = m_session->GetSockId();
        }
    }

    return sockId;
}

const char*
CRtpSessionWrapper::GetLocalIp(char localIp[64]) const
{
    strcpy(localIp, "0.0.0.0");

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetLocalIp(localIp);
        }
    }

    return localIp;
}

unsigned short
CRtpSessionWrapper::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            localPort = m_session->GetLocalPort();
        }
    }

    return localPort;
}

const char*
CRtpSessionWrapper::GetRemoteIp(char remoteIp[64]) const
{
    strcpy(remoteIp, "0.0.0.0");

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetRemoteIp(remoteIp);
        }
    }

    return remoteIp;
}

unsigned short
CRtpSessionWrapper::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            remotePort = m_session->GetRemotePort();
        }
    }

    return remotePort;
}

void
CRtpSessionWrapper::SetRemoteIpAndPort(const char*    remoteIp,   /* = NULL */
                                       unsigned short remotePort) /* = 0 */
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_session->SetRemoteIpAndPort(remoteIp, remotePort);
}

bool
CRtpSessionWrapper::IsTcpConnected() const
{
    bool connected = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            connected = m_session->IsTcpConnected();
        }
    }

    return connected;
}

bool
CRtpSessionWrapper::SendPacket(IRtpPacket* packet,
                               bool*       tryAgain) /* = NULL */
{
    if (tryAgain != NULL)
    {
        *tryAgain = false;
    }

    assert(packet != NULL);
    if (packet == NULL)
    {
        return false;
    }

    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return false;
        }

        if (!m_enableOutput)
        {
            return false;
        }

        ret = PushPacket(packet);
    }

    return ret;
}

bool
CRtpSessionWrapper::SendPacketByTimer(IRtpPacket*  packet,
                                      unsigned int sendDurationMs) /* = 0 */
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return false;
        }

        if (!m_enableOutput)
        {
            return false;
        }

        if (m_sendTimerId == 0)
        {
            m_sendTimerId = m_reactor->SetupMmTimer(this, 1, 1); /* 1ms */
        }

        packet->AddRef();
        m_pushPackets.push_back(packet);
        m_sendDurationMs = sendDurationMs;
        m_pushTick       = ProGetTickCount64();
    }

    return true;
}

bool
CRtpSessionWrapper::PushPacket(IRtpPacket* packet)
{
    assert(packet != NULL);
    assert(m_session != NULL);
    assert(m_bucket != NULL);

    m_pushToBucketRet2 = m_bucket->PushBackAddRef(packet);
    if (m_pushToBucketRet1 && !m_pushToBucketRet2)      /* 1 ---> 0 */
    {
        m_packetErased = true;
        m_session->RequestOnSend();
    }
    else if (!m_pushToBucketRet1 && m_pushToBucketRet2) /* 0 ---> 1 */
    {
        m_packetErased = false;
    }
    else
    {
    }
    m_pushToBucketRet1 = m_pushToBucketRet2;

    if (CRtpSessionBase::IsUdpSession(m_info.sessionType))
    {
        DoSendPacket();
    }
    else if (!m_outputPending)
    {
        m_outputPending = DoSendPacket();
    }
    else
    {
    }

    return m_pushToBucketRet2;
}

bool
CRtpSessionWrapper::DoSendPacket()
{
    assert(m_session != NULL);
    assert(m_bucket != NULL);

    IRtpPacket* packet = m_bucket->GetFront();
    if (packet == NULL)
    {
        return false;
    }

    bool tryAgain = false;
    bool ret      = m_session->SendPacket(packet, &tryAgain);

    if (ret)
    {
        if (
            packet->GetMarker()
            ||
            m_info.mmType < RTP_MMT_VIDEO_MIN || m_info.mmType > RTP_MMT_VIDEO_MAX /* non-video */
           )
        {
            m_statFrameRateOutput.PushDataBits(1);
        }
        m_statBitRateOutput.PushDataBytes(packet->GetPayloadSize());
        m_statLossRateOutput.PushData(packet->GetSequence());

        m_bucket->PopFrontRelease(packet);
    }
    else if (!tryAgain)
    {
        m_bucket->PopFrontRelease(packet);
    }
    else
    {
    }

    return ret;
}

void
CRtpSessionWrapper::GetSendOnSendTick(int64_t* onSendTick1,       /* = NULL */
                                      int64_t* onSendTick2) const /* = NULL */
{
    if (onSendTick1 != NULL)
    {
        *onSendTick1 = 0;
    }
    if (onSendTick2 != NULL)
    {
        *onSendTick2 = 0;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_session != NULL)
        {
            m_session->GetSendOnSendTick(onSendTick1, onSendTick2);
        }
    }
}

void
CRtpSessionWrapper::RequestOnSend()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_session->RequestOnSend();
}

void
CRtpSessionWrapper::SuspendRecv()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_session->SuspendRecv();
}

void
CRtpSessionWrapper::ResumeRecv()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_session->ResumeRecv();
}

bool
CRtpSessionWrapper::AddMcastReceiver(const char* mcastIp)
{
    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return false;
        }

        ret = m_session->AddMcastReceiver(mcastIp);
    }

    return ret;
}

void
CRtpSessionWrapper::RemoveMcastReceiver(const char* mcastIp)
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_session->RemoveMcastReceiver(mcastIp);
}

void
CRtpSessionWrapper::EnableInput(bool enable)
{
    enable = enable ? true : false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (enable == m_enableInput)
        {
            return;
        }

        m_enableInput = enable;

        m_statFrameRateInput.Reset();
        m_statBitRateInput.Reset();
        m_statLossRateInput.Reset();
    }
}

void
CRtpSessionWrapper::EnableOutput(bool enable)
{
    enable = enable ? true : false;

    CProStlDeque<IRtpPacket*> pushPackets;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (enable == m_enableOutput)
        {
            return;
        }

        m_enableOutput = enable;

        m_bucket->Reset();
        m_pushToBucketRet1 = true; /* !!! */
        m_pushToBucketRet2 = true; /* !!! */
        m_packetErased     = false;
        m_outputPending    = false;

        pushPackets = m_pushPackets;
        m_pushPackets.clear();

        m_bucket->ResetFlowctrlInfo();

        m_statFrameRateOutput.Reset();
        m_statBitRateOutput.Reset();
        m_statLossRateOutput.Reset();
    }

    int i = 0;
    int c = (int)pushPackets.size();

    for (; i < c; ++i)
    {
        pushPackets[i]->Release();
    }
}

void
CRtpSessionWrapper::SetOutputRedline(size_t redlineBytes,   /* = 0 */
                                     size_t redlineFrames,  /* = 0 */
                                     size_t redlineDelayMs) /* = 0 */
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_bucket->SetRedline(redlineBytes, redlineFrames, redlineDelayMs);
}

void
CRtpSessionWrapper::GetOutputRedline(size_t* redlineBytes,         /* = NULL */
                                     size_t* redlineFrames,        /* = NULL */
                                     size_t* redlineDelayMs) const /* = NULL */
{
    if (redlineBytes != NULL)
    {
        *redlineBytes   = 0;
    }
    if (redlineFrames != NULL)
    {
        *redlineFrames  = 0;
    }
    if (redlineDelayMs != NULL)
    {
        *redlineDelayMs = 0;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_bucket != NULL)
        {
            m_bucket->GetRedline(redlineBytes, redlineFrames, redlineDelayMs);
        }
    }
}

void
CRtpSessionWrapper::GetFlowctrlInfo(float*  srcFrameRate,       /* = NULL */
                                    float*  srcBitRate,         /* = NULL */
                                    float*  outFrameRate,       /* = NULL */
                                    float*  outBitRate,         /* = NULL */
                                    size_t* cachedBytes,        /* = NULL */
                                    size_t* cachedFrames) const /* = NULL */
{
    if (srcFrameRate != NULL)
    {
        *srcFrameRate = 0;
    }
    if (srcBitRate != NULL)
    {
        *srcBitRate   = 0;
    }
    if (outFrameRate != NULL)
    {
        *outFrameRate = 0;
    }
    if (outBitRate != NULL)
    {
        *outBitRate   = 0;
    }
    if (cachedBytes != NULL)
    {
        *cachedBytes  = 0;
    }
    if (cachedFrames != NULL)
    {
        *cachedFrames = 0;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_bucket != NULL)
        {
            m_bucket->GetFlowctrlInfo(
                srcFrameRate,
                srcBitRate,
                outFrameRate,
                outBitRate,
                cachedBytes,
                cachedFrames
                );
        }
    }
}

void
CRtpSessionWrapper::ResetFlowctrlInfo()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_bucket->ResetFlowctrlInfo();
}

void
CRtpSessionWrapper::GetInputStat(float*    frameRate,       /* = NULL */
                                 float*    bitRate,         /* = NULL */
                                 float*    lossRate,        /* = NULL */
                                 uint64_t* lossCount) const /* = NULL */
{
    CProThreadMutexGuard mon(m_lock);

    if (frameRate != NULL)
    {
        *frameRate = (float)m_statFrameRateInput.CalcBitRate();
    }
    if (bitRate != NULL)
    {
        *bitRate   = (float)m_statBitRateInput.CalcBitRate();
    }
    if (lossRate != NULL)
    {
        *lossRate  = (float)m_statLossRateInput.CalcLossRate();
    }
    if (lossCount != NULL)
    {
        *lossCount = (uint64_t)m_statLossRateInput.CalcLossCount();
    }
}

void
CRtpSessionWrapper::GetOutputStat(float*    frameRate,       /* = NULL */
                                  float*    bitRate,         /* = NULL */
                                  float*    lossRate,        /* = NULL */
                                  uint64_t* lossCount) const /* = NULL */
{
    CProThreadMutexGuard mon(m_lock);

    if (frameRate != NULL)
    {
        *frameRate = (float)m_statFrameRateOutput.CalcBitRate();
    }
    if (bitRate != NULL)
    {
        *bitRate   = (float)m_statBitRateOutput.CalcBitRate();
    }
    if (lossRate != NULL)
    {
        *lossRate  = (float)m_statLossRateOutput.CalcLossRate();
    }
    if (lossCount != NULL)
    {
        *lossCount = (uint64_t)m_statLossRateOutput.CalcLossCount();
    }
}

void
CRtpSessionWrapper::ResetInputStat()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_statFrameRateInput.Reset();
    m_statBitRateInput.Reset();
    m_statLossRateInput.Reset();
}

void
CRtpSessionWrapper::ResetOutputStat()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
    {
        return;
    }

    m_statFrameRateOutput.Reset();
    m_statBitRateOutput.Reset();
    m_statLossRateOutput.Reset();
}

void
CRtpSessionWrapper::SetMagic(int64_t magic)
{
    CProThreadMutexGuard mon(m_lock);

    m_magic = magic;
}

int64_t
CRtpSessionWrapper::GetMagic() const
{
    int64_t magic = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        magic = m_magic;
    }

    return magic;
}

void
CRtpSessionWrapper::OnOkSession(IRtpSession* session)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer   = NULL;
    bool                 udpSession = false;

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

        m_session->GetInfo(&m_info);

        if (CRtpSessionBase::IsUdpSession(m_info.sessionType))
        {
            udpSession = true;

            /*
             * send all the cached packets
             */
            while (DoSendPacket())
            {
            }
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    m_onOkCalled = true;
    observer->OnOkSession(this);
    observer->Release();

    if (!udpSession)
    {
        RequestOnSend();
    }
}

void
CRtpSessionWrapper::OnRecvSession(IRtpSession* session,
                                  IRtpPacket*  packet)
{
    assert(session != NULL);
    assert(packet != NULL);
    if (session == NULL || packet == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

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

        if (!m_enableInput)
        {
            return;
        }

        if (
            packet->GetMarker()
            ||
            m_info.mmType < RTP_MMT_VIDEO_MIN || m_info.mmType > RTP_MMT_VIDEO_MAX /* non-video */
           )
        {
            m_statFrameRateInput.PushDataBits(1);
        }
        m_statBitRateInput.PushDataBytes(packet->GetPayloadSize());
        m_statLossRateInput.PushData(packet->GetSequence());

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnRecvSession(this, packet);
    observer->Release();
}

void
CRtpSessionWrapper::OnSendSession(IRtpSession* session,
                                  bool         packetErased)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

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

        m_outputPending = DoSendPacket();
        if (m_outputPending && !m_packetErased)
        {
            return;
        }

        packetErased = m_packetErased;
        m_packetErased = false;

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnSendSession(this, packetErased);
    observer->Release();
}

void
CRtpSessionWrapper::OnCloseSession(IRtpSession* session,
                                   int          errorCode,
                                   int          sslCode,
                                   bool         tcpConnected)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

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

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnCloseSession(this, errorCode, sslCode, tcpConnected);
    observer->Release();
}

void
CRtpSessionWrapper::OnHeartbeatSession(IRtpSession* session,
                                       int64_t      peerAliveTick)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

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

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnHeartbeatSession(this, peerAliveTick);
    observer->Release();
}

void
CRtpSessionWrapper::OnTimer(void*    factory,
                            uint64_t timerId,
                            int64_t  tick,
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

        if (m_observer == NULL || m_reactor == NULL || m_session == NULL || m_bucket == NULL)
        {
            return;
        }

        if (timerId == m_timerId)
        {
            do
            {{{
                if (tick - m_traceTick < TRACE_INTERVAL * 1000)
                {
                    break;
                }

                m_traceTick = tick;

                size_t redlineBytes   = 0;
                size_t redlineFrames  = 0;
                size_t redlineDelayMs = 0;
                m_bucket->GetRedline(&redlineBytes, &redlineFrames, &redlineDelayMs);

                char traceInfo[2048] = "";

                if (m_info.mmType >= RTP_MMT_MSG_MIN && m_info.mmType <= RTP_MMT_MSG_MAX)
                {
                    snprintf_pro(
                        traceInfo,
                        sizeof(traceInfo),
                        "\n"
                        " CRtpSessionWrapper(M) --- [pid : %u/0x%X, this : %p, session : %p, mmType : %u] \n"
                        "\t CRtpSessionWrapper(M) - redlineBytes        : %u \n"
                        "\t CRtpSessionWrapper(M) - bucketBytes         : %u \n"
                        "\t CRtpSessionWrapper(M) - pushToBucketRet1    : %d \n"
                        "\t CRtpSessionWrapper(M) - pushToBucketRet2    : %d \n"
                        "\t CRtpSessionWrapper(M) - packetErased        : %d \n"
                        "\t CRtpSessionWrapper(M) - enableInput         : %d \n"
                        "\t CRtpSessionWrapper(M) - enableOutput        : %d \n"
                        "\t CRtpSessionWrapper(M) - onOkCalled          : %d \n"
                        "\t CRtpSessionWrapper(M) - ... ... \n"
                        "\t CRtpSessionWrapper(M) - sendDuration(timer) : %u (ms) \n"
                        "\t CRtpSessionWrapper(M) - pushPackets (timer) : %u (packets) \n"
                        "\t CRtpSessionWrapper(M) - pushTick    (timer) : %lld \n"
                        "\t CRtpSessionWrapper(M) - tick        (timer) : %lld \n"
                        ,
                        (unsigned int)ProGetProcessId(),
                        (unsigned int)ProGetProcessId(),
                        this,
                        m_session,
                        (unsigned int)m_info.mmType,
                        (unsigned int)redlineBytes,
                        (unsigned int)m_bucket->GetTotalBytes(),
                        (int)(m_pushToBucketRet1 ? 1 : 0),
                        (int)(m_pushToBucketRet2 ? 1 : 0),
                        (int)(m_packetErased     ? 1 : 0),
                        (int)(m_enableInput      ? 1 : 0),
                        (int)(m_enableOutput     ? 1 : 0),
                        (int)(m_onOkCalled       ? 1 : 0),
                        (unsigned int)m_sendDurationMs,
                        (unsigned int)m_pushPackets.size(),
                        (long long)m_pushTick,
                        (long long)tick
                        );
#if defined(_WIN32)
                    ::OutputDebugStringA(traceInfo);
#else
                    printf("%s", traceInfo);
#endif
                }
                else if (m_info.mmType >= RTP_MMT_AUDIO_MIN && m_info.mmType <= RTP_MMT_AUDIO_MAX)
                {
                    snprintf_pro(
                        traceInfo,
                        sizeof(traceInfo),
                        "\n"
                        " CRtpSessionWrapper(A) --- [pid : %u/0x%X, this : %p, session : %p, mmType : %u] \n"
                        "\t CRtpSessionWrapper(A) - redlineBytes        : %u \n"
                        "\t CRtpSessionWrapper(A) - redlineDelayMs      : %u \n"
                        "\t CRtpSessionWrapper(A) - bucketBytes         : %u \n"
                        "\t CRtpSessionWrapper(A) - pushToBucketRet1    : %d \n"
                        "\t CRtpSessionWrapper(A) - pushToBucketRet2    : %d \n"
                        "\t CRtpSessionWrapper(A) - packetErased        : %d \n"
                        "\t CRtpSessionWrapper(A) - enableInput         : %d \n"
                        "\t CRtpSessionWrapper(A) - enableOutput        : %d \n"
                        "\t CRtpSessionWrapper(A) - onOkCalled          : %d \n"
                        "\t CRtpSessionWrapper(A) - ... ... \n"
                        "\t CRtpSessionWrapper(A) - sendDuration(timer) : %u (ms) \n"
                        "\t CRtpSessionWrapper(A) - pushPackets (timer) : %u (packets) \n"
                        "\t CRtpSessionWrapper(A) - pushTick    (timer) : %lld \n"
                        "\t CRtpSessionWrapper(A) - tick        (timer) : %lld \n"
                        ,
                        (unsigned int)ProGetProcessId(),
                        (unsigned int)ProGetProcessId(),
                        this,
                        m_session,
                        (unsigned int)m_info.mmType,
                        (unsigned int)redlineBytes,
                        (unsigned int)redlineDelayMs,
                        (unsigned int)m_bucket->GetTotalBytes(),
                        (int)(m_pushToBucketRet1 ? 1 : 0),
                        (int)(m_pushToBucketRet2 ? 1 : 0),
                        (int)(m_packetErased     ? 1 : 0),
                        (int)(m_enableInput      ? 1 : 0),
                        (int)(m_enableOutput     ? 1 : 0),
                        (int)(m_onOkCalled       ? 1 : 0),
                        (unsigned int)m_sendDurationMs,
                        (unsigned int)m_pushPackets.size(),
                        (long long)m_pushTick,
                        (long long)tick
                        );
#if defined(_WIN32)
                    ::OutputDebugStringA(traceInfo);
#else
                    printf("%s", traceInfo);
#endif
                }
                else if (m_info.mmType >= RTP_MMT_VIDEO_MIN && m_info.mmType <= RTP_MMT_VIDEO_MAX)
                {
                    snprintf_pro(
                        traceInfo,
                        sizeof(traceInfo),
                        "\n"
                        " CRtpSessionWrapper(V) --- [pid : %u/0x%X, this : %p, session : %p, mmType : %u] \n"
                        "\t CRtpSessionWrapper(V) - redlineBytes        : %u \n"
                        "\t CRtpSessionWrapper(V) - redlineFrames       : %u \n"
                        "\t CRtpSessionWrapper(V) - redlineDelayMs      : %u \n"
                        "\t CRtpSessionWrapper(V) - bucketBytes         : %u \n"
                        "\t CRtpSessionWrapper(V) - pushToBucketRet1    : %d \n"
                        "\t CRtpSessionWrapper(V) - pushToBucketRet2    : %d \n"
                        "\t CRtpSessionWrapper(V) - packetErased        : %d \n"
                        "\t CRtpSessionWrapper(V) - enableInput         : %d \n"
                        "\t CRtpSessionWrapper(V) - enableOutput        : %d \n"
                        "\t CRtpSessionWrapper(V) - onOkCalled          : %d \n"
                        "\t CRtpSessionWrapper(V) - ... ... \n"
                        "\t CRtpSessionWrapper(V) - sendDuration(timer) : %u (ms) \n"
                        "\t CRtpSessionWrapper(V) - pushPackets (timer) : %u (packets) \n"
                        "\t CRtpSessionWrapper(V) - pushTick    (timer) : %lld \n"
                        "\t CRtpSessionWrapper(V) - tick        (timer) : %lld \n"
                        ,
                        (unsigned int)ProGetProcessId(),
                        (unsigned int)ProGetProcessId(),
                        this,
                        m_session,
                        (unsigned int)m_info.mmType,
                        (unsigned int)redlineBytes,
                        (unsigned int)redlineFrames,
                        (unsigned int)redlineDelayMs,
                        (unsigned int)m_bucket->GetTotalBytes(),
                        (int)(m_pushToBucketRet1 ? 1 : 0),
                        (int)(m_pushToBucketRet2 ? 1 : 0),
                        (int)(m_packetErased     ? 1 : 0),
                        (int)(m_enableInput      ? 1 : 0),
                        (int)(m_enableOutput     ? 1 : 0),
                        (int)(m_onOkCalled       ? 1 : 0),
                        (unsigned int)m_sendDurationMs,
                        (unsigned int)m_pushPackets.size(),
                        (long long)m_pushTick,
                        (long long)tick
                        );
#if defined(_WIN32)
                    ::OutputDebugStringA(traceInfo);
#else
                    printf("%s", traceInfo);
#endif
                }
                else
                {
                }
            }}}
            while (0);
        }
        else if (timerId == m_sendTimerId)
        {
            int64_t sendDurationMs = m_pushTick + m_sendDurationMs - tick;
            if (sendDurationMs < 1)
            {
                sendDurationMs = 1;
            }

            int64_t maxSendCount = (m_pushPackets.size() + sendDurationMs / 2) / sendDurationMs; /* rounded */
            if (maxSendCount < 1)
            {
                maxSendCount = 1;
            }

            for (int i = 0; i < maxSendCount; ++i)
            {
                if (m_pushPackets.size() == 0)
                {
                    break;
                }

                IRtpPacket* packet = m_pushPackets.front();
                m_pushPackets.pop_front();
                PushPacket(packet);
                packet->Release();
            }
        }
        else
        {
        }
    }
}
