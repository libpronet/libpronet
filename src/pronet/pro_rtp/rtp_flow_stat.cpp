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

#include "rtp_flow_stat.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CRtpFlowStat::CRtpFlowStat()
{
    m_timeSpan     = 1;
    m_startTick    = 0;
    m_inFrames     = 0;
    m_inFrameRate  = 0;
    m_inBits       = 0;
    m_inBitRate    = 0;
    m_outFrames    = 0;
    m_outFrameRate = 0;
    m_outBits      = 0;
    m_outBitRate   = 0;
}

void
CRtpFlowStat::SetTimeSpan(unsigned long timeSpanInSeconds) /* = 1 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CRtpFlowStat::PushData(unsigned long frames,
                       unsigned long bytes)
{
    if (m_startTick == 0)
    {
        m_startTick = ProGetTickCount64();
    }

    m_inFrames += frames;
    m_inBits   += (double)bytes * 8;

    Update();
}

void
CRtpFlowStat::PopData(unsigned long frames,
                      unsigned long bytes)
{
    if (m_startTick == 0)
    {
        return;
    }

    m_outFrames += frames;
    m_outBits   += (double)bytes * 8;

    Update();
}

void
CRtpFlowStat::CalcInfo(float* inFrameRate,  /* = NULL */
                       float* inBitRate,    /* = NULL */
                       float* outFrameRate, /* = NULL */
                       float* outBitRate)   /* = NULL */
{
    Update();

    if (inFrameRate != NULL)
    {
        *inFrameRate  = (float)m_inFrameRate;
    }
    if (inBitRate != NULL)
    {
        *inBitRate    = (float)m_inBitRate;
    }
    if (outFrameRate != NULL)
    {
        *outFrameRate = (float)m_outFrameRate;
    }
    if (outBitRate != NULL)
    {
        *outBitRate   = (float)m_outBitRate;
    }
}

void
CRtpFlowStat::Update()
{
    if (m_startTick == 0)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();

    if (tick - m_startTick >= m_timeSpan * 1000)
    {
        m_inFrameRate  = m_inFrames  * 1000 / (tick - m_startTick);
        m_inBitRate    = m_inBits    * 1000 / (tick - m_startTick);
        m_outFrameRate = m_outFrames * 1000 / (tick - m_startTick);
        m_outBitRate   = m_outBits   * 1000 / (tick - m_startTick);

        m_startTick = tick;
        m_inFrames  = 0;
        m_inBits    = 0;
        m_outFrames = 0;
        m_outBits   = 0;
    }
}

void
CRtpFlowStat::Reset()
{
    m_startTick    = 0;
    m_inFrames     = 0;
    m_inFrameRate  = 0;
    m_inBits       = 0;
    m_inBitRate    = 0;
    m_outFrames    = 0;
    m_outFrameRate = 0;
    m_outBits      = 0;
    m_outBitRate   = 0;
}
