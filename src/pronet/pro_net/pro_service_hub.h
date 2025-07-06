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

#ifndef PRO_SERVICE_HUB_H
#define PRO_SERVICE_HUB_H

#include "pro_service_pipe.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProServiceHub
:
public IProAcceptorObserver,
public IProServicePipeObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CProServiceHub* CreateInstance(
        bool enableServiceExt,
        bool enableLoadBalance
        );

    bool Init(
        IProServiceHubObserver* observer,
        IProReactor*            reactor,
        unsigned short          servicePort,
        unsigned int            timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

private:

    CProServiceHub(
        bool enableServiceExt,
        bool enableLoadBalance
        );

    virtual ~CProServiceHub();

    virtual void OnAccept(
        IProAcceptor*  acceptor,
        int64_t        sockId,
        bool           unixSocket,
        const char*    localIp,
        const char*    remoteIp,
        unsigned short remotePort
        );

    virtual void OnAccept(
        IProAcceptor*    acceptor,
        int64_t          sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnRecv(
        CProServicePipe*          pipe,
        const PRO_SERVICE_PACKET& packet
        );

    virtual void OnRecvFd(
        CProServicePipe*          pipe,
        int64_t                   fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        )
    {
    }

    virtual void OnClose(CProServicePipe* pipe);

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

    void AcceptIpc(
        IProAcceptor* acceptor,
        int64_t       sockId,
        bool          unixSocket,
        const char*   localIp
        );

    void AcceptApp(
        IProAcceptor*    acceptor,
        int64_t          sockId,
        bool             unixSocket,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE& nonce
        );

private:

    const bool                                  m_enableServiceExt;
    const bool                                  m_enableLoadBalance;
    IProServiceHubObserver*                     m_observer;
    IProReactor*                                m_reactor;
    IProAcceptor*                               m_acceptor;
    uint64_t                                    m_timerId;

    CProStlMap<PRO_SERVICE_PIPE, unsigned char> m_pipe2ServiceId;       /* all pipes */
    CProStlMap<CProServicePipe*, uint32_t>      m_readyPipe2Socks[256]; /* serviceId0 ~ serviceId255 */
    CProStlSet<PRO_SERVICE_SOCK>                m_expireSocks;

    CProThreadMutex                             m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SERVICE_HUB_H */
