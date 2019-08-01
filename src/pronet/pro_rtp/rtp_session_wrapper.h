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

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CRtpSessionWrapper(const RTP_SESSION_INFO& localInfo);

    virtual ~CRtpSessionWrapper();

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
        );

    virtual bool PRO_CALLTYPE IsTcpConnected() const;

    virtual bool PRO_CALLTYPE IsReady() const;

    virtual bool PRO_CALLTYPE SendPacket(IRtpPacket* packet);

    virtual bool PRO_CALLTYPE SendPacketByTimer(
        IRtpPacket*   packet,
        unsigned long sendDurationMs /* = 0 */
        );

    virtual void PRO_CALLTYPE GetSendOnSendTick(
        PRO_INT64* sendTick,         /* = NULL */
        PRO_INT64* onSendTick        /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE RequestOnSend();

    virtual void PRO_CALLTYPE SuspendRecv();

    virtual void PRO_CALLTYPE ResumeRecv();

    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp);

    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp);

    virtual void PRO_CALLTYPE EnableInput(bool enable);

    virtual void PRO_CALLTYPE EnableOutput(bool enable);

    virtual void PRO_CALLTYPE SetOutputRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        );

    virtual void PRO_CALLTYPE GetOutputRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo();

    virtual void PRO_CALLTYPE GetInputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE GetOutputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE ResetInputStat();

    virtual void PRO_CALLTYPE ResetOutputStat();

    virtual void PRO_CALLTYPE OnOkSession(IRtpSession* session);

    virtual void PRO_CALLTYPE OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        );

    virtual void PRO_CALLTYPE OnSendSession(
        IRtpSession* session,
        bool         packetErased
        );

    virtual void PRO_CALLTYPE OnCloseSession(
        IRtpSession* session,
        long         errorCode,
        long         sslCode,
        bool         tcpConnected
        );

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        );

    bool SendPacketUnlock(IRtpPacket* packet);

    bool SendPacketUnlock();

private:

    RTP_SESSION_INFO          m_info;
    IRtpSessionObserver*      m_observer;
    IProReactor*              m_reactor;
    IRtpSession*              m_session;
    IRtpBucket*               m_bucket;
    bool                      m_pushToBucketRet1;
    bool                      m_pushToBucketRet2;
    bool                      m_packetErased;
    bool                      m_enableInput;
    bool                      m_enableOutput;
    unsigned long             m_timerId;
    bool                      m_onOkCalled;
    PRO_INT64                 m_traceTick;

    unsigned long             m_sendTimerId;
    unsigned long             m_sendDurationMs;
    PRO_INT64                 m_pushTick;
    CProStlDeque<IRtpPacket*> m_pushPackets;

    IRtpReorder*              m_reorderInput;
    mutable CProStatBitRate   m_statFrameRateInput;
    mutable CProStatBitRate   m_statFrameRateOutput;
    mutable CProStatBitRate   m_statBitRateInput;
    mutable CProStatBitRate   m_statBitRateOutput;
    mutable CProStatLossRate  m_statLossRateInput;
    mutable CProStatLossRate  m_statLossRateOutput;

    mutable CProThreadMutex   m_lock;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_WRAPPER_H */
