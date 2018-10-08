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

#include "pro_a.h"
#include "pro_reorder.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_time_util.h"
#include "pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_LOSS_COUNT 15000

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(____RTP_FRAMEWORK_H____)
#define RTP_MM_TYPE unsigned char
#endif

/*
 * rtp packet
 *
 * please refer to "rtp_framework/rtp_framework.h"
 */
#if !defined(____IRtpPacket____)
#define ____IRtpPacket____
class IRtpPacket
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE SetMarker(bool m) = 0;

    virtual bool PRO_CALLTYPE GetMarker() const = 0;

    virtual void PRO_CALLTYPE SetPayloadType(char pt) = 0;

    virtual char PRO_CALLTYPE GetPayloadType() const = 0;

    virtual void PRO_CALLTYPE SetSequence(PRO_UINT16 seq) = 0;

    virtual PRO_UINT16 PRO_CALLTYPE GetSequence() const = 0;

    virtual void PRO_CALLTYPE SetTimeStamp(PRO_UINT32 ts) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetTimeStamp() const = 0;

    virtual void PRO_CALLTYPE SetSsrc(PRO_UINT32 ssrc) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetSsrc() const = 0;

    virtual void PRO_CALLTYPE SetMmId(PRO_UINT32 mmId) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetMmId() const = 0;

    virtual void PRO_CALLTYPE SetMmType(RTP_MM_TYPE mmType) = 0;

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const = 0;

    virtual void PRO_CALLTYPE SetKeyFrame(bool keyFrame) = 0;

    virtual bool PRO_CALLTYPE GetKeyFrame() const = 0;

    virtual void PRO_CALLTYPE SetFirstPacketOfFrame(bool firstPacket) = 0;

    virtual bool PRO_CALLTYPE GetFirstPacketOfFrame() const = 0;

    virtual const void* PRO_CALLTYPE GetPayloadBuffer() const = 0;

    virtual void* PRO_CALLTYPE GetPayloadBuffer() = 0;

    virtual PRO_UINT16 PRO_CALLTYPE GetPayloadSize() const = 0;

    virtual void PRO_CALLTYPE SetTick_i(PRO_INT64 tick) = 0;

    virtual PRO_INT64 PRO_CALLTYPE GetTick_i() const = 0;
};
#endif /* ____IRtpPacket____ */

#if !defined(____RTP_FRAMEWORK_H____)
#undef RTP_MM_TYPE
#endif

/////////////////////////////////////////////////////////////////////////////
////

CProReorder::CProReorder()
{
    m_maxPacketCount     = 5;
    m_maxWaitingDuration = 1;
    m_maxBrokenDuration  = 10;
    m_minSeq64           = -1;
    m_lastValidTick      = 0;
}

CProReorder::~CProReorder()
{
    Reset();
}

void
CProReorder::SetMaxPacketCount(unsigned char maxPacketCount) /* = 5 */
{
    assert(maxPacketCount > 0);
    if (maxPacketCount == 0)
    {
        return;
    }

    m_maxPacketCount = maxPacketCount;
}

void
CProReorder::SetMaxWaitingDuration(unsigned char maxWaitingDurationInSeconds) /* = 1 */
{
    assert(maxWaitingDurationInSeconds > 0);
    if (maxWaitingDurationInSeconds == 0)
    {
        return;
    }

    m_maxWaitingDuration = maxWaitingDurationInSeconds;
}

void
CProReorder::SetMaxBrokenDuration(unsigned char maxBrokenDurationInSeconds) /* = 10 */
{
    assert(maxBrokenDurationInSeconds > 0);
    if (maxBrokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = maxBrokenDurationInSeconds;
}

void
CProReorder::PushBack(IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();

    const PRO_UINT16 seq16 = packet->GetSequence();
    if (m_minSeq64 == -1)
    {
        m_minSeq64      = seq16;
        m_lastValidTick = tick;
    }

    if (tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator       itr = m_seq64ToPacket.begin();
        CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator const end = m_seq64ToPacket.end();

        for (; itr != end; ++itr)
        {
            IRtpPacket* const packet2 = itr->second;
            packet2->Release();
        }

        m_seq64ToPacket.clear();

        packet->SetTick_i(tick);
        packet->AddRef();
        m_seq64ToPacket[seq16] = packet;

        m_minSeq64      = seq16; /* set value */
        m_lastValidTick = tick;

        return;
    }

    PRO_INT64 seq64 = -1;

    if (seq16 == (PRO_UINT16)m_minSeq64)
    {
        seq64 = m_minSeq64;
    }
    else if (seq16 < (PRO_UINT16)m_minSeq64)
    {
        const PRO_UINT16 dist1 = (PRO_UINT16)-1 - (PRO_UINT16)m_minSeq64 + seq16 + 1;
        const PRO_UINT16 dist2 = (PRO_UINT16)m_minSeq64 - seq16;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* forward */
        {
            seq64 = m_minSeq64 >> 16;
            ++seq64;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* back */
        {
            seq64 =   m_minSeq64 >> 16;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }
    else
    {
        const PRO_UINT16 dist1 = seq16 - (PRO_UINT16)m_minSeq64;
        const PRO_UINT16 dist2 = (PRO_UINT16)-1 - seq16 + (PRO_UINT16)m_minSeq64 + 1;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* forward */
        {
            seq64 =   m_minSeq64 >> 16;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* back */
        {
            seq64 = m_minSeq64 >> 16;
            --seq64;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }

    if (seq64 == -1)
    {
        CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator       itr = m_seq64ToPacket.begin();
        CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator const end = m_seq64ToPacket.end();

        for (; itr != end; ++itr)
        {
            IRtpPacket* const packet2 = itr->second;
            packet2->Release();
        }

        m_seq64ToPacket.clear();

        packet->SetTick_i(tick);
        packet->AddRef();
        m_seq64ToPacket[seq16] = packet;

        m_minSeq64      = seq16; /* set value */
        m_lastValidTick = tick;
    }
    else if (seq64 >= m_minSeq64) /* >=!!! */
    {
        if (m_seq64ToPacket.find(seq64) == m_seq64ToPacket.end())
        {
            packet->SetTick_i(tick);
            packet->AddRef();
            m_seq64ToPacket[seq64] = packet;

            m_lastValidTick = tick;
        }
    }
    else
    {
    }
}

IRtpPacket*
CProReorder::PopFront()
{
    CProStlMap<PRO_INT64, IRtpPacket*>::iterator const itr = m_seq64ToPacket.begin();
    if (itr == m_seq64ToPacket.end())
    {
        return (NULL);
    }

    const PRO_INT64 tick = ProGetTickCount64();

    const PRO_INT64   seq64  = itr->first;
    IRtpPacket* const packet = itr->second;

    if (seq64 == m_minSeq64                        ||
        m_seq64ToPacket.size() >= m_maxPacketCount ||
        tick - packet->GetTick_i() >= m_maxWaitingDuration * 1000)
    {
        m_seq64ToPacket.erase(itr);
        m_minSeq64 = seq64 + 1; /* set value */

        return (packet);
    }
    else
    {
        return (NULL);
    }
}

void
CProReorder::Reset()
{
    m_minSeq64      = -1;
    m_lastValidTick = 0;

    CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator       itr = m_seq64ToPacket.begin();
    CProStlMap<PRO_INT64, IRtpPacket*>::const_iterator const end = m_seq64ToPacket.end();

    for (; itr != end; ++itr)
    {
        IRtpPacket* const packet = itr->second;
        packet->Release();
    }

    m_seq64ToPacket.clear();
}
