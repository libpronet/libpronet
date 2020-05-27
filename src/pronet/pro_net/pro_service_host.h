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

#if !defined(PRO_SERVICE_HOST_H)
#define PRO_SERVICE_HOST_H

#include "pro_service_pipe.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

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

    static CProServiceHost* CreateInstance();

    bool Init(
        IProServiceHostObserver* observer,
        IProReactor*             reactor,
        unsigned short           servicePort,
        unsigned char            serviceId
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CProServiceHost();

    virtual ~CProServiceHost();

    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector*   connector,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
        );

    virtual void PRO_CALLTYPE OnRecv(
        CProServicePipe*          pipe,
        const PRO_SERVICE_PACKET& packet
        );

    virtual void PRO_CALLTYPE OnRecvFd(
        CProServicePipe*          pipe,
        PRO_INT64                 fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        );

    virtual void PRO_CALLTYPE OnClose(CProServicePipe* pipe);

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

private:

    IProServiceHostObserver* m_observer;
    IProReactor*             m_reactor;
    IProConnector*           m_connector;
    CProServicePipe*         m_pipe;
    PRO_UINT64               m_timerId;

    unsigned short           m_servicePort;
    unsigned char            m_serviceId;
    PRO_INT64                m_connectTick;

    CProThreadMutex          m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SERVICE_HOST_H */
