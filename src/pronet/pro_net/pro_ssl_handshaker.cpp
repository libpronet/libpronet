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

#include "pro_ssl_handshaker.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_send_pool.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

#include "mbedtls/ssl.h"

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CProSslHandshaker*
CProSslHandshaker::CreateInstance()
{
    CProSslHandshaker* const handshaker = new CProSslHandshaker;

    return (handshaker);
}

CProSslHandshaker::CProSslHandshaker()
{
    m_observer    = NULL;
    m_reactorTask = NULL;
    m_ctx         = NULL;
    m_sslOk       = false;
    m_sockId      = -1;
    m_unixSocket  = false;
    m_onWr        = false;
    m_recvFirst   = false;
    m_timerId     = 0;
}

CProSslHandshaker::~CProSslHandshaker()
{
    Fini();

    ProSslCtx_Delete(m_ctx);
    ProCloseSockId(m_sockId);
    m_ctx    = NULL;
    m_sockId = -1;
}

bool
CProSslHandshaker::Init(IProSslHandshakerObserver* observer,
                        CProTpReactorTask*         reactorTask,
                        PRO_SSL_CTX*               ctx,
                        PRO_INT64                  sockId,
                        bool                       unixSocket,
                        const void*                sendData,         /* = NULL */
                        size_t                     sendDataSize,     /* = 0 */
                        size_t                     recvDataSize,     /* = 0 */
                        bool                       recvFirst,        /* = false */
                        unsigned long              timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(ctx != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactorTask == NULL || ctx == NULL || sockId == -1)
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        assert(m_ctx == NULL);
        if (m_observer != NULL || m_reactorTask != NULL || m_ctx != NULL)
        {
            return (false);
        }

        if (sendData != NULL && sendDataSize > 0)
        {
            m_sendPool.Fill(sendData, sendDataSize);
        }

        if (recvDataSize > 0 && !m_recvPool.Resize(recvDataSize))
        {
            return (false);
        }

        if (!reactorTask->AddHandler(
            sockId, this, PRO_MASK_WRITE | PRO_MASK_READ))
        {
            return (false);
        }

        observer->AddRef();
        m_observer    = observer;
        m_reactorTask = reactorTask;
        m_ctx         = ctx;
        m_sslOk       = ((mbedtls_ssl_context*)ctx)->state == MBEDTLS_SSL_HANDSHAKE_OVER;
        m_sockId      = sockId;
        m_unixSocket  = unixSocket;
        m_onWr        = true;
        m_recvFirst   = recvFirst;
        m_timerId     = reactorTask->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false, 0);
    }

    return (true);
}

void
CProSslHandshaker::Fini()
{
    IProSslHandshakerObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_ctx != NULL && m_sockId != -1 && m_sslOk)
        {
            mbedtls_ssl_close_notify((mbedtls_ssl_context*)m_ctx);
        }

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
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

void
PRO_CALLTYPE
CProSslHandshaker::OnInput(PRO_INT64 sockId)
{
    DoRecv(sockId);
}

void
PRO_CALLTYPE
CProSslHandshaker::OnOutput(PRO_INT64 sockId)
{
    DoSend(sockId);
    DoRecv(sockId); /* !!! */
}

void
CProSslHandshaker::DoRecv(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProSslHandshakerObserver* observer  = NULL;
    int                        errorCode = 0;
    int                        sslCode   = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        bool   error   = false;
        bool   regWr   = false;
        size_t msgSize = 0;

        if (!m_sslOk)
        {
            const int ret = mbedtls_ssl_handshake((mbedtls_ssl_context*)m_ctx);
            if (ret == 0)
            {
                m_sslOk = true;
            }
            else if (ret == MBEDTLS_ERR_SSL_WANT_READ)
            {
                return;
            }
            else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                if (m_onWr)
                {
                    return;
                }

                if (m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE))
                {
                    m_onWr = true;

                    return;
                }

                error     = true;
                errorCode = -1;

                goto EXIT;
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = ret;

                goto EXIT;
            }
        }

        assert(m_sslOk);

        do
        {
            const size_t idleSize = m_recvPool.ContinuousIdleSize();
            const size_t minSize  = (msgSize == 0 || msgSize > idleSize) ? idleSize : msgSize;

            if (idleSize == 0)
            {
                m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);
                regWr = true; /* make a callback */
                break;
            }

            const int recvSize = mbedtls_ssl_read((mbedtls_ssl_context*)m_ctx,
                (unsigned char*)m_recvPool.ContinuousIdleBuf(), minSize);
            assert(recvSize <= (int)minSize);

            if (recvSize > (int)minSize)
            {
                error     = true;
                errorCode = -1;
                sslCode   = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
                break;
            }
            else if (recvSize > 0)
            {
                m_recvPool.Fill(recvSize);
                msgSize = mbedtls_ssl_get_bytes_avail( /* remaining message */
                    (mbedtls_ssl_context*)m_ctx);

                if (m_recvPool.ContinuousIdleSize() > 0)
                {
                    continue;
                }

                m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_READ);
                regWr = true; /* make a callback */
                break;
            }
            else if (recvSize == 0)
            {
                error = true;
                break;
            }
            else if (recvSize == MBEDTLS_ERR_SSL_WANT_READ)
            {
                break;
            }
            else if (recvSize == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                regWr = true;
                break;
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = recvSize;
                break;
            }
        }
        while (msgSize > 0); /* read a complete ssl record */

        if (!error)
        {
            unsigned long     theSize = 0;
            const void* const theBuf  = m_sendPool.PreSend(theSize);

            if (
                !m_onWr
                &&
                (
                 regWr
                 ||
                 (theBuf != NULL && theSize > 0)
                )
               )
            {
                if (m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE)) /* !!! */
                {
                    m_onWr = true;
                }
                else
                {
                    error     = true;
                    errorCode = -1;

                    goto EXIT;
                }
            }

            return;
        }

