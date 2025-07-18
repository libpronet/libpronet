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

#include "pro_net.h"
#include "pro_acceptor.h"
#include "pro_connector.h"
#include "pro_mcast_transport.h"
#include "pro_service_host.h"
#include "pro_service_hub.h"
#include "pro_ssl_handshaker.h"
#include "pro_ssl_transport.h"
#include "pro_tcp_handshaker.h"
#include "pro_tcp_transport.h"
#include "pro_tp_reactor_task.h"
#include "pro_udp_transport.h"
#include "../pro_util/pro_a.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

extern
void
ProSslInit();

/////////////////////////////////////////////////////////////////////////////
////

PRO_NET_API
void
ProNetInit()
{
    static bool s_flag = false;
    if (s_flag)
    {
        return;
    }
    s_flag = true;

    pbsd_startup();
    ProSslInit();
}

PRO_NET_API
void
ProNetVersion(unsigned char* major, /* = NULL */
              unsigned char* minor, /* = NULL */
              unsigned char* patch) /* = NULL */
{
    if (major != NULL)
    {
        *major = PRO_VER_MAJOR;
    }
    if (minor != NULL)
    {
        *minor = PRO_VER_MINOR;
    }
    if (patch != NULL)
    {
        *patch = PRO_VER_PATCH;
    }
}

PRO_NET_API
IProReactor*
ProCreateReactor(unsigned int ioThreadCount)
{
    ProNetInit();

    CProTpReactorTask* reactorTask = new CProTpReactorTask;
    if (!reactorTask->Start(ioThreadCount))
    {
        delete reactorTask;

        return NULL;
    }

    return reactorTask;
}

PRO_NET_API
void
ProDeleteReactor(IProReactor* reactor)
{
    if (reactor == NULL)
    {
        return;
    }

    CProTpReactorTask* p = (CProTpReactorTask*)reactor;
    delete p;
}

PRO_NET_API
IProAcceptor*
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp,   /* = NULL */
                  unsigned short        localPort) /* = 0 */
{
    ProNetInit();

    CProAcceptor* acceptor = CProAcceptor::CreateInstance(false);
    if (acceptor == NULL)
    {
        return NULL;
    }

    if (!acceptor->Init(observer, (CProTpReactorTask*)reactor, localIp, localPort, 0))
    {
        acceptor->Release();

        return NULL;
    }

    return (IProAcceptor*)acceptor;
}

PRO_NET_API
IProAcceptor*
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp,          /* = NULL */
                    unsigned short        localPort,        /* = 0 */
                    unsigned int          timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProAcceptor* acceptor = CProAcceptor::CreateInstance(true);
    if (acceptor == NULL)
    {
        return NULL;
    }

    if (!acceptor->Init(
        observer, (CProTpReactorTask*)reactor, localIp, localPort, timeoutInSeconds))
    {
        acceptor->Release();

        return NULL;
    }

    return (IProAcceptor*)acceptor;
}

PRO_NET_API
unsigned short
ProGetAcceptorPort(IProAcceptor* acceptor)
{
    assert(acceptor != NULL);
    if (acceptor == NULL)
    {
        return 0;
    }

    CProAcceptor* p = (CProAcceptor*)acceptor;

    return p->GetLocalPort();
}

PRO_NET_API
void
ProDeleteAcceptor(IProAcceptor* acceptor)
{
    if (acceptor == NULL)
    {
        return;
    }

    CProAcceptor* p = (CProAcceptor*)acceptor;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProConnector*
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp,      /* = NULL */
                   unsigned int           timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProConnector* connector = CProConnector::CreateInstance(enableUnixSocket, false, 0, 0);
    if (connector == NULL)
    {
        return NULL;
    }

    if (!connector->Init(observer, (CProTpReactorTask*)reactor,
        remoteIp, remotePort, localBindIp, timeoutInSeconds))
    {
        connector->Release();

        return NULL;
    }

    return (IProConnector*)connector;
}

PRO_NET_API
IProConnector*
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp,      /* = NULL */
                     unsigned int           timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProConnector* connector = CProConnector::CreateInstance(
        enableUnixSocket, true, serviceId, serviceOpt);
    if (connector == NULL)
    {
        return NULL;
    }

    if (!connector->Init(observer, (CProTpReactorTask*)reactor,
        remoteIp, remotePort, localBindIp, timeoutInSeconds))
    {
        connector->Release();

        return NULL;
    }

    return (IProConnector*)connector;
}

