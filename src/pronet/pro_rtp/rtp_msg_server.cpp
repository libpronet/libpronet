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

#include "rtp_msg_server.h"
#include "rtp_base.h"
#include "rtp_msg.h"
#include "rtp_msg_command.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_functor_command.h"
#include "../pro_util/pro_functor_command_task.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_PENDING_COUNT         (PRO_ACCEPTOR_LENGTH + PRO_SERVICER_LENGTH)
#define DEFAULT_REDLINE_BYTES_C2S (1024 * 1024 * 8)
#define DEFAULT_REDLINE_BYTES_USR (1024 * 1024)
#define DEFAULT_TIMEOUT           20

static const RTP_MSG_USER  ROOT_ID    (1, 1, 0);            /* 1-1 */
static const RTP_MSG_USER  ROOT_ID_C2S(1, 1, 65535);        /* 1-1-65535 */
static const unsigned char SERVER_CID    = 1;               /* 1-... */
static const uint64_t      NODE_UID_MIN  = 1;               /* 1 ~ ... */
static const uint64_t      NODE_UID_MAX  = 0xEFFFFFFFFFULL; /* 1 ~ 0xEFFFFFFFFF */
static const uint64_t      NODE_UID_MAXX = 0xFFFFFFFFFFULL; /* 1 ~ 0xFFFFFFFFFF */

typedef void (CRtpMsgServer::* ACTION)(int64_t*);

static uint64_t        g_s_nextServerId = 0xF000000000ULL; /* 0xF000000000 ~ 0xFFFFFFFFFF */
static uint64_t        g_s_nextClientId = 0xF000000000ULL; /* 0xF000000000 ~ 0xFFFFFFFFFF */
static CProThreadMutex g_s_lock;

/////////////////////////////////////////////////////////////////////////////
////

static
uint64_t
MakeServerId_i()
{
    g_s_lock.Lock();

    const uint64_t userId = g_s_nextServerId;
    ++g_s_nextServerId;
    if (g_s_nextServerId > 0xFFFFFFFFFFULL)
    {
        g_s_nextServerId = 0xF000000000ULL;
    }

    g_s_lock.Unlock();

    return userId;
}

static
uint64_t
MakeClientId_i()
{
    g_s_lock.Lock();

    const uint64_t userId = g_s_nextClientId;
    ++g_s_nextClientId;
    if (g_s_nextClientId > 0xFFFFFFFFFFULL)
    {
        g_s_nextClientId = 0xF000000000ULL;
    }

    g_s_lock.Unlock();

    return userId;
}

/////////////////////////////////////////////////////////////////////////////
////

CRtpMsgServer*
CRtpMsgServer::CreateInstance(RTP_MM_TYPE                  mmType,
                              const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                              bool                         sslForced) /* = false */
{
    assert(mmType >= RTP_MMT_MSG_MIN);
    assert(mmType <= RTP_MMT_MSG_MAX);
    if (mmType < RTP_MMT_MSG_MIN || mmType > RTP_MMT_MSG_MAX)
    {
        return NULL;
    }

    return new CRtpMsgServer(mmType, sslConfig, sslForced);
}

CRtpMsgServer::CRtpMsgServer(RTP_MM_TYPE                  mmType,
                             const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                             bool                         sslForced) /* = false */
:
m_mmType(mmType),
m_sslConfig(sslConfig),
m_sslForced(sslForced)
{
    m_observer        = NULL;
    m_reactor         = NULL;
    m_task            = NULL;
    m_service         = NULL;
    m_serviceHubPort  = 0;
    m_redlineBytesC2s = DEFAULT_REDLINE_BYTES_C2S;
    m_redlineBytesUsr = DEFAULT_REDLINE_BYTES_USR;
}

CRtpMsgServer::~CRtpMsgServer()
{
    Fini();
}

bool
CRtpMsgServer::Init(IRtpMsgServerObserver* observer,
                    IProReactor*           reactor,
                    unsigned short         serviceHubPort,
                    unsigned int           timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(serviceHubPort > 0);
    if (observer == NULL || reactor == NULL || serviceHubPort == 0)
    {
        return false;
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    CProFunctorCommandTask* task    = NULL;
    IRtpService*            service = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_task == NULL);
        assert(m_service == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_task != NULL || m_service != NULL)
        {
            return false;
        }

        task = new CProFunctorCommandTask;
        if (!task->Start())
        {
            goto EXIT;
        }

        service = CreateRtpService(
            m_sslConfig, this, reactor, m_mmType, serviceHubPort, timeoutInSeconds);
        if (service == NULL)
        {
            goto EXIT;
        }

        observer->AddRef();
        m_observer       = observer;
        m_reactor        = reactor;
        m_task           = task;
        m_service        = service;
        m_serviceHubPort = serviceHubPort;
    }

    return true;

EXIT:

    DeleteRtpService(service);
    delete task;

    return false;
}

void
CRtpMsgServer::Fini()
{
    IRtpMsgServerObserver*                      observer = NULL;
    CProFunctorCommandTask*                     task     = NULL;
    IRtpService*                                service  = NULL;
    CProStlMap<IRtpSession*, RTP_MSG_LINK_CTX*> session2Ctx;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        session2Ctx = m_session2Ctx;
        m_session2Ctx.clear();
        m_user2Ctx.clear();

        service = m_service;
        m_service = NULL;
        task = m_task;
        m_task = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    auto itr = session2Ctx.begin();
    auto end = session2Ctx.end();

    for (; itr != end; ++itr)
    {
        DeleteRtpSessionWrapper(itr->first);
        delete itr->second;
    }

    DeleteRtpService(service);
    delete task;
    observer->Release();
}

unsigned long
CRtpMsgServer::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CRtpMsgServer::Release()
{
    return CProRefCount::Release();
}

unsigned short
CRtpMsgServer::GetServicePort() const
{
    unsigned short servicePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        servicePort = m_serviceHubPort;
    }

    return servicePort;
}

