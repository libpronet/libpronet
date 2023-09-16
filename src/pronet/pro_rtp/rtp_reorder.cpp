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

#include "rtp_reorder.h"
#include "rtp_base.h"
#include "rtp_packet.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_LOSS_COUNT 15000

/////////////////////////////////////////////////////////////////////////////
////

CRtpReorder::CRtpReorder()
{
    m_heightInPackets   = 100;
    m_heightInMs        = 500;
    m_maxBrokenDuration = 10;

    Reset();
}

CRtpReorder::~CRtpReorder()
{
    Reset();
}

void
CRtpReorder::Reset()
{
    m_minSeq64      = -1;
    m_lastValidTick = 0;

    Clean();
}

void
CRtpReorder::Clean()
{
    CProStlMap<int64_t, IRtpPacket*>::iterator       itr = m_seq64ToPacket.begin();
    CProStlMap<int64_t, IRtpPacket*>::iterator const end = m_seq64ToPacket.end();

    for (; itr != end; ++itr)
    {
        itr->second->Release();
    }

    m_seq64ToPacket.clear();
}

void
CRtpReorder::SetWallHeightInPackets(size_t heightInPackets) /* = 100 */
{
    assert(heightInPackets > 0);
    if (heightInPackets == 0)
    {
        return;
    }

    m_heightInPackets = heightInPackets;
}

void
CRtpReorder::SetWallHeightInMilliseconds(size_t heightInMs) /* = 500 */
{
    assert(heightInMs > 0);
    if (heightInMs == 0)
    {
        return;
    }

    m_heightInMs = heightInMs;
}

void
CRtpReorder::SetMaxBrokenDuration(size_t brokenDurationInSeconds) /* = 10 */
{
    assert(brokenDurationInSeconds > 0);
    if (brokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = brokenDurationInSeconds;
}

size_t
CRtpReorder::GetTotalPackets() const
{
    return m_seq64ToPacket.size();
}

void
CRtpReorder::PushBackAddRef(IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return;
    }

    const int64_t tick = ProGetTickCount64();

    const uint16_t seq16 = packet->GetSequence();
    if (m_minSeq64 < 0)
    {
        m_minSeq64      = seq16;
        m_lastValidTick = tick;
    }

    if (tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        Clean();

        ((CRtpPacket*)packet)->SetMagic2(tick);
        packet->AddRef();
        m_seq64ToPacket[seq16] = packet;

        m_minSeq64      = seq16; /* update */
        m_lastValidTick = tick;

        return;
    }

    int64_t seq64 = -1;

    if (seq16 == (uint16_t)m_minSeq64)
    {
        seq64 = m_minSeq64;
    }
    else if (seq16 < (uint16_t)m_minSeq64)
    {
        const uint16_t dist1 = (uint16_t)-1 - (uint16_t)m_minSeq64 + seq16 + 1;
        const uint16_t dist2 = (uint16_t)m_minSeq64 - seq16;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* go forward */
        {
            seq64 =   m_minSeq64 >> 16;
            ++seq64;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* go back */
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
        const uint16_t dist1 = seq16 - (uint16_t)m_minSeq64;
        const uint16_t dist2 = (uint16_t)-1 - seq16 + (uint16_t)m_minSeq64 + 1;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* go forward */
        {
            seq64 =   m_minSeq64 >> 16;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* go back */
        {
            seq64 =   m_minSeq64 >> 16;
            --seq64;
            seq64 <<= 16;
            seq64 |=  seq16;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }

    if (seq64 < 0)
    {
        Clean();

        ((CRtpPacket*)packet)->SetMagic2(tick);
        packet->AddRef();
        m_seq64ToPacket[seq16] = packet;

        m_minSeq64      = seq16; /* update */
        m_lastValidTick = tick;
    }
    else if (seq64 >= m_minSeq64) /* >=!!! */
    {
        if (m_seq64ToPacket.find(seq64) == m_seq64ToPacket.end())
        {
            ((CRtpPacket*)packet)->SetMagic2(tick);
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
CRtpReorder::PopFront(bool force)
{
    const int64_t tick = ProGetTickCount64();

    if (tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        Clean();

        return NULL;
    }

    CProStlMap<int64_t, IRtpPacket*>::iterator const itr = m_seq64ToPacket.begin();
    if (itr == m_seq64ToPacket.end())
    {
        return NULL;
    }

    const int64_t     seq64  = itr->first;
    CRtpPacket* const packet = (CRtpPacket*)itr->second;

    if (seq64 == m_minSeq64                        ||
        m_seq64ToPacket.size() > m_heightInPackets ||
        tick - packet->GetMagic2() > m_heightInMs  ||
        force)
    {
        m_seq64ToPacket.erase(itr);
        m_minSeq64 = seq64 + 1; /* update */

        return packet;
    }
    else
    {
        return NULL;
    }
}
