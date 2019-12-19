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

#include "rtp_session_base.h"
#include "rtp_base.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_file_monitor.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(_WIN32_WCE)
extern CProFileMonitor g_fileMonitor;
#endif

#if defined(__cplusplus)
}
#endif

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionBase::CRtpSessionBase(bool suspendRecv)
: m_suspendRecv(suspendRecv)
{
    m_magic          = 0;
    m_observer       = NULL;
    m_reactor        = NULL;
    m_trans          = NULL;
    m_dummySockId    = -1;
    m_actionId       = 0;
    m_initTick       = ProGetTickCount64();
    m_sendTick       = m_initTick;
    m_onSendTick1    = m_initTick;
    m_onSendTick2    = m_initTick; /* assert(m_onSendTick2 >= m_onSendTick1) */
    m_peerAliveTick  = m_initTick;
    m_timeoutTimerId = 0;
    m_onOkTimerId    = 0;
    m_tcpConnected   = false;
    m_handshakeOk    = false;
    m_onOkCalled     = false;
    m_bigPacket      = NULL;

    m_canUpcall      = true;

    memset(&m_info            , 0, sizeof(RTP_SESSION_INFO));
    memset(&m_ack             , 0, sizeof(RTP_SESSION_ACK));
    memset(&m_localAddr       , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddr      , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddrConfig, 0, sizeof(pbsd_sockaddr_in));
}

CRtpSessionBase::~CRtpSessionBase()
{
    if (m_bigPacket != NULL)
    {
        m_bigPacket->Release();
        m_bigPacket = NULL;
    }
}

unsigned long
PRO_CALLTYPE
CRtpSessionBase::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpSessionBase::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpSessionBase::GetInfo(RTP_SESSION_INFO* info) const
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
PRO_CALLTYPE
CRtpSessionBase::GetAck(RTP_SESSION_ACK* ack) const
{
    assert(ack != NULL);
    if (ack == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        *ack = m_ack;
    }
}

void
PRO_CALLTYPE
CRtpSessionBase::GetSyncId(unsigned char syncId[14]) const
{
    memset(syncId, 0, 14);
}

PRO_SSL_SUITE_ID
PRO_CALLTYPE
CRtpSessionBase::GetSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_trans != NULL)
        {
            suiteId = m_trans->GetSslSuite(suiteName);
        }
    }

    return (suiteId);
}

PRO_INT64
PRO_CALLTYPE
CRtpSessionBase::GetSockId() const
{
    PRO_INT64 sockId = -1;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_trans != NULL)
        {
            sockId = m_trans->GetSockId();
        }
    }

    return (sockId);
}

const char*
PRO_CALLTYPE
CRtpSessionBase::GetLocalIp(char localIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_localAddr.sin_addr.s_addr, localIp);
    }

    return (localIp);
}

unsigned short
PRO_CALLTYPE
CRtpSessionBase::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        localPort = pbsd_ntoh16(m_localAddr.sin_port);
    }

    return (localPort);
}

const char*
PRO_CALLTYPE
CRtpSessionBase::GetRemoteIp(char remoteIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_info.sessionType == RTP_ST_UDPCLIENT ||
            m_info.sessionType == RTP_ST_UDPSERVER)
        {
            if (m_remoteAddr.sin_addr.s_addr != 0)
            {
                pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);
            }
            else
            {
                pbsd_inet_ntoa(m_remoteAddrConfig.sin_addr.s_addr, remoteIp);
            }
        }
        else
        {
            pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);
        }
    }

    return (remoteIp);
}

unsigned short
PRO_CALLTYPE
CRtpSessionBase::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_info.sessionType == RTP_ST_UDPCLIENT ||
            m_info.sessionType == RTP_ST_UDPSERVER)
        {
            if (m_remoteAddr.sin_addr.s_addr != 0)
            {
                remotePort = pbsd_ntoh16(m_remoteAddr.sin_port);
            }
            else
            {
                remotePort = pbsd_ntoh16(m_remoteAddrConfig.sin_port);
            }
        }
        else
        {
            remotePort = pbsd_ntoh16(m_remoteAddr.sin_port);
        }
    }

    return (remotePort);
}

bool
PRO_CALLTYPE
CRtpSessionBase::IsTcpConnected() const
{
    bool connected = false;

    {
        CProThreadMutexGuard mon(m_lock);

        connected = m_tcpConnected;
    }

    return (connected);
}

