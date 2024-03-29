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

#include "test.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_unicode.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance()
{
    return new CTest;
}

CTest::CTest()
{
    m_reactor          = NULL;
    m_sslConfig        = NULL;
    m_timerId          = 0;

    m_heartbeatData[0] = 0;
    m_heartbeatData[1] = 0;
    m_heartbeatSize    = sizeof(uint16_t);
}

CTest::~CTest()
{
    Fini();
}

bool
CTest::Init(IProReactor*                  reactor,
            const TCP_CLIENT_CONFIG_INFO& configInfo)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return false;
    }

    PRO_SSL_CLIENT_CONFIG* sslConfig = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_sslConfig == NULL);
        if (m_reactor != NULL || m_sslConfig != NULL)
        {
            return false;
        }

        if (configInfo.tcpc_enable_ssl)
        {
            CProStlVector<const char*>      caFiles;
            CProStlVector<const char*>      crlFiles;
            CProStlVector<PRO_SSL_SUITE_ID> suites;

            int i = 0;
            int c = (int)configInfo.tcpc_ssl_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.tcpc_ssl_cafiles[i].empty())
                {
                    caFiles.push_back(configInfo.tcpc_ssl_cafiles[i].c_str());
                }
            }

            i = 0;
            c = (int)configInfo.tcpc_ssl_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.tcpc_ssl_crlfiles[i].empty())
                {
                    crlFiles.push_back(configInfo.tcpc_ssl_crlfiles[i].c_str());
                }
            }

            if (configInfo.tcpc_ssl_aes256)
            {
                suites.push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384);
                suites.push_back(PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
                suites.push_back(PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384);
            }
            else
            {
                suites.push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
                suites.push_back(PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
                suites.push_back(PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256);
            }

            if (caFiles.size() > 0)
            {
                sslConfig = ProSslClientConfig_Create();
                if (sslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslClientConfig_EnableSha1Cert(sslConfig, configInfo.tcpc_ssl_enable_sha1cert);

                if (!ProSslClientConfig_SetCaList(
                    sslConfig,
                    &caFiles[0],
                    caFiles.size(),
                    crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                    crlFiles.size()
                    ))
                {
                    goto EXIT;
                }

                if (!ProSslClientConfig_SetSuiteList(sslConfig, &suites[0], suites.size()))
                {
                    goto EXIT;
                }
            }
        }

        m_reactor    = reactor;
        m_configInfo = configInfo;
        m_sslConfig  = sslConfig;
        m_timerId    = reactor->SetupTimer(this, 100, 100); /* 100ms */
    }

    SetHeartbeatDataSize(configInfo.tcpc_heartbeat_bytes);

    return true;

EXIT:

    ProSslClientConfig_Delete(sslConfig);

    return false;
}

void
CTest::Fini()
{
    PRO_SSL_CLIENT_CONFIG*         sslConfig = NULL;
    CProStlSet<IProConnector*>     connectors;
    CProStlSet<IProTcpHandshaker*> tcpHandshakers;
    CProStlSet<IProSslHandshaker*> sslHandshakers;
    CProStlSet<IProTransport*>     transports;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timerId);
        m_timerId = 0;

        transports = m_transports;
        m_transports.clear();
        sslHandshakers = m_sslHandshakers;
        m_sslHandshakers.clear();
        tcpHandshakers = m_tcpHandshakers;
        m_tcpHandshakers.clear();
        connectors = m_connectors;
        m_connectors.clear();
        sslConfig = m_sslConfig;
        m_sslConfig = NULL;
        m_reactor = NULL;
    }

    {
        auto itr = transports.begin();
        auto end = transports.end();

        for (; itr != end; ++itr)
        {
            ProDeleteTransport(*itr);
        }
    }

    {
        auto itr = sslHandshakers.begin();
        auto end = sslHandshakers.end();

        for (; itr != end; ++itr)
        {
            ProDeleteSslHandshaker(*itr);
        }
    }

    {
        auto itr = tcpHandshakers.begin();
        auto end = tcpHandshakers.end();

        for (; itr != end; ++itr)
        {
            ProDeleteTcpHandshaker(*itr);
        }
    }

    {
        auto itr = connectors.begin();
        auto end = connectors.end();

        for (; itr != end; ++itr)
        {
            ProDeleteConnector(*itr);
        }
    }

    ProSslClientConfig_Delete(sslConfig);
}

