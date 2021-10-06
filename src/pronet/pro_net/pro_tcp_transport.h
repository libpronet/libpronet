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

#if !defined(PRO_TCP_TRANSPORT_H)
#define PRO_TCP_TRANSPORT_H

#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "pro_send_pool.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class  CProTpReactorTask;
struct PRO_SERVICE_PACKET;

class IProTransportObserverEx : public IProTransportObserver
{
public:

    virtual ~IProTransportObserverEx() {}

    virtual void PRO_CALLTYPE OnRecvFd(
        IProTransport*            trans,
        PRO_INT64                 fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

class CProTcpTransport : public IProTransport, public CProEventHandler
{
public:

    static CProTcpTransport* CreateInstance(
        bool   recvFdMode,
        size_t recvPoolSize /* = 0 */
        );

    bool Init(
        IProTransportObserver* observer,
        CProTpReactorTask*     reactorTask,
        PRO_INT64              sockId,
        bool                   unixSocket,
        size_t                 sockBufSizeRecv, /* = 0 */
        size_t                 sockBufSizeSend, /* = 0 */
        bool                   suspendRecv      /* = false */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    virtual PRO_TRANS_TYPE PRO_CALLTYPE GetType() const
    {
        return (PRO_TRANS_TCP);
    }

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(
        char suiteName[64]
        ) const;

    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const;

    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetLocalPort() const;

    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetRemotePort() const;

    virtual IProRecvPool* PRO_CALLTYPE GetRecvPool()
    {
        return (&m_recvPool);
    }

    virtual bool PRO_CALLTYPE SendData(
        const void*             buf,
        size_t                  size,
        PRO_UINT64              actionId,  /* = 0 */
        const pbsd_sockaddr_in* remoteAddr /* = NULL */
        );

    virtual void PRO_CALLTYPE RequestOnSend();

    virtual void PRO_CALLTYPE SuspendRecv();

    virtual void PRO_CALLTYPE ResumeRecv();

    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp)
    {
        return (false);
    }

    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp)
    {
    }

    virtual void PRO_CALLTYPE StartHeartbeat();

    virtual void PRO_CALLTYPE StopHeartbeat();

    virtual void PRO_CALLTYPE UdpConnResetAsError(
        const pbsd_sockaddr_in* remoteAddr /* = NULL */
        )
    {
    }

    bool SendFd(const PRO_SERVICE_PACKET& s2cPacket);

protected:

    CProTcpTransport(
        bool   recvFdMode,
        size_t recvPoolSize /* = 0 */
        );

    virtual ~CProTcpTransport();

    virtual void PRO_CALLTYPE OnInput(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnOutput(PRO_INT64 sockId);

    virtual void PRO_CALLTYPE OnError(
        PRO_INT64 sockId,
        long      errorCode
        );

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    void OnInputData(PRO_INT64 sockId);

    void OnInputFd(PRO_INT64 sockId);

    void OnOutput(
        PRO_INT64 sockId,
        bool&     tryAgain
        );

protected:

    const bool              m_recvFdMode;
    const size_t            m_recvPoolSize;
    IProTransportObserver*  m_observer;
    CProTpReactorTask*      m_reactorTask;
    PRO_INT64               m_sockId;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_remoteAddr;
    bool                    m_onWr;
    volatile bool           m_pendingWr;
    bool                    m_requestOnSend;
    CProRecvPool            m_recvPool;
    CProSendPool            m_sendPool;
    PRO_INT64               m_sendingFd;
    PRO_UINT64              m_timerId;
    mutable CProThreadMutex m_lock;

    volatile bool           m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_TCP_TRANSPORT_H */
