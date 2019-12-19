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

#include "pro_service_host.h"
#include "pro_service_hub.h"
#include "pro_service_pipe.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define HEARTBEAT_INTERVAL 1
#define RECONNECT_INTERVAL 5
#define PIPE_TIMEOUT       10

/////////////////////////////////////////////////////////////////////////////
////

CProServiceHost*
CProServiceHost::CreateInstance()
{
    CProServiceHost* const host = new CProServiceHost;

    return (host);
}

CProServiceHost::CProServiceHost()
{
    m_observer    = NULL;
    m_reactor     = NULL;
    m_connector   = NULL;
    m_pipe        = NULL;
    m_timerId     = 0;

    m_servicePort = 0;
    m_serviceId   = 0;
    m_connectTick = 0;
}

CProServiceHost::~CProServiceHost()
{
    Fini();
}

bool
CProServiceHost::Init(IProServiceHostObserver* observer,
                      IProReactor*             reactor,
                      unsigned short           servicePort,
                      unsigned char            serviceId)
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(servicePort > 0);
    assert(serviceId > 0);
    if (observer == NULL || reactor == NULL || servicePort == 0 ||
        serviceId == 0)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_connector == NULL);
        assert(m_pipe == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_connector != NULL ||
            m_pipe != NULL)
        {
            return (false);
        }

        m_connector = ProCreateConnectorEx(
            true, /* enable unixSocket */
            0,    /* serviceId.  [0 for ipc-pipe, !0 for media-link] */
            0,    /* serviceOpt. [0 for tcp     , !0 for ssl] */
            this,
            reactor,
            "127.0.0.1",
            servicePort,
            "0.0.0.0",
            PIPE_TIMEOUT
            );
        if (m_connector == NULL)
        {
            return (false);
        }

        observer->AddRef();
        m_observer    = observer;
        m_reactor     = reactor;
        m_timerId     = reactor->ScheduleTimer(this, HEARTBEAT_INTERVAL * 1000, true);
        m_servicePort = servicePort;
        m_serviceId   = serviceId;
        m_connectTick = ProGetTickCount64();
    }

    return (true);
}

void
CProServiceHost::Fini()
{
    IProServiceHostObserver* observer  = NULL;
    IProConnector*           connector = NULL;
    CProServicePipe*         pipe      = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        pipe = m_pipe;
        m_pipe = NULL;
        connector = m_connector;
        m_connector = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteServicePipe(pipe);
    ProDeleteConnector(connector);
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CProServiceHost::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProServiceHost::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CProServiceHost::OnConnectOk(IProConnector*   connector,
                             PRO_INT64        sockId,
                             bool             unixSocket,
                             const char*      remoteIp,
                             unsigned short   remotePort,
                             unsigned char    serviceId,
                             unsigned char    serviceOpt,
                             const PRO_NONCE* nonce)
{
    assert(connector != NULL);
    assert(sockId != -1);
    if (connector == NULL || sockId == -1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_connector == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (connector != m_connector)
        {
            ProCloseSockId(sockId);

            return;
        }

        assert(m_pipe == NULL);

#if defined(WIN32) || defined(_WIN32_WCE)
        const bool recvFdMode = false;
#else
        const bool recvFdMode = true;
#endif

        m_pipe = ProCreateServicePipe(
            recvFdMode, this, m_reactor, sockId, unixSocket);
        if (m_pipe == NULL)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
        else
        {
            /*
             * todo...
             */
            PRO_SERVICE_PACKET c2sPacket;
            c2sPacket.c2s.serviceId = m_serviceId;
            c2sPacket.c2s.processId = ProGetProcessId();
            m_pipe->SendData(c2sPacket);
        }

        m_connector = NULL;
    }

    ProDeleteConnector(connector);
}

void
PRO_CALLTYPE
CProServiceHost::OnConnectError(IProConnector* connector,
                                const char*    remoteIp,
                                unsigned short remotePort,
                                unsigned char  serviceId,
                                unsigned char  serviceOpt,
                                bool           timeout)
{
    assert(connector != NULL);
    if (connector == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_connector == NULL)
        {
            return;
        }

        if (connector != m_connector)
        {
            return;
        }

        m_connector = NULL;
    }

    ProDeleteConnector(connector);
}

void
PRO_CALLTYPE
CProServiceHost::OnRecv(CProServicePipe*          pipe,
                        const PRO_SERVICE_PACKET& packet)
{
    assert(pipe != NULL);
    assert(packet.CheckMagic());
    if (pipe == NULL || !packet.CheckMagic())
    {
        return;
    }

    PRO_SERVICE_PACKET s2cPacket = packet;

    IProServiceHostObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_pipe == NULL)
        {
            return;
        }

        if (pipe != m_pipe || s2cPacket.s2c.serviceId != m_serviceId)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    PRO_INT64        sockId = -1;
    pbsd_sockaddr_in remoteAddr;

#if defined(_WIN32_WCE)
    sockId = s2cPacket.s2c.oldSock.sockId;
    if (sockId != -1)
    {
        if (pbsd_getpeername(sockId, &remoteAddr) != 0)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
    }
