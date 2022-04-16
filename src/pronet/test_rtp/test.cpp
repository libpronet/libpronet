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

#define SOCK_BUF_SIZE_RECV     (1024 * 1024 * 2) /* a double size buffer will be used on Linux */
#define SOCK_BUF_SIZE_SEND     (1024 * 1024 * 2) /* a double size buffer will be used on Linux */
#define FRAME_PACKETS          10
#define GOP_FRAMES             30
#define REDLINE_BYTES          (1024 * 1024 * 64)
#define MEDIA_PAYLOAD_TYPE     109
#define MEDIA_MM_TYPE          RTP_MMT_MSG
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
    m_session           = NULL;
    m_sendTimerId       = 0;
    m_recvTimerId       = 0;
    m_htbtTimerId       = 0;

    m_outputSeq         = 0;
    m_outputTs64        = -1;
    m_outputSsrc        = (PRO_UINT32)(ProRand_0_1() * (PRO_UINT32)-1);
    m_outputPacketCount = 0;
    m_outputFrameSeq    = 0;
    m_echoClient        = false;

    memset(&m_params, 0, sizeof(TEST_PARAMS));
    m_sender.Start(false);
}

CTest::~CTest()
{
    Fini();
    m_sender.Stop();
}

bool
CTest::Init(IProReactor*       reactor,
            const TEST_SERVER& param)
{
    assert(reactor != NULL);
    assert(IsServerMode(m_mode));
    if (reactor == NULL || !IsServerMode(m_mode))
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_session == NULL);
        if (m_reactor != NULL || m_session != NULL)
        {
            return (false);
        }

        if (m_mode == TM_UDPE || m_mode == TM_UDPS)
        {
            m_session = CreateUdpServer(
                reactor, param.local_ip, param.local_port);
        }
        else
        {
            m_session = CreateTcpServer(
                reactor, param.local_ip, param.local_port);
        }

        if (m_session == NULL)
        {
            return (false);
        }

        m_params.server   = param;
        m_reactor         = reactor;
        if (m_mode != TM_UDPE && m_mode != TM_TCPE)
        {
            m_sendTimerId = m_sender.ScheduleTimer(this, SEND_TIMER_MS, true, 1); /* 1 --- send */
        }
        m_recvTimerId     = reactor->ScheduleTimer(this, RECV_TIMER_MS, true, 2); /* 2 --- recv */
        m_htbtTimerId     = reactor->ScheduleTimer(this, HTBT_TIMER_MS, true, 3); /* 3 --- htbt */
        m_outputShaper.SetAvgBitRate((double)param.kbps * 1000);
    }

    return (true);
}

bool
CTest::Init(IProReactor*       reactor,
            const TEST_CLIENT& param)
{
    assert(reactor != NULL);
    assert(!IsServerMode(m_mode));
    if (reactor == NULL || IsServerMode(m_mode))
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_session == NULL);
        if (m_reactor != NULL || m_session != NULL)
        {
            return (false);
        }

        if (m_mode == TM_UDPC)
        {
            m_session = CreateUdpClient(
                reactor, param.remote_ip, param.remote_port);
        }
        else
        {
            m_session = CreateTcpClient(
                reactor, param.remote_ip, param.remote_port);
        }

        if (m_session == NULL)
        {
            return (false);
        }

        m_params.client   = param;
        m_reactor         = reactor;
        if (m_mode != TM_UDPE && m_mode != TM_TCPE)
        {
            m_sendTimerId = m_sender.ScheduleTimer(this, SEND_TIMER_MS, true, 1); /* 1 --- send */
        }
        m_recvTimerId     = reactor->ScheduleTimer(this, RECV_TIMER_MS, true, 2); /* 2 --- recv */
        m_htbtTimerId     = reactor->ScheduleTimer(this, HTBT_TIMER_MS, true, 3); /* 3 --- htbt */
        m_outputShaper.SetAvgBitRate((double)param.kbps * 1000);
    }

    return (true);
}

IRtpSession*
CTest::CreateUdpServer(IProReactor*   reactor,
                       const char*    localIp,
                       unsigned short localPort)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (NULL);
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
    if (localIp != NULL)
    {
        strncpy_pro(initArgs.udpserverEx.localIp,
            sizeof(initArgs.udpserverEx.localIp), localIp);
    }

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_UDPSERVER_EX, &initArgs, &localInfo);
    if (session != NULL)
    {
        const PRO_INT64 sockId = session->GetSockId();

        int option;
        option = (int)SOCK_BUF_SIZE_RECV;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)SOCK_BUF_SIZE_SEND;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

        session->SetOutputRedline(REDLINE_BYTES, 0, 0);
        PrintSessionCreated(session);
    }

    return (session);
}

