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

#if !defined(PRO_CONNECTOR_H)
#define PRO_CONNECTOR_H

#include "pro_event_handler.h"
#include "pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProTpReactorTask;

/////////////////////////////////////////////////////////////////////////////
////

class CProConnector : public IProTcpHandshakerObserver, public CProEventHandler
{
public:

    static CProConnector* CreateInstance(
        bool          enableUnixSocket,
        bool          enableServiceExt,
        unsigned char serviceId,
        unsigned char serviceOpt
        );

    bool Init(
        IProConnectorObserver* observer,
        CProTpReactorTask*     reactorTask,
        const char*            remoteIp,
        unsigned short         remotePort,
        const char*            localBindIp,     /* = NULL */
        unsigned long          timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CProConnector(
        bool          enableUnixSocket,
        bool          enableServiceExt,
        unsigned char serviceId,
        unsigned char serviceOpt
        );

    virtual ~CProConnector();

    virtual void PRO_CALLTYPE OnInput(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnOutput(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnException(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnError(
        PRO_INT64 sockId,
        long      errorCode
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

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

private:

    const bool             m_enableUnixSocket;
    const bool             m_enableServiceExt;
    const unsigned char    m_serviceId;
    const unsigned char    m_serviceOpt;
    IProConnectorObserver* m_observer;
    CProTpReactorTask*     m_reactorTask;
    IProTcpHandshaker*     m_handshaker;
    PRO_INT64              m_sockId;
    bool                   m_unixSocket;
    pbsd_sockaddr_in       m_localAddr;
    pbsd_sockaddr_in       m_remoteAddr;
    unsigned long          m_timeoutInSeconds;
    PRO_UINT64             m_timerId0;
    PRO_UINT64             m_timerId1;
    CProThreadMutex        m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_CONNECTOR_H */
