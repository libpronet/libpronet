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

#include "rtp_session_tcpserver_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpserverEx*
CRtpSessionTcpserverEx::CreateInstance(const RTP_SESSION_INFO* localInfo,
                                       bool                    suspendRecv)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return NULL;
    }

    assert(
        localInfo->packMode == RTP_EPM_DEFAULT ||
        localInfo->packMode == RTP_EPM_TCP2    ||
        localInfo->packMode == RTP_EPM_TCP4
        );
    if (localInfo->packMode != RTP_EPM_DEFAULT &&
        localInfo->packMode != RTP_EPM_TCP2    &&
        localInfo->packMode != RTP_EPM_TCP4)
    {
        return NULL;
    }

    return new CRtpSessionTcpserverEx(*localInfo, suspendRecv, NULL);
}

CRtpSessionTcpserverEx::CRtpSessionTcpserverEx(const RTP_SESSION_INFO& localInfo,
                                               bool                    suspendRecv,
                                               PRO_SSL_CTX*            sslCtx) /* = NULL */
:
CRtpSessionBase(suspendRecv),
m_sslCtx(sslCtx)
{
    m_info              = localInfo;
    m_info.localVersion = RTP_SESSION_PROTOCOL_VERSION;
    m_info.sessionType  = RTP_ST_TCPSERVER_EX;
}

CRtpSessionTcpserverEx::~CRtpSessionTcpserverEx()
{
    Fini();

    ProSslCtx_Delete(m_sslCtx);
    m_sslCtx = NULL;
}

bool
CRtpSessionTcpserverEx::Init(IRtpSessionObserver* observer,
                             IProReactor*         reactor,
                             int64_t              sockId,
                             bool                 unixSocket,
                             bool                 useAckData,
                             char                 ackData[64])
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactor == NULL || sockId == -1)
    {
        return false;
    }

    size_t sockBufSizeRecv = 0; /* zero by default */
    size_t sockBufSizeSend = 0; /* zero by default */
    size_t recvPoolSize    = 0;
    GetRtpTcpSocketParams(m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL)
        {
            return false;
        }

        if (m_sslCtx != NULL)
        {
            m_trans = ProCreateSslTransport(this, reactor, m_sslCtx, sockId, unixSocket,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize, m_suspendRecv);
            if (m_trans != NULL)
            {
                m_sslCtx = NULL;
            }
        }
        else
        {
            m_trans = ProCreateTcpTransport(this, reactor, sockId, unixSocket,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize, m_suspendRecv);
        }
        if (m_trans == NULL)
        {
            return false;
        }

        char theIp[64] = "";
        m_localAddr.sin_family       = AF_INET;
        m_localAddr.sin_port         = pbsd_hton16(m_trans->GetLocalPort());
        m_localAddr.sin_addr.s_addr  = pbsd_inet_aton(m_trans->GetLocalIp(theIp));
        m_remoteAddr.sin_family      = AF_INET;
        m_remoteAddr.sin_port        = pbsd_hton16(m_trans->GetRemotePort());
        m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetRemoteIp(theIp));

        m_trans->StartHeartbeat();

        observer->AddRef();
        m_observer     = observer;
        m_reactor      = reactor;
        m_tcpConnected = true; /* !!! */
        m_handshakeOk  = true; /* !!! */
        m_onOkTimerId  = reactor->SetupTimer(this, 0, 0);

        if (!DoHandshake(useAckData, ackData))
        {
            m_reactor->CancelTimer(m_onOkTimerId);
            m_onOkTimerId = 0;

            goto EXIT;
        }
    }

    return true;

EXIT:

    Fini();

    return false;
}

void
CRtpSessionTcpserverEx::Fini()
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

        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteTransport(trans);
    observer->Release();
}

unsigned long
CRtpSessionTcpserverEx::AddRef()
{
    return CRtpSessionBase::AddRef();
}

unsigned long
CRtpSessionTcpserverEx::Release()
{
    return CRtpSessionBase::Release();
}

void
CRtpSessionTcpserverEx::OnRecv(IProTransport*          trans,
                               const pbsd_sockaddr_in* remoteAddr)
{
    assert(trans != NULL);
    if (trans == NULL)
    {
        return;
    }

    while (1)
    {
        IRtpSessionObserver* observer = NULL;
        CRtpPacket*          packet   = NULL;
        bool                 error    = false;
        bool                 leave    = false;

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

            m_peerAliveTick = ProGetTickCount64();

            if (m_info.packMode == RTP_EPM_DEFAULT)
            {
                error = !Recv0(packet, leave);
            }
            else if (m_info.packMode == RTP_EPM_TCP2)
            {
                error = !Recv2(packet, leave);
            }
            else
            {
                error = !Recv4(packet, leave);
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
            else
            {
                DoCallbackOnOk(observer);
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
            break;
        }

        if (error || packet == NULL || leave)
        {
            break;
        }
    } /* end of while () */
}

bool
CRtpSessionTcpserverEx::Recv0(CRtpPacket*& packet,
                              bool&        leave)
{
    assert(m_info.packMode == RTP_EPM_DEFAULT);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;
    leave  = false;

    bool ret = true;

    while (1)
    {
        IProRecvPool& recvPool = *m_trans->GetRecvPool();
        size_t        dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(RTP_EXT))
        {
            leave = true;
            break;
        }

        RTP_EXT ext;
        recvPool.PeekData(&ext, sizeof(RTP_EXT));
        ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);
        if (dataSize < sizeof(RTP_EXT) + ext.hdrAndPayloadSize)
        {
            leave = true;
            break;
        }

        if (ext.hdrAndPayloadSize == 0)
        {
            recvPool.Flush(sizeof(RTP_EXT));
            continue;
        }

        packet = CRtpPacket::CreateInstance(
            sizeof(RTP_EXT) + ext.hdrAndPayloadSize, m_info.packMode);
        if (packet == NULL)
        {
            ret = false;
            break;
        }

        recvPool.PeekData(
            packet->GetPayloadBuffer(),
            sizeof(RTP_EXT) + ext.hdrAndPayloadSize
            );
        recvPool.Flush(sizeof(RTP_EXT) + ext.hdrAndPayloadSize);

        if (!CRtpPacket::ParseExtBuffer(
            (char*)packet->GetPayloadBuffer(), packet->GetPayloadSize16()))
        {
            packet->Release();
            packet = NULL;

            ret = false;
            break;
        }

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
            continue;
        }
        break;
    } /* end of while () */

    return ret;
}

