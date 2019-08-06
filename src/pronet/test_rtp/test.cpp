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
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32)
#include <windows.h>
#endif

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define RECV_BUF_SIZE     (1024 * 1024 * 2)
#define SEND_BUF_SIZE     (1024 * 1024 * 2)
#define SEND_PACKET_SIZE  1024
#define SEND_PAYLOAD_TYPE 109
#define SEND_TIMER_MS     1
#define RECV_TIMER_MS     1000

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance()
{
    CTest* const tester = new CTest;

    return (tester);
}

CTest::CTest()
{
    m_reactor     = NULL;
    m_session     = NULL;
    m_sendTimerId = 0;
    m_recvTimerId = 0;

    m_outputSeq   = 0;
    m_outputTs64  = -1;
    m_outputSsrc  = (PRO_UINT32)(ProRand_0_1() * (PRO_UINT32)-1);
}

CTest::~CTest()
{
    Fini();
}

bool
CTest::Init(IProReactor*   reactor,
            const char*    remoteIp,
            unsigned short remotePort,
            const char*    localIp,
            unsigned short localPort,
            unsigned long  bitRate)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (false);
    }

    CProStlString theLocalIp = "";
    if (localIp == NULL || localIp[0] == '\0')
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

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_session == NULL);
        if (m_reactor != NULL || m_session != NULL)
        {
            return (false);
        }

        RTP_SESSION_INFO localInfo;
        memset(&localInfo, 0, sizeof(RTP_SESSION_INFO));
        localInfo.mmType = RTP_MMT_VIDEO;

        RTP_INIT_ARGS initArgs;
        memset(&initArgs, 0, sizeof(RTP_INIT_ARGS));
        initArgs.udpclient.observer  = this;
        initArgs.udpclient.reactor   = reactor;
        initArgs.udpclient.localPort = localPort;
        strncpy_pro(initArgs.udpclient.localIp,
            sizeof(initArgs.udpclient.localIp), theLocalIp.c_str());

        m_session = CreateRtpSessionWrapper(RTP_ST_UDPCLIENT, &initArgs, &localInfo);
        if (m_session == NULL)
        {
            return (false);
        }

        const PRO_INT64 sockId = m_session->GetSockId();
        if (sockId != -1)
        {
            int option = RECV_BUF_SIZE;
            pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
            option = SEND_BUF_SIZE;
            pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
        }

        m_reactor = reactor;
        m_session->SetRemoteIpAndPort(remoteIp, remotePort);
        m_outputShaper.SetMaxBitRate(bitRate);
    }

    return (true);
}

void
CTest::Fini()
{
    IRtpSession* session = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        m_reactor->CancelMmTimer(m_sendTimerId);
        m_reactor->CancelTimer(m_recvTimerId);
        m_sendTimerId = 0;
        m_recvTimerId = 0;

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

        char           localIp[64] = "";
        unsigned short localPort   = 0;
        m_session->GetLocalIp(localIp);
        localPort = m_session->GetLocalPort();

        printf(
            " test_rtp[ver-%d.%d.%d] --- [listen on %s:%u] --- ok! \n\n"
            ,
            PRO_VER_MAJOR,
            PRO_VER_MINOR,
            PRO_VER_PATCH,
            localIp,
            (unsigned int)localPort
            );

#if defined(WIN32)
        char info[256] = "";
        sprintf(info, "test_rtp --- listen on %s:%u", localIp, (unsigned int)localPort);
        ::SetConsoleTitle(info);
#endif

        m_sendTimerId = m_reactor->ScheduleMmTimer(this, SEND_TIMER_MS, true);
        m_recvTimerId = m_reactor->ScheduleTimer  (this, RECV_TIMER_MS, true);
    }
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

        if (m_reactor == NULL || m_session == NULL)
        {
            return;
        }

        if (timerId == m_sendTimerId)
        {
            double packetSize = SEND_PACKET_SIZE;
            if (m_outputShaper.GetMaxBitRate() <= 64000)
            {
                packetSize = 160;
            }

            bool marker = false;

            while (!marker)
            {
                double greenBits = m_outputShaper.CalcGreenBits();
                if (greenBits < packetSize * 8)
                {
                    break;
                }

                m_outputShaper.FlushGreenBits(packetSize * 8);

                greenBits = m_outputShaper.CalcGreenBits();
                if (greenBits < packetSize * 8)
                {
                    marker = true;
                }

                IRtpPacket* const packet = CreateRtpPacketSpace((unsigned long)packetSize);
                if (packet == NULL)
                {
                    break;
                }

                if (m_outputTs64 == -1)
                {
                    m_outputTs64 = ProGetTickCount64() * 90;
                }
                packet->SetMarker(marker);
                packet->SetPayloadType(SEND_PAYLOAD_TYPE);
                packet->SetSequence(m_outputSeq++);
                packet->SetTimeStamp((PRO_UINT32)m_outputTs64);
                packet->SetSsrc(m_outputSsrc);
                packet->SetMmType(RTP_MMT_VIDEO);
                if (marker)
                {
                    m_outputTs64 = -1;
                }

                m_session->SendPacket(packet);
                packet->Release();
            } /* end of while (...) */
        }
        else if (timerId == m_recvTimerId)
        {
            float outputBitRate  = 0;
            float inputBitRate   = 0;
            float inputLossRate  = 0;
            float inputLossCount = 0;
            m_session->GetOutputStat(NULL, &outputBitRate, NULL, NULL);
            m_session->GetInputStat(NULL, &inputBitRate, &inputLossRate, &inputLossCount);

            printf(
                " SEND : %9.1f(kbps)\t RECV : %9.1f(kbps)\t LOSS : %4.1f%% [%u] \n\n"
                ,
                outputBitRate / 1000,
                inputBitRate  / 1000,
                inputLossRate * 100,
                (unsigned int)inputLossCount
                );
        }
        else
        {
        }
    }
}
