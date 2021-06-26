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

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

#include <cassert>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define TRACE_EXT_NAME ".pro_rtp.trace"

#if !defined(_WIN32_WCE)
CProFileMonitor               g_fileMonitor;
#endif

static CRtpPortAllocator*     g_s_udpPortAllocator   = NULL;
static CRtpPortAllocator*     g_s_tcpPortAllocator   = NULL;
static volatile unsigned long g_s_keepaliveInSeconds = 60;
static volatile unsigned long g_s_flowctrlInSeconds  = 1;
static volatile unsigned long g_s_statInSeconds      = 5;
static unsigned long          g_s_udpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static unsigned long          g_s_udpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static unsigned long          g_s_udpRecvPoolSize[256];    /* mmType0 ~ mmType255 */
static unsigned long          g_s_tcpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static unsigned long          g_s_tcpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static unsigned long          g_s_tcpRecvPoolSize[256];    /* mmType0 ~ mmType255 */

/////////////////////////////////////////////////////////////////////////////
////

PRO_RTP_API
void
PRO_CALLTYPE
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

#if !defined(_WIN32_WCE)
    char exeRoot[1024] = "";
    ProGetExePath(exeRoot);
    strncat(exeRoot, TRACE_EXT_NAME, sizeof(exeRoot) - 1);
    g_fileMonitor.Init(exeRoot);
    g_fileMonitor.UpdateFileExist();
#endif
}

