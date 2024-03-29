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

#include "rtp_service.h"
#include "rtp_base.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CRtpService*
CRtpService::CreateInstance(const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                            RTP_MM_TYPE                  mmType)
{
    assert(mmType != 0);
    if (mmType == 0)
    {
        return NULL;
    }

    return new CRtpService(sslConfig, mmType);
}

CRtpService::CRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                         RTP_MM_TYPE                  mmType)
:
m_sslConfig(sslConfig),
m_mmType(mmType)
{
    m_observer         = NULL;
    m_reactor          = NULL;
    m_serviceHost      = NULL;
    m_timeoutInSeconds = 0;
}

CRtpService::~CRtpService()
{
    Fini();
}

bool
CRtpService::Init(IRtpServiceObserver* observer,
                  IProReactor*         reactor,
                  unsigned short       serviceHubPort,
                  unsigned int         timeoutInSeconds) /* = 0 */
{
    assert(observer != NULL);
    assert(reactor != NULL);
    assert(serviceHubPort > 0);
    if (observer == NULL || reactor == NULL || serviceHubPort == 0)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactor == NULL);
        assert(m_serviceHost == NULL);
        if (m_observer != NULL || m_reactor != NULL || m_serviceHost != NULL)
        {
            return false;
        }

        m_serviceHost = ProCreateServiceHostEx(this, reactor, serviceHubPort, m_mmType);
        if (m_serviceHost == NULL)
        {
            return false;
        }

        observer->AddRef();
        m_observer         = observer;
        m_reactor          = reactor;
        m_timeoutInSeconds = timeoutInSeconds;
    }

    return true;
}

void
CRtpService::Fini()
{
    IRtpServiceObserver*                      observer    = NULL;
    IProServiceHost*                          serviceHost = NULL;
    CProStlMap<IProTcpHandshaker*, PRO_NONCE> tcpHandshaker2Nonce;
    CProStlMap<IProSslHandshaker*, PRO_NONCE> sslHandshaker2Nonce;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            return;
        }

        sslHandshaker2Nonce = m_sslHandshaker2Nonce;
        m_sslHandshaker2Nonce.clear();
        tcpHandshaker2Nonce = m_tcpHandshaker2Nonce;
        m_tcpHandshaker2Nonce.clear();
        serviceHost = m_serviceHost;
        m_serviceHost = NULL;
        m_reactor = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    {
        auto itr = sslHandshaker2Nonce.begin();
        auto end = sslHandshaker2Nonce.end();

        for (; itr != end; ++itr)
        {
            ProDeleteSslHandshaker(itr->first);
        }
    }

    {
        auto itr = tcpHandshaker2Nonce.begin();
        auto end = tcpHandshaker2Nonce.end();

        for (; itr != end; ++itr)
        {
            ProDeleteTcpHandshaker(itr->first);
        }
    }

    ProDeleteServiceHost(serviceHost);
    observer->Release();
}

unsigned long
CRtpService::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CRtpService::Release()
{
    return CProRefCount::Release();
}