bool
PRO_CALLTYPE
CRtpSessionBase::IsReady() const
{
    bool ready = false;

    {
        CProThreadMutexGuard mon(m_lock);

        ready = m_onOkCalled;
    }

    return (ready);
}

bool
PRO_CALLTYPE
CRtpSessionBase::SendPacket(IRtpPacket* packet,
                            bool*       tryAgain) /* = NULL */
{
    if (tryAgain != NULL)
    {
        *tryAgain = false;
    }

    assert(packet != NULL);
    if (packet == NULL)
    {
        return (false);
    }

    PRO_UINT16 otherSize = 0;

    switch (m_info.sessionType)
    {
    case RTP_ST_UDPCLIENT:
    case RTP_ST_UDPSERVER:
    case RTP_ST_MCAST:
        {
            otherSize = sizeof(RTP_HEADER);
            break;
        }
    case RTP_ST_TCPCLIENT:
    case RTP_ST_TCPSERVER:
        {
            otherSize = sizeof(PRO_UINT16) + sizeof(RTP_HEADER);
            break;
        }
    case RTP_ST_UDPCLIENT_EX:
    case RTP_ST_UDPSERVER_EX:
    case RTP_ST_MCAST_EX:
        {
            otherSize = sizeof(RTP_EXT) + sizeof(RTP_HEADER);
            break;
        }
    case RTP_ST_TCPCLIENT_EX:
    case RTP_ST_TCPSERVER_EX:
    case RTP_ST_SSLCLIENT_EX:
    case RTP_ST_SSLSERVER_EX:
        {
            assert(packet->GetPackMode() == m_info.packMode);
            if (packet->GetPackMode() != m_info.packMode)
            {
                return (false);
            }

            if (m_info.packMode == RTP_EPM_DEFAULT)
            {
                otherSize = sizeof(RTP_EXT) + sizeof(RTP_HEADER);
            }
            else if (m_info.packMode == RTP_EPM_TCP2)
            {
                otherSize = sizeof(PRO_UINT16);
            }
            else
            {
                otherSize = sizeof(PRO_UINT32);
            }
            break;
        }
    } /* end of switch (...) */

    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return (false);
        }

        if (!m_onOkCalled)
        {
            if (tryAgain != NULL)
            {
                *tryAgain = true;
            }

            return (false);
        }

        assert(
            m_info.outSrcMmId == 0 ||
            packet->GetMmId() == m_info.outSrcMmId
            );
        assert(packet->GetMmType() == m_info.mmType);
        if (
            (m_info.outSrcMmId != 0 && packet->GetMmId() != m_info.outSrcMmId)
            ||
            packet->GetMmType() != m_info.mmType
           )
        {
            return (false);
        }

        if (m_info.sessionType == RTP_ST_UDPCLIENT ||
            m_info.sessionType == RTP_ST_UDPSERVER)
        {
            if (m_remoteAddr.sin_addr.s_addr != 0)
            {
                ret = m_trans->SendData(
                    (char*)packet->GetPayloadBuffer() - otherSize,
                    packet->GetPayloadSize() + otherSize,
                    m_actionId + 1,
                    &m_remoteAddr
                    );
                if (!ret && tryAgain != NULL)
                {
                    *tryAgain = true;
                }
            }
            else if (m_remoteAddrConfig.sin_addr.s_addr != 0)
            {
                ret = m_trans->SendData(
                    (char*)packet->GetPayloadBuffer() - otherSize,
                    packet->GetPayloadSize() + otherSize,
                    m_actionId + 1,
                    &m_remoteAddrConfig
                    );
                if (!ret && tryAgain != NULL)
                {
                    *tryAgain = true;
                }
            }
            else
            {
            }
        }
        else
        {
            ret = m_trans->SendData(
                (char*)packet->GetPayloadBuffer() - otherSize,
                packet->GetPayloadSize() + otherSize,
                m_actionId + 1,
                &m_remoteAddr
                );
            if (!ret && tryAgain != NULL)
            {
                *tryAgain = true;
            }
        }

        m_sendTick = ProGetTickCount64();

        if (ret)
        {
            ++m_actionId;

            if (m_info.sessionType == RTP_ST_TCPCLIENT    ||
                m_info.sessionType == RTP_ST_TCPSERVER    ||
                m_info.sessionType == RTP_ST_TCPCLIENT_EX ||
                m_info.sessionType == RTP_ST_TCPSERVER_EX ||
                m_info.sessionType == RTP_ST_SSLCLIENT_EX ||
                m_info.sessionType == RTP_ST_SSLSERVER_EX)
            {
                m_onSendTick1 = m_sendTick;
                m_onSendTick2 = m_sendTick - 1; /* assert(m_onSendTick2 < m_onSendTick1) */
            }
        }
    }

    return (ret);
}

