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
#include "pro_stat.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_time_util.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define REORDER_BITMAP_BYTES 40  /* 320 bits */
#define MAX_LOSS_COUNT       15000
#define INIT_SPAN_MS         3000
#define DELTA_SPAN_MS        200 /* 100 ~ 500 */
#define CALC_SPAN_MS         100
#define MAX_POP_INTERVAL_MS  1500

/////////////////////////////////////////////////////////////////////////////
////

CProStatBitRate::CProStatBitRate()
{
    m_timeSpan = 5;

    Reset();
}

void
CProStatBitRate::Reset()
{
    m_startTick = 0;
    m_bits      = 0;
    m_bitRate   = 0;
}

void
CProStatBitRate::SetTimeSpan(unsigned int timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatBitRate::PushDataBytes(size_t dataBytes)
{
    PushDataBits(dataBytes * 8);
}

void
CProStatBitRate::PushDataBits(size_t dataBits)
{
    int64_t tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        int64_t initSpan = m_timeSpan * 1000;
        if (initSpan > INIT_SPAN_MS)
        {
            initSpan = INIT_SPAN_MS;
        }

        m_startTick = tick - initSpan;
    }

    m_bits += dataBits;

    Update(tick);
}

double
CProStatBitRate::CalcBitRate()
{
    Update(ProGetTickCount64());

    return m_bitRate;
}

void
CProStatBitRate::Update(int64_t tick)
{
    if (m_startTick == 0)
    {
        return;
    }

    m_bitRate = m_bits * 1000 / (tick - m_startTick);

    if (tick - m_startTick >= m_timeSpan * 1000 + DELTA_SPAN_MS)
    {
        m_startTick = tick - m_timeSpan * 1000;
        m_bits      = m_bitRate * m_timeSpan;
    }
}

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
        popTick = 0;

        memset(bitmap, 0, sizeof(bitmap));
    }

    int Push(
        int64_t seq,
        int64_t tick
        )
    {
        assert(seq >= 0);
        if (seq < 0)
        {
            return -1;
        }

        /*
         * first packet
         */
        if (baseSeq < 0)
        {
            baseSeq   =  seq;
            itrSeq    =  seq;
            popTick   =  tick;
            bitmap[0] |= 1;

            return 0;
        }

        /*
         * too low
         */
        if (seq < itrSeq)
        {
            return -1;
        }

        /*
         * too high
         */
        if (seq >= baseSeq + REORDER_BITMAP_BYTES * 8)
        {
            return 1;
        }

        int i = (int)(seq - baseSeq) / 8;
        int j = (int)(seq - baseSeq) % 8;

        bitmap[i] |= 1 << j;

        return 0;
    }

    void Read(
        CProStlVector<int64_t>& seqs,
        int64_t                 tick
        )
    {
        if (baseSeq < 0)
        {
            return;
        }

        int64_t endSeq = baseSeq + REORDER_BITMAP_BYTES * 8;

        for (; itrSeq < endSeq; ++itrSeq)
        {
            int i = (int)(itrSeq - baseSeq) / 8;
            int j = (int)(itrSeq - baseSeq) % 8;

            /*
             * there is a gap
             */
            if ((bitmap[i] & (1 << j)) == 0)
            {
                break;
            }

            seqs.push_back(itrSeq);
        }

        int i = 0;
        int c = (int)(itrSeq - baseSeq) / 8;

        for (; i < c; ++i)
        {
            Shift8();
        }

        if (seqs.size() > 0)
        {
            popTick = tick;
        }
    }

    void Read8(
        CProStlVector<int64_t>& seqs,
        int64_t                 tick
        )
    {
        if (baseSeq < 0)
        {
            return;
        }

        int64_t endSeq = baseSeq + 8;

        for (; itrSeq < endSeq; ++itrSeq)
        {
            int i = (int)(itrSeq - baseSeq) / 8;
            int j = (int)(itrSeq - baseSeq) % 8;

            if ((bitmap[i] & (1 << j)) != 0)
            {
                seqs.push_back(itrSeq);
            }
        }

        int i = 0;
        int c = (int)(itrSeq - baseSeq) / 8;

        for (; i < c; ++i)
        {
            Shift8();
        }

        if (seqs.size() > 0)
        {
            popTick = tick;
        }
    }

    bool IsEmpty()
    {
        bool ret = true;

        for (int i = 0; i < REORDER_BITMAP_BYTES; ++i)
        {
            if (bitmap[i] != 0)
            {
                ret = false;
                break;
            }
        }

        return ret;
    }

    int64_t       baseSeq;
    int64_t       itrSeq;
    int64_t       popTick;
    unsigned char bitmap[REORDER_BITMAP_BYTES];

