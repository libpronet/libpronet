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

#if !defined(PRO_SSL_HANDSHAKER_H)
#define PRO_SSL_HANDSHAKER_H

#include "pro_event_handler.h"
#include "pro_recv_pool.h"
#include "pro_send_pool.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class  CProTpReactorTask;
class  IProSslHandshakerObserver;
struct PRO_SSL_CTX;

/////////////////////////////////////////////////////////////////////////////
////

class CProSslHandshaker : public CProEventHandler
{
public:

    static CProSslHandshaker* CreateInstance();

    bool Init(
        IProSslHandshakerObserver* observer,
        CProTpReactorTask*         reactorTask,
        PRO_SSL_CTX*               ctx,
        int64_t                    sockId,
        bool                       unixSocket,
        const void*                sendData,        /* = NULL */
        size_t                     sendDataSize,    /* = 0 */
        size_t                     recvDataSize,    /* = 0 */
        bool                       recvFirst,       /* = false */
        unsigned long              timeoutInSeconds /* = 0 */
        );

    void Fini();

private:

    CProSslHandshaker();

    virtual ~CProSslHandshaker();

    virtual void OnInput(int64_t sockId);

    virtual void OnOutput(int64_t sockId);

    virtual void OnError(
        int64_t sockId,
        long    errorCode
        );

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  userData
        );

    void DoRecv(int64_t sockId);

    void DoSend(int64_t sockId);

private:

    IProSslHandshakerObserver* m_observer;
    CProTpReactorTask*         m_reactorTask;
    PRO_SSL_CTX*               m_ctx;
    bool                       m_sslOk;
    int64_t                    m_sockId;
    bool                       m_unixSocket;
    bool                       m_onWr;
    bool                       m_recvFirst;
    CProRecvPool               m_recvPool;
    CProSendPool               m_sendPool;
    uint64_t                   m_timerId;
    CProThreadMutex            m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SSL_HANDSHAKER_H */
