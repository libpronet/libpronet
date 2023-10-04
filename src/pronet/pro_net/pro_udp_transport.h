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

#ifndef PRO_UDP_TRANSPORT_H
#define PRO_UDP_TRANSPORT_H

#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_recv_pool.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProTpReactorTask;

/////////////////////////////////////////////////////////////////////////////
////

class CProUdpTransport : public IProTransport, public CProEventHandler
{
public:

    static CProUdpTransport* CreateInstance(
        bool   bindToLocal, /* = false */
        size_t recvPoolSize /* = 0 */
        );

    bool Init(
        IProTransportObserver* observer,
        CProTpReactorTask*     reactorTask,
        const char*            localIp,           /* = NULL */
        unsigned short         localPort,         /* = 0 */
        const char*            defaultRemoteIp,   /* = NULL */
        unsigned short         defaultRemotePort, /* = 0 */
        size_t                 sockBufSizeRecv,   /* = 0 */
        size_t                 sockBufSizeSend    /* = 0 */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    virtual PRO_TRANS_TYPE GetType() const
    {
        return PRO_TRANS_UDP;
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
        uint64_t                actionId,  /* ignored */
        const pbsd_sockaddr_in* remoteAddr /* = NULL */
        );

    virtual void RequestOnSend()
    {
    }

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

    virtual void UdpConnResetAsError(const pbsd_sockaddr_in* remoteAddr); /* = NULL */

protected:

    CProUdpTransport(
        bool   bindToLocal, /* = false */
        size_t recvPoolSize /* = 0 */
        );

    virtual ~CProUdpTransport();

protected:

    const bool              m_bindToLocal;
    const size_t            m_recvPoolSize;
    IProTransportObserver*  m_observer;
    CProTpReactorTask*      m_reactorTask;
    int64_t                 m_sockId;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_defaultRemoteAddr;
    CProRecvPool            m_recvPool;
    uint64_t                m_timerId;
    mutable CProThreadMutex m_lock;

private:

    virtual void OnInput(int64_t sockId);

    virtual void OnOutput(int64_t sockId)
    {
    }

    virtual void OnError(
        int64_t sockId,
        int     errorCode
        );

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  userData
        );

private:

    bool          m_connResetAsError;
    volatile bool m_canUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_UDP_TRANSPORT_H */
