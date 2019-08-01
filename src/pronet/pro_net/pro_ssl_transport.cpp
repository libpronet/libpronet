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

#include "pro_ssl_transport.h"
#include "pro_net.h"
#include "pro_tcp_transport.h"
#include "pro_tp_reactor_task.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_z.h"

#include "mbedtls/ssl.h"

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_RECV_BUF_SIZE (1024 * 56)
#define DEFAULT_SEND_BUF_SIZE (1024 * 8)

/////////////////////////////////////////////////////////////////////////////
////

CProSslTransport*
CProSslTransport::CreateInstance(size_t recvPoolSize)   /* = 0 */
{
    CProSslTransport* const trans = new CProSslTransport(recvPoolSize);

    return (trans);
}

CProSslTransport::CProSslTransport(size_t recvPoolSize) /* = 0 */
: CProTcpTransport(false, recvPoolSize)
{
    m_ctx     = NULL;
    m_suiteId = PRO_SSL_SUITE_NONE;

    strcpy(m_suiteName, "NONE");
}

CProSslTransport::~CProSslTransport()
{
    Fini();

    ProSslCtx_Delete(m_ctx);
    ProCloseSockId(m_sockId, true);
    m_ctx    = NULL;
    m_sockId = -1;
}

bool
CProSslTransport::Init(IProTransportObserver* observer,
                       CProTpReactorTask*     reactorTask,
                       PRO_SSL_CTX*           ctx,
                       PRO_INT64              sockId,
                       bool                   unixSocket,
                       size_t                 sockBufSizeRecv, /* = 0 */
                       size_t                 sockBufSizeSend) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(ctx != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactorTask == NULL || ctx == NULL || sockId == -1)
    {
        return (false);
    }

    assert(((mbedtls_ssl_context*)ctx)->state == MBEDTLS_SSL_HANDSHAKE_OVER);
    if (((mbedtls_ssl_context*)ctx)->state != MBEDTLS_SSL_HANDSHAKE_OVER)
    {
        return (false);
    }

    if (sockBufSizeRecv == 0)
    {
        sockBufSizeRecv = DEFAULT_RECV_BUF_SIZE;
    }
    if (sockBufSizeSend == 0)
    {
        sockBufSizeSend = DEFAULT_SEND_BUF_SIZE;
    }

    int option;
    option = (int)sockBufSizeRecv;
    pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
    option = (int)sockBufSizeSend;
    pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        assert(m_ctx == NULL);
        if (m_observer != NULL || m_reactorTask != NULL || m_ctx != NULL)
        {
            return (false);
        }

#if !defined(WIN32) && !defined(_WIN32_WCE)
        if (unixSocket)
        {
            pbsd_sockaddr_un localAddrUn;
            pbsd_sockaddr_un remoteAddrUn;
            if (pbsd_getsockname_un(sockId, &localAddrUn)  != 0 ||
                pbsd_getpeername_un(sockId, &remoteAddrUn) != 0)
            {
                return (false);
            }

            const char* const prefix = "/tmp/libpronet_127001_";
            const char* const local  = strstr(localAddrUn.sun_path , prefix);
            const char* const remote = strstr(remoteAddrUn.sun_path, prefix);
            if (local == NULL && remote == NULL)
            {
                return (false);
            }

            if (local != NULL)
            {
                int port = 0;
                sscanf(local + strlen(prefix), "%d", &port);
                if (port <= 0 || port > 65535)
                {
                    return (false);
                }

                m_localAddr.sin_port         = pbsd_hton16((PRO_UINT16)port);
                m_localAddr.sin_addr.s_addr  = pbsd_inet_aton("127.0.0.1");
                m_remoteAddr.sin_port        = pbsd_hton16((PRO_UINT16)65535); /* a dummy for unix socket */
                m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton("127.0.0.1");
            }
            else
            {
                int port = 0;
                sscanf(remote + strlen(prefix), "%d", &port);
                if (port <= 0 || port > 65535)
                {
                    return (false);
                }

                m_localAddr.sin_port         = pbsd_hton16((PRO_UINT16)65535); /* a dummy for unix socket */
                m_localAddr.sin_addr.s_addr  = pbsd_inet_aton("127.0.0.1");
                m_remoteAddr.sin_port        = pbsd_hton16((PRO_UINT16)port);
                m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton("127.0.0.1");
            }
        }
        else
#endif
        {
            if (pbsd_getsockname(sockId, &m_localAddr)  != 0 ||
                pbsd_getpeername(sockId, &m_remoteAddr) != 0)
            {
                return (false);
            }
        }

        if (!m_recvPool.Resize(m_recvPoolSize))
        {
            return (false);
        }

        if (!reactorTask->AddHandler(sockId, this, PRO_MASK_WRITE | PRO_MASK_READ))
        {
            return (false);
        }

        observer->AddRef();
        m_observer    = observer;
        m_reactorTask = reactorTask;
        m_ctx         = ctx;
        m_suiteId     = ProSslCtx_GetSuite(ctx, m_suiteName);
        m_sockId      = sockId;
        m_onWr        = true;
    }

    return (true);
}

void
CProSslTransport::Fini()
{
    IProTransportObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_ctx != NULL && m_sockId != -1)
        {
            mbedtls_ssl_close_notify((mbedtls_ssl_context*)m_ctx);
        }

        if (m_observer == NULL || m_reactorTask == NULL || m_ctx == NULL)
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

PRO_TRANS_TYPE
PRO_CALLTYPE
CProSslTransport::GetType() const
{
    return (PRO_TRANS_SSL);
}

