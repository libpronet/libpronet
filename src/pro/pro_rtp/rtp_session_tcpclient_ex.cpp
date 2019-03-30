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

#include "rtp_session_tcpclient_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpclientEx*
CRtpSessionTcpclientEx::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
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
        return (NULL);
    }

    CRtpSessionTcpclientEx* const session = new CRtpSessionTcpclientEx(*localInfo, NULL, NULL);

    return (session);
}

CRtpSessionTcpclientEx::CRtpSessionTcpclientEx(const RTP_SESSION_INFO&      localInfo,
                                               const PRO_SSL_CLIENT_CONFIG* sslConfig,      /* = NULL */
                                               const char*                  sslServiceName) /* = NULL */
                                               :
m_sslConfig(sslConfig),
m_sslServiceName(sslServiceName != NULL ? sslServiceName : "")
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
                             unsigned long        timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(remoteIp != NULL);
    assert(remoteIp[0] != '\0');
    assert(remotePort > 0);
    if (observer == NULL || reactor == NULL ||
        remoteIp == NULL || remoteIp[0] == '\0' || remotePort == 0)
    {
        return (false);
    }

    if (timeoutInSeconds == 0)
    {
        timeoutInSeconds = DEFAULT_TIMEOUT;
    }

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
            return (false);
        }

        m_connector = ProCreateConnectorEx(
            true,                /* enable unixSocket */
            m_info.mmType,       /* serviceId.  [0 for ipc-pipe, !0 for media-link] */
            m_sslConfig != NULL, /* serviceOpt. [0 for tcp     , !0 for ssl] */
            this,
            reactor,
            remoteIp,
            remotePort,
            localIp,
            timeoutInSeconds
            );
        if (m_connector == NULL)
        {
            return (false);
        }

        observer->AddRef();
        m_observer         = observer;
        m_reactor          = reactor;
        m_timeoutTimerId   = reactor->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false);
        m_password         = password != NULL ? password : "";
        m_timeoutInSeconds = timeoutInSeconds;
    }

    return (true);
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
PRO_CALLTYPE
CRtpSessionTcpclientEx::AddRef()
{
    const unsigned long refCount = CRtpSessionBase::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpclientEx::Release()
{
    const unsigned long refCount = CRtpSessionBase::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnConnectOk(IProConnector* connector,
                                    PRO_INT64      sockId,
                                    bool           unixSocket,
                                    const char*    remoteIp,
                                    unsigned short remotePort,
                                    unsigned char  serviceId,
                                    unsigned char  serviceOpt,
                                    PRO_UINT64     nonce)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(connector != NULL);
    assert(sockId != -1);
    if (connector == NULL || sockId == -1)
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

        if (!DoHandshake(sockId, unixSocket, nonce))
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
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnConnectError(IProConnector* connector,
                                       const char*    remoteIp,
                                       unsigned short remotePort,
                                       unsigned char  serviceId,
                                       unsigned char  serviceOpt,
                                       bool           timeout)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(connector != NULL);
    if (connector == NULL)
    {
        return;
    }

    IRtpSessionObserver* observer  = NULL;
    const long           errorCode = timeout ? PBSD_ETIMEDOUT : -1;

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
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnHandshakeOk(IProTcpHandshaker* handshaker,
                                      PRO_INT64          sockId,
                                      bool               unixSocket,
                                      const void*        buf,
                                      unsigned long      size)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(handshaker != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || sockId == -1)
    {
        return;
    }

    unsigned long sockBufSizeRecv = 0;
    unsigned long sockBufSizeSend = 0;
    unsigned long recvPoolSize    = 0;
    GetRtpTcpSocketParams(
        m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

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
            if (!CRtpPacket::ParseExtBuffer((char*)buf, (PRO_UINT16)size))
            {
                ProCloseSockId(sockId);
                sockId = -1;
            }
            else
            {
                RTP_SESSION_ACK ack;
                memcpy(
                    &ack,
                    (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
                    sizeof(RTP_SESSION_ACK)
                    );
                ack.version = pbsd_ntoh16(ack.version);

                m_info.remoteVersion = ack.version;

                m_trans = ProCreateTcpTransport(this, m_reactor, sockId, unixSocket,
                    sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
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
            if (!m_onOkCalled)
            {
                m_onOkCalled = true;
                observer->OnOkSession(this);
            }
        }
        else
        {
        }
    }

    observer->Release();
    ProDeleteTcpHandshaker(handshaker);
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnHandshakeError(IProTcpHandshaker* handshaker,
                                         long               errorCode)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

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
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnHandshakeOk(IProSslHandshaker* handshaker,
                                      PRO_SSL_CTX*       ctx,
                                      PRO_INT64          sockId,
                                      bool               unixSocket,
                                      const void*        buf,
                                      unsigned long      size)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(handshaker != NULL);
    assert(ctx != NULL);
    assert(sockId != -1);
    if (handshaker == NULL || ctx == NULL || sockId == -1)
    {
        return;
    }

    unsigned long sockBufSizeRecv = 0;
    unsigned long sockBufSizeSend = 0;
    unsigned long recvPoolSize    = 0;
    GetRtpTcpSocketParams(
        m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

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
            if (!CRtpPacket::ParseExtBuffer((char*)buf, (PRO_UINT16)size))
            {
                ProSslCtx_Delete(ctx);
                ProCloseSockId(sockId);
                ctx    = NULL;
                sockId = -1;
            }
            else
            {
                RTP_SESSION_ACK ack;
                memcpy(
                    &ack,
                    (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
                    sizeof(RTP_SESSION_ACK)
                    );
                ack.version = pbsd_ntoh16(ack.version);

                m_info.remoteVersion = ack.version;

                m_trans = ProCreateSslTransport(this, m_reactor, ctx, sockId, unixSocket,
                    sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
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
            if (!m_onOkCalled)
            {
                m_onOkCalled = true;
                observer->OnOkSession(this);
            }
        }
        else
        {
        }
    }

    observer->Release();
    ProDeleteSslHandshaker(handshaker);
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnHandshakeError(IProSslHandshaker* handshaker,
                                         long               errorCode,
                                         long               sslCode)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

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
}}

void
PRO_CALLTYPE
CRtpSessionTcpclientEx::OnRecv(IProTransport*          trans,
                               const pbsd_sockaddr_in* remoteAddr)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

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
                error = !Recv0(packet);
            }
            else if (m_info.packMode == RTP_EPM_TCP2)
            {
                error = !Recv2(packet);
            }
            else
            {
                error = !Recv4(packet);
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

        if (error || packet == NULL)
        {
            break;
        }
    } /* end of while (...) */
}}

bool
CRtpSessionTcpclientEx::Recv0(CRtpPacket*& packet)
{
    assert(m_info.packMode == RTP_EPM_DEFAULT);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;

    bool ret = true;

    while (1)
    {
        IProRecvPool&       recvPool = *m_trans->GetRecvPool();
        const unsigned long dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(RTP_EXT))
        {
            break;
        }

        RTP_EXT ext;
        recvPool.PeekData(&ext, sizeof(RTP_EXT));
        ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);
        if (dataSize < sizeof(RTP_EXT) + ext.hdrAndPayloadSize)
        {
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
        }
        else
        {
            recvPool.PeekData(
                packet->GetPayloadBuffer(), sizeof(RTP_EXT) + ext.hdrAndPayloadSize);

            if (!CRtpPacket::ParseExtBuffer(
                (char*)packet->GetPayloadBuffer(), packet->GetPayloadSize16()))
            {
                ret = false;
            }
            else
            {
                RTP_PACKET& magicPacket = packet->GetPacket();

                magicPacket.ext = (RTP_EXT*)packet->GetPayloadBuffer();
                magicPacket.hdr = (RTP_HEADER*)(magicPacket.ext + 1);

                assert(m_info.inSrcMmId == 0 || packet->GetMmId() == m_info.inSrcMmId);
                assert(packet->GetMmType() == m_info.mmType);
                if (m_info.inSrcMmId != 0 && packet->GetMmId() != m_info.inSrcMmId
                    ||
                    packet->GetMmType() != m_info.mmType)
                {
                    ret = false;
                }
            }

            if (!ret)
            {
                packet->Release();
                packet = NULL;
            }
        }

        recvPool.Flush(sizeof(RTP_EXT) + ext.hdrAndPayloadSize);
    } /* end of while (...) */

    return (ret);
}

