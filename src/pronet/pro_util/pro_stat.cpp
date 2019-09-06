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
#include "pro_time_util.h"
#include "pro_z.h"
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
    m_timeSpan  = 5;
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
CProStatBitRate::PushDataBytes(size_t dataBytes)
{
    if (m_startTick == 0)
    {
        m_startTick = ProGetTickCount64() - INIT_TIME_SPAN_MS;
    }

    m_bits += (double)dataBytes * 8;

    Update();
}

void
CProStatBitRate::PushDataBits(size_t dataBits)
{
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

void
CProStatBitRate::Reset()
{
    m_startTick = 0;
    m_calcTick  = 0;
    m_bits      = 0;
    m_bitRate   = 0;
}

/////////////////////////////////////////////////////////////////////////////
////

CProStatLossRate::CProStatLossRate()
{
    m_timeSpan          = 5;
    m_maxBrokenDuration = 10;
    m_startTick         = 0;
    m_calcTick          = 0;
    m_lastValidTick     = 0;
    m_nextSeq           = 0;
    m_count             = 0;
    m_lossCount         = 0;
    m_lossCountAll      = 0;
    m_lossRate          = 0;
}

void
CProStatLossRate::SetTimeSpan(unsigned long timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatLossRate::SetMaxBrokenDuration(unsigned char maxBrokenDurationInSeconds) /* = 10 */
{
    assert(maxBrokenDurationInSeconds > 0);
    if (maxBrokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = maxBrokenDurationInSeconds;
}

void
CProStatLossRate::PushData(PRO_UINT16 dataSeq)
{
    const PRO_INT64 tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        m_startTick     = tick - INIT_TIME_SPAN_MS;
        m_lastValidTick = tick;
        m_nextSeq       = dataSeq;
    }

    if (tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        m_nextSeq = dataSeq + 1;
        ++m_count;
        ++m_lossCount;
        ++m_lossCountAll;

        m_lastValidTick = tick;

        Update();

        return;
    }

    if (dataSeq == m_nextSeq)
    {
        ++m_nextSeq;
        ++m_count;

        m_lastValidTick = tick;
    }
    else
    {
        PRO_UINT16 dist1 = 0;
        PRO_UINT16 dist2 = 0;

        if (dataSeq < m_nextSeq)
        {
            dist1 = (PRO_UINT16)-1 - m_nextSeq + dataSeq + 1;
            dist2 = m_nextSeq - dataSeq;
        }
        else
        {
            dist1 = dataSeq - m_nextSeq;
            dist2 = (PRO_UINT16)-1 - dataSeq + m_nextSeq + 1;
        }

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* forward */
        {
            m_nextSeq      =  dataSeq + 1;
            m_count        += dist1 + 1;
            m_lossCount    += dist1;
            m_lossCountAll += dist1;

            m_lastValidTick = tick;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* back */
        {
        }
        else                                              /* reset */
        {
            m_nextSeq = dataSeq + 1;
            ++m_count;
            ++m_lossCount;
            ++m_lossCountAll;

            m_lastValidTick = tick;
        }
    }

    Update();
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

void
CProStatLossRate::Reset()
{
    m_startTick     = 0;
    m_calcTick      = 0;
    m_lastValidTick = 0;
    m_nextSeq       = 0;
    m_count         = 0;
    m_lossCount     = 0;
    m_lossCountAll  = 0;
    m_lossRate      = 0;
}

/////////////////////////////////////////////////////////////////////////////
////

CProStatAvgValue::CProStatAvgValue()
{
    m_timeSpan  = 5;
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

void
CProStatAvgValue::Reset()
{
    m_startTick = 0;
    m_calcTick  = 0;
    m_count     = 0;
    m_sum       = 0;
    m_avgValue  = 0;
}
