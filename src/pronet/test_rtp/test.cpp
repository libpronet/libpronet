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

#include "test.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_shaper.h"
#include "../pro_util/pro_stat.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#if defined(USING_VIDEO_BUCKET)
#define FRAME_PACKETS          10
#else
#define FRAME_PACKETS          1
#endif
#define GOP_FRAMES             30

#define RECV_BUF_SIZE          (1024 * 1024 * 2) /* for udp */
#define SEND_BUF_SIZE          (1024 * 1024 * 2) /* for udp */
#define STD_PACKET_SIZE        1024
#define LOW_PACKET_SIZE        160
#define LOW_BIT_RATE           64000
#define REDLINE_BYTES          (1024 * 512)
#define REDLINE_FRAMES         (REDLINE_BYTES / STD_PACKET_SIZE / FRAME_PACKETS + 1)
#define REDLINE_DELAY_MS       1200
#define MEDIA_PAYLOAD_TYPE     109
#define MEDIA_MM_TYPE          RTP_MMT_VIDEO
#define SEND_TIMER_MS          1
#define RECV_TIMER_MS          1000
#define HTBT_TIMER_MS          100
#define UDP_SERVER_TIMEOUT     20
#define UDP_CLIENT_TIMEOUT     10
#define TCP_SERVER_TIMEOUT     3600
#define TCP_CLIENT_TIMEOUT     20
#define TRAFFIC_BROKEN_TIMEOUT 5

/////////////////////////////////////////////////////////////////////////////
////

static
bool
PRO_CALLTYPE
CheckMode_i(TEST_MODE mode)
{
    bool ret = false;

    switch (mode)
    {
    case TM_UDPE:
    case TM_TCPE:
    case TM_UDPS:
    case TM_TCPS:
    case TM_UDPC:
    case TM_TCPC:
        {
            ret = true;
            break;
        }
    }

    return (ret);
}

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance(TEST_MODE mode)
{
    assert(CheckMode_i(mode));
    if (!CheckMode_i(mode))
    {
        return (NULL);
    }

    CTest* const tester = new CTest(mode);

    return (tester);
}

CTest::CTest(TEST_MODE mode)
: m_mode(mode)
{
    m_reactor           = NULL;
    m_udpSession        = NULL;
    m_tcpSession        = NULL;
    m_sendTimerId       = 0;
    m_recvTimerId       = 0;
    m_htbtTimerId       = 0;

    m_outputSeq         = 0;
    m_outputTs64        = -1;
    m_outputSsrc        = (PRO_UINT32)(ProRand_0_1() * (PRO_UINT32)-1);
    m_outputPacketCount = 0;
    m_outputFrameIndex  = 0;
    m_echoClient        = false;

    memset(&m_params, 0, sizeof(TEST_PARAMS));
}

CTest::~CTest()
{
    Fini();
}

bool
CTest::Init(IProReactor*       reactor,
            const TEST_SERVER& param)
{
    assert(reactor != NULL);
    assert(
        m_mode == TM_UDPE || m_mode == TM_TCPE ||
        m_mode == TM_UDPS || m_mode == TM_TCPS
        );
    if (
        reactor == NULL
        ||
        (m_mode != TM_UDPE && m_mode != TM_TCPE &&
         m_mode != TM_UDPS && m_mode != TM_TCPS)
       )
    {
        return (false);
    }

    IRtpSession* udpSession = NULL;
    IRtpSession* tcpSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_udpSession == NULL);
        assert(m_tcpSession == NULL);
        if (m_reactor != NULL || m_udpSession != NULL || m_tcpSession != NULL)
        {
            return (false);
        }

        if (m_mode == TM_UDPE || m_mode == TM_UDPS)
        {
            udpSession = CreateUdpServer(
                reactor, param.local_ip, param.local_port, true);
            if (udpSession == NULL)
            {
                goto EXIT;
            }
        }
        else
        {
            tcpSession = CreateTcpServer(
                reactor, param.local_ip, param.local_port, true);
            if (tcpSession == NULL)
            {
                goto EXIT;
            }
        }

        m_params.server = param;
        m_reactor       = reactor;
        m_udpSession    = udpSession;
        m_tcpSession    = tcpSession;
        m_sendTimerId   = reactor->ScheduleTimer(this, SEND_TIMER_MS, true);
        m_recvTimerId   = reactor->ScheduleTimer(this, RECV_TIMER_MS, true);
        m_htbtTimerId   = reactor->ScheduleTimer(this, HTBT_TIMER_MS, true);
        m_outputShaper.SetMaxBitRate(param.kbps * 1000);
    }

    return (true);

