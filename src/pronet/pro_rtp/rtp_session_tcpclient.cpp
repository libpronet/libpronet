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

#include "rtp_session_tcpclient.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpclient*
CRtpSessionTcpclient::CreateInstance(const RTP_SESSION_INFO* localInfo,
                                     bool                    suspendRecv)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionTcpclient* const session =
        new CRtpSessionTcpclient(*localInfo, suspendRecv);

    return (session);
}

CRtpSessionTcpclient::CRtpSessionTcpclient(const RTP_SESSION_INFO& localInfo,
                                           bool                    suspendRecv)
                                           :
CRtpSessionBase(suspendRecv)
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_TCPCLIENT;

    m_connector          = NULL;
}

CRtpSessionTcpclient::~CRtpSessionTcpclient()
{
    Fini();
}

bool
CRtpSessionTcpclient::Init(IRtpSessionObserver* observer,
                           IProReactor*         reactor,
                           const char*          remoteIp,
                           unsigned short       remotePort,
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

    const char* const anyIp = "0.0.0.0";
    if (localIp == NULL || localIp[0] == '\0')
    {
        localIp = anyIp;
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

    if (localAddr.sin_addr.s_addr  == (PRO_UINT32)-1 ||
        remoteAddr.sin_addr.s_addr == (PRO_UINT32)-1 ||
        remoteAddr.sin_addr.s_addr == 0)
    {
        return (false);
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
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL ||
            m_connector != NULL)
        {
            return (false);
        }

        m_connector = ProCreateConnector(
            true, /* enable unixSocket */
            this,
            reactor,
            remoteIp2,
            remotePort,
            localIp2,
            timeoutInSeconds
            );
        if (m_connector == NULL)
        {
            return (false);
        }

        observer->AddRef();
        m_observer   = observer;
        m_reactor    = reactor;
        m_localAddr  = localAddr;
        m_remoteAddr = remoteAddr;
    }

    return (true);
}

void
CRtpSessionTcpclient::Fini()
{
    IRtpSessionObserver* observer  = NULL;
    IProTransport*       trans     = NULL;
    IProConnector*       connector = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        connector = m_connector;
        m_connector = NULL;
        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteConnector(connector);
    ProDeleteTransport(trans);
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpclient::AddRef()
{
    const unsigned long refCount = CRtpSessionBase::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpclient::Release()
{
    const unsigned long refCount = CRtpSessionBase::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpSessionTcpclient::OnConnectOk(IProConnector*   connector,
                                  PRO_INT64        sockId,
                                  bool             unixSocket,
                                  const char*      remoteIp,
                                  unsigned short   remotePort,
                                  unsigned char    serviceId,
                                  unsigned char    serviceOpt,
                                  const PRO_NONCE* nonce)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(connector != NULL);
    assert(sockId != -1);
    if (connector == NULL || sockId == -1)
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
        assert(m_trans == NULL);

        m_trans = ProCreateTcpTransport(
            this, m_reactor, sockId, unixSocket, sockBufSizeRecv,
            sockBufSizeSend, recvPoolSize, m_suspendRecv);
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
        else
        {
            if (!m_onOkCalled)
            {
                m_onOkCalled = true;
                observer->OnOkSession(this);
            }
        }
    }

    observer->Release();
    ProDeleteConnector(connector);
}}

void
PRO_CALLTYPE
CRtpSessionTcpclient::OnConnectError(IProConnector* connector,
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
CRtpSessionTcpclient::OnRecv(IProTransport*          trans,
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

            packet = CRtpPacket::CreateInstance(packetSize, RTP_EPM_DEFAULT);
            if (packet == NULL)
            {
                error = true;
            }
            else
            {
                recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);

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
                if (!ret || payloadBuffer == NULL || payloadSize == 0)
                {
                    packet->Release();
                    packet = NULL;
                    recvPool.Flush(packetSize);
                    continue;
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

            recvPool.Flush(packetSize);

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
    } /* end of while (...) */
}}
