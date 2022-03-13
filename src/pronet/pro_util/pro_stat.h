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

#if !defined(____PRO_STAT_H____)
#define ____PRO_STAT_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProStatBitRate
{
public:

    CProStatBitRate();

    void SetTimeSpan(unsigned long timeSpanInSeconds); /* = 5 */

    void PushDataBytes(size_t dataBytes);

    void PushDataBits(size_t dataBits);

    double CalcBitRate();

    void Reset();

private:

    void Update(PRO_INT64 tick);

private:

    PRO_INT64 m_timeSpan;
    PRO_INT64 m_startTick;
    double    m_bits;
    double    m_bitRate;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

struct PRO_REORDER_BLOCK
{
    PRO_REORDER_BLOCK()
    {
        Reset();
    }

    void Reset()
    {
        baseSeq = -1;
        itrSeq  = -1;
        bits[0] = 0;
        bits[1] = 0;
    }

    int Push(PRO_INT64 seq)
    {
        if (seq < 0)
        {
            return (-1);
        }

        if (baseSeq < 0)
        {
            baseSeq =  seq;
            itrSeq  =  seq;
            bits[0] |= 1;
            bits[1] =  0;

            return (0);
        }

        if (seq < itrSeq)
        {
            return (-1);
        }

        if (seq > baseSeq + 127)
        {
            return (1);
        }

        const int i = (int)(seq - baseSeq) / 64;
        const int j = (int)(seq - baseSeq) % 64;

        bits[i] |= (PRO_UINT64)1 << j;

        return (0);
    }

    void Shift()
    {
        if (baseSeq < 0)
        {
            return;
        }

        baseSeq += 64;
        itrSeq  =  baseSeq;
        bits[0] =  bits[1];
        bits[1] =  0;
    }

    void Read(
        CProStlVector<PRO_INT64>& seqs,
        bool                      append = false
        )
    {
        if (!append)
        {
            seqs.clear();
        }

        if (itrSeq < 0)
        {
            return;
        }

        for (int i = (int)(itrSeq - baseSeq); i < 64; ++i)
        {
            if ((bits[0] & ((PRO_UINT64)1 << i)) == 0)
            {
                break;
            }

            seqs.push_back(itrSeq);
            ++itrSeq;
        }
    }

    void ReadAll(
        CProStlVector<PRO_INT64>& seqs,
        bool                      append = false
        )
    {
        if (!append)
        {
            seqs.clear();
        }

        if (itrSeq < 0)
        {
            return;
        }

        for (int i = (int)(itrSeq - baseSeq); i < 64; ++i)
        {
            if ((bits[0] & ((PRO_UINT64)1 << i)) != 0)
            {
                seqs.push_back(itrSeq);
            }

            ++itrSeq;
        }
    }

    PRO_INT64  baseSeq;
    PRO_INT64  itrSeq;
    PRO_UINT64 bits[2];

    DECLARE_SGI_POOL(0)
};

class CProStatLossRate
{
public:

    CProStatLossRate();

    void SetTimeSpan(unsigned long timeSpanInSeconds);                /* = 5 */

    void SetMaxBrokenDuration(unsigned long brokenDurationInSeconds); /* = 10 */

    void PushData(PRO_UINT16 dataSeq);

    double CalcLossRate();

    double CalcLossCount();

    void Reset();

private:

    void Push(PRO_INT64 seq64);

    void Update(PRO_INT64 tick);

private:

    PRO_INT64         m_timeSpan;
    PRO_INT64         m_maxBrokenDuration;
    PRO_INT64         m_startTick;
    PRO_INT64         m_calcTick;
    PRO_INT64         m_lastValidTick;
    PRO_INT64         m_nextSeq64;
    double            m_count;
    double            m_lossCount;
    double            m_lossCountAll;
    double            m_lossRate;

    PRO_REORDER_BLOCK m_reorder;

    DECLARE_SGI_POOL(0)
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

    void Update(PRO_INT64 tick);

private:

    PRO_INT64 m_timeSpan;
    PRO_INT64 m_startTick;
    double    m_count;
    double    m_sum;
    double    m_avgValue;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_STAT_H____ */
