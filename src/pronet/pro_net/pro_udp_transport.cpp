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

#include "pro_udp_transport.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_RECV_POOL_SIZE (1024 * 65) /* EMSGSIZE */

/////////////////////////////////////////////////////////////////////////////
////

CProUdpTransport*
CProUdpTransport::CreateInstance(bool   bindToLocal,  /* = false */
                                 size_t recvPoolSize) /* = 0 */
{
    return new CProUdpTransport(bindToLocal, recvPoolSize);
}

CProUdpTransport::CProUdpTransport(bool   bindToLocal,  /* = false */
                                   size_t recvPoolSize) /* = 0 */
:
m_bindToLocal(bindToLocal),
m_recvPoolSize(recvPoolSize > 0 ? recvPoolSize : DEFAULT_RECV_POOL_SIZE)
{
    m_observer         = NULL;
    m_reactorTask      = NULL;
    m_sockId           = -1;
    m_timerId          = 0;

    m_connResetAsError = false;
    m_canUpcall        = true;

    memset(&m_localAddr        , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_defaultRemoteAddr, 0, sizeof(pbsd_sockaddr_in));
}

CProUdpTransport::~CProUdpTransport()
{
    Fini();

    ProCloseSockId(m_sockId);
    m_sockId = -1;
}

bool
CProUdpTransport::Init(IProTransportObserver* observer,
                       CProTpReactorTask*     reactorTask,
                       const char*            localIp,           /* = NULL */
                       unsigned short         localPort,         /* = 0 */
                       const char*            defaultRemoteIp,   /* = NULL */
                       unsigned short         defaultRemotePort, /* = 0 */
                       size_t                 sockBufSizeRecv,   /* = 0 */
                       size_t                 sockBufSizeSend)   /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    if (observer == NULL || reactorTask == NULL)
    {
        return false;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(defaultRemotePort);
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(defaultRemoteIp); /* DNS */

    if (localAddr.sin_addr.s_addr  == (uint32_t)-1 ||
        remoteAddr.sin_addr.s_addr == (uint32_t)-1)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return false;
        }

        if (!m_recvPool.Resize(m_recvPoolSize))
        {
            return false;
        }

        int64_t sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
        if (sockId == -1)
        {
            return false;
        }

        int option;
        option = (int)sockBufSizeRecv;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)sockBufSizeSend;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

        if (m_bindToLocal && pbsd_bind(sockId, &localAddr, false) != 0)
        {
            ProCloseSockId(sockId);

            return false;
        }

        if (m_bindToLocal && pbsd_getsockname(sockId, &localAddr) != 0)
        {
            ProCloseSockId(sockId);

            return false;
        }

        if (!reactorTask->AddHandler(sockId, this, PRO_MASK_READ))
        {
            ProCloseSockId(sockId);

            return false;
        }

        observer->AddRef();
        m_observer          = observer;
        m_reactorTask       = reactorTask;
        m_sockId            = sockId;
        m_localAddr         = localAddr;
        m_defaultRemoteAddr = remoteAddr;
    }

    return true;
}

void
CProUdpTransport::Fini()
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

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->Release();
}

unsigned long
CProUdpTransport::AddRef()
{
    return CProEventHandler::AddRef();
}

unsigned long
CProUdpTransport::Release()
{
    return CProEventHandler::Release();
}

PRO_SSL_SUITE_ID
CProUdpTransport::GetSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    return PRO_SSL_SUITE_NONE;
}

int64_t
CProUdpTransport::GetSockId() const
{
    int64_t sockId = -1;

    {
        CProThreadMutexGuard mon(m_lock);

        sockId = m_sockId;
    }

    return sockId;
}

const char*
CProUdpTransport::GetLocalIp(char localIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_localAddr.sin_addr.s_addr, localIp);
    }

    return localIp;
}

unsigned short
CProUdpTransport::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        localPort = pbsd_ntoh16(m_localAddr.sin_port);
    }

    return localPort;
}

const char*
CProUdpTransport::GetRemoteIp(char remoteIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_defaultRemoteAddr.sin_addr.s_addr, remoteIp);
    }

    return remoteIp;
}

