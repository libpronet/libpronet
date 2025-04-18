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

#include "rtp_base.h"
#include "rtp_bucket.h"
#include "rtp_packet.h"
#include "rtp_port_allocator.h"
#include "rtp_reorder.h"
#include "rtp_service.h"
#include "rtp_session_wrapper.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_file_monitor.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define TRACE_EXT_NAME ".pro_rtp.trace"

CProFileMonitor              g_fileMonitor;

static CRtpPortAllocator*    g_s_udpPortAllocator   = NULL;
static CRtpPortAllocator*    g_s_tcpPortAllocator   = NULL;
static volatile unsigned int g_s_keepaliveInSeconds = 60;
static volatile unsigned int g_s_flowctrlInSeconds  = 1;
static volatile unsigned int g_s_statInSeconds      = 5;
static size_t                g_s_udpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static size_t                g_s_udpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static size_t                g_s_udpRecvPoolSize[256];    /* mmType0 ~ mmType255 */
static size_t                g_s_tcpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static size_t                g_s_tcpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static size_t                g_s_tcpRecvPoolSize[256];    /* mmType0 ~ mmType255 */

/////////////////////////////////////////////////////////////////////////////
////

PRO_RTP_API
void
ProRtpInit()
{
    static bool s_flag = false;
    if (s_flag)
    {
        return;
    }
    s_flag = true;

    ProNetInit();

    g_s_udpPortAllocator = new CRtpPortAllocator;
    g_s_tcpPortAllocator = new CRtpPortAllocator;

    for (int i = 0; i < 256; ++i)
    {
        g_s_udpSockBufSizeRecv[i] = 0; /* zero by default */
        g_s_udpSockBufSizeSend[i] = 0; /* zero by default */
        g_s_udpRecvPoolSize[i]    = 1024 * 65;

        g_s_tcpSockBufSizeRecv[i] = 0; /* zero by default */
        g_s_tcpSockBufSizeSend[i] = 0; /* zero by default */
        g_s_tcpRecvPoolSize[i]    = 1024 * 65;
    }

    char exeRoot[1024] = "";
    ProGetExePath(exeRoot, NULL);
    strncat(exeRoot, TRACE_EXT_NAME, sizeof(exeRoot) - 1);
    g_fileMonitor.Init(exeRoot);
    g_fileMonitor.UpdateFileExist();
}

PRO_RTP_API
void
ProRtpVersion(unsigned char* major, /* = NULL */
              unsigned char* minor, /* = NULL */
              unsigned char* patch) /* = NULL */
{
    if (major != NULL)
    {
        *major = PRO_VER_MAJOR;
    }
    if (minor != NULL)
    {
        *minor = PRO_VER_MINOR;
    }
    if (patch != NULL)
    {
        *patch = PRO_VER_PATCH;
    }
}

PRO_RTP_API
IRtpPacket*
CreateRtpPacket(const void*       payloadBuffer,
                size_t            payloadSize,
                RTP_EXT_PACK_MODE packMode) /* = RTP_EPM_DEFAULT */
{
    return CRtpPacket::CreateInstance(payloadBuffer, payloadSize, packMode);
}

PRO_RTP_API
IRtpPacket*
CreateRtpPacketSpace(size_t            payloadSize,
                     RTP_EXT_PACK_MODE packMode) /* = RTP_EPM_DEFAULT */
{
    return CRtpPacket::CreateInstance(payloadSize, packMode);
}

PRO_RTP_API
IRtpPacket*
CloneRtpPacket(const IRtpPacket* packet)
{
    return CRtpPacket::Clone(packet);
}

PRO_RTP_API
IRtpPacket*
ParseRtpStreamToPacket(const void* streamBuffer,
                       uint16_t    streamSize)
{
    RTP_HEADER  hdr;
    const char* payloadBuffer = NULL;
    uint16_t    payloadSize   = 0;

    bool ret = CRtpPacket::ParseRtpBuffer(
        (char*)streamBuffer,
        streamSize,
        hdr,
        payloadBuffer,
        payloadSize
        );
    if (!ret || payloadBuffer == NULL || payloadSize == 0)
    {
        return NULL;
    }

    hdr.v  = 2;
    hdr.p  = 0;
    hdr.x  = 0;
    hdr.cc = 0;

    IRtpPacket* packet = CRtpPacket::CreateInstance(
        payloadBuffer, payloadSize, RTP_EPM_DEFAULT);
    if (packet == NULL)
    {
        return NULL;
    }

    RTP_HEADER* packetHdr = (RTP_HEADER*)((char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER));
    *packetHdr = hdr;

    return packet;
}

