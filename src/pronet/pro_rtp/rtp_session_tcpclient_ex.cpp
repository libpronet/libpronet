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

#include "rtp_session_tcpclient_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpclientEx*
CRtpSessionTcpclientEx::CreateInstance(const RTP_SESSION_INFO* localInfo,
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

    return new CRtpSessionTcpclientEx(*localInfo, suspendRecv, NULL, NULL);
}

CRtpSessionTcpclientEx::CRtpSessionTcpclientEx(const RTP_SESSION_INFO&      localInfo,
                                               bool                         suspendRecv,
                                               const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
                                               const char*                  sslSni)    /* = NULL */
:
CRtpSessionBase(suspendRecv),
m_sslConfig(sslConfig),
m_sslSni(sslSni != NULL ? sslSni : "")
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_TCPCLIENT_EX;

    m_password           = "";
    m_timeoutInSeconds   = DEFAULT_TIMEOUT;

    m_connector          = NULL;
    m_tcpHandshaker      = NULL;
    m_sslHandshaker      = NULL;
}

CRtpSessionTcpclientEx::~CRtpSessionTcpclientEx()
{
    Fini();
}

bool
CRtpSessionTcpclientEx::Init(IRtpSessionObserver* observer,
                             IProReactor*         reactor,
                             const char*          remoteIp,
                             unsigned short       remotePort,
                             const char*          password,         /* = NULL */
                             const char*          localIp,          /* = NULL */
                             unsigned int         timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    if (observer == NULL || reactor == NULL ||
        remoteIp == NULL || remoteIp[0] == '\0' || remotePort == 0)
    {
        return false;
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(remotePort);
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(remoteIp); /* DNS */

    if (localAddr.sin_addr.s_addr  == (uint32_t)-1 ||
        remoteAddr.sin_addr.s_addr == (uint32_t)-1 ||
        remoteAddr.sin_addr.s_addr == 0)
    {
        return false;
    }

    char localIp2[64]  = "";
    char remoteIp2[64] = "";
    pbsd_inet_ntoa(localAddr.sin_addr.s_addr , localIp2);
    pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp2);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        assert(m_connector == NULL);
        assert(m_tcpHandshaker == NULL);
        assert(m_sslHandshaker == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL ||
            m_connector != NULL || m_tcpHandshaker != NULL || m_sslHandshaker != NULL)
        {
            return false;
        }

        m_connector = ProCreateConnectorEx(
            true,                /* enable unixSocket */
            m_info.mmType,       /* serviceId.  [0 for ipc-pipe, !0 for media-link] */
            m_sslConfig != NULL, /* serviceOpt. [0 for tcp     , !0 for ssl] */
            this,
            reactor,
            remoteIp2,
            remotePort,
            localIp2,
            timeoutInSeconds
            );
        if (m_connector == NULL)
        {
            return false;
        }

        observer->AddRef();
        m_observer         = observer;
        m_reactor          = reactor;
        m_localAddr        = localAddr;
        m_remoteAddr       = remoteAddr;
        m_timeoutTimerId   = reactor->SetupTimer(this, (uint64_t)timeoutInSeconds * 1000, 0);
        m_password         = password != NULL ? password : "";
        m_timeoutInSeconds = timeoutInSeconds;
    }

    return true;
}

void
CRtpSessionTcpclientEx::Fini()
{
    IRtpSessionObserver* observer      = NULL;
    IProTransport*       trans         = NULL;
    IProConnector*       connector     = NULL;
    IProTcpHandshaker*   tcpHandshaker = NULL;
    IProSslHandshaker*   sslHandshaker = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timeoutTimerId);
        m_timeoutTimerId = 0;

        if (!m_password.empty())
        {
            ProZeroMemory(&m_password[0], m_password.length());
            m_password = "";
        }

        sslHandshaker = m_sslHandshaker;
        m_sslHandshaker = NULL;
        tcpHandshaker = m_tcpHandshaker;
        m_tcpHandshaker = NULL;
        connector = m_connector;
        m_connector = NULL;
        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteSslHandshaker(sslHandshaker);
    ProDeleteTcpHandshaker(tcpHandshaker);
    ProDeleteConnector(connector);
    ProDeleteTransport(trans);
    observer->Release();
}

