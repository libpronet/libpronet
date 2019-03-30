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

#if !defined(RTP_MSG_SERVER_H)
#define RTP_MSG_SERVER_H

#include "rtp_foundation.h"
#include "rtp_framework.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RTP_MSG_PROTOCOL_VERSION 1
#define RTP_MSG_PACK_MODE        RTP_EPM_TCP4

class CProFunctorCommandTask;

struct RTP_MSG_HEADER0
{
    PRO_UINT16     version;      /* the current protocol version is 1 */
    RTP_MSG_USER   user;
    union
    {
        char       reserved[10];
        PRO_UINT32 publicIp;
    };

    DECLARE_SGI_POOL(0);
};

struct RTP_MSG_HEADER
{
    PRO_UINT16     charset;      /* ANSI, UTF-8, ... */
    RTP_MSG_USER   srcUser;
    char           reserved[1];
    unsigned char  dstUserCount; /* to 255 users at most */
    RTP_MSG_USER   dstUsers[1];  /* a variable-length array */

    DECLARE_SGI_POOL(0);
};

struct RTP_MSG_AsyncOnAcceptSession
{
    RTP_MSG_AsyncOnAcceptSession()
    {
        sslCtx     = NULL;
        sockId     = -1;
        unixSocket = false;
        remoteIp   = "";
        remotePort = 0;
        nonce      = 0;
        memset(&remoteInfo, 0, sizeof(RTP_SESSION_INFO));
    }

    PRO_SSL_CTX*     sslCtx;
    PRO_INT64        sockId;
    bool             unixSocket;
    CProStlString    remoteIp;
    unsigned short   remotePort;
    PRO_UINT64       nonce;
    RTP_SESSION_INFO remoteInfo;

    DECLARE_SGI_POOL(0);
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

    DECLARE_SGI_POOL(0);
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

    DECLARE_SGI_POOL(0);
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

    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes);

    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const;

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
        const RTP_MSG_USER* srcUser,
        const RTP_MSG_USER* dstUsers,    /* = NULL */
        unsigned char       dstUserCount /* = 0 */
        );

    void NotifyKickout(
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
    IRtpService*                                m_service;
    CProFunctorCommandTask*                     m_task;
    unsigned long                               m_timeoutInSeconds;
    unsigned long                               m_redlineBytes;

    CProStlMap<IRtpSession*, RTP_MSG_LINK_CTX*> m_session2Ctx;
    CProStlMap<RTP_MSG_USER, RTP_MSG_LINK_CTX*> m_user2Ctx;

    mutable CProThreadMutex                     m_lock;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_SERVER_H */
