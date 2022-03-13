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
#include "pro_shaper.h"
#include "pro_memory_pool.h"
#include "pro_time_util.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CProShaper::CProShaper()
{
    m_maxBitRate  = 0;
    m_minTimeSpan = 1;
    m_maxTimeSpan = 1000;

    Reset();
}

void
CProShaper::Reset()
{
    m_greenBits = 0;
    m_startTick = 0;
}

void
CProShaper::SetMaxBitRate(double bitRate)
{
    assert(bitRate >= 0);
    if (bitRate < 0)
    {
        return;
    }

    m_maxBitRate = bitRate;
}

double
CProShaper::GetMaxBitRate() const
{
    return (m_maxBitRate);
}

void
CProShaper::SetMinTimeSpan(unsigned long timeSpanInMs) /* = 1 */
{
    assert(timeSpanInMs > 0);
    if (timeSpanInMs == 0)
    {
        return;
    }

    m_minTimeSpan = timeSpanInMs;
}

unsigned long
CProShaper::GetMinTimeSpan() const
{
    return ((unsigned long)m_minTimeSpan);
}

void
CProShaper::SetMaxTimeSpan(unsigned long timeSpanInMs) /* = 1000 */
{
    assert(timeSpanInMs > 0);
    if (timeSpanInMs == 0)
    {
        return;
    }

    m_maxTimeSpan = timeSpanInMs;
}

unsigned long
CProShaper::GetMaxTimeSpan() const
{
    return ((unsigned long)m_maxTimeSpan);
}

double
CProShaper::CalcGreenBits()
{
    const PRO_INT64 tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        PRO_INT64 minTimeSpan = m_minTimeSpan;
        if (minTimeSpan > m_maxTimeSpan)
        {
            minTimeSpan = m_maxTimeSpan;
        }

        m_greenBits = m_maxBitRate * minTimeSpan / 1000; /* min(ms) */
        m_startTick = tick;

        return (m_greenBits);
    }

    m_greenBits += m_maxBitRate * (tick - m_startTick) / 1000;

    const double maxBits = m_maxBitRate * m_maxTimeSpan / 1000;
    if (m_greenBits < -maxBits)
    {
        m_greenBits = -maxBits; /* -max(ms) */
    }
    else if (m_greenBits > maxBits)
    {
        m_greenBits = maxBits;  /* +max(ms) */
    }
    else
    {
    }

    m_startTick = tick;

    return (m_greenBits > 0 ? m_greenBits : 0);
}

void
CProShaper::FlushGreenBits(double greenBits)
{
    assert(greenBits >= 0);
    if (greenBits < 0)
    {
        return;
    }

    m_greenBits -= greenBits;
}
