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

#if !defined(RTP_FLOW_STAT_H)
#define RTP_FLOW_STAT_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpFlowStat
{
public:

    CRtpFlowStat();

    void SetTimeSpan(size_t timeSpanInSeconds); /* = 1 */

    void PushData(
        size_t frames,
        size_t bytes
        );

    void PopData(
        size_t frames,
        size_t bytes
        );

    void CalcInfo(
        float* srcFrameRate, /* = NULL */
        float* srcBitRate,   /* = NULL */
        float* outFrameRate, /* = NULL */
        float* outBitRate    /* = NULL */
        );

    void Reset();

private:

    void Update();

private:

    int64_t m_timeSpan;
    int64_t m_startTick;
    double  m_srcFrames;
    double  m_srcFrameRate;
    double  m_srcBits;
    double  m_srcBitRate;
    double  m_outFrames;
    double  m_outFrameRate;
    double  m_outBits;
    double  m_outBitRate;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_FLOW_STAT_H */
