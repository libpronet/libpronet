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
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_LOSS_COUNT 15000

/////////////////////////////////////////////////////////////////////////////
////

CRtpReorder::CRtpReorder()
{
    m_gatePacketCount    = 5;
    m_maxWaitingDuration = 1;
    m_maxBrokenDuration  = 10;
    m_minSeq64           = -1;
    m_lastValidTick      = 0;
}

CRtpReorder::~CRtpReorder()
{
    Reset();
}

void
PRO_CALLTYPE
CRtpReorder::SetGatePacketCount(unsigned char gatePacketCount) /* = 5 */
{
    assert(gatePacketCount > 0);
    if (gatePacketCount == 0)
    {
        return;
    }

    m_gatePacketCount = gatePacketCount;
}

void
PRO_CALLTYPE
CRtpReorder::SetMaxWaitingDuration(unsigned char maxWaitingDurationInSeconds) /* = 1 */
{
    assert(maxWaitingDurationInSeconds > 0);
    if (maxWaitingDurationInSeconds == 0)
    {
        return;
    }

    m_maxWaitingDuration = maxWaitingDurationInSeconds;
}

void
PRO_CALLTYPE
CRtpReorder::SetMaxBrokenDuration(unsigned char maxBrokenDurationInSeconds) /* = 10 */
{
    assert(maxBrokenDurationInSeconds > 0);
    if (maxBrokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = maxBrokenDurationInSeconds;
}

void
PRO_CALLTYPE
CRtpReorder::PushBack(IRtpPacket* packet)
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

        packet->SetMagic(tick);
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

        packet->SetMagic(tick);
        packet->AddRef();
        m_seq64ToPacket[seq16] = packet;

        m_minSeq64      = seq16; /* set value */
        m_lastValidTick = tick;
    }
    else if (seq64 >= m_minSeq64) /* >=!!! */
    {
        if (m_seq64ToPacket.find(seq64) == m_seq64ToPacket.end())
        {
            packet->SetMagic(tick);
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
PRO_CALLTYPE
CRtpReorder::PopFront()
{
    CProStlMap<PRO_INT64, IRtpPacket*>::iterator const itr =
        m_seq64ToPacket.begin();
    if (itr == m_seq64ToPacket.end())
    {
        return (NULL);
    }

    const PRO_INT64 tick = ProGetTickCount64();

    const PRO_INT64   seq64  = itr->first;
    IRtpPacket* const packet = itr->second;

    if (seq64 == m_minSeq64                         ||
        m_seq64ToPacket.size() >= m_gatePacketCount ||
        tick - packet->GetMagic() >= m_maxWaitingDuration * 1000)
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
PRO_CALLTYPE
CRtpReorder::Reset()
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
