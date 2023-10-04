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

#include "pro_acceptor.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_tp_reactor_task.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define SERVICE_HANDSHAKE_BYTES 4 /* serviceId + serviceOpt + (r) + (r+1) */
#define DEFAULT_TIMEOUT         10

/////////////////////////////////////////////////////////////////////////////
////

static
void
MakeNonce_i(PRO_NONCE& nonce)
{
    for (int i = 0; i < 32; ++i)
    {
        nonce.nonce[i] = (unsigned char)(ProRand_0_32767() % 256);
    }
}

/////////////////////////////////////////////////////////////////////////////
////

CProAcceptor*
CProAcceptor::CreateInstance(bool enableServiceExt)
{
    return new CProAcceptor(enableServiceExt, enableServiceExt);
}

CProAcceptor*
CProAcceptor::CreateInstanceOnlyLoopExt()
{
    return new CProAcceptor(false, true);
}

CProAcceptor::CProAcceptor(bool nonloopExt,
                           bool loopExt)
:
m_nonloopExt(nonloopExt),
m_loopExt(loopExt)
{
    m_observer         = NULL;
    m_reactorTask      = NULL;
    m_sockId           = -1;
    m_sockIdUn         = -1;
    m_timeoutInSeconds = DEFAULT_TIMEOUT;

    memset(&m_localAddr  , 0, sizeof(pbsd_sockaddr_in));
    memset(&m_localAddrUn, 0, sizeof(pbsd_sockaddr_un));
}

CProAcceptor::~CProAcceptor()
{
    Fini();

    ProCloseSockId(m_sockId);
    ProCloseSockId(m_sockIdUn);
#if !defined(_WIN32)
    if (m_sockIdUn != -1)
    {
        unlink(m_localAddrUn.sun_path);
    }
#endif
    m_sockId   = -1;
    m_sockIdUn = -1;
}

bool
CProAcceptor::Init(IProAcceptorObserver* observer,
                   CProTpReactorTask*    reactorTask,
                   const char*           localIp,          /* = NULL */
                   unsigned short        localPort,        /* = 0 */
                   unsigned int          timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    if (observer == NULL || reactorTask == NULL)
    {
        return false;
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (uint32_t)-1)
    {
        return false;
    }

    int64_t          sockId   = -1;
    int64_t          sockIdUn = -1;
    pbsd_sockaddr_un localAddrUn;
    memset(&localAddrUn, 0, sizeof(pbsd_sockaddr_un));

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return false;
        }

        sockId = pbsd_socket(AF_INET, SOCK_STREAM, 0);
        if (sockId == -1)
        {
            goto EXIT;
        }

        int option = 1;
        pbsd_setsockopt(sockId, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));

#if defined(_WIN32)
        if (pbsd_bind(sockId, &localAddr, false) != 0)
#else
        if (pbsd_bind(sockId, &localAddr, true)  != 0)
#endif
        {
            goto EXIT;
        }

        if (pbsd_getsockname(sockId, &localAddr) != 0)
        {
            goto EXIT;
        }

        if (pbsd_listen(sockId) != 0)
        {
            goto EXIT;
        }

        if (!reactorTask->AddHandler(sockId, this, PRO_MASK_ACCEPT))
        {
            goto EXIT;
        }

#if !defined(_WIN32)

        sockIdUn = pbsd_socket(AF_LOCAL, SOCK_STREAM, 0);
        if (sockIdUn == -1)
        {
            goto EXIT;
        }

        localAddrUn.sun_family = AF_LOCAL;
        sprintf(localAddrUn.sun_path, "/tmp/libpronet_127001_%u",
            (unsigned int)pbsd_ntoh16(localAddr.sin_port));

        if (access(localAddrUn.sun_path, F_OK) == 0)
        {
            /*
             * This may fail with "Permission Denied"!!!
             */
            unlink(localAddrUn.sun_path);
        }

        if (pbsd_bind_un(sockIdUn, &localAddrUn) != 0)
        {
            goto EXIT;
        }

        /*
         * Allow other users to access the file. "Write" permissions are required.
         */
        chmod(localAddrUn.sun_path, S_IRWXU | S_IRWXG | S_IRWXO);

        if (pbsd_listen(sockIdUn) != 0)
        {
            goto EXIT;
        }

        if (!reactorTask->AddHandler(sockIdUn, this, PRO_MASK_ACCEPT))
        {
            goto EXIT;
        }

#endif /* _WIN32 */

        observer->AddRef();
        m_observer         = observer;
        m_reactorTask      = reactorTask;
        m_sockId           = sockId;
        m_sockIdUn         = sockIdUn;
        m_localAddr        = localAddr;
        m_localAddrUn      = localAddrUn;
        m_timeoutInSeconds = timeoutInSeconds;
    }

    return true;

EXIT:

    reactorTask->RemoveHandler(sockId  , this, PRO_MASK_ACCEPT);
    reactorTask->RemoveHandler(sockIdUn, this, PRO_MASK_ACCEPT);
    ProCloseSockId(sockId);
    ProCloseSockId(sockIdUn);
#if !defined(_WIN32)
    if (sockIdUn != -1)
    {
        unlink(localAddrUn.sun_path);
    }
#endif

    return false;
}

void
CProAcceptor::Fini()
{
    IProAcceptorObserver*                     observer = NULL;
    CProStlMap<IProTcpHandshaker*, PRO_NONCE> handshaker2Nonce;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->RemoveHandler(m_sockId  , this, PRO_MASK_ACCEPT);
        m_reactorTask->RemoveHandler(m_sockIdUn, this, PRO_MASK_ACCEPT);

        handshaker2Nonce = m_handshaker2Nonce;
        m_handshaker2Nonce.clear();
        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    auto itr = handshaker2Nonce.begin();
    auto end = handshaker2Nonce.end();

    for (; itr != end; ++itr)
    {
        ProDeleteTcpHandshaker(itr->first);
    }

    observer->Release();
}

