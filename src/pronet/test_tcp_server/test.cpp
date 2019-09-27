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
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance()
{
    CTest* const tester = new CTest;

    return (tester);
}

CTest::CTest()
{
    m_reactor          = NULL;
    m_sslConfig        = NULL;
    m_acceptor         = NULL;
    m_service          = NULL;

    m_heartbeatData[0] = 0;
    m_heartbeatData[1] = 0;
    m_heartbeatSize    = sizeof(PRO_UINT16);
}

CTest::~CTest()
{
    Fini();
}

bool
CTest::Init(IProReactor*                  reactor,
            const TCP_SERVER_CONFIG_INFO& configInfo)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (false);
    }

    PRO_SSL_SERVER_CONFIG* sslConfig = NULL;
    IProAcceptor*          acceptor  = NULL;
    IProServiceHost*       service   = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_sslConfig == NULL);
        assert(m_acceptor == NULL);
        assert(m_service == NULL);
        if (m_reactor != NULL || m_sslConfig != NULL || m_acceptor != NULL ||
            m_service != NULL)
        {
            return (false);
        }

        if (configInfo.tcps_enable_ssl)
        {
            CProStlVector<const char*> caFiles;
            CProStlVector<const char*> crlFiles;
            CProStlVector<const char*> certFiles;

            int i = 0;
            int c = (int)configInfo.tcps_ssl_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.tcps_ssl_cafiles[i].empty())
                {
                    caFiles.push_back(&configInfo.tcps_ssl_cafiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.tcps_ssl_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.tcps_ssl_crlfiles[i].empty())
                {
                    crlFiles.push_back(&configInfo.tcps_ssl_crlfiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.tcps_ssl_certfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.tcps_ssl_certfiles[i].empty())
                {
                    certFiles.push_back(&configInfo.tcps_ssl_certfiles[i][0]);
                }
            }

            if (caFiles.size() > 0 && certFiles.size() > 0)
            {
                sslConfig = ProSslServerConfig_Create();
                if (sslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslServerConfig_EnableSha1Cert(
                    sslConfig, configInfo.tcps_ssl_enable_sha1cert);

                if (!ProSslServerConfig_SetCaList(
                    sslConfig,
                    &caFiles[0],
                    caFiles.size(),
                    crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                    crlFiles.size()
                    ))
                {
                    goto EXIT;
                }

                if (!ProSslServerConfig_AppendCertChain(
                    sslConfig,
                    &certFiles[0],
                    certFiles.size(),
                    configInfo.tcps_ssl_keyfile.c_str(),
                    NULL /* password to decrypt the keyfile */
                    ))
                {
                    goto EXIT;
                }
            }
        }

        if (configInfo.tcps_using_hub)
        {
            service  = ProCreateServiceHost(
                this, reactor, configInfo.tcps_port, 1); /* serviceId is 1 */
        }
        else
        {
            acceptor = ProCreateAcceptorEx(this, reactor, "0.0.0.0",
                configInfo.tcps_port, configInfo.tcps_handshake_timeout);
        }
        if (acceptor == NULL && service == NULL)
        {
            goto EXIT;
        }

        m_reactor    = reactor;
        m_configInfo = configInfo;
        m_sslConfig  = sslConfig;
        m_acceptor   = acceptor;
        m_service    = service;
    }

    SetHeartbeatDataSize(configInfo.tcps_heartbeat_bytes);

    return (true);

EXIT:

    ProDeleteServiceHost(service);
    ProDeleteAcceptor(acceptor);
    ProSslServerConfig_Delete(sslConfig);

    return (false);
}