void
PRO_CALLTYPE
CRtpSessionBase::GetSendOnSendTick(PRO_INT64* onSendTick1,       /* = NULL */
                                   PRO_INT64* onSendTick2) const /* = NULL */
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (onSendTick1 != NULL)
        {
            *onSendTick1 = m_onSendTick1;
        }
        if (onSendTick2 != NULL)
        {
            *onSendTick2 = m_onSendTick2;
        }
    }
}

void
PRO_CALLTYPE
CRtpSessionBase::RequestOnSend()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_trans->RequestOnSend();
    }
}

void
PRO_CALLTYPE
CRtpSessionBase::SuspendRecv()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_trans->SuspendRecv();
    }
}

void
PRO_CALLTYPE
CRtpSessionBase::ResumeRecv()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_trans->ResumeRecv();
    }
}

void
PRO_CALLTYPE
CRtpSessionBase::SetMagic(PRO_INT64 magic)
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_magic = magic;
    }
}

PRO_INT64
PRO_CALLTYPE
CRtpSessionBase::GetMagic() const
{
    PRO_INT64 magic = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        magic = m_magic;
    }

    return (magic);
}

void
PRO_CALLTYPE
CRtpSessionBase::OnSend(IProTransport* trans,
                        PRO_UINT64     actionId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (trans != m_trans)
        {
            return;
        }

        if (!m_onOkCalled)
        {
            return;
        }

        switch (m_info.sessionType)
        {
        case RTP_ST_TCPCLIENT:
        case RTP_ST_TCPSERVER:
        case RTP_ST_TCPCLIENT_EX:
        case RTP_ST_TCPSERVER_EX:
        case RTP_ST_SSLCLIENT_EX:
        case RTP_ST_SSLSERVER_EX:
            {
                if (actionId > 0 && actionId == m_actionId)
                {
                    m_onSendTick2 = ProGetTickCount64();
                }
                break;
            }
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        observer->OnSendSession(this, false);
    }

    observer->Release();
}}

void
PRO_CALLTYPE
CRtpSessionBase::OnClose(IProTransport* trans,
                         long           errorCode,
                         long           sslCode)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (trans != m_trans)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, errorCode, sslCode, m_tcpConnected);
    }

    observer->Release();

    Fini();
}}

