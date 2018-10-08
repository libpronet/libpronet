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

#include "pro_service_pipe.h"
#include "pro_net.h"
#include "pro_tcp_transport.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define RECV_BUF_SIZE  (1024 * 16)
#define SEND_BUF_SIZE  (1024 * 16)
#define RECV_POOL_SIZE (1024 * 16)

/////////////////////////////////////////////////////////////////////////////
////

CProServicePipe*
CProServicePipe::CreateInstance()
{
    CProServicePipe* const pipe = new CProServicePipe;

    return (pipe);
}

CProServicePipe::CProServicePipe()
{
    m_observer = NULL;
    m_reactor  = NULL;
    m_trans    = NULL;
}

CProServicePipe::~CProServicePipe()
{
    Fini();
}

bool
CProServicePipe::Init(bool                     recvFdMode,
                      IProServicePipeObserver* observer,
                      IProReactor*             reactor,
                      PRO_INT64                sockId,
                      bool                     unixSocket)
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactor == NULL || sockId == -1)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL)
        {
            return (false);
        }

        m_trans = CProTcpTransport::CreateInstance(recvFdMode, RECV_POOL_SIZE);
        if (m_trans == NULL)
        {
            return (false);
        }

        if (!m_trans->Init(this, (CProTpReactorTask*)reactor,
            sockId, unixSocket, RECV_BUF_SIZE, SEND_BUF_SIZE))
        {
            m_trans->Release();
            m_trans = NULL;

            return (false);
        }

        observer->AddRef();
        m_observer = observer;
        m_reactor  = reactor;
    }

    return (true);
}

void
CProServicePipe::Fini()
{
    IProServicePipeObserver* observer = NULL;
    CProTcpTransport*        trans    = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    trans->Fini();
    trans->Release();
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CProServicePipe::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProServicePipe::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CProServicePipe::SendData(const PRO_SERVICE_PACKET& packet)
{
    assert(packet.CheckMagic());
    if (!packet.CheckMagic())
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_packets.push_back(packet);

        const PRO_SERVICE_PACKET& packet2 = m_packets.front();
        if (m_trans->SendData(&packet2, sizeof(PRO_SERVICE_PACKET), 0, NULL))
        {
            m_packets.pop_front();
        }
    }
}

void
CProServicePipe::SendFd(const PRO_SERVICE_PACKET& s2cPacket)
{
    assert(s2cPacket.s2c.oldSock.sockId != -1);
    assert(s2cPacket.CheckMagic());
    if (s2cPacket.s2c.oldSock.sockId == -1 || !s2cPacket.CheckMagic())
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_fdPackets.push_back(s2cPacket);

        const PRO_SERVICE_PACKET& packet = m_fdPackets.front();
        if (m_trans->SendFd(packet))
        {
            m_fdPackets.pop_front();
        }
    }
}

void
PRO_CALLTYPE
CProServicePipe::OnRecv(IProTransport*          trans,
                        const pbsd_sockaddr_in* remoteAddr)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    bool error = false;

    while (1)
    {
        IProServicePipeObserver* observer = NULL;
        PRO_SERVICE_PACKET       packet;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
            {
                return;
            }

            if (trans != m_trans)
            {
                return;
            }

            IProRecvPool&       recvPool = *m_trans->GetRecvPool();
            const unsigned long dataSize = recvPool.PeekDataSize();

            if (dataSize < sizeof(PRO_SERVICE_PACKET))
            {
                break;
            }

            recvPool.PeekData(&packet, sizeof(PRO_SERVICE_PACKET));
            recvPool.Flush(sizeof(PRO_SERVICE_PACKET));

            assert(packet.CheckMagic());
            if (!packet.CheckMagic())
            {
                error = true;
            }

            m_observer->AddRef();
            observer = m_observer;
        }

        if (error)
        {
            observer->OnClose(this);
            observer->Release();
            break;
        }
        else
        {
            observer->OnRecv(this, packet);
            observer->Release();
        }
    } /* end of while (...) */

    if (error)
    {
        Fini();
    }
}

void
PRO_CALLTYPE
CProServicePipe::OnRecvFd(IProTransport*            trans,
                          PRO_INT64                 fd,
                          bool                      unixSocket,
                          const PRO_SERVICE_PACKET& s2cPacket)
{
    assert(trans != NULL);
    assert(fd != -1);
    assert(s2cPacket.CheckMagic());
    if (trans == NULL || fd == -1 || !s2cPacket.CheckMagic())
    {
        return;
    }

    IProServicePipeObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            ProCloseSockId(fd);

            return;
        }

        if (trans != m_trans)
        {
            ProCloseSockId(fd);

            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnRecvFd(this, fd, unixSocket, s2cPacket);
    observer->Release();
}

void
PRO_CALLTYPE
CProServicePipe::OnSend(IProTransport* trans,
                        PRO_UINT64     actionId)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (trans != m_trans)
        {
            return;
        }

        if (m_packets.size() > 0)
        {
            const PRO_SERVICE_PACKET& packet = m_packets.front();
            if (m_trans->SendData(&packet, sizeof(PRO_SERVICE_PACKET), 0, NULL))
            {
                m_packets.pop_front();
            }
        }

        if (m_fdPackets.size() > 0)
        {
            const PRO_SERVICE_PACKET& packet = m_fdPackets.front();
            if (m_trans->SendFd(packet))
            {
                m_fdPackets.pop_front();
            }
        }
    }
}

void
PRO_CALLTYPE
CProServicePipe::OnClose(IProTransport* trans,
                         long           errorCode,
                         long           sslCode)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    IProServicePipeObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (trans != m_trans)
        {
            return;
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    observer->OnClose(this);
    observer->Release();

    Fini();
}

/////////////////////////////////////////////////////////////////////////////
////

CProServicePipe*
PRO_CALLTYPE
ProCreateServicePipe(bool                     recvFdMode,
                     IProServicePipeObserver* observer,
                     IProReactor*             reactor,
                     PRO_INT64                sockId,
                     bool                     unixSocket)
{
    CProServicePipe* const pipe = CProServicePipe::CreateInstance();
    if (pipe == NULL)
    {
        return (NULL);
    }

    if (!pipe->Init(recvFdMode, observer, reactor, sockId, unixSocket))
    {
        pipe->Release();

        return (NULL);
    }

    return (pipe);
}

void
PRO_CALLTYPE
ProDeleteServicePipe(CProServicePipe* pipe)
{
    if (pipe == NULL)
    {
        return;
    }

    pipe->Fini();
    pipe->Release();
}
