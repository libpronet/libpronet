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
 * 1) client -----> rtp(RTP_SESSION_INFO with RTP_MSG_HEADER0) -----> server
 * 2) client <-----            rtp(RTP_SESSION_ACK)            <----- server
 * 3) client <-----            tcp4(RTP_MSG_HEADER0)           <----- server
 * 4) client <<====                  tcp4(msg)                 ====>> server
 *                   msg system handshake protocol flow chart
 */

#if !defined(RTP_MSG_C2S_H)
#define RTP_MSG_C2S_H

#include "rtp_base.h"
#include "rtp_msg.h"
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
        const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig, /* = NULL */
        const char*                  uplinkSslSni,    /* = NULL */
        const PRO_SSL_SERVER_CONFIG* localSslConfig,  /* = NULL */
        bool                         localSslForced   /* = false */
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

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const;

    virtual void PRO_CALLTYPE GetUplinkUser(RTP_MSG_USER* myUser) const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetUplinkSslSuite(char suiteName[64]) const;

    virtual const char* PRO_CALLTYPE GetUplinkLocalIp(char localIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetUplinkLocalPort() const;

    virtual const char* PRO_CALLTYPE GetUplinkRemoteIp(char remoteIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetUplinkRemotePort() const;

    virtual void PRO_CALLTYPE SetUplinkOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetUplinkOutputRedline() const;

    virtual unsigned long PRO_CALLTYPE GetUplinkSendingBytes() const;

    virtual unsigned short PRO_CALLTYPE GetLocalServicePort() const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetLocalSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const;

    virtual void PRO_CALLTYPE GetLocalUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* userCount         /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE KickoutLocalUser(const RTP_MSG_USER* user);

    virtual void PRO_CALLTYPE SetLocalOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetLocalOutputRedline() const;

    virtual unsigned long PRO_CALLTYPE GetLocalSendingBytes(const RTP_MSG_USER* user) const;

private:

    CRtpMsgC2s(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig, /* = NULL */
        const char*                  uplinkSslSni,    /* = NULL */
        const PRO_SSL_SERVER_CONFIG* localSslConfig,  /* = NULL */
        bool                         localSslForced   /* = false */
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

    static bool SendAckToDownlink(
        RTP_MM_TYPE         mmType,
        IRtpSession*        session,
        const RTP_MSG_USER* user,
        const char*         publicIp
        );

    static bool SendMsgToDownlink(
        RTP_MM_TYPE         mmType,
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

    void AsyncKickoutLocalUser(PRO_INT64* args);

private:

    const RTP_MM_TYPE                                       m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const                      m_uplinkSslConfig;
    const CProStlString                                     m_uplinkSslSni;
    const PRO_SSL_SERVER_CONFIG* const                      m_localSslConfig;
    const bool                                              m_localSslForced;

    IRtpMsgC2sObserver*                                     m_observer;
    IProReactor*                                            m_reactor;
    CProFunctorCommandTask*                                 m_task;
    CRtpMsgClient*                                          m_msgClient;
    IRtpService*                                            m_service;
    unsigned short                                          m_serviceHubPort;
    unsigned long                                           m_timerId;
    PRO_INT64                                               m_connectTick;
    CProStlString                                           m_uplinkIp;
    unsigned short                                          m_uplinkPort;
    RTP_MSG_USER                                            m_uplinkUser;
    CProStlString                                           m_uplinkPassword;
    CProStlString                                           m_uplinkLocalIp;
    unsigned long                                           m_uplinkTimeoutInSeconds;
    unsigned long                                           m_uplinkRedlineBytes;
    unsigned long                                           m_localTimeoutInSeconds;
    unsigned long                                           m_localRedlineBytes;
    RTP_MSG_USER                                            m_myUserNow;
    RTP_MSG_USER                                            m_myUserBak;

    CProStlMap<unsigned long, RTP_MSG_AsyncOnAcceptSession> m_timerId2Info;
    CProStlMap<IRtpSession*, RTP_MSG_USER>                  m_session2User;
    CProStlMap<RTP_MSG_USER, IRtpSession*>                  m_user2Session;

    mutable CProThreadMutex                                 m_lock;
    CProThreadMutex                                         m_lockUpcall;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_C2S_H */