PRO_SSL_SUITE_ID
CRtpMsgServer::GetSslSuite(const RTP_MSG_USER* user,
                           char                suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    assert(user != NULL);
    if (user == NULL)
    {
        return suiteId;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        auto itr = m_user2Ctx.find(*user);
        if (itr != m_user2Ctx.end())
        {
            const RTP_MSG_LINK_CTX* ctx = itr->second;

            suiteId = ctx->session->GetSslSuite(suiteName);
        }
    }

    return suiteId;
}

void
CRtpMsgServer::GetUserCount(size_t* pendingUserCount,   /* = NULL */
                            size_t* baseUserCount,      /* = NULL */
                            size_t* subUserCount) const /* = NULL */
{
    CProThreadMutexGuard mon(m_lock);

    if (pendingUserCount != NULL)
    {
        *pendingUserCount = m_task != NULL ? m_task->GetSize() : 0;
    }
    if (baseUserCount != NULL)
    {
        *baseUserCount    = m_session2Ctx.size();
    }
    if (subUserCount != NULL)
    {
        *subUserCount     = m_user2Ctx.size() - m_session2Ctx.size();
    }
}

void
CRtpMsgServer::KickoutUser(const RTP_MSG_USER* user)
{
    assert(user != NULL);
    if (user == NULL || user->classId == 0 || user->UserId() == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        IProFunctorCommand* command =
            CProFunctorCommand_cpp<CRtpMsgServer, ACTION>::CreateInstance(
                *this,
                &CRtpMsgServer::AsyncKickoutUser,
                (int64_t)user->classId,
                (int64_t)user->UserId(),
                (int64_t)user->instId
                );
        m_task->Put(command);
    }
}

void
CRtpMsgServer::AsyncKickoutUser(int64_t* args)
{
    RTP_MSG_USER user((unsigned char)args[0], args[1], (uint16_t)args[2]);
    assert(user.classId > 0);
    assert(user.UserId() > 0);

    IRtpMsgServerObserver*   observer   = NULL;
    IRtpSession*             oldSession = NULL;
    CProStlSet<RTP_MSG_USER> oldUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_user2Ctx.find(user);
        if (itr == m_user2Ctx.end())
        {
            return;
        }

        RTP_MSG_LINK_CTX* ctx = itr->second;

        if (user == ctx->baseUser)
        {
            oldSession = ctx->session;
            oldUsers   = ctx->subUsers;

            auto itr2 = oldUsers.begin();
            auto end2 = oldUsers.end();

            for (; itr2 != end2; ++itr2)
            {
                m_user2Ctx.erase(*itr2);
            }

            m_user2Ctx.erase(itr);
            m_session2Ctx.erase(oldSession);
            delete ctx;

            oldUsers.insert(user);
        }
        else
        {
            oldUsers.insert(user);

            ctx->subUsers.erase(user);
            m_user2Ctx.erase(itr);

            NotifyKickout(m_mmType, ctx->session, ctx->baseUser, user);
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    auto itr = oldUsers.begin();
    auto end = oldUsers.end();

    for (; itr != end; ++itr)
    {
        const RTP_MSG_USER& user2 = *itr;
        observer->OnCloseUser(this, &user2, -1, 0);
    }

    observer->Release();
    DeleteRtpSessionWrapper(oldSession);
}

bool
CRtpMsgServer::SendMsg(const void*         buf,
                       size_t              size,
                       uint16_t            charset,
                       const RTP_MSG_USER* dstUsers,
                       unsigned char       dstUserCount)
{
    return SendMsg2(buf, size, NULL, 0, charset, dstUsers, dstUserCount);
}

bool
CRtpMsgServer::SendMsg2(const void*         buf1,
                        size_t              size1,
                        const void*         buf2,  /* = NULL */
                        size_t              size2, /* = 0 */
                        uint16_t            charset,
                        const RTP_MSG_USER* dstUsers,
                        unsigned char       dstUserCount)
{
    assert(buf1 != NULL);
    assert(size1 > 0);
    assert(dstUsers != NULL);
    assert(dstUserCount > 0);
    if (buf1 == NULL || size1 == 0 || dstUsers == NULL || dstUserCount == 0)
    {
        return false;
    }

    bool                                                   ret          = true;
    unsigned char                                          sessionCount = 0;
    IRtpSession*                                           sessions[255];
    CProStlMap<IRtpSession*, CProStlVector<RTP_MSG_USER> > session2SubUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return false;
        }

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            if (dstUsers[i].classId == 0 || dstUsers[i].UserId() == 0 || dstUsers[i].IsRoot())
            {
                ret = false;
                continue;
            }

            auto itr = m_user2Ctx.find(dstUsers[i]);
            if (itr == m_user2Ctx.end())
            {
                ret = false;
                continue;
            }

            const RTP_MSG_LINK_CTX* dstCtx = itr->second;

            if (dstUsers[i] == dstCtx->baseUser)
            {
                dstCtx->session->AddRef();
                sessions[sessionCount] = dstCtx->session;
                ++sessionCount;
            }
            else
            {
                auto itr2 = session2SubUsers.find(dstCtx->session);
                if (itr2 != session2SubUsers.end())
                {
                    itr2->second.push_back(dstUsers[i]);
                }
                else
                {
                    dstCtx->session->AddRef();
                    session2SubUsers[dstCtx->session].push_back(dstUsers[i]);
                }
            }
        }
    }

    /*
     * to baseUsers
     */
    for (int i = 0; i < (int)sessionCount; ++i)
    {
        bool ret2 = SendMsgToDownlink(m_mmType, sessions + i, 1, buf1, size1,
            buf2, size2, charset, ROOT_ID, NULL, 0);
        if (!ret2)
        {
            ret = false;
        }

        sessions[i]->Release();
    }

    /*
     * to subUsers
     */
    {
        auto itr = session2SubUsers.begin();
        auto end = session2SubUsers.end();

        for (; itr != end; ++itr)
        {
            IRtpSession*                       session = itr->first;
            const CProStlVector<RTP_MSG_USER>& users   = itr->second;

            bool ret2 = SendMsgToDownlink(m_mmType, &session, 1, buf1, size1,
                buf2, size2, charset, ROOT_ID, &users[0], (unsigned char)users.size());
            if (!ret2)
            {
                ret = false;
            }

            session->Release();
        }
    }

    return ret;
}

