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

#include "rtp_session_tcpserver_ex.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpserverEx*
CRtpSessionTcpserverEx::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionTcpserverEx* const session = new CRtpSessionTcpserverEx(*localInfo, NULL);

    return (session);
}

CRtpSessionTcpserverEx::CRtpSessionTcpserverEx(const RTP_SESSION_INFO& localInfo,
                                               PRO_SSL_CTX*            sslCtx) /* = NULL */
                                               :
m_sslCtx(sslCtx)
{
    m_info              = localInfo;
    m_info.localVersion = GetRtpSessionVersion();
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
                             PRO_INT64            sockId,
                             bool                 unixSocket)
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(sockId != -1);
    if (observer == NULL || reactor == NULL || sockId == -1)
    {
        return (false);
    }

    unsigned long sockBufSizeRecv = 0;
    unsigned long sockBufSizeSend = 0;
    unsigned long recvPoolSize    = 0;
    GetRtpTcpSocketParams(
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

        if (m_sslCtx != NULL)
        {
            m_trans = ProCreateSslTransport(this, reactor, m_sslCtx, sockId, unixSocket,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
            if (m_trans != NULL)
            {
                m_sslCtx = NULL;
            }
        }
        else
        {
            m_trans = ProCreateTcpTransport(this, reactor, sockId, unixSocket,
                sockBufSizeRecv, sockBufSizeSend, recvPoolSize);
        }
        if (m_trans == NULL)
        {
            return (false);
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
        m_tcpConnected = true;
        m_handshakeOk  = true;
        m_onOkTimerId  = reactor->ScheduleTimer(this, 0, false);

        if (DoHandshake())
        {
            return (true);
        }

        m_reactor->CancelTimer(m_onOkTimerId);
        m_onOkTimerId = 0;
    }

    Fini();

    return (false);
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
PRO_CALLTYPE
CRtpSessionTcpserverEx::AddRef()
{
    const unsigned long refCount = CRtpSessionBase::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpserverEx::Release()
{
    const unsigned long refCount = CRtpSessionBase::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpSessionTcpserverEx::OnRecv(IProTransport*          trans,
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

            packet = CRtpPacket::CreateInstance(sizeof(RTP_EXT) + ext.hdrAndPayloadSize);
            if (packet == NULL)
            {
                error = true;
            }
            else
            {
                recvPool.PeekData(
                    packet->GetPayloadBuffer(), sizeof(RTP_EXT) + ext.hdrAndPayloadSize);

                if (!CRtpPacket::ParseExtBuffer(
                    (char*)packet->GetPayloadBuffer(), packet->GetPayloadSize()))
                {
                    error = true;
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
                        error = true;
                    }
                }
            }

            recvPool.Flush(sizeof(RTP_EXT) + ext.hdrAndPayloadSize);

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
CRtpSessionTcpserverEx::DoHandshake()
{
    assert(m_trans != NULL);
    if (m_trans == NULL)
    {
        return (false);
    }

    const PRO_UINT16 version = pbsd_hton16(m_info.localVersion);

    IRtpPacket* const packet = CreateRtpPacket(&version, sizeof(PRO_UINT16));
    if (packet == NULL)
    {
        return (false);
    }

    packet->SetMmId(m_info.mmId);
    packet->SetMmType(m_info.mmType);

    const bool ret = m_trans->SendData(
        (char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER) - sizeof(RTP_EXT),
        packet->GetPayloadSize() + sizeof(RTP_HEADER) + sizeof(RTP_EXT)
        );
    packet->Release();

    return (ret);
}
