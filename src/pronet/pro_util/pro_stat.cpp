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

#include "pro_a.h"
#include "pro_stat.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_time_util.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MAX_LOSS_COUNT    15000
#define INIT_TIME_SPAN_MS 3000
#define TICK_SPAN_MS      500 /* 100 ~ 500 */

/////////////////////////////////////////////////////////////////////////////
////

CProStatBitRate::CProStatBitRate()
{
    m_timeSpan = 5;

    Reset();
}

void
CProStatBitRate::Reset()
{
    m_startTick = 0;
    m_calcTick  = 0;
    m_bits      = 0;
    m_bitRate   = 0;
}

void
CProStatBitRate::SetTimeSpan(unsigned long timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatBitRate::PushDataBytes(double dataBytes)
{
    PushDataBits(dataBytes * 8);
}

void
CProStatBitRate::PushDataBits(double dataBits)
{
    assert(dataBits >= 0);
    if (dataBits < 0)
    {
        return;
    }

    if (m_startTick == 0)
    {
        m_startTick = ProGetTickCount64() - INIT_TIME_SPAN_MS;
    }

    m_bits += dataBits;

    Update();
}

double
CProStatBitRate::CalcBitRate()
{
    Update();

    return (m_bitRate);
}

void
CProStatBitRate::Update()
{
    if (m_startTick == 0)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();

    if (tick - m_calcTick >= TICK_SPAN_MS)
    {
        m_calcTick = tick;
        m_bitRate  = m_bits * 1000 / (tick - m_startTick);

        if (tick - m_startTick >= m_timeSpan * 1000)
        {
            m_startTick = tick - m_timeSpan * 1000;
            m_bits      = m_bitRate * (tick - m_startTick) / 1000;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
////

CProStatLossRate::CProStatLossRate()
{
    m_timeSpan          = 5;
    m_maxBrokenDuration = 10;

    Reset();
}

void
CProStatLossRate::Reset()
{
    m_startTick     = 0;
    m_calcTick      = 0;
    m_lastValidTick = 0;
    m_nextSeq64     = -1;
    m_count         = 0;
    m_lossCount     = 0;
    m_lossCountAll  = 0;
    m_lossRate      = 0;

    m_reorder.Reset();
}

void
CProStatLossRate::SetTimeSpan(unsigned long timeSpanInSeconds)                /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatLossRate::SetMaxBrokenDuration(unsigned char brokenDurationInSeconds) /* = 10 */
{
    assert(brokenDurationInSeconds > 0);
    if (brokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = brokenDurationInSeconds;
}

void
CProStatLossRate::PushData(PRO_UINT16 dataSeq)
{
    const PRO_INT64 tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        m_startTick     = tick - INIT_TIME_SPAN_MS;
        m_lastValidTick = tick;
        m_nextSeq64     = -1;

        m_reorder.Reset();
        m_reorder.Push(dataSeq);
    }

    if (tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        m_lastValidTick = tick;
        m_nextSeq64     = -1;
        ++m_count;
        ++m_lossCount;
        ++m_lossCountAll;

        m_reorder.Reset();
        m_reorder.Push(dataSeq);

        Update();

        return;
    }

    PRO_INT64 seq64 = -1;

    if (dataSeq == (PRO_UINT16)m_reorder.itrSeq)
    {
        seq64 = m_reorder.itrSeq;
    }
    else if (dataSeq < (PRO_UINT16)m_reorder.itrSeq)
    {
        const PRO_UINT16 dist1 = (PRO_UINT16)-1 - ((PRO_UINT16)m_reorder.itrSeq - dataSeq) + 1;
        const PRO_UINT16 dist2 = (PRO_UINT16)m_reorder.itrSeq - dataSeq;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* forward */
        {
            seq64 =   m_reorder.itrSeq >> 16;
            ++seq64;
            seq64 <<= 16;
            seq64 |=  dataSeq;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* back */
        {
            seq64 =   m_reorder.itrSeq >> 16;
            seq64 <<= 16;
            seq64 |=  dataSeq;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }
    else
    {
        const PRO_UINT16 dist1 = dataSeq - (PRO_UINT16)m_reorder.itrSeq;
        const PRO_UINT16 dist2 = (PRO_UINT16)-1 - (dataSeq - (PRO_UINT16)m_reorder.itrSeq) + 1;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* forward */
        {
            seq64 =   m_reorder.itrSeq >> 16;
            seq64 <<= 16;
            seq64 |=  dataSeq;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* back */
        {
            seq64 =   m_reorder.itrSeq >> 16;
            --seq64;
            seq64 <<= 16;
            seq64 |=  dataSeq;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }

    if (seq64 < 0)
    {
        m_lastValidTick = tick;
        m_nextSeq64     = -1;
        ++m_count;
        ++m_lossCount;
        ++m_lossCountAll;

        m_reorder.Reset();
        m_reorder.Push(dataSeq);

        Update();

        return;
    }

    CProStlVector<PRO_INT64> seqs;

    const int ret = m_reorder.Push(seq64);
    if (ret < 0)
    {
    }
    else if (ret == 0)
    {
        m_lastValidTick = tick;

        m_reorder.Read(seqs);
    }
    else
    {
        m_reorder.ReadAll(seqs);
        m_reorder.Shift();

        if (m_reorder.Push(seq64) > 0)
        {
            m_reorder.ReadAll(seqs, true); /* append is true */
            m_reorder.Reset();
            m_reorder.Push(seq64);
        }
    }

    int       i = 0;
    const int c = (int)seqs.size();

    for (; i < c; ++i)
    {
        Push(seqs[i]);
    }

    Update();
}

void
CProStatLossRate::Push(PRO_INT64 seq64)
{
    assert(seq64 >= 0);
    assert(seq64 >= m_nextSeq64);
    if (seq64 < 0 || seq64 < m_nextSeq64)
    {
        return;
    }

    if (m_nextSeq64 < 0)
    {
        m_nextSeq64 = seq64;
    }

    if (seq64 == m_nextSeq64)
    {
        ++m_nextSeq64;
        ++m_count;
    }
    else
    {
        const PRO_INT64 dist = seq64 - m_nextSeq64;

        m_nextSeq64    =  seq64 + 1;
        m_count        += dist + 1;
        m_lossCount    += dist;
        m_lossCountAll += dist;
    }
}

double
CProStatLossRate::CalcLossRate()
{
    return (m_lossRate);
}

double
CProStatLossRate::CalcLossCount()
{
    return (m_lossCountAll);
}

void
CProStatLossRate::Update()
{
    if (m_startTick == 0)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();

    if (tick - m_calcTick >= TICK_SPAN_MS && m_count >= 1) /* double */
    {
        m_calcTick = tick;
        m_lossRate = m_lossCount / m_count;

        if (tick - m_startTick >= m_timeSpan * 1000)
        {
            const double countRate = m_count * 1000 / (tick - m_startTick);

            m_startTick = tick - m_timeSpan * 1000;
            m_count     = countRate * (tick - m_startTick) / 1000;
            m_lossCount = m_count * m_lossRate;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
////

CProStatAvgValue::CProStatAvgValue()
{
    m_timeSpan = 5;

    Reset();
}

void
CProStatAvgValue::Reset()
{
    m_startTick = 0;
    m_calcTick  = 0;
    m_count     = 0;
    m_sum       = 0;
    m_avgValue  = 0;
}

void
CProStatAvgValue::SetTimeSpan(unsigned long timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatAvgValue::PushData(double dataValue)
{
    if (m_startTick == 0)
    {
        m_startTick = ProGetTickCount64() - INIT_TIME_SPAN_MS;
    }

    ++m_count;
    m_sum += dataValue;

    Update();
}

double
CProStatAvgValue::CalcAvgValue()
{
    return (m_avgValue);
}

void
CProStatAvgValue::Update()
{
    if (m_startTick == 0)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();

    if (tick - m_calcTick >= TICK_SPAN_MS && m_count >= 1) /* double */
    {
        m_calcTick = tick;
        m_avgValue = m_sum / m_count;

        if (tick - m_startTick >= m_timeSpan * 1000)
        {
            const double countRate = m_count * 1000 / (tick - m_startTick);

            m_startTick = tick - m_timeSpan * 1000;
            m_count     = countRate * (tick - m_startTick) / 1000;
            m_sum       = m_count * m_avgValue;
        }
    }
}