void
PRO_CALLTYPE
CRtpSessionBase::OnHeartbeat(IProTransport* trans)
{{
#if !defined(_WIN32_WCE)
    bool enableTrace = false;
    if (
        (m_info.mmType >= RTP_MMT_MSG_MIN &&
         m_info.mmType <= RTP_MMT_MSG_MAX)
        ||
        (m_info.mmType >= RTP_MMT_AUDIO_MIN &&
         m_info.mmType <= RTP_MMT_AUDIO_MAX)
        ||
        (m_info.mmType >= RTP_MMT_VIDEO_MIN &&
         m_info.mmType <= RTP_MMT_VIDEO_MAX)
       )
    {
        enableTrace = g_fileMonitor.QueryFileExist();
    }
#endif

    CProThreadMutexGuard mon(m_lockUpcall);

    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer      = NULL;
    PRO_INT64            peerAliveTick = 0;
    const PRO_INT64      tick          = ProGetTickCount64();

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (trans != m_trans)
        {
            return;
        }

#if !defined(_WIN32_WCE)
        do
        {
            if (!enableTrace)
            {
                break;
            }

            char suiteName[64] = "";
            m_trans->GetSslSuite(suiteName);

            char localIp[64]        = "";
            char remoteIp[64]       = "";
            char remoteIpConfig[64] = "";
            pbsd_inet_ntoa(m_localAddr.sin_addr.s_addr       , localIp);
            pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr      , remoteIp);
            pbsd_inet_ntoa(m_remoteAddrConfig.sin_addr.s_addr, remoteIpConfig);

            const unsigned long size   = 1024 * 8;
            char* const         buffer = (char*)ProMalloc(size);
            if (buffer == NULL)
            {
                break;
            }

            buffer[0] = '\0';

            if (m_info.mmType >= RTP_MMT_MSG_MIN &&
                m_info.mmType <= RTP_MMT_MSG_MAX)
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(M) --- [pid : %u/0x%X, this : %p, mmType : %u] \n"
                    "\t CRtpSessionBase(M) - localVersion     : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - remoteVersion    : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - sessionType      : %u \n"
                    "\t CRtpSessionBase(M) - packMode         : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - sslSuiteName     : %s (for ssl_ex) \n"
                    "\t CRtpSessionBase(M) - someId           : %u \n"
                    "\t CRtpSessionBase(M) - mmId             : %u \n"
                    "\t CRtpSessionBase(M) - inSrcMmId        : %u \n"
                    "\t CRtpSessionBase(M) - outSrcMmId       : %u \n"
                    "\t CRtpSessionBase(M) - localAddr        : %s:%u \n"
                    "\t CRtpSessionBase(M) - remoteAddr       : %s:%u \n"
                    "\t CRtpSessionBase(M) - remoteAddrConfig : %s:%u (for udp) \n"
                    "\t CRtpSessionBase(M) - actionId         : " PRO_PRT64U " \n"
                    "\t CRtpSessionBase(M) - initTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - sendTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - onSendTick1      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - onSendTick2      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(M) - canUpcall        : %d \n"
                    ,
                    (unsigned int)ProGetProcessId(),
                    (unsigned int)ProGetProcessId(),
                    this,
                    (unsigned int)m_info.mmType,
                    (unsigned int)m_info.localVersion,
                    (unsigned int)m_info.remoteVersion,
                    (unsigned int)m_info.sessionType,
                    (unsigned int)m_info.packMode,
                    suiteName,
                    (unsigned int)m_info.someId,
                    (unsigned int)m_info.mmId,
                    (unsigned int)m_info.inSrcMmId,
                    (unsigned int)m_info.outSrcMmId,
                    localIp,
                    (unsigned int)pbsd_ntoh16(m_localAddr.sin_port),
                    remoteIp,
                    (unsigned int)pbsd_ntoh16(m_remoteAddr.sin_port),
                    remoteIpConfig,
                    (unsigned int)pbsd_ntoh16(m_remoteAddrConfig.sin_port),
                    m_actionId,
                    m_initTick,
                    m_sendTick,
                    m_onSendTick1,
                    m_onSendTick2,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
#if defined(WIN32)
                ::OutputDebugString(buffer);
#else
                printf("%s", buffer);
#endif
            }
            else if (
                m_info.mmType >= RTP_MMT_AUDIO_MIN &&
                m_info.mmType <= RTP_MMT_AUDIO_MAX
                )
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(A) --- [pid : %u/0x%X, this : %p, mmType : %u] \n"
                    "\t CRtpSessionBase(A) - localVersion     : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - remoteVersion    : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - sessionType      : %u \n"
                    "\t CRtpSessionBase(A) - packMode         : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - sslSuiteName     : %s (for ssl_ex) \n"
                    "\t CRtpSessionBase(A) - someId           : %u \n"
                    "\t CRtpSessionBase(A) - mmId             : %u \n"
                    "\t CRtpSessionBase(A) - inSrcMmId        : %u \n"
                    "\t CRtpSessionBase(A) - outSrcMmId       : %u \n"
                    "\t CRtpSessionBase(A) - localAddr        : %s:%u \n"
                    "\t CRtpSessionBase(A) - remoteAddr       : %s:%u \n"
                    "\t CRtpSessionBase(A) - remoteAddrConfig : %s:%u (for udp) \n"
                    "\t CRtpSessionBase(A) - actionId         : " PRO_PRT64U " \n"
                    "\t CRtpSessionBase(A) - initTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - sendTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - onSendTick1      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - onSendTick2      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(A) - canUpcall        : %d \n"
                    ,
                    (unsigned int)ProGetProcessId(),
                    (unsigned int)ProGetProcessId(),
                    this,
                    (unsigned int)m_info.mmType,
                    (unsigned int)m_info.localVersion,
                    (unsigned int)m_info.remoteVersion,
                    (unsigned int)m_info.sessionType,
                    (unsigned int)m_info.packMode,
                    suiteName,
                    (unsigned int)m_info.someId,
                    (unsigned int)m_info.mmId,
                    (unsigned int)m_info.inSrcMmId,
                    (unsigned int)m_info.outSrcMmId,
                    localIp,
                    (unsigned int)pbsd_ntoh16(m_localAddr.sin_port),
                    remoteIp,
                    (unsigned int)pbsd_ntoh16(m_remoteAddr.sin_port),
                    remoteIpConfig,
                    (unsigned int)pbsd_ntoh16(m_remoteAddrConfig.sin_port),
                    m_actionId,
                    m_initTick,
                    m_sendTick,
                    m_onSendTick1,
                    m_onSendTick2,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
#if defined(WIN32)
                ::OutputDebugString(buffer);
#else
                printf("%s", buffer);
#endif
            }
            else if (
                m_info.mmType >= RTP_MMT_VIDEO_MIN &&
                m_info.mmType <= RTP_MMT_VIDEO_MAX
                )
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(V) --- [pid : %u/0x%X, this : %p, mmType : %u] \n"
                    "\t CRtpSessionBase(V) - localVersion     : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - remoteVersion    : %u (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - sessionType      : %u \n"
                    "\t CRtpSessionBase(V) - packMode         : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - sslSuiteName     : %s (for ssl_ex) \n"
                    "\t CRtpSessionBase(V) - someId           : %u \n"
                    "\t CRtpSessionBase(V) - mmId             : %u \n"
                    "\t CRtpSessionBase(V) - inSrcMmId        : %u \n"
                    "\t CRtpSessionBase(V) - outSrcMmId       : %u \n"
                    "\t CRtpSessionBase(V) - localAddr        : %s:%u \n"
                    "\t CRtpSessionBase(V) - remoteAddr       : %s:%u \n"
                    "\t CRtpSessionBase(V) - remoteAddrConfig : %s:%u (for udp) \n"
                    "\t CRtpSessionBase(V) - actionId         : " PRO_PRT64U " \n"
                    "\t CRtpSessionBase(V) - initTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - sendTick         : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - onSendTick1      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - onSendTick2      : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(V) - canUpcall        : %d \n"
                    ,
                    (unsigned int)ProGetProcessId(),
                    (unsigned int)ProGetProcessId(),
                    this,
                    (unsigned int)m_info.mmType,
                    (unsigned int)m_info.localVersion,
                    (unsigned int)m_info.remoteVersion,
                    (unsigned int)m_info.sessionType,
                    (unsigned int)m_info.packMode,
                    suiteName,
                    (unsigned int)m_info.someId,
                    (unsigned int)m_info.mmId,
                    (unsigned int)m_info.inSrcMmId,
                    (unsigned int)m_info.outSrcMmId,
                    localIp,
                    (unsigned int)pbsd_ntoh16(m_localAddr.sin_port),
                    remoteIp,
                    (unsigned int)pbsd_ntoh16(m_remoteAddr.sin_port),
                    remoteIpConfig,
                    (unsigned int)pbsd_ntoh16(m_remoteAddrConfig.sin_port),
                    m_actionId,
                    m_initTick,
                    m_sendTick,
                    m_onSendTick1,
                    m_onSendTick2,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
