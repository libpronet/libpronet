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

#include "pro_connector.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_tp_reactor_task.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define SERVICE_HANDSHAKE_BYTES 4 /* serviceId + serviceOpt + (r) + (r+1) */
#define DEFAULT_TIMEOUT         20

/////////////////////////////////////////////////////////////////////////////
////

CProConnector*
CProConnector::CreateInstance(bool          enableUnixSocket,
                              bool          enableServiceExt,
                              unsigned char serviceId,
                              unsigned char serviceOpt)
{
#if defined(_WIN32)
    enableUnixSocket = false;
#endif

    return new CProConnector(enableUnixSocket, enableServiceExt, serviceId, serviceOpt);
}

CProConnector::CProConnector(bool          enableUnixSocket,
                             bool          enableServiceExt,
                             unsigned char serviceId,
                             unsigned char serviceOpt)
:
m_enableUnixSocket(enableUnixSocket),
m_enableServiceExt(enableServiceExt),
m_serviceId (enableServiceExt ? serviceId  : 0),
m_serviceOpt(enableServiceExt ? serviceOpt : 0)
{
    m_observer         = NULL;
    m_reactorTask      = NULL;
    m_handshaker       = NULL;
    m_sockId           = -1;
    m_unixSocket       = false;
    m_timeoutInSeconds = DEFAULT_TIMEOUT;
    m_timerId0         = 0;
    m_timerId1         = 0;

    memset(&m_localAddr , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_remoteAddr, 0, sizeof(pbsd_sockaddr_in));
}

CProConnector::~CProConnector()
{
    Fini();

    ProCloseSockId(m_sockId);
    m_sockId = -1;
}

bool
CProConnector::Init(IProConnectorObserver* observer,
                    CProTpReactorTask*     reactorTask,
                    const char*            remoteIp,
                    unsigned short         remotePort,
                    const char*            localBindIp,      /* = NULL */
                    unsigned long          timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    if (observer == NULL || reactorTask == NULL ||
        remoteIp == NULL || remoteIp[0] == '\0' || remotePort == 0)
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localBindIp);

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(remotePort);
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(remoteIp); /* DNS */

    if (localAddr.sin_addr.s_addr  == (uint32_t)-1 ||
        remoteAddr.sin_addr.s_addr == (uint32_t)-1 ||
        remoteAddr.sin_addr.s_addr == 0)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        assert(m_handshaker == NULL);
        if (m_observer != NULL || m_reactorTask != NULL ||
            m_handshaker != NULL)
        {
            return (false);
        }

        if (m_enableUnixSocket && remoteAddr.sin_addr.s_addr == pbsd_inet_aton("127.0.0.1"))
        {
            m_unixSocket = true;
        }
        else
        {
            m_unixSocket = false;
        }

        observer->AddRef();
        m_observer         = observer;
        m_reactorTask      = reactorTask;
        m_localAddr        = localAddr;
        m_remoteAddr       = remoteAddr;
        m_timeoutInSeconds = timeoutInSeconds;
        m_timerId0         = reactorTask->ScheduleTimer(this, 0, false, 0);
        m_timerId1         = reactorTask->ScheduleTimer(this, (uint64_t)timeoutInSeconds * 1000, false, 0);
    }

    return (true);
}

void
CProConnector::Fini()
{
    IProConnectorObserver* observer   = NULL;
    IProTcpHandshaker*     handshaker = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_CONNECT);

        handshaker = m_handshaker;
        m_handshaker = NULL;
        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteTcpHandshaker(handshaker);
    observer->Release();
}

unsigned long
CProConnector::AddRef()
{
    const unsigned long refCount = CProEventHandler::AddRef();

    return (refCount);
}

unsigned long
CProConnector::Release()
{
    const unsigned long refCount = CProEventHandler::Release();

    return (refCount);
}

void
CProConnector::OnInput(int64_t sockId)
{
    OnError(sockId, -1);
}