IRtpSession*
CTest::CreateTcpServer(IProReactor*   reactor,
                       const char*    localIp,
                       unsigned short localPort)
{
    assert(reactor != NULL);
    if (reactor == NULL)
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
    if (localIp != NULL)
    {
        strncpy_pro(initArgs.tcpserver.localIp,
            sizeof(initArgs.tcpserver.localIp), localIp);
    }

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_TCPSERVER, &initArgs, &localInfo);
    if (session != NULL)
    {
        session->SetOutputRedline(REDLINE_BYTES, 0, 0);
        PrintSessionCreated(session);
    }

    return (session);
}

IRtpSession*
CTest::CreateUdpClient(IProReactor*   reactor,
                       const char*    remoteIp,
                       unsigned short remotePort)
{
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    if (reactor == NULL || remoteIp == NULL || remoteIp[0] == '\0' ||
        remotePort == 0)
    {
        return (NULL);
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
    strncpy_pro(initArgs.udpclientEx.remoteIp,
        sizeof(initArgs.udpclientEx.remoteIp), remoteIp);

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_UDPCLIENT_EX, &initArgs, &localInfo);
    if (session != NULL)
    {
        const PRO_INT64 sockId = session->GetSockId();

        int option;
        option = (int)SOCK_BUF_SIZE_RECV;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)SOCK_BUF_SIZE_SEND;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

        session->SetOutputRedline(REDLINE_BYTES, 0, 0);
        PrintSessionCreated(session);
    }

    return (session);
}

IRtpSession*
CTest::CreateTcpClient(IProReactor*   reactor,
                       const char*    remoteIp,
                       unsigned short remotePort)
{
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    if (reactor == NULL || remoteIp == NULL || remoteIp[0] == '\0' ||
        remotePort == 0)
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
    strncpy_pro(initArgs.tcpclient.remoteIp,
        sizeof(initArgs.tcpclient.remoteIp), remoteIp);

    IRtpSession* const session = CreateRtpSessionWrapper(
        RTP_ST_TCPCLIENT, &initArgs, &localInfo);
    if (session != NULL)
    {
        session->SetOutputRedline(REDLINE_BYTES, 0, 0);
        PrintSessionCreated(session);
    }

    return (session);
}

void
CTest::Fini()
{
    IRtpSession* session = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        m_sender.CancelTimer(m_sendTimerId);
        m_reactor->CancelTimer(m_recvTimerId);
        m_reactor->CancelTimer(m_htbtTimerId);
        m_sendTimerId = 0;
        m_recvTimerId = 0;
        m_htbtTimerId = 0;

        session = m_session;
        m_session = NULL;
        m_reactor = NULL;
    }

    DeleteRtpSessionWrapper(session);
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
CTest::PrintSessionCreated(IRtpSession* session)
{{{
    assert(session != NULL);

    char           localIp[64]  = "";
    char           remoteIp[64] = "";
    unsigned short localPort    = 0;
    unsigned short remotePort   = 0;
    session->GetLocalIp(localIp);
    session->GetRemoteIp(remoteIp);
    localPort  = session->GetLocalPort();
    remotePort = session->GetRemotePort();

    char status[64] = "";
    if (IsServerMode(m_mode))
    {
        static int s_count = 0;
        sprintf(status, "waiting...[%d]", ++s_count);
    }
    else
    {
        strcpy(status, "connecting...");
    }

    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

    printf(
        "\n\n"
        "%s \n"
        " test_rtp [ver-%d.%d.%d] --- [%s, Lc-%s:%u, Rm-%s:%u] --- %s \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        IsUdpMode(m_mode) ? "UDP" : "TCP",
        localIp,
        (unsigned int)localPort,
        remoteIp,
        (unsigned int)remotePort,
        status
        );
}}}

void
CTest::PrintSessionConnected(IRtpSession* session)
{{{
    assert(session != NULL);

    char           localIp[64]  = "";
    char           remoteIp[64] = "";
    unsigned short localPort    = 0;
    unsigned short remotePort   = 0;
    session->GetLocalIp(localIp);
    session->GetRemoteIp(remoteIp);
    localPort  = session->GetLocalPort();
    remotePort = session->GetRemotePort();

    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

    printf(
        "\n\n"
        "%s \n"
        " test_rtp [ver-%d.%d.%d] --- [%s, Lc-%s:%u, Rm-%s:%u] --- connected! \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        IsUdpMode(m_mode) ? "UDP" : "TCP",
        localIp,
        (unsigned int)localPort,
        remoteIp,
        (unsigned int)remotePort
        );
}}}

