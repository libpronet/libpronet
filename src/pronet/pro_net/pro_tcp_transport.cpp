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

#include "pro_tcp_transport.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_send_pool.h"
#include "pro_service_pipe.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_RECV_POOL_SIZE (1024 * 65)

/*
 * Linux uses double-size values
 *
 * please refer to "/usr/src/linux-a.b.c.d/net/core/sock.c",
 * sock_setsockopt()
 */
#define DEFAULT_RECV_BUF_SIZE  (1024 * 56)
#define DEFAULT_SEND_BUF_SIZE  (1024 * 8)

#if !defined(WIN32) && !defined(_WIN32_WCE)

union PRO_CMSG_CTRL
{
    struct cmsghdr cmsg;
    char           control[CMSG_SPACE(sizeof(int))];
};

#endif /* WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

CProTcpTransport*
CProTcpTransport::CreateInstance(bool   recvFdMode,
                                 size_t recvPoolSize)   /* = 0 */
{
#if defined(WIN32) || defined(_WIN32_WCE)
    recvFdMode = false;
#endif

    CProTcpTransport* const trans =
        new CProTcpTransport(recvFdMode, recvPoolSize);

    return (trans);
}

CProTcpTransport::CProTcpTransport(bool   recvFdMode,
                                   size_t recvPoolSize) /* = 0 */
                                   :
m_recvFdMode(recvFdMode),
m_recvPoolSize(recvPoolSize > 0 ? recvPoolSize : DEFAULT_RECV_POOL_SIZE)
{
    m_observer      = NULL;
    m_reactorTask   = NULL;
    m_sockId        = -1;
    m_onWr          = false;
    m_pendingWr     = false;
    m_requestOnSend = false;
    m_sendingFd     = -1;
    m_timerId       = 0;

    m_canUpcall     = true;

    memset(&m_localAddr , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddr, 0, sizeof(pbsd_sockaddr_in));
}

CProTcpTransport::~CProTcpTransport()
{
    Fini();

    ProCloseSockId(m_sockId, true);
    m_sockId = -1;
}

bool
CProTcpTransport::Init(IProTransportObserver* observer,
                       CProTpReactorTask*     reactorTask,
                       PRO_INT64              sockId,
                       bool                   unixSocket,
                       size_t                 sockBufSizeRecv, /* = 0 */
                       size_t                 sockBufSizeSend, /* = 0 */
                       bool                   suspendRecv)     /* = false */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactorTask == NULL || sockId == -1)
    {
        return (false);
    }

    if (sockBufSizeRecv == 0)
    {
        sockBufSizeRecv = DEFAULT_RECV_BUF_SIZE;
    }
    if (sockBufSizeSend == 0)
    {
        sockBufSizeSend = DEFAULT_SEND_BUF_SIZE;
    }

    if (!unixSocket)
    {
        int option;
        option = (int)sockBufSizeRecv;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)sockBufSizeSend;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return (false);
        }

#if !defined(WIN32) && !defined(_WIN32_WCE)
        if (unixSocket)
        {
            pbsd_sockaddr_un localAddrUn;
            pbsd_sockaddr_un remoteAddrUn;
            if (pbsd_getsockname_un(sockId, &localAddrUn)  != 0 ||
                pbsd_getpeername_un(sockId, &remoteAddrUn) != 0)
            {
                return (false);
            }

            const char* const prefix = "/tmp/libpronet_127001_";
            const char* const local  = strstr(localAddrUn.sun_path , prefix);
            const char* const remote = strstr(remoteAddrUn.sun_path, prefix);
            if (local == NULL && remote == NULL)
            {
                return (false);
            }

            if (local != NULL)
            {
                int port = 0;
                sscanf(local + strlen(prefix), "%d", &port);
                if (port <= 0 || port > 65535)
                {
                    return (false);
                }

                m_localAddr.sin_port         = pbsd_hton16((PRO_UINT16)port);
                m_localAddr.sin_addr.s_addr  = pbsd_inet_aton("127.0.0.1");
                m_remoteAddr.sin_port        = pbsd_hton16((PRO_UINT16)65535); /* a dummy for unix socket */
                m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton("127.0.0.1");
            }
            else
            {
                int port = 0;
                sscanf(remote + strlen(prefix), "%d", &port);
                if (port <= 0 || port > 65535)
                {
                    return (false);
                }

                m_localAddr.sin_port         = pbsd_hton16((PRO_UINT16)65535); /* a dummy for unix socket */
                m_localAddr.sin_addr.s_addr  = pbsd_inet_aton("127.0.0.1");
                m_remoteAddr.sin_port        = pbsd_hton16((PRO_UINT16)port);
                m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton("127.0.0.1");
            }
        }
        else