EXIT:

    DeleteRtpSessionWrapper(tcpSession);
    DeleteRtpSessionWrapper(udpSession);

    return (false);
}

bool
CTest::Init(IProReactor*       reactor,
            const TEST_CLIENT& param)
{
    assert(reactor != NULL);
    assert(m_mode == TM_UDPC || m_mode == TM_TCPC);
    if (
        reactor == NULL
        ||
        (m_mode != TM_UDPC && m_mode != TM_TCPC)
       )
    {
        return (false);
    }

    IRtpSession* udpSession = NULL;
    IRtpSession* tcpSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_udpSession == NULL);
        assert(m_tcpSession == NULL);
        if (m_reactor != NULL || m_udpSession != NULL || m_tcpSession != NULL)
        {
            return (false);
        }

        if (m_mode == TM_UDPC)
        {
            udpSession = CreateUdpClient(reactor,
                param.remote_ip, param.remote_port, param.local_ip, true);
            if (udpSession == NULL)
            {
                goto EXIT;
            }
        }
        else
        {
            tcpSession = CreateTcpClient(reactor,
                param.remote_ip, param.remote_port, param.local_ip, true);
            if (tcpSession == NULL)
            {
                goto EXIT;
            }
        }

        m_params.client = param;
        m_reactor       = reactor;
        m_udpSession    = udpSession;
        m_tcpSession    = tcpSession;
        m_sendTimerId   = reactor->ScheduleTimer(this, SEND_TIMER_MS, true);
        m_recvTimerId   = reactor->ScheduleTimer(this, RECV_TIMER_MS, true);
        m_htbtTimerId   = reactor->ScheduleTimer(this, HTBT_TIMER_MS, true);
        m_outputShaper.SetMaxBitRate(param.kbps * 1000);
    }

    return (true);

EXIT:

    DeleteRtpSessionWrapper(tcpSession);
    DeleteRtpSessionWrapper(udpSession);

    return (false);
}

IRtpSession*
CTest::CreateUdpServer(IProReactor*   reactor,
                       const char*    localIp,
                       unsigned short localPort,
                       bool           printReady)
{
    assert(reactor != NULL);
    assert(localIp != NULL);
    assert(localPort > 0);
    if (reactor == NULL || localIp == NULL || localPort == 0)
    {
        return (NULL);
    }

    CProStlString theLocalIp = "";
    if (localIp[0] == '\0')
    {
        char localFirstIp[64] = "";
        ProGetLocalFirstIp(localFirstIp);
        theLocalIp = localFirstIp;
    }
    else
    {
        const PRO_UINT32 localIp2 = pbsd_inet_aton(localIp);
        if (localIp2 == (PRO_UINT32)-1 || localIp2 == 0)
        {
            char localFirstIp[64] = "";
            ProGetLocalFirstIp(localFirstIp);
            theLocalIp = localFirstIp;
        }
        else
        {
            theLocalIp = localIp;
        }
    }

    RTP_SESSION_INFO localInfo;
    memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
    localInfo.mmType = MEDIA_MM_TYPE;

    RTP_INIT_ARGS initArgs;
    memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));
    initArgs.udpserverEx.observer         = this;
    initArgs.udpserverEx.reactor          = reactor;
    initArgs.udpserverEx.localPort        = localPort;
    initArgs.udpserverEx.timeoutInSeconds = UDP_SERVER_TIMEOUT;