void
CRtpMsgServer::SetOutputRedlineToC2s(size_t redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_redlineBytesC2s = redlineBytes;
    }
}

size_t
CRtpMsgServer::GetOutputRedlineToC2s() const
{
    size_t redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        redlineBytes = m_redlineBytesC2s;
    }

    return redlineBytes;
}

void
CRtpMsgServer::SetOutputRedlineToUsr(size_t redlineBytes)
{
    if (redlineBytes == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_redlineBytesUsr = redlineBytes;
    }
}

size_t
CRtpMsgServer::GetOutputRedlineToUsr() const
{
    size_t redlineBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        redlineBytes = m_redlineBytesUsr;
    }

    return redlineBytes;
}

size_t
CRtpMsgServer::GetSendingBytes(const RTP_MSG_USER* user) const
{
    assert(user != NULL);
    if (user == NULL)
    {
        return 0;
    }

    size_t sendingBytes = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        auto itr = m_user2Ctx.find(*user);
        if (itr != m_user2Ctx.end())
        {
            const RTP_MSG_LINK_CTX* ctx = itr->second;

            ctx->session->GetFlowctrlInfo(NULL, NULL, NULL, NULL, &sendingBytes, NULL);
        }
    }

    return sendingBytes;
}

void
CRtpMsgServer::OnAcceptSession(IRtpService*            service,
                               int64_t                 sockId,
                               bool                    unixSocket,
                               const char*             remoteIp,
                               unsigned short          remotePort,
                               const RTP_SESSION_INFO* remoteInfo,
                               const PRO_NONCE*        nonce)
{
    assert(service != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    assert(nonce != NULL);
    if (service == NULL || sockId == -1 || remoteIp == NULL || remoteInfo == NULL || nonce == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_TCPCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    assert(remoteInfo->packMode == RTP_MSG_PACK_MODE);
    if (remoteInfo->sessionType != RTP_ST_TCPCLIENT_EX || remoteInfo->mmType != m_mmType ||
        remoteInfo->packMode != RTP_MSG_PACK_MODE)
    {
        goto EXIT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            goto EXIT;
        }

        if (service != m_service)
        {
            goto EXIT;
        }

        if (m_task->GetSize() >= MAX_PENDING_COUNT)
        {
            goto EXIT;
        }

        RTP_MSG_AsyncOnAcceptSession* arg = new RTP_MSG_AsyncOnAcceptSession;
        arg->sockId     = sockId;
        arg->unixSocket = unixSocket;
        arg->remoteIp   = remoteIp;
        arg->remotePort = remotePort;
        arg->remoteInfo = *remoteInfo;
        arg->nonce      = *nonce;

        IProFunctorCommand* command =
            CProFunctorCommand_cpp<CRtpMsgServer, ACTION>::CreateInstance(
                *this,
                &CRtpMsgServer::AsyncOnAcceptSession,
                (int64_t)arg
                );
        m_task->Put(command);
    }

    return;

EXIT:

    ProCloseSockId(sockId);
}

void
CRtpMsgServer::OnAcceptSession(IRtpService*            service,
                               PRO_SSL_CTX*            sslCtx,
                               int64_t                 sockId,
                               bool                    unixSocket,
                               const char*             remoteIp,
                               unsigned short          remotePort,
                               const RTP_SESSION_INFO* remoteInfo,
                               const PRO_NONCE*        nonce)
{
    assert(service != NULL);
    assert(sslCtx != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(remoteInfo != NULL);
    assert(nonce != NULL);
    if (service == NULL || sslCtx == NULL || sockId == -1 || remoteIp == NULL ||
        remoteInfo == NULL || nonce == NULL)
    {
        return;
    }

    assert(remoteInfo->sessionType == RTP_ST_SSLCLIENT_EX);
    assert(remoteInfo->mmType == m_mmType);
    assert(remoteInfo->packMode == RTP_MSG_PACK_MODE);
    if (remoteInfo->sessionType != RTP_ST_SSLCLIENT_EX || remoteInfo->mmType != m_mmType ||
        remoteInfo->packMode != RTP_MSG_PACK_MODE)
    {
        goto EXIT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            goto EXIT;
        }

        if (service != m_service)
        {
            goto EXIT;
        }

        if (m_task->GetSize() >= MAX_PENDING_COUNT)
        {
            goto EXIT;
        }

        RTP_MSG_AsyncOnAcceptSession* arg = new RTP_MSG_AsyncOnAcceptSession;
        arg->sslCtx     = sslCtx;
        arg->sockId     = sockId;
        arg->unixSocket = unixSocket;
        arg->remoteIp   = remoteIp;
        arg->remotePort = remotePort;
        arg->remoteInfo = *remoteInfo;
        arg->nonce      = *nonce;

        IProFunctorCommand* command =
            CProFunctorCommand_cpp<CRtpMsgServer, ACTION>::CreateInstance(
                *this,
                &CRtpMsgServer::AsyncOnAcceptSession,
                (int64_t)arg
                );
        m_task->Put(command);
    }

    return;

EXIT:

    ProSslCtx_Delete(sslCtx);
    ProCloseSockId(sockId);
}

void
CRtpMsgServer::AsyncOnAcceptSession(int64_t* args)
{
    RTP_MSG_AsyncOnAcceptSession* arg = (RTP_MSG_AsyncOnAcceptSession*)args[0];

    assert(arg->sockId != -1);
    assert(
        arg->remoteInfo.sessionType == RTP_ST_TCPCLIENT_EX ||
        arg->remoteInfo.sessionType == RTP_ST_SSLCLIENT_EX
        );
    assert(arg->remoteInfo.mmType == m_mmType);
    assert(arg->remoteInfo.packMode == RTP_MSG_PACK_MODE);

    bool                   ret      = false;
    IRtpMsgServerObserver* observer = NULL;
    RTP_MSG_HEADER0        hdr0;
    RTP_MSG_USER           baseUser;
    uint64_t               userId   = 0;
    uint16_t               instId   = 0;
    int64_t                appData  = 0;
    bool                   isC2s    = false;

    if (m_sslConfig == NULL)
    {
        if (arg->remoteInfo.sessionType != RTP_ST_TCPCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else if (m_sslForced)
    {
        if (arg->remoteInfo.sessionType != RTP_ST_SSLCLIENT_EX)
        {
            goto EXIT;
        }
    }
    else
    {
    }

    memcpy(&hdr0, arg->remoteInfo.userData, sizeof(RTP_MSG_HEADER0));
    hdr0.version = pbsd_ntoh16(hdr0.version);

    baseUser        = hdr0.user;
    baseUser.instId = pbsd_ntoh16(baseUser.instId);

    assert(baseUser.classId > 0);
    assert(baseUser.UserId() <= NODE_UID_MAXX);
    assert(!baseUser.IsRoot());
    if (baseUser.classId == 0 || baseUser.UserId() > NODE_UID_MAXX || baseUser.IsRoot())
    {
        goto EXIT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            goto EXIT;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (baseUser.UserId() == 0)
    {
        baseUser.UserId(MakeUserId_i(baseUser.classId));
    }

    /*
     * preset value
     */
    userId = baseUser.UserId();
    instId = baseUser.instId;

    ret = observer->OnCheckUser(
        this,
        &baseUser,
        arg->remoteIp.c_str(),
        NULL,
        arg->remoteInfo.passwordHash,
        arg->nonce.nonce,
        &userId,
        &instId,
        &appData,
        &isC2s
        );
    observer->Release();

    if (!ret)
    {
        goto EXIT;
    }

    ret = false;

    assert(userId > 0);
    assert(userId <= NODE_UID_MAXX);
    if (userId == 0 || userId > NODE_UID_MAXX)
    {
        goto EXIT;
    }

    baseUser.UserId(userId);
    baseUser.instId = instId;

    memset(&hdr0, 0, sizeof(RTP_MSG_HEADER0));
    hdr0.version     = pbsd_hton16(RTP_MSG_PROTOCOL_VERSION);
    hdr0.user        = baseUser;
    hdr0.user.instId = pbsd_hton16(hdr0.user.instId);
    hdr0.publicIp    = pbsd_inet_aton(arg->remoteIp.c_str());

    RTP_SESSION_INFO localInfo;
    memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
    localInfo.remoteVersion = arg->remoteInfo.localVersion;
    localInfo.mmType        = m_mmType;
    localInfo.packMode      = arg->remoteInfo.packMode;

    RTP_INIT_ARGS initArgs;
    memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));

    if (arg->remoteInfo.sessionType == RTP_ST_SSLCLIENT_EX)
    {
        initArgs.sslserverEx.observer    = this;
        initArgs.sslserverEx.reactor     = m_reactor;
        initArgs.sslserverEx.sslCtx      = arg->sslCtx;
        initArgs.sslserverEx.sockId      = arg->sockId;
        initArgs.sslserverEx.unixSocket  = arg->unixSocket;
        initArgs.sslserverEx.suspendRecv = true; /* !!! */
        initArgs.sslserverEx.useAckData  = true;
        memcpy(initArgs.sslserverEx.ackData, &hdr0, sizeof(RTP_MSG_HEADER0));

        ret = AddBaseUser(RTP_ST_SSLSERVER_EX, initArgs, localInfo, baseUser,
            arg->remoteIp, appData, isC2s);
    }
    else
    {
        initArgs.tcpserverEx.observer    = this;
        initArgs.tcpserverEx.reactor     = m_reactor;
        initArgs.tcpserverEx.sockId      = arg->sockId;
        initArgs.tcpserverEx.unixSocket  = arg->unixSocket;
        initArgs.tcpserverEx.suspendRecv = true; /* !!! */
        initArgs.tcpserverEx.useAckData  = true;
        memcpy(initArgs.tcpserverEx.ackData, &hdr0, sizeof(RTP_MSG_HEADER0));

        ret = AddBaseUser(RTP_ST_TCPSERVER_EX, initArgs, localInfo, baseUser,
            arg->remoteIp, appData, isC2s);
    }

EXIT:

    if (!ret)
    {
        ProSslCtx_Delete(arg->sslCtx);
        ProCloseSockId(arg->sockId);
    }

    delete arg;
}

void
CRtpMsgServer::OnRecvSession(IRtpSession* session,
                             IRtpPacket*  packet)
{
    assert(session != NULL);
    assert(packet != NULL);
    assert(packet->GetPayloadSize() > sizeof(RTP_MSG_HEADER));
    if (session == NULL || packet == NULL || packet->GetPayloadSize() <= sizeof(RTP_MSG_HEADER))
    {
        return;
    }

    RTP_MSG_HEADER* msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    if (msgHeaderPtr->dstUserCount == 0)
    {
        msgHeaderPtr->dstUserCount = 1;
    }

    size_t msgHeaderSize =
        sizeof(RTP_MSG_HEADER) + sizeof(RTP_MSG_USER) * (msgHeaderPtr->dstUserCount - 1);

    const void* msgBodyPtr  = (char*)msgHeaderPtr + msgHeaderSize;
    int         msgBodySize = (int)(packet->GetPayloadSize() - msgHeaderSize);
    uint16_t    charset     = pbsd_ntoh16(msgHeaderPtr->charset);

    RTP_MSG_USER srcUser = msgHeaderPtr->srcUser;
    srcUser.instId       = pbsd_ntoh16(srcUser.instId);

    assert(msgBodySize > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (msgBodySize == 0 || srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return;
    }

    IRtpMsgServerObserver*                                 observer     = NULL;
    unsigned char                                          sessionCount = 0;
    IRtpSession*                                           sessions[255];
    CProStlMap<IRtpSession*, CProStlVector<RTP_MSG_USER> > session2SubUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_session2Ctx.find(session);
        if (itr == m_session2Ctx.end())
        {
            return;
        }

        const RTP_MSG_LINK_CTX* srcCtx = itr->second;
        if (srcUser != srcCtx->baseUser &&
            srcCtx->subUsers.find(srcUser) == srcCtx->subUsers.end())
        {
            return;
        }

        bool toStdPort = false;
        bool toC2sPort = false;

        for (int i = 0; i < (int)msgHeaderPtr->dstUserCount; ++i)
        {
            RTP_MSG_USER dstUser = msgHeaderPtr->dstUsers[i];
            dstUser.instId       = pbsd_ntoh16(dstUser.instId);

            if (dstUser.classId == 0 || dstUser.UserId() == 0)
            {
                continue;
            }

            if (dstUser.IsRoot())
            {
                if (dstUser.instId == ROOT_ID_C2SPORT.instId)
                {
                    toC2sPort = true;
                }
                else
                {
                    toStdPort = true;
                }
                continue;
            }

            auto itr2 = m_user2Ctx.find(dstUser);
            if (itr2 == m_user2Ctx.end())
            {
                continue;
            }

            const RTP_MSG_LINK_CTX* dstCtx = itr2->second;

            if (dstUser == dstCtx->baseUser)
            {
                dstCtx->session->AddRef();
                sessions[sessionCount] = dstCtx->session;
                ++sessionCount;
            }
            else
            {
                auto itr3 = session2SubUsers.find(dstCtx->session);
                if (itr3 != session2SubUsers.end())
                {
                    itr3->second.push_back(dstUser);
                }
                else
                {
                    dstCtx->session->AddRef();
                    session2SubUsers[dstCtx->session].push_back(dstUser);
                }
            }
        } /* end of for () */

        /*
         * to c2sPort
         */
        do
        {
            if (!toC2sPort || !srcCtx->isC2s || srcUser != srcCtx->baseUser)
            {
                break;
            }

            CProStlString                  theString((char*)msgBodyPtr, msgBodySize);
            CProStlVector<PRO_CONFIG_ITEM> theConfigs;

            if (!CProConfigStream::StringToConfigs(theString, theConfigs))
            {
                break;
            }

            RTP_MSG_AsyncOnRecvSession* arg = new RTP_MSG_AsyncOnRecvSession;
            arg->session = session;
            arg->c2sUser = srcUser;
            arg->msgStream.Add(theConfigs);

            CProStlString msgName;
            arg->msgStream.Get(TAG_msg_name, msgName);

            if (stricmp(msgName.c_str(), MSG_client_login) == 0)
            {
                if (m_task->GetSize() >= MAX_PENDING_COUNT)
                {
                    delete arg;
                    break;
                }
            }
            else if (stricmp(msgName.c_str(), MSG_client_logout) == 0)
            {
            }
            else
            {
                delete arg;
                break;
            }

            arg->session->AddRef();
            IProFunctorCommand* command =
                CProFunctorCommand_cpp<CRtpMsgServer, ACTION>::CreateInstance(
                    *this,
                    &CRtpMsgServer::AsyncOnRecvSession,
                    (int64_t)arg
                    );
            m_task->Put(command);
        }
        while (0);

        /*
         * to stdPort
         */
        if (
            toStdPort
            ||
            (toC2sPort && !srcCtx->isC2s)
           )
        {
            m_observer->AddRef();
            observer = m_observer;
        }
    }

    /*
     * to baseUsers
     */
    for (int i = 0; i < (int)sessionCount; ++i)
    {
        SendMsgToDownlink(m_mmType, sessions + i, 1, msgBodyPtr, msgBodySize,
            NULL, 0, charset, srcUser, NULL, 0);
        sessions[i]->Release();
    }

    /*
     * to subUsers
     */
    {
        auto itr = session2SubUsers.begin();
        auto end = session2SubUsers.end();

        for (; itr != end; ++itr)
        {
            IRtpSession*                       session = itr->first;
            const CProStlVector<RTP_MSG_USER>& users   = itr->second;

            SendMsgToDownlink(m_mmType, &session, 1, msgBodyPtr, msgBodySize,
                NULL, 0, charset, srcUser, &users[0], (unsigned char)users.size());
            session->Release();
        }
    }

    /*
     * to root
     */
    if (observer != NULL)
    {
        observer->OnRecvMsg(this, msgBodyPtr, msgBodySize, charset, &srcUser);
        observer->Release();
    }
}

void
CRtpMsgServer::AsyncOnRecvSession(int64_t* args)
{
    RTP_MSG_AsyncOnRecvSession* arg = (RTP_MSG_AsyncOnRecvSession*)args[0];

    CProStlString msgName;
    arg->msgStream.Get(TAG_msg_name, msgName);

    if (stricmp(msgName.c_str(), MSG_client_login) == 0)
    {
        ProcessMsg_client_login(arg->session, arg->msgStream, arg->c2sUser);
    }
    else if (stricmp(msgName.c_str(), MSG_client_logout) == 0)
    {
        ProcessMsg_client_logout(arg->session, arg->msgStream, arg->c2sUser);
    }
    else
    {
    }

    arg->session->Release();
    delete arg;
}

void
CRtpMsgServer::ProcessMsg_client_login(IRtpSession*            session,
                                       const CProConfigStream& msgStream,
                                       const RTP_MSG_USER&     c2sUser)
{
    assert(session != NULL);
    assert(c2sUser.classId > 0);
    assert(c2sUser.UserId() > 0);
    if (session == NULL || c2sUser.classId == 0 || c2sUser.UserId() == 0)
    {
        return;
    }

    uint64_t      client_index;
    CProStlString client_id;
    CProStlString client_public_ip;
    CProStlString client_hash_string;
    CProStlString client_nonce_string;

    msgStream.GetUint64(TAG_client_index       , client_index);
    msgStream.Get      (TAG_client_id          , client_id);
    msgStream.Get      (TAG_client_public_ip   , client_public_ip);
    msgStream.Get      (TAG_client_hash_string , client_hash_string);
    msgStream.Get      (TAG_client_nonce_string, client_nonce_string);

    RTP_MSG_USER subUser;
    RtpMsgString2User(client_id.c_str(), &subUser);

    assert(subUser.classId > 0);
    assert(subUser.UserId() <= NODE_UID_MAXX);
    assert(!subUser.IsRoot());
    assert(subUser != c2sUser);
    if (subUser.classId == 0 || subUser.UserId() > NODE_UID_MAXX || subUser.IsRoot() ||
        subUser == c2sUser)
    {
        return;
    }

    assert(client_hash_string.length() == 64);
    assert(client_nonce_string.length() == 64);
    if (client_hash_string.length() != 64 || client_nonce_string.length() != 64)
    {
        return;
    }

    unsigned char hash[32];
    unsigned char nonce[32];

    {
        const char* p = client_hash_string.c_str();

        for (int i = 0; i < 32; ++i)
        {
            char tmpString[3] = { 0 };
            tmpString[0] = p[i * 2];
            tmpString[1] = p[i * 2 + 1];

            unsigned int tmpInt = 0;
            sscanf(tmpString, "%02x", &tmpInt);

            hash[i] = (unsigned char)tmpInt;
        }
    }

    {
        const char* p = client_nonce_string.c_str();

        for (int i = 0; i < 32; ++i)
        {
            char tmpString[3] = { 0 };
            tmpString[0] = p[i * 2];
            tmpString[1] = p[i * 2 + 1];

            unsigned int tmpInt = 0;
            sscanf(tmpString, "%02x", &tmpInt);

            nonce[i] = (unsigned char)tmpInt;
        }
    }

    bool                   ret      = false;
    IRtpMsgServerObserver* observer = NULL;
    uint64_t               userId   = 0;
    uint16_t               instId   = 0;
    int64_t                appData  = 0;
    bool                   isC2s    = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (subUser.UserId() == 0)
    {
        if (subUser.classId == SERVER_CID)
        {
            subUser.UserId(MakeServerId_i());
        }
        else
        {
            subUser.UserId(MakeClientId_i());
        }
    }

    /*
     * preset value
     */
    userId = subUser.UserId();
    instId = subUser.instId;

    ret = observer->OnCheckUser(
        this,
        &subUser,
        client_public_ip.c_str(),
        &c2sUser,
        hash,
        nonce,
        &userId,
        &instId,
        &appData,
        &isC2s
        );
    observer->Release();

    if (ret)
    {
        subUser.UserId(userId);
        subUser.instId = instId;

        assert(userId > 0);
        assert(userId <= NODE_UID_MAXX);
        assert(subUser != c2sUser);
        if (userId == 0 || userId > NODE_UID_MAXX || subUser == c2sUser)
        {
            ret = false;
        }
    }

    if (ret)
    {
        char idString[64] = "";
        RtpMsgUser2String(&subUser, idString);

        CProConfigStream msgStream;
        msgStream.Add      (TAG_msg_name    , MSG_client_login_ok);
        msgStream.AddUint64(TAG_client_index, client_index);
        msgStream.Add      (TAG_client_id   , idString);

        CProStlString theString;
        msgStream.ToString(theString);

        AddSubUser(c2sUser, subUser, client_public_ip, theString, appData);
    }
    else
    {
        CProConfigStream msgStream;
        msgStream.Add      (TAG_msg_name    , MSG_client_login_error);
        msgStream.AddUint64(TAG_client_index, client_index);

        CProStlString theString;
        msgStream.ToString(theString);

        SendMsgToDownlink(m_mmType, &session, 1, theString.c_str(), theString.length(),
            NULL, 0, 0, ROOT_ID_C2S, &c2sUser, 1);
    }
}

void
CRtpMsgServer::ProcessMsg_client_logout(IRtpSession*            session,
                                        const CProConfigStream& msgStream,
                                        const RTP_MSG_USER&     c2sUser)
{
    assert(session != NULL);
    assert(c2sUser.classId > 0);
    assert(c2sUser.UserId() > 0);
    if (session == NULL || c2sUser.classId == 0 || c2sUser.UserId() == 0)
    {
        return;
    }

    CProStlString client_id;
    msgStream.Get(TAG_client_id, client_id);

    RTP_MSG_USER subUser;
    RtpMsgString2User(client_id.c_str(), &subUser);

    assert(subUser.classId > 0);
    assert(subUser.UserId() > 0);
    assert(subUser != c2sUser);
    if (subUser.classId == 0 || subUser.UserId() == 0 || subUser == c2sUser)
    {
        return;
    }

    IRtpMsgServerObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_user2Ctx.find(subUser);
        if (itr == m_user2Ctx.end())
        {
            return;
        }

        RTP_MSG_LINK_CTX* ctx = itr->second;
        if (!ctx->isC2s || c2sUser != ctx->baseUser)
        {
            return;
        }

        ctx->subUsers.erase(subUser);
        m_user2Ctx.erase(itr);

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnCloseUser(this, &subUser, -1, 0);
    observer->Release();
}

void
CRtpMsgServer::OnCloseSession(IRtpSession* session,
                              int          errorCode,
                              int          sslCode,
                              bool         tcpConnected)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        session->AddRef();
        IProFunctorCommand* command =
            CProFunctorCommand_cpp<CRtpMsgServer, ACTION>::CreateInstance(
                *this,
                &CRtpMsgServer::AsyncOnCloseSession,
                (int64_t)session,
                (int64_t)errorCode,
                (int64_t)sslCode
                );
        m_task->Put(command);
    }
}

