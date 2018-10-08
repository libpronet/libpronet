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
#include "pro_shaper.h"
#include "pro_memory_pool.h"
#include "pro_time_util.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CProShaper::CProShaper()
{
    m_maxBitRate = 0;
    m_greenBits  = 0;
    m_startTick  = 0;
}

void
CProShaper::SetMaxBitRate(double maxBitRate)
{
    assert(maxBitRate >= 0);
    if (maxBitRate < 0)
    {
        return;
    }

    m_maxBitRate = maxBitRate;
}

double
CProShaper::GetMaxBitRate() const
{
    return (m_maxBitRate);
}

double
CProShaper::CalcGreenBits()
{
    const PRO_INT64 tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        m_greenBits = m_maxBitRate / 1000; /* 1ms */
        m_startTick = tick;

        return (m_greenBits);
    }

    m_greenBits += m_maxBitRate * (PRO_UINT32)(tick - m_startTick) / 1000;

    if (m_greenBits < -m_maxBitRate)
    {
        m_greenBits = -m_maxBitRate; /* -1s */
    }
    else if (m_greenBits > m_maxBitRate)
    {
        m_greenBits = m_maxBitRate;  /* +1s */
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

void
CProShaper::Reset()
{
    m_greenBits = 0;
    m_startTick = 0;
}
