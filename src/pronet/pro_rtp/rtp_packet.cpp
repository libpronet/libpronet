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

#include "rtp_packet.h"
#include "rtp_base.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define TCP2_PAYLOAD_SIZE (1024 * 63)

/////////////////////////////////////////////////////////////////////////////
////

CRtpPacket*
CRtpPacket::CreateInstance(const void*       payloadBuffer,
                           size_t            payloadSize,
                           RTP_EXT_PACK_MODE packMode)
{
    assert(payloadBuffer != NULL);
    assert(payloadSize > 0);
    if (payloadBuffer == NULL || payloadSize == 0)
    {
        return NULL;
    }

    assert(packMode == RTP_EPM_DEFAULT || packMode == RTP_EPM_TCP2 || packMode == RTP_EPM_TCP4);
    if (packMode != RTP_EPM_DEFAULT && packMode != RTP_EPM_TCP2 && packMode != RTP_EPM_TCP4)
    {
        return NULL;
    }

    if (packMode == RTP_EPM_DEFAULT || packMode == RTP_EPM_TCP2)
    {
        assert(payloadSize <= TCP2_PAYLOAD_SIZE);
        if (payloadSize > TCP2_PAYLOAD_SIZE)
        {
            return NULL;
        }
    }
    else
    {
        assert(payloadSize <= PRO_TCP4_PAYLOAD_SIZE);
        if (payloadSize > PRO_TCP4_PAYLOAD_SIZE)
        {
            return NULL;
        }
    }

    CRtpPacket* packet = new CRtpPacket(packMode);
    packet->Init(payloadBuffer, payloadSize);
    if (packet->m_packet == NULL)
    {
        delete packet;
        packet = NULL;
    }

    return packet;
}

CRtpPacket*
CRtpPacket::CreateInstance(size_t            payloadSize,
                           RTP_EXT_PACK_MODE packMode)
{
    assert(payloadSize > 0);
    if (payloadSize == 0)
    {
        return NULL;
    }

    assert(packMode == RTP_EPM_DEFAULT || packMode == RTP_EPM_TCP2 || packMode == RTP_EPM_TCP4);
    if (packMode != RTP_EPM_DEFAULT && packMode != RTP_EPM_TCP2 && packMode != RTP_EPM_TCP4)
    {
        return NULL;
    }

    if (packMode == RTP_EPM_DEFAULT || packMode == RTP_EPM_TCP2)
    {
        assert(payloadSize <= TCP2_PAYLOAD_SIZE);
        if (payloadSize > TCP2_PAYLOAD_SIZE)
        {
            return NULL;
        }
    }
    else
    {
        assert(payloadSize <= PRO_TCP4_PAYLOAD_SIZE);
        if (payloadSize > PRO_TCP4_PAYLOAD_SIZE)
        {
            return NULL;
        }
    }

    CRtpPacket* packet = new CRtpPacket(packMode);
    packet->Init(NULL, payloadSize);
    if (packet->m_packet == NULL)
    {
        delete packet;
        packet = NULL;
    }

    return packet;
}

CRtpPacket*
CRtpPacket::Clone(const IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return NULL;
    }

    CRtpPacket* packet2 = (CRtpPacket*)packet;

    CRtpPacket* newPacket = new CRtpPacket(packet2->m_packMode);
    newPacket->Init(packet2->GetPayloadBuffer(), packet2->GetPayloadSize());
    if (newPacket->m_packet == NULL)
    {
        delete newPacket;
        newPacket = NULL;
    }
    else
    {
        newPacket->m_ssrc         = packet2->m_ssrc;
        newPacket->m_magic        = packet2->m_magic;
        newPacket->m_magic2       = packet2->m_magic2;
        *newPacket->m_packet->ext = *packet2->m_packet->ext;
        *newPacket->m_packet->hdr = *packet2->m_packet->hdr;
    }

    return newPacket;
}

