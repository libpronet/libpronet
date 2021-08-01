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

#include "rtp_session_tcpserver.h"
#include "rtp_packet.h"
#include "rtp_session_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_TRY_TIMES   100
#define DEFAULT_TIMEOUT 20

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionTcpserver*
CRtpSessionTcpserver::CreateInstance(const RTP_SESSION_INFO* localInfo,
                                     bool                    suspendRecv)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionTcpserver* const session =
        new CRtpSessionTcpserver(*localInfo, suspendRecv);

    return (session);
}

CRtpSessionTcpserver::CRtpSessionTcpserver(const RTP_SESSION_INFO& localInfo,
                                           bool                    suspendRecv)
                                           :
CRtpSessionBase(suspendRecv)
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_TCPSERVER;

    m_acceptor           = NULL;
}

CRtpSessionTcpserver::~CRtpSessionTcpserver()
{
    Fini();
}

bool
CRtpSessionTcpserver::Init(IRtpSessionObserver* observer,
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
    localAddr.sin_port        = pbsd_hton16(localPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localIp);

    if (localAddr.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_trans == NULL);
        assert(m_acceptor == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_trans != NULL ||
            m_acceptor != NULL)
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
                localPort2 = AllocRtpTcpPort(true); /* rfc is true */
            }

            if (localPort2 % 2 == 0)
            {
                m_dummySockId = ProOpenTcpSockId(localIp, localPort2 + 1);
                if (m_dummySockId == -1)
                {
                    continue;
                }
            }

            m_acceptor = ProCreateAcceptor(this, reactor, localIp, localPort2);
            if (m_acceptor != NULL)
            {
                break;
            }

            ProCloseSockId(m_dummySockId);
            m_dummySockId = -1;
        }

        if (m_acceptor == NULL)
        {
            return (false);
        }

        localAddr.sin_port = pbsd_hton16(ProGetAcceptorPort(m_acceptor));

        observer->AddRef();
        m_observer       = observer;
        m_reactor        = reactor;
        m_localAddr      = localAddr;
        m_timeoutTimerId = reactor->ScheduleTimer(this, (PRO_UINT64)timeoutInSeconds * 1000, false);
    }

    return (true);
}

void
CRtpSessionTcpserver::Fini()
{
    IRtpSessionObserver* observer = NULL;
    IProTransport*       trans    = NULL;
    IProAcceptor*        acceptor = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL)
        {
            return;
        }

        m_reactor->CancelTimer(m_timeoutTimerId);
        m_timeoutTimerId = 0;

        ProCloseSockId(m_dummySockId);
        m_dummySockId = -1;

        acceptor = m_acceptor;
        m_acceptor = NULL;
        trans = m_trans;
        m_trans = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    ProDeleteAcceptor(acceptor);
    ProDeleteTransport(trans);
    observer->Release();
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpserver::AddRef()
{
    const unsigned long refCount = CRtpSessionBase::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpSessionTcpserver::Release()
{
    const unsigned long refCount = CRtpSessionBase::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpSessionTcpserver::OnAccept(IProAcceptor*  acceptor,
                               PRO_INT64      sockId,
                               bool           unixSocket,
                               const char*    localIp,
                               const char*    remoteIp,
                               unsigned short remotePort)
{{
    CProThreadMutexGuard mon(m_lockUpcall);

    assert(acceptor != NULL);
    assert(sockId != -1);
    assert(localIp != NULL);
    assert(remoteIp != NULL);
    if (acceptor == NULL || sockId == -1 || localIp == NULL ||
        remoteIp == NULL)
    {
        return;
    }

    unsigned long sockBufSizeRecv = 0; /* zero by default */
    unsigned long sockBufSizeSend = 0; /* zero by default */
    unsigned long recvPoolSize    = 0;
    GetRtpTcpSocketParams(
        m_info.mmType, &sockBufSizeRecv, &sockBufSizeSend, &recvPoolSize);

    IRtpSessionObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_acceptor == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (acceptor != m_acceptor)
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

            m_reactor->CancelTimer(m_timeoutTimerId);
            m_timeoutTimerId = 0;
        }

        m_acceptor = NULL;

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
    ProDeleteAcceptor(acceptor);
}}

void
PRO_CALLTYPE
CRtpSessionTcpserver::OnRecv(IProTransport*          trans,
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

            error = !Recv(packet, leave);

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

        if (error || packet == NULL || leave)
        {
            break;
        }
    } /* end of while (...) */
}}

bool
CRtpSessionTcpserver::Recv(CRtpPacket*& packet,
                           bool&        leave)
{{
    assert(m_trans != NULL);

    packet = NULL;
    leave  = false;

    bool ret = true;

    while (1)
    {
        IProRecvPool&       recvPool = *m_trans->GetRecvPool();
        const unsigned long dataSize = recvPool.PeekDataSize();

        if (dataSize < sizeof(PRO_UINT16))
        {
            leave = true;
            break;
        }

        PRO_UINT16 packetSize = 0;
        recvPool.PeekData(&packetSize, sizeof(PRO_UINT16));
        packetSize = pbsd_ntoh16(packetSize);
        if (dataSize < sizeof(PRO_UINT16) + packetSize) /* 2 + ... */
        {
            leave = true;
            break;
        }

        if (packetSize == 0)
        {
            recvPool.Flush(sizeof(PRO_UINT16));
            continue;
        }

        packet = CRtpPacket::CreateInstance(packetSize, RTP_EPM_DEFAULT);
        if (packet == NULL)
        {
            ret = false;
            break;
        }

        recvPool.Flush(sizeof(PRO_UINT16));
        recvPool.PeekData(packet->GetPayloadBuffer(), packetSize);
        recvPool.Flush(packetSize);

        RTP_HEADER  hdr;
        const char* payloadBuffer = NULL;
        PRO_UINT16  payloadSize   = 0;

        const bool ret2 = CRtpPacket::ParseRtpBuffer(
            (char*)packet->GetPayloadBuffer(),
            packet->GetPayloadSize16(),
            hdr,
            payloadBuffer,
            payloadSize
            );
        if (!ret2 || payloadBuffer == NULL || payloadSize == 0)
        {
            packet->Release();
            packet = NULL;
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
        break;
    } /* end of while (...) */

    return (ret);
}}