unsigned long
CRtpSessionTcpclientEx::AddRef()
{
    return CRtpSessionBase::AddRef();
}

unsigned long
CRtpSessionTcpclientEx::Release()
{
    return CRtpSessionBase::Release();
}

void
CRtpSessionTcpclientEx::OnConnectOk(IProConnector*   connector,
                                    int64_t          sockId,
                                    bool             unixSocket,
                                    const char*      remoteIp,
                                    unsigned short   remotePort,
                                    unsigned char    serviceId,
                                    unsigned char    serviceOpt,
                                    const PRO_NONCE* nonce)
{
    assert(connector != NULL);
    assert(sockId != -1);
    assert(nonce != NULL);
    if (connector == NULL || sockId == -1 || nonce == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_connector == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (connector != m_connector)
        {
            ProCloseSockId(sockId);

            return;
        }

        m_tcpConnected = true;
        assert(m_tcpHandshaker == NULL);
        assert(m_sslHandshaker == NULL);

        if (!DoHandshake(sockId, unixSocket, *nonce))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        m_connector = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (sockId == -1)
        {
            m_canUpcall = false;
            observer->OnCloseSession(this, -1, 0, m_tcpConnected);
        }
    }

    observer->Release();
    ProDeleteConnector(connector);
}

void
CRtpSessionTcpclientEx::OnConnectError(IProConnector* connector,
                                       const char*    remoteIp,
                                       unsigned short remotePort,
                                       unsigned char  serviceId,
                                       unsigned char  serviceOpt,
                                       bool           timeout)
{
    assert(connector != NULL);
    if (connector == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer  = NULL;
    int                  errorCode = timeout ? PBSD_ETIMEDOUT : -1;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_connector == NULL)
        {
            return;
        }

        if (connector != m_connector)
        {
            return;
        }

        m_connector = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, errorCode, 0, m_tcpConnected);
    }

    observer->Release();
    ProDeleteConnector(connector);
}

void
CRtpSessionTcpclientEx::OnHandshakeOk(IProTcpHandshaker* handshaker,
                                      int64_t            sockId,
                                      bool               unixSocket,
                                      const void*        buf,
                                      size_t             size)
{
    assert(handshaker != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || sockId == -1)
    {
        return;
    }

    size_t sockBufSizeRecv = 0; /* zero by default */
    size_t sockBufSizeSend = 0; /* zero by default */
    size_t recvPoolSize    = 0;
    GetRtpTcpSocketParams(m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_tcpHandshaker == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (handshaker != m_tcpHandshaker)
        {
            ProCloseSockId(sockId);

            return;
        }

        assert(m_sslConfig == NULL);
        assert(m_trans == NULL);

        assert(buf != NULL);
        assert(size == sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK));
        if (buf == NULL ||
            size != sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }
        else
        {
            if (!CRtpPacket::ParseExtBuffer((char*)buf, (uint16_t)size))
            {
                ProCloseSockId(sockId);
                sockId = -1;
            }
            else
            {
                /*
                 * grab the ACK result
                 */
                memcpy(
                    &m_ack,
                    (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
                    sizeof(RTP_SESSION_ACK)
                    );
                m_ack.version = pbsd_ntoh16(m_ack.version);

                m_info.remoteVersion = m_ack.version;

                m_trans = ProCreateTcpTransport(this, m_reactor, sockId, unixSocket,
                    sockBufSizeRecv, sockBufSizeSend, recvPoolSize, m_suspendRecv);
                if (m_trans == NULL)
                {
                    ProCloseSockId(sockId);
                    sockId = -1;
                }
                else
                {
                    char theIp[64] = "";
                    m_localAddr.sin_family       = AF_INET;
                    m_localAddr.sin_port         = pbsd_hton16(m_trans->GetLocalPort());
                    m_localAddr.sin_addr.s_addr  = pbsd_inet_aton(m_trans->GetLocalIp(theIp));
                    m_remoteAddr.sin_family      = AF_INET;
                    m_remoteAddr.sin_port        = pbsd_hton16(m_trans->GetRemotePort());
                    m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetRemoteIp(theIp));

                    m_trans->StartHeartbeat();

                    m_handshakeOk = true;

                    m_reactor->CancelTimer(m_timeoutTimerId);
                    m_timeoutTimerId = 0;
                }
            }
        }

        m_tcpHandshaker = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (sockId == -1)
        {
            m_canUpcall = false;
            observer->OnCloseSession(this, -1, 0, m_tcpConnected);
        }
        else if (m_handshakeOk)
        {
            DoCallbackOnOk(observer);
        }
        else
        {
        }
    }

    observer->Release();
    ProDeleteTcpHandshaker(handshaker);
}

