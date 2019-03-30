/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (http://www.libpro.org)
 */

#include "rtp_session_udpserver_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define UDP_HANDSHAKE_BYTES 1400
#define MAX_TRY_TIMES       100
#define DEFAULT_TIMEOUT     20

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionUdpserverEx*
CRtpSessionUdpserverEx::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionUdpserverEx* const session = new CRtpSessionUdpserverEx(*localInfo);

    return (session);
}

CRtpSessionUdpserverEx::CRtpSessionUdpserverEx(const RTP_SESSION_INFO& localInfo)
{
    m_info               = localInfo;
    m_info.localVersion  = 0;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_UDPSERVER_EX;
}

CRtpSessionUdpserverEx::~CRtpSessionUdpserverEx()
{
    Fini();
}

bool
CRtpSessionUdpserverEx::Init(IRtpSessionObserver* observer,
                             IProReactor*         reactor,
                             const char*          localIp,          /* = NULL */
                             unsigned short       localPort,        /* = 0 */
                             unsigned long        timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    if (observer == NULL || reactor == NULL)
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
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

            m_trans = ProCreateUdpTransport(this, reactor, localIp, localPort2,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
            if (m_trans != NULL)
            {
                break;
            }
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
        m_observer       = observer;
        m_reactor        = reactor;
        m_timeoutTimerId = reactor->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false);
    }

    return (true);
}

void
CRtpSessionUdpserverEx::Fini()
{
    IRtpSessionObserver* observer = NULL;
    IProTransport*       trans    = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timeoutTimerId);
        m_timeoutTimerId = 0;

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
CRtpSessionUdpserverEx::OnRecv(IProTransport*          trans,
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

            if (dataSize < sizeof(RTP_EXT))
            {
                recvPool.Flush(dataSize);
                break;
            }

            RTP_EXT ext;
            recvPool.PeekData(&ext, sizeof(RTP_EXT));
            ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);
            if (dataSize < sizeof(RTP_EXT) + ext.hdrAndPayloadSize)
            {
                recvPool.Flush(dataSize);
                break;
            }

            if (ext.hdrAndPayloadSize == 0)
            {
                if (remoteAddr->sin_addr.s_addr == m_remoteAddr.sin_addr.s_addr &&
                    remoteAddr->sin_port        == m_remoteAddr.sin_port)
                {
                    m_peerAliveTick = ProGetTickCount64();
                }

                recvPool.Flush(sizeof(RTP_EXT));
                continue;
            }

            if (!m_handshakeOk)
            {
                assert(ext.hdrAndPayloadSize == sizeof(RTP_HEADER) + UDP_HANDSHAKE_BYTES);
                if (ext.hdrAndPayloadSize != sizeof(RTP_HEADER) + UDP_HANDSHAKE_BYTES)
                {
                    recvPool.Flush(dataSize);
                    break;
                }

                const PRO_UINT16 size = sizeof(RTP_EXT) + sizeof(RTP_HEADER) + UDP_HANDSHAKE_BYTES;
                char             buffer[size];

                recvPool.PeekData(buffer, size);

                if (!CRtpPacket::ParseExtBuffer(buffer, size))
                {
                    recvPool.Flush(dataSize);
                    break;
                }

                recvPool.Flush(size);
                m_remoteAddr  = *remoteAddr; /* bind */
                m_handshakeOk = true;

                m_reactor->CancelTimer(m_timeoutTimerId);
                m_timeoutTimerId = 0;

                if (!DoHandshake())
                {
                    error = true;
                }
            }
            else
            {
                if (remoteAddr->sin_addr.s_addr != m_remoteAddr.sin_addr.s_addr ||
                    remoteAddr->sin_port        != m_remoteAddr.sin_port)
                {
                    recvPool.Flush(dataSize);
                    break;
                }

                m_peerAliveTick = ProGetTickCount64();

                packet = CRtpPacket::CreateInstance(
                    sizeof(RTP_EXT) + ext.hdrAndPayloadSize, RTP_EPM_DEFAULT);
                if (packet == NULL)
                {
                    error = true;
                }
                else
                {
                    recvPool.PeekData(
                        packet->GetPayloadBuffer(), sizeof(RTP_EXT) + ext.hdrAndPayloadSize);

                    if (!CRtpPacket::ParseExtBuffer(
                        (char*)packet->GetPayloadBuffer(), packet->GetPayloadSize16()))
                    {
                        packet->Release();
                        packet = NULL;
                        recvPool.Flush(dataSize);
                        break;
                    }

                    RTP_PACKET& magicPacket = packet->GetPacket();

                    magicPacket.ext = (RTP_EXT*)packet->GetPayloadBuffer();
                    magicPacket.hdr = (RTP_HEADER*)(magicPacket.ext + 1);

                    assert(m_info.inSrcMmId == 0 || packet->GetMmId() == m_info.inSrcMmId);
                    assert(packet->GetMmType() == m_info.mmType);
                    if (m_info.inSrcMmId != 0 && packet->GetMmId() != m_info.inSrcMmId
                        ||
                        packet->GetMmType() != m_info.mmType)
                    {
                        packet->Release();
                        packet = NULL;
                        recvPool.Flush(dataSize);
                        break;
                    }
                }

                recvPool.Flush(sizeof(RTP_EXT) + ext.hdrAndPayloadSize);
            }

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
            else if (m_handshakeOk)
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
            else
            {
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
            break;
        }
    } /* end of while (...) */
}}

bool
CRtpSessionUdpserverEx::DoHandshake()
{
    assert(m_trans != NULL);
    if (m_trans == NULL)
    {
        return (false);
    }

    IRtpPacket* const packet = CreateRtpPacketSpace(UDP_HANDSHAKE_BYTES);
    if (packet == NULL)
    {
        return (false);
    }

    memset(packet->GetPayloadBuffer(), 0, packet->GetPayloadSize());
    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);

    const bool ret = m_trans->SendData(
        (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT),
        packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT)
        );
    packet->Release();

    return (ret);
}
