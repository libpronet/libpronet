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

#if !defined(RTP_MSG_C2S_H)
#define RTP_MSG_C2S_H

#include "rtp_foundation.h"
#include "rtp_framework.h"
#include "rtp_msg_client.h"
#include "rtp_msg_server.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProConfigStream;
class CProFunctorCommandTask;

/////////////////////////////////////////////////////////////////////////////
////

class CRtpMsgC2s
:
public IRtpMsgC2s,
public IRtpServiceObserver,
public IRtpSessionObserver,
public IRtpMsgClientObserverEx,
public IProOnTimer,
public CProRefCount
{
public:

    static CRtpMsgC2s* CreateInstance(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,      /* = NULL */
        const char*                  uplinkSslServiceName, /* = NULL */
        const PRO_SSL_SERVER_CONFIG* localSslConfig,       /* = NULL */
        bool                         localSslForced        /* = false */
        );

    bool Init(
        IRtpMsgC2sObserver* observer,
        IProReactor*        reactor,
        const char*         uplinkIp,
        unsigned short      uplinkPort,
        const RTP_MSG_USER* uplinkUser,
        const char*         uplinkPassword,
        const char*         uplinkLocalIp,          /* = NULL */
        unsigned long       uplinkTimeoutInSeconds, /* = 0 */
        unsigned short      localServiceHubPort,
        unsigned long       localTimeoutInSeconds   /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    virtual void PRO_CALLTYPE GetC2sUser(RTP_MSG_USER* c2sUser) const;

    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* userCount         /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user);

    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const;

private:

    CRtpMsgC2s(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,      /* = NULL */
        const char*                  uplinkSslServiceName, /* = NULL */
        const PRO_SSL_SERVER_CONFIG* localSslConfig,       /* = NULL */
        bool                         localSslForced        /* = false */
        );

    virtual ~CRtpMsgC2s();

    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        PRO_UINT64              nonce
        );

    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        PRO_UINT64              nonce
        );

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
        )
    {
    }

    virtual void PRO_CALLTYPE OnCloseSession(
        IRtpSession* session,
        long         errorCode,
        long         sslCode,
        bool         tcpConnected
        );

    virtual void PRO_CALLTYPE OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        );

    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        );

    virtual void PRO_CALLTYPE OnTransferMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual void PRO_CALLTYPE OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,
        long           sslCode,
        bool           tcpConnected
        );

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        );

    void AcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        PRO_UINT64              nonce
        );

    bool SendAckToDownlink(
        IRtpSession*        session,
        const RTP_MSG_USER* user,
        const char*         publicIp
        );

    bool SendMsgToDownlink(
        IRtpSession**       sessions,
        unsigned char       sessionCount,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        );

    static void ReportLogout(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER& user
        );

    void ProcessMsg_client_login_ok(
        IRtpMsgClient*          msgClient,
        const CProConfigStream& msgStream
        );

    void ProcessMsg_client_login_error(
        IRtpMsgClient*          msgClient,
        const CProConfigStream& msgStream
        );

    void ProcessMsg_client_kickout(
        IRtpMsgClient*          msgClient,
        const CProConfigStream& msgStream
        );

    void AsyncKickoutUser(PRO_INT64* args);

private:

    const RTP_MM_TYPE                                       m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const                      m_uplinkSslConfig;
    const CProStlString                                     m_uplinkSslServiceName;
    const PRO_SSL_SERVER_CONFIG* const                      m_localSslConfig;
    const bool                                              m_localSslForced;

    IRtpMsgC2sObserver*                                     m_observer;
    IProReactor*                                            m_reactor;
    IRtpService*                                            m_service;
    CProFunctorCommandTask*                                 m_task;
    CRtpMsgClient*                                          m_msgClient;
    unsigned long                                           m_timerId;
    PRO_INT64                                               m_connectTick;
    CProStlString                                           m_uplinkIp;
    unsigned short                                          m_uplinkPort;
    RTP_MSG_USER                                            m_uplinkUser;
    CProStlString                                           m_uplinkPassword;
    CProStlString                                           m_uplinkLocalIp;
    unsigned long                                           m_uplinkTimeoutInSeconds;
    unsigned long                                           m_localTimeoutInSeconds;
    unsigned long                                           m_redlineBytes;
    RTP_MSG_USER                                            m_c2sUser;
    RTP_MSG_USER                                            m_c2sUserBak;

    CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession> m_timerId2Info;
    CProStlMap<IRtpSession*, RTP_MSG_USER>                  m_session2User;
    CProStlMap<RTP_MSG_USER, IRtpSession*>                  m_user2Session;

    mutable CProThreadMutex                                 m_lock;
    CProThreadMutex                                         m_lockUpcall;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_C2S_H */