void
CRtpMsgServer::AsyncOnCloseSession(int64_t* args)
{
    IRtpSession* session   = (IRtpSession*)args[0];
    int          errorCode = (int)         args[1];
    int          sslCode   = (int)         args[2];

    IRtpMsgServerObserver*   observer = NULL;
    CProStlSet<RTP_MSG_USER> oldUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            session->Release();

            return;
        }

        auto itr = m_session2Ctx.find(session);
        if (itr == m_session2Ctx.end())
        {
            session->Release();

            return;
        }

        RTP_MSG_LINK_CTX* ctx = itr->second;
        session->Release();

        oldUsers = ctx->subUsers;
        oldUsers.insert(ctx->baseUser);

        auto itr2 = oldUsers.begin();
        auto end2 = oldUsers.end();

        for (; itr2 != end2; ++itr2)
        {
            m_user2Ctx.erase(*itr2);
        }

        m_session2Ctx.erase(itr);
        delete ctx;

        m_observer->AddRef();
        observer = m_observer;
    }

    auto itr = oldUsers.begin();
    auto end = oldUsers.end();

    for (; itr != end; ++itr)
    {
        const RTP_MSG_USER& user = *itr;
        observer->OnCloseUser(this, &user, errorCode, sslCode);
    }

    observer->Release();
    DeleteRtpSessionWrapper(session);
}

