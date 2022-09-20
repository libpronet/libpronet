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
 * This is an implementation of the "Decorate" pattern.
 */

#if !defined(RTP_SESSION_WRAPPER_H)
#define RTP_SESSION_WRAPPER_H

#include "rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stat.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionWrapper
:
public IRtpSession,
public IRtpSessionObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CRtpSessionWrapper* CreateInstance(const RTP_SESSION_INFO* localInfo);

    bool Init(
        RTP_SESSION_TYPE     sessionType,
        const RTP_INIT_ARGS* initArgs
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

private:

    CRtpSessionWrapper(const RTP_SESSION_INFO& localInfo);

    virtual ~CRtpSessionWrapper();

    virtual void GetInfo(RTP_SESSION_INFO* info) const;

    virtual void GetAck(RTP_SESSION_ACK* ack) const;

    virtual void GetSyncId(unsigned char syncId[14]) const;

    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const;

    virtual PRO_INT64 GetSockId() const;

    virtual const char* GetLocalIp(char localIp[64]) const;

    virtual unsigned short GetLocalPort() const;

    virtual const char* GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short GetRemotePort() const;

    virtual void SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        );

    virtual bool IsTcpConnected() const;

    virtual bool IsReady() const
    {
        return (m_onOkCalled);
    }

    virtual bool SendPacket(
        IRtpPacket* packet,
        bool*       tryAgain /* = NULL */
        );

    virtual bool SendPacketByTimer(
        IRtpPacket*   packet,
        unsigned long sendDurationMs /* = 0 */
        );

    virtual void GetSendOnSendTick(
        PRO_INT64* onSendTick1, /* = NULL */
        PRO_INT64* onSendTick2  /* = NULL */
        ) const;

    virtual void RequestOnSend();

    virtual void SuspendRecv();

    virtual void ResumeRecv();

    virtual bool AddMcastReceiver(const char* mcastIp);

    virtual void RemoveMcastReceiver(const char* mcastIp);

    virtual void EnableInput(bool enable);

    virtual void EnableOutput(bool enable);

    virtual void SetOutputRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames, /* = 0 */
        unsigned long redlineDelayMs /* = 0 */
        );

    virtual void GetOutputRedline(
        unsigned long* redlineBytes,  /* = NULL */
        unsigned long* redlineFrames, /* = NULL */
        unsigned long* redlineDelayMs /* = NULL */
        ) const;

    virtual void GetFlowctrlInfo(
        float*         srcFrameRate, /* = NULL */
        float*         srcBitRate,   /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const;

    virtual void ResetFlowctrlInfo();

    virtual void GetInputStat(
        float*      frameRate, /* = NULL */
        float*      bitRate,   /* = NULL */
        float*      lossRate,  /* = NULL */
        PRO_UINT64* lossCount  /* = NULL */
        ) const;

    virtual void GetOutputStat(
        float*      frameRate, /* = NULL */
        float*      bitRate,   /* = NULL */
        float*      lossRate,  /* = NULL */
        PRO_UINT64* lossCount  /* = NULL */
        ) const;

    virtual void ResetInputStat();

    virtual void ResetOutputStat();

    virtual void SetMagic(PRO_INT64 magic);

    virtual PRO_INT64 GetMagic() const;

    virtual void OnOkSession(IRtpSession* session);

    virtual void OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        );

    virtual void OnSendSession(
        IRtpSession* session,
        bool         packetErased
        );

    virtual void OnCloseSession(
        IRtpSession* session,
        long         errorCode,
        long         sslCode,
        bool         tcpConnected
        );

    virtual void OnHeartbeatSession(
        IRtpSession* session,
        PRO_INT64    peerAliveTick
        );

    virtual void OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    bool PushPacket(IRtpPacket* packet);

    bool DoSendPacket();

private:

    RTP_SESSION_INFO          m_info;
    PRO_INT64                 m_magic;
    IRtpSessionObserver*      m_observer;
    IProReactor*              m_reactor;
    IRtpSession*              m_session;
    IRtpBucket*               m_bucket;
    bool                      m_pushToBucketRet1;
    bool                      m_pushToBucketRet2;
    bool                      m_packetErased;
    bool                      m_enableInput;
    bool                      m_enableOutput;
    bool                      m_outputPending;
    PRO_UINT64                m_timerId;
    volatile bool             m_onOkCalled;
    PRO_INT64                 m_traceTick;

    PRO_UINT64                m_sendTimerId;
    unsigned long             m_sendDurationMs;
    PRO_INT64                 m_pushTick;
    CProStlDeque<IRtpPacket*> m_pushPackets;

    mutable CProStatBitRate   m_statFrameRateInput;
    mutable CProStatBitRate   m_statFrameRateOutput;
    mutable CProStatBitRate   m_statBitRateInput;
    mutable CProStatBitRate   m_statBitRateOutput;
    mutable CProStatLossRate  m_statLossRateInput;
    mutable CProStatLossRate  m_statLossRateOutput;

    mutable CProThreadMutex   m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_WRAPPER_H */
