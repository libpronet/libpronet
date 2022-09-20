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

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

#include <cassert>

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
    CProServiceHub* const hub =
        new CProServiceHub(enableServiceExt, enableLoadBalance);

    return (hub);
}

CProServiceHub::CProServiceHub(bool enableServiceExt,
                               bool enableLoadBalance)
                               :
m_enableServiceExt(enableServiceExt),
m_enableLoadBalance(enableLoadBalance)
{
    m_reactor  = NULL;
    m_acceptor = NULL;
    m_timerId  = 0;
}

CProServiceHub::~CProServiceHub()
{
    Fini();
}

bool
CProServiceHub::Init(IProReactor*   reactor,
                     unsigned short servicePort,
                     unsigned long  timeoutInSeconds) /* = 0 */
{
    assert(reactor != NULL);
    assert(servicePort > 0);
    if (reactor == NULL || servicePort == 0)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_acceptor == NULL);
        if (m_reactor != NULL || m_acceptor != NULL)
        {
            return (false);
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
            return (false);
        }

        m_reactor = reactor;
        m_timerId = reactor->ScheduleTimer(this, HEARTBEAT_INTERVAL * 1000, true);
    }

    return (true);
}

void
CProServiceHub::Fini()
{
    IProAcceptor*                               acceptor = NULL;
    CProStlMap<PRO_SERVICE_PIPE, unsigned char> pipe2ServiceId;
    CProStlSet<PRO_SERVICE_SOCK>                expireSocks;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
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
    }

    {
        CProStlSet<PRO_SERVICE_SOCK>::iterator       itr = expireSocks.begin();
        CProStlSet<PRO_SERVICE_SOCK>::iterator const end = expireSocks.end();

        for (; itr != end; ++itr)
        {
            ProCloseSockId(itr->sockId);
        }
    }

    {
        CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator       itr = pipe2ServiceId.begin();
        CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator const end = pipe2ServiceId.end();

        for (; itr != end; ++itr)
        {
            ProDeleteServicePipe(itr->first.pipe);
        }
    }

    ProDeleteAcceptor(acceptor);
}

unsigned long
CProServiceHub::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
CProServiceHub::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CProServiceHub::OnAccept(IProAcceptor*  acceptor,
                         PRO_INT64      sockId,
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
                         PRO_INT64        sockId,
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
                          PRO_INT64     sockId,
                          bool          unixSocket,
                          const char*   localIp)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(localIp != NULL);

