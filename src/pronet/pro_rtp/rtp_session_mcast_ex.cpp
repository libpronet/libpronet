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

#include "rtp_session_mcast_ex.h"
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

CRtpSessionMcastEx*
CRtpSessionMcastEx::CreateInstance(const RTP_SESSION_INFO* localInfo)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    if (localInfo == NULL || localInfo->mmType == 0)
    {
        return (NULL);
    }

    CRtpSessionMcastEx* const session = new CRtpSessionMcastEx(*localInfo);

    return (session);
}

CRtpSessionMcastEx::CRtpSessionMcastEx(const RTP_SESSION_INFO& localInfo)
: CRtpSessionBase(false)
{
    m_info               = localInfo;
    m_info.localVersion  = RTP_SESSION_PROTOCOL_VERSION;
    m_info.remoteVersion = 0;
    m_info.sessionType   = RTP_ST_MCAST_EX;
    m_info.inSrcMmId     = 0;
    m_info.outSrcMmId    = 0;
}

CRtpSessionMcastEx::~CRtpSessionMcastEx()
{
    Fini();
}

bool
CRtpSessionMcastEx::Init(IRtpSessionObserver* observer,
                         IProReactor*         reactor,
                         const char*          mcastIp,
                         unsigned short       mcastPort, /* = 0 */
                         const char*          localIp)   /* = NULL */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(mcastIp != NULL);
    assert(mcastIp[0] != '\0');
    if (observer == NULL || reactor == NULL || mcastIp == NULL ||
        mcastIp[0] == '\0')
    {
        return (false);
    }

    unsigned long sockBufSizeRecv = 0; /* zero by default */
    unsigned long sockBufSizeSend = 0; /* zero by default */
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
        if (mcastPort > 0)
        {
            count = 1;
        }

        for (int i = 0; i < count; ++i)
        {
            unsigned short mcastPort2 = mcastPort;
            if (mcastPort2 == 0)
            {
                mcastPort2 = AllocRtpUdpPort(false); /* rfc is false */
            }

            m_trans = ProCreateMcastTransport(
                this, reactor, mcastIp, mcastPort2, localIp,
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
        m_localAddr.sin_family       = AF_INET;
        m_localAddr.sin_port         = pbsd_hton16(m_trans->GetLocalPort());
        m_localAddr.sin_addr.s_addr  = pbsd_inet_aton(m_trans->GetLocalIp(theIp));
        m_remoteAddr.sin_family      = AF_INET;
        m_remoteAddr.sin_port        = pbsd_hton16(m_trans->GetRemotePort());
        m_remoteAddr.sin_addr.s_addr = pbsd_inet_aton(m_trans->GetRemoteIp(theIp));

        m_trans->StartHeartbeat();

        observer->AddRef();
        m_observer    = observer;
        m_reactor     = reactor;
        m_onOkTimerId = reactor->ScheduleTimer(this, 0, false);
    }

    return (true);
}

void
CRtpSessionMcastEx::Fini()
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

bool
PRO_CALLTYPE
CRtpSessionMcastEx::AddMcastReceiver(const char* mcastIp)
{
    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return (false);
        }

        ret = m_trans->AddMcastReceiver(mcastIp);
    }

    return (ret);
}

void
PRO_CALLTYPE
CRtpSessionMcastEx::RemoveMcastReceiver(const char* mcastIp)
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_trans == NULL)
        {
            return;
        }

        m_trans->RemoveMcastReceiver(mcastIp);
    }
}

void
PRO_CALLTYPE
CRtpSessionMcastEx::OnRecv(IProTransport*          trans,
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
            if (dataSize != sizeof(RTP_EXT) + ext.hdrAndPayloadSize)
            {
                recvPool.Flush(dataSize);
                break;
            }

            if (ext.hdrAndPayloadSize == 0)
            {
                m_peerAliveTick = ProGetTickCount64();

                recvPool.Flush(dataSize);
                break;
            }

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

                assert(
                    m_info.inSrcMmId  == 0 ||
                    packet->GetMmId() == m_info.inSrcMmId
                    );
                assert(packet->GetMmType() == m_info.mmType);
                if (
                    (m_info.inSrcMmId  != 0 &&
                     packet->GetMmId() != m_info.inSrcMmId)
                    ||
                    packet->GetMmType() != m_info.mmType /* drop this packet */
                   )
                {
                    packet->Release();
                    packet = NULL;
                    break;
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
