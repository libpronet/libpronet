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

#if !defined(RTP_MSG_C2S_H)
#define RTP_MSG_C2S_H

#include "rtp_base.h"
#include "rtp_msg.h"
#include "rtp_msg_client.h"
#include "rtp_msg_server.h"
#include "../pro_util/pro_memory_pool.h"
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

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    virtual RTP_MM_TYPE GetMmType() const
    {
        return (m_mmType);
    }

    virtual void GetUplinkUser(RTP_MSG_USER* myUser) const;

    virtual PRO_SSL_SUITE_ID GetUplinkSslSuite(char suiteName[64]) const;

    virtual const char* GetUplinkLocalIp(char localIp[64]) const;

    virtual unsigned short GetUplinkLocalPort() const;

    virtual const char* GetUplinkRemoteIp(char remoteIp[64]) const;

    virtual unsigned short GetUplinkRemotePort() const;

    virtual void SetUplinkOutputRedline(unsigned long redlineBytes);

    virtual unsigned long GetUplinkOutputRedline() const;

    virtual unsigned long GetUplinkSendingBytes() const;

    virtual unsigned short GetLocalServicePort() const;

    virtual PRO_SSL_SUITE_ID GetLocalSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const;

    virtual void GetLocalUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* userCount         /* = NULL */
        ) const;

    virtual void KickoutLocalUser(const RTP_MSG_USER* user);

    virtual void SetLocalOutputRedline(unsigned long redlineBytes);

    virtual unsigned long GetLocalOutputRedline() const;

    virtual unsigned long GetLocalSendingBytes(const RTP_MSG_USER* user) const;

private:

    CRtpMsgC2s(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig, /* = NULL */
        const char*                  uplinkSslSni,    /* = NULL */
        const PRO_SSL_SERVER_CONFIG* localSslConfig,  /* = NULL */
        bool                         localSslForced   /* = false */
        );

    virtual ~CRtpMsgC2s();

    virtual void OnAcceptSession(
        IRtpService*            service,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        const PRO_NONCE*        nonce
        );

    virtual void OnAcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        const PRO_NONCE*        nonce
        );

    virtual void OnOkSession(IRtpSession* session)
    {
    }

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
        long         errorCode,
        long         sslCode,
        bool         tcpConnected
        );

    virtual void OnHeartbeatSession(
        IRtpSession* session,
        PRO_INT64    peerAliveTick
        );

    virtual void OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        );

    virtual void OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        );

    virtual void OnTransferMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER& srcUser,
        const RTP_MSG_USER* dstUsers,
        unsigned char       dstUserCount
        );

    virtual void OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,
        long           sslCode,
        bool           tcpConnected
        );

    virtual void OnHeartbeatMsg(
        IRtpMsgClient* msgClient,
        PRO_INT64      peerAliveTick
        );

    virtual void OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    void AcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx, /* = NULL */
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO& remoteInfo,
        const PRO_NONCE&        nonce
        );

    static bool SendMsgToDownlink(
        RTP_MM_TYPE         mmType,
        IRtpSession**       sessions,
        unsigned char       sessionCount,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER& srcUser
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

    const RTP_MM_TYPE                                    m_mmType;
    const PRO_SSL_CLIENT_CONFIG* const                   m_uplinkSslConfig;
    const CProStlString                                  m_uplinkSslSni;
    const PRO_SSL_SERVER_CONFIG* const                   m_localSslConfig;
    const bool                                           m_localSslForced;

    IRtpMsgC2sObserver*                                  m_observer;
    IProReactor*                                         m_reactor;
    CProFunctorCommandTask*                              m_task;
    CRtpMsgClient*                                       m_msgClient;
    IRtpService*                                         m_service;
    unsigned short                                       m_serviceHubPort;
    PRO_UINT64                                           m_timerId;
    PRO_INT64                                            m_connectTick;
    CProStlString                                        m_uplinkIp;
    unsigned short                                       m_uplinkPort;
    RTP_MSG_USER                                         m_uplinkUser;
    CProStlString                                        m_uplinkPassword;
    CProStlString                                        m_uplinkLocalIp;
    unsigned long                                        m_uplinkTimeoutInSeconds;
    unsigned long                                        m_uplinkRedlineBytes;
    unsigned long                                        m_localTimeoutInSeconds;
    unsigned long                                        m_localRedlineBytes;
    RTP_MSG_USER                                         m_myUserNow;
    RTP_MSG_USER                                         m_myUserBak;

    CProStlMap<PRO_UINT64, RTP_MSG_AsyncOnAcceptSession> m_timerId2Info;
    CProStlMap<IRtpSession*, RTP_MSG_USER>               m_session2User;
    CProStlMap<RTP_MSG_USER, IRtpSession*>               m_user2Session;

    mutable CProThreadMutex                              m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_C2S_H */