void
CRtpSessionTcpclientEx::OnHandshakeError(IProTcpHandshaker* handshaker,
                                         int                errorCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_tcpHandshaker == NULL)
        {
            return;
        }

        if (handshaker != m_tcpHandshaker)
        {
            return;
        }

        m_tcpHandshaker = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, errorCode, 0, m_tcpConnected);
    }

    observer->Release();
    ProDeleteTcpHandshaker(handshaker);
}

void
CRtpSessionTcpclientEx::OnHandshakeOk(IProSslHandshaker* handshaker,
                                      PRO_SSL_CTX*       ctx,
                                      int64_t            sockId,
                                      bool               unixSocket,
                                      const void*        buf,
                                      size_t             size)
{
    assert(handshaker != NULL);
    assert(ctx != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || ctx == NULL || sockId == -1)
    {
        return;
    }

    size_t sockBufSizeRecv = 0; /* zero by default */
    size_t sockBufSizeSend = 0; /* zero by default */
    size_t recvPoolSize    = 0;
    GetRtpTcpSocketParams(m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_sslHandshaker == NULL)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        if (handshaker != m_sslHandshaker)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        assert(m_sslConfig != NULL);
        assert(m_trans == NULL);

        assert(ctx != NULL);
        assert(buf != NULL);
        assert(size == sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK));
        if (ctx == NULL || buf == NULL ||
            size != sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK))
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);
            ctx    = NULL;
            sockId = -1;
        }
        else
        {
            if (!CRtpPacket::ParseExtBuffer((char*)buf, (uint16_t)size))
            {
                ProSslCtx_Delete(ctx);
                ProCloseSockId(sockId);
                ctx    = NULL;
                sockId = -1;
            }
            else
            {
                /*
                 * grab the ACK result
                 */
                memcpy(
                    &m_ack,
                    (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
                    sizeof(RTP_SESSION_ACK)
                    );
                m_ack.version = pbsd_ntoh16(m_ack.version);

                m_info.remoteVersion = m_ack.version;

                m_trans = ProCreateSslTransport(this, m_reactor, ctx, sockId, unixSocket,
                    sockBufSizeRecv, sockBufSizeSend, recvPoolSize, m_suspendRecv);
                if (m_trans == NULL)
                {
                    ProSslCtx_Delete(ctx);
                    ProCloseSockId(sockId);
                    ctx    = NULL;
                    sockId = -1;
                }
                else
                {
                    char theIp[64] = "";
                    m_localAddr.sin_family       = AF_INET;
                    m_localAddr.sin_port         = pbsd_hton16(m_trans->GetLocalPort());
                    m_localAddr.sin_addr.s_addr  = pbsd_inet_aton(m_trans->GetLocalIp(theIp));
                    m_remoteAddr.sin_family      = AF_INET;
                    m_remoteAddr.sin_port        = pbsd_hton16(m_trans->GetRemotePort());
                    m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetRemoteIp(theIp));

                    m_trans->StartHeartbeat();

                    m_handshakeOk = true;

                    m_reactor->CancelTimer(m_timeoutTimerId);
                    m_timeoutTimerId = 0;
                }
            }
        }

        m_sslHandshaker = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        if (sockId == -1)
        {
            m_canUpcall = false;
            observer->OnCloseSession(this, -1, 0, m_tcpConnected);
        }
        else if (m_handshakeOk)
        {
            DoCallbackOnOk(observer);
        }
        else
        {
        }
    }

    observer->Release();
    ProDeleteSslHandshaker(handshaker);
}

