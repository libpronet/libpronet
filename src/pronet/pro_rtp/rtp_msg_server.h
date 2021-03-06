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

#if !defined(RTP_MSG_SERVER_H)
#define RTP_MSG_SERVER_H

#include "rtp_base.h"
#include "rtp_msg.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RTP_MSG_PROTOCOL_VERSION 2
#define RTP_MSG_PACK_MODE        RTP_EPM_TCP4

class CProFunctorCommandTask;

struct RTP_MSG_AsyncOnAcceptSession
{
    RTP_MSG_AsyncOnAcceptSession()
    {
        sslCtx     = NULL;
        sockId     = -1;
        unixSocket = false;
        remoteIp   = "";
        remotePort = 0;

        memset(&remoteInfo, 0, sizeof(RTP_SESSION_INFO));
        memset(&nonce     , 0, sizeof(PRO_NONCE));
    }

    PRO_SSL_CTX*     sslCtx;
    PRO_INT64        sockId;
    bool             unixSocket;
    CProStlString    remoteIp;
    unsigned short   remotePort;
    RTP_SESSION_INFO remoteInfo;
    PRO_NONCE        nonce;

    DECLARE_SGI_POOL(0)
};

struct RTP_MSG_AsyncOnRecvSession
{
    RTP_MSG_AsyncOnRecvSession()
    {
        session = NULL;
    }

    IRtpSession*     session;
    RTP_MSG_USER     c2sUser;
    CProConfigStream msgStream;

    DECLARE_SGI_POOL(0)
};

struct RTP_MSG_LINK_CTX
{
    RTP_MSG_LINK_CTX()
    {
        session = NULL;
        isC2s   = false;
    }

    IRtpSession*             session;
    RTP_MSG_USER             baseUser;
    CProStlSet<RTP_MSG_USER> subUsers;
    bool                     isC2s;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpMsgServer
:
public IRtpMsgServer,
public IRtpServiceObserver,
public IRtpSessionObserver,
public CProRefCount
{
public:

    static CRtpMsgServer* CreateInstance(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
        bool                         sslForced  /* = false */
        );

    bool Init(
        IRtpMsgServerObserver* observer,
        IProReactor*           reactor,
        unsigned short         serviceHubPort,
        unsigned long          timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const;

    virtual unsigned short PRO_CALLTYPE GetServicePort() const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const;

    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* baseUserCount,    /* = NULL */
        unsigned long* subUserCount      /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user);

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

    virtual void PRO_CALLTYPE SetOutputRedlineToC2s(
        unsigned long redlineBytes
        );

    virtual unsigned long PRO_CALLTYPE GetOutputRedlineToC2s() const;

    virtual void PRO_CALLTYPE SetOutputRedlineToUsr(
        unsigned long redlineBytes
        );

    virtual unsigned long PRO_CALLTYPE GetOutputRedlineToUsr() const;

    virtual unsigned long PRO_CALLTYPE GetSendingBytes(
        const RTP_MSG_USER* user
        ) const;

private:

    CRtpMsgServer(
        RTP_MM_TYPE                  mmType,
        const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
        bool                         sslForced  /* = false */
        );

    virtual ~CRtpMsgServer();

    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        const PRO_NONCE*        nonce
        );

    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,
        PRO_INT64               sockId,
        bool                    unixSocket,
        const char*             remoteIp,
        unsigned short          remotePort,
        const RTP_SESSION_INFO* remoteInfo,
        const PRO_NONCE*        nonce
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

    virtual void PRO_CALLTYPE OnHeartbeatSession(
        IRtpSession* session,
        PRO_INT64    peerAliveTick
        );

    bool AddBaseUser(
        RTP_SESSION_TYPE        sessionType,
        const RTP_INIT_ARGS&    initArgs,
        const RTP_SESSION_INFO& localInfo,
        const RTP_MSG_USER&     baseUser,
        const CProStlString&    publicIp,
        PRO_INT64               appData,
        bool                    isC2s
        );

    void AddSubUser(
        const RTP_MSG_USER&  c2sUser,
        const RTP_MSG_USER&  subUser,
        const CProStlString& publicIp,
        const CProStlString& msgText,
        PRO_INT64            appData
        );

    static bool SendMsgToDownlink(
        RTP_MM_TYPE         mmType,
        IRtpSession**       sessions,
        unsigned char       sessionCount,
        const void*         buf1,
        unsigned long       size1,
        const void*         buf2,        /* = NULL */
        unsigned long       size2,       /* = 0 */
        PRO_UINT16          charset,
        const RTP_MSG_USER& srcUser,
        const RTP_MSG_USER* dstUsers,    /* = NULL */
        unsigned char       dstUserCount /* = 0 */
        );

    static void NotifyKickout(
        RTP_MM_TYPE         mmType,
        IRtpSession*        session,
        const RTP_MSG_USER& c2sUser,
        const RTP_MSG_USER& subUser
        );

    void ProcessMsg_client_login(
        IRtpSession*            session,
        const CProConfigStream& msgStream,
        const RTP_MSG_USER&     c2sUser
        );

    void ProcessMsg_client_logout(
        IRtpSession*            session,
        const CProConfigStream& msgStream,
        const RTP_MSG_USER&     c2sUser
        );

    void AsyncKickoutUser(PRO_INT64* args);

    void AsyncOnAcceptSession(PRO_INT64* args);

    void AsyncOnRecvSession(PRO_INT64* args);

    void AsyncOnCloseSession(PRO_INT64* args);

private:

    const RTP_MM_TYPE                           m_mmType;
    const PRO_SSL_SERVER_CONFIG* const          m_sslConfig;
    const bool                                  m_sslForced;

    IRtpMsgServerObserver*                      m_observer;
    IProReactor*                                m_reactor;
    CProFunctorCommandTask*                     m_task;
    IRtpService*                                m_service;
    unsigned short                              m_serviceHubPort;
    unsigned long                               m_timeoutInSeconds;
    unsigned long                               m_redlineBytesC2s;
    unsigned long                               m_redlineBytesUsr;

    CProStlMap<IRtpSession*, RTP_MSG_LINK_CTX*> m_session2Ctx;
    CProStlMap<RTP_MSG_USER, RTP_MSG_LINK_CTX*> m_user2Ctx;

    mutable CProThreadMutex                     m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_SERVER_H */