#if defined(USING_VIDEO_BUCKET)
    initArgs.udpserverEx.bucket           = CreateRtpVideoBucket();
#else
    initArgs.udpserverEx.bucket           = CreateRtpBaseBucket();
#endif
    strncpy_pro(initArgs.udpserverEx.localIp,
        sizeof(initArgs.udpserverEx.localIp), theLocalIp.c_str());

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_UDPSERVER_EX, &initArgs, &localInfo);
    if (session != NULL)
    {
        const PRO_INT64 sockId = session->GetSockId();
        if (sockId != -1)
        {
            int option;
            option = RECV_BUF_SIZE;
            pbsd_setsockopt(
                sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
            option = SEND_BUF_SIZE;
            pbsd_setsockopt(
                sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
        }

        session->SetOutputRedline(
            REDLINE_BYTES, REDLINE_FRAMES, REDLINE_DELAY_MS);

        if (printReady)
        {
            PrintSessionReady(session, true);
        }
    }

    return (session);
}

IRtpSession*
CTest::CreateTcpServer(IProReactor*   reactor,
                       const char*    localIp,
                       unsigned short localPort,
                       bool           printReady)
{
    assert(reactor != NULL);
    assert(localIp != NULL);
    assert(localPort > 0);
    if (reactor == NULL || localIp == NULL || localPort == 0)
    {
        return (NULL);
    }

    RTP_SESSION_INFO localInfo;
    memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
    localInfo.mmType = MEDIA_MM_TYPE;

    RTP_INIT_ARGS initArgs;
    memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));
    initArgs.tcpserver.observer         = this;
    initArgs.tcpserver.reactor          = reactor;
    initArgs.tcpserver.localPort        = localPort;
    initArgs.tcpserver.timeoutInSeconds = TCP_SERVER_TIMEOUT;
    initArgs.tcpserver.bucket           = CreateRtpBaseBucket();
    strncpy_pro(initArgs.tcpserver.localIp,
        sizeof(initArgs.tcpserver.localIp), localIp);

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_TCPSERVER, &initArgs, &localInfo);
    if (session != NULL)
    {
        session->SetOutputRedline(
            REDLINE_BYTES, REDLINE_FRAMES, REDLINE_DELAY_MS);

        if (printReady)
        {
            PrintSessionReady(session, false);
        }
    }

    return (session);
}

IRtpSession*
CTest::CreateUdpClient(IProReactor*   reactor,
                       const char*    remoteIp,
                       unsigned short remotePort,
                       const char*    localIp,
                       bool           printReady)
{
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    assert(localIp != NULL);
    if (reactor == NULL || remoteIp == NULL || remoteIp[0] == '\0' ||
        remotePort == 0 || localIp == NULL)
    {
        return (NULL);
    }

    CProStlString theLocalIp = "";
    if (localIp[0] == '\0')
    {
        char localFirstIp[64] = "";
        ProGetLocalFirstIp(localFirstIp, remoteIp);
        theLocalIp = localFirstIp;
    }
    else
    {
        const PRO_UINT32 localIp2 = pbsd_inet_aton(localIp);
        if (localIp2 == (PRO_UINT32)-1 || localIp2 == 0)
        {
            char localFirstIp[64] = "";
            ProGetLocalFirstIp(localFirstIp, remoteIp);
            theLocalIp = localFirstIp;
        }
        else
        {
            theLocalIp = localIp;
        }
    }

    RTP_SESSION_INFO localInfo;
    memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
    localInfo.mmType = MEDIA_MM_TYPE;

    RTP_INIT_ARGS initArgs;
    memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));
    initArgs.udpclientEx.observer         = this;
    initArgs.udpclientEx.reactor          = reactor;
    initArgs.udpclientEx.remotePort       = remotePort;
    initArgs.udpclientEx.timeoutInSeconds = UDP_CLIENT_TIMEOUT;
