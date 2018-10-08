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

#include "rtp_packet.h"
#include "rtp_framework.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_PAYLOAD_SIZE (1024 * 63) /* 60 + 3 */

/////////////////////////////////////////////////////////////////////////////
////

CRtpPacket*
CRtpPacket::CreateInstance(const void*   payloadBuffer,
                           unsigned long payloadSize)
{
    if (payloadBuffer == NULL || payloadSize == 0 || payloadSize > MAX_PAYLOAD_SIZE)
    {
        return (NULL);
    }

    CRtpPacket* packet = new CRtpPacket;
    packet->Init(payloadBuffer, (PRO_UINT16)payloadSize);
    if (packet->m_packet == NULL)
    {
        delete packet;
        packet = NULL;
    }

    return (packet);
}

CRtpPacket*
CRtpPacket::CreateInstance(unsigned long payloadSize)
{
    if (payloadSize == 0 || payloadSize > MAX_PAYLOAD_SIZE)
    {
        return (NULL);
    }

    CRtpPacket* packet = new CRtpPacket;
    packet->Init(NULL, (PRO_UINT16)payloadSize);
    if (packet->m_packet == NULL)
    {
        delete packet;
        packet = NULL;
    }

    return (packet);
}

bool
CRtpPacket::ParseRtpBuffer(const char*  buffer,
                           PRO_UINT16   size,
                           RTP_HEADER&  hdr, /* network byte order */
                           const char*& payloadBuffer,
                           PRO_UINT16&  payloadSize)
{
    payloadBuffer = NULL;
    payloadSize   = 0;

    if (buffer == NULL || size == 0)
    {
        return (false);
    }

    bool ret = false;

    do
    {
        unsigned long needSize = sizeof(RTP_HEADER); /* unsigned long, >=32bits!!! */
        const char*   now      = buffer;

        if (size < needSize)
        {
            break;
        }

        hdr = *(RTP_HEADER*)now;
        if (hdr.v != 2)
        {
            break;
        }

        now += sizeof(RTP_HEADER);

        needSize += 4 * hdr.cc;
        if (size < needSize)
        {
            break;
        }

        now += 4 * hdr.cc;

        if (hdr.x != 0)
        {
            needSize += 2;
            needSize += 2;
            if (size < needSize)
            {
                break;
            }

            now += 2;
            const PRO_UINT16 extCount = pbsd_ntoh16(*(PRO_UINT16*)now);
            now += 2;

            needSize += 4 * extCount;
            if (size < needSize)
            {
                break;
            }

            now += 4 * extCount;
        }

        payloadBuffer = now;
        payloadSize   = (PRO_UINT16)(buffer + size - now);

        if (hdr.p != 0)
        {
            if (size < needSize + 1)
            {
                break;
            }

            unsigned char paddingSize = buffer[size - 1];
            if (paddingSize < 1)
            {
                paddingSize = 1;
            }

            needSize += paddingSize;
            if (size < needSize)
            {
                break;
            }

            payloadSize -= paddingSize;
        }

        if (payloadSize == 0)
        {
            payloadBuffer = NULL;
        }

        ret = true;
    }
    while (0);

    if (!ret)
    {
        payloadBuffer = NULL;
        payloadSize   = 0;
    }

    return (ret);
}

bool
CRtpPacket::ParseExtBuffer(const char* buffer,
                           PRO_UINT16  size)
{
    if (buffer == NULL || size < sizeof(RTP_EXT) + sizeof(RTP_HEADER) + 1)
    {
        return (false);
    }

    RTP_EXT ext = *(RTP_EXT*)buffer;
    ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);

    if (sizeof(RTP_EXT) + ext.hdrAndPayloadSize != size)
    {
        return (false);
    }

    const RTP_HEADER* const hdr = (RTP_HEADER*)(buffer + sizeof(RTP_EXT));
    if (hdr->v != 2 || hdr->p != 0 || hdr->x != 0 || hdr->cc != 0)
    {
        return (false);
    }

    return (true);
}

CRtpPacket::CRtpPacket()
{
    m_packet = NULL;
    m_tick   = 0;
}

CRtpPacket::~CRtpPacket()
{
    ProFree(m_packet);
    m_packet = NULL;
}