bool
CRtpSessionTcpclientEx::Recv2(CRtpPacket*& packet)
{
    assert(m_info.packMode == RTP_EPM_TCP2);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;

    bool ret = true;

    while (1)
    {
        IProRecvPool&       recvPool = *m_trans->GetRecvPool();
        const unsigned long dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(PRO_UINT16))
        {
            break;
        }

        PRO_UINT16 packetSize = 0;
        recvPool.PeekData(&packetSize, sizeof(PRO_UINT16));
        packetSize = pbsd_ntoh16(packetSize);
        if (dataSize < sizeof(PRO_UINT16) + packetSize) /* 2 + ... */
        {
            break;
        }

        recvPool.Flush(sizeof(PRO_UINT16));

        if (packetSize == 0)
        {
            continue;
        }

        packet = CRtpPacket::CreateInstance(packetSize, m_info.packMode);
        if (packet == NULL)
        {
            ret = false;
        }
        else
        {
            recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);

            packet->SetMmId(m_info.inSrcMmId);
            packet->SetMmType(m_info.mmType);
        }

        recvPool.Flush(packetSize);
    } /* end of while (...) */

    return (ret);
}

bool
CRtpSessionTcpclientEx::Recv4(CRtpPacket*& packet)
{
    assert(m_info.packMode == RTP_EPM_TCP4);
    assert(m_trans != NULL);
    assert(m_handshakeOk);

    packet = NULL;

    bool ret = true;

    while (1)
    {
        IProRecvPool&       recvPool = *m_trans->GetRecvPool();
        const unsigned long dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(PRO_UINT32))
        {
            break;
        }

        PRO_UINT32 packetSize = 0;
        recvPool.PeekData(&packetSize, sizeof(PRO_UINT32));
        packetSize = pbsd_ntoh32(packetSize);
        if (dataSize < sizeof(PRO_UINT32) + packetSize) /* 4 + ... */
        {
            break;
        }

        recvPool.Flush(sizeof(PRO_UINT32));

        if (packetSize == 0)
        {
            continue;
        }

        packet = CRtpPacket::CreateInstance(packetSize, m_info.packMode);
        if (packet == NULL)
        {
            ret = false;
        }
        else
        {
            recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);

            packet->SetMmId(m_info.inSrcMmId);
            packet->SetMmType(m_info.mmType);
        }

        recvPool.Flush(packetSize);
    } /* end of while (...) */

    return (ret);
}

