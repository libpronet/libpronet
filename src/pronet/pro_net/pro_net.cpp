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
#include <cassert>
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

extern
void
PRO_CALLTYPE
ProSslInit();

/////////////////////////////////////////////////////////////////////////////
////

PRO_NET_API
void
PRO_CALLTYPE
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
PRO_CALLTYPE
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
PRO_CALLTYPE
ProCreateReactor(unsigned long ioThreadCount,
                 long          ioThreadPriority) /* = 0 */
{
    ProNetInit();

    CProTpReactorTask* const reactorTask = new CProTpReactorTask;
    if (!reactorTask->Start(ioThreadCount, ioThreadPriority))
    {
        delete reactorTask;

        return (NULL);
    }

    return (reactorTask);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteReactor(IProReactor* reactor)
{
    if (reactor == NULL)
    {
        return;
    }

    CProTpReactorTask* const p = (CProTpReactorTask*)reactor;
    p->Stop();
    delete p;
}

PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp,   /* = NULL */
                  unsigned short        localPort) /* = 0 */
{
    ProNetInit();

    CProAcceptor* const acceptor = CProAcceptor::CreateInstance(false);
    if (acceptor == NULL)
    {
        return (NULL);
    }

    if (!acceptor->Init(
        observer, (CProTpReactorTask*)reactor, localIp, localPort, 0))
    {
        acceptor->Release();

        return (NULL);
    }

    return ((IProAcceptor*)acceptor);
}

PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp,          /* = NULL */
                    unsigned short        localPort,        /* = 0 */
                    unsigned long         timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProAcceptor* const acceptor = CProAcceptor::CreateInstance(true);
    if (acceptor == NULL)
    {
        return (NULL);
    }

    if (!acceptor->Init(observer, (CProTpReactorTask*)reactor,
        localIp, localPort, timeoutInSeconds))
    {
        acceptor->Release();

        return (NULL);
    }

    return ((IProAcceptor*)acceptor);
}