void
CRtpMsgServer::OnHeartbeatSession(IRtpSession* session,
                                  int64_t      peerAliveTick)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    IRtpMsgServerObserver* observer = NULL;
    RTP_MSG_USER           user;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_session2Ctx.find(session);
        if (itr == m_session2Ctx.end())
        {
            return;
        }

        const RTP_MSG_LINK_CTX* ctx = itr->second;
        user = ctx->baseUser;

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnHeartbeatUser(this, &user, peerAliveTick);
    observer->Release();
}

bool
CRtpMsgServer::AddBaseUser(RTP_SESSION_TYPE        sessionType,
                           const RTP_INIT_ARGS&    initArgs,
                           const RTP_SESSION_INFO& localInfo,
                           const RTP_MSG_USER&     baseUser,
                           const CProStlString&    publicIp,
                           int64_t                 appData,
                           bool                    isC2s)
{
    assert(sessionType == RTP_ST_TCPSERVER_EX || sessionType == RTP_ST_SSLSERVER_EX);
    assert(localInfo.mmType == m_mmType);
    assert(localInfo.packMode == RTP_MSG_PACK_MODE);

    assert(baseUser.classId > 0);
    assert(baseUser.UserId() > 0);
    if (baseUser.classId == 0 || baseUser.UserId() == 0)
    {
        return false;
    }

    IRtpMsgServerObserver*   observer   = NULL;
    IRtpSession*             newSession = NULL;
    IRtpSession*             oldSession = NULL;
    CProStlSet<RTP_MSG_USER> oldUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return false;
        }

        newSession = CreateRtpSessionWrapper(sessionType, &initArgs, &localInfo);
        if (newSession == NULL)
        {
            return false;
        }

        newSession->SetOutputRedline(isC2s ? m_redlineBytesC2s : m_redlineBytesUsr, 0, 0);

        /*
         * remove old
         */
        auto itr = m_user2Ctx.find(baseUser);
        if (itr != m_user2Ctx.end())
        {
            RTP_MSG_LINK_CTX* ctx = itr->second;

            if (baseUser == ctx->baseUser)
            {
                oldSession = ctx->session;
                oldUsers   = ctx->subUsers;

                auto itr2 = oldUsers.begin();
                auto end2 = oldUsers.end();

                for (; itr2 != end2; ++itr2)
                {
                    m_user2Ctx.erase(*itr2);
                }

                m_user2Ctx.erase(itr);
                m_session2Ctx.erase(oldSession);
                delete ctx;

                oldUsers.insert(baseUser);
            }
            else
            {
                oldUsers.insert(baseUser);

                ctx->subUsers.erase(baseUser);
                m_user2Ctx.erase(itr);

                NotifyKickout(m_mmType, ctx->session, ctx->baseUser, baseUser);
            }
        }

        /*
         * add new
         */
        RTP_MSG_LINK_CTX* ctx = new RTP_MSG_LINK_CTX;
        ctx->session  = newSession;
        ctx->baseUser = baseUser;
        ctx->isC2s    = isC2s;
        m_session2Ctx[newSession] = ctx;
        m_user2Ctx[baseUser]      = ctx;

        newSession->AddRef();
        m_observer->AddRef();
        observer = m_observer;
    }

    auto itr = oldUsers.begin();
    auto end = oldUsers.end();

    for (; itr != end; ++itr)
    {
        const RTP_MSG_USER& user = *itr;
        observer->OnCloseUser(this, &user, -1, 0);
    }

    observer->OnOkUser(this, &baseUser, publicIp.c_str(), NULL, appData);
    observer->Release();

    newSession->ResumeRecv(); /* !!! */
    newSession->Release();
    DeleteRtpSessionWrapper(oldSession);

    return true;
}

