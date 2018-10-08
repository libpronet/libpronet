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

#if !defined(RTP_MSG_CLIENT_H)
#define RTP_MSG_CLIENT_H

#include "rtp_foundation.h"
#include "rtp_framework.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

class IRtpMsgClientObserverEx : public IRtpMsgClientObserver
{
public:

    virtual void PRO_CALLTYPE OnTransferMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT32          charset,
        const RTP_MSG_USER* srcUser,
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
        const PRO_SSL_CLIENT_CONFIG* sslConfig,     /* = NULL */
        const char*                  sslServiceName /* = NULL */
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

    virtual void PRO_CALLTYPE GetUser(RTP_MSG_USER* user) const;

    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT32          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    bool TransferMsg(
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT32          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser
        );

    void SetOutputRedline(unsigned long redlineBytes);

private:

    CRtpMsgClient(
        bool                         enableTransfer,
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* sslConfig,     /* = NULL */
        const char*                  sslServiceName /* = NULL */
        );

    virtual ~CRtpMsgClient();

    virtual void PRO_CALLTYPE OnOkSession(IRtpSession* session)
    {
    }

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

    bool PushMsg(
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT32          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser /* = NULL */
        );

    void SendData(bool onOkCalled);

private:

    const bool                         m_enableTransfer; /* for c2s */
    const RTP_MM_TYPE                  m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslServiceName;

    IRtpMsgClientObserver*             m_observer;
    IProReactor*                       m_reactor;
    IRtpSession*                       m_session;
    IRtpBucket*                        m_bucket;
    RTP_MSG_USER                       m_user;
    unsigned long                      m_timerId;
    bool                               m_onOkCalled;
    mutable CProThreadMutex            m_lock;

    bool                               m_canUpcall;
    CProThreadMutex                    m_lockUpcall;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_CLIENT_H */
