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

/*
 * This is an implementation of the "Template Method" pattern.
 */

#ifndef RTP_SESSION_BASE_H
#define RTP_SESSION_BASE_H

#include "rtp_base.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RTP_SESSION_PROTOCOL_VERSION 2

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

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    static bool IsUdpSession(RTP_SESSION_TYPE sessionType)
    {
        return sessionType == RTP_ST_UDPCLIENT    ||
               sessionType == RTP_ST_UDPSERVER    ||
               sessionType == RTP_ST_UDPCLIENT_EX ||
               sessionType == RTP_ST_UDPSERVER_EX ||
               sessionType == RTP_ST_MCAST        ||
               sessionType == RTP_ST_MCAST_EX;
    }

protected:

    CRtpSessionBase(bool suspendRecv);

    virtual ~CRtpSessionBase();

    virtual void GetInfo(RTP_SESSION_INFO* info) const;

    virtual void GetAck(RTP_SESSION_ACK* ack) const;

    virtual void GetSyncId(unsigned char syncId[14]) const;

    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const;

    virtual int64_t GetSockId() const;

    virtual const char* GetLocalIp(char localIp[64]) const;

    virtual unsigned short GetLocalPort() const;

    virtual const char* GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short GetRemotePort() const;

    virtual void SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        )
    {
    }

    virtual bool IsTcpConnected() const
    {
        return m_tcpConnected;
    }

    virtual bool IsReady() const
    {
        return m_onOkCalledPre || m_onOkCalledPost;
    }

    virtual bool SendPacket(
        IRtpPacket* packet,
        bool*       tryAgain /* = NULL */
        );

    virtual bool SendPacketByTimer(
        IRtpPacket*  packet,
        unsigned int sendDurationMs /* = 0 */
        )
    {
        return false;
    }

    virtual void GetSendOnSendTick(
        int64_t* onSendTick1, /* = NULL */
        int64_t* onSendTick2  /* = NULL */
        ) const;

    virtual void RequestOnSend();

    virtual void SuspendRecv();

    virtual void ResumeRecv();

    virtual bool AddMcastReceiver(const char* mcastIp)
    {
        return false;
    }

    virtual void RemoveMcastReceiver(const char* mcastIp)
    {
    }

    virtual void EnableInput(bool enable)
    {
    }

    virtual void EnableOutput(bool enable)
    {
    }

    virtual void SetOutputRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        )
    {
    }

    virtual void GetOutputRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const
    {
    }

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const
    {
    }

    virtual void ResetFlowctrlInfo()
    {
    }

    virtual void GetInputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void GetOutputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void ResetInputStat()
    {
    }

    virtual void ResetOutputStat()
    {
    }

    virtual void SetMagic(int64_t magic);

    virtual int64_t GetMagic() const;

    virtual void OnSend(
        IProTransport* trans,
        uint64_t       actionId
        );

    virtual void OnClose(
        IProTransport* trans,
        int            errorCode,
        int            sslCode
        );

    virtual void OnHeartbeat(IProTransport* trans);

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

    virtual void Fini()
    {
    }

    void DoCallbackOnOk(IRtpSessionObserver* observer);

protected:

    const bool              m_suspendRecv;
    RTP_SESSION_INFO        m_info;
    RTP_SESSION_ACK         m_ack;
    int64_t                 m_magic;
    IRtpSessionObserver*    m_observer;
    IProReactor*            m_reactor;
    IProTransport*          m_trans;
    CRtpPacket*             m_bigPacket;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_remoteAddr;
    pbsd_sockaddr_in        m_remoteAddrConfig; /* for udp */
    int64_t                 m_dummySockId;
    uint64_t                m_actionId;
    int64_t                 m_initTick;
    int64_t                 m_sendTick;
    int64_t                 m_onSendTick1;      /* for tcp, tcp_ex, ssl_ex */
    int64_t                 m_onSendTick2;      /* for tcp, tcp_ex, ssl_ex */
    int64_t                 m_peerAliveTick;
    uint64_t                m_timeoutTimerId;
    uint64_t                m_onOkTimerId;
    volatile bool           m_tcpConnected;     /* for tcp, tcp_ex, ssl_ex */
    bool                    m_handshakeOk;      /* for udp_ex, tcp_ex, ssl_ex */
    volatile bool           m_onOkCalledPre;
    volatile bool           m_onOkCalledPost;
    CProThreadMutex         m_lockOnOk;
    mutable CProThreadMutex m_lock;

    volatile bool           m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_BASE_H */