unsigned long
CTest::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CTest::Release()
{
    return CProRefCount::Release();
}

void
CTest::SetHeartbeatDataSize(unsigned long size) /* 0 ~ 1024 */
{
    if (size < sizeof(uint16_t))
    {
        size = sizeof(uint16_t);
    }
    if (size > sizeof(m_heartbeatData))
    {
        size = sizeof(m_heartbeatData);
    }

    {
        CProThreadMutexGuard mon(m_lock2); /* lock2 */

        m_heartbeatData[0] = pbsd_hton16((uint16_t)(size - sizeof(uint16_t)));
        m_heartbeatSize    = size;
    }
}

unsigned long
CTest::GetHeartbeatDataSize() const
{
    unsigned long size = 0;

    {
        CProThreadMutexGuard mon(m_lock2); /* lock2 */

        size = m_heartbeatSize;
    }

    return size;
}

void
CTest::SendMsg(const char* msg)
{
    assert(msg != NULL);
    assert(msg[0] != '\0');
    if (msg == NULL || msg[0] == '\0')
    {
        return;
    }

#if defined(_WIN32)
    CProStlString tmp;
    ProAnsiToUtf8(msg, tmp);
    msg = tmp.c_str();
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_transports.size() == 0)
        {
            return;
        }

        uint16_t length = (uint16_t)strlen(msg);

        unsigned long size = sizeof(uint16_t) + length;
        char*         buf  = (char*)ProMalloc(size);
        if (buf == NULL)
        {
            return;
        }

        length = pbsd_hton16(length);

        memcpy(buf, &length, sizeof(uint16_t));
        memcpy(buf + sizeof(uint16_t), msg, size - sizeof(uint16_t));

        IProTransport* trans = *m_transports.begin();
        trans->SendData(buf, size);

        ProFree(buf);
    }
}

void
CTest::OnConnectOk(IProConnector*   connector,
                   int64_t          sockId,
                   bool             unixSocket,
                   const char*      remoteIp,
                   unsigned short   remotePort,
                   unsigned char    serviceId,
                   unsigned char    serviceOpt,
                   const PRO_NONCE* nonce)
{
    assert(connector != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(nonce != NULL);
    if (connector == NULL || sockId == -1 || remoteIp == NULL || nonce == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (m_connectors.find(connector) == m_connectors.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        if (!DoHandshake(sockId, unixSocket, *nonce))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        m_connectors.erase(connector);
    }

    if (0)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnConnectOk(server : %s:%u) \n"
            ,
            timeString.c_str(),
            remoteIp,
            (unsigned int)remotePort
            );
    }}}

    ProDeleteConnector(connector);
}

bool
CTest::DoHandshake(int64_t          sockId,
                   bool             unixSocket,
                   const PRO_NONCE& nonce)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return false;
    }

    IProTcpHandshaker* tcpHandshaker = NULL;
    IProSslHandshaker* sslHandshaker = NULL;

    if (m_sslConfig != NULL)
    {
        PRO_SSL_CTX* sslCtx = ProSslCtx_CreateC(
            m_sslConfig, m_configInfo.tcpc_ssl_sni.c_str(), sockId, &nonce);
        if (sslCtx != NULL)
        {
            sslHandshaker = ProCreateSslHandshaker(
                this,
                m_reactor,
                sslCtx,
                sockId,
                unixSocket,
                NULL,  /* sendData */
                0,     /* sendDataSize */
                0,     /* recvDataSize */
                false, /* recvFirst is false */
                m_configInfo.tcpc_handshake_timeout
                );
            if (sslHandshaker == NULL)
            {
                ProSslCtx_Delete(sslCtx);
            }
            else
            {
                m_sslHandshakers.insert(sslHandshaker);
            }
        }
    }
    else
    {
        tcpHandshaker = ProCreateTcpHandshaker(
            this,
            m_reactor,
            sockId,
            unixSocket,
            NULL,  /* sendData */
            0,     /* sendDataSize */
            0,     /* recvDataSize */
            false, /* recvFirst is false */
            m_configInfo.tcpc_handshake_timeout
            );
        if (tcpHandshaker != NULL)
        {
            m_tcpHandshakers.insert(tcpHandshaker);
        }
    }

    return tcpHandshaker != NULL || sslHandshaker != NULL;
}

