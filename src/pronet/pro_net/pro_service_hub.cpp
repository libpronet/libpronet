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

#include "pro_service_hub.h"
#include "pro_acceptor.h"
#include "pro_service_pipe.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define HEARTBEAT_INTERVAL 1
#define SOCK_TIMEOUT       5
#define PIPE_TIMEOUT       10

/////////////////////////////////////////////////////////////////////////////
////

CProServiceHub*
CProServiceHub::CreateInstance(bool enableServiceExt,
                               bool enableLoadBalance)
{
    return new CProServiceHub(enableServiceExt, enableLoadBalance);
}

CProServiceHub::CProServiceHub(bool enableServiceExt,
                               bool enableLoadBalance)
:
m_enableServiceExt(enableServiceExt),
m_enableLoadBalance(enableLoadBalance)
{
    m_observer = NULL;
    m_reactor  = NULL;
    m_acceptor = NULL;
    m_timerId  = 0;
}

CProServiceHub::~CProServiceHub()
{
    Fini();
}

bool
CProServiceHub::Init(IProServiceHubObserver* observer,
                     IProReactor*            reactor,
                     unsigned short          servicePort,
                     unsigned int            timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(servicePort > 0);
    if (observer == NULL || reactor == NULL || servicePort == 0)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_acceptor == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_acceptor != NULL)
        {
            return false;
        }

        if (m_enableServiceExt)
        {
            m_acceptor = ProCreateAcceptorEx(
                this, reactor, "0.0.0.0", servicePort, timeoutInSeconds);
        }
        else
        {
            CProAcceptor* acceptor = CProAcceptor::CreateInstanceOnlyLoopExt();
            if (acceptor != NULL)
            {
                if (!acceptor->Init(this, (CProTpReactorTask*)reactor,
                    "0.0.0.0", servicePort, timeoutInSeconds))
                {
                    acceptor->Release();
                    acceptor = NULL;
                }
            }

            m_acceptor = (IProAcceptor*)acceptor;
        }

        if (m_acceptor == NULL)
        {
            return false;
        }

        observer->AddRef();
        m_observer = observer;
        m_reactor  = reactor;
        m_timerId  = reactor->SetupTimer(this, HEARTBEAT_INTERVAL * 1000, HEARTBEAT_INTERVAL * 1000);
    }

    return true;
}

void
CProServiceHub::Fini()
{
    IProServiceHubObserver*                     observer = NULL;
    IProAcceptor*                               acceptor = NULL;
    CProStlMap<PRO_SERVICE_PIPE, unsigned char> pipe2ServiceId;
    CProStlSet<PRO_SERVICE_SOCK>                expireSocks;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        for (int i = 0; i < 256; ++i)
        {
            m_readyPipe2Socks[i].clear();
        }

        expireSocks = m_expireSocks;
        m_expireSocks.clear();
        pipe2ServiceId = m_pipe2ServiceId;
        m_pipe2ServiceId.clear();
        acceptor = m_acceptor;
        m_acceptor = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    {
        auto itr = expireSocks.begin();
        auto end = expireSocks.end();

        for (; itr != end; ++itr)
        {
            ProCloseSockId(itr->sockId);
        }
    }

    {
        auto itr = pipe2ServiceId.begin();
        auto end = pipe2ServiceId.end();

        for (; itr != end; ++itr)
        {
            ProDeleteServicePipe(itr->first.pipe);
        }
    }

    ProDeleteAcceptor(acceptor);
    observer->Release();
}

unsigned long
CProServiceHub::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CProServiceHub::Release()
{
    return CProRefCount::Release();
}

void
CProServiceHub::OnAccept(IProAcceptor*  acceptor,
                         int64_t        sockId,
                         bool           unixSocket,
                         const char*    localIp,
                         const char*    remoteIp,
                         unsigned short remotePort)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(localIp != NULL);
    if (acceptor == NULL || sockId == -1 || localIp == NULL)
    {
        return;
    }

    PRO_NONCE nonce;
    memset(&nonce, 0, sizeof(PRO_NONCE));

    AcceptApp(acceptor, sockId, unixSocket, 0, 0, nonce);
}

void
CProServiceHub::OnAccept(IProAcceptor*    acceptor,
                         int64_t          sockId,
                         bool             unixSocket,
                         const char*      localIp,
                         const char*      remoteIp,
                         unsigned short   remotePort,
                         unsigned char    serviceId,
                         unsigned char    serviceOpt,
                         const PRO_NONCE* nonce)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(localIp != NULL);
    assert(nonce != NULL);
    if (acceptor == NULL || sockId == -1 || localIp == NULL || nonce == NULL)
    {
        return;
    }

    if (!m_enableServiceExt)
    {
        assert(serviceId == 0);
        if (serviceId != 0)
        {
            ProCloseSockId(sockId);

            return;
        }
    }

    if (serviceId == 0)
    {
        AcceptIpc(acceptor, sockId, unixSocket, localIp);
    }
    else
    {
        AcceptApp(acceptor, sockId, unixSocket, serviceId, serviceOpt, *nonce);
    }
}