#endif
        {
            if (pbsd_getsockname(sockId, &m_localAddr)  != 0 ||
                pbsd_getpeername(sockId, &m_remoteAddr) != 0)
            {
                return (false);
            }
        }

        if (!m_recvPool.Resize(m_recvPoolSize))
        {
            return (false);
        }

        if (!suspendRecv &&
            !reactorTask->AddHandler(sockId, this, PRO_MASK_READ))
        {
            return (false);
        }

        observer->AddRef();
        m_observer    = observer;
        m_reactorTask = reactorTask;
        m_sockId      = sockId;
    }

    return (true);
}

void
CProTcpTransport::Fini()
{
    IProTransportObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(
            m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->Release();
}

unsigned long
PRO_CALLTYPE
CProTcpTransport::AddRef()
{
    const unsigned long refCount = CProEventHandler::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProTcpTransport::Release()
{
    const unsigned long refCount = CProEventHandler::Release();

    return (refCount);
}

PRO_TRANS_TYPE
PRO_CALLTYPE
CProTcpTransport::GetType() const
{
    return (PRO_TRANS_TCP);
}

PRO_SSL_SUITE_ID
PRO_CALLTYPE
CProTcpTransport::GetSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    return (PRO_SSL_SUITE_NONE);
}

PRO_INT64
PRO_CALLTYPE
CProTcpTransport::GetSockId() const
{
    PRO_INT64 sockId = -1;

    {
        CProThreadMutexGuard mon(m_lock);

        sockId = m_sockId;
    }

    return (sockId);
}

const char*
PRO_CALLTYPE
CProTcpTransport::GetLocalIp(char localIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_localAddr.sin_addr.s_addr, localIp);
    }

    return (localIp);
}

unsigned short
PRO_CALLTYPE
CProTcpTransport::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        localPort = pbsd_ntoh16(m_localAddr.sin_port);
    }

    return (localPort);
}

const char*
PRO_CALLTYPE
CProTcpTransport::GetRemoteIp(char remoteIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);
    }

    return (remoteIp);
}

unsigned short
PRO_CALLTYPE
CProTcpTransport::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        remotePort = pbsd_ntoh16(m_remoteAddr.sin_port);
    }

    return (remotePort);
}

IProRecvPool*
PRO_CALLTYPE
CProTcpTransport::GetRecvPool()
{
    return (&m_recvPool);
}

bool
PRO_CALLTYPE
CProTcpTransport::SendData(const void*             buf,
                           size_t                  size,
                           PRO_UINT64              actionId,   /* = 0 */
                           const pbsd_sockaddr_in* remoteAddr) /* = NULL */
{
    assert(buf != NULL);
    assert(size > 0);
    if (buf == NULL || size == 0)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return (false);
        }

        if (m_pendingWr)
        {
            return (false);
        }

        if (!m_onWr)
        {
            if (!m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE))
            {
                return (false);
            }

            m_onWr = true;
        }

        m_sendPool.Fill(buf, size, actionId);
        m_pendingWr = true;
    }

    return (true);
}

bool
CProTcpTransport::SendFd(const PRO_SERVICE_PACKET& s2cPacket)
{
    assert(s2cPacket.s2c.oldSock.sockId != -1);
    assert(s2cPacket.CheckMagic());
    if (s2cPacket.s2c.oldSock.sockId == -1 || !s2cPacket.CheckMagic())
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return (false);
        }

        if (m_pendingWr)
        {
            return (false);
        }

        if (!m_onWr)
        {
            if (!m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE))
            {
                return (false);
            }

            m_onWr = true;
        }

        m_sendPool.Fill(&s2cPacket, sizeof(PRO_SERVICE_PACKET));
        m_sendingFd = s2cPacket.s2c.oldSock.sockId;
        m_pendingWr = true;
    }

    return (true);
}

void
PRO_CALLTYPE
CProTcpTransport::RequestOnSend()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (m_requestOnSend)
        {
            return;
        }

        if (!m_onWr)
        {
            if (!m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE))
            {
                return;
            }

            m_onWr = true;
        }

        m_requestOnSend = true;
    }
}

