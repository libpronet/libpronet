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
CProServiceHub::CreateInstance()
{
    CProServiceHub* const hub = new CProServiceHub;

    return (hub);
}

CProServiceHub::CProServiceHub()
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

        m_acceptor = ProCreateAcceptorEx(
            this, reactor, "0.0.0.0", servicePort, timeoutInSeconds);
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
    IProAcceptor*                acceptor = NULL;
    CProStlSet<PRO_SERVICE_PIPE> allPipes;
    CProStlSet<PRO_SERVICE_SOCK> expireSocks;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        expireSocks = m_expireSocks;
        m_expireSocks.clear();
        m_serviceId2Pipe.clear();
        allPipes = m_allPipes;
        m_allPipes.clear();
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
        CProStlSet<PRO_SERVICE_PIPE>::iterator       itr = allPipes.begin();
        CProStlSet<PRO_SERVICE_PIPE>::iterator const end = allPipes.end();

        for (; itr != end; ++itr)
        {
            ProDeleteServicePipe(itr->pipe);
        }
    }

    ProDeleteAcceptor(acceptor);
}

unsigned long
PRO_CALLTYPE
CProServiceHub::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProServiceHub::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CProServiceHub::OnAccept(IProAcceptor*    acceptor,
                         PRO_INT64        sockId,
                         bool             unixSocket,
                         const char*      remoteIp,
                         unsigned short   remotePort,
                         unsigned char    serviceId,
                         unsigned char    serviceOpt,
                         const PRO_NONCE* nonce)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(nonce != NULL);
    if (acceptor == NULL || sockId == -1 || remoteIp == NULL || nonce == NULL)
    {
        return;
    }

    if (serviceId == 0)
    {
        OnAcceptIpc(sockId, unixSocket, remoteIp);
    }
    else
    {
        OnAcceptOther(sockId, unixSocket, serviceId, serviceOpt, *nonce);
    }
}

void
CProServiceHub::OnAcceptIpc(PRO_INT64   sockId,
                            bool        unixSocket,
                            const char* remoteIp)
{
    assert(sockId != -1);
    assert(remoteIp != NULL);

    if (pbsd_inet_aton(remoteIp) != pbsd_inet_aton("127.0.0.1"))
    {
        ProCloseSockId(sockId);

        return;
    }

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    if (!unixSocket)
    {
        ProCloseSockId(sockId);

        return;
    }
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
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
        sp.pending    = true;
        sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;

        m_allPipes.insert(sp);
    }
}

void
CProServiceHub::OnAcceptOther(PRO_INT64        sockId,
                              bool             unixSocket,
                              unsigned char    serviceId,
                              unsigned char    serviceOpt,
                              const PRO_NONCE& nonce)
{
    assert(sockId != -1);
    assert(serviceId > 0);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        CProStlMap<unsigned char, PRO_SERVICE_PIPE>::iterator const itr =
            m_serviceId2Pipe.find(serviceId);
        if (itr == m_serviceId2Pipe.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        const PRO_SERVICE_PIPE& sp = itr->second;
        assert(sp.pipe != NULL);
        assert(!sp.pending);
        assert(sp.serviceId == serviceId);

        PRO_SERVICE_PACKET s2cPacket;
        s2cPacket.s2c.serviceId          = serviceId;
        s2cPacket.s2c.serviceOpt         = serviceOpt;
        s2cPacket.s2c.nonce              = nonce;
        s2cPacket.s2c.oldSock.expireTick = ProGetTickCount64() + SOCK_TIMEOUT * 1000;
        s2cPacket.s2c.oldSock.sockId     = sockId;
        s2cPacket.s2c.oldSock.unixSocket = unixSocket;

#if defined(_WIN32_WCE)
        sp.pipe->SendData(s2cPacket);
#elif defined(_WIN32)
        if (::WSADuplicateSocket((SOCKET)sockId,
            (unsigned long)sp.processId, &s2cPacket.s2c.protocolInfo) != 0)
        {
            ProCloseSockId(sockId);

            return;
        }

        sp.pipe->SendData(s2cPacket);
#else
        sp.pipe->SendFd(s2cPacket);
#endif

        m_expireSocks.insert(s2cPacket.s2c.oldSock);
    }
}

