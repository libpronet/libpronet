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

#include "pro_tcp_handshaker.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_send_pool.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CProTcpHandshaker*
CProTcpHandshaker::CreateInstance()
{
    return new CProTcpHandshaker;
}

CProTcpHandshaker::CProTcpHandshaker()
{
    m_observer    = NULL;
    m_reactorTask = NULL;
    m_sockId      = -1;
    m_unixSocket  = false;
    m_timerId     = 0;
}

CProTcpHandshaker::~CProTcpHandshaker()
{
    Fini();

    ProCloseSockId(m_sockId);
    m_sockId = -1;
}

bool
CProTcpHandshaker::Init(IProTcpHandshakerObserver* observer,
                        CProTpReactorTask*         reactorTask,
                        int64_t                    sockId,
                        bool                       unixSocket,
                        const void*                sendData,         /* = NULL */
                        size_t                     sendDataSize,     /* = 0 */
                        size_t                     recvDataSize,     /* = 0 */
                        bool                       recvFirst,        /* = false */
                        unsigned int               timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactorTask == NULL || sockId == -1)
    {
        return false;
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return false;
        }

        if (sendData != NULL && sendDataSize > 0)
        {
            m_sendPool.Fill(sendData, sendDataSize);
        }

        if (recvDataSize > 0 && !m_recvPool.Resize(recvDataSize))
        {
            return false;
        }

        unsigned long mask = 0;
        if (recvDataSize == 0)
        {
            mask = PRO_MASK_WRITE;
        }
        else if (sendData == NULL || sendDataSize == 0)
        {
            mask = PRO_MASK_READ;
        }
        else if (recvFirst)
        {
            mask = PRO_MASK_READ;
        }
        else
        {
            mask = PRO_MASK_WRITE | PRO_MASK_READ;
        }
        if (!reactorTask->AddHandler(sockId, this, mask))
        {
            return false;
        }

        observer->AddRef();
        m_observer    = observer;
        m_reactorTask = reactorTask;
        m_sockId      = sockId;
        m_unixSocket  = unixSocket;
        m_timerId     = reactorTask->ScheduleTimer(this, (uint64_t)timeoutInSeconds * 1000, false, 0);
    }

    return true;
}

void
CProTcpHandshaker::Fini()
{
    IProTcpHandshakerObserver* observer = NULL;

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

void
CProTcpHandshaker::OnInput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTcpHandshakerObserver* observer  = NULL;
    int                        errorCode = 0;

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
        if (idleSize == 0)
        {
            m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);

            return;
        }

        int recvSize = pbsd_recv(m_sockId, m_recvPool.ContinuousIdleBuf(), idleSize, 0);
        assert(recvSize <= (int)idleSize);

        if (recvSize > (int)idleSize)
        {
            errorCode = -1;
        }
        else if (recvSize > 0)
        {
            m_recvPool.Fill(recvSize);
            if (m_recvPool.ContinuousIdleSize() > 0)
            {
                return;
            }

            m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);
            if (m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE)) /* make a callback */
            {
                return;
            }

            errorCode = -1;
        }
        else if (recvSize == 0)
        {
        }
        else
        {
            errorCode = pbsd_errno((void*)&pbsd_recv);
            if (errorCode == PBSD_EWOULDBLOCK)
            {
                return;
            }
        }

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->OnHandshakeError((IProTcpHandshaker*)this, errorCode);
    observer->Release();
}

void
CProTcpHandshaker::OnOutput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTcpHandshakerObserver* observer  = NULL;
    void*                      buf       = NULL;
    size_t                     size      = 0;
    int                        errorCode = 0;
    bool                       error     = false;

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

        size_t            theSize   = 0;
        const void*       theBuf    = m_sendPool.PreSend(theSize);
        const CProBuffer* onSendBuf = NULL;
        size_t            idleSize  = 0;

        if (theBuf != NULL && theSize > 0)
        {
            int sentSize = pbsd_send(m_sockId, theBuf, theSize, 0);
            assert(sentSize <= (int)theSize);

            if (sentSize > (int)theSize)
            {
                error     = true;
                errorCode = -1;
            }
            else if (sentSize > 0)
            {
                m_sendPool.Flush(sentSize);

                onSendBuf = m_sendPool.OnSendBuf();
                if (onSendBuf != NULL)
                {
                    m_sendPool.PostSend();
                }
            }
            else if (sentSize == 0)
            {
            }
            else
            {
                int errorCode2 = pbsd_errno((void*)&pbsd_send);
                if (errorCode2 != PBSD_EWOULDBLOCK)
                {
                    error     = true;
                    errorCode = errorCode2;
                }
            }

            if (error)
            {
                goto EXIT;
            }

            if (onSendBuf == NULL)
            {
                return;
            }
        }

        assert(theBuf == NULL || theSize == 0 || onSendBuf != NULL);

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);

        idleSize = m_recvPool.ContinuousIdleSize();
        if (idleSize > 0)
        {
            return;
        }

        size = m_recvPool.PeekDataSize();
        if (size > 0)
        {
            buf = ProMalloc(size);
            if (buf == NULL)
            {
                error     = true;
                errorCode = -1;
            }
            else
            {
                m_recvPool.PeekData(buf, size);
            }
        }

EXIT:

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);
        if (!error)
        {
            m_sockId = -1; /* cut */
        }

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    if (error)
    {
        observer->OnHandshakeError((IProTcpHandshaker*)this, errorCode);
    }
    else
    {
        observer->OnHandshakeOk((IProTcpHandshaker*)this, sockId, m_unixSocket, buf, size);
    }

    ProFree(buf);
    observer->Release();
}

void
CProTcpHandshaker::OnError(int64_t sockId,
                           int     errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTcpHandshakerObserver* observer = NULL;

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

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->OnHandshakeError((IProTcpHandshaker*)this, errorCode);
    observer->Release();
}

void
CProTcpHandshaker::OnTimer(void*    factory,
                           uint64_t timerId,
                           int64_t  userData)
{
    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    IProTcpHandshakerObserver* observer  = NULL;
    int                        errorCode = PBSD_ETIMEDOUT;

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

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->OnHandshakeError((IProTcpHandshaker*)this, errorCode);
    observer->Release();
}
