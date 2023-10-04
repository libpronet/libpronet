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

#ifndef PRO_SERVICE_HOST_H
#define PRO_SERVICE_HOST_H

#include "pro_service_pipe.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProServiceHost
:
public IProConnectorObserver,
public IProServicePipeObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CProServiceHost* CreateInstance(unsigned char serviceId); /* = 0 */

    bool Init(
        IProServiceHostObserver* observer,
        IProReactor*             reactor,
        unsigned short           servicePort
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    void DecServiceLoad(int64_t sockId);

private:

    CProServiceHost(unsigned char serviceId);

    virtual ~CProServiceHost();

    virtual void OnConnectOk(
        IProConnector* connector,
        int64_t        sockId,
        bool           unixSocket,
        const char*    remoteIp,
        unsigned short remotePort
        )
    {
    }

    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        bool           timeout
        )
    {
    }

    virtual void OnConnectOk(
        IProConnector*   connector,
        int64_t          sockId,
        bool             unixSocket,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
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
        );

    virtual void OnClose(CProServicePipe* pipe);

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

private:

    const unsigned char      m_serviceId;
    IProServiceHostObserver* m_observer;
    IProReactor*             m_reactor;
    IProConnector*           m_connector;
    CProServicePipe*         m_pipe;
    uint64_t                 m_timerId;
    unsigned short           m_servicePort;
    int64_t                  m_connectTick;
    CProThreadMutex          m_lock;

    CProStlSet<int64_t>      m_onlineSockIds;
    CProThreadMutex          m_onlineLock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

void
ProDecServiceLoad(int64_t sockId);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SERVICE_HOST_H */
