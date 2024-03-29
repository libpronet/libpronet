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

#ifndef TEST_H
#define TEST_H

#include "../pro_rtp/rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_shaper.h"
#include "../pro_util/pro_stat.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * [[[[ modes
 */
typedef unsigned char TEST_MODE;

static const TEST_MODE TM_UDPE = 1;
static const TEST_MODE TM_TCPE = 2;
static const TEST_MODE TM_UDPS = 3;
static const TEST_MODE TM_TCPS = 4;
static const TEST_MODE TM_UDPC = 5;
static const TEST_MODE TM_TCPC = 6;
/*
 * ]]]]
 */

struct TEST_SERVER
{
    char           local_ip[64];
    unsigned short local_port;
    unsigned long  kbps;
    unsigned long  packet_size; /* offset is N */

    DECLARE_SGI_POOL(0)
};

struct TEST_CLIENT
{
    char           remote_ip[64];
    unsigned short remote_port;
    unsigned long  kbps;
    unsigned long  packet_size; /* offset is N */

    DECLARE_SGI_POOL(0)
};

union TEST_PARAMS
{
    TEST_SERVER server;
    TEST_CLIENT client;

    DECLARE_SGI_POOL(0)
};

struct TEST_PACKET_HDR
{
    TEST_PACKET_HDR()
    {
        version   = 1;
        ack       = false;
        reserved1 = 0;
        reserved2 = 0;
        srcTick   = ProGetTickCount64();
    }

    uint16_t version;
    bool     ack;
    char     reserved1;
    uint32_t reserved2;
    int64_t  srcTick;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CTest
:
public IRtpSessionObserver,
public IProOnTimer,
public CProRefCount
{
public:

    static CTest* CreateInstance(TEST_MODE mode);

    bool Init(
        IProReactor*       reactor,
        const TEST_SERVER& param
        );

    bool Init(
        IProReactor*       reactor,
        const TEST_CLIENT& param
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    static bool IsUdpMode(TEST_MODE mode)
    {
        return mode == TM_UDPE || mode == TM_UDPS || mode == TM_UDPC;
    }

    static bool IsServerMode(TEST_MODE mode)
    {
        return mode == TM_UDPE || mode == TM_TCPE || mode == TM_UDPS || mode == TM_TCPS;
    }

private:

    CTest(TEST_MODE mode);

    virtual ~CTest();

    virtual void OnOkSession(IRtpSession* session);

    virtual void OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        );

    virtual void OnSendSession(
        IRtpSession* session,
        bool         packetErased
        )
    {
    }

    virtual void OnCloseSession(
        IRtpSession* session,
        int          errorCode,
        int          sslCode,
        bool         tcpConnected
        );

    virtual void OnHeartbeatSession(
        IRtpSession* session,
        int64_t      peerAliveTick
        )
    {
    }

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        );

    IRtpSession* CreateUdpServer(
        IProReactor*   reactor,
        const char*    localIp,
        unsigned short localPort
        );

    IRtpSession* CreateTcpServer(
        IProReactor*   reactor,
        const char*    localIp,
        unsigned short localPort
        );

    IRtpSession* CreateUdpClient(
        IProReactor*   reactor,
        const char*    remoteIp,
        unsigned short remotePort
        );

    IRtpSession* CreateTcpClient(
        IProReactor*   reactor,
        const char*    remoteIp,
        unsigned short remotePort
        );

    void PrintSessionCreated(IRtpSession* session);

    void PrintSessionConnected(IRtpSession* session);

    void PrintSessionBroken(IRtpSession* session);

    void OnTimerSend(
        uint64_t timerId,
        int64_t  tick,
        bool&    tryAgain
        );

    void OnTimerRecv(
        uint64_t timerId,
        int64_t  tick
        );

    void OnTimerHtbt(
        uint64_t timerId,
        int64_t  tick
        );

private:

    const TEST_MODE  m_mode;
    TEST_PARAMS      m_params;
    IProReactor*     m_reactor;
    IRtpSession*     m_session;
    CProTimerFactory m_sender; /* a separate, dedicated thread */
    uint64_t         m_sendTimerId;
    uint64_t         m_recvTimerId;
    uint64_t         m_htbtTimerId;

    uint16_t         m_outputSeq;
    int64_t          m_outputTs64;
    uint32_t         m_outputSsrc;
    unsigned long    m_outputPacketCount;
    unsigned long    m_outputFrameSeq;
    CProShaper       m_outputShaper;
    bool             m_echoClient;
    CProStatAvgValue m_statRttxDelay;

    CProThreadMutex  m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* TEST_H */