private:

    void Shift8()
    {
        if (baseSeq < 0)
        {
            return;
        }

        baseSeq += 8;
        if (itrSeq < baseSeq)
        {
            itrSeq = baseSeq;
        }

        for (int i = 0; i < REORDER_BITMAP_BYTES - 1; ++i)
        {
            bitmap[i]     =  bitmap[i + 1];
            bitmap[i + 1] =  0;
        }
    }

    DECLARE_SGI_POOL(0)
};

CProStatLossRate::CProStatLossRate()
{
    m_timeSpan          = 5;
    m_maxBrokenDuration = 10;
    m_reorder           = new PRO_REORDER_BLOCK;

    m_prevSeq64         = -1;
    m_prevSeq16         = 0;

    Reset();
}

CProStatLossRate::~CProStatLossRate()
{
    delete m_reorder;
    m_reorder = NULL;
}

void
CProStatLossRate::Reset()
{
    m_startTick     = 0;
    m_calcTick      = 0;
    m_lastValidTick = 0;
    m_nextSeq64     = -1;
    m_count         = 0;
    m_lossCount     = 0;
    m_lossCountAll  = 0;
    m_lossRate      = 0;

    m_reorder->Reset();
}

void
CProStatLossRate::SetTimeSpan(unsigned int timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatLossRate::SetMaxBrokenDuration(unsigned int brokenDurationInSeconds) /* = 5 */
{
    assert(brokenDurationInSeconds > 0);
    if (brokenDurationInSeconds == 0)
    {
        return;
    }

    m_maxBrokenDuration = brokenDurationInSeconds;
}

void
CProStatLossRate::PushData(uint16_t dataSeq)
{
    int64_t tick = ProGetTickCount64();

    /*
     * first packet
     */
    if (m_startTick == 0)
    {
        int64_t initSpan = m_timeSpan * 1000;
        if (initSpan > INIT_SPAN_MS)
        {
            initSpan = INIT_SPAN_MS;
        }

        m_startTick     = tick - initSpan;
        m_lastValidTick = tick;
        m_nextSeq64     = -1;

        m_reorder->Reset();
        m_reorder->Push(dataSeq, tick);
    }

    int64_t seq64 = ProSeq16ToSeq64(dataSeq, m_nextSeq64);

    /*
     * reset
     */
    if (seq64 < 0 || tick - m_lastValidTick > m_maxBrokenDuration * 1000)
    {
        m_lastValidTick = tick;
        m_nextSeq64     = -1;
        ++m_count;
        ++m_lossCount;
        ++m_lossCountAll;

        m_reorder->Reset();
        m_reorder->Push(dataSeq, tick);

        Update(tick);

        m_prevSeq64 = seq64;
        m_prevSeq16 = dataSeq;

        return;
    }

    CProStlVector<int64_t> seqs;

    int ret = m_reorder->Push(seq64, tick);
    if (ret > 0)
    {
        for (int i = 0; i < REORDER_BITMAP_BYTES; ++i)
        {
            m_reorder->Read8(seqs, tick);

            ret = m_reorder->Push(seq64, tick);
            if (ret <= 0)
            {
                break;
            }
        }

        if (ret > 0)
        {
            m_reorder->Reset();
            ret = m_reorder->Push(seq64, tick);
        }
    }

    /*
     * The packet has been queued.
     */
    if (ret == 0)
    {
        m_lastValidTick = tick;
    }

    m_reorder->Read(seqs, tick);

    int i = 0;
    int c = (int)seqs.size();

    for (; i < c; ++i)
    {
        Push(seqs[i]);
    }

    Update(tick);

    m_prevSeq64 = seq64;
    m_prevSeq16 = dataSeq;
}

void
CProStatLossRate::Push(int64_t seq64)
{
    assert(seq64 >= 0);
    assert(seq64 >= m_nextSeq64);
    if (seq64 < 0 || seq64 < m_nextSeq64)
    {
        return;
    }

    if (m_nextSeq64 < 0)
    {
        m_nextSeq64 = seq64;
    }

    if (seq64 == m_nextSeq64)
    {
        ++m_nextSeq64;
        ++m_count;
    }
    else
    {
        int64_t dist = seq64 - m_nextSeq64;

        m_nextSeq64    =  seq64 + 1;
        m_count        += dist + 1;
        m_lossCount    += dist;
        m_lossCountAll += dist;
    }
}

double
CProStatLossRate::CalcLossRate()
{
    if (m_startTick > 0 && !m_reorder->IsEmpty())
    {
        int64_t tick = ProGetTickCount64();
        if (tick - m_reorder->popTick > MAX_POP_INTERVAL_MS)
        {
            CProStlVector<int64_t> seqs;

            for (int i = 0; i < REORDER_BITMAP_BYTES; ++i)
            {
                m_reorder->Read8(seqs, tick);
            }

            m_reorder->Reset();

            int i = 0;
            int c = (int)seqs.size();

            for (; i < c; ++i)
            {
                Push(seqs[i]);
            }

            Update(tick);
        }
    }

    return m_lossRate;
}

double
CProStatLossRate::CalcLossCount()
{
    return m_lossCountAll;
}

void
CProStatLossRate::Update(int64_t tick)
{
    if (m_startTick == 0)
    {
        return;
    }

    if (tick - m_calcTick >= CALC_SPAN_MS && m_count >= 1) /* double */
    {
        m_calcTick = tick;
        m_lossRate = m_lossCount / m_count;
    }

    if (tick - m_startTick >= m_timeSpan * 1000 + DELTA_SPAN_MS)
    {
        double countRate = m_count * 1000 / (tick - m_startTick);

        m_startTick = tick - m_timeSpan * 1000;
        m_count     = countRate * m_timeSpan;
        m_lossCount = m_count * m_lossRate;
    }
}

/////////////////////////////////////////////////////////////////////////////
////

CProStatAvgValue::CProStatAvgValue()
{
    m_timeSpan = 5;

    Reset();
}

void
CProStatAvgValue::Reset()
{
    m_startTick = 0;
    m_count     = 0;
    m_sum       = 0;
    m_avgValue  = 0;
}

void
CProStatAvgValue::SetTimeSpan(unsigned int timeSpanInSeconds) /* = 5 */
{
    assert(timeSpanInSeconds > 0);
    if (timeSpanInSeconds == 0)
    {
        return;
    }

    m_timeSpan = timeSpanInSeconds;
}

void
CProStatAvgValue::PushData(double dataValue)
{
    int64_t tick = ProGetTickCount64();

    if (m_startTick == 0)
    {
        int64_t initSpan = m_timeSpan * 1000;
        if (initSpan > INIT_SPAN_MS)
        {
            initSpan = INIT_SPAN_MS;
        }

        m_startTick = tick - initSpan;
    }

    ++m_count;
    m_sum += dataValue;

    Update(tick);
}

double
CProStatAvgValue::CalcAvgValue()
{
    return m_avgValue;
}

void
CProStatAvgValue::Update(int64_t tick)
{
    if (m_startTick == 0)
    {
        return;
    }

    if (m_count >= 1) /* double */
    {
        m_avgValue = m_sum / m_count;
    }

    if (tick - m_startTick >= m_timeSpan * 1000 + DELTA_SPAN_MS)
    {
        double countRate = m_count * 1000 / (tick - m_startTick);

        m_startTick = tick - m_timeSpan * 1000;
        m_count     = countRate * m_timeSpan;
        m_sum       = m_count * m_avgValue;
    }
}

/////////////////////////////////////////////////////////////////////////////
////

int64_t
ProSeq16ToSeq64(uint16_t inputSeq,
                int64_t  referenceSeq64)
{
    int64_t seq64 = -1;

    if (referenceSeq64 < 0)
    {
        seq64 = inputSeq;
    }
    else if (inputSeq == (uint16_t)referenceSeq64)
    {
        seq64 = referenceSeq64;
    }
    else if (inputSeq < (uint16_t)referenceSeq64)
    {
        uint16_t dist1 = (uint16_t)-1 - (uint16_t)referenceSeq64 + inputSeq + 1;
        uint16_t dist2 = (uint16_t)referenceSeq64 - inputSeq;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* go forward */
        {
            seq64 =   referenceSeq64 >> 16;
            ++seq64;
            seq64 <<= 16;
            seq64 |=  inputSeq;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* go back */
        {
            seq64 =   referenceSeq64 >> 16;
            seq64 <<= 16;
            seq64 |=  inputSeq;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }
    else
    {
        uint16_t dist1 = inputSeq - (uint16_t)referenceSeq64;
        uint16_t dist2 = (uint16_t)-1 - inputSeq + (uint16_t)referenceSeq64 + 1;

        if (dist1 < dist2 && dist1 < MAX_LOSS_COUNT)      /* go forward */
        {
            seq64 =   referenceSeq64 >> 16;
            seq64 <<= 16;
            seq64 |=  inputSeq;
        }
        else if (dist2 < dist1 && dist2 < MAX_LOSS_COUNT) /* go back */
        {
            seq64 =   referenceSeq64 >> 16;
            --seq64;
            seq64 <<= 16;
            seq64 |=  inputSeq;
        }
        else                                              /* reset */
        {
            seq64 = -1;
        }
    }

    return seq64;
}