#if !defined(_WIN32) && !defined(_WIN32_WCE)
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

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (acceptor != m_acceptor)
        {
            ProCloseSockId(sockId);

            return;
        }

        CProServicePipe* const pipe = ProCreateServicePipe(
            false, this, m_reactor, sockId, unixSocket); /* recvFdMode is false */
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
                          PRO_INT64        sockId,
                          bool             unixSocket,
                          unsigned char    serviceId,
                          unsigned char    serviceOpt,
                          const PRO_NONCE& nonce)
{
    assert(acceptor != NULL);
    assert(sockId != -1);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
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
        PRO_UINT32       socks = 0;

        CProStlMap<CProServicePipe*, PRO_UINT32>::iterator       itr = m_readyPipe2Socks[serviceId].begin();
        CProStlMap<CProServicePipe*, PRO_UINT32>::iterator const end = m_readyPipe2Socks[serviceId].end();

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

        CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator const itr2 =
            m_pipe2ServiceId.find(pipe);
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

#if defined(_WIN32_WCE)
        pipe->SendData(s2cPacket);
#elif defined(_WIN32)
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

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator const itr =
            m_pipe2ServiceId.find(pipe);
        if (itr == m_pipe2ServiceId.end())
        {
            return;
        }

        PRO_SERVICE_PIPE& sp = (PRO_SERVICE_PIPE&)itr->first;

        /*
         * register or update a service host
         */
        CProStlMap<CProServicePipe*, PRO_UINT32>::iterator const itr2 =
            m_readyPipe2Socks[packet.c2s.serviceId].find(pipe);
        if (itr2 == m_readyPipe2Socks[packet.c2s.serviceId].end())
        {
            if (!m_enableLoadBalance &&
                m_readyPipe2Socks[packet.c2s.serviceId].size() > 0)
            {
                return;
            }

#if defined(_WIN32_WCE)
            assert(packet.c2s.processId == ProGetProcessId());
            if (packet.c2s.processId != ProGetProcessId())
            {
                return;
            }
#endif

            sp.processId  = packet.c2s.processId;
            sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;

            /*
             * activate the pipe
             */
            itr->second                                   = packet.c2s.serviceId;
            m_readyPipe2Socks[packet.c2s.serviceId][pipe] = 0;

            {{{
                CProStlString timeString = "";
                ProGetLocalTimeString(timeString);

                printf(
                    "\n"
                    "%s \n"
                    " CProServiceHub::OnRecv(port : %u,"
                    " serviceId : %u, processId : %u/0x%X) [ok] \n"
                    ,
                    timeString.c_str(),
                    (unsigned int)ProGetAcceptorPort(m_acceptor),
                    (unsigned int)packet.c2s.serviceId,
                    (unsigned int)packet.c2s.processId,
                    (unsigned int)packet.c2s.processId
                    );
            }}}
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

#if !defined(_WIN32_WCE)
                ProCloseSockId(packet.c2s.oldSock.sockId, true); /* linger is true!!! */
#endif
            }
        }
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

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator const itr =
            m_pipe2ServiceId.find(pipe);
        if (itr == m_pipe2ServiceId.end())
        {
            return;
        }

        const PRO_SERVICE_PIPE sp = itr->first;
        const unsigned char    id = itr->second;

        m_pipe2ServiceId.erase(itr);

        CProStlMap<CProServicePipe*, PRO_UINT32>::iterator const itr2 =
            m_readyPipe2Socks[id].find(pipe);
        if (itr2 != m_readyPipe2Socks[id].end())
        {
            m_readyPipe2Socks[id].erase(itr2);

            {{{
                CProStlString timeString = "";
                ProGetLocalTimeString(timeString);

                printf(
                    "\n"
                    "%s \n"
                    " CProServiceHub::OnClose(port : %u,"
                    " serviceId : %u, processId : %u/0x%X) [broken] \n"
                    ,
                    timeString.c_str(),
                    (unsigned int)ProGetAcceptorPort(m_acceptor),
                    (unsigned int)id,
                    (unsigned int)sp.processId,
                    (unsigned int)sp.processId
                    );
            }}}
        }
    }

    ProDeleteServicePipe(pipe);
}

void
CProServiceHub::OnTimer(void*      factory,
                        PRO_UINT64 timerId,
                        PRO_INT64  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    CProStlSet<CProServicePipe*> pipes;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        const PRO_INT64 tick = ProGetTickCount64();

        {
            CProStlSet<PRO_SERVICE_SOCK>::iterator       itr = m_expireSocks.begin();
            CProStlSet<PRO_SERVICE_SOCK>::iterator const end = m_expireSocks.end();

            while (itr != end)
            {
                const PRO_SERVICE_SOCK ss = *itr;
                if (ss.expireTick > tick)
                {
                    break;
                }

                m_expireSocks.erase(itr++);
                ProCloseSockId(ss.sockId);
            }
        }

        {
            CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator       itr = m_pipe2ServiceId.begin();
            CProStlMap<PRO_SERVICE_PIPE, unsigned char>::iterator const end = m_pipe2ServiceId.end();

            while (itr != end)
            {
                const PRO_SERVICE_PIPE sp = itr->first;
                const unsigned char    id = itr->second;

                if (sp.expireTick > tick)
                {
                    ++itr;
                    continue;
                }

                m_pipe2ServiceId.erase(itr++);
                pipes.insert(sp.pipe);

                CProStlMap<CProServicePipe*, PRO_UINT32>::iterator const itr2 =
                    m_readyPipe2Socks[id].find(sp.pipe);
                if (itr2 == m_readyPipe2Socks[id].end())
                {
                    continue;
                }

                m_readyPipe2Socks[id].erase(itr2);

                {{{
                    CProStlString timeString = "";
                    ProGetLocalTimeString(timeString);

                    printf(
                        "\n"
                        "%s \n"
                        " CProServiceHub::OnTimer(port : %u,"
                        " serviceId : %u, processId : %u/0x%X)"
                        " [timeout] \n"
                        ,
                        timeString.c_str(),
                        (unsigned int)ProGetAcceptorPort(m_acceptor),
                        (unsigned int)id,
                        (unsigned int)sp.processId,
                        (unsigned int)sp.processId
                        );
                }}}
            } /* end of while (...) */
        }
    }

    CProStlSet<CProServicePipe*>::iterator       itr = pipes.begin();
    CProStlSet<CProServicePipe*>::iterator const end = pipes.end();

    for (; itr != end; ++itr)
    {
        ProDeleteServicePipe(*itr);
    }
}