void
PRO_CALLTYPE
CProTcpTransport::SuspendRecv()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);
    }
}

void
PRO_CALLTYPE
CProTcpTransport::ResumeRecv()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_READ);
    }
}

void
PRO_CALLTYPE
CProTcpTransport::StartHeartbeat()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (m_timerId == 0)
        {
            m_timerId = m_reactorTask->ScheduleHeartbeatTimer(this, 0);
        }
    }
}

void
PRO_CALLTYPE
CProTcpTransport::StopHeartbeat()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;
    }
}

void
PRO_CALLTYPE
CProTcpTransport::OnInput(PRO_INT64 sockId)
{
    if (m_recvFdMode)
    {
        OnInputFd(sockId);
    }
    else
    {
        OnInputData(sockId);
    }
}

void
CProTcpTransport::OnInputData(PRO_INT64 sockId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer  = NULL;
    int                    recvSize  = 0;
    int                    errorCode = 0;
    const int              sslCode   = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        const size_t idleSize = m_recvPool.ContinuousIdleSize();

        assert(idleSize > 0);
        if (idleSize == 0)
        {
            recvSize  = -1;
            errorCode = -1;

            goto EXIT;
        }

        recvSize = pbsd_recv(
            m_sockId, m_recvPool.ContinuousIdleBuf(), (int)idleSize, 0);
        assert(recvSize <= (int)idleSize);

        if (recvSize > (int)idleSize)
        {
            recvSize  = -1;
            errorCode = -1;
        }
        else if (recvSize > 0)
        {
            m_recvPool.Fill(recvSize);
        }
        else if (recvSize == 0)
        {
        }
        else
        {
            errorCode = pbsd_errno((void*)&pbsd_recv);
        }

EXIT:

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (recvSize > 0)
        {
            observer->OnRecv(this, &m_remoteAddr);
            assert(m_recvPool.ContinuousIdleSize() > 0);
        }
        else if (
            (recvSize < 0 && errorCode != PBSD_EWOULDBLOCK)
            ||
            recvSize == 0
           )
        {
            m_canUpcall = false;
            observer->OnClose(this, errorCode, sslCode);
        }
        else
        {
        }
    }

    observer->Release();

    if (!m_canUpcall)
    {
        Fini();
    }
}}

void
CProTcpTransport::OnInputFd(PRO_INT64 sockId)
{{
#if !defined(WIN32) && !defined(_WIN32_WCE)

    CProThreadMutexGuard mon(m_lockUpcall);

    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserverEx* observer  = NULL;
    const int                errorCode = -1;
    const int                sslCode   = 0;
    PRO_INT64                fd        = -1;
    PRO_SERVICE_PACKET       s2cPacket;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        struct iovec iov;
        iov.iov_base = &s2cPacket;
        iov.iov_len  = sizeof(PRO_SERVICE_PACKET);

        PRO_CMSG_CTRL ctrl;

        pbsd_msghdr msg;
        memset(&msg, 0, sizeof(pbsd_msghdr));
        msg.msg_iov        = &iov;
        msg.msg_iovlen     = 1;
        msg.msg_control    = ctrl.control;
        msg.msg_controllen = sizeof(ctrl.control);

        if (pbsd_recvmsg(m_sockId, &msg, 0) == sizeof(PRO_SERVICE_PACKET) &&
            s2cPacket.CheckMagic())
        {
            const struct cmsghdr* const cmsg = CMSG_FIRSTHDR(&msg);
            if (cmsg == NULL                              ||
                cmsg->cmsg_len   != CMSG_LEN(sizeof(int)) ||
                cmsg->cmsg_level != SOL_SOCKET            ||
                cmsg->cmsg_type  != SCM_RIGHTS)
            {
                return;
            }

            fd = *(int*)CMSG_DATA(cmsg);
            if (fd >= 0)
            {
                pbsd_ioctl_nonblock(fd);
                pbsd_ioctl_closexec(fd);
            }
            else
            {
                fd = -1;
            }
        }

        m_observer->AddRef();
        observer = (IProTransportObserverEx*)m_observer;
    }

    if (m_canUpcall)
    {
        if (fd != -1)
        {
            observer->OnRecvFd(
                this, fd, s2cPacket.s2c.oldSock.unixSocket, s2cPacket);
            fd = -1;
        }
        else
        {
            m_canUpcall = false;
            observer->OnClose(this, errorCode, sslCode);
        }
    }

    observer->Release();
    ProCloseSockId(fd);

    if (!m_canUpcall)
    {
        Fini();
    }

#endif /* WIN32, _WIN32_WCE */
}}