void
CProConnector::OnOutput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProConnectorObserver* observer = NULL;

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

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_CONNECT);

        if (m_enableServiceExt)
        {
            unsigned char serviceData[SERVICE_HANDSHAKE_BYTES];
            serviceData[0] = m_serviceId;                              /* serviceId */
            serviceData[1] = m_serviceOpt;                             /* serviceOpt */
            serviceData[2] = (unsigned char)(ProRand_0_32767() % 256); /* r */
            serviceData[3] = (unsigned char)(serviceData[2] + 1);      /* r + 1 */

            assert(m_handshaker == NULL);

            m_handshaker = ProCreateTcpHandshaker(
                this,
                m_reactorTask,
                m_sockId,
                m_unixSocket,
                serviceData,             /* sendData */
                SERVICE_HANDSHAKE_BYTES, /* sendDataSize */
                sizeof(PRO_NONCE),       /* recvDataSize */
                false,                   /* recvFirst is false */
                m_timeoutInSeconds
                );
            if (m_handshaker == NULL)
            {
                sockId = -1;

                goto EXIT;
            }

            m_sockId = -1; /* cut, change its owner */

            return;
        }

        m_sockId = -1;     /* cut */

EXIT:

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    char remoteIp[64] = "";
    pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);

    if (m_enableServiceExt)
    {
        assert(sockId == -1);

        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            m_serviceId,
            m_serviceOpt,
            false /* isn't a timeout */
            );
    }
    else
    {
        if (sockId != -1)
        {
            observer->OnConnectOk(
                (IProConnector*)this,
                sockId,
                m_unixSocket,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port)
                );
        }
        else
        {
            observer->OnConnectError(
                (IProConnector*)this,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port),
                false /* isn't a timeout */
                );
        }
    }

    observer->Release();
}

void
CProConnector::OnException(int64_t sockId)
{
    OnError(sockId, -1);
}

void
CProConnector::OnError(int64_t sockId,
                       long    errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProConnectorObserver* observer = NULL;

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

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_CONNECT);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    char remoteIp[64] = "";
    pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);

    if (m_enableServiceExt)
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            m_serviceId,
            m_serviceOpt,
            false /* isn't a timeout */
            );
    }
    else
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            false /* isn't a timeout */
            );
    }

    observer->Release();
}

void
CProConnector::OnHandshakeOk(IProTcpHandshaker* handshaker,
                             int64_t            sockId,
                             bool               unixSocket,
                             const void*        buf,
                             unsigned long      size)
{
    assert(handshaker != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || sockId == -1)
    {
        return;
    }

    IProConnectorObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_handshaker == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (handshaker != m_handshaker)
        {
            ProCloseSockId(sockId);

            return;
        }

        assert(buf != NULL);
        assert(size == sizeof(PRO_NONCE));
        if (buf == NULL || size != sizeof(PRO_NONCE))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        m_handshaker = NULL;
        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    char remoteIp[64] = "";
    pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);

    if (m_enableServiceExt)
    {
        if (sockId != -1)
        {
            PRO_NONCE nonce;
            memcpy(&nonce, buf, size);

            observer->OnConnectOk(
                (IProConnector*)this,
                sockId,
                unixSocket,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port),
                m_serviceId,
                m_serviceOpt,
                &nonce
                );
        }
        else
        {
            observer->OnConnectError(
                (IProConnector*)this,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port),
                m_serviceId,
                m_serviceOpt,
                false /* isn't a timeout */
                );
        }
    }
    else
    {
        if (sockId != -1)
        {
            observer->OnConnectOk(
                (IProConnector*)this,
                sockId,
                unixSocket,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port)
                );
        }
        else
        {
            observer->OnConnectError(
                (IProConnector*)this,
                remoteIp,
                pbsd_ntoh16(m_remoteAddr.sin_port),
                false /* isn't a timeout */
                );
        }
    }

    observer->Release();
    ProDeleteTcpHandshaker(handshaker);
}