void
CProServiceHub::AcceptIpc(IProAcceptor* acceptor,
                          int64_t       sockId,
                          bool          unixSocket,
                          const char*   localIp)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(localIp != NULL);

#if !defined(_WIN32)
    assert(unixSocket);
    if (!unixSocket)
    {
        ProCloseSockId(sockId);

        return;
    }
#endif

    assert(strcmp(localIp, "127.0.0.1") == 0);
    if (strcmp(localIp, "127.0.0.1") != 0)
    {
        ProCloseSockId(sockId);

        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (acceptor != m_acceptor)
        {
            ProCloseSockId(sockId);

            return;
        }

        CProServicePipe* pipe = ProCreateServicePipe(false, /* recvFdMode is false */
            this, m_reactor, sockId, unixSocket);
        if (pipe == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        PRO_SERVICE_PIPE sp;
        sp.pipe       = pipe;
        sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;

        /*
         * add a pending pipe
         */
        m_pipe2ServiceId[sp] = 0;
    }
}

void
CProServiceHub::AcceptApp(IProAcceptor*    acceptor,
                          int64_t          sockId,
                          bool             unixSocket,
                          unsigned char    serviceId,
                          unsigned char    serviceOpt,
                          const PRO_NONCE& nonce)
{
    assert(acceptor != NULL);
    assert(sockId != -1);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (acceptor != m_acceptor)
        {
            ProCloseSockId(sockId);

            return;
        }

        CProServicePipe* pipe  = NULL;
        uint32_t         socks = 0;

        auto itr = m_readyPipe2Socks[serviceId].begin();
        auto end = m_readyPipe2Socks[serviceId].end();

        for (; itr != end; ++itr)
        {
            if (pipe == NULL || itr->second < socks)
            {
                pipe  = itr->first;
                socks = itr->second;
            }
        }

        if (pipe == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        auto itr2 = m_pipe2ServiceId.find(pipe);
        if (itr2 == m_pipe2ServiceId.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        const PRO_SERVICE_PIPE& sp = itr2->first;

        PRO_SERVICE_PACKET s2cPacket;
        s2cPacket.s2c.serviceOpt         = serviceOpt;
        s2cPacket.s2c.nonce              = nonce;
        s2cPacket.s2c.oldSock.expireTick = ProGetTickCount64() + SOCK_TIMEOUT * 1000;
        s2cPacket.s2c.oldSock.sockId     = sockId;
        s2cPacket.s2c.oldSock.unixSocket = unixSocket;

#if defined(_WIN32)
        if (::WSADuplicateSocket((SOCKET)sockId,
            (unsigned long)sp.processId, &s2cPacket.s2c.protocolInfo) != 0)
        {
            ProCloseSockId(sockId);

            return;
        }

        pipe->SendData(s2cPacket);
#else
        pipe->SendFd(s2cPacket);
#endif

        m_expireSocks.insert(s2cPacket.s2c.oldSock);
    }
}

void
CProServiceHub::OnRecv(CProServicePipe*          pipe,
                       const PRO_SERVICE_PACKET& packet)
{
    assert(pipe != NULL);
    assert(packet.CheckMagic());
    if (pipe == NULL || !packet.CheckMagic())
    {
        return;
    }

    if (m_enableServiceExt)
    {
        assert(packet.c2s.serviceId > 0);
        if (packet.c2s.serviceId == 0)
        {
            return;
        }
    }
    else
    {
        assert(packet.c2s.serviceId == 0);
        if (packet.c2s.serviceId != 0)
        {
            return;
        }
    }

    IProServiceHubObserver* observer    = NULL;
    unsigned short          servicePort = 0;
    unsigned char           serviceId   = 0;
    unsigned int            processId   = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        auto itr = m_pipe2ServiceId.find(pipe);
        if (itr == m_pipe2ServiceId.end())
        {
            return;
        }

        PRO_SERVICE_PIPE& sp = (PRO_SERVICE_PIPE&)itr->first;

        /*
         * register or update a service host
         */
        auto itr2 = m_readyPipe2Socks[packet.c2s.serviceId].find(pipe);
        if (itr2 == m_readyPipe2Socks[packet.c2s.serviceId].end())
        {
            if (!m_enableLoadBalance && m_readyPipe2Socks[packet.c2s.serviceId].size() > 0)
            {
                return;
            }

            sp.processId  = packet.c2s.processId;
            sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;

            /*
             * activate the pipe
             */
            itr->second                                   = packet.c2s.serviceId;
            m_readyPipe2Socks[packet.c2s.serviceId][pipe] = 0;

            servicePort = ProGetAcceptorPort(m_acceptor);
            serviceId   = packet.c2s.serviceId;
            processId   = packet.c2s.processId;

            m_observer->AddRef();
            observer = m_observer;
        }
        else
        {
            assert(packet.c2s.processId == sp.processId);
            if (packet.c2s.processId != sp.processId)
            {
                return;
            }

            /*
             * update the pipe
             */
            sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;
            itr2->second  = packet.c2s.totalSocks;

            if (packet.c2s.oldSock.sockId != -1)
            {
                m_expireSocks.erase(packet.c2s.oldSock);
                ProCloseSockId(packet.c2s.oldSock.sockId, true); /* linger is true!!! */
            }
        }
    }

    if (observer != NULL)
    {
        observer->OnServiceHostConnected((IProServiceHub*)this, servicePort, serviceId, processId);
        observer->Release();
    }
}