EXIT:

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(
            m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->OnHandshakeError((IProSslHandshaker*)this, errorCode, sslCode);
    observer->Release();
}

void
CProSslHandshaker::DoSend(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProSslHandshakerObserver* observer  = NULL;
    PRO_SSL_CTX*               ctx       = NULL;
    void*                      buf       = NULL;
    unsigned long              size      = 0;
    int                        errorCode = 0;
    int                        sslCode   = 0;
    bool                       error     = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
        {
            return;
        }

        unsigned long     theSize   = 0;
        const void* const theBuf    = m_sendPool.PreSend(theSize);
        const CProBuffer* onSendBuf = NULL;
        size_t            idleSize  = 0;

        if (!m_sslOk)
        {
            const int ret = mbedtls_ssl_handshake((mbedtls_ssl_context*)m_ctx);
            if (ret == 0)
            {
                m_sslOk = true;
            }
            else if (ret == MBEDTLS_ERR_SSL_WANT_READ)
            {
                if (m_onWr)
                {
                    m_reactorTask->RemoveHandler(
                        m_sockId, this, PRO_MASK_WRITE);
                    m_onWr = false;
                }

                return;
            }
            else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                return;
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = ret;

                goto EXIT;
            }
        }

        assert(m_sslOk);

        idleSize = m_recvPool.ContinuousIdleSize();
        if (m_recvFirst && idleSize > 0)
        {
            if (m_onWr)
            {
                m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
                m_onWr = false;
            }

            return;
        }

        if (theBuf != NULL && theSize > 0)
        {
            const int sentSize = mbedtls_ssl_write(
                (mbedtls_ssl_context*)m_ctx, (unsigned char*)theBuf, theSize);
            assert(sentSize <= (int)theSize);

            if (sentSize > (int)theSize)
            {
                error     = true;
                errorCode = -1;
                sslCode   = MBEDTLS_ERR_SSL_INTERNAL_ERROR;

                goto EXIT;
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
            else if (sentSize == 0 || sentSize == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
            }
            else if (sentSize == MBEDTLS_ERR_SSL_WANT_READ)
            {
                if (m_onWr)
                {
                    m_reactorTask->RemoveHandler(
                        m_sockId, this, PRO_MASK_WRITE);
                    m_onWr = false;
                }
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = sentSize;

                goto EXIT;
            }

            if (onSendBuf == NULL)
            {
                return;
            }
        }

        assert(theBuf == NULL || theSize == 0 || onSendBuf != NULL);

        if (m_onWr)
        {
            m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
            m_onWr = false;
        }

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

        m_reactorTask->RemoveHandler(
            m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);
        if (!error)
        {
            ctx = m_ctx;
            m_ctx    = NULL; /* cut */
            m_sockId = -1;   /* cut */
        }

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    if (error)
    {
        observer->OnHandshakeError(
            (IProSslHandshaker*)this, errorCode, sslCode);
    }
    else
    {
        observer->OnHandshakeOk(
            (IProSslHandshaker*)this, ctx, sockId, m_unixSocket, buf, size);
    }

    ProFree(buf);
    observer->Release();
}

void
PRO_CALLTYPE
CProSslHandshaker::OnError(PRO_INT64 sockId,
                           long      errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProSslHandshakerObserver* observer = NULL;
    const int                  sslCode  = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
        {
            return;
        }

        if (sockId != m_sockId)
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

    observer->OnHandshakeError((IProSslHandshaker*)this, errorCode, sslCode);
    observer->Release();
}

void
PRO_CALLTYPE
CProSslHandshaker::OnTimer(unsigned long timerId,
                           PRO_INT64     userData)
{
    assert(timerId > 0);
    if (timerId == 0)
    {
        return;
    }

    IProSslHandshakerObserver* observer  = NULL;
    const int                  errorCode = PBSD_ETIMEDOUT;
    const int                  sslCode   = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
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

    observer->OnHandshakeError((IProSslHandshaker*)this, errorCode, sslCode);
    observer->Release();
}