bool
CRtpPacket::ParseRtpBuffer(const char*  buffer,
                           uint16_t     size,
                           RTP_HEADER&  hdr, /* network byte order */
                           const char*& payloadBuffer,
                           uint16_t&    payloadSize)
{
    payloadBuffer = NULL;
    payloadSize   = 0;

    assert(buffer != NULL);
    assert(size > 0);
    if (buffer == NULL || size == 0)
    {
        return false;
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

        memcpy(&hdr, now, sizeof(RTP_HEADER));
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

            uint16_t extCount = 0;
            ((char*)&extCount)[0] = now[0];
            ((char*)&extCount)[1] = now[1];
            extCount = pbsd_ntoh16(extCount);

            now += 2;

            needSize += 4 * extCount;
            if (size < needSize)
            {
                break;
            }

            now += 4 * extCount;
        }

        payloadBuffer = now;
        payloadSize   = (uint16_t)(buffer + size - now);

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

    return ret;
}

bool
CRtpPacket::ParseExtBuffer(const char* buffer,
                           uint16_t    size)
{
    assert(buffer != NULL);
    assert(size > sizeof(RTP_EXT) + sizeof(RTP_HEADER));
    if (buffer == NULL || size <= sizeof(RTP_EXT) + sizeof(RTP_HEADER))
    {
        return false;
    }

    RTP_EXT ext;
    memcpy(&ext, buffer, sizeof(RTP_EXT));
    ext.hdrAndPayloadSize = pbsd_ntoh16(ext.hdrAndPayloadSize);
    if (sizeof(RTP_EXT) + ext.hdrAndPayloadSize != size)
    {
        return false;
    }

    RTP_HEADER hdr;
    memcpy(&hdr, buffer + sizeof(RTP_EXT), sizeof(RTP_HEADER));
    if (hdr.v != 2 || hdr.p != 0 || hdr.x != 0 || hdr.cc != 0)
    {
        return false;
    }

    return true;
}

CRtpPacket::CRtpPacket(RTP_EXT_PACK_MODE packMode)
: m_packMode(packMode)
{
    m_ssrc   = 0;
    m_magic  = 0;
    m_magic2 = 0;
    m_packet = NULL;
}

CRtpPacket::~CRtpPacket()
{
    ProFree(m_packet);
    m_packet = NULL;
}

void
CRtpPacket::Init(const void* payloadBuffer,
                 size_t      payloadSize)
{
    assert(payloadSize > 0);
    if (payloadSize == 0)
    {
        return;
    }

    m_packet = (RTP_PACKET*)ProMalloc(sizeof(RTP_PACKET) + payloadSize + 16); /* (n + 16) bytes, for ssl */
    if (m_packet == NULL)
    {
        return;
    }

    memset(m_packet, 0, sizeof(RTP_PACKET));
    m_packet->ext = (RTP_EXT*)m_packet->dummyBuffer;
    m_packet->hdr = (RTP_HEADER*)(m_packet->ext + 1);

    uint16_t payloadSize16 = (uint16_t)payloadSize;
    uint32_t payloadSize32 = (uint32_t)payloadSize;

    m_packet->hdr->v                     = 2;
    if (m_packMode == RTP_EPM_DEFAULT)
    {
        m_packet->ext->hdrAndPayloadSize = pbsd_hton16(sizeof(RTP_HEADER) + payloadSize16);
    }
    else if (m_packMode == RTP_EPM_TCP2)
    {
        m_packet->hdr->len2              = pbsd_hton16(payloadSize16);
    }
    else
    {
        m_packet->hdr->len4              = pbsd_hton32(payloadSize32);
    }

    if (payloadBuffer != NULL)
    {
        memcpy(m_packet->hdr + 1, payloadBuffer, payloadSize);
    }
}

unsigned long
CRtpPacket::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CRtpPacket::Release()
{
    return CProRefCount::Release();
}

void
CRtpPacket::SetMarker(bool m)
{
    m_packet->hdr->m = m ? 1 : 0;
}

bool
CRtpPacket::GetMarker() const
{
    return m_packet->hdr->m != 0;
}

void
CRtpPacket::SetPayloadType(char pt)
{
    m_packet->hdr->pt = pt;
}