void
CTest::OnConnectError(IProConnector* connector,
                      const char*    remoteIp,
                      unsigned short remotePort,
                      unsigned char  serviceId,
                      unsigned char  serviceOpt,
                      bool           timeout)
{
    assert(connector != NULL);
    assert(remoteIp != NULL);
    if (connector == NULL || remoteIp == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (m_connectors.find(connector) == m_connectors.end())
        {
            return;
        }

        m_connectors.erase(connector);
    }

    if (1)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnConnectError(server : %s:%u) \n"
            ,
            timeString.c_str(),
            remoteIp,
            (unsigned int)remotePort
            );
    }}}

    ProDeleteConnector(connector);
}

void
CTest::OnHandshakeOk(IProTcpHandshaker* handshaker,
                     int64_t            sockId,
                     bool               unixSocket,
                     const void*        buf,
                     size_t             size)
{
    assert(handshaker != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || sockId == -1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (m_tcpHandshakers.find(handshaker) == m_tcpHandshakers.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        assert(m_sslConfig == NULL);

        IProTransport* trans = ProCreateTcpTransport(
            this,
            m_reactor,
            sockId,
            unixSocket,
            m_configInfo.tcpc_sockbuf_size_recv,
            m_configInfo.tcpc_sockbuf_size_send,
            m_configInfo.tcpc_recvpool_size
            );
        if (trans == NULL)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
        else
        {
            trans->StartHeartbeat();
            m_transports.insert(trans);
        }

        m_tcpHandshakers.erase(handshaker);
    }

    if (0)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnHandshakeOk() [tcp] \n"
            ,
            timeString.c_str()
            );
    }}}

    ProDeleteTcpHandshaker(handshaker);
}

void
CTest::OnHandshakeError(IProTcpHandshaker* handshaker,
                        int                errorCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (m_tcpHandshakers.find(handshaker) == m_tcpHandshakers.end())
        {
            return;
        }

        m_tcpHandshakers.erase(handshaker);
    }

    if (1)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnHandshakeError(errorCode : %d) [tcp] \n"
            ,
            timeString.c_str(),
            errorCode
            );
    }}}

    ProDeleteTcpHandshaker(handshaker);
}

void
CTest::OnHandshakeOk(IProSslHandshaker* handshaker,
                     PRO_SSL_CTX*       ctx,
                     int64_t            sockId,
                     bool               unixSocket,
                     const void*        buf,
                     size_t             size)
{
    assert(handshaker != NULL);
    assert(ctx != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || ctx == NULL || sockId == -1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        if (m_sslHandshakers.find(handshaker) == m_sslHandshakers.end())
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        assert(m_sslConfig != NULL);

        IProTransport* trans = ProCreateSslTransport(
            this,
            m_reactor,
            ctx,
            sockId,
            unixSocket,
            m_configInfo.tcpc_sockbuf_size_recv,
            m_configInfo.tcpc_sockbuf_size_send,
            m_configInfo.tcpc_recvpool_size
            );
        if (trans == NULL)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);
            ctx    = NULL;
            sockId = -1;
        }
        else
        {
            trans->StartHeartbeat();
            m_transports.insert(trans);
        }

        m_sslHandshakers.erase(handshaker);
    }

    if (0)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnHandshakeOk() [ssl] \n"
            ,
            timeString.c_str()
            );
    }}}

    ProDeleteSslHandshaker(handshaker);
}

void
CTest::OnHandshakeError(IProSslHandshaker* handshaker,
                        int                errorCode,
                        int                sslCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (m_sslHandshakers.find(handshaker) == m_sslHandshakers.end())
        {
            return;
        }

        m_sslHandshakers.erase(handshaker);
    }

    if (1)
    {{{
        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnHandshakeError(errorCode : [%d, %d]) [ssl] \n"
            ,
            timeString.c_str(),
            errorCode,
            sslCode
            );
    }}}

    ProDeleteSslHandshaker(handshaker);
}

