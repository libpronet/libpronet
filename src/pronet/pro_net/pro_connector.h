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

#ifndef PRO_CONNECTOR_H
#define PRO_CONNECTOR_H

#include "pro_event_handler.h"
#include "pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

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
        unsigned int           timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

private:

    CProConnector(
        bool          enableUnixSocket,
        bool          enableServiceExt,
        unsigned char serviceId,
        unsigned char serviceOpt
        );

    virtual ~CProConnector();

    virtual void OnInput(int64_t sockId);

    virtual void OnOutput(int64_t sockId);

    virtual void OnException(int64_t sockId);

    virtual void OnError(
        int64_t sockId,
        int     errorCode
        );

    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        int64_t            sockId,
        bool               unixSocket,
        const void*        buf,
        size_t             size
        );

    virtual void OnHandshakeError(
        IProTcpHandshaker* handshaker,
        int                errorCode
        );

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

private:

    const bool             m_enableUnixSocket;
    const bool             m_enableServiceExt;
    const unsigned char    m_serviceId;
    const unsigned char    m_serviceOpt;
    IProConnectorObserver* m_observer;
    CProTpReactorTask*     m_reactorTask;
    IProTcpHandshaker*     m_handshaker;
    int64_t                m_sockId;
    bool                   m_unixSocket;
    pbsd_sockaddr_in       m_localAddr;
    pbsd_sockaddr_in       m_remoteAddr;
    unsigned int           m_timeoutInSeconds;
    uint64_t               m_timerId0;
    uint64_t               m_timerId1;
    CProThreadMutex        m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_CONNECTOR_H */