void
CProServiceHub::OnClose(CProServicePipe* pipe)
{
    assert(pipe != NULL);
    if (pipe == NULL)
    {
        return;
    }

    IProServiceHubObserver* observer    = NULL;
    unsigned short          servicePort = 0;
    unsigned char           serviceId   = 0;
    unsigned int            processId   = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        auto itr = m_pipe2ServiceId.find(pipe);
        if (itr == m_pipe2ServiceId.end())
        {
            return;
        }

        PRO_SERVICE_PIPE sp = itr->first;
        unsigned char    id = itr->second;

        m_pipe2ServiceId.erase(itr);

        auto itr2 = m_readyPipe2Socks[id].find(pipe);
        if (itr2 != m_readyPipe2Socks[id].end())
        {
            m_readyPipe2Socks[id].erase(itr2);

            servicePort = ProGetAcceptorPort(m_acceptor);
            serviceId   = id;
            processId   = sp.processId;

            m_observer->AddRef();
            observer = m_observer;
        }
    }

    if (observer != NULL)
    {
        observer->OnServiceHostDisconnected(
            (IProServiceHub*)this, servicePort, serviceId, processId, false); /* timeout is false */
        observer->Release();
    }

    ProDeleteServicePipe(pipe);
}

void
CProServiceHub::OnTimer(void*    factory,
                        uint64_t timerId,
                        int64_t  tick,
                        int64_t  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IProServiceHubObserver*                     observer    = NULL;
    unsigned short                              servicePort = 0;
    CProStlSet<CProServicePipe*>                pipes;
    CProStlMap<PRO_SERVICE_PIPE, unsigned char> pipe2ServiceId;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        {
            auto itr = m_expireSocks.begin();
            auto end = m_expireSocks.end();

            while (itr != end)
            {
                PRO_SERVICE_SOCK ss = *itr;
                if (ss.expireTick > tick)
                {
                    break;
                }

                m_expireSocks.erase(itr++);
                ProCloseSockId(ss.sockId);
            }
        }

        servicePort = ProGetAcceptorPort(m_acceptor);

        {
            auto itr = m_pipe2ServiceId.begin();
            auto end = m_pipe2ServiceId.end();

            while (itr != end)
            {
                PRO_SERVICE_PIPE sp = itr->first;
                unsigned char    id = itr->second;

                if (sp.expireTick > tick)
                {
                    ++itr;
                    continue;
                }

                m_pipe2ServiceId.erase(itr++);
                pipes.insert(sp.pipe);

                auto itr2 = m_readyPipe2Socks[id].find(sp.pipe);
                if (itr2 == m_readyPipe2Socks[id].end())
                {
                    continue;
                }

                m_readyPipe2Socks[id].erase(itr2);
                pipe2ServiceId[sp] = id;
            }
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    {
        auto itr = pipes.begin();
        auto end = pipes.end();

        for (; itr != end; ++itr)
        {
            ProDeleteServicePipe(*itr);
        }
    }

    {
        auto itr = pipe2ServiceId.begin();
        auto end = pipe2ServiceId.end();

        for (; itr != end; ++itr)
        {
            const PRO_SERVICE_PIPE& sp = itr->first;
            unsigned char           id = itr->second;

            observer->OnServiceHostDisconnected(
                (IProServiceHub*)this, servicePort, id, sp.processId, true); /* timeout is true */
        }
    }

    observer->Release();
}