#if defined(USING_VIDEO_BUCKET)
    initArgs.udpclientEx.bucket           = CreateRtpVideoBucket();
#else
    initArgs.udpclientEx.bucket           = CreateRtpBaseBucket();
#endif
    strncpy_pro(initArgs.udpclientEx.remoteIp,
        sizeof(initArgs.udpclientEx.remoteIp), remoteIp);
    strncpy_pro(initArgs.udpclientEx.localIp,
        sizeof(initArgs.udpclientEx.localIp), theLocalIp.c_str());

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_UDPCLIENT_EX, &initArgs, &localInfo);
    if (session != NULL)
    {
        const PRO_INT64 sockId = session->GetSockId();
        if (sockId != -1)
        {
            int option;
            option = RECV_BUF_SIZE;
            pbsd_setsockopt(
                sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
            option = SEND_BUF_SIZE;
            pbsd_setsockopt(
                sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
        }

        session->SetOutputRedline(
            REDLINE_BYTES, REDLINE_FRAMES, REDLINE_DELAY_MS);

        if (printReady)
        {
            PrintSessionReady(session, true);
        }
    }

    return (session);
}

IRtpSession*
CTest::CreateTcpClient(IProReactor*   reactor,
                       const char*    remoteIp,
                       unsigned short remotePort,
                       const char*    localIp,
                       bool           printReady)
{
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    assert(localIp != NULL);
    if (reactor == NULL || remoteIp == NULL || remoteIp[0] == '\0' ||
        remotePort == 0 || localIp == NULL)
    {
        return (NULL);
    }

    RTP_SESSION_INFO localInfo;
    memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
    localInfo.mmType = MEDIA_MM_TYPE;

    RTP_INIT_ARGS initArgs;
    memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));
    initArgs.tcpclient.observer         = this;
    initArgs.tcpclient.reactor          = reactor;
    initArgs.tcpclient.remotePort       = remotePort;
    initArgs.tcpclient.timeoutInSeconds = TCP_CLIENT_TIMEOUT;
    initArgs.tcpclient.bucket           = CreateRtpBaseBucket();
    strncpy_pro(initArgs.tcpclient.remoteIp,
        sizeof(initArgs.tcpclient.remoteIp), remoteIp);
    strncpy_pro(initArgs.tcpclient.localIp,
        sizeof(initArgs.tcpclient.localIp), localIp);

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_TCPCLIENT, &initArgs, &localInfo);
    if (session != NULL)
    {
        session->SetOutputRedline(
            REDLINE_BYTES, REDLINE_FRAMES, REDLINE_DELAY_MS);

        if (printReady)
        {
            PrintSessionReady(session, false);
        }
    }

    return (session);
}

void
CTest::PrintSessionReady(IRtpSession* session,
                         bool         udp)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    char           localIp[64]  = "";
    char           remoteIp[64] = "";
    unsigned short localPort    = 0;
    unsigned short remotePort   = 0;
    session->GetLocalIp(localIp);
    session->GetRemoteIp(remoteIp);
    localPort  = session->GetLocalPort();
    remotePort = session->GetRemotePort();

    char status[64] = "";
    if (m_mode == TM_UDPE || m_mode == TM_TCPE ||
        m_mode == TM_UDPS || m_mode == TM_TCPS)
    {
        static int count = 0;
        sprintf(status, "waiting...[%d]", ++count);
    }
    else
    {
        strcpy(status, "connecting...");
    }

    printf(
        "\n test_rtp [ver-%d.%d.%d] --- [%s, Lc-%s:%u, Rm-%s:%u] --- %s \n"
        ,
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        udp ? "UDP" : "TCP",
        localIp,
        (unsigned int)localPort,
        remoteIp,
        (unsigned int)remotePort,
        status
        );
}

