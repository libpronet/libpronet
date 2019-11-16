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

#include "rtp_session_udpclient.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_TRY_TIMES 100

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionUdpclient*
CRtpSessionUdpclient::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionUdpclient* const session = new CRtpSessionUdpclient(*localInfo);

    return (session);
}

CRtpSessionUdpclient::CRtpSessionUdpclient(const RTP_SESSION_INFO& localInfo)
: CRtpSessionBase(false)
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_UDPCLIENT;
}

CRtpSessionUdpclient::~CRtpSessionUdpclient()
{
    Fini();
}

bool
CRtpSessionUdpclient::Init(IRtpSessionObserver* observer,
                           IProReactor*         reactor,
                           const char*          localIp,   /* = NULL */
                           unsigned short       localPort) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    if (observer == NULL || reactor == NULL)
    {
        return (false);
    }

    unsigned long sockBufSizeRecv = 0;
    unsigned long sockBufSizeSend = 0;
    unsigned long recvPoolSize    = 0;
    GetRtpUdpSocketParams(
        m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL)
        {
            return (false);
        }

        int count = MAX_TRY_TIMES;
        if (localPort > 0)
        {
            count = 1;
        }

        for (int i = 0; i < count; ++i)
        {
            unsigned short localPort2 = localPort;
            if (localPort2 == 0)
            {
                localPort2 = AllocRtpUdpPort();
            }

            if (localPort2 % 2 == 0)
            {
                m_dummySockId = ProOpenUdpSockId(localIp, localPort2 + 1);
                if (m_dummySockId == -1)
                {
                    continue;
                }
            }

            m_trans = ProCreateUdpTransport(
                this, reactor, localIp, localPort2,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
            if (m_trans != NULL)
            {
                break;
            }

            ProCloseSockId(m_dummySockId);
            m_dummySockId = -1;
        }

        if (m_trans == NULL)
        {
            return (false);
        }

        char theIp[64] = "";
        m_localAddr.sin_family      = AF_INET;
        m_localAddr.sin_port        = pbsd_hton16(m_trans->GetLocalPort());
        m_localAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetLocalIp(theIp));

        m_trans->StartHeartbeat();

        observer->AddRef();
        m_observer    = observer;
        m_reactor     = reactor;
        m_onOkTimerId = reactor->ScheduleTimer(this, 0, false);
    }

    return (true);
}

void
CRtpSessionUdpclient::Fini()
{
    IRtpSessionObserver* observer = NULL;
    IProTransport*       trans    = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_onOkTimerId);
        m_onOkTimerId = 0;

        ProCloseSockId(m_dummySockId);
        m_dummySockId = -1;

        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteTransport(trans);
    observer->Release();
}

void
PRO_CALLTYPE
CRtpSessionUdpclient::SetRemoteIpAndPort(const char*    remoteIp,   /* = NULL */
                                         unsigned short remotePort) /* = 0 */
{
    pbsd_sockaddr_in remoteAddrConfig;
    memset(&remoteAddrConfig, 0, sizeof(pbsd_sockaddr_in));
    remoteAddrConfig.sin_family      = AF_INET;
    remoteAddrConfig.sin_port        = pbsd_hton16(remotePort);
    remoteAddrConfig.sin_addr.s_addr = pbsd_inet_aton(remoteIp); /* DNS */

    if (remoteAddrConfig.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        m_remoteAddrConfig = remoteAddrConfig;
    }
}

void
PRO_CALLTYPE
CRtpSessionUdpclient::OnRecv(IProTransport*          trans,
                             const pbsd_sockaddr_in* remoteAddr)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(trans != NULL);
    assert(remoteAddr != NULL);
    if (trans == NULL || remoteAddr == NULL)
    {
        return;
    }

    while (1)
    {
        IRtpSessionObserver* observer = NULL;
        CRtpPacket*          packet   = NULL;
        bool                 error    = false;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
            {
                return;
            }

            if (trans != m_trans)
            {
                return;
            }

            IProRecvPool&       recvPool = *m_trans->GetRecvPool();
            const unsigned long dataSize = recvPool.PeekDataSize();

            if (dataSize == 0)
            {
                break;
            }

            if (m_remoteAddr.sin_addr.s_addr != 0
                &&
                (remoteAddr->sin_addr.s_addr != m_remoteAddr.sin_addr.s_addr ||
                 remoteAddr->sin_port        != m_remoteAddr.sin_port)
                &&
                (remoteAddr->sin_addr.s_addr != m_remoteAddrConfig.sin_addr.s_addr ||
                 remoteAddr->sin_port        != m_remoteAddrConfig.sin_port)) /* check */
            {
                recvPool.Flush(dataSize);
                break;
            }

            packet = CRtpPacket::CreateInstance(dataSize, RTP_EPM_DEFAULT);
            if (packet == NULL)
            {
                error = true;
            }
            else
            {
                recvPool.PeekData(packet->GetPayloadBuffer(), dataSize);

                RTP_HEADER  hdr;
                const char* payloadBuffer = NULL;
                PRO_UINT16  payloadSize   = 0;

                const bool ret = CRtpPacket::ParseRtpBuffer(
                    (char*)packet->GetPayloadBuffer(),
                    packet->GetPayloadSize16(),
                    hdr,
                    payloadBuffer,
                    payloadSize
                    );
                if (!ret)
                {
                    packet->Release();
                    packet = NULL;
                    recvPool.Flush(dataSize);
                    break;
                }

                m_peerAliveTick = ProGetTickCount64();
                m_remoteAddr    = *remoteAddr; /* rebind */

                if (payloadBuffer == NULL || payloadSize == 0) /* h.460 */
                {
                    packet->Release();
                    packet = NULL;
                    recvPool.Flush(dataSize);
                    break;
                }

                hdr.v  = 2;
                hdr.p  = 0;
                hdr.x  = 0;
                hdr.cc = 0;

                RTP_PACKET& magicPacket = packet->GetPacket();

                magicPacket.ext = (RTP_EXT*)(payloadBuffer - sizeof(RTP_HEADER) - sizeof(RTP_EXT));
                magicPacket.hdr = (RTP_HEADER*)(magicPacket.ext + 1);

                magicPacket.ext->mmId              = pbsd_hton32(m_info.inSrcMmId);
                magicPacket.ext->mmType            = m_info.mmType;
                magicPacket.ext->hdrAndPayloadSize = pbsd_hton16(sizeof(RTP_HEADER) + payloadSize);

                *magicPacket.hdr = hdr;
            }

            recvPool.Flush(dataSize);

            m_observer->AddRef();
            observer = m_observer;
        }

        if (m_canUpcall)
        {
            if (error)
            {
                m_canUpcall = false;
                observer->OnCloseSession(this, -1, 0, m_tcpConnected);
            }
            else
            {
                if (!m_onOkCalled)
                {
                    m_onOkCalled = true;
                    observer->OnOkSession(this);
                }

                if (packet != NULL)
                {
                    observer->OnRecvSession(this, packet);
                }
            }
        }

        if (packet != NULL)
        {
            packet->Release();
        }

        observer->Release();

        if (!m_canUpcall)
        {
            Fini();
        }
        break;
    } /* end of while (...) */
}}
