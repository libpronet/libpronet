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

#include "pro_udp_transport.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_RECV_BUF_SIZE  (1024 * 56)
#define DEFAULT_SEND_BUF_SIZE  (1024 * 56)
#define DEFAULT_RECV_POOL_SIZE (1024 * 65) /* EMSGSIZE */

/////////////////////////////////////////////////////////////////////////////
////

CProUdpTransport*
CProUdpTransport::CreateInstance(size_t recvPoolSize)   /* = 0 */
{
    CProUdpTransport* const trans = new CProUdpTransport(recvPoolSize);

    return (trans);
}

CProUdpTransport::CProUdpTransport(size_t recvPoolSize) /* = 0 */
: m_recvPoolSize(recvPoolSize > 0 ? recvPoolSize : DEFAULT_RECV_POOL_SIZE)
{
    m_observer      = NULL;
    m_reactorTask   = NULL;
    m_sockId        = -1;
    m_timerId       = 0;
    m_onWr          = false;
    m_pendingWr     = false;
    m_requestOnSend = false;
    m_actionId      = 0;

    m_canUpcall     = true;

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
        return (false);
    }

    const char* const anyIp = "0.0.0.0";
    if (localIp == NULL || localIp[0] == '\0')
    {
        localIp         = anyIp;
    }
    if (defaultRemoteIp == NULL || defaultRemoteIp[0] == '\0')
    {
        defaultRemoteIp = anyIp;
    }

    if (sockBufSizeRecv == 0)
    {
        sockBufSizeRecv = DEFAULT_RECV_BUF_SIZE;
    }
    if (sockBufSizeSend == 0)
    {
        sockBufSizeSend = DEFAULT_SEND_BUF_SIZE;
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
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(defaultRemoteIp);

    if (localAddr.sin_addr.s_addr  == (PRO_UINT32)-1 ||
        remoteAddr.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return (false);
        }

        if (!m_recvPool.Resize(m_recvPoolSize))
        {
            return (false);
        }

        const PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
        if (sockId == -1)
        {
            return (false);
        }

        int option;
        option = (int)sockBufSizeRecv;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)sockBufSizeSend;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

        if (pbsd_bind(sockId, &localAddr, false) != 0)
        {
            ProCloseSockId(sockId);

            return (false);
        }

        if (pbsd_getsockname(sockId, &localAddr) != 0)
        {
            ProCloseSockId(sockId);

            return (false);
        }

        if (!reactorTask->AddHandler(sockId, this, PRO_MASK_READ))
        {
            ProCloseSockId(sockId);

            return (false);
        }

        observer->AddRef();
        m_observer          = observer;
        m_reactorTask       = reactorTask;
        m_sockId            = sockId;
        m_localAddr         = localAddr;
        m_defaultRemoteAddr = remoteAddr;
    }

    return (true);
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

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->Release();
}

unsigned long
PRO_CALLTYPE
CProUdpTransport::AddRef()
{
    const unsigned long refCount = CProEventHandler::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProUdpTransport::Release()
{
    const unsigned long refCount = CProEventHandler::Release();

    return (refCount);
}

PRO_TRANS_TYPE
PRO_CALLTYPE
CProUdpTransport::GetType() const
{
    return (PRO_TRANS_UDP);
}

PRO_SSL_SUITE_ID
PRO_CALLTYPE
CProUdpTransport::GetSslSuite(char suiteName[64]) const
{
    strcpy(suiteName, "NONE");

    return (PRO_SSL_SUITE_NONE);
}

PRO_INT64
PRO_CALLTYPE
CProUdpTransport::GetSockId() const
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
CProUdpTransport::GetLocalIp(char localIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_localAddr.sin_addr.s_addr, localIp);
    }

    return (localIp);
}

unsigned short
PRO_CALLTYPE
CProUdpTransport::GetLocalPort() const
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
CProUdpTransport::GetRemoteIp(char remoteIp[64]) const
{
    {
        CProThreadMutexGuard mon(m_lock);

        pbsd_inet_ntoa(m_defaultRemoteAddr.sin_addr.s_addr, remoteIp);
    }

    return (remoteIp);
}

unsigned short
PRO_CALLTYPE
CProUdpTransport::GetRemotePort() const
{
    unsigned short remotePort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        remotePort = pbsd_ntoh16(m_defaultRemoteAddr.sin_port);
    }

    return (remotePort);
}

IProRecvPool*
PRO_CALLTYPE
CProUdpTransport::GetRecvPool()
{
    return (&m_recvPool);
}

bool
PRO_CALLTYPE
CProUdpTransport::SendData(const void*             buf,
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

        const pbsd_sockaddr_in* const realAddr =
            remoteAddr != NULL ? remoteAddr : &m_defaultRemoteAddr;
        if (realAddr->sin_addr.s_addr == 0 || realAddr->sin_port == 0)
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

        pbsd_sendto(m_sockId, buf, (int)size, 0, realAddr);
        m_pendingWr = true;
        m_actionId  = actionId;
    }

    return (true);
}

void
PRO_CALLTYPE
CProUdpTransport::RequestOnSend()
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
CProUdpTransport::SuspendRecv()
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
CProUdpTransport::ResumeRecv()
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
CProUdpTransport::StartHeartbeat()
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
CProUdpTransport::StopHeartbeat()
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
CProUdpTransport::OnInput(PRO_INT64 sockId)
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

        const size_t idleSize = m_recvPool.ContinuousIdleSize();

        assert(idleSize > 0);
        if (idleSize == 0)
        {
            recvSize  = -1;
            errorCode = -1;

            goto EXIT;
        }

        recvSize = pbsd_recvfrom(
            m_sockId, m_recvPool.ContinuousIdleBuf(), (int)idleSize, 0, &remoteAddr);
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
        else if (
            recvSize < 0 && errorCode != PBSD_EWOULDBLOCK &&
            errorCode != PBSD_ECONNRESET && errorCode != PBSD_EMSGSIZE
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
PRO_CALLTYPE
CProUdpTransport::OnOutput(PRO_INT64 sockId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer = NULL;
    PRO_UINT64             actionId = 0;

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

        if (m_pendingWr || m_requestOnSend)
        {
            actionId = m_actionId;

            m_pendingWr     = false;
            m_requestOnSend = false;
            m_actionId      = 0;

            m_observer->AddRef();
            observer = m_observer;
        }
    }

    if (observer != NULL)
    {
        if (m_canUpcall)
        {
            observer->OnSend(this, actionId);

            {
                CProThreadMutexGuard mon(m_lock);

                if (m_observer != NULL && m_reactorTask != NULL)
                {
                    if (m_onWr && !m_pendingWr && !m_requestOnSend)
                    {
                        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
                        m_onWr = false;
                    }
                }
            }
        }

        observer->Release();
    }
}}

void
PRO_CALLTYPE
CProUdpTransport::OnError(PRO_INT64 sockId,
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
CProUdpTransport::OnTimer(unsigned long timerId,
                          PRO_INT64     userData)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(timerId > 0);
    if (timerId == 0)
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
