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

#include "rtp_session_base.h"
#include "rtp_framework.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_file_monitor.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
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

#if defined(WIN32) && !defined(_WIN32_WCE)
extern CProFileMonitor g_fileMonitor;
#endif

#if defined(__cplusplus)
}
#endif

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionBase::CRtpSessionBase()
{
    m_observer       = NULL;
    m_reactor        = NULL;
    m_trans          = NULL;
    m_dummySockId    = -1;
    m_actionId       = 0;
    m_initTick       = ProGetTickCount64();
    m_sendingTick    = m_initTick;
    m_sendTick       = m_initTick;
    m_onSendTick     = m_initTick; /* assert(m_onSendTick >= m_sendTick) */
    m_peerAliveTick  = m_initTick;
    m_timeoutTimerId = 0;
    m_onOkTimerId    = 0;
    m_tcpConnected   = false;
    m_handshakeOk    = false;
    m_onOkCalled     = false;

    m_canUpcall      = true;

    memset(&m_info            , 0, sizeof(RTP_SESSION_INFO));
    memset(&m_localAddr       , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddr      , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddrConfig, 0, sizeof(pbsd_sockaddr_in));
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

        if (m_info.sessionType == RTP_ST_UDPCLIENT || m_info.sessionType == RTP_ST_UDPSERVER)
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

        if (m_info.sessionType == RTP_ST_UDPCLIENT || m_info.sessionType == RTP_ST_UDPSERVER)
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
CRtpSessionBase::SendPacket(IRtpPacket* packet)
{
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
    }

    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return (false);
        }

        if (!m_onOkCalled)
        {
            return (false);
        }

        assert(m_info.outSrcMmId == 0 || packet->GetMmId() == m_info.outSrcMmId);
        assert(packet->GetMmType() == m_info.mmType);
        if (m_info.outSrcMmId != 0 && packet->GetMmId() != m_info.outSrcMmId
            ||
            packet->GetMmType() != m_info.mmType)
        {
            return (false);
        }

        if (m_info.sessionType == RTP_ST_UDPCLIENT || m_info.sessionType == RTP_ST_UDPSERVER)
        {
            if (m_remoteAddr.sin_addr.s_addr != 0)
            {
                ret = m_trans->SendData(
                    (char*)packet->GetPayloadBuffer() - otherSize,
                    packet->GetPayloadSize() + otherSize,
                    m_actionId + 1,
                    &m_remoteAddr
                    );
            }
            else if (m_remoteAddrConfig.sin_addr.s_addr != 0)
            {
                ret = m_trans->SendData(
                    (char*)packet->GetPayloadBuffer() - otherSize,
                    packet->GetPayloadSize() + otherSize,
                    m_actionId + 1,
                    &m_remoteAddrConfig
                    );
            }
            else
            {
                ret = true; /* drop this packet */
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
        }

        m_sendingTick = ProGetTickCount64();

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
                m_sendTick   = m_sendingTick;
                m_onSendTick = m_sendingTick - 1; /* assert(m_onSendTick < m_sendTick) */
            }
        }
    }

    return (ret);
}

void
PRO_CALLTYPE
CRtpSessionBase::GetSendOnSendTick(PRO_INT64* sendTick,         /* = NULL */
                                   PRO_INT64* onSendTick) const /* = NULL */
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (sendTick != NULL)
        {
            *sendTick   = m_sendTick;
        }
        if (onSendTick != NULL)
        {
            *onSendTick = m_onSendTick;
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
                    m_onSendTick = ProGetTickCount64();
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

        m_trans = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, errorCode, sslCode, m_tcpConnected);
    }

    observer->Release();
    ProDeleteTransport(trans);
}}