unsigned long
CProAcceptor::AddRef()
{
    return CProEventHandler::AddRef();
}

unsigned long
CProAcceptor::Release()
{
    return CProEventHandler::Release();
}

unsigned short
CProAcceptor::GetLocalPort() const
{
    unsigned short localPort = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        localPort = pbsd_ntoh16(m_localAddr.sin_port);
    }

    return localPort;
}

void
CProAcceptor::OnInput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    IProAcceptorObserver* observer   = NULL;
    IProTcpHandshaker*    handshaker = NULL;
    int64_t               newSockId  = -1;
    bool                  unixSocket = false;
    pbsd_sockaddr_in      localAddr;
    pbsd_sockaddr_in      remoteAddr;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        if (sockId == m_sockId)
        {
            newSockId  = pbsd_accept(m_sockId, &remoteAddr);
            unixSocket = false;

            if (newSockId != -1)
            {
                int option = 1;
                pbsd_setsockopt(newSockId, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));

                if (pbsd_getsockname(newSockId, &localAddr) != 0)
                {
                    ProCloseSockId(newSockId);
                    newSockId = -1;
                }
            }
        }
        else if (sockId == m_sockIdUn)
        {
            pbsd_sockaddr_un remoteAddrUn;
            newSockId  = pbsd_accept_un(m_sockIdUn, &remoteAddrUn);
            unixSocket = true;
        }
        else
        {
            return;
        }

        if (newSockId == -1)
        {
            return;
        }

        /*
         * ddos?
         */
        if (m_handshaker2Nonce.size() >= PRO_ACCEPTOR_LENGTH)
        {
            ProCloseSockId(newSockId);

            return;
        }

        bool isLoop = false;
        if (unixSocket || localAddr.sin_addr.s_addr == pbsd_inet_aton("127.0.0.1"))
        {
            isLoop = true;
        }

        if (
            (isLoop  && m_loopExt)
            ||
            (!isLoop && m_nonloopExt)
           )
        {
            PRO_NONCE nonce;
            MakeNonce_i(nonce);

            handshaker = ProCreateTcpHandshaker(
                this,
                m_reactorTask,
                newSockId,
                unixSocket,
                &nonce,                  /* sendData */
                sizeof(PRO_NONCE),       /* sendDataSize */
                SERVICE_HANDSHAKE_BYTES, /* recvDataSize = serviceId + serviceOpt + (r) + (r+1) */
                false,                   /* recvFirst is false */
                m_timeoutInSeconds
                );
            if (handshaker != NULL)
            {
                m_handshaker2Nonce[handshaker] = nonce;
            }
            else
            {
                ProCloseSockId(newSockId);
            }

            return; /* return now */
        }

        m_observer->AddRef();
        observer = m_observer;
    }

    char           localIp[64]  = "127.0.0.1"; /* a dummy for unix socket */
    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    if (!unixSocket)
    {
        pbsd_inet_ntoa(localAddr.sin_addr.s_addr , localIp);
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    observer->OnAccept(
        (IProAcceptor*)this,
        newSockId,
        unixSocket,
        localIp,
        remoteIp,
        remotePort
        );
    observer->Release();
}

void
CProAcceptor::OnHandshakeOk(IProTcpHandshaker* handshaker,
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

    const unsigned char* serviceData = (unsigned char*)buf;

    IProAcceptorObserver* observer = NULL;
    PRO_NONCE             nonce;
    pbsd_sockaddr_in      localAddr;
    pbsd_sockaddr_in      remoteAddr;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        auto itr = m_handshaker2Nonce.find(handshaker);
        if (itr == m_handshaker2Nonce.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        if (!unixSocket)
        {
            if (pbsd_getsockname(sockId, &localAddr)  != 0 ||
                pbsd_getpeername(sockId, &remoteAddr) != 0)
            {
                ProCloseSockId(sockId);
                sockId = -1;
            }
        }

        assert(serviceData != NULL);
        assert(size == SERVICE_HANDSHAKE_BYTES); /* serviceId + serviceOpt + (r) + (r+1) */
        assert(serviceData[3] == (unsigned char)(serviceData[2] + 1));
        if (serviceData == NULL || size != SERVICE_HANDSHAKE_BYTES ||
            serviceData[3] != (unsigned char)(serviceData[2] + 1))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        nonce = itr->second;
        m_handshaker2Nonce.erase(itr);

        m_observer->AddRef();
        observer = m_observer;
    }

    ProDeleteTcpHandshaker(handshaker);

    if (sockId == -1)
    {
        observer->Release();

        return;
    }

    char           localIp[64]  = "127.0.0.1"; /* a dummy for unix socket */
    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    unsigned char  serviceId    = serviceData[0];
    unsigned char  serviceOpt   = serviceData[1];
    if (!unixSocket)
    {
        pbsd_inet_ntoa(localAddr.sin_addr.s_addr , localIp);
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    observer->OnAccept(
        (IProAcceptor*)this,
        sockId,
        unixSocket,
        localIp,
        remoteIp,
        remotePort,
        serviceId,
        serviceOpt,
        &nonce
        );
    observer->Release();
}

void
CProAcceptor::OnHandshakeError(IProTcpHandshaker* handshaker,
                               int                errorCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        auto itr = m_handshaker2Nonce.find(handshaker);
        if (itr == m_handshaker2Nonce.end())
        {
            return;
        }

        m_handshaker2Nonce.erase(itr);
    }

    ProDeleteTcpHandshaker(handshaker);
}
