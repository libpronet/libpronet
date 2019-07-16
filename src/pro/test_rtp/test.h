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

#if !defined(TEST_H)
#define TEST_H

#include "../pro_rtp/rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_shaper.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

class CTest
:
public IRtpSessionObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CTest* CreateInstance();

    bool Init(
        IProReactor*   reactor,
        const char*    remoteIp,
        unsigned short remotePort,
        const char*    localIp,
        unsigned short localPort,
        unsigned long  bitRate
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CTest();

    virtual ~CTest();

    virtual void PRO_CALLTYPE OnOkSession(IRtpSession* session);

    virtual void PRO_CALLTYPE OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        )
    {
    }

    virtual void PRO_CALLTYPE OnSendSession(
        IRtpSession* session,
        bool         packetErased
        )
    {
    }

    virtual void PRO_CALLTYPE OnCloseSession(
        IRtpSession* session,
        long         errorCode,
        long         sslCode,
        bool         tcpConnected
        )
    {
    }

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        );

private:

    IProReactor*    m_reactor;
    IRtpSession*    m_session;
    unsigned long   m_sendTimerId;
    unsigned long   m_recvTimerId;

    PRO_UINT16      m_outputSeq;
    PRO_INT64       m_outputTs64;
    PRO_UINT32      m_outputSsrc;
    CProShaper      m_outputShaper;

    CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* TEST_H */