void
CRtpMsgServer::AddSubUser(const RTP_MSG_USER&  c2sUser,
                          const RTP_MSG_USER&  subUser,
                          const CProStlString& publicIp,
                          const CProStlString& msgText,
                          int64_t              appData)
{
    assert(c2sUser.classId > 0);
    assert(c2sUser.UserId() > 0);
    assert(subUser.classId > 0);
    assert(subUser.UserId() > 0);
    assert(subUser != c2sUser);
    if (c2sUser.classId == 0 || c2sUser.UserId() == 0 ||
        subUser.classId == 0 || subUser.UserId() == 0 || subUser == c2sUser)
    {
        return;
    }

    IRtpMsgServerObserver*   observer   = NULL;
    IRtpSession*             newSession = NULL;
    IRtpSession*             oldSession = NULL;
    CProStlSet<RTP_MSG_USER> oldUsers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_task == NULL || m_service == NULL)
        {
            return;
        }

        auto itr = m_user2Ctx.find(c2sUser);
        if (itr == m_user2Ctx.end())
        {
            return;
        }

        RTP_MSG_LINK_CTX* newCtx = itr->second;
        if (!newCtx->isC2s || c2sUser != newCtx->baseUser)
        {
            return;
        }

        newSession = newCtx->session;

        /*
         * remove old
         */
        itr = m_user2Ctx.find(subUser);
        if (itr != m_user2Ctx.end())
        {
            RTP_MSG_LINK_CTX* oldCtx = itr->second;

            if (subUser == oldCtx->baseUser)
            {
                oldSession = oldCtx->session;
                oldUsers   = oldCtx->subUsers;

                auto itr2 = oldUsers.begin();
                auto end2 = oldUsers.end();

                for (; itr2 != end2; ++itr2)
                {
                    m_user2Ctx.erase(*itr2);
                }

                m_user2Ctx.erase(itr);
                m_session2Ctx.erase(oldSession);
                delete oldCtx;

                oldUsers.insert(subUser);
            }
            else
            {
                oldUsers.insert(subUser);

                oldCtx->subUsers.erase(subUser);
                m_user2Ctx.erase(itr);

                if (c2sUser != oldCtx->baseUser)
                {
                    NotifyKickout(m_mmType, oldCtx->session, oldCtx->baseUser, subUser);
                }
            }
        }

        /*
         * add new
         */
        newCtx->subUsers.insert(subUser);
        m_user2Ctx[subUser] = newCtx;

        newSession->AddRef();
        m_observer->AddRef();
        observer = m_observer;
    }

    auto itr = oldUsers.begin();
    auto end = oldUsers.end();

    for (; itr != end; ++itr)
    {
        const RTP_MSG_USER& user = *itr;
        observer->OnCloseUser(this, &user, -1, 0);
    }

    /*
     * 1. make a callback
     */
    observer->OnOkUser(this, &subUser, publicIp.c_str(), &c2sUser, appData);
    observer->Release();

    /*
     * 2. response
     */
    SendMsgToDownlink(m_mmType, &newSession, 1, msgText.c_str(), msgText.length(),
        NULL, 0, 0, ROOT_ID_C2S, &c2sUser, 1);

    newSession->ResumeRecv(); /* !!! */
    newSession->Release();
    DeleteRtpSessionWrapper(oldSession);
}