#if defined(WIN32)
                ::OutputDebugString(buffer);
#else
                printf("%s", buffer);
#endif
            }
            else
            {
            }

            void*  freeList[64];
            size_t objSize[64];
            size_t busyObjNum[64];
            size_t totalObjNum[64];
            size_t heapBytes;

            {
                ProGetSgiPoolInfo(
                    freeList, objSize, busyObjNum, totalObjNum, &heapBytes, 0);
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(POOL-0) --- [pid : %u/0x%X, this : %p] \n"
                    "\t CRtpSessionBase(POOL-0) - [00] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [01] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [02] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [03] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [04] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [05] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [06] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [07] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [08] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [09] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [10] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [11] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [12] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [13] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [14] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [15] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [16] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [17] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [18] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [19] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [20] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [21] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [22] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [23] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [24] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [25] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [26] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [27] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [28] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [29] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [30] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [31] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [32] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [33] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [34] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [35] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [36] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [37] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [38] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [39] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [40] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [41] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [42] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [43] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [44] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [45] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [46] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [47] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [48] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [49] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [50] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [51] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [52] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [53] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [54] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [55] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [56] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [57] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [58] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [59] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [60] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [61] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [62] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - [63] : %p, [%u], *%u/%u \n"
                    "\t CRtpSessionBase(POOL-0) - size : %u \n"
                    ,
                    (unsigned int)ProGetProcessId(),
                    (unsigned int)ProGetProcessId(),
                    this,
                    freeList[0] , (unsigned int)objSize[0] , (unsigned int)busyObjNum[0] , (unsigned int)totalObjNum[0] ,
                    freeList[1] , (unsigned int)objSize[1] , (unsigned int)busyObjNum[1] , (unsigned int)totalObjNum[1] ,
                    freeList[2] , (unsigned int)objSize[2] , (unsigned int)busyObjNum[2] , (unsigned int)totalObjNum[2] ,
                    freeList[3] , (unsigned int)objSize[3] , (unsigned int)busyObjNum[3] , (unsigned int)totalObjNum[3] ,
                    freeList[4] , (unsigned int)objSize[4] , (unsigned int)busyObjNum[4] , (unsigned int)totalObjNum[4] ,
                    freeList[5] , (unsigned int)objSize[5] , (unsigned int)busyObjNum[5] , (unsigned int)totalObjNum[5] ,
                    freeList[6] , (unsigned int)objSize[6] , (unsigned int)busyObjNum[6] , (unsigned int)totalObjNum[6] ,
                    freeList[7] , (unsigned int)objSize[7] , (unsigned int)busyObjNum[7] , (unsigned int)totalObjNum[7] ,
                    freeList[8] , (unsigned int)objSize[8] , (unsigned int)busyObjNum[8] , (unsigned int)totalObjNum[8] ,
                    freeList[9] , (unsigned int)objSize[9] , (unsigned int)busyObjNum[9] , (unsigned int)totalObjNum[9] ,
                    freeList[10], (unsigned int)objSize[10], (unsigned int)busyObjNum[10], (unsigned int)totalObjNum[10],
                    freeList[11], (unsigned int)objSize[11], (unsigned int)busyObjNum[11], (unsigned int)totalObjNum[11],
                    freeList[12], (unsigned int)objSize[12], (unsigned int)busyObjNum[12], (unsigned int)totalObjNum[12],
                    freeList[13], (unsigned int)objSize[13], (unsigned int)busyObjNum[13], (unsigned int)totalObjNum[13],
                    freeList[14], (unsigned int)objSize[14], (unsigned int)busyObjNum[14], (unsigned int)totalObjNum[14],
                    freeList[15], (unsigned int)objSize[15], (unsigned int)busyObjNum[15], (unsigned int)totalObjNum[15],
                    freeList[16], (unsigned int)objSize[16], (unsigned int)busyObjNum[16], (unsigned int)totalObjNum[16],
                    freeList[17], (unsigned int)objSize[17], (unsigned int)busyObjNum[17], (unsigned int)totalObjNum[17],
                    freeList[18], (unsigned int)objSize[18], (unsigned int)busyObjNum[18], (unsigned int)totalObjNum[18],
                    freeList[19], (unsigned int)objSize[19], (unsigned int)busyObjNum[19], (unsigned int)totalObjNum[19],
                    freeList[20], (unsigned int)objSize[20], (unsigned int)busyObjNum[20], (unsigned int)totalObjNum[20],
                    freeList[21], (unsigned int)objSize[21], (unsigned int)busyObjNum[21], (unsigned int)totalObjNum[21],
                    freeList[22], (unsigned int)objSize[22], (unsigned int)busyObjNum[22], (unsigned int)totalObjNum[22],
                    freeList[23], (unsigned int)objSize[23], (unsigned int)busyObjNum[23], (unsigned int)totalObjNum[23],
                    freeList[24], (unsigned int)objSize[24], (unsigned int)busyObjNum[24], (unsigned int)totalObjNum[24],
                    freeList[25], (unsigned int)objSize[25], (unsigned int)busyObjNum[25], (unsigned int)totalObjNum[25],
                    freeList[26], (unsigned int)objSize[26], (unsigned int)busyObjNum[26], (unsigned int)totalObjNum[26],
                    freeList[27], (unsigned int)objSize[27], (unsigned int)busyObjNum[27], (unsigned int)totalObjNum[27],
                    freeList[28], (unsigned int)objSize[28], (unsigned int)busyObjNum[28], (unsigned int)totalObjNum[28],
                    freeList[29], (unsigned int)objSize[29], (unsigned int)busyObjNum[29], (unsigned int)totalObjNum[29],
                    freeList[30], (unsigned int)objSize[30], (unsigned int)busyObjNum[30], (unsigned int)totalObjNum[30],
                    freeList[31], (unsigned int)objSize[31], (unsigned int)busyObjNum[31], (unsigned int)totalObjNum[31],
                    freeList[32], (unsigned int)objSize[32], (unsigned int)busyObjNum[32], (unsigned int)totalObjNum[32],
                    freeList[33], (unsigned int)objSize[33], (unsigned int)busyObjNum[33], (unsigned int)totalObjNum[33],
                    freeList[34], (unsigned int)objSize[34], (unsigned int)busyObjNum[34], (unsigned int)totalObjNum[34],
                    freeList[35], (unsigned int)objSize[35], (unsigned int)busyObjNum[35], (unsigned int)totalObjNum[35],
                    freeList[36], (unsigned int)objSize[36], (unsigned int)busyObjNum[36], (unsigned int)totalObjNum[36],
                    freeList[37], (unsigned int)objSize[37], (unsigned int)busyObjNum[37], (unsigned int)totalObjNum[37],
                    freeList[38], (unsigned int)objSize[38], (unsigned int)busyObjNum[38], (unsigned int)totalObjNum[38],
                    freeList[39], (unsigned int)objSize[39], (unsigned int)busyObjNum[39], (unsigned int)totalObjNum[39],
                    freeList[40], (unsigned int)objSize[40], (unsigned int)busyObjNum[40], (unsigned int)totalObjNum[40],
                    freeList[41], (unsigned int)objSize[41], (unsigned int)busyObjNum[41], (unsigned int)totalObjNum[41],
                    freeList[42], (unsigned int)objSize[42], (unsigned int)busyObjNum[42], (unsigned int)totalObjNum[42],
                    freeList[43], (unsigned int)objSize[43], (unsigned int)busyObjNum[43], (unsigned int)totalObjNum[43],
                    freeList[44], (unsigned int)objSize[44], (unsigned int)busyObjNum[44], (unsigned int)totalObjNum[44],
                    freeList[45], (unsigned int)objSize[45], (unsigned int)busyObjNum[45], (unsigned int)totalObjNum[45],
                    freeList[46], (unsigned int)objSize[46], (unsigned int)busyObjNum[46], (unsigned int)totalObjNum[46],
                    freeList[47], (unsigned int)objSize[47], (unsigned int)busyObjNum[47], (unsigned int)totalObjNum[47],
                    freeList[48], (unsigned int)objSize[48], (unsigned int)busyObjNum[48], (unsigned int)totalObjNum[48],
                    freeList[49], (unsigned int)objSize[49], (unsigned int)busyObjNum[49], (unsigned int)totalObjNum[49],
                    freeList[50], (unsigned int)objSize[50], (unsigned int)busyObjNum[50], (unsigned int)totalObjNum[50],
                    freeList[51], (unsigned int)objSize[51], (unsigned int)busyObjNum[51], (unsigned int)totalObjNum[51],
                    freeList[52], (unsigned int)objSize[52], (unsigned int)busyObjNum[52], (unsigned int)totalObjNum[52],
                    freeList[53], (unsigned int)objSize[53], (unsigned int)busyObjNum[53], (unsigned int)totalObjNum[53],
                    freeList[54], (unsigned int)objSize[54], (unsigned int)busyObjNum[54], (unsigned int)totalObjNum[54],
                    freeList[55], (unsigned int)objSize[55], (unsigned int)busyObjNum[55], (unsigned int)totalObjNum[55],
                    freeList[56], (unsigned int)objSize[56], (unsigned int)busyObjNum[56], (unsigned int)totalObjNum[56],
                    freeList[57], (unsigned int)objSize[57], (unsigned int)busyObjNum[57], (unsigned int)totalObjNum[57],
                    freeList[58], (unsigned int)objSize[58], (unsigned int)busyObjNum[58], (unsigned int)totalObjNum[58],
                    freeList[59], (unsigned int)objSize[59], (unsigned int)busyObjNum[59], (unsigned int)totalObjNum[59],
                    freeList[60], (unsigned int)objSize[60], (unsigned int)busyObjNum[60], (unsigned int)totalObjNum[60],
                    freeList[61], (unsigned int)objSize[61], (unsigned int)busyObjNum[61], (unsigned int)totalObjNum[61],
                    freeList[62], (unsigned int)objSize[62], (unsigned int)busyObjNum[62], (unsigned int)totalObjNum[62],
                    freeList[63], (unsigned int)objSize[63], (unsigned int)busyObjNum[63], (unsigned int)totalObjNum[63],
                    (unsigned int)heapBytes
                    );