#elif defined(WIN32)
    if (sizeof(SOCKET) == 8)
    {
        sockId = (PRO_INT64)::WSASocket(
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            &s2cPacket.s2c.protocolInfo,
            0,
            WSA_FLAG_OVERLAPPED
            );
    }
    else
    {
        sockId = (PRO_INT32)::WSASocket(
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            FROM_PROTOCOL_INFO,
            &s2cPacket.s2c.protocolInfo,
            0,
            WSA_FLAG_OVERLAPPED
            );
    }
    if (sockId < 0)
    {
        sockId = -1;
    }
    else
    {
        if (pbsd_getpeername(sockId, &remoteAddr) != 0)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
    }
#else
    sockId = s2cPacket.s2c.oldSock.sockId;
    if (sockId != -1 && !s2cPacket.s2c.oldSock.unixSocket)
    {
        if (pbsd_getpeername(sockId, &remoteAddr) != 0)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
    }
#endif

    if (sockId == -1)
    {
        observer->Release();

        return;
    }

    /*
     * 1. notify the hub
     */
    PRO_SERVICE_PACKET c2sPacket;
    c2sPacket.c2s.serviceId = s2cPacket.s2c.serviceId;
    c2sPacket.c2s.processId = ProGetProcessId();
    c2sPacket.c2s.oldSock   = s2cPacket.s2c.oldSock;
    pipe->SendData(c2sPacket);

    /*
     * 2. make a callback
     */
    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    if (!s2cPacket.s2c.oldSock.unixSocket)
    {
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    observer->OnServiceAccept(
        (IProServiceHost*)this,
        sockId,
        s2cPacket.s2c.oldSock.unixSocket,
        remoteIp,
        remotePort,
        s2cPacket.s2c.serviceId,
        s2cPacket.s2c.serviceOpt,
        &s2cPacket.s2c.nonce
        );
    observer->Release();
}

void
PRO_CALLTYPE
CProServiceHost::OnRecvFd(CProServicePipe*          pipe,
                          PRO_INT64                 fd,
                          bool                      unixSocket,
                          const PRO_SERVICE_PACKET& s2cPacket)
{
    assert(pipe != NULL);
    assert(fd != -1);
    assert(s2cPacket.CheckMagic());
    if (pipe == NULL || fd == -1 || !s2cPacket.CheckMagic())
    {
        return;
    }

    IProServiceHostObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_pipe == NULL)
        {
            ProCloseSockId(fd);

            return;
        }

        if (pipe != m_pipe || s2cPacket.s2c.serviceId != m_serviceId)
        {
            ProCloseSockId(fd);

            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    pbsd_sockaddr_in remoteAddr;

    if (!unixSocket)
    {
        if (pbsd_getpeername(fd, &remoteAddr) != 0)
        {
            ProCloseSockId(fd);
            fd = -1;
        }
    }

    if (fd == -1)
    {
        observer->Release();

        return;
    }

    /*
     * 1. notify the hub
     */
    PRO_SERVICE_PACKET c2sPacket;
    c2sPacket.c2s.serviceId = s2cPacket.s2c.serviceId;
    c2sPacket.c2s.processId = ProGetProcessId();
    c2sPacket.c2s.oldSock   = s2cPacket.s2c.oldSock;
    pipe->SendData(c2sPacket);

    /*
     * 2. make a callback
     */
    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    if (!unixSocket)
    {
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    observer->OnServiceAccept(
        (IProServiceHost*)this,
        fd,
        unixSocket,
        remoteIp,
        remotePort,
        s2cPacket.s2c.serviceId,
        s2cPacket.s2c.serviceOpt,
        &s2cPacket.s2c.nonce
        );
    observer->Release();
}

void
PRO_CALLTYPE
CProServiceHost::OnClose(CProServicePipe* pipe)
{
    assert(pipe != NULL);
    if (pipe == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_pipe == NULL)
        {
            return;
        }

        if (pipe != m_pipe)
        {
            return;
        }

        m_pipe = NULL;
    }

    ProDeleteServicePipe(pipe);
}

void
PRO_CALLTYPE
CProServiceHost::OnTimer(void*      factory,
                         PRO_UINT64 timerId,
                         PRO_INT64  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        const PRO_INT64 tick = ProGetTickCount64();

        if (m_pipe == NULL && m_connector == NULL &&
            tick - m_connectTick >= RECONNECT_INTERVAL * 1000)
        {
            m_connectTick = tick;

            m_connector = ProCreateConnectorEx(
                true, /* enable unixSocket */
                0,    /* serviceId.  [0 for ipc-pipe, !0 for media-link] */
                0,    /* serviceOpt. [0 for tcp     , !0 for ssl] */
                this,
                m_reactor,
                "127.0.0.1",
                m_servicePort,
                "0.0.0.0",
                PIPE_TIMEOUT
                );
        }
        else if (m_pipe != NULL)
        {
            /*
             * todo...
             */
            PRO_SERVICE_PACKET c2sPacket;
            c2sPacket.c2s.serviceId = m_serviceId;
            c2sPacket.c2s.processId = ProGetProcessId();
            m_pipe->SendData(c2sPacket);
        }
        else
        {
        }
    }
}
