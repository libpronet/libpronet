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

/////////////////////////////////////////////////////////////////////////////
////

CRtpFlowStat::CRtpFlowStat()
{
    m_timeSpan     = 1;
    m_startTick    = 0;
    m_srcFrames    = 0;
    m_srcFrameRate = 0;
    m_srcBits      = 0;
    m_srcBitRate   = 0;
    m_outFrames    = 0;
    m_outFrameRate = 0;
    m_outBits      = 0;
    m_outBitRate   = 0;
}

void
CRtpFlowStat::SetTimeSpan(size_t timeSpanInSeconds) /* = 1 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CRtpFlowStat::PushData(size_t frames,
                       size_t bytes)
{
    if (m_startTick == 0)
    {
        m_startTick = ProGetTickCount64();
    }

    m_srcFrames += frames;
    m_srcBits   += (double)bytes * 8;

    Update();
}

void
CRtpFlowStat::PopData(size_t frames,
                      size_t bytes)
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
CRtpFlowStat::CalcInfo(float* srcFrameRate, /* = NULL */
                       float* srcBitRate,   /* = NULL */
                       float* outFrameRate, /* = NULL */
                       float* outBitRate)   /* = NULL */
{
    Update();

    if (srcFrameRate != NULL)
    {
        *srcFrameRate = (float)m_srcFrameRate;
    }
    if (srcBitRate != NULL)
    {
        *srcBitRate   = (float)m_srcBitRate;
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

    const int64_t tick = ProGetTickCount64();

    if (tick - m_startTick >= m_timeSpan * 1000)
    {
        m_srcFrameRate = m_srcFrames * 1000 / (tick - m_startTick);
        m_srcBitRate   = m_srcBits   * 1000 / (tick - m_startTick);
        m_outFrameRate = m_outFrames * 1000 / (tick - m_startTick);
        m_outBitRate   = m_outBits   * 1000 / (tick - m_startTick);

        m_startTick = tick;
        m_srcFrames = 0;
        m_srcBits   = 0;
        m_outFrames = 0;
        m_outBits   = 0;
    }
}

void
CRtpFlowStat::Reset()
{
    m_startTick    = 0;
    m_srcFrames    = 0;
    m_srcFrameRate = 0;
    m_srcBits      = 0;
    m_srcBitRate   = 0;
    m_outFrames    = 0;
    m_outFrameRate = 0;
    m_outBits      = 0;
    m_outBitRate   = 0;
}