void
CTest::PrintSessionBroken(IRtpSession* session,
                          bool         udp)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    char           localIp[64]  = "";
    char           remoteIp[64] = "";
    unsigned short localPort    = 0;
    unsigned short remotePort   = 0;
    session->GetLocalIp(localIp);
    session->GetRemoteIp(remoteIp);
    localPort  = session->GetLocalPort();
    remotePort = session->GetRemotePort();

    printf(
        "\n test_rtp [ver-%d.%d.%d] --- [%s, Lc-%s:%u, Rm-%s:%u] --- broken! \n"
        ,
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        udp ? "UDP" : "TCP",
        localIp,
        (unsigned int)localPort,
        remoteIp,
        (unsigned int)remotePort
        );
}

void
CTest::Fini()
{
    IRtpSession* udpSession = NULL;
    IRtpSession* tcpSession = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_sendTimerId);
        m_reactor->CancelTimer(m_recvTimerId);
        m_reactor->CancelTimer(m_htbtTimerId);
        m_sendTimerId = 0;
        m_recvTimerId = 0;
        m_htbtTimerId = 0;

        tcpSession = m_tcpSession;
        udpSession = m_udpSession;
        m_tcpSession = NULL;
        m_udpSession = NULL;
        m_reactor = NULL;
    }

    DeleteRtpSessionWrapper(tcpSession);
    DeleteRtpSessionWrapper(udpSession);
}

unsigned long
PRO_CALLTYPE
CTest::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CTest::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CTest::OnOkSession(IRtpSession* session)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        char           localIp[64]  = "";
        char           remoteIp[64] = "";
        unsigned short localPort    = 0;
        unsigned short remotePort   = 0;
        session->GetLocalIp(localIp);
        session->GetRemoteIp(remoteIp);
        localPort  = session->GetLocalPort();
        remotePort = session->GetRemotePort();

        if (session == m_udpSession)
        {
            printf(
                "\n test_rtp [ver-%d.%d.%d] --- [UDP, Lc-%s:%u, Rm-%s:%u] --- connected! \n\n"
                ,
                PRO_VER_MAJOR,
                PRO_VER_MINOR,
                PRO_VER_PATCH,
                localIp,
                (unsigned int)localPort,
                remoteIp,
                (unsigned int)remotePort
                );
        }
        else if (session == m_tcpSession)
        {
            printf(
                "\n test_rtp [ver-%d.%d.%d] --- [TCP, Lc-%s:%u, Rm-%s:%u] --- connected! \n\n"
                ,
                PRO_VER_MAJOR,
                PRO_VER_MINOR,
                PRO_VER_PATCH,
                localIp,
                (unsigned int)localPort,
                remoteIp,
                (unsigned int)remotePort
                );
        }
        else
        {
            return;
        }

        session->SetMagic(ProGetTickCount64());
    }
}

void
PRO_CALLTYPE
CTest::OnRecvSession(IRtpSession* session,
                     IRtpPacket*  packet)
{
    assert(session != NULL);
    assert(packet != NULL);
    assert(packet->GetPayloadSize() >= sizeof(TEST_PACKET_HDR));
    if (session == NULL || packet == NULL ||
        packet->GetPayloadSize() < sizeof(TEST_PACKET_HDR))
    {
        return;
    }

    TEST_PACKET_HDR* const ptr =
        (TEST_PACKET_HDR*)packet->GetPayloadBuffer();

    TEST_PACKET_HDR hdr = *ptr;
    hdr.version = pbsd_ntoh16(hdr.version);
    hdr.srcSeq  = pbsd_ntoh16(hdr.srcSeq);
    hdr.srcTick = pbsd_ntoh64(hdr.srcTick);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (session != m_udpSession && session != m_tcpSession)
        {
            return;
        }

        const PRO_INT64 tick = ProGetTickCount64();
        session->SetMagic(tick);

        if (m_mode == TM_UDPE || m_mode == TM_TCPE)
        {
            ptr->ack = true;
            session->SendPacket(packet);
        }
        else if (m_mode == TM_UDPC || m_mode == TM_TCPC)
        {
            if (hdr.ack)
            {
                m_statRttLossRate.PushData(hdr.srcSeq);
                m_statRttDelay.PushData((double)(tick - hdr.srcTick));
            }

            m_echoClient = hdr.ack;
        }
        else
        {
        }
    }
}