void
CRtpSessionTcpclientEx::OnHandshakeError(IProSslHandshaker* handshaker,
                                         int                errorCode,
                                         int                sslCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_sslHandshaker == NULL)
        {
            return;
        }

        if (handshaker != m_sslHandshaker)
        {
            return;
        }

        m_sslHandshaker = NULL;

        m_observer->AddRef();
        observer = m_observer;
    }

    if (m_canUpcall)
    {
        m_canUpcall = false;
        observer->OnCloseSession(this, errorCode, sslCode, m_tcpConnected);
    }

    observer->Release();
    ProDeleteSslHandshaker(handshaker);
}

void
CRtpSessionTcpclientEx::OnRecv(IProTransport*          trans,
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
CRtpSessionTcpclientEx::Recv0(CRtpPacket*& packet,
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
CRtpSessionTcpclientEx::Recv2(CRtpPacket*& packet,
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
CRtpSessionTcpclientEx::Recv4(CRtpPacket*& packet,
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
CRtpSessionTcpclientEx::DoHandshake(int64_t          sockId,
                                    bool             unixSocket,
                                    const PRO_NONCE& nonce)
{
    assert(sockId != -1);
    assert(m_tcpHandshaker == NULL);
    assert(m_sslHandshaker == NULL);
    if (sockId == -1 || m_tcpHandshaker != NULL || m_sslHandshaker != NULL)
    {
        return false;
    }

    RTP_SESSION_INFO localInfo = m_info;
    localInfo.localVersion  = pbsd_hton16(localInfo.localVersion);
    localInfo.remoteVersion = pbsd_hton16(localInfo.remoteVersion);
    localInfo.someId        = pbsd_hton32(localInfo.someId);
    localInfo.mmId          = pbsd_hton32(localInfo.mmId);
    localInfo.inSrcMmId     = pbsd_hton32(localInfo.inSrcMmId);
    localInfo.outSrcMmId    = pbsd_hton32(localInfo.outSrcMmId);
    ProCalcPasswordHash(nonce.nonce, m_password.c_str(), localInfo.passwordHash);

    if (!m_password.empty())
    {
        ProZeroMemory(&m_password[0], m_password.length());
        m_password = "";
    }

    IRtpPacket* packet = CreateRtpPacket(&localInfo, sizeof(RTP_SESSION_INFO));
    if (packet == NULL)
    {
        return false;
    }

    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);

    if (m_sslConfig != NULL)
    {
        PRO_SSL_CTX* sslCtx = ProSslCtx_CreateC(m_sslConfig, m_sslSni.c_str(), sockId, &nonce);
        if (sslCtx != NULL)
        {
            m_sslHandshaker = ProCreateSslHandshaker(
                this,
                m_reactor,
                sslCtx,
                sockId,
                unixSocket,
                (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT), /* sendData */
                packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT),          /* sendDataSize */
                sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK),           /* recvDataSize */
                false,                                                                    /* recvFirst is false */
                m_timeoutInSeconds
                );
            if (m_sslHandshaker == NULL)
            {
                ProSslCtx_Delete(sslCtx);
            }
        }
    }
    else
    {
        m_tcpHandshaker = ProCreateTcpHandshaker(
            this,
            m_reactor,
            sockId,
            unixSocket,
            (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT), /* sendData */
            packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT),          /* sendDataSize */
            sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_ACK),           /* recvDataSize */
            false,                                                                    /* recvFirst is false */
            m_timeoutInSeconds
            );
    }

    packet->Release();

    return m_tcpHandshaker != NULL || m_sslHandshaker != NULL;
}