void
PRO_CALLTYPE
CRtpSessionBase::OnHeartbeat(IProTransport* trans)
{{
#if defined(WIN32) && !defined(_WIN32_WCE)
    bool enableTrace = false;
    if (m_info.mmType >= RTP_MMT_MSG_MIN   && m_info.mmType <= RTP_MMT_MSG_MAX   ||
        m_info.mmType >= RTP_MMT_AUDIO_MIN && m_info.mmType <= RTP_MMT_AUDIO_MAX ||
        m_info.mmType >= RTP_MMT_VIDEO_MIN && m_info.mmType <= RTP_MMT_VIDEO_MAX)
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

    char suiteName[64] = "";
    trans->GetSslSuite(suiteName);

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

        const PRO_INT64 tick = ProGetTickCount64();

#if defined(WIN32) && !defined(_WIN32_WCE)
        do
        {
            if (!enableTrace)
            {
                break;
            }

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

            buffer[size - 1] = '\0';

            if (m_info.mmType >= RTP_MMT_MSG_MIN && m_info.mmType <= RTP_MMT_MSG_MAX)
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(M) --- [pid : %u/0x%X, this : 0x%p, mmType : %u] \n"
                    "\t CRtpSessionBase(M) - localVersion     : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - remoteVersion    : %u (for tcp_ex, ssl_ex) \n"
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
                    "\t CRtpSessionBase(M) - sendingTick      : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - sendTick         : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - onSendTick       : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(M) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(M) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(M) - canUpcall        : %d \n"
                    ,
                    (unsigned int)::GetCurrentProcessId(),
                    (unsigned int)::GetCurrentProcessId(),
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
                    m_sendingTick,
                    m_sendTick,
                    m_onSendTick,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
                ::OutputDebugString(buffer);
            }
            else if (m_info.mmType >= RTP_MMT_AUDIO_MIN && m_info.mmType <= RTP_MMT_AUDIO_MAX)
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(A) --- [pid : %u/0x%X, this : 0x%p, mmType : %u] \n"
                    "\t CRtpSessionBase(A) - localVersion     : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - remoteVersion    : %u (for tcp_ex, ssl_ex) \n"
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
                    "\t CRtpSessionBase(A) - sendingTick      : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - sendTick         : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - onSendTick       : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(A) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(A) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(A) - canUpcall        : %d \n"
                    ,
                    (unsigned int)::GetCurrentProcessId(),
                    (unsigned int)::GetCurrentProcessId(),
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
                    m_sendingTick,
                    m_sendTick,
                    m_onSendTick,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
                ::OutputDebugString(buffer);
            }
            else if (m_info.mmType >= RTP_MMT_VIDEO_MIN && m_info.mmType <= RTP_MMT_VIDEO_MAX)
            {
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(V) --- [pid : %u/0x%X, this : 0x%p, mmType : %u] \n"
                    "\t CRtpSessionBase(V) - localVersion     : %u (for tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - remoteVersion    : %u (for tcp_ex, ssl_ex) \n"
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
                    "\t CRtpSessionBase(V) - sendingTick      : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - sendTick         : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - onSendTick       : " PRO_PRT64D " (for tcp, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - peerAliveTick    : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - tick             : " PRO_PRT64D " \n"
                    "\t CRtpSessionBase(V) - tcpConnected     : %d (for tcp   , tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - handshakeOk      : %d (for udp_ex, tcp_ex, ssl_ex) \n"
                    "\t CRtpSessionBase(V) - onOkCalled       : %d \n"
                    "\t CRtpSessionBase(V) - canUpcall        : %d \n"
                    ,
                    (unsigned int)::GetCurrentProcessId(),
                    (unsigned int)::GetCurrentProcessId(),
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
                    m_sendingTick,
                    m_sendTick,
                    m_onSendTick,
                    m_peerAliveTick,
                    tick,
                    (int)(m_tcpConnected ? 1 : 0),
                    (int)(m_handshakeOk  ? 1 : 0),
                    (int)(m_onOkCalled   ? 1 : 0),
                    (int)(m_canUpcall    ? 1 : 0)
                    );
                ::OutputDebugString(buffer);
            }
            else
            {
            }

            void*  freeList[64];
            size_t objSize[64];
            size_t heapSize;

            {
                ProGetSgiPoolInfo(freeList, objSize, &heapSize, 0);
                snprintf_pro(
                    buffer,
                    size,
                    "\n"
                    " CRtpSessionBase(POOL-0) --- [pid : %u/0x%X, this : 0x%p] \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[0]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[1]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[2]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[3]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[4]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[5]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[6]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[7]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[8]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[9]  : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[10] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[11] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[12] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[13] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[14] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[15] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[16] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[17] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[18] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[19] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[20] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[21] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[22] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[23] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[24] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[25] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[26] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[27] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[28] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[29] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[30] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[31] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[32] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[33] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[34] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[35] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[36] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[37] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[38] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[39] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[40] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[41] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[42] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[43] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[44] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[45] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[46] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[47] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[48] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[49] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[50] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[51] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[52] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[53] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[54] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[55] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[56] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[57] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[58] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[59] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[60] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[61] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[62] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - freeList[63] : 0x%p \n"
                    "\t CRtpSessionBase(POOL-0) - heapSize     : %u \n"
                    ,
                    (unsigned int)::GetCurrentProcessId(),
                    (unsigned int)::GetCurrentProcessId(),
                    this,
                    freeList[0],
                    freeList[1],
                    freeList[2],
                    freeList[3],
                    freeList[4],
                    freeList[5],
                    freeList[6],
                    freeList[7],
                    freeList[8],
                    freeList[9],
                    freeList[10],
                    freeList[11],
                    freeList[12],
                    freeList[13],
                    freeList[14],
                    freeList[15],
                    freeList[16],
                    freeList[17],
                    freeList[18],
                    freeList[19],
                    freeList[20],
                    freeList[21],
                    freeList[22],
                    freeList[23],
                    freeList[24],
                    freeList[25],
                    freeList[26],
                    freeList[27],
                    freeList[28],
                    freeList[29],
                    freeList[30],
                    freeList[31],
                    freeList[32],
                    freeList[33],
                    freeList[34],
                    freeList[35],
                    freeList[36],
                    freeList[37],
                    freeList[38],
                    freeList[39],
                    freeList[40],
                    freeList[41],
                    freeList[42],
                    freeList[43],
                    freeList[44],
                    freeList[45],
                    freeList[46],
                    freeList[47],
                    freeList[48],
                    freeList[49],
                    freeList[50],
                    freeList[51],
                    freeList[52],
                    freeList[53],
                    freeList[54],
                    freeList[55],
                    freeList[56],
                    freeList[57],
                    freeList[58],
                    freeList[59],
                    freeList[60],
                    freeList[61],
                    freeList[62],
                    freeList[63],
                    (unsigned int)heapSize
                    );
                ::OutputDebugString(buffer);
            }

            ProFree(buffer);
        }
        while (0);
#endif /* WIN32, _WIN32_WCE */

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
        }

        /*
         * a timeout occured?
         */
        if (tick - m_peerAliveTick < (PRO_INT64)GetRtpKeepaliveTimeout() * 1000)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, PBSD_ETIMEDOUT, 0, m_tcpConnected);
    }

    observer->Release();

    Fini();
}}

void
PRO_CALLTYPE
CRtpSessionBase::OnTimer(unsigned long timerId,
                         PRO_INT64     userData)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(timerId > 0);
    if (timerId == 0)
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