PRO_NET_API
void
ProDeleteConnector(IProConnector* connector)
{
    if (connector == NULL)
    {
        return;
    }

    CProConnector* p = (CProConnector*)connector;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProTcpHandshaker*
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData,         /* = NULL */
                       size_t                     sendDataSize,     /* = 0 */
                       size_t                     recvDataSize,     /* = 0 */
                       bool                       recvFirst,        /* = false */
                       unsigned int               timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProTcpHandshaker* handshaker = CProTcpHandshaker::CreateInstance();
    if (handshaker == NULL)
    {
        return NULL;
    }

    if (!handshaker->Init(observer, (CProTpReactorTask*)reactor, sockId, unixSocket,
        sendData, sendDataSize, recvDataSize, recvFirst, timeoutInSeconds))
    {
        handshaker->Release();

        return NULL;
    }

    return (IProTcpHandshaker*)handshaker;
}

PRO_NET_API
void
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker)
{
    if (handshaker == NULL)
    {
        return;
    }

    CProTcpHandshaker* p = (CProTcpHandshaker*)handshaker;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProSslHandshaker*
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData,         /* = NULL */
                       size_t                     sendDataSize,     /* = 0 */
                       size_t                     recvDataSize,     /* = 0 */
                       bool                       recvFirst,        /* = false */
                       unsigned int               timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProSslHandshaker* handshaker = CProSslHandshaker::CreateInstance();
    if (handshaker == NULL)
    {
        return NULL;
    }

    if (!handshaker->Init(observer, (CProTpReactorTask*)reactor, ctx, sockId, unixSocket,
        sendData, sendDataSize, recvDataSize, recvFirst, timeoutInSeconds))
    {
        handshaker->Release();

        return NULL;
    }

    return (IProSslHandshaker*)handshaker;
}

PRO_NET_API
void
ProDeleteSslHandshaker(IProSslHandshaker* handshaker)
{
    if (handshaker == NULL)
    {
        return;
    }

    CProSslHandshaker* p = (CProSslHandshaker*)handshaker;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProTransport*
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv, /* = 0 */
                      size_t                 sockBufSizeSend, /* = 0 */
                      size_t                 recvPoolSize,    /* = 0 */
                      bool                   suspendRecv)     /* = false */
{
    ProNetInit();

    CProTcpTransport* trans = CProTcpTransport::CreateInstance(false, recvPoolSize);
    if (trans == NULL)
    {
        return NULL;
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, sockId, unixSocket,
        sockBufSizeRecv, sockBufSizeSend, suspendRecv))
    {
        trans->Release();

        return NULL;
    }

    return trans;
}

PRO_NET_API
IProTransport*
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      bool                   bindToLocal,       /* = false */
                      const char*            localIp,           /* = NULL */
                      unsigned short         localPort,         /* = 0 */
                      size_t                 sockBufSizeRecv,   /* = 0 */
                      size_t                 sockBufSizeSend,   /* = 0 */
                      size_t                 recvPoolSize,      /* = 0 */
                      const char*            defaultRemoteIp,   /* = NULL */
                      unsigned short         defaultRemotePort) /* = 0 */
{
    ProNetInit();

    CProUdpTransport* trans = CProUdpTransport::CreateInstance(bindToLocal, recvPoolSize);
    if (trans == NULL)
    {
        return NULL;
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, localIp, localPort,
        defaultRemoteIp, defaultRemotePort, sockBufSizeRecv, sockBufSizeSend))
    {
        trans->Release();

        return NULL;
    }

    return trans;
}

PRO_NET_API
IProTransport*
ProCreateMcastTransport(IProTransportObserver* observer,
                        IProReactor*           reactor,
                        const char*            mcastIp,
                        unsigned short         mcastPort,       /* = 0 */
                        const char*            localBindIp,     /* = NULL */
                        size_t                 sockBufSizeRecv, /* = 0 */
                        size_t                 sockBufSizeSend, /* = 0 */
                        size_t                 recvPoolSize)    /* = 0 */
{
    ProNetInit();

    CProMcastTransport* trans = CProMcastTransport::CreateInstance(recvPoolSize);
    if (trans == NULL)
    {
        return NULL;
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, mcastIp, mcastPort, localBindIp,
        sockBufSizeRecv, sockBufSizeSend))
    {
        trans->Release();

        return NULL;
    }

    return trans;
}

PRO_NET_API
IProTransport*
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv, /* = 0 */
                      size_t                 sockBufSizeSend, /* = 0 */
                      size_t                 recvPoolSize,    /* = 0 */
                      bool                   suspendRecv)     /* = false */
{
    ProNetInit();

    CProSslTransport* trans = CProSslTransport::CreateInstance(recvPoolSize);
    if (trans == NULL)
    {
        return NULL;
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, ctx, sockId, unixSocket,
        sockBufSizeRecv, sockBufSizeSend, suspendRecv))
    {
        trans->Release();

        return NULL;
    }

    return trans;
}

