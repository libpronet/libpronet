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

#if !defined(RTP_MSG_CLIENT_H)
#define RTP_MSG_CLIENT_H

#include "rtp_base.h"
#include "rtp_msg.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

class IRtpMsgClientObserverEx : public IRtpMsgClientObserver
{
public:

    virtual ~IRtpMsgClientObserverEx() {}

    virtual void PRO_CALLTYPE OnTransferMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER& srcUser,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpMsgClient
:
public IRtpMsgClient,
public IRtpSessionObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CRtpMsgClient* CreateInstance(
        bool                         enableTransfer,
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
        const char*                  sslSni     /* = NULL */
        );

    bool Init(
        IRtpMsgClientObserver* observer,
        IProReactor*           reactor,
        const char*            remoteIp,
        unsigned short         remotePort,
        const RTP_MSG_USER*    user,
        const char*            password,        /* = NULL */
        const char*            localIp,         /* = NULL */
        unsigned long          timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const;

    virtual void PRO_CALLTYPE GetUser(RTP_MSG_USER* myUser) const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(
        char suiteName[64]
        ) const;

    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetLocalPort() const;

    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetRemotePort() const;

    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual bool PRO_CALLTYPE SendMsg2(
        const void*         buf1,
        unsigned long       size1,
        const void*         buf2,  /* = NULL */
        unsigned long       size2, /* = 0 */
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const;

    virtual unsigned long PRO_CALLTYPE GetSendingBytes() const;

    bool TransferMsg(
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER& srcUser
        );

private:

    CRtpMsgClient(
        bool                         enableTransfer,
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
        const char*                  sslSni     /* = NULL */
        );

    virtual ~CRtpMsgClient();

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

    virtual void PRO_CALLTYPE OnHeartbeatSession(
        IRtpSession* session,
        PRO_INT64    peerAliveTick
        );

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    bool PushData(
        const void*         buf1,
        unsigned long       size1,
        const void*         buf2,   /* = NULL */
        unsigned long       size2,  /* = 0 */
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser /* = NULL */
        );

    void DoSendData(bool onOkCalled);

private:

    const bool                         m_enableTransfer; /* for c2s */
    const RTP_MM_TYPE                  m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslSni;

    IRtpMsgClientObserver*             m_observer;
    IProReactor*                       m_reactor;
    IRtpSession*                       m_session;
    IRtpBucket*                        m_bucket;
    RTP_MSG_USER                       m_userBak;
    PRO_UINT64                         m_timerId;
    bool                               m_onOkCalled;
    mutable CProThreadMutex            m_lock;

    volatile bool                      m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_CLIENT_H */