PRO_SSL_SUITE_ID
PRO_CALLTYPE
CProSslTransport::GetSslSuite(char suiteName[64]) const
{
    PRO_SSL_SUITE_ID suiteId = PRO_SSL_SUITE_NONE;

    {
        CProThreadMutexGuard mon(m_lock);

        suiteId = m_suiteId;
        strncpy_pro(suiteName, 64, m_suiteName);
    }

    return (suiteId);
}

void
PRO_CALLTYPE
CProSslTransport::OnInput(PRO_INT64 sockId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    DoRecv(sockId);
}}

void
PRO_CALLTYPE
CProSslTransport::OnOutput(PRO_INT64 sockId)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    DoSend(sockId);
    DoRecv(sockId); /* !!! */
}}

void
CProSslTransport::DoRecv(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    size_t msgSize = 0;

    do
    {
        IProTransportObserver* observer  = NULL;
        int                    recvSize  = 0;
        int                    errorCode = 0;
        int                    sslCode   = 0;
        bool                   error     = false;
        bool                   regWr     = false;

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

            const size_t idleSize = m_recvPool.ContinuousIdleSize();
            const size_t minSize  = (msgSize == 0 || msgSize > idleSize) ? idleSize : msgSize;

            assert(idleSize > 0);
            if (idleSize == 0)
            {
                error     = true;
                errorCode = -1;

                goto EXIT;
            }

            recvSize = mbedtls_ssl_read((mbedtls_ssl_context*)m_ctx,
                (unsigned char*)m_recvPool.ContinuousIdleBuf(), minSize);
            assert(recvSize <= (int)minSize);

            if (recvSize > (int)minSize)
            {
                error     = true;
                errorCode = -1;
                sslCode   = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
            }
            else if (recvSize > 0)
            {
                m_recvPool.Fill(recvSize);
                msgSize = mbedtls_ssl_get_bytes_avail((mbedtls_ssl_context*)m_ctx); /* remaining message */
            }
            else if (recvSize == 0)
            {
                error = true;
            }
            else if (recvSize == MBEDTLS_ERR_SSL_WANT_READ)
            {
                msgSize = 0;
            }
            else if (recvSize == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                regWr   = true;
                msgSize = 0;
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = recvSize;
            }

            if (!error && !m_onWr && (regWr || m_pendingWr || m_requestOnSend))
            {
                if (m_reactorTask->AddHandler(m_sockId, this, PRO_MASK_WRITE)) /* !!! */
                {
                    m_onWr = true;
                }
                else
                {
                    error     = true;
                    errorCode = -1;
                }
            }

EXIT:

            m_observer->AddRef();
            observer = m_observer;
        }

        if (m_canUpcall)
        {
            if (error)
            {
                m_canUpcall = false;
                observer->OnClose(this, errorCode, sslCode);
            }
            else if (recvSize > 0)
            {
                observer->OnRecv(this, &m_remoteAddr);
                assert(m_recvPool.ContinuousIdleSize() > 0);
            }
            else
            {
            }
        }

        observer->Release();

        if (!m_canUpcall)
        {
            Fini();
            break;
        }
    }
    while (msgSize > 0); /* read a complete ssl record */
}

void
CProSslTransport::DoSend(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProTransportObserver* observer      = NULL;
    int                    sentSize      = 0;
    int                    errorCode     = 0;
    int                    sslCode       = 0;
    bool                   error         = false;
    const CProBuffer*      onSendBuf     = NULL;
    bool                   requestOnSend = false;
    PRO_UINT64             actionId      = 0;

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

        unsigned long     theSize = 0;
        const void* const theBuf  = m_sendPool.PreSend(theSize);

        if (theBuf == NULL || theSize == 0)
        {
            m_pendingWr = false;

            if (!m_requestOnSend)
            {
                if (m_onWr)
                {
                    m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
                    m_onWr = false;
                }

                return;
            }
        }
        else
        {
            sentSize = mbedtls_ssl_write(
                (mbedtls_ssl_context*)m_ctx, (unsigned char*)theBuf, theSize);
            assert(sentSize <= (int)theSize);

            if (sentSize > (int)theSize)
            {
                error     = true;
                errorCode = -1;
                sslCode   = MBEDTLS_ERR_SSL_INTERNAL_ERROR;
            }
            else if (sentSize > 0)
            {
                m_sendPool.Flush(sentSize);

                onSendBuf = m_sendPool.OnSendBuf();
                if (onSendBuf != NULL)
                {
                    m_sendPool.PostSend();
                    m_pendingWr = false;
                }
            }
            else if (sentSize == 0 || sentSize == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
            }
            else if (sentSize == MBEDTLS_ERR_SSL_WANT_READ)
            {
                if (m_onWr)
                {
                    m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
                    m_onWr = false;
                }
            }
            else
            {
                error     = true;
                errorCode = -1;
                sslCode   = sentSize;
            }
        }

        requestOnSend = m_requestOnSend;
        m_requestOnSend = false;
        if (onSendBuf != NULL)
        {
            actionId = onSendBuf->GetMagic();
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (error)
        {
            m_canUpcall = false;
            observer->OnClose(this, errorCode, sslCode);
        }
        else if (onSendBuf != NULL || requestOnSend)
        {
            observer->OnSend(this, actionId);

            {
                CProThreadMutexGuard mon(m_lock);

                if (m_observer != NULL && m_reactorTask != NULL && m_ctx != NULL)
                {
                    if (m_onWr && !m_pendingWr && !m_requestOnSend)
                    {
                        m_reactorTask->RemoveHandler(m_sockId, this, PRO_MASK_WRITE);
                        m_onWr = false;
                    }
                }
            }
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