PRO_NET_API
void
ProDeleteTransport(IProTransport* trans)
{
    if (trans == NULL)
    {
        return;
    }

    PRO_TRANS_TYPE type = trans->GetType();

    if (type == PRO_TRANS_TCP)
    {
        CProTcpTransport* p = (CProTcpTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_UDP)
    {
        CProUdpTransport* p = (CProUdpTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_MCAST)
    {
        CProMcastTransport* p = (CProMcastTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_SSL)
    {
        CProSslTransport* p = (CProSslTransport*)trans;
        p->Fini();
        p->Release();
    }
    else
    {
    }
}

PRO_NET_API
IProServiceHub*
ProCreateServiceHub(IProServiceHubObserver* observer,
                    IProReactor*            reactor,
                    unsigned short          servicePort,
                    bool                    enableLoadBalance) /* = false */
{
    ProNetInit();

    CProServiceHub* hub = CProServiceHub::CreateInstance(
        false, /* enableServiceExt is false */
        enableLoadBalance
        );
    if (hub == NULL)
    {
        return NULL;
    }

    if (!hub->Init(observer, reactor, servicePort, 0))
    {
        hub->Release();

        return NULL;
    }

    return (IProServiceHub*)hub;
}

PRO_NET_API
IProServiceHub*
ProCreateServiceHubEx(IProServiceHubObserver* observer,
                      IProReactor*            reactor,
                      unsigned short          servicePort,
                      bool                    enableLoadBalance, /* = false */
                      unsigned int            timeoutInSeconds)  /* = 0 */
{
    ProNetInit();

    CProServiceHub* hub = CProServiceHub::CreateInstance(
        true, /* enableServiceExt is true */
        enableLoadBalance
        );
    if (hub == NULL)
    {
        return NULL;
    }

    if (!hub->Init(observer, reactor, servicePort, timeoutInSeconds))
    {
        hub->Release();

        return NULL;
    }

    return (IProServiceHub*)hub;
}

PRO_NET_API
void
ProDeleteServiceHub(IProServiceHub* hub)
{
    if (hub == NULL)
    {
        return;
    }

    CProServiceHub* p = (CProServiceHub*)hub;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProServiceHost*
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort)
{
    ProNetInit();

    CProServiceHost* host = CProServiceHost::CreateInstance(0); /* serviceId is 0 */
    if (host == NULL)
    {
        return NULL;
    }

    if (!host->Init(observer, reactor, servicePort))
    {
        host->Release();

        return NULL;
    }

    return (IProServiceHost*)host;
}

PRO_NET_API
IProServiceHost*
ProCreateServiceHostEx(IProServiceHostObserver* observer,
                       IProReactor*             reactor,
                       unsigned short           servicePort,
                       unsigned char            serviceId)
{
    ProNetInit();

    assert(serviceId > 0);
    if (serviceId == 0)
    {
        return NULL;
    }

    CProServiceHost* host = CProServiceHost::CreateInstance(serviceId);
    if (host == NULL)
    {
        return NULL;
    }

    if (!host->Init(observer, reactor, servicePort))
    {
        host->Release();

        return NULL;
    }

    return (IProServiceHost*)host;
}

PRO_NET_API
void
ProDeleteServiceHost(IProServiceHost* host)
{
    if (host == NULL)
    {
        return;
    }

    CProServiceHost* p = (CProServiceHost*)host;
    p->Fini();
    p->Release();
}

PRO_NET_API
int64_t
ProOpenTcpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort)
{
    ProNetInit();

    assert(localPort > 0);
    if (localPort == 0)
    {
        return -1;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (uint32_t)-1)
    {
        return -1;
    }

    int64_t sockId = pbsd_socket(AF_INET, SOCK_STREAM, 0);
    if (sockId == -1)
    {
        return -1;
    }

    if (pbsd_bind(sockId, &localAddr, false) != 0)
    {
        ProCloseSockId(sockId);
        sockId = -1;
    }

    return sockId;
}

PRO_NET_API
int64_t
ProOpenUdpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort)
{
    ProNetInit();

    assert(localPort > 0);
    if (localPort == 0)
    {
        return -1;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (uint32_t)-1)
    {
        return -1;
    }

    int64_t sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockId == -1)
    {
        return -1;
    }

    if (pbsd_bind(sockId, &localAddr, false) != 0)
    {
        ProCloseSockId(sockId);
        sockId = -1;
    }

    return sockId;
}

PRO_NET_API
void
ProCloseSockId(int64_t sockId,
               bool    linger) /* = false */
{
    if (sockId == -1)
    {
        return;
    }

    ProDecServiceLoad(sockId); /* for load-balance */
    pbsd_closesocket(sockId, linger);
}

/////////////////////////////////////////////////////////////////////////////
////

class CProNetDotCpp
{
public:

    CProNetDotCpp()
    {
        ProNetInit();
    }
};

static volatile CProNetDotCpp g_s_initiator;

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