void
CProConnector::OnHandshakeError(IProTcpHandshaker* handshaker,
                                long               errorCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    IProConnectorObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_handshaker == NULL)
        {
            return;
        }

        if (handshaker != m_handshaker)
        {
            return;
        }

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        m_handshaker = NULL;
        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    char remoteIp[64] = "";
    pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);

    if (m_enableServiceExt)
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            m_serviceId,
            m_serviceOpt,
            errorCode == PBSD_ETIMEDOUT
            );
    }
    else
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            errorCode == PBSD_ETIMEDOUT
            );
    }

    observer->Release();
    ProDeleteTcpHandshaker(handshaker);
}

void
CProConnector::OnTimer(void*    factory,
                       uint64_t timerId,
                       int64_t  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IProConnectorObserver* observer   = NULL;
    IProTcpHandshaker*     handshaker = NULL;
    bool                   error      = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (timerId != m_timerId0 && timerId != m_timerId1)
        {
            return;
        }

        do
        {
            if (timerId != m_timerId0)
            {
                break;
            }

            assert(m_sockId == -1);

            if (m_unixSocket)
            {
                m_sockId = pbsd_socket(AF_LOCAL, SOCK_STREAM, 0);
            }
            else
            {
                m_sockId = pbsd_socket(AF_INET , SOCK_STREAM, 0);
            }
            if (m_sockId == -1)
            {
                error = true;
                break;
            }

#if !defined(_WIN32)
            if (m_unixSocket)
            {
                pbsd_sockaddr_un remoteAddrUn;
                memset(&remoteAddrUn, 0, sizeof(pbsd_sockaddr_un));
                remoteAddrUn.sun_family = AF_LOCAL;
                sprintf(remoteAddrUn.sun_path, "/tmp/libpronet_127001_%u",
                    (unsigned int)pbsd_ntoh16(m_remoteAddr.sin_port));

                if (pbsd_connect_un(m_sockId, &remoteAddrUn) != 0 &&
                    pbsd_errno((void*)&pbsd_connect_un) != PBSD_EINPROGRESS)
                {
                    error = true;
                    break;
                }
            }
            else
#endif
            {
                const int option = 1;
                pbsd_setsockopt(m_sockId, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));

                if (pbsd_bind(m_sockId, &m_localAddr, false) != 0)
                {
                    error = true;
                    break;
                }

                if (pbsd_getsockname(m_sockId, &m_localAddr) != 0)
                {
                    error = true;
                    break;
                }

                if (pbsd_connect(m_sockId, &m_remoteAddr) != 0 &&
                    pbsd_errno((void*)&pbsd_connect) != PBSD_EINPROGRESS)
                {
                    error = true;
                    break;
                }
            }

            if (!m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_CONNECT))
            {
                error = true;
                break;
            }

            return;
        }
        while (0);

        m_reactorTask->CancelTimer(m_timerId0);
        m_reactorTask->CancelTimer(m_timerId1);
        m_timerId0 = 0;
        m_timerId1 = 0;

        if (error)
        {
            ProCloseSockId(m_sockId);
            m_sockId = -1;
        }
        else
        {
            m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_CONNECT);
        }

        handshaker = m_handshaker;
        m_handshaker = NULL;
        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    char remoteIp[64] = "";
    pbsd_inet_ntoa(m_remoteAddr.sin_addr.s_addr, remoteIp);

    if (m_enableServiceExt)
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            m_serviceId,
            m_serviceOpt,
            !error /* is a timeout? */
            );
    }
    else
    {
        observer->OnConnectError(
            (IProConnector*)this,
            remoteIp,
            pbsd_ntoh16(m_remoteAddr.sin_port),
            !error /* is a timeout? */
            );
    }

    ProDeleteTcpHandshaker(handshaker);
    observer->Release();
}