void
CRtpService::OnServiceAccept(IProServiceHost* serviceHost,
                             int64_t          sockId,
                             bool             unixSocket,
                             const char*      localIp,
                             const char*      remoteIp,
                             unsigned short   remotePort,
                             unsigned char    serviceId,
                             unsigned char    serviceOpt,
                             const PRO_NONCE* nonce)
{
    assert(serviceHost != NULL);
    assert(sockId != -1);
    assert(serviceId > 0);
    assert(nonce != NULL);
    if (serviceHost == NULL || sockId == -1 || serviceId == 0 || nonce == NULL)
    {
        return;
    }

    RTP_MM_TYPE mmType = serviceId;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (serviceHost != m_serviceHost || mmType != m_mmType)
        {
            ProCloseSockId(sockId);

            return;
        }

        /*
         * ddos?
         */
        if (m_tcpHandshaker2Nonce.size() + m_sslHandshaker2Nonce.size() >= PRO_SERVICER_LENGTH)
        {
            ProCloseSockId(sockId);

            return;
        }

        if (serviceOpt == 0)
        {
            IProTcpHandshaker* handshaker = ProCreateTcpHandshaker(
                this,
                m_reactor,
                sockId,
                unixSocket,
                NULL,                                                            /* sendData */
                0,                                                               /* sendDataSize */
                sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO), /* recvDataSize */
                false,                                                           /* recvFirst is false */
                m_timeoutInSeconds
                );
            if (handshaker == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            m_tcpHandshaker2Nonce[handshaker] = *nonce;
        }
        else
        {
            if (m_sslConfig == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            PRO_SSL_CTX* sslCtx = ProSslCtx_CreateS(m_sslConfig, sockId, nonce);
            if (sslCtx == NULL)
            {
                ProCloseSockId(sockId);

                return;
            }

            IProSslHandshaker* handshaker = ProCreateSslHandshaker(
                this,
                m_reactor,
                sslCtx,
                sockId,
                unixSocket,
                NULL,                                                            /* sendData */
                0,                                                               /* sendDataSize */
                sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO), /* recvDataSize */
                false,                                                           /* recvFirst is false */
                m_timeoutInSeconds
                );
            if (handshaker == NULL)
            {
                ProSslCtx_Delete(sslCtx);
                ProCloseSockId(sockId);

                return;
            }

            m_sslHandshaker2Nonce[handshaker] = *nonce;
        }
    }
}

void
CRtpService::OnHandshakeOk(IProTcpHandshaker* handshaker,
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

    IRtpServiceObserver* observer = NULL;
    PRO_NONCE            nonce;
    pbsd_sockaddr_in     remoteAddr;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            ProCloseSockId(sockId);

            return;
        }

        auto itr = m_tcpHandshaker2Nonce.find(handshaker);
        if (itr == m_tcpHandshaker2Nonce.end())
        {
            ProCloseSockId(sockId);

            return;
        }

        if (!unixSocket && pbsd_getpeername(sockId, &remoteAddr) != 0)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        assert(buf != NULL);
        assert(size == sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO));
        if (buf == NULL ||
            size != sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO))
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        nonce = itr->second;
        m_tcpHandshaker2Nonce.erase(itr);

        m_observer->AddRef();
        observer = m_observer;
    }

    ProDeleteTcpHandshaker(handshaker);

    if (sockId == -1)
    {
        observer->Release();

        return;
    }

    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    if (!unixSocket)
    {
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    if (!CRtpPacket::ParseExtBuffer((char*)buf, (uint16_t)size))
    {
        ProCloseSockId(sockId);
    }
    else
    {
        RTP_SESSION_INFO remoteInfo;
        memcpy(
            &remoteInfo,
            (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
            (uint16_t)size - sizeof(RTP_EXT) - sizeof(RTP_HEADER)
            );
        remoteInfo.localVersion  = pbsd_ntoh16(remoteInfo.localVersion);
        remoteInfo.remoteVersion = pbsd_ntoh16(remoteInfo.remoteVersion);
        remoteInfo.someId        = pbsd_ntoh32(remoteInfo.someId);
        remoteInfo.mmId          = pbsd_ntoh32(remoteInfo.mmId);
        remoteInfo.inSrcMmId     = pbsd_ntoh32(remoteInfo.inSrcMmId);
        remoteInfo.outSrcMmId    = pbsd_ntoh32(remoteInfo.outSrcMmId);

        assert(
            remoteInfo.packMode == RTP_EPM_DEFAULT ||
            remoteInfo.packMode == RTP_EPM_TCP2    ||
            remoteInfo.packMode == RTP_EPM_TCP4
            );
        if (remoteInfo.packMode != RTP_EPM_DEFAULT &&
            remoteInfo.packMode != RTP_EPM_TCP2    &&
            remoteInfo.packMode != RTP_EPM_TCP4)
        {
            ProCloseSockId(sockId);
        }
        else
        {
            observer->OnAcceptSession(
                (IRtpService*)this,
                sockId,
                unixSocket,
                remoteIp,
                remotePort,
                &remoteInfo,
                &nonce
                );
        }
    }

    observer->Release();
}

void
CRtpService::OnHandshakeError(IProTcpHandshaker* handshaker,
                              int                errorCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            return;
        }

        auto itr = m_tcpHandshaker2Nonce.find(handshaker);
        if (itr == m_tcpHandshaker2Nonce.end())
        {
            return;
        }

        m_tcpHandshaker2Nonce.erase(itr);
    }

    ProDeleteTcpHandshaker(handshaker);
}