bool
CRtpSessionTcpclientEx::DoHandshake(PRO_INT64  sockId,
                                    bool       unixSocket,
                                    PRO_UINT64 nonce)
{
    assert(sockId != -1);
    assert(m_tcpHandshaker == NULL);
    assert(m_sslHandshaker == NULL);
    if (sockId == -1 || m_tcpHandshaker != NULL || m_sslHandshaker != NULL)
    {
        return (false);
    }

    RTP_SESSION_INFO localInfo = m_info;
    localInfo.localVersion  = pbsd_hton16(localInfo.localVersion);
    localInfo.remoteVersion = pbsd_hton16(localInfo.remoteVersion);
    localInfo.someId        = pbsd_hton32(localInfo.someId);
    localInfo.mmId          = pbsd_hton32(localInfo.mmId);
    localInfo.inSrcMmId     = pbsd_hton32(localInfo.inSrcMmId);
    localInfo.outSrcMmId    = pbsd_hton32(localInfo.outSrcMmId);
    ProCalcPasswordHash(nonce, m_password.c_str(), localInfo.passwordHash);

    if (!m_password.empty())
    {
        ProZeroMemory(&m_password[0], m_password.length());
        m_password = "";
    }

    IRtpPacket* const packet = CreateRtpPacket(&localInfo, sizeof(RTP_SESSION_INFO));
    if (packet == NULL)
    {
        return (false);
    }

    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);

    if (m_sslConfig != NULL)
    {
        PRO_SSL_CTX* const sslCtx = ProSslCtx_Createc(
            m_sslConfig, m_sslServiceName.c_str(), sockId, nonce);
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

    return (m_tcpHandshaker != NULL || m_sslHandshaker != NULL);
}
