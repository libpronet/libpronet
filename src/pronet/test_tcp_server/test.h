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

#if !defined(TEST_H)
#define TEST_H

#include "../pro_net/pro_net.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

struct TCP_SERVER_CONFIG_INFO
{
    TCP_SERVER_CONFIG_INFO()
    {
        tcps_thread_count        = 40;
        tcps_using_hub           = false;
        tcps_port                = 3000;
        tcps_handshake_timeout   = 20;
        tcps_heartbeat_interval  = 200;
        tcps_heartbeat_bytes     = 0;
        tcps_sockbuf_size_recv   = 0;
        tcps_sockbuf_size_send   = 0;
        tcps_recvpool_size       = 0;

        tcps_enable_ssl          = true;
        tcps_ssl_enable_sha1cert = true;
        tcps_ssl_keyfile         = "./server.key";

        tcps_ssl_cafiles.push_back("./ca.crt");
        tcps_ssl_cafiles.push_back("");
        tcps_ssl_crlfiles.push_back("");
        tcps_ssl_crlfiles.push_back("");
        tcps_ssl_certfiles.push_back("./server.crt");
        tcps_ssl_certfiles.push_back("");
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint("tcps_thread_count"       , tcps_thread_count);
        configStream.AddInt ("tcps_using_hub"          , tcps_using_hub);
        configStream.AddUint("tcps_port"               , tcps_port);
        configStream.AddUint("tcps_handshake_timeout"  , tcps_handshake_timeout);
        configStream.AddUint("tcps_heartbeat_interval" , tcps_heartbeat_interval);
        configStream.AddUint("tcps_heartbeat_bytes"    , tcps_heartbeat_bytes);
        configStream.AddUint("tcps_sockbuf_size_recv"  , tcps_sockbuf_size_recv);
        configStream.AddUint("tcps_sockbuf_size_send"  , tcps_sockbuf_size_send);
        configStream.AddUint("tcps_recvpool_size"      , tcps_recvpool_size);

        configStream.AddInt ("tcps_enable_ssl"         , tcps_enable_ssl);
        configStream.AddInt ("tcps_ssl_enable_sha1cert", tcps_ssl_enable_sha1cert);
        configStream.Add    ("tcps_ssl_cafile"         , tcps_ssl_cafiles);
        configStream.Add    ("tcps_ssl_crlfile"        , tcps_ssl_crlfiles);
        configStream.Add    ("tcps_ssl_certfile"       , tcps_ssl_certfiles);
        configStream.Add    ("tcps_ssl_keyfile"        , tcps_ssl_keyfile);

        configStream.Get(configs);
    }

    unsigned int                 tcps_thread_count;      /* 1 ~ 100 */
    bool                         tcps_using_hub;
    unsigned short               tcps_port;
    unsigned int                 tcps_handshake_timeout;
    unsigned int                 tcps_heartbeat_interval;
    unsigned int                 tcps_heartbeat_bytes;   /* 0 ~ 1024 */
    unsigned int                 tcps_sockbuf_size_recv; /* 0 or >= 1024 */
    unsigned int                 tcps_sockbuf_size_send; /* 0 or >= 1024 */
    unsigned int                 tcps_recvpool_size;     /* 0 or >= 1024 */

    bool                         tcps_enable_ssl;
    bool                         tcps_ssl_enable_sha1cert;
    CProStlVector<CProStlString> tcps_ssl_cafiles;
    CProStlVector<CProStlString> tcps_ssl_crlfiles;
    CProStlVector<CProStlString> tcps_ssl_certfiles;
    CProStlString                tcps_ssl_keyfile;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CTest
:
public IProAcceptorObserver,
public IProServiceHostObserver,
public IProTcpHandshakerObserver,
public IProSslHandshakerObserver,
public IProTransportObserver,
public CProRefCount
{
public:

    static CTest* CreateInstance();

    bool Init(
        IProReactor*                  reactor,
        const TCP_SERVER_CONFIG_INFO& configInfo
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    void SetHeartbeatDataSize(unsigned long size); /* 0 ~ 1024 */

    unsigned long GetHeartbeatDataSize() const;

    unsigned long GetTransportCount() const;

private:

    CTest();

    virtual ~CTest();

    virtual void OnAccept(
        IProAcceptor*  acceptor,
        PRO_INT64      sockId,
        bool           unixSocket,
        const char*    localIp,
        const char*    remoteIp,
        unsigned short remotePort
        )
    {
    }

    virtual void OnAccept(
        IProAcceptor*    acceptor,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort
        )
    {
    }

    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode
        );

    virtual void OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,
        long               sslCode
        );

    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        )
    {
    }

    virtual void OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void OnHeartbeat(IProTransport* trans);

private:

    IProReactor*                              m_reactor;
    TCP_SERVER_CONFIG_INFO                    m_configInfo;
    PRO_SSL_SERVER_CONFIG*                    m_sslConfig;
    IProAcceptor*                             m_acceptor;
    IProServiceHost*                          m_service;
    CProStlMap<IProTcpHandshaker*, PRO_NONCE> m_tcpHandshaker2Nonce;
    CProStlMap<IProSslHandshaker*, PRO_NONCE> m_sslHandshaker2Nonce;
    CProStlSet<IProTransport*>                m_transports;
    mutable CProThreadMutex                   m_lock;

    PRO_UINT16                                m_heartbeatData[512];
    unsigned long                             m_heartbeatSize; /* 0 ~ 1024 */
    mutable CProThreadMutex                   m_lock2;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* TEST_H */
