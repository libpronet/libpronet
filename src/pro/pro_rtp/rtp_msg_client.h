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
 * 1) client -----> rtp(RTP_SESSION_INFO with RTP_MSG_HEADER0) -----> server
 * 2) client <-----            rtp(RTP_SESSION_ACK)            <----- server
 * 3) client <-----            tcp4(RTP_MSG_HEADER0)           <----- server
 * 4) client <<====                  tcp4(msg)                 ====>> server
 *                   msg system handshake protocol flow chart
 */

#if !defined(RTP_MSG_CLIENT_H)
#define RTP_MSG_CLIENT_H

#include "rtp_base.h"
#include "rtp_msg.h"
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
        unsigned long       size,
        PRO_UINT16          charset,
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

    virtual void PRO_CALLTYPE GetUser(RTP_MSG_USER* user) const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(char suiteName[64]) const;

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

    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const;

    bool TransferMsg(
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser
        );

private:

    CRtpMsgClient(
        bool                         enableTransfer,
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
        const char*                  sslSni     /* = NULL */
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

    bool PushData(
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount,
        const RTP_MSG_USER* srcUser /* = NULL */
        );

    void SendData(bool onOkCalled);

    void RecvAck(
        IRtpSession* session,
        IRtpPacket*  packet
        );

    void RecvData(
        IRtpSession* session,
        IRtpPacket*  packet
        );

private:

    const bool                         m_enableTransfer; /* for c2s */
    const RTP_MM_TYPE                  m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslSni;

    IRtpMsgClientObserver*             m_observer;
    IProReactor*                       m_reactor;
    IRtpSession*                       m_session;
    IRtpBucket*                        m_bucket;
    CProStlString                      m_remoteIp;
    unsigned short                     m_remotePort;
    RTP_MSG_USER                       m_userBak;
    unsigned long                      m_timerId;
    bool                               m_onOkCalled;
    mutable CProThreadMutex            m_lock;

    bool                               m_canUpcall;
    CProThreadMutex                    m_lockUpcall;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_CLIENT_H */