#if defined(WIN32)
                    ::OutputDebugString(buffer);
#else
                    printf("%s", buffer);
#endif
            }

            ProFree(buffer);
        }
        while (0);
#endif /* _WIN32_WCE */

        if (!m_onOkCalled)
        {
            return;
        }

        /*
         * send a heartbeat packet
         */
        switch (m_info.sessionType)
        {
        case RTP_ST_TCPCLIENT:
        case RTP_ST_TCPSERVER:
            {
                PRO_UINT16 zero = 0;
                m_trans->SendData(&zero, sizeof(PRO_UINT16));

                return;
            }
        case RTP_ST_UDPCLIENT_EX:
        case RTP_ST_UDPSERVER_EX:
            {
                RTP_EXT ext;
                memset(&ext, 0, sizeof(RTP_EXT));
                ext.mmType = m_info.mmType;
                m_trans->SendData(&ext, sizeof(RTP_EXT));
                break;
            }
        case RTP_ST_TCPCLIENT_EX:
        case RTP_ST_TCPSERVER_EX:
        case RTP_ST_SSLCLIENT_EX:
        case RTP_ST_SSLSERVER_EX:
            {
                if (m_info.packMode == RTP_EPM_DEFAULT)
                {
                    RTP_EXT ext;
                    memset(&ext, 0, sizeof(RTP_EXT));
                    ext.mmType = m_info.mmType;
                    m_trans->SendData(&ext, sizeof(RTP_EXT));
                }
                else if (m_info.packMode == RTP_EPM_TCP2)
                {
                    PRO_UINT16 zero = 0;
                    m_trans->SendData(&zero, sizeof(PRO_UINT16));
                }
                else
                {
                    PRO_UINT32 zero = 0;
                    m_trans->SendData(&zero, sizeof(PRO_UINT32));
                }
                break;
            }
        default:
            {
                return;
            }
        } /* end of switch (...) */

        peerAliveTick = m_peerAliveTick;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        /*
         * a timeout occured?
         */
        if (tick - peerAliveTick >=
            (PRO_INT64)GetRtpKeepaliveTimeout() * 1000)
        {
            m_canUpcall = false;
            observer->OnCloseSession(this, PBSD_ETIMEDOUT, 0, m_tcpConnected);
        }
        else
        {
            observer->OnHeartbeatSession(this, peerAliveTick);
        }
    }

    observer->Release();

    if (!m_canUpcall)
    {
        Fini();
    }
}}

void
PRO_CALLTYPE
CRtpSessionBase::OnTimer(void*      factory,
                         PRO_UINT64 timerId,
                         PRO_INT64  userData)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;
    bool                 timeout  = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        if (timerId != m_timeoutTimerId && timerId != m_onOkTimerId)
        {
            return;
        }

        if (timerId == m_timeoutTimerId)
        {
            m_reactor->CancelTimer(m_timeoutTimerId);
            m_timeoutTimerId = 0;

            timeout = true;
        }
        else
        {
            m_reactor->CancelTimer(m_onOkTimerId);
            m_onOkTimerId = 0;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (timeout)
        {
            m_canUpcall = false;
            observer->OnCloseSession(this, PBSD_ETIMEDOUT, 0, m_tcpConnected);
        }
        else
        {
            if (!m_onOkCalled)
            {
                m_onOkCalled = true;
                observer->OnOkSession(this);
            }
        }
    }

    observer->Release();

    if (!m_canUpcall)
    {
        Fini();
    }
}}