void
CRtpPacket::Init(const void* payloadBuffer,
                 PRO_UINT16  payloadSize)
{
    m_packet = (RTP_PACKET*)ProMalloc(sizeof(RTP_PACKET) + payloadSize + 16); /* (n + 16) bytes, for ssl */
    if (m_packet == NULL)
    {
        return;
    }

    memset(m_packet, 0, sizeof(RTP_PACKET));
    m_packet->ext = (RTP_EXT*)m_packet->dummyBuffer;
    m_packet->hdr = (RTP_HEADER*)(m_packet->ext + 1);

    m_packet->ext->hdrAndPayloadSize = pbsd_hton16(sizeof(RTP_HEADER) + payloadSize);
    m_packet->hdr->v                 = 2;

    if (payloadBuffer != NULL)
    {
        memcpy(m_packet->hdr + 1, payloadBuffer, payloadSize);
    }
}

unsigned long
PRO_CALLTYPE
CRtpPacket::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CRtpPacket::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
PRO_CALLTYPE
CRtpPacket::SetMarker(bool m)
{
    m_packet->hdr->m = m ? 1 : 0;
}

bool
PRO_CALLTYPE
CRtpPacket::GetMarker() const
{
    return (m_packet->hdr->m != 0);
}

void
PRO_CALLTYPE
CRtpPacket::SetPayloadType(char pt)
{
    m_packet->hdr->pt = pt;
}

char
PRO_CALLTYPE
CRtpPacket::GetPayloadType() const
{
    return (m_packet->hdr->pt);
}

void
PRO_CALLTYPE
CRtpPacket::SetSequence(PRO_UINT16 seq)
{
    m_packet->hdr->seq = pbsd_hton16(seq);
}

PRO_UINT16
PRO_CALLTYPE
CRtpPacket::GetSequence() const
{
    return (pbsd_ntoh16(m_packet->hdr->seq));
}

void
PRO_CALLTYPE
CRtpPacket::SetTimeStamp(PRO_UINT32 ts)
{
    m_packet->hdr->ts = pbsd_hton32(ts);
}

PRO_UINT32
PRO_CALLTYPE
CRtpPacket::GetTimeStamp() const
{
    return (pbsd_ntoh32(m_packet->hdr->ts));
}

void
PRO_CALLTYPE
CRtpPacket::SetSsrc(PRO_UINT32 ssrc)
{
    m_packet->hdr->ssrc = pbsd_hton32(ssrc);
}

PRO_UINT32
PRO_CALLTYPE
CRtpPacket::GetSsrc() const
{
    return (pbsd_ntoh32(m_packet->hdr->ssrc));
}

void
PRO_CALLTYPE
CRtpPacket::SetMmId(PRO_UINT32 mmId)
{
    m_packet->ext->mmId = pbsd_hton32(mmId);
}

PRO_UINT32
PRO_CALLTYPE
CRtpPacket::GetMmId() const
{
    return (pbsd_ntoh32(m_packet->ext->mmId));
}

void
PRO_CALLTYPE
CRtpPacket::SetMmType(RTP_MM_TYPE mmType)
{
    m_packet->ext->mmType = mmType;
}

RTP_MM_TYPE
PRO_CALLTYPE
CRtpPacket::GetMmType() const
{
    return (m_packet->ext->mmType);
}

void
PRO_CALLTYPE
CRtpPacket::SetKeyFrame(bool keyFrame)
{
    m_packet->ext->keyFrame = keyFrame ? 1 : 0;
}

bool
PRO_CALLTYPE
CRtpPacket::GetKeyFrame() const
{
    return (m_packet->ext->keyFrame != 0);
}

void
PRO_CALLTYPE
CRtpPacket::SetFirstPacketOfFrame(bool firstPacket)
{
    m_packet->ext->firstPacketOfFrame = firstPacket ? 1 : 0;
}

bool
PRO_CALLTYPE
CRtpPacket::GetFirstPacketOfFrame() const
{
    return (m_packet->ext->firstPacketOfFrame != 0);
}

const void*
PRO_CALLTYPE
CRtpPacket::GetPayloadBuffer() const
{
    return (m_packet->hdr + 1);
}

void*
PRO_CALLTYPE
CRtpPacket::GetPayloadBuffer()
{
    return (m_packet->hdr + 1);
}

PRO_UINT16
PRO_CALLTYPE
CRtpPacket::GetPayloadSize() const
{
    const PRO_UINT16 hdrAndPayloadSize = pbsd_ntoh16(m_packet->ext->hdrAndPayloadSize);

    return (hdrAndPayloadSize - sizeof(RTP_HEADER));
}

void
PRO_CALLTYPE
CRtpPacket::SetTick_i(PRO_INT64 tick)
{
    m_tick = tick;
}

PRO_INT64
PRO_CALLTYPE
CRtpPacket::GetTick_i() const
{
    return (m_tick);
}

const RTP_PACKET&
CRtpPacket::GetPacket() const
{
    return (*m_packet);
}

RTP_PACKET&
CRtpPacket::GetPacket()
{
    return (*m_packet);
}
