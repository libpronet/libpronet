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

#ifndef RTP_SESSION_WRAPPER_H
#define RTP_SESSION_WRAPPER_H

#include "rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stat.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

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

    virtual int64_t GetSockId() const;

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
        return m_onOkCalled;
    }

    virtual bool SendPacket(
        IRtpPacket* packet,
        bool*       tryAgain /* = NULL */
        );

    virtual bool SendPacketByTimer(
        IRtpPacket*  packet,
        unsigned int sendDurationMs /* = 0 */
        );

    virtual void GetSendOnSendTick(
        int64_t* onSendTick1, /* = NULL */
        int64_t* onSendTick2  /* = NULL */
        ) const;

    virtual void RequestOnSend();

    virtual void SuspendRecv();

    virtual void ResumeRecv();

    virtual bool AddMcastReceiver(const char* mcastIp);

    virtual void RemoveMcastReceiver(const char* mcastIp);

    virtual void EnableInput(bool enable);

    virtual void EnableOutput(bool enable);

    virtual void SetOutputRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        );

    virtual void GetOutputRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const;

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const;

    virtual void ResetFlowctrlInfo();

    virtual void GetInputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const;

    virtual void GetOutputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const;

    virtual void ResetInputStat();

    virtual void ResetOutputStat();

    virtual void SetMagic(int64_t magic);

    virtual int64_t GetMagic() const;

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
        int          errorCode,
        int          sslCode,
        bool         tcpConnected
        );

    virtual void OnHeartbeatSession(
        IRtpSession* session,
        int64_t      peerAliveTick
        );

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  userData
        );

    bool PushPacket(IRtpPacket* packet);

    bool DoSendPacket();

private:

    RTP_SESSION_INFO          m_info;
    int64_t                   m_magic;
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
    uint64_t                  m_timerId;
    volatile bool             m_onOkCalled;
    int64_t                   m_traceTick;

    uint64_t                  m_sendTimerId;
    unsigned int              m_sendDurationMs;
    int64_t                   m_pushTick;
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