void
CTest::PrintSessionBroken(IRtpSession* session)
{{{
    assert(session != NULL);

    char           localIp[64]  = "";
    char           remoteIp[64] = "";
    unsigned short localPort    = 0;
    unsigned short remotePort   = 0;
    session->GetLocalIp(localIp);
    session->GetRemoteIp(remoteIp);
    localPort  = session->GetLocalPort();
    remotePort = session->GetRemotePort();

    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

    printf(
        "\n\n"
        "%s \n"
        " test_rtp [ver-%d.%d.%d] --- [%s, Lc-%s:%u, Rm-%s:%u] --- broken! \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        IsUdpMode(m_mode) ? "UDP" : "TCP",
        localIp,
        (unsigned int)localPort,
        remoteIp,
        (unsigned int)remotePort
        );
}}}

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

        if (m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        m_session->SetMagic(ProGetTickCount64());
        PrintSessionConnected(m_session);
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

    TEST_PACKET_HDR* const ptr = (TEST_PACKET_HDR*)packet->GetPayloadBuffer();

    TEST_PACKET_HDR hdr = *ptr;
    hdr.version = pbsd_ntoh16(hdr.version);
    hdr.srcTick = pbsd_ntoh64(hdr.srcTick);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        const PRO_INT64 tick = ProGetTickCount64();
        m_session->SetMagic(tick);

        if (m_mode == TM_UDPE || m_mode == TM_TCPE)
        {
            ptr->ack = true;

            packet->SetSequence(m_outputSeq++);
            m_session->SendPacket(packet);
        }
        else if (m_mode == TM_UDPC || m_mode == TM_TCPC)
        {
            if (hdr.ack && tick - hdr.srcTick >= 0)
            {
                m_statRttxDelay.PushData((double)(tick - hdr.srcTick));
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

        if (m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (session != m_session)
        {
            return;
        }

        PrintSessionBroken(m_session);

        /*
         * reset
         */
        m_outputSeq         = 0;
        m_outputTs64        = -1;
        m_outputPacketCount = 0;
        m_outputFrameSeq    = 0;

        m_session = NULL;
    }

    DeleteRtpSessionWrapper(session);
}

void
PRO_CALLTYPE
CTest::OnTimer(void*      factory,
               PRO_UINT64 timerId,
               PRO_INT64  userData)
{
    if (userData == 1)
    {
        bool tryAgain = false;

        do
        {
            OnTimerSend(timerId, tryAgain);
        }
        while (tryAgain);
    }
    else if (userData == 2)
    {
        OnTimerRecv(timerId);
    }
    else if (userData == 3)
    {
        OnTimerHtbt(timerId);
    }
    else
    {
    }
}

void
CTest::OnTimerSend(PRO_UINT64 timerId,
                   bool&      tryAgain)
{
    tryAgain = false;

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

        if (timerId != m_sendTimerId)
        {
            return;
        }

        if (m_session == NULL || !m_session->IsReady())
        {
            return;
        }

        double packetSize = m_params.server.packet_size;
        if (packetSize > m_outputShaper.GetAvgBitRate() / 8 - 1)
        {
            packetSize = m_outputShaper.GetAvgBitRate() / 8 - 1;
        }
        if (packetSize < sizeof(TEST_PACKET_HDR))
        {
            packetSize = sizeof(TEST_PACKET_HDR);
        }

        if (m_outputShaper.CalcGreenBits() < packetSize * 8)
        {
            return;
        }

        unsigned long cachedBytes = 0;
        m_session->GetFlowctrlInfo(
            NULL, NULL, NULL, NULL, &cachedBytes, NULL);
        if (cachedBytes >= REDLINE_BYTES)
        {
            return;
        }

        IRtpPacket* const packet =
            CreateRtpPacketSpace((unsigned long)packetSize);
        if (packet == NULL)
        {
            return;
        }

        ++m_outputPacketCount;
        m_outputShaper.FlushGreenBits(packetSize * 8);

        const PRO_INT64 tick = ProGetTickCount64();

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
        packet->SetKeyFrame(m_outputFrameSeq % GOP_FRAMES == 0);
        if (packet->GetMarker())
        {
            m_outputTs64 = -1;
            ++m_outputFrameSeq;
        }

        TEST_PACKET_HDR hdr;
        hdr.version = pbsd_hton16(hdr.version);
        hdr.srcTick = pbsd_hton64(tick);

        TEST_PACKET_HDR* const ptr =
            (TEST_PACKET_HDR*)packet->GetPayloadBuffer();
        *ptr = hdr;

        m_session->SendPacket(packet);
        packet->Release();
    }

    tryAgain = true;
}

void
CTest::OnTimerRecv(PRO_UINT64 timerId)
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

        if (timerId != m_recvTimerId)
        {
            return;
        }

        if (m_session == NULL || !m_session->IsReady())
        {
            return;
        }

        float         srcFrameRate   = 0;
        float         srcBitRate     = 0;
        float         outFrameRate   = 0;
        float         outBitRate     = 0;
        unsigned long cachedBytes    = 0;
        unsigned long cachedFrames   = 0;
        float         inputFrameRate = 0;
        float         inputBitRate   = 0;
        float         inputLossRate  = 0;
        PRO_UINT64    inputLossCount = 0;
        unsigned long rttxDelay      = 0;

        m_session->GetFlowctrlInfo(&srcFrameRate, &srcBitRate,
            &outFrameRate, &outBitRate, &cachedBytes, &cachedFrames);
        m_session->GetInputStat(&inputFrameRate, &inputBitRate,
            &inputLossRate, &inputLossCount);

        if (m_mode == TM_UDPE || m_mode == TM_TCPE)
        {{{
            printf(
                "\n"
                " SEND(s/o) : %.1f/%.1f(kbps) %u/%u(pps) [%u(KB)] [%u]"
                "\t RECV : %.1f(kbps) %u(pps)"
                "\t LOSS : %.2f%% [" PRO_PRT64U "] \n"
                ,
                srcBitRate / 1000,
                outBitRate / 1000,
                (unsigned int)srcFrameRate,
                (unsigned int)outFrameRate,
                (unsigned int)(cachedBytes / 1024),
                (unsigned int)cachedFrames,
                inputBitRate / 1000,
                (unsigned int)inputFrameRate,
                inputLossRate * 100,
                inputLossCount
                );
        }}}
        else if (m_echoClient)
        {{{
            rttxDelay = (unsigned long)m_statRttxDelay.CalcAvgValue();

            printf(
                "\n"
                " SEND(s/o) : %.1f/%.1f(kbps) %u/%u(pps) [%u(KB)] [%u]"
                "\t RECV : %.1f(kbps) %u(pps)"
                "\t LOSS : %.2f%% [" PRO_PRT64U "]"
                "\t RTT' : %u(ms) \n"
                ,
                srcBitRate / 1000,
                outBitRate / 1000,
                (unsigned int)srcFrameRate,
                (unsigned int)outFrameRate,
                (unsigned int)(cachedBytes / 1024),
                (unsigned int)cachedFrames,
                inputBitRate / 1000,
                (unsigned int)inputFrameRate,
                inputLossRate * 100,
                inputLossCount,
                (unsigned int)rttxDelay
                );
        }}}
        else
        {{{
            printf(
                "\n"
                " SEND(s/o) : %.1f/%.1f(kbps) %u/%u(pps) [%u(KB)] [%u]"
                "\t RECV : %.1f(kbps) %u(pps)"
                "\t LOSS : %.2f%% [" PRO_PRT64U "] \n"
                ,
                srcBitRate / 1000,
                outBitRate / 1000,
                (unsigned int)srcFrameRate,
                (unsigned int)outFrameRate,
                (unsigned int)(cachedBytes / 1024),
                (unsigned int)cachedFrames,
                inputBitRate / 1000,
                (unsigned int)inputFrameRate,
                inputLossRate * 100,
                inputLossCount
                );
        }}}
    }
}

void
CTest::OnTimerHtbt(PRO_UINT64 timerId)
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

        if (timerId != m_htbtTimerId)
        {
            return;
        }

        /*
         * delete old
         */
        if (m_session != NULL && IsUdpMode(m_mode))
        {
            const PRO_INT64 magic = m_session->GetMagic();
            if (magic > 0 &&
                ProGetTickCount64() - magic >= TRAFFIC_BROKEN_TIMEOUT * 1000)
            {
                PrintSessionBroken(m_session);

                DeleteRtpSessionWrapper(m_session);
                m_session = NULL;
            }
        }

        /*
         * create new
         */
        if (m_session == NULL && IsServerMode(m_mode))
        {
            if (m_mode == TM_UDPE || m_mode == TM_UDPS)
            {
                m_session = CreateUdpServer(m_reactor,
                    m_params.server.local_ip, m_params.server.local_port);
            }
            else
            {
                m_session = CreateTcpServer(m_reactor,
                    m_params.server.local_ip, m_params.server.local_port);
            }
        }
    }
}