void
CTest::Fini()
{
    PRO_SSL_SERVER_CONFIG*                     sslConfig = NULL;
    IProAcceptor*                              acceptor  = NULL;
    IProServiceHost*                           service   = NULL;
    CProStlMap<IProTcpHandshaker*, PRO_UINT64> tcpHandshaker2Nonce;
    CProStlMap<IProSslHandshaker*, PRO_UINT64> sslHandshaker2Nonce;
    CProStlSet<IProTransport*>                 transports;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        transports = m_transports;
        m_transports.clear();
        sslHandshaker2Nonce = m_sslHandshaker2Nonce;
        m_sslHandshaker2Nonce.clear();
        tcpHandshaker2Nonce = m_tcpHandshaker2Nonce;
        m_tcpHandshaker2Nonce.clear();
        service = m_service;
        m_service = NULL;
        acceptor = m_acceptor;
        m_acceptor = NULL;
        sslConfig = m_sslConfig;
        m_sslConfig = NULL;
        m_reactor = NULL;
    }

    {
        CProStlSet<IProTransport*>::const_iterator       itr = transports.begin();
        CProStlSet<IProTransport*>::const_iterator const end = transports.end();

        for (; itr != end; ++itr)
        {
            ProDeleteTransport(*itr);
        }
    }

    {
        CProStlMap<IProSslHandshaker*, PRO_UINT64>::const_iterator       itr = sslHandshaker2Nonce.begin();
        CProStlMap<IProSslHandshaker*, PRO_UINT64>::const_iterator const end = sslHandshaker2Nonce.end();

        for (; itr != end; ++itr)
        {
            ProDeleteSslHandshaker(itr->first);
        }
    }

    {
        CProStlMap<IProTcpHandshaker*, PRO_UINT64>::const_iterator       itr = tcpHandshaker2Nonce.begin();
        CProStlMap<IProTcpHandshaker*, PRO_UINT64>::const_iterator const end = tcpHandshaker2Nonce.end();

        for (; itr != end; ++itr)
        {
            ProDeleteTcpHandshaker(itr->first);
        }
    }

    ProDeleteServiceHost(service);
    ProDeleteAcceptor(acceptor);
    ProSslServerConfig_Delete(sslConfig);
}

unsigned long
PRO_CALLTYPE
CTest::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CTest::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CTest::SetHeartbeatDataSize(unsigned long size) /* 0 ~ 1024 */
{
    if (size < sizeof(PRO_UINT16))
    {
        size = sizeof(PRO_UINT16);
    }
    if (size > sizeof(m_heartbeatData))
    {
        size = sizeof(m_heartbeatData);
    }

    {
        CProThreadMutexGuard mon(m_lock2); /* lock2 */

        m_heartbeatData[0] = pbsd_hton16((PRO_UINT16)(size - sizeof(PRO_UINT16)));
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

    return (size);
}

unsigned long
CTest::GetTransportCount() const
{
    unsigned long count = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        count = (unsigned long)m_transports.size();
    }

    return (count);
}