void
PRO_CALLTYPE
CProServiceHub::OnRecv(CProServicePipe*          pipe,
                       const PRO_SERVICE_PACKET& packet)
{
    assert(pipe != NULL);
    assert(packet.CheckMagic());
    if (pipe == NULL || !packet.CheckMagic())
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_acceptor == NULL)
        {
            return;
        }

        PRO_SERVICE_PIPE sp;
        sp.pipe = pipe;

        CProStlSet<PRO_SERVICE_PIPE>::iterator const itr = m_allPipes.find(sp);
        if (itr == m_allPipes.end())
        {
            return;
        }

        sp = *itr;
        assert(sp.pipe == pipe);

        if (sp.pending)
        {
            if (m_serviceId2Pipe.find(packet.c2s.serviceId) !=
                m_serviceId2Pipe.end())
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

            sp.pending    = false;
            sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;
            sp.serviceId  = packet.c2s.serviceId;
            sp.processId  = packet.c2s.processId;

            m_serviceId2Pipe[sp.serviceId] = sp;

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
                    (unsigned int)sp.serviceId,
                    (unsigned int)sp.processId,
                    (unsigned int)sp.processId
                    );
            }}}
        }
        else
        {
            assert(packet.c2s.serviceId == sp.serviceId);
            assert(packet.c2s.processId == sp.processId);
            if (packet.c2s.serviceId != sp.serviceId ||
                packet.c2s.processId != sp.processId)
            {
                return;
            }

            sp.expireTick = ProGetTickCount64() + PIPE_TIMEOUT * 1000;

            if (packet.c2s.oldSock.sockId != -1)
            {
                m_expireSocks.erase(packet.c2s.oldSock);

#if !defined(_WIN32_WCE)
                ProCloseSockId(packet.c2s.oldSock.sockId, true); /* true!!! */
#endif
            }
        }

        /*
         * update pipes
         */
        m_allPipes.erase(itr);
        m_allPipes.insert(sp);
    }
}

void
PRO_CALLTYPE
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

        PRO_SERVICE_PIPE sp;
        sp.pipe = pipe;

        CProStlSet<PRO_SERVICE_PIPE>::iterator const itr = m_allPipes.find(sp);
        if (itr == m_allPipes.end())
        {
            return;
        }

        sp = *itr;
        assert(sp.pipe == pipe);

        CProStlMap<unsigned char, PRO_SERVICE_PIPE>::iterator const itr2 =
            m_serviceId2Pipe.find(sp.serviceId);
        if (itr2 != m_serviceId2Pipe.end() && itr2->second.pipe == pipe)
        {
            m_serviceId2Pipe.erase(itr2);

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
                    (unsigned int)sp.serviceId,
                    (unsigned int)sp.processId,
                    (unsigned int)sp.processId
                    );
            }}}
        }

        m_allPipes.erase(itr);
    }

    ProDeleteServicePipe(pipe);
}

void
PRO_CALLTYPE
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
            CProStlSet<PRO_SERVICE_PIPE>::iterator       itr = m_allPipes.begin();
            CProStlSet<PRO_SERVICE_PIPE>::iterator const end = m_allPipes.end();

            while (itr != end)
            {
                const PRO_SERVICE_PIPE sp = *itr;
                if (sp.expireTick > tick)
                {
                    ++itr;
                }
                else
                {
                    m_allPipes.erase(itr++);

                    CProStlMap<unsigned char, PRO_SERVICE_PIPE>::iterator const itr2 =
                        m_serviceId2Pipe.find(sp.serviceId);
                    if (itr2 != m_serviceId2Pipe.end() &&
                        itr2->second.pipe == sp.pipe)
                    {
                        m_serviceId2Pipe.erase(itr2);

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
                                (unsigned int)sp.serviceId,
                                (unsigned int)sp.processId,
                                (unsigned int)sp.processId
                                );
                        }}}
                    }

                    pipes.insert(sp.pipe);
                }
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