PRO_NET_API
unsigned short
PRO_CALLTYPE
ProGetAcceptorPort(IProAcceptor* acceptor)
{
    assert(acceptor != NULL);
    if (acceptor == NULL)
    {
        return (0);
    }

    CProAcceptor* const p = (CProAcceptor*)acceptor;

    return (p->GetLocalPort());
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteAcceptor(IProAcceptor* acceptor)
{
    if (acceptor == NULL)
    {
        return;
    }

    CProAcceptor* const p = (CProAcceptor*)acceptor;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp,      /* = NULL */
                   unsigned long          timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProConnector* const connector =
        CProConnector::CreateInstance(enableUnixSocket, false, 0, 0);
    if (connector == NULL)
    {
        return (NULL);
    }

    if (!connector->Init(observer, (CProTpReactorTask*)reactor,
        remoteIp, remotePort, localBindIp, timeoutInSeconds))
    {
        connector->Release();

        return (NULL);
    }

    return ((IProConnector*)connector);
}

PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp,      /* = NULL */
                     unsigned long          timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProConnector* const connector = CProConnector::CreateInstance(
        enableUnixSocket, true, serviceId, serviceOpt);
    if (connector == NULL)
    {
        return (NULL);
    }

    if (!connector->Init(observer, (CProTpReactorTask*)reactor,
        remoteIp, remotePort, localBindIp, timeoutInSeconds))
    {
        connector->Release();

        return (NULL);
    }

    return ((IProConnector*)connector);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteConnector(IProConnector* connector)
{
    if (connector == NULL)
    {
        return;
    }

    CProConnector* const p = (CProConnector*)connector;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProTcpHandshaker*
PRO_CALLTYPE
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData,         /* = NULL */
                       size_t                     sendDataSize,     /* = 0 */
                       size_t                     recvDataSize,     /* = 0 */
                       bool                       recvFirst,        /* = false */
                       unsigned long              timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProTcpHandshaker* const handshaker = CProTcpHandshaker::CreateInstance();
    if (handshaker == NULL)
    {
        return (NULL);
    }

    if (!handshaker->Init(observer, (CProTpReactorTask*)reactor,
        sockId, unixSocket, sendData, sendDataSize, recvDataSize,
        recvFirst, timeoutInSeconds))
    {
        handshaker->Release();

        return (NULL);
    }

    return ((IProTcpHandshaker*)handshaker);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker)
{
    if (handshaker == NULL)
    {
        return;
    }

    CProTcpHandshaker* const p = (CProTcpHandshaker*)handshaker;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProSslHandshaker*
PRO_CALLTYPE
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData,         /* = NULL */
                       size_t                     sendDataSize,     /* = 0 */
                       size_t                     recvDataSize,     /* = 0 */
                       bool                       recvFirst,        /* = false */
                       unsigned long              timeoutInSeconds) /* = 0 */
{
    ProNetInit();

    CProSslHandshaker* const handshaker = CProSslHandshaker::CreateInstance();
    if (handshaker == NULL)
    {
        return (NULL);
    }

    if (!handshaker->Init(observer, (CProTpReactorTask*)reactor,
        ctx, sockId, unixSocket, sendData, sendDataSize, recvDataSize,
        recvFirst, timeoutInSeconds))
    {
        handshaker->Release();

        return (NULL);
    }

    return ((IProSslHandshaker*)handshaker);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteSslHandshaker(IProSslHandshaker* handshaker)
{
    if (handshaker == NULL)
    {
        return;
    }

    CProSslHandshaker* const p = (CProSslHandshaker*)handshaker;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv, /* = 0 */
                      size_t                 sockBufSizeSend, /* = 0 */
                      size_t                 recvPoolSize,    /* = 0 */
                      bool                   suspendRecv)     /* = false */
{
    ProNetInit();

    CProTcpTransport* const trans =
        CProTcpTransport::CreateInstance(false, recvPoolSize);
    if (trans == NULL)
    {
        return (NULL);
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, sockId,
        unixSocket, sockBufSizeRecv, sockBufSizeSend, suspendRecv))
    {
        trans->Release();

        return (NULL);
    }

    return (trans);
}

PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      const char*            localIp,           /* = NULL */
                      unsigned short         localPort,         /* = 0 */
                      size_t                 sockBufSizeRecv,   /* = 0 */
                      size_t                 sockBufSizeSend,   /* = 0 */
                      size_t                 recvPoolSize,      /* = 0 */
                      const char*            defaultRemoteIp,   /* = NULL */
                      unsigned short         defaultRemotePort) /* = 0 */
{
    ProNetInit();

    CProUdpTransport* const trans =
        CProUdpTransport::CreateInstance(recvPoolSize);
    if (trans == NULL)
    {
        return (NULL);
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor,
        localIp, localPort, defaultRemoteIp, defaultRemotePort,
        sockBufSizeRecv, sockBufSizeSend))
    {
        trans->Release();

        return (NULL);
    }

    return (trans);
}

PRO_NET_API
IProTransport*
PRO_CALLTYPE
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

    CProMcastTransport* const trans =
        CProMcastTransport::CreateInstance(recvPoolSize);
    if (trans == NULL)
    {
        return (NULL);
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor,
        mcastIp, mcastPort, localBindIp, sockBufSizeRecv, sockBufSizeSend))
    {
        trans->Release();

        return (NULL);
    }

    return (trans);
}

PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv, /* = 0 */
                      size_t                 sockBufSizeSend, /* = 0 */
                      size_t                 recvPoolSize,    /* = 0 */
                      bool                   suspendRecv)     /* = false */
{
    ProNetInit();

    CProSslTransport* const trans =
        CProSslTransport::CreateInstance(recvPoolSize);
    if (trans == NULL)
    {
        return (NULL);
    }

    if (!trans->Init(observer, (CProTpReactorTask*)reactor, ctx, sockId,
        unixSocket, sockBufSizeRecv, sockBufSizeSend, suspendRecv))
    {
        trans->Release();

        return (NULL);
    }

    return (trans);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTransport(IProTransport* trans)
{
    if (trans == NULL)
    {
        return;
    }

    const PRO_TRANS_TYPE type = trans->GetType();

    if (type == PRO_TRANS_TCP)
    {
        CProTcpTransport* const p = (CProTcpTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_UDP)
    {
        CProUdpTransport* const p = (CProUdpTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_MCAST)
    {
        CProMcastTransport* const p = (CProMcastTransport*)trans;
        p->Fini();
        p->Release();
    }
    else if (type == PRO_TRANS_SSL)
    {
        CProSslTransport* const p = (CProSslTransport*)trans;
        p->Fini();
        p->Release();
    }
    else
    {
    }
}

PRO_NET_API
IProServiceHub*
PRO_CALLTYPE
ProCreateServiceHub(IProReactor*   reactor,
                    unsigned short servicePort,
                    bool           enableLoadBalance) /* = false */
{
    ProNetInit();

    CProServiceHub* const hub = CProServiceHub::CreateInstance(
        false, /* enableServiceExt is false */
        enableLoadBalance
        );
    if (hub == NULL)
    {
        return (NULL);
    }

    if (!hub->Init(reactor, servicePort, 0))
    {
        hub->Release();

        return (NULL);
    }

    return ((IProServiceHub*)hub);
}

PRO_NET_API
IProServiceHub*
PRO_CALLTYPE
ProCreateServiceHubEx(IProReactor*   reactor,
                      unsigned short servicePort,
                      bool           enableLoadBalance, /* = false */
                      unsigned long  timeoutInSeconds)  /* = 0 */
{
    ProNetInit();

    CProServiceHub* const hub = CProServiceHub::CreateInstance(
        true, /* enableServiceExt is true */
        enableLoadBalance
        );
    if (hub == NULL)
    {
        return (NULL);
    }

    if (!hub->Init(reactor, servicePort, timeoutInSeconds))
    {
        hub->Release();

        return (NULL);
    }

    return ((IProServiceHub*)hub);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHub(IProServiceHub* hub)
{
    if (hub == NULL)
    {
        return;
    }

    CProServiceHub* const p = (CProServiceHub*)hub;
    p->Fini();
    p->Release();
}

PRO_NET_API
IProServiceHost*
PRO_CALLTYPE
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort)
{
    ProNetInit();

    CProServiceHost* const host = CProServiceHost::CreateInstance(0); /* serviceId is 0 */
    if (host == NULL)
    {
        return (NULL);
    }

    if (!host->Init(observer, reactor, servicePort))
    {
        host->Release();

        return (NULL);
    }

    return ((IProServiceHost*)host);
}

PRO_NET_API
IProServiceHost*
PRO_CALLTYPE
ProCreateServiceHostEx(IProServiceHostObserver* observer,
                       IProReactor*             reactor,
                       unsigned short           servicePort,
                       unsigned char            serviceId)
{
    ProNetInit();

    assert(serviceId > 0);
    if (serviceId == 0)
    {
        return (NULL);
    }

    CProServiceHost* const host = CProServiceHost::CreateInstance(serviceId);
    if (host == NULL)
    {
        return (NULL);
    }

    if (!host->Init(observer, reactor, servicePort))
    {
        host->Release();

        return (NULL);
    }

    return ((IProServiceHost*)host);
}

PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHost(IProServiceHost* host)
{
    if (host == NULL)
    {
        return;
    }

    CProServiceHost* const p = (CProServiceHost*)host;
    p->Fini();
    p->Release();
}

PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenTcpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort)
{
    ProNetInit();

    assert(localPort > 0);
    if (localPort == 0)
    {
        return (-1);
    }

    const char* const anyIp = "0.0.0.0";
    if (localIp == NULL || localIp[0] == '\0')
    {
        localIp = anyIp;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return (-1);
    }

    PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_STREAM, 0);
    if (sockId == -1)
    {
        return (-1);
    }

    if (pbsd_bind(sockId, &localAddr, false) != 0)
    {
        ProCloseSockId(sockId);
        sockId = -1;
    }

    return (sockId);
}

PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenUdpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort)
{
    ProNetInit();

    assert(localPort > 0);
    if (localPort == 0)
    {
        return (-1);
    }

    const char* const anyIp = "0.0.0.0";
    if (localIp == NULL || localIp[0] == '\0')
    {
        localIp = anyIp;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return (-1);
    }

    PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockId == -1)
    {
        return (-1);
    }

    if (pbsd_bind(sockId, &localAddr, false) != 0)
    {
        ProCloseSockId(sockId);
        sockId = -1;
    }

    return (sockId);
}

PRO_NET_API
void
PRO_CALLTYPE
ProCloseSockId(PRO_INT64 sockId,
               bool      linger) /* = false */
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