PRO_RTP_API
const void*
FindRtpStreamFromPacket(const IRtpPacket* packet,
                        uint16_t*         streamSize)
{
    assert(packet != NULL);
    assert(streamSize != NULL);
    if (packet == NULL || streamSize == NULL)
    {
        return NULL;
    }

    if (packet->GetPackMode() != RTP_EPM_DEFAULT)
    {
        return NULL;
    }

    const char* payloadBuffer = (char*)packet->GetPayloadBuffer();
    uint16_t    payloadSize   = packet->GetPayloadSize16();

    *streamSize = payloadSize + sizeof(RTP_HEADER);

    return payloadBuffer - sizeof(RTP_HEADER);
}

PRO_RTP_API
void
SetRtpPortRange(unsigned short minUdpPort, /* = 0 */
                unsigned short maxUdpPort, /* = 0 */
                unsigned short minTcpPort, /* = 0 */
                unsigned short maxTcpPort) /* = 0 */
{
    assert(g_s_udpPortAllocator != NULL);
    assert(g_s_tcpPortAllocator != NULL);

    g_s_udpPortAllocator->SetPortRange(minUdpPort, maxUdpPort);
    g_s_tcpPortAllocator->SetPortRange(minTcpPort, maxTcpPort);
}

PRO_RTP_API
void
GetRtpPortRange(unsigned short* minUdpPort, /* = NULL */
                unsigned short* maxUdpPort, /* = NULL */
                unsigned short* minTcpPort, /* = NULL */
                unsigned short* maxTcpPort) /* = NULL */
{
    assert(g_s_udpPortAllocator != NULL);
    assert(g_s_tcpPortAllocator != NULL);

    unsigned short minUdpPort2 = 0;
    unsigned short maxUdpPort2 = 0;
    unsigned short minTcpPort2 = 0;
    unsigned short maxTcpPort2 = 0;
    g_s_udpPortAllocator->GetPortRange(minUdpPort2, maxUdpPort2);
    g_s_tcpPortAllocator->GetPortRange(minTcpPort2, maxTcpPort2);

    if (minUdpPort != NULL)
    {
        *minUdpPort = minUdpPort2;
    }
    if (maxUdpPort != NULL)
    {
        *maxUdpPort = maxUdpPort2;
    }
    if (minTcpPort != NULL)
    {
        *minTcpPort = minTcpPort2;
    }
    if (maxTcpPort != NULL)
    {
        *maxTcpPort = maxTcpPort2;
    }
}

PRO_RTP_API
unsigned short
AllocRtpUdpPort(bool rfc)
{
    assert(g_s_udpPortAllocator != NULL);

    return g_s_udpPortAllocator->AllocPort(rfc);
}

PRO_RTP_API
unsigned short
AllocRtpTcpPort(bool rfc)
{
    assert(g_s_tcpPortAllocator != NULL);

    return g_s_tcpPortAllocator->AllocPort(rfc);
}

PRO_RTP_API
void
SetRtpKeepaliveTimeout(unsigned int keepaliveInSeconds) /* = 60 */
{
    assert(keepaliveInSeconds > 0);
    if (keepaliveInSeconds == 0)
    {
        return;
    }

    g_s_keepaliveInSeconds = keepaliveInSeconds;
}

PRO_RTP_API
unsigned int
GetRtpKeepaliveTimeout()
{
    return g_s_keepaliveInSeconds;
}

PRO_RTP_API
void
SetRtpFlowctrlTimeSpan(unsigned int flowctrlInSeconds) /* = 1 */
{
    assert(flowctrlInSeconds > 0);
    if (flowctrlInSeconds == 0)
    {
        return;
    }

    g_s_flowctrlInSeconds = flowctrlInSeconds;
}

PRO_RTP_API
unsigned int
GetRtpFlowctrlTimeSpan()
{
    return g_s_flowctrlInSeconds;
}

PRO_RTP_API
void
SetRtpStatTimeSpan(unsigned int statInSeconds) /* = 5 */
{
    assert(statInSeconds > 0);
    if (statInSeconds == 0)
    {
        return;
    }

    g_s_statInSeconds = statInSeconds;
}

PRO_RTP_API
unsigned int
GetRtpStatTimeSpan()
{
    return g_s_statInSeconds;
}

PRO_RTP_API
void
SetRtpUdpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize)    /* = 0 */
{
    if (sockBufSizeRecv > 0)
    {
        g_s_udpSockBufSizeRecv[mmType] = sockBufSizeRecv;
    }
    if (sockBufSizeSend > 0)
    {
        g_s_udpSockBufSizeSend[mmType] = sockBufSizeSend;
    }
    if (recvPoolSize > 0)
    {
        g_s_udpRecvPoolSize[mmType]    = recvPoolSize;
    }
}