void
PRO_CALLTYPE
CTest::OnCloseSession(IRtpSession* session,
                      long         errorCode,
                      long         sslCode,
                      bool         tcpConnected)
{
    assert(session != NULL);
    if (session == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        if (session == m_udpSession)
        {
            m_udpSession = NULL;
            PrintSessionBroken(session, true);
        }
        else if (session == m_tcpSession)
        {
            m_tcpSession = NULL;
            PrintSessionBroken(session, false);
        }
        else
        {
            return;
        }

        /*
         * reset
         */
        m_outputSeq         = 0;
        m_outputTs64        = -1;
        m_outputPacketCount = 0;
        m_outputFrameIndex  = 0;
    }

    DeleteRtpSessionWrapper(session);
}

void
PRO_CALLTYPE
CTest::OnTimer(unsigned long timerId,
               PRO_INT64     userData)
{
    assert(timerId > 0);
    if (timerId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        const PRO_INT64 tick = ProGetTickCount64();

        if (timerId == m_sendTimerId)
        {
            if (m_mode == TM_UDPE || m_mode == TM_TCPE)
            {
                return;
            }

            IRtpSession* const session =
                m_udpSession != NULL ? m_udpSession : m_tcpSession;
            if (session == NULL || !session->IsReady())
            {
                return;
            }

            double packetSize = STD_PACKET_SIZE;
            if (m_outputShaper.GetMaxBitRate() <= LOW_BIT_RATE)
            {
                packetSize = LOW_PACKET_SIZE;
            }
            if (packetSize > m_outputShaper.GetMaxBitRate() / 8 - 1)
            {
                packetSize = m_outputShaper.GetMaxBitRate() / 8 - 1;
            }

            while (1)
            {
                const double greenBits = m_outputShaper.CalcGreenBits();
                if (greenBits < packetSize * 8)
                {
                    break;
                }

                unsigned long cachedBytes  = 0;
                unsigned long cachedFrames = 0;
                session->GetFlowctrlInfo(
                    NULL, NULL, NULL, NULL, &cachedBytes, &cachedFrames);
                if (cachedBytes  >= REDLINE_BYTES ||
                    cachedFrames >= REDLINE_FRAMES)
                {
                    break;
                }

                IRtpPacket* const packet =
                    CreateRtpPacketSpace((unsigned long)packetSize);
                if (packet == NULL)
                {
                    break;
                }

                ++m_outputPacketCount;
                m_outputShaper.FlushGreenBits(packetSize * 8);

                if (m_outputTs64 == -1)
                {
                    m_outputTs64 = tick * 90;
                    packet->SetFirstPacketOfFrame(true);
                }
                packet->SetMarker(m_outputPacketCount % FRAME_PACKETS == 0);
                packet->SetPayloadType(MEDIA_PAYLOAD_TYPE);
                packet->SetSequence(m_outputSeq++);
                packet->SetTimeStamp((PRO_UINT32)m_outputTs64);
                packet->SetSsrc(m_outputSsrc);
                packet->SetMmType(MEDIA_MM_TYPE);
                packet->SetKeyFrame(m_outputFrameIndex % GOP_FRAMES == 0);
                if (packet->GetMarker())
                {
                    m_outputTs64 = -1;
                    ++m_outputFrameIndex;
                }

                TEST_PACKET_HDR hdr;
                hdr.version = pbsd_hton16(hdr.version);
                hdr.srcSeq  = pbsd_hton16(packet->GetSequence());
                hdr.srcTick = pbsd_hton64(tick);
                memcpy(packet->GetPayloadBuffer(), &hdr,
                    sizeof(TEST_PACKET_HDR));

                session->SendPacket(packet);
                packet->Release();
            } /* end of while (...) */
        }
        else if (timerId == m_recvTimerId)
        {
            IRtpSession* const session =
                m_udpSession != NULL ? m_udpSession : m_tcpSession;
            if (session == NULL || !session->IsReady())
            {
                return;
            }

            float outputBitRate  = 0;
            float inputBitRate   = 0;
            float inputLossRate  = 0;
            float inputLossCount = 0;
            float rttDelay       = 0;

            session->GetOutputStat(NULL, &outputBitRate, NULL, NULL);
            session->GetInputStat(
                NULL, &inputBitRate, &inputLossRate, &inputLossCount);

            if (m_mode == TM_UDPE || m_mode == TM_TCPE)
            {
                /*
                 * using a single line
                 */
                printf(
                    "\r SEND : %9.1f(kbps)\t RECV : %9.1f(kbps)\t"
                    " LOSS : %5.2f%% [%u] "
                    ,
                    outputBitRate / 1000,
                    inputBitRate  / 1000,
                    inputLossRate * 100,
                    (unsigned int)inputLossCount
                    );
                fflush(stdout);
            }
            else if (m_echoClient)
            {
                inputLossRate  = (float)m_statRttLossRate.CalcLossRate();
                inputLossCount = (float)m_statRttLossRate.CalcLossCount();
                rttDelay       = (float)m_statRttDelay.CalcAvgValue();

                printf(
                    "\n SEND : %9.1f(kbps)\t RECV : %9.1f(kbps)\t"
                    " LOSS : %5.2f%% [%u]\t RTT' : %u(ms) \n"
                    ,
                    outputBitRate / 1000,
                    inputBitRate  / 1000,
                    inputLossRate * 100,
                    (unsigned int)inputLossCount,
                    (unsigned int)rttDelay
                    );
            }
            else
            {
                printf(
                    "\n SEND : %9.1f(kbps)\t RECV : %9.1f(kbps)\t"
                    " LOSS : %5.2f%% [%u] \n"
                    ,
                    outputBitRate / 1000,
                    inputBitRate  / 1000,
                    inputLossRate * 100,
                    (unsigned int)inputLossCount
                    );
            }
        }
        else if (timerId == m_htbtTimerId)
        {
            /*
             * delete
             */
            if (m_mode == TM_UDPE || m_mode == TM_UDPS || m_mode == TM_UDPC)
            {
                if (m_udpSession != NULL)
                {
                    const PRO_INT64 magic = m_udpSession->GetMagic();
                    if (magic > 0 &&
                        tick - magic >= TRAFFIC_BROKEN_TIMEOUT * 1000)
                    {
                        PrintSessionBroken(m_udpSession, true);
                        DeleteRtpSessionWrapper(m_udpSession);
                        m_udpSession = NULL;
                    }
                }
            }

            /*
             * create
             */
            if (m_mode == TM_UDPE || m_mode == TM_UDPS)
            {
                if (m_udpSession == NULL)
                {
                    m_udpSession = CreateUdpServer(
                        m_reactor,
                        m_params.server.local_ip,
                        m_params.server.local_port,
                        true
                        );
                }
            }
            else if (m_mode == TM_TCPE || m_mode == TM_TCPS)
            {
                if (m_tcpSession == NULL)
                {
                    m_tcpSession = CreateTcpServer(
                        m_reactor,
                        m_params.server.local_ip,
                        m_params.server.local_port,
                        true
                        );
                }
            }
            else
            {
            }
        }
        else
        {
        }
    }
}
