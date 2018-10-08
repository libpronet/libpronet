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

#if !defined(RTP_FLOW_STAT_H)
#define RTP_FLOW_STAT_H

#include "../pro_util/pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpFlowStat
{
public:

    CRtpFlowStat();

    void SetTimeSpan(unsigned long timeSpanInSeconds); /* = 1 */

    void PushData(
        unsigned long frames,
        unsigned long bytes
        );

    void PopData(
        unsigned long frames,
        unsigned long bytes
        );

    void CalcInfo(
        float* inFrameRate,  /* = NULL */
        float* inBitRate,    /* = NULL */
        float* outFrameRate, /* = NULL */
        float* outBitRate    /* = NULL */
        );

    void Reset();

private:

    void Update();

private:

    PRO_INT64 m_timeSpan;
    PRO_INT64 m_startTick;
    float     m_inFrames;
    float     m_inFrameRate;
    float     m_inBits;
    float     m_inBitRate;
    float     m_outFrames;
    float     m_outFrameRate;
    float     m_outBits;
    float     m_outBitRate;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_FLOW_STAT_H */