bool
CRtpMsgServer::SendMsgToDownlink(RTP_MM_TYPE         mmType,
                                 IRtpSession**       sessions,
                                 unsigned char       sessionCount,
                                 const void*         buf1,
                                 size_t              size1,
                                 const void*         buf2,         /* = NULL */
                                 size_t              size2,        /* = 0 */
                                 uint16_t            charset,
                                 const RTP_MSG_USER& srcUser,
                                 const RTP_MSG_USER* dstUsers,     /* = NULL */
                                 unsigned char       dstUserCount) /* = 0 */
{
    assert(sessions != NULL);
    assert(sessionCount > 0);
    assert(buf1 != NULL);
    assert(size1 > 0);
    assert(srcUser.classId > 0);
    assert(srcUser.UserId() > 0);
    if (sessions == NULL || sessionCount == 0 || buf1 == NULL || size1 == 0 ||
        srcUser.classId == 0 || srcUser.UserId() == 0)
    {
        return false;
    }

    if (buf2 == NULL || size2 == 0)
    {
        buf2  = NULL;
        size2 = 0;
    }

    if (dstUsers == NULL || dstUserCount == 0)
    {
        dstUsers     = NULL;
        dstUserCount = 1;
    }

    size_t msgHeaderSize = sizeof(RTP_MSG_HEADER) + sizeof(RTP_MSG_USER) * (dstUserCount - 1);

    IRtpPacket* packet = CreateRtpPacketSpace(msgHeaderSize + size1 + size2, RTP_MSG_PACK_MODE);
    if (packet == NULL)
    {
        return false;
    }

    RTP_MSG_HEADER* msgHeaderPtr = (RTP_MSG_HEADER*)packet->GetPayloadBuffer();
    memset(msgHeaderPtr, 0, msgHeaderSize);

    msgHeaderPtr->charset        = pbsd_hton16(charset);
    msgHeaderPtr->srcUser        = srcUser;
    msgHeaderPtr->srcUser.instId = pbsd_hton16(msgHeaderPtr->srcUser.instId);

    if (dstUsers != NULL && dstUserCount > 0)
    {
        msgHeaderPtr->dstUserCount = dstUserCount;

        for (int i = 0; i < (int)dstUserCount; ++i)
        {
            msgHeaderPtr->dstUsers[i]        = dstUsers[i];
            msgHeaderPtr->dstUsers[i].instId = pbsd_hton16(msgHeaderPtr->dstUsers[i].instId);
        }
    }

    memcpy((char*)msgHeaderPtr + msgHeaderSize, buf1, size1);
    if (buf2 != NULL && size2 > 0)
    {
        memcpy((char*)msgHeaderPtr + msgHeaderSize + size1, buf2, size2);
    }

    packet->SetMmType(mmType);

    bool ret = true;

    for (int i = 0; i < (int)sessionCount; ++i)
    {
        assert(sessions[i] != NULL);
        if (!sessions[i]->SendPacket(packet))
        {
            ret = false;
        }
    }

    packet->Release();

    return ret;
}

void
CRtpMsgServer::NotifyKickout(RTP_MM_TYPE         mmType,
                             IRtpSession*        session,
                             const RTP_MSG_USER& c2sUser,
                             const RTP_MSG_USER& subUser)
{
    assert(session != NULL);
    assert(c2sUser.classId > 0);
    assert(c2sUser.UserId() > 0);
    assert(subUser.classId > 0);
    assert(subUser.UserId() > 0);
    assert(subUser != c2sUser);
    if (session == NULL || c2sUser.classId == 0 || c2sUser.UserId() == 0 ||
        subUser.classId == 0 || subUser.UserId() == 0 || subUser == c2sUser)
    {
        return;
    }

    char idString[64] = "";
    RtpMsgUser2String(&subUser, idString);

    CProConfigStream msgStream;
    msgStream.Add(TAG_msg_name , MSG_client_kickout);
    msgStream.Add(TAG_client_id, idString);

    CProStlString theString;
    msgStream.ToString(theString);

    SendMsgToDownlink(mmType, &session, 1, theString.c_str(), theString.length(),
        NULL, 0, 0, ROOT_ID_C2S, &c2sUser, 1);
}