PRO_RTP_API
void
PRO_CALLTYPE
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
PRO_CALLTYPE
CreateRtpPacket(const void*       payloadBuffer,
                unsigned long     payloadSize,
                RTP_EXT_PACK_MODE packMode)      /* = RTP_EPM_DEFAULT */
{
    CRtpPacket* const packet =
        CRtpPacket::CreateInstance(payloadBuffer, payloadSize, packMode);

    return (packet);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CreateRtpPacketSpace(unsigned long     payloadSize,
                     RTP_EXT_PACK_MODE packMode) /* = RTP_EPM_DEFAULT */
{
    CRtpPacket* const packet =
        CRtpPacket::CreateInstance(payloadSize, packMode);

    return (packet);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CloneRtpPacket(const IRtpPacket* packet)
{
    CRtpPacket* const newPacket = CRtpPacket::Clone(packet);

    return (newPacket);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
ParseRtpStreamToPacket(const void* streamBuffer,
                       PRO_UINT16  streamSize)
{
    RTP_HEADER  hdr;
    const char* payloadBuffer = NULL;
    PRO_UINT16  payloadSize   = 0;

    const bool ret = CRtpPacket::ParseRtpBuffer(
        (char*)streamBuffer,
        streamSize,
        hdr,
        payloadBuffer,
        payloadSize
        );
    if (!ret || payloadBuffer == NULL || payloadSize == 0)
    {
        return (NULL);
    }

    hdr.v  = 2;
    hdr.p  = 0;
    hdr.x  = 0;
    hdr.cc = 0;

    IRtpPacket* const packet = CRtpPacket::CreateInstance(
        payloadBuffer, payloadSize, RTP_EPM_DEFAULT);
    if (packet == NULL)
    {
        return (NULL);
    }

    RTP_HEADER* const packetHdr =
        (RTP_HEADER*)((char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER));
    *packetHdr = hdr;

    return (packet);
}

PRO_RTP_API
const void*
PRO_CALLTYPE
FindRtpStreamFromPacket(const IRtpPacket* packet,
                        PRO_UINT16*       streamSize)
{
    assert(packet != NULL);
    assert(streamSize != NULL);
    if (packet == NULL || streamSize == NULL)
    {
        return (NULL);
    }

    if (packet->GetPackMode() != RTP_EPM_DEFAULT)
    {
        return (NULL);
    }

    const char* const payloadBuffer = (char*)packet->GetPayloadBuffer();
    const PRO_UINT16  payloadSize   = packet->GetPayloadSize16();

    *streamSize = payloadSize + sizeof(RTP_HEADER);

    return (payloadBuffer - sizeof(RTP_HEADER));
}

PRO_RTP_API
void
PRO_CALLTYPE
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
PRO_CALLTYPE
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
PRO_CALLTYPE
AllocRtpUdpPort()
{
    assert(g_s_udpPortAllocator != NULL);

    return (g_s_udpPortAllocator->AllocPort());
}

PRO_RTP_API
unsigned short
PRO_CALLTYPE
AllocRtpTcpPort()
{
    assert(g_s_tcpPortAllocator != NULL);

    return (g_s_tcpPortAllocator->AllocPort());
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpKeepaliveTimeout(unsigned long keepaliveInSeconds) /* = 60 */
{
    assert(keepaliveInSeconds > 0);
    if (keepaliveInSeconds == 0)
    {
        return;
    }

    g_s_keepaliveInSeconds = keepaliveInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpKeepaliveTimeout()
{
    return (g_s_keepaliveInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpFlowctrlTimeSpan(unsigned long flowctrlInSeconds) /* = 1 */
{
    assert(flowctrlInSeconds > 0);
    if (flowctrlInSeconds == 0)
    {
        return;
    }

    g_s_flowctrlInSeconds = flowctrlInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpFlowctrlTimeSpan()
{
    return (g_s_flowctrlInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpStatTimeSpan(unsigned long statInSeconds) /* = 5 */
{
    assert(statInSeconds > 0);
    if (statInSeconds == 0)
    {
        return;
    }

    g_s_statInSeconds = statInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpStatTimeSpan()
{
    return (g_s_statInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpUdpSocketParams(RTP_MM_TYPE   mmType,
                      unsigned long sockBufSizeRecv, /* = 0 */
                      unsigned long sockBufSizeSend, /* = 0 */
                      unsigned long recvPoolSize)    /* = 0 */
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
PRO_CALLTYPE
GetRtpUdpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize)    /* = NULL */
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
PRO_CALLTYPE
SetRtpTcpSocketParams(RTP_MM_TYPE   mmType,
                      unsigned long sockBufSizeRecv, /* = 0 */
                      unsigned long sockBufSizeSend, /* = 0 */
                      unsigned long recvPoolSize)    /* = 0 */
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
PRO_CALLTYPE
GetRtpTcpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize)    /* = NULL */
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
PRO_CALLTYPE
CreateRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig,        /* = NULL */
                 IRtpServiceObserver*         observer,
                 IProReactor*                 reactor,
                 RTP_MM_TYPE                  mmType,
                 unsigned short               serviceHubPort,
                 unsigned long                timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpService* const service =
        CRtpService::CreateInstance(sslConfig, mmType);
    if (service == NULL)
    {
        return (NULL);
    }

    if (!service->Init(observer, reactor, serviceHubPort, timeoutInSeconds))
    {
        service->Release();

        return (NULL);
    }

    return ((IRtpService*)service);
}

PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpService(IRtpService* service)
{
    if (service == NULL)
    {
        return;
    }

    CRtpService* const p = (CRtpService*)service;
    p->Fini();
    p->Release();
}

PRO_RTP_API
bool
PRO_CALLTYPE
CheckRtpServiceData(const char  serviceNonce[32],
                    const char* servicePassword,
                    const char  clientPasswordHash[32])
{
    char servicePasswordHash[32];
    ProCalcPasswordHash(serviceNonce, servicePassword, servicePasswordHash);

    return (memcmp(clientPasswordHash, servicePasswordHash, 32) == 0);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionWrapper(RTP_SESSION_TYPE        sessionType,
                        const RTP_INIT_ARGS*    initArgs,
                        const RTP_SESSION_INFO* localInfo)
{
    ProRtpInit();

    CRtpSessionWrapper* const sessionWrapper =
        CRtpSessionWrapper::CreateInstance(localInfo);
    if (sessionWrapper == NULL)
    {
        return (NULL);
    }

    if (!sessionWrapper->Init(sessionType, initArgs))
    {
        sessionWrapper->Release();

        return (NULL);
    }

    return (sessionWrapper);
}

PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpSessionWrapper(IRtpSession* sessionWrapper)
{
    if (sessionWrapper == NULL)
    {
        return;
    }

    CRtpSessionWrapper* const p = (CRtpSessionWrapper*)sessionWrapper;
    p->Fini();
    p->Release();
}

PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpBaseBucket()
{
    CRtpBucket* const bucket = new CRtpBucket;

    return (bucket);
}

PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpAudioBucket()
{
    CRtpAudioBucket* const bucket = new CRtpAudioBucket;

    return (bucket);
}

PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpVideoBucket()
{
    CRtpVideoBucket* const bucket = new CRtpVideoBucket;

    return (bucket);
}

PRO_RTP_API
IRtpReorder*
PRO_CALLTYPE
CreateRtpReorder()
{
    CRtpReorder* const reorder = new CRtpReorder;

    return (reorder);
}

PRO_RTP_API
void
PRO_CALLTYPE
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
