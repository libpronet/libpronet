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

#include "rtp_session_udpserver_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_TRY_TIMES         100
#define SEND_SYNC_INTERVAL_MS 150
#define DEFAULT_TIMEOUT       10

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionUdpserverEx*
CRtpSessionUdpserverEx::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return NULL;
    }

    return new CRtpSessionUdpserverEx(*localInfo);
}

CRtpSessionUdpserverEx::CRtpSessionUdpserverEx(const RTP_SESSION_INFO& localInfo)
: CRtpSessionBase(false)
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_UDPSERVER_EX;

    m_syncReceived       = false;
    memset(&m_syncToPeer, 0, sizeof(RTP_UDPX_SYNC));
    m_syncToPeer.version = pbsd_hton16(RTP_SESSION_PROTOCOL_VERSION);
    m_syncTimerId        = 0;
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
                             unsigned int         timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    if (observer == NULL || reactor == NULL)
    {
        return false;
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    size_t sockBufSizeRecv = 0; /* zero by default */
    size_t sockBufSizeSend = 0; /* zero by default */
    size_t recvPoolSize    = 0;
    GetRtpUdpSocketParams(m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL)
        {
            return false;
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
                localPort2 = AllocRtpUdpPort(false); /* rfc is false */
            }

            m_trans = ProCreateUdpTransport(this, reactor, true, /* bindToLocal is true */
                localIp, localPort2, sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
            if (m_trans != NULL)
            {
                break;
            }
        }

        if (m_trans == NULL)
        {
            return false;
        }

        char theIp[64] = "";
        m_localAddr.sin_family      = AF_INET;
        m_localAddr.sin_port        = pbsd_hton16(m_trans->GetLocalPort());
        m_localAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetLocalIp(theIp));

        m_trans->StartHeartbeat();

        observer->AddRef();
        m_observer       = observer;
        m_reactor        = reactor;
        m_timeoutTimerId = reactor->SetupTimer(this, (uint64_t)timeoutInSeconds * 1000, 0);
        m_syncTimerId    = reactor->SetupTimer(this, SEND_SYNC_INTERVAL_MS, SEND_SYNC_INTERVAL_MS);
    }

    return true;
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
        m_reactor->CancelTimer(m_syncTimerId);
        m_timeoutTimerId = 0;
        m_syncTimerId    = 0;

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
CRtpSessionUdpserverEx::GetSyncId(unsigned char syncId[14]) const
{
    CProThreadMutexGuard mon(m_lock);

    memcpy(syncId, m_syncToPeer.nonce, 14);
}