unsigned short
CProUdpTransport::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        remotePort = pbsd_ntoh16(m_defaultRemoteAddr.sin_port);
    }

    return remotePort;
}

bool
CProUdpTransport::SendData(const void*             buf,
                           size_t                  size,
                           uint64_t                actionId,   /* ignored */
                           const pbsd_sockaddr_in* remoteAddr) /* = NULL */
{
    assert(buf != NULL);
    assert(size > 0);
    if (buf == NULL || size == 0)
    {
        return false;
    }

    const pbsd_sockaddr_in* realAddr = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return false;
        }

        realAddr = remoteAddr != NULL ? remoteAddr : &m_defaultRemoteAddr;
        if (realAddr->sin_addr.s_addr == 0 || realAddr->sin_port == 0)
        {
            return false;
        }
    }

    int sentSize = pbsd_sendto(m_sockId, buf, size, 0, realAddr);

    return sentSize == (int)size;
}

void
CProUdpTransport::SuspendRecv()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactorTask == NULL)
    {
        return;
    }

    m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);
}

void
CProUdpTransport::ResumeRecv()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactorTask == NULL)
    {
        return;
    }

    m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_READ);
}

void
CProUdpTransport::StartHeartbeat()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactorTask == NULL)
    {
        return;
    }

    if (m_timerId == 0)
    {
        m_timerId = m_reactorTask->SetupHeartbeatTimer(this, 0);
    }
}

void
CProUdpTransport::StopHeartbeat()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactorTask == NULL)
    {
        return;
    }

    m_reactorTask->CancelTimer(m_timerId);
    m_timerId = 0;
}

void
CProUdpTransport::UdpConnResetAsError(const pbsd_sockaddr_in* remoteAddr) /* = NULL */
{
    CProThreadMutexGuard mon(m_lock);

    if (m_observer == NULL || m_reactorTask == NULL)
    {
        return;
    }

    m_connResetAsError = true;

#if defined(_WIN32)
    long          arg           = 1;
    unsigned long bytesReturned = 0;
    ::WSAIoctl((SOCKET)m_sockId, (unsigned long)SIO_UDP_CONNRESET,
        &arg, sizeof(long), NULL, 0, &bytesReturned, NULL, NULL);
#endif

#if !defined(PRO_LACKS_UDP_CONNECT)
    if (remoteAddr != NULL)
    {
        pbsd_connect(m_sockId, remoteAddr);

        pbsd_sockaddr_in localAddr;
        if (pbsd_getsockname(m_sockId, &localAddr) == 0)
        {
            m_localAddr = localAddr;
        }
    }
#endif
}

void
CProUdpTransport::OnInput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer  = NULL;
    int                    recvSize  = 0;
    int                    errorCode = 0;
    pbsd_sockaddr_in       remoteAddr;

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

        size_t idleSize = m_recvPool.ContinuousIdleSize();

        assert(idleSize > 0);
        if (idleSize == 0)
        {
            recvSize  = -1;
            errorCode = -1;

            goto EXIT;
        }

        recvSize = pbsd_recvfrom(
            m_sockId, m_recvPool.ContinuousIdleBuf(), idleSize, 0, &remoteAddr);
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
            errorCode = pbsd_errno((void*)&pbsd_recvfrom);
        }

EXIT:

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (recvSize > 0)
        {
            observer->OnRecv(this, &remoteAddr);
            assert(m_recvPool.ContinuousIdleSize() > 0);
        }
        else if (recvSize < 0 && errorCode == PBSD_ECONNRESET && m_connResetAsError)
        {
            m_canUpcall = false;
            observer->OnClose(this, PBSD_ECONNRESET, 0);
        }
        else if (
            recvSize < 0 && errorCode != PBSD_EWOULDBLOCK &&
            errorCode != PBSD_ECONNRESET && errorCode != PBSD_EMSGSIZE
            )
        {
            m_canUpcall = false;
            observer->OnClose(this, errorCode, 0);
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
}

void
CProUdpTransport::OnError(int64_t sockId,
                          int     errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
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
        observer->OnClose(this, errorCode, 0);
    }

    observer->Release();

    Fini();
}

void
CProUdpTransport::OnTimer(void*    factory,
                          uint64_t timerId,
                          int64_t  userData)
{
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
}
