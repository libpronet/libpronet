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

#ifndef ____PRO_STAT_H____
#define ____PRO_STAT_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

struct PRO_REORDER_BLOCK;

/////////////////////////////////////////////////////////////////////////////
////

class CProStatBitRate
{
public:

    CProStatBitRate();

    void SetTimeSpan(unsigned int timeSpanInSeconds); /* = 5 */

    void PushDataBytes(size_t dataBytes);

    void PushDataBits(size_t dataBits);

    double CalcBitRate();

    void Reset();

private:

    void Update(int64_t tick);

private:

    int64_t m_timeSpan;
    int64_t m_startTick;
    double  m_bits;
    double  m_bitRate;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProStatLossRate
{
public:

    CProStatLossRate();

    ~CProStatLossRate();

    void SetTimeSpan(unsigned int timeSpanInSeconds); /* = 5 */

    void SetMaxBrokenDuration(unsigned int brokenDurationInSeconds); /* = 5 */

    void PushData(uint16_t dataSeq);

    double CalcLossRate();

    double CalcLossCount();

    void Reset();

private:

    void Push(int64_t seq64);

    void Update(int64_t tick);

private:

    int64_t            m_timeSpan;
    int64_t            m_maxBrokenDuration;
    int64_t            m_startTick;
    int64_t            m_calcTick;
    int64_t            m_lastValidTick;
    int64_t            m_nextSeq64;
    double             m_count;
    double             m_lossCount;
    double             m_lossCountAll;
    double             m_lossRate;
    PRO_REORDER_BLOCK* m_reorder;

    int64_t            m_prevSeq64; /* for debugging */
    uint16_t           m_prevSeq16; /* for debugging */

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProStatAvgValue
{
public:

    CProStatAvgValue();

    void SetTimeSpan(unsigned int timeSpanInSeconds); /* = 5 */

    void PushData(double dataValue);

    double CalcAvgValue();

    void Reset();

private:

    void Update(int64_t tick);

private:

    int64_t m_timeSpan;
    int64_t m_startTick;
    double  m_count;
    double  m_sum;
    double  m_avgValue;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

int64_t
ProSeq16ToSeq64(uint16_t inputSeq,
                int64_t  referenceSeq64);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_STAT_H____ */
