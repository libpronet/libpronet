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

#if !defined(____PRO_STAT_H___)
#define ____PRO_STAT_H___

#include "pro_a.h"
#include "pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProStatBitRate
{
public:

    CProStatBitRate();

    void SetTimeSpan(unsigned long timeSpanInSeconds); /* = 5 */

    void PushData(unsigned long dataBytes);

    double CalcBitRate();

    void Reset();

private:

    void Update();

private:

    PRO_INT64 m_timeSpan;
    PRO_INT64 m_startTick;
    double    m_bits;
    double    m_bitRate;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CProStatLossRate
{
public:

    CProStatLossRate();

    void SetTimeSpan(unsigned long timeSpanInSeconds);                   /* = 5 */

    void SetMaxBrokenDuration(unsigned char maxBrokenDurationInSeconds); /* = 10 */

    void PushData(PRO_UINT16 dataSeq);

    double CalcLossRate();

    double CalcLossCount();

    void Reset();

private:

    void Update();

private:

    PRO_INT64  m_timeSpan;
    PRO_INT64  m_maxBrokenDuration;
    PRO_INT64  m_startTick;
    PRO_INT64  m_lastValidTick;
    PRO_UINT16 m_nextSeq;
    double     m_count;
    double     m_lossCount;
    double     m_lossCountAll;
    double     m_lossRate;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CProStatAvgValue
{
public:

    CProStatAvgValue();

    void SetTimeSpan(unsigned long timeSpanInSeconds); /* = 5 */

    void PushData(double dataValue);

    double CalcAvgValue();

    void Reset();

private:

    void Update();

private:

    PRO_INT64 m_timeSpan;
    PRO_INT64 m_startTick;
    double    m_count;
    double    m_sum;
    double    m_avgValue;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_STAT_H___ */
