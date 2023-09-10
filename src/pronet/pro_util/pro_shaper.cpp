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
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CProShaper::CProShaper()
{
    m_avgBitRate   = 0;
    m_initTimeSpan = 5;
    m_maxTimeSpan  = 1000;

    Reset();
}

void
CProShaper::Reset()
{
    m_startTick = -1;
}

void
CProShaper::SetAvgBitRate(double bitRate)
{
    assert(bitRate >= 0);
    if (bitRate < 0)
    {
        return;
    }

    m_avgBitRate = bitRate;
}

double
CProShaper::GetAvgBitRate() const
{
    return m_avgBitRate;
}

void
CProShaper::SetInitialTimeSpan(unsigned int timeSpanInMs) /* = 5 */
{
    assert(timeSpanInMs > 0);
    if (timeSpanInMs == 0)
    {
        return;
    }

    m_initTimeSpan = (double)timeSpanInMs;
}

unsigned int
CProShaper::GetInitialTimeSpan() const
{
    return (unsigned int)m_initTimeSpan;
}

void
CProShaper::SetMaxTimeSpan(unsigned int timeSpanInMs) /* = 1000 */
{
    assert(timeSpanInMs > 0);
    if (timeSpanInMs == 0)
    {
        return;
    }

    m_maxTimeSpan = (double)timeSpanInMs;
}

unsigned int
CProShaper::GetMaxTimeSpan() const
{
    return (unsigned int)m_maxTimeSpan;
}

double
CProShaper::CalcGreenBits()
{
    const double tick = (double)ProGetTickCount64();

    if (m_startTick <= 0)
    {
        m_startTick = tick - m_initTimeSpan;
    }
    if (m_startTick < tick - m_maxTimeSpan)
    {
        m_startTick = tick - m_maxTimeSpan;
    }

    const double greenBits = m_avgBitRate * (tick - m_startTick) / 1000;

    return greenBits > 0 ? greenBits : 0;
}

void
CProShaper::FlushGreenBits(double greenBits)
{
    assert(greenBits >= 0);
    if (greenBits < 0)
    {
        return;
    }

    if (m_startTick <= 0 || m_avgBitRate < 0.000001)
    {
        return;
    }

    m_startTick += greenBits * 1000 / m_avgBitRate;
}