void
CRtpService::OnHandshakeOk(IProSslHandshaker* handshaker,
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

    IRtpServiceObserver* observer = NULL;
    PRO_NONCE            nonce;
    pbsd_sockaddr_in     remoteAddr;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        auto itr = m_sslHandshaker2Nonce.find(handshaker);
        if (itr == m_sslHandshaker2Nonce.end())
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);

            return;
        }

        if (!unixSocket && pbsd_getpeername(sockId, &remoteAddr) != 0)
        {
            ProCloseSockId(sockId);
            sockId = -1;
        }

        assert(ctx != NULL);
        assert(buf != NULL);
        assert(size == sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO));
        if (ctx == NULL || buf == NULL ||
            size != sizeof(RTP_EXT) + sizeof(RTP_HEADER) + sizeof(RTP_SESSION_INFO))
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);
            ctx    = NULL;
            sockId = -1;
        }

        nonce = itr->second;
        m_sslHandshaker2Nonce.erase(itr);

        m_observer->AddRef();
        observer = m_observer;
    }

    ProDeleteSslHandshaker(handshaker);

    if (sockId == -1)
    {
        ProSslCtx_Delete(ctx);
        observer->Release();

        return;
    }

    char           remoteIp[64] = "127.0.0.1"; /* a dummy for unix socket */
    unsigned short remotePort   = 65535;       /* a dummy for unix socket */
    if (!unixSocket)
    {
        pbsd_inet_ntoa(remoteAddr.sin_addr.s_addr, remoteIp);
        remotePort = pbsd_ntoh16(remoteAddr.sin_port);
    }

    if (!CRtpPacket::ParseExtBuffer((char*)buf, (uint16_t)size))
    {
        ProSslCtx_Delete(ctx);
        ProCloseSockId(sockId);
    }
    else
    {
        RTP_SESSION_INFO remoteInfo;
        memcpy(
            &remoteInfo,
            (char*)buf + sizeof(RTP_EXT) + sizeof(RTP_HEADER),
            (uint16_t)size - sizeof(RTP_EXT) - sizeof(RTP_HEADER)
            );
        remoteInfo.localVersion  = pbsd_ntoh16(remoteInfo.localVersion);
        remoteInfo.remoteVersion = pbsd_ntoh16(remoteInfo.remoteVersion);
        remoteInfo.someId        = pbsd_ntoh32(remoteInfo.someId);
        remoteInfo.mmId          = pbsd_ntoh32(remoteInfo.mmId);
        remoteInfo.inSrcMmId     = pbsd_ntoh32(remoteInfo.inSrcMmId);
        remoteInfo.outSrcMmId    = pbsd_ntoh32(remoteInfo.outSrcMmId);

        assert(
            remoteInfo.packMode == RTP_EPM_DEFAULT ||
            remoteInfo.packMode == RTP_EPM_TCP2    ||
            remoteInfo.packMode == RTP_EPM_TCP4
            );
        if (remoteInfo.packMode != RTP_EPM_DEFAULT &&
            remoteInfo.packMode != RTP_EPM_TCP2    &&
            remoteInfo.packMode != RTP_EPM_TCP4)
        {
            ProSslCtx_Delete(ctx);
            ProCloseSockId(sockId);
        }
        else
        {
            observer->OnAcceptSession(
                (IRtpService*)this,
                ctx,
                sockId,
                unixSocket,
                remoteIp,
                remotePort,
                &remoteInfo,
                &nonce
                );
        }
    }

    observer->Release();
}

void
CRtpService::OnHandshakeError(IProSslHandshaker* handshaker,
                              int                errorCode,
                              int                sslCode)
{
    assert(handshaker != NULL);
    if (handshaker == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactor == NULL || m_serviceHost == NULL)
        {
            return;
        }

        auto itr = m_sslHandshaker2Nonce.find(handshaker);
        if (itr == m_sslHandshaker2Nonce.end())
        {
            return;
        }

        m_sslHandshaker2Nonce.erase(itr);
    }

    ProDeleteSslHandshaker(handshaker);
}
