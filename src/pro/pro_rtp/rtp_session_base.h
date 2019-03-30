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

/*
 * This is an implementation of the "Template Method" pattern.
 */

#if !defined(RTP_SESSION_BASE_H)
#define RTP_SESSION_BASE_H

#include "rtp_framework.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RTP_SESSION_PROTOCOL_VERSION 1

struct RTP_SESSION_ACK
{
    PRO_UINT16 version;      /* the current protocol version is 1 */
    char       reserved[30]; /* zero value */

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionBase
:
public IRtpSession,
public IProTransportObserver,
public IProOnTimer,
public CProRefCount
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

protected:

    CRtpSessionBase();

    virtual ~CRtpSessionBase()
    {
    }

    virtual void PRO_CALLTYPE GetInfo(RTP_SESSION_INFO* info) const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(char suiteName[64]) const;

    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const;

    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetLocalPort() const;

    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetRemotePort() const;

    virtual void PRO_CALLTYPE SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        )
    {
    }

    virtual bool PRO_CALLTYPE IsTcpConnected() const;

    virtual bool PRO_CALLTYPE IsReady() const;

    virtual bool PRO_CALLTYPE SendPacket(IRtpPacket* packet);

    virtual bool PRO_CALLTYPE SendPacketByTimer(
        IRtpPacket*   packet,
        unsigned long sendDurationMs /* = 0 */
        )
    {
        return (false);
    }

    virtual void PRO_CALLTYPE GetSendOnSendTick(
        PRO_INT64* sendTick,         /* = NULL */
        PRO_INT64* onSendTick        /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE RequestOnSend();

    virtual void PRO_CALLTYPE SuspendRecv();

    virtual void PRO_CALLTYPE ResumeRecv();

    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp)
    {
        return (false);
    }

    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp)
    {
    }

    virtual void PRO_CALLTYPE EnableInput(bool enable)
    {
    }

    virtual void PRO_CALLTYPE EnableOutput(bool enable)
    {
    }

    virtual void PRO_CALLTYPE SetOutputRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        )
    {
    }

    virtual void PRO_CALLTYPE GetOutputRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE ResetFlowctrlInfo()
    {
    }

    virtual void PRO_CALLTYPE GetInputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE GetOutputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE ResetInputStat()
    {
    }

    virtual void PRO_CALLTYPE ResetOutputStat()
    {
    }

    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        );

    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans);

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        );

    virtual void Fini() = 0;

protected:

    RTP_SESSION_INFO        m_info;
    IRtpSessionObserver*    m_observer;
    IProReactor*            m_reactor;
    IProTransport*          m_trans;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_remoteAddr;
    pbsd_sockaddr_in        m_remoteAddrConfig; /* for udp */
    PRO_INT64               m_dummySockId;
    PRO_UINT64              m_actionId;
    PRO_INT64               m_initTick;
    PRO_INT64               m_sendingTick;
    PRO_INT64               m_sendTick;         /* for tcp, tcp_ex, ssl_ex */
    PRO_INT64               m_onSendTick;       /* for tcp, tcp_ex, ssl_ex */
    PRO_INT64               m_peerAliveTick;
    unsigned long           m_timeoutTimerId;
    unsigned long           m_onOkTimerId;
    bool                    m_tcpConnected;     /* for tcp, tcp_ex, ssl_ex */
    bool                    m_handshakeOk;      /* for udp_ex, tcp_ex, ssl_ex */
    bool                    m_onOkCalled;
    mutable CProThreadMutex m_lock;

    bool                    m_canUpcall;
    CProThreadMutex         m_lockUpcall;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_BASE_H */