void
PRO_CALLTYPE
CTest::OnAccept(IProAcceptor*  acceptor,
                PRO_INT64      sockId,
                bool           unixSocket,
                const char*    remoteIp,
                unsigned short remotePort,
                unsigned char  serviceId,
                unsigned char  serviceOpt,
                PRO_UINT64     nonce)
{
    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    if (acceptor == NULL || sockId == -1 || remoteIp == NULL)
    {
        return;
    }

    if (serviceId == 0)
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

        if (serviceOpt == 0)
        {
            IProTcpHandshaker* const handshaker = ProCreateTcpHandshaker(
                this,
                m_reactor,
                sockId,
                unixSocket,
                NULL,  /* sendData */
                0,     /* sendDataSize */
                0,     /* recvDataSize */
                false, /* recvFirst is false */
                m_configInfo.tcps_handshake_timeout
                );
            if (handshaker == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            m_tcpHandshaker2Nonce[handshaker] = nonce;
        }
        else
        {
            if (m_sslConfig == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            PRO_SSL_CTX* const sslCtx =
                ProSslCtx_Creates(m_sslConfig, sockId, nonce);
            if (sslCtx == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            IProSslHandshaker* const handshaker = ProCreateSslHandshaker(
                this,
                m_reactor,
                sslCtx,
                sockId,
                unixSocket,
                NULL,  /* sendData */
                0,     /* sendDataSize */
                0,     /* recvDataSize */
                false, /* recvFirst is false */
                m_configInfo.tcps_handshake_timeout
                );
            if (handshaker == NULL)
            {
                ProSslCtx_Delete(sslCtx);
                ProCloseSockId(sockId);

                return;
            }

            m_sslHandshaker2Nonce[handshaker] = nonce;
        }
    }
}

void
PRO_CALLTYPE
CTest::OnServiceAccept(IProServiceHost* serviceHost,
                       PRO_INT64        sockId,
                       bool             unixSocket,
                       const char*      remoteIp,
                       unsigned short   remotePort,
                       unsigned char    serviceId,
                       unsigned char    serviceOpt,
                       PRO_UINT64       nonce)
{
    assert(serviceHost != NULL);
    assert(sockId != -1);
    assert(remoteIp != NULL);
    assert(serviceId > 0);
    if (serviceHost == NULL || sockId == -1 || remoteIp == NULL ||
        serviceId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_service == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (serviceHost != (IProServiceHost*)m_service)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (serviceOpt == 0)
        {
            IProTcpHandshaker* const handshaker = ProCreateTcpHandshaker(
                this,
                m_reactor,
                sockId,
                unixSocket,
                NULL,  /* sendData */
                0,     /* sendDataSize */
                0,     /* recvDataSize */
                false, /* recvFirst is false */
                m_configInfo.tcps_handshake_timeout
                );
            if (handshaker == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            m_tcpHandshaker2Nonce[handshaker] = nonce;
        }
        else
        {
            if (m_sslConfig == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            PRO_SSL_CTX* const sslCtx =
                ProSslCtx_Creates(m_sslConfig, sockId, nonce);
            if (sslCtx == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            IProSslHandshaker* const handshaker = ProCreateSslHandshaker(
                this,
                m_reactor,
                sslCtx,
                sockId,
                unixSocket,
                NULL,  /* sendData */
                0,     /* sendDataSize */
                0,     /* recvDataSize */
                false, /* recvFirst is false */
                m_configInfo.tcps_handshake_timeout
                );
            if (handshaker == NULL)
            {
                ProSslCtx_Delete(sslCtx);
                ProCloseSockId(sockId);

                return;
            }

            m_sslHandshaker2Nonce[handshaker] = nonce;
        }
    }
}

void
PRO_CALLTYPE
CTest::OnHandshakeOk(IProTcpHandshaker* handshaker,
                     PRO_INT64          sockId,
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

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        CProStlMap<IProTcpHandshaker*, PRO_UINT64>::iterator const itr =
            m_tcpHandshaker2Nonce.find(handshaker);
        if (itr == m_tcpHandshaker2Nonce.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        IProTransport* const trans = ProCreateTcpTransport(
            this,
            m_reactor,
            sockId,
            unixSocket,
            m_configInfo.tcps_sockbuf_size_recv,
            m_configInfo.tcps_sockbuf_size_send,
            m_configInfo.tcps_recvpool_size
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

        m_tcpHandshaker2Nonce.erase(itr);
    }

    ProDeleteTcpHandshaker(handshaker);
}

void
PRO_CALLTYPE
CTest::OnHandshakeError(IProTcpHandshaker* handshaker,
                        long               errorCode)
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

        CProStlMap<IProTcpHandshaker*, PRO_UINT64>::iterator const itr =
            m_tcpHandshaker2Nonce.find(handshaker);
        if (itr == m_tcpHandshaker2Nonce.end())
        {
            return;
        }

        m_tcpHandshaker2Nonce.erase(itr);
    }

    ProDeleteTcpHandshaker(handshaker);
}

void
PRO_CALLTYPE
CTest::OnHandshakeOk(IProSslHandshaker* handshaker,
                     PRO_SSL_CTX*       ctx,
                     PRO_INT64          sockId,
                     bool               unixSocket,
                     const void*        buf,
                     unsigned long      size)
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

        CProStlMap<IProSslHandshaker*, PRO_UINT64>::iterator const itr =
            m_sslHandshaker2Nonce.find(handshaker);
        if (itr == m_sslHandshaker2Nonce.end())
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        IProTransport* const trans = ProCreateSslTransport(
            this,
            m_reactor,
            ctx,
            sockId,
            unixSocket,
            m_configInfo.tcps_sockbuf_size_recv,
            m_configInfo.tcps_sockbuf_size_send,
            m_configInfo.tcps_recvpool_size
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

        m_sslHandshaker2Nonce.erase(itr);
    }

    ProDeleteSslHandshaker(handshaker);
}

void
PRO_CALLTYPE
CTest::OnHandshakeError(IProSslHandshaker* handshaker,
                        long               errorCode,
                        long               sslCode)
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

        CProStlMap<IProSslHandshaker*, PRO_UINT64>::iterator const itr =
            m_sslHandshaker2Nonce.find(handshaker);
        if (itr == m_sslHandshaker2Nonce.end())
        {
            return;
        }

        m_sslHandshaker2Nonce.erase(itr);
    }

    ProDeleteSslHandshaker(handshaker);
}

void
PRO_CALLTYPE
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
        char*         buf  = NULL;
        unsigned long size = 0;

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
            IProRecvPool&       recvPool = *trans->GetRecvPool();
            const unsigned long dataSize = recvPool.PeekDataSize();

            if (dataSize < sizeof(PRO_UINT16))
            {
                break;
            }

            PRO_UINT16 length = 0;
            recvPool.PeekData(&length, sizeof(PRO_UINT16));
            length = pbsd_ntoh16(length);
            if (dataSize < sizeof(PRO_UINT16) + length) /* 2 + ... */
            {
                break;
            }

            recvPool.Flush(sizeof(PRO_UINT16));

            /*
             * a haeartbeat packet
             */
            if (length == 0)
            {
                continue;
            }

            size = sizeof(PRO_UINT16) + length;
            buf  = (char*)ProMalloc(size + 1);
            if (buf == NULL)
            {
                break;
            }

            buf[size] = '\0';

            recvPool.PeekData(buf + sizeof(PRO_UINT16), length);
            recvPool.Flush(length);
            length = pbsd_hton16(length);
            memcpy(buf, &length, sizeof(PRO_UINT16));

            /*
             * a packet for testing purposes
             */
            if (buf[sizeof(PRO_UINT16)] == '\0')
            {
                ProFree(buf);
                continue;
            }
        }

        if (1)
        {{{
            char remoteIp[64] = "";
            trans->GetRemoteIp(remoteIp);

            printf(
                "\n"
                " CTest::OnRecv(peer : %s:%u) \n"
                "\t %s \n"
                ,
                remoteIp,
                (unsigned int)trans->GetRemotePort(),
                buf + sizeof(PRO_UINT16)
                );
        }}}

        trans->SendData(buf, size); /* response */

        ProFree(buf);
    } /* end of while (...) */
}

void
PRO_CALLTYPE
CTest::OnClose(IProTransport* trans,
               long           errorCode,
               long           sslCode)
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

    if (0)
    {{{
        char remoteIp[64] = "";
        trans->GetRemoteIp(remoteIp);

        printf(
            "\n CTest::OnClose(errorCode : [%d, %d], peer : %s:%u) \n"
            ,
            (int)errorCode,
            (int)sslCode,
            remoteIp,
            (unsigned int)trans->GetRemotePort()
            );
    }}}

    ProDeleteTransport(trans);
}

void
PRO_CALLTYPE
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
