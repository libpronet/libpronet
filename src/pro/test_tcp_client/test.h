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

#if !defined(TEST_H)
#define TEST_H

#include "../pro_net/pro_net.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

struct TCP_CLIENT_CONFIG_INFO
{
    TCP_CLIENT_CONFIG_INFO()
    {
        tcpc_thread_count        = 10;
        tcpc_server_ip           = "127.0.0.1";
        tcpc_server_port         = 3000;
        tcpc_local_ip            = "0.0.0.0";
        tcpc_connection_count    = 1;
        tcpc_max_pending_count   = 100;
        tcpc_handshake_timeout   = 20;
        tcpc_heartbeat_interval  = 200;
        tcpc_heartbeat_bytes     = 0;
        tcpc_sockbuf_size_recv   = 2048;
        tcpc_sockbuf_size_send   = 2048;
        tcpc_recvpool_size       = 2048;

        tcpc_enable_ssl          = false;
        tcpc_ssl_enable_sha1cert = true;
        tcpc_ssl_sni             = "server.libpro.org";
        tcpc_ssl_aes256          = false;

        tcpc_ssl_cafiles.push_back("./ca.crt");
        tcpc_ssl_cafiles.push_back("");
        tcpc_ssl_crlfiles.push_back("");
        tcpc_ssl_crlfiles.push_back("");
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint("tcpc_thread_count"       , tcpc_thread_count);
        configStream.Add    ("tcpc_server_ip"          , tcpc_server_ip);
        configStream.AddUint("tcpc_server_port"        , tcpc_server_port);
        configStream.Add    ("tcpc_local_ip"           , tcpc_local_ip);
        configStream.AddUint("tcpc_connection_count"   , tcpc_connection_count);
        configStream.AddUint("tcpc_max_pending_count"  , tcpc_max_pending_count);
        configStream.AddUint("tcpc_handshake_timeout"  , tcpc_handshake_timeout);
        configStream.AddUint("tcpc_heartbeat_interval" , tcpc_heartbeat_interval);
        configStream.AddUint("tcpc_heartbeat_bytes"    , tcpc_heartbeat_bytes);
        configStream.AddUint("tcpc_sockbuf_size_recv"  , tcpc_sockbuf_size_recv);
        configStream.AddUint("tcpc_sockbuf_size_send"  , tcpc_sockbuf_size_send);
        configStream.AddUint("tcpc_recvpool_size"      , tcpc_recvpool_size);

        configStream.AddInt ("tcpc_enable_ssl"         , tcpc_enable_ssl);
        configStream.AddInt ("tcpc_ssl_enable_sha1cert", tcpc_ssl_enable_sha1cert);
        configStream.Add    ("tcpc_ssl_cafile"         , tcpc_ssl_cafiles);
        configStream.Add    ("tcpc_ssl_crlfile"        , tcpc_ssl_crlfiles);
        configStream.Add    ("tcpc_ssl_sni"            , tcpc_ssl_sni);
        configStream.AddInt ("tcpc_ssl_aes256"         , tcpc_ssl_aes256);

        configStream.Get(configs);
    }

    unsigned int                 tcpc_thread_count;      /* 1 ~ 100 */
    CProStlString                tcpc_server_ip;
    unsigned short               tcpc_server_port;
    CProStlString                tcpc_local_ip;
    unsigned int                 tcpc_connection_count;  /* 1 ~ 60000 */
    unsigned int                 tcpc_max_pending_count; /* 1 ~ 1000 */
    unsigned int                 tcpc_handshake_timeout;
    unsigned int                 tcpc_heartbeat_interval;
    unsigned int                 tcpc_heartbeat_bytes;   /* 0 ~ 1024 */
    unsigned int                 tcpc_sockbuf_size_recv; /* >= 1024 */
    unsigned int                 tcpc_sockbuf_size_send; /* >= 1024 */
    unsigned int                 tcpc_recvpool_size;     /* >= 1024 */

    bool                         tcpc_enable_ssl;
    bool                         tcpc_ssl_enable_sha1cert;
    CProStlVector<CProStlString> tcpc_ssl_cafiles;
    CProStlVector<CProStlString> tcpc_ssl_crlfiles;
    CProStlString                tcpc_ssl_sni;
    bool                         tcpc_ssl_aes256;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CTest
:
public IProConnectorObserver,
public IProTcpHandshakerObserver,
public IProSslHandshakerObserver,
public IProTransportObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CTest* CreateInstance();

    bool Init(
        IProReactor*                  reactor,
        const TCP_CLIENT_CONFIG_INFO& configInfo
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    void SetHeartbeatDataSize(unsigned long size); /* 0 ~ 1024 */

    unsigned long GetHeartbeatDataSize() const;

    void SendMsg(const char* msg);

private:

    CTest();

    virtual ~CTest();

    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector* connector,
        PRO_INT64      sockId,
        bool           unixSocket,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        PRO_UINT64     nonce
        );

    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
        );

    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void PRO_CALLTYPE OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode
        );

    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void PRO_CALLTYPE OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,
        long               sslCode
        );

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        )
    {
    }

    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans);

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        );

    bool DoHandshake(
        PRO_INT64  sockId,
        bool       unixSocket,
        PRO_UINT64 nonce
        );

private:

    IProReactor*                   m_reactor;
    TCP_CLIENT_CONFIG_INFO         m_configInfo;
    PRO_SSL_CLIENT_CONFIG*         m_sslConfig;
    unsigned long                  m_timerId;

    CProStlSet<IProConnector*>     m_connectors;
    CProStlSet<IProTcpHandshaker*> m_tcpHandshakers;
    CProStlSet<IProSslHandshaker*> m_sslHandshakers;
    CProStlSet<IProTransport*>     m_transports;
    PRO_UINT16                     m_heartbeatData[512];
    unsigned long                  m_heartbeatSize; /* 0 ~ 1024 */

    mutable CProThreadMutex        m_lock;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* TEST_H */