char
CRtpPacket::GetPayloadType() const
{
    return m_packet->hdr->pt;
}

void
CRtpPacket::SetSequence(uint16_t seq)
{
    m_packet->hdr->seq = pbsd_hton16(seq);
}

uint16_t
CRtpPacket::GetSequence() const
{
    return pbsd_ntoh16(m_packet->hdr->seq);
}

void
CRtpPacket::SetTimeStamp(uint32_t ts)
{
    m_packet->hdr->ts = pbsd_hton32(ts);
}

uint32_t
CRtpPacket::GetTimeStamp() const
{
    return pbsd_ntoh32(m_packet->hdr->ts);
}

void
CRtpPacket::SetSsrc(uint32_t ssrc)
{
    if (m_packMode == RTP_EPM_DEFAULT)
    {
        m_packet->hdr->ssrc = pbsd_hton32(ssrc);
    }
    else
    {
        m_ssrc              = ssrc;
    }
}

uint32_t
CRtpPacket::GetSsrc() const
{
    uint32_t ssrc = 0;

    if (m_packMode == RTP_EPM_DEFAULT)
    {
        ssrc = pbsd_ntoh32(m_packet->hdr->ssrc);
    }
    else
    {
        ssrc = m_ssrc;
    }

    return ssrc;
}

void
CRtpPacket::SetMmId(uint32_t mmId)
{
    m_packet->ext->mmId = pbsd_hton32(mmId);
}

uint32_t
CRtpPacket::GetMmId() const
{
    return pbsd_ntoh32(m_packet->ext->mmId);
}

void
CRtpPacket::SetMmType(RTP_MM_TYPE mmType)
{
    m_packet->ext->mmType = mmType;
}

RTP_MM_TYPE
CRtpPacket::GetMmType() const
{
    return m_packet->ext->mmType;
}

void
CRtpPacket::SetKeyFrame(bool keyFrame)
{
    m_packet->ext->keyFrame = keyFrame ? 1 : 0;
}

bool
CRtpPacket::GetKeyFrame() const
{
    return m_packet->ext->keyFrame != 0;
}

void
CRtpPacket::SetFirstPacketOfFrame(bool firstPacket)
{
    m_packet->ext->firstPacketOfFrame = firstPacket ? 1 : 0;
}

bool
CRtpPacket::GetFirstPacketOfFrame() const
{
    return m_packet->ext->firstPacketOfFrame != 0;
}

const void*
CRtpPacket::GetPayloadBuffer() const
{
    return m_packet->hdr + 1;
}

void*
CRtpPacket::GetPayloadBuffer()
{
    return m_packet->hdr + 1;
}

size_t
CRtpPacket::GetPayloadSize() const
{
    size_t size = 0;

    if (m_packMode == RTP_EPM_DEFAULT)
    {
        size =  pbsd_ntoh16(m_packet->ext->hdrAndPayloadSize);
        size -= sizeof(RTP_HEADER);
    }
    else if (m_packMode == RTP_EPM_TCP2)
    {
        size =  pbsd_ntoh16(m_packet->hdr->len2);
    }
    else
    {
        size =  pbsd_ntoh32(m_packet->hdr->len4);
    }

    return size;
}

uint16_t
CRtpPacket::GetPayloadSize16() const
{
    size_t size = 0;

    if (m_packMode == RTP_EPM_DEFAULT)
    {
        size =  pbsd_ntoh16(m_packet->ext->hdrAndPayloadSize);
        size -= sizeof(RTP_HEADER);
    }
    else if (m_packMode == RTP_EPM_TCP2)
    {
        size =  pbsd_ntoh16(m_packet->hdr->len2);
    }
    else
    {
        size =  pbsd_ntoh32(m_packet->hdr->len4);
        size =  size <= TCP2_PAYLOAD_SIZE ? size : 0;
    }

    return (uint16_t)size;
}

void
CRtpPacket::SetUdpxSync(bool sync)
{
    m_packet->ext->udpxSync = sync ? 1 : 0;
}

bool
CRtpPacket::GetUdpxSync() const
{
    return m_packet->ext->udpxSync != 0;
}