void
CRtpSessionUdpserverEx::OnRecv(IProTransport*          trans,
                               const pbsd_sockaddr_in* remoteAddr)
{
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

            IProRecvPool& recvPool = *m_trans->GetRecvPool();
            size_t        dataSize = recvPool.PeekDataSize();

            if (
                m_syncReceived
                &&
                (remoteAddr->sin_addr.s_addr != m_remoteAddr.sin_addr.s_addr ||
                 remoteAddr->sin_port        != m_remoteAddr.sin_port)
               )
            {
                recvPool.Flush(dataSize);
                break;
            }

            if (dataSize < sizeof(RTP_EXT))
            {
                recvPool.Flush(dataSize);
                break;
            }

            RTP_EXT ext;
            recvPool.PeekData(&ext, sizeof(RTP_EXT));
            ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);
            if (dataSize != sizeof(RTP_EXT) + ext.hdrAndPayloadSize)
            {
                recvPool.Flush(dataSize);
                break;
            }

            if (!m_syncReceived || ext.udpxSync)
            {
                /*
                 * The packet must be a sync packet.
                 */
                if (ext.hdrAndPayloadSize != sizeof(RTP_HEADER) + sizeof(RTP_UDPX_SYNC) ||
                    !ext.udpxSync)
                {
                    recvPool.Flush(dataSize);
                    break;
                }

                const uint16_t size = sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_UDPX_SYNC);
                char           buffer[size];

                recvPool.PeekData(buffer, size);
                recvPool.Flush(dataSize);

                if (!CRtpPacket::ParseExtBuffer(buffer, size))
                {
                    break;
                }

                m_peerAliveTick = ProGetTickCount64();

                RTP_UDPX_SYNC sync;
                memcpy(
                    &sync,
                    buffer + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
                    sizeof(RTP_UDPX_SYNC)
                    );
                if (pbsd_hton16(sync.CalcChecksum()) != sync.checksum)
                {
                    break;
                }

                if (m_syncReceived)
                {
                    break;
                }

                memcpy(
                    (char*)&m_syncToPeer + sizeof(uint16_t),
                    (char*)&sync + sizeof(uint16_t),
                    sizeof(RTP_UDPX_SYNC) - sizeof(uint16_t)
                    );

                m_info.remoteVersion = pbsd_ntoh16(sync.version);
                m_remoteAddr         = *remoteAddr; /* bind */
                m_syncReceived       = true;

                if (!DoHandshake2())
                {
                    error = true;
                }
            }
            else
            {
                if (ext.hdrAndPayloadSize == 0)
                {
                    m_peerAliveTick = ProGetTickCount64();

                    recvPool.Flush(dataSize);
                    if (m_handshakeOk)
                    {
                        break;
                    }
                }
                else
                {
                    packet = CRtpPacket::CreateInstance(
                        sizeof(RTP_EXT) + ext.hdrAndPayloadSize, RTP_EPM_DEFAULT);
                    if (packet == NULL)
                    {
                        recvPool.Flush(dataSize);
                        error = true;
                    }
                    else
                    {
                        recvPool.PeekData(
                            packet->GetPayloadBuffer(),
                            sizeof(RTP_EXT) + ext.hdrAndPayloadSize
                            );
                        recvPool.Flush(dataSize);

                        if (!CRtpPacket::ParseExtBuffer(
                            (char*)packet->GetPayloadBuffer(),
                            packet->GetPayloadSize16()
                            ))
                        {
                            packet->Release();
                            packet = NULL;
                            break;
                        }

                        m_peerAliveTick = ProGetTickCount64();

                        RTP_PACKET& magicPacket = packet->GetPacket();

                        magicPacket.ext = (RTP_EXT*)packet->GetPayloadBuffer();
                        magicPacket.hdr = (RTP_HEADER*)(magicPacket.ext + 1);

                        assert(m_info.inSrcMmId == 0 || packet->GetMmId() == m_info.inSrcMmId);
                        assert(packet->GetMmType() == m_info.mmType);
                        if (
                            (m_info.inSrcMmId != 0 && packet->GetMmId() != m_info.inSrcMmId)
                            ||
                            packet->GetMmType() != m_info.mmType /* drop this packet */
                           )
                        {
                            packet->Release();
                            packet = NULL;
                            break;
                        }
                    }
                }

                if (!m_handshakeOk)
                {
                    m_handshakeOk = true;

                    m_reactor->CancelTimer(m_timeoutTimerId);
                    m_timeoutTimerId = 0;

                    /*
                     * Activate ECONNRESET event
                     */
                    m_trans->UdpConnResetAsError(&m_remoteAddr);

                    char theIp[64] = "";
                    m_localAddr.sin_port        = pbsd_hton16(m_trans->GetLocalPort());
                    m_localAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetLocalIp(theIp));
                }
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
                DoCallbackOnOk(observer);
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
        }
        break;
    } /* end of while () */
}

void
CRtpSessionUdpserverEx::OnTimer(void*    factory,
                                uint64_t timerId,
                                int64_t  tick,
                                int64_t  userData)
{
    CRtpSessionBase::OnTimer(factory, timerId, tick, userData);

    assert(factory != NULL);
    assert(timerId > 0);
    if (factory == NULL || timerId == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        if (timerId != m_syncTimerId)
        {
            return;
        }

        if (!m_syncReceived)
        {
            return;
        }

        if (!m_handshakeOk)
        {
            DoHandshake2();
        }
        else
        {
            m_reactor->CancelTimer(m_syncTimerId);
            m_syncTimerId = 0;
        }
    }
}

bool
CRtpSessionUdpserverEx::DoHandshake2()
{
    assert(m_trans != NULL);
    if (m_trans == NULL)
    {
        return false;
    }

    CRtpPacket* packet = CRtpPacket::CreateInstance(
        &m_syncToPeer, sizeof(RTP_UDPX_SYNC), RTP_EPM_DEFAULT);
    if (packet == NULL)
    {
        return false;
    }

    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);
    packet->SetUdpxSync(true); /* sync flag */

    bool ret = m_trans->SendData(
        (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT),
        packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT),
        0,
        &m_remoteAddr
        );
    packet->Release();

    return ret;
}
