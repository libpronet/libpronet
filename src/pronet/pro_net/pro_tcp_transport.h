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

#ifndef PRO_TCP_TRANSPORT_H
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

    virtual void OnRecvFd(
        IProTransport*            trans,
        int64_t                   fd,
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
        int64_t                sockId,
        bool                   unixSocket,
        size_t                 sockBufSizeRecv, /* = 0 */
        size_t                 sockBufSizeSend, /* = 0 */
        bool                   suspendRecv      /* = false */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    virtual PRO_TRANS_TYPE GetType() const
    {
        return PRO_TRANS_TCP;
    }

    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const;

    virtual int64_t GetSockId() const;

    virtual const char* GetLocalIp(char localIp[64]) const;

    virtual unsigned short GetLocalPort() const;

    virtual const char* GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short GetRemotePort() const;

    virtual IProRecvPool* GetRecvPool()
    {
        return &m_recvPool;
    }

    virtual bool SendData(
        const void*             buf,
        size_t                  size,
        uint64_t                actionId,  /* = 0 */
        const pbsd_sockaddr_in* remoteAddr /* = NULL */
        );

    virtual void RequestOnSend();

    virtual void SuspendRecv();

    virtual void ResumeRecv();

    virtual bool AddMcastReceiver(const char* mcastIp)
    {
        return false;
    }

    virtual void RemoveMcastReceiver(const char* mcastIp)
    {
    }

    virtual void StartHeartbeat();

    virtual void StopHeartbeat();

    virtual void UdpConnResetAsError(const pbsd_sockaddr_in* remoteAddr) /* = NULL */
    {
    }

    bool SendFd(const PRO_SERVICE_PACKET& s2cPacket);

protected:

    CProTcpTransport(
        bool   recvFdMode,
        size_t recvPoolSize /* = 0 */
        );

    virtual ~CProTcpTransport();

    virtual void OnInput(int64_t sockId);

    virtual void OnOutput(int64_t sockId);

    virtual void OnError(
        int64_t sockId,
        int     errorCode
        );

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

    void OnInputData(int64_t sockId);

    void OnInputFd(int64_t sockId);

    void OnOutput(
        int64_t sockId,
        bool&   tryAgain
        );

protected:

    const bool              m_recvFdMode;
    const size_t            m_recvPoolSize;
    IProTransportObserver*  m_observer;
    CProTpReactorTask*      m_reactorTask;
    int64_t                 m_sockId;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_remoteAddr;
    bool                    m_onWr;
    volatile bool           m_pendingWr;
    bool                    m_requestOnSend;
    CProRecvPool            m_recvPool;
    CProSendPool            m_sendPool;
    int64_t                 m_sendingFd;
    uint64_t                m_timerId;
    mutable CProThreadMutex m_lock;

    volatile bool           m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_TCP_TRANSPORT_H */