void
CTest::OnRecv(IProTransport*          trans,
              const pbsd_sockaddr_in* remoteAddr)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    while (1)
    {
        char* buf = NULL;

        {
#if 0
            CProThreadMutexGuard mon(m_lock);

            if (m_reactor == NULL)
            {
                return;
            }

            if (m_transports.find(trans) == m_transports.end())
            {
                return;
            }
#endif
            IProRecvPool& recvPool = *trans->GetRecvPool();
            size_t        dataSize = recvPool.PeekDataSize();

            if (dataSize < sizeof(uint16_t))
            {
                break;
            }

            uint16_t length = 0;
            recvPool.PeekData(&length, sizeof(uint16_t));
            length = pbsd_ntoh16(length);
            if (dataSize < sizeof(uint16_t) + length) /* 2 + ... */
            {
                break;
            }

            recvPool.Flush(sizeof(uint16_t));

            /*
             * a haeartbeat packet
             */
            if (length == 0)
            {
                continue;
            }

            buf = (char*)ProMalloc(length + 1);
            if (buf == NULL)
            {
                break;
            }

            buf[length] = '\0';

            recvPool.PeekData(buf, length);
            recvPool.Flush(length);

            /*
             * a packet for testing purposes
             */
            if (buf[sizeof(uint16_t)] == '\0')
            {
                ProFree(buf);
                continue;
            }
        }

        if (1)
        {{{
            char localIp[64]  = "";
            char remoteIp[64] = "";
            trans->GetLocalIp(localIp);
            trans->GetRemoteIp(remoteIp);

            CProStlString timeString;
            ProGetLocalTimeString(timeString);

            printf(
                "\n"
                "%s \n"
                " CTest::OnRecv(server : %s:%u, me : %s:%u) \n"
                "\t %s \n"
                ,
                timeString.c_str(),
                remoteIp,
                (unsigned int)trans->GetRemotePort(),
                localIp,
                (unsigned int)trans->GetLocalPort(),
                buf
                );
        }}}

        ProFree(buf);
    } /* end of while () */
}

void
CTest::OnClose(IProTransport* trans,
               int            errorCode,
               int            sslCode)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (m_transports.find(trans) == m_transports.end())
        {
            return;
        }

        m_transports.erase(trans);
    }

    if (1)
    {{{
        char localIp[64]  = "";
        char remoteIp[64] = "";
        trans->GetLocalIp(localIp);
        trans->GetRemoteIp(remoteIp);

        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnClose(errorCode : [%d, %d], server : %s:%u, me : %s:%u) \n"
            ,
            timeString.c_str(),
            errorCode,
            sslCode,
            remoteIp,
            (unsigned int)trans->GetRemotePort(),
            localIp,
            (unsigned int)trans->GetLocalPort()
            );
    }}}

    ProDeleteTransport(trans);
}

void
CTest::OnHeartbeat(IProTransport* trans)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock2); /* lock2 */

        trans->SendData(m_heartbeatData, m_heartbeatSize);
    }
}

void
CTest::OnTimer(void*    factory,
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

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (timerId != m_timerId)
        {
            return;
        }

        while (1)
        {
            if (m_connectors.size() + m_tcpHandshakers.size() +
                m_sslHandshakers.size() >= m_configInfo.tcpc_max_pending_count)
            {
                break;
            }

            if (m_connectors.size() + m_tcpHandshakers.size() +
                m_sslHandshakers.size() + m_transports.size() >=
                m_configInfo.tcpc_connection_count)
            {
                break;
            }

            IProConnector* connector = ProCreateConnectorEx(
                true,                /* enable unixSocket */
                1,                   /* serviceId.  [0 for ipc-pipe, !0 for media-link] */
                m_sslConfig != NULL, /* serviceOpt. [0 for tcp     , !0 for ssl] */
                this,
                m_reactor,
                m_configInfo.tcpc_server_ip.c_str(),
                m_configInfo.tcpc_server_port,
                m_configInfo.tcpc_local_ip.c_str(),
                m_configInfo.tcpc_handshake_timeout
                );
            if (connector == NULL)
            {
                break;
            }

            m_connectors.insert(connector);
        } /* end of while () */
    }
}