PRO_RTP_API
void
GetRtpUdpSocketParams(RTP_MM_TYPE mmType,
                      size_t*     sockBufSizeRecv, /* = NULL */
                      size_t*     sockBufSizeSend, /* = NULL */
                      size_t*     recvPoolSize)    /* = NULL */
{
    if (sockBufSizeRecv != NULL)
    {
        *sockBufSizeRecv = g_s_udpSockBufSizeRecv[mmType];
    }
    if (sockBufSizeSend != NULL)
    {
        *sockBufSizeSend = g_s_udpSockBufSizeSend[mmType];
    }
    if (recvPoolSize != NULL)
    {
        *recvPoolSize    = g_s_udpRecvPoolSize[mmType];
    }
}

PRO_RTP_API
void
SetRtpTcpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize)    /* = 0 */
{
    if (sockBufSizeRecv > 0)
    {
        g_s_tcpSockBufSizeRecv[mmType] = sockBufSizeRecv;
    }
    if (sockBufSizeSend > 0)
    {
        g_s_tcpSockBufSizeSend[mmType] = sockBufSizeSend;
    }
    if (recvPoolSize > 0)
    {
        g_s_tcpRecvPoolSize[mmType]    = recvPoolSize;
    }
}

PRO_RTP_API
void
GetRtpTcpSocketParams(RTP_MM_TYPE mmType,
                      size_t*     sockBufSizeRecv, /* = NULL */
                      size_t*     sockBufSizeSend, /* = NULL */
                      size_t*     recvPoolSize)    /* = NULL */
{
    if (sockBufSizeRecv != NULL)
    {
        *sockBufSizeRecv = g_s_tcpSockBufSizeRecv[mmType];
    }
    if (sockBufSizeSend != NULL)
    {
        *sockBufSizeSend = g_s_tcpSockBufSizeSend[mmType];
    }
    if (recvPoolSize != NULL)
    {
        *recvPoolSize    = g_s_tcpRecvPoolSize[mmType];
    }
}

PRO_RTP_API
IRtpService*
CreateRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig,        /* = NULL */
                 IRtpServiceObserver*         observer,
                 IProReactor*                 reactor,
                 RTP_MM_TYPE                  mmType,
                 unsigned short               serviceHubPort,
                 unsigned int                 timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpService* service = CRtpService::CreateInstance(sslConfig, mmType);
    if (service == NULL)
    {
        return NULL;
    }

    if (!service->Init(observer, reactor, serviceHubPort, timeoutInSeconds))
    {
        service->Release();

        return NULL;
    }

    return (IRtpService*)service;
}

PRO_RTP_API
void
DeleteRtpService(IRtpService* service)
{
    if (service == NULL)
    {
        return;
    }

    CRtpService* p = (CRtpService*)service;
    p->Fini();
    p->Release();
}

PRO_RTP_API
bool
CheckRtpServiceData(const unsigned char serviceNonce[32],
                    const char*         servicePassword,
                    const unsigned char clientPasswordHash[32])
{
    unsigned char servicePasswordHash[32];
    ProCalcPasswordHash(serviceNonce, servicePassword, servicePasswordHash);

    return memcmp(clientPasswordHash, servicePasswordHash, 32) == 0;
}

PRO_RTP_API
IRtpSession*
CreateRtpSessionWrapper(RTP_SESSION_TYPE        sessionType,
                        const RTP_INIT_ARGS*    initArgs,
                        const RTP_SESSION_INFO* localInfo)
{
    ProRtpInit();

    CRtpSessionWrapper* sessionWrapper = CRtpSessionWrapper::CreateInstance(localInfo);
    if (sessionWrapper == NULL)
    {
        return NULL;
    }

    if (!sessionWrapper->Init(sessionType, initArgs))
    {
        sessionWrapper->Release();

        return NULL;
    }

    return sessionWrapper;
}

PRO_RTP_API
void
DeleteRtpSessionWrapper(IRtpSession* sessionWrapper)
{
    if (sessionWrapper == NULL)
    {
        return;
    }

    CRtpSessionWrapper* p = (CRtpSessionWrapper*)sessionWrapper;
    p->Fini();
    p->Release();
}

PRO_RTP_API
IRtpBucket*
CreateRtpBaseBucket()
{
    return new CRtpBucket;
}

PRO_RTP_API
IRtpBucket*
CreateRtpAudioBucket()
{
    return new CRtpAudioBucket;
}

PRO_RTP_API
IRtpBucket*
CreateRtpVideoBucket()
{
    return new CRtpVideoBucket;
}

PRO_RTP_API
IRtpReorder*
CreateRtpReorder()
{
    return new CRtpReorder;
}

PRO_RTP_API
void
DeleteRtpReorder(IRtpReorder* reorder)
{
    delete (CRtpReorder*)reorder;
}

/////////////////////////////////////////////////////////////////////////////
////

class CRtpBaseDotCpp
{
public:

    CRtpBaseDotCpp()
    {
        ProRtpInit();
    }
};

static volatile CRtpBaseDotCpp g_s_initiator;

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