bool
CRtpSessionTcpserverEx::Recv2(CRtpPacket*& packet,
                              bool&        leave)
{
    assert(m_info.packMode == RTP_EPM_TCP2);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;
    leave  = false;

    bool ret = true;

    while (1)
    {
        IProRecvPool& recvPool = *m_trans->GetRecvPool();
        size_t        dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(uint16_t))
        {
            leave = true;
            break;
        }

        uint16_t packetSize = 0;
        recvPool.PeekData(&packetSize, sizeof(uint16_t));
        packetSize = pbsd_ntoh16(packetSize);
        if (dataSize < sizeof(uint16_t) + packetSize) /* 2 + ... */
        {
            leave = true;
            break;
        }

        if (packetSize == 0)
        {
            recvPool.Flush(sizeof(uint16_t));
            continue;
        }

        packet = CRtpPacket::CreateInstance(packetSize, m_info.packMode);
        if (packet == NULL)
        {
            ret = false;
            break;
        }

        recvPool.Flush(sizeof(uint16_t));
        recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);
        recvPool.Flush(packetSize);

        packet->SetMmId(m_info.inSrcMmId);
        packet->SetMmType(m_info.mmType);
        break;
    } /* end of while () */

    return ret;
}

bool
CRtpSessionTcpserverEx::Recv4(CRtpPacket*& packet,
                              bool&        leave)
{
    assert(m_info.packMode == RTP_EPM_TCP4);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;
    leave  = false;

    bool ret = true;

    while (1)
    {
        IProRecvPool& recvPool = *m_trans->GetRecvPool();
        size_t        dataSize = recvPool.PeekDataSize();
        size_t        freeSize = recvPool.GetFreeSize();

        if (m_bigPacket == NULL)
        {
            /*
             * standard mode
             */
            if (dataSize < sizeof(uint32_t))
            {
                leave = true;
                break;
            }

            uint32_t packetSize = 0;
            recvPool.PeekData(&packetSize, sizeof(uint32_t));
            packetSize = pbsd_ntoh32(packetSize);
            if (packetSize > PRO_TCP4_PAYLOAD_SIZE)
            {
                ret = false;
                break;
            }

            if (packetSize == 0)
            {
                recvPool.Flush(sizeof(uint32_t));
                continue;
            }

            if (dataSize + freeSize < sizeof(uint32_t) + packetSize) /* a big-packet */
            {
                m_bigPacket = CRtpPacket::CreateInstance(packetSize, m_info.packMode);
                if (m_bigPacket == NULL)
                {
                    ret = false;
                    break;
                }

                recvPool.Flush(sizeof(uint32_t));

                m_bigPacket->SetMmId(m_info.inSrcMmId);
                m_bigPacket->SetMmType(m_info.mmType);
                continue; /* switch to big-packet mode */
            }
            else if (dataSize < sizeof(uint32_t) + packetSize) /* 4 + ... */
            {
                leave = true;
                break;
            }
            else
            {
                packet = CRtpPacket::CreateInstance(packetSize, m_info.packMode);
                if (packet == NULL)
                {
                    ret = false;
                    break;
                }

                recvPool.Flush(sizeof(uint32_t));
                recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);
                recvPool.Flush(packetSize);

                packet->SetMmId(m_info.inSrcMmId);
                packet->SetMmType(m_info.mmType);
                break;
            }
        }
        else
        {
            /*
             * big-packet mode
             */
            if (dataSize == 0)
            {
                leave = true;
                break;
            }

            size_t pos = (size_t)m_bigPacket->GetMagic2();
            void*  buf = (char*)m_bigPacket->GetPayloadBuffer() + pos;
            size_t len = m_bigPacket->GetPayloadSize() - pos;
            if (len > dataSize)
            {
                len = dataSize;
            }

            recvPool.PeekData(buf, len);
            recvPool.Flush(len);

            m_bigPacket->SetMagic2(pos + len);
            if (pos + len < m_bigPacket->GetPayloadSize())
            {
                continue;
            }

            packet = m_bigPacket;
            m_bigPacket = NULL;
            break;
        }
    } /* end of while () */

    return ret;
}

bool
CRtpSessionTcpserverEx::DoHandshake(bool useAckData,
                                    char ackData[64])
{
    assert(m_trans != NULL);
    if (m_trans == NULL)
    {
        return false;
    }

    /*
     * send the ACK result
     */
    RTP_SESSION_ACK ack;
    memset(&ack, 0, sizeof(RTP_SESSION_ACK));
    ack.version = pbsd_hton16(m_info.localVersion);
    if (useAckData)
    {
        memcpy(ack.userData, ackData, 64);
    }

    IRtpPacket* packet = CreateRtpPacket(&ack, sizeof(RTP_SESSION_ACK));
    if (packet == NULL)
    {
        return false;
    }

    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);

    bool ret = m_trans->SendData(
        (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT),
        packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT)
        );
    packet->Release();

    return ret;
}