void
PRO_CALLTYPE
CProTcpTransport::OnOutput(PRO_INT64 sockId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer      = NULL;
    int                    sentSize      = 0;
    int                    errorCode     = 0;
    const int              sslCode       = 0;
    const CProBuffer*      onSendBuf     = NULL;
    bool                   requestOnSend = false;
    PRO_UINT64             actionId      = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        unsigned long     theSize = 0;
        const void* const theBuf  = m_sendPool.PreSend(theSize);

        if (theBuf == NULL || theSize == 0)
        {
            m_pendingWr = false;

            if (!m_requestOnSend)
            {
                if (m_onWr)
                {
                    m_reactorTask->RemoveHandler(
                        m_sockId, this, PRO_MASK_WRITE);
                    m_onWr = false;
                }

                return;
            }
        }
        else if (m_sendingFd == -1)
        {
            sentSize = pbsd_send(m_sockId, theBuf, theSize, 0);
            assert(sentSize <= (int)theSize);

            if (sentSize > (int)theSize)
            {
                sentSize  = -1;
                errorCode = -1;
            }
            else if (sentSize > 0)
            {
                m_sendPool.Flush(sentSize);

                onSendBuf = m_sendPool.OnSendBuf();
                if (onSendBuf != NULL)
                {
                    m_sendPool.PostSend();
                    m_pendingWr = false;
                }
            }
            else if (sentSize == 0)
            {
                sentSize  = -1;
                errorCode = PBSD_EWOULDBLOCK;
            }
            else
            {
                errorCode = pbsd_errno((void*)&pbsd_send);
            }
        }
        else
        {
#if !defined(WIN32) && !defined(_WIN32_WCE)
            struct iovec iov;
            iov.iov_base = (void*)theBuf;
            iov.iov_len  = theSize;

            PRO_CMSG_CTRL ctrl;

            pbsd_msghdr msg;
            memset(&msg, 0, sizeof(pbsd_msghdr));
            msg.msg_iov        = &iov;
            msg.msg_iovlen     = 1;
            msg.msg_control    = ctrl.control;
            msg.msg_controllen = sizeof(ctrl.control);

            struct cmsghdr* const cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_len         = CMSG_LEN(sizeof(int));
            cmsg->cmsg_level       = SOL_SOCKET;
            cmsg->cmsg_type        = SCM_RIGHTS;
            *(int*)CMSG_DATA(cmsg) = (int)m_sendingFd;

            sentSize = pbsd_sendmsg(m_sockId, &msg, 0);
            if (sentSize != (int)theSize)
            {
                sentSize  = -1;
                errorCode = -1;
            }

            m_sendingFd = -1;
#endif
        }

        requestOnSend = m_requestOnSend;
        m_requestOnSend = false;
        if (onSendBuf != NULL)
        {
            actionId = onSendBuf->GetMagic();
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (sentSize < 0 && errorCode != PBSD_EWOULDBLOCK)
        {
            m_canUpcall = false;
            observer->OnClose(this, errorCode, sslCode);
        }
        else if (onSendBuf != NULL || requestOnSend)
        {
            observer->OnSend(this, actionId);

            {
                CProThreadMutexGuard mon(m_lock);

                if (m_observer != NULL && m_reactorTask != NULL)
                {
                    if (m_onWr && !m_pendingWr && !m_requestOnSend)
                    {
                        m_reactorTask->RemoveHandler(
                            m_sockId, this, PRO_MASK_WRITE);
                        m_onWr = false;
                    }
                }
            }
        }
        else
        {
        }
    }

    observer->Release();

    if (!m_canUpcall)
    {
        Fini();
    }
}}

void
PRO_CALLTYPE
CProTcpTransport::OnError(PRO_INT64 sockId,
                          long      errorCode)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer = NULL;
    const int              sslCode  = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnClose(this, errorCode, sslCode);
    }

    observer->Release();

    Fini();
}}

void
PRO_CALLTYPE
CProTcpTransport::OnTimer(void*      factory,
                          PRO_UINT64 timerId,
                          PRO_INT64  userData)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IProTransportObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        observer->OnHeartbeat(this);
    }

    observer->Release();
}}
