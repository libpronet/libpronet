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

#ifndef RTP_MSG_CLIENT_H
#define RTP_MSG_CLIENT_H

#include "rtp_base.h"
#include "rtp_msg.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class IRtpMsgClientObserverEx : public IRtpMsgClientObserver
{
public:

    virtual ~IRtpMsgClientObserverEx() {}

    virtual void OnTransferMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        size_t              size,
        uint16_t            charset,
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
        unsigned int           timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    virtual RTP_MM_TYPE GetMmType() const
    {
        return m_mmType;
    }

    virtual void GetUser(RTP_MSG_USER* myUser) const;

    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const;

    virtual const char* GetLocalIp(char localIp[64]) const;

    virtual unsigned short GetLocalPort() const;

    virtual const char* GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short GetRemotePort() const;

    virtual bool SendMsg(
        const void*         buf,
        size_t              size,
        uint16_t            charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual bool SendMsg2(
        const void*         buf1,
        size_t              size1,
        const void*         buf2,  /* = NULL */
        size_t              size2, /* = 0 */
        uint16_t            charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual void SetOutputRedline(size_t redlineBytes);

    virtual size_t GetOutputRedline() const;

    virtual size_t GetSendingBytes() const;

    bool TransferMsg(
        const void*         buf,
        size_t              size,
        uint16_t            charset,
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

    virtual void OnOkSession(IRtpSession* session);

    virtual void OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        );

    virtual void OnSendSession(
        IRtpSession* session,
        bool         packetErased
        )
    {
    }

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

    bool PushData(
        const void*         buf1,
        size_t              size1,
        const void*         buf2,   /* = NULL */
        size_t              size2,  /* = 0 */
        uint16_t            charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser /* = NULL */
        );

private:

    const bool                         m_enableTransfer; /* for c2s */
    const RTP_MM_TYPE                  m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslSni;

    IRtpMsgClientObserver*             m_observer;
    IProReactor*                       m_reactor;
    IRtpSession*                       m_session;
    RTP_MSG_USER                       m_userBak;
    uint64_t                           m_timerId;
    mutable CProThreadMutex            m_lock;

    volatile bool                      m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_CLIENT_H */
