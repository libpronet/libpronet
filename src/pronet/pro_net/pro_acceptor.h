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

#if !defined(PRO_ACCEPTOR_H)
#define PRO_ACCEPTOR_H

#include "pro_event_handler.h"
#include "pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProTpReactorTask;

/////////////////////////////////////////////////////////////////////////////
////

class CProAcceptor : public IProTcpHandshakerObserver, public CProEventHandler
{
public:

    static CProAcceptor* CreateInstance(bool enableServiceExt);

    bool Init(
        IProAcceptorObserver* observer,
        CProTpReactorTask*    reactorTask,
        const char*           localIp,         /* = NULL */
        unsigned short        localPort,       /* = 0 */
        unsigned long         timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    unsigned short GetLocalPort() const;

private:

    CProAcceptor(bool enableServiceExt);

    virtual ~CProAcceptor();

    virtual void PRO_CALLTYPE OnInput(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnError(
        PRO_INT64 sockId,
        long      errorCode
        )
    {
    }

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

private:

    const bool                                m_enableServiceExt;
    IProAcceptorObserver*                     m_observer;
    CProTpReactorTask*                        m_reactorTask;
    PRO_INT64                                 m_sockId;
    PRO_INT64                                 m_sockIdUn;
    pbsd_sockaddr_in                          m_localAddr;
    pbsd_sockaddr_un                          m_localAddrUn;
    unsigned long                             m_timeoutInSeconds;

    CProStlMap<IProTcpHandshaker*, PRO_NONCE> m_handshaker2Nonce;

    mutable CProThreadMutex                   m_lock;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_ACCEPTOR_H */
