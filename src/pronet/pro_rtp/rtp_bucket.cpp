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

#include "rtp_bucket.h"
#include "rtp_base.h"
#include "rtp_flow_stat.h"
#include "rtp_packet.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define BASE_REDLINE_BYTES   (1024 * 1024)
#define AUDIO_REDLINE_BYTES  (1024 * 16)
#define VIDEO_REDLINE_BYTES  (1024 * 512)
#define VIDEO_REDLINE_FRAMES 30
#define MAX_FRAME_SIZE       (1024 * 1024)
#define MAX_AUDIO_DELAY_MS   1000
#define MAX_VIDEO_DELAY_MS   1200

struct RTP_VIDEO_FRAME
{
    RTP_VIDEO_FRAME()
    {
        tick     = ProGetTickCount64();
        keyFrame = false;

        bucket.SetRedline(MAX_FRAME_SIZE, 0, 0);
    }

    int64_t    tick;
    bool       keyFrame;
    CRtpBucket bucket;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

CRtpBucket::CRtpBucket()
{
    m_redlineBytes   = BASE_REDLINE_BYTES;
    m_redlineFrames  = 0;
    m_redlineDelayMs = 0;
    m_totalBytes     = 0;

    m_flowStat.SetTimeSpan(GetRtpFlowctrlTimeSpan());
}

CRtpBucket::~CRtpBucket()
{
    Reset();
}

void
CRtpBucket::Destroy()
{
    delete this;
}

IRtpPacket*
CRtpBucket::GetFront()
{
    if (m_packets.size() == 0)
    {
        return NULL;
    }

    return m_packets.front();
}

bool
CRtpBucket::PushBackAddRef(IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return false;
    }

    size_t size = packet->GetPayloadSize();
    m_flowStat.PushData(1, size);

    /*
     * arrival time
     */
    int64_t tick = ProGetTickCount64();
    ((CRtpPacket*)packet)->SetMagic2(tick);

    /*
     * remove old packets
     */
    while (m_packets.size() > 0)
    {
        CRtpPacket* packet2 = (CRtpPacket*)m_packets.front();
        if (tick - packet2->GetMagic2() > m_redlineDelayMs && m_redlineDelayMs > 0)
        {
            m_packets.pop_front();
            m_totalBytes -= packet2->GetPayloadSize();
            packet2->Release();
            continue;
        }
        break;
    }

    if (m_totalBytes > 0)
    {
        if (
            m_totalBytes + size > m_redlineBytes
            ||
            (m_packets.size() >= m_redlineFrames && m_redlineFrames > 0)
           )
        {
            return false;
        }
    }

    packet->AddRef();
    m_packets.push_back(packet);
    m_totalBytes += size;

    return true;
}

void
CRtpBucket::PopFrontRelease(IRtpPacket* packet)
{
    if (packet == NULL || m_packets.size() == 0 || packet != m_packets.front())
    {
        return;
    }

    m_packets.pop_front();

    size_t size = packet->GetPayloadSize();
    m_flowStat.PopData(1, size);

    m_totalBytes -= size;
    packet->Release();
}

void
CRtpBucket::Reset()
{
    int i = 0;
    int c = (int)m_packets.size();

    for (; i < c; ++i)
    {
        m_packets[i]->Release();
    }

    m_totalBytes = 0;
    m_packets.clear();

    m_flowStat.Reset();
}

void
CRtpBucket::SetRedline(size_t redlineBytes,   /* = 0 */
                       size_t redlineFrames,  /* = 0 */
                       size_t redlineDelayMs) /* = 0 */
{
    if (redlineBytes > 0)
    {
        m_redlineBytes   = redlineBytes;
    }
    if (redlineFrames > 0)
    {
        m_redlineFrames  = redlineFrames;
    }
    if (redlineDelayMs > 0)
    {
        m_redlineDelayMs = redlineDelayMs;
    }
}

void
CRtpBucket::GetRedline(size_t* redlineBytes,         /* = NULL */
                       size_t* redlineFrames,        /* = NULL */
                       size_t* redlineDelayMs) const /* = NULL */
{
    if (redlineBytes != NULL)
    {
        *redlineBytes   = m_redlineBytes;
    }
    if (redlineFrames != NULL)
    {
        *redlineFrames  = m_redlineFrames;
    }
    if (redlineDelayMs != NULL)
    {
        *redlineDelayMs = (size_t)m_redlineDelayMs;
    }
}

void
CRtpBucket::GetFlowctrlInfo(float*  srcFrameRate,       /* = NULL */
                            float*  srcBitRate,         /* = NULL */
                            float*  outFrameRate,       /* = NULL */
                            float*  outBitRate,         /* = NULL */
                            size_t* cachedBytes,        /* = NULL */
                            size_t* cachedFrames) const /* = NULL */
{
    m_flowStat.CalcInfo(srcFrameRate, srcBitRate, outFrameRate, outBitRate);

    if (cachedBytes != NULL)
    {
        *cachedBytes  = m_totalBytes;
    }
    if (cachedFrames != NULL)
    {
        *cachedFrames = m_packets.size();
    }
}

void
CRtpBucket::ResetFlowctrlInfo()
{
    m_flowStat.Reset();
}

/////////////////////////////////////////////////////////////////////////////
////

CRtpAudioBucket::CRtpAudioBucket()
{
    m_redlineBytes   = AUDIO_REDLINE_BYTES;
    m_redlineDelayMs = MAX_AUDIO_DELAY_MS;
}

bool
CRtpAudioBucket::PushBackAddRef(IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return false;
    }

    size_t size = packet->GetPayloadSize();
    m_flowStat.PushData(1, size);

    /*
     * arrival time
     */
    int64_t tick = ProGetTickCount64();
    ((CRtpPacket*)packet)->SetMagic2(tick);

    /*
     * remove old packets
     */
    while (m_packets.size() > 0)
    {
        CRtpPacket* packet2 = (CRtpPacket*)m_packets.front();
        if (tick - packet2->GetMagic2() > m_redlineDelayMs || m_totalBytes + size > m_redlineBytes)
        {
            m_packets.pop_front();
            m_totalBytes -= packet2->GetPayloadSize();
            packet2->Release();
            continue;
        }
        break;
    }

    packet->AddRef();
    m_packets.push_back(packet);
    m_totalBytes += size;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
////

CRtpVideoBucket::CRtpVideoBucket()
{
    m_redlineBytes   = VIDEO_REDLINE_BYTES;
    m_redlineFrames  = VIDEO_REDLINE_FRAMES;
    m_redlineDelayMs = MAX_VIDEO_DELAY_MS;
    m_totalBytes     = 0;
    m_totalFrames    = 0;
    m_waitingFrame   = NULL;
    m_sendingFrame   = NULL;
    m_needKeyFrame   = true;

    m_flowStat.SetTimeSpan(GetRtpFlowctrlTimeSpan());
}

CRtpVideoBucket::~CRtpVideoBucket()
{
    Reset();
}

void
CRtpVideoBucket::Destroy()
{
    delete this;
}

IRtpPacket*
CRtpVideoBucket::GetFront()
{
    if (m_sendingFrame != NULL)
    {
        IRtpPacket* packet = m_sendingFrame->bucket.GetFront();
        if (packet != NULL)
        {
            return packet;
        }

        delete m_sendingFrame;
        m_sendingFrame = NULL;
        --m_totalFrames;
    }

    if (m_frames.size() == 0)
    {
        return NULL;
    }

    m_sendingFrame = m_frames.front();
    m_frames.pop_front();

    return m_sendingFrame->bucket.GetFront();
}

bool
CRtpVideoBucket::PushBackAddRef(IRtpPacket* packet)
{
    assert(packet != NULL);
    if (packet == NULL)
    {
        return false;
    }

    bool marker             = packet->GetMarker();
    bool keyFrame           = packet->GetKeyFrame();
    bool firstPacketOfFrame = packet->GetFirstPacketOfFrame();

    size_t size = packet->GetPayloadSize();
    m_flowStat.PushData(marker ? 1 : 0, size);

    /*
     * 1. synchronization point
     */
    if (m_needKeyFrame)
    {
        if (!keyFrame || !firstPacketOfFrame)
        {
            return false;
        }

        m_needKeyFrame = false;
    }

    /*
     * 2. waiting-frame
     */
    {
        if (firstPacketOfFrame)
        {
            if (m_waitingFrame != NULL)
            {
                m_totalBytes -= m_waitingFrame->bucket.GetTotalBytes();
                delete m_waitingFrame;
                m_waitingFrame = NULL;
                --m_totalFrames;
            }

            m_waitingFrame = new RTP_VIDEO_FRAME;
            m_waitingFrame->keyFrame = keyFrame;
            ++m_totalFrames;
        }
        else
        {
            if (m_waitingFrame == NULL)
            {
                m_needKeyFrame = true; /* ===resynchronize=== */

                return false;
            }
        }

        /*
         * move to waiting-frame
         */
        m_waitingFrame->bucket.PushBackAddRef(packet);
        m_totalBytes += size;

        /*
         * check waiting-frame
         */
        if (m_waitingFrame->bucket.GetTotalBytes() > MAX_FRAME_SIZE)
        {
            m_totalBytes -= m_waitingFrame->bucket.GetTotalBytes();
            delete m_waitingFrame;
            m_waitingFrame = NULL;
            --m_totalFrames;

            m_needKeyFrame = true; /* ===resynchronize=== */

            return false;
        }

        if (!marker)
        {
            return true;
        }
    }

    /*
     * 3. P-frame
     */
    if (!m_waitingFrame->keyFrame)
    {
        /*
         * check redline
         */
        if (m_totalBytes > m_redlineBytes || m_totalFrames > m_redlineFrames)
        {
            m_totalBytes -= m_waitingFrame->bucket.GetTotalBytes();
            delete m_waitingFrame;
            m_waitingFrame = NULL;
            --m_totalFrames;

            m_needKeyFrame = true; /* ===resynchronize=== */

            return false;
        }

        /*
         * move to frame list
         */
        m_frames.push_back(m_waitingFrame);
        m_waitingFrame = NULL;

        /*
         * check timeout
         */
        RemoveOldFrames();

        return m_frames.size() > 0;
    }

    /*
     * 4. I-frame
     */
    {
        /*
         * clean old frames
         */
        while (m_frames.size() > 0)
        {
            RTP_VIDEO_FRAME* frame = m_frames.front();
            m_frames.pop_front();
            m_totalBytes -= frame->bucket.GetTotalBytes();
            delete frame;
            --m_totalFrames;
        }

        /*
         * move to frame list
         */
        m_frames.push_back(m_waitingFrame);
        m_waitingFrame = NULL;
    }

    return true;
}

void
CRtpVideoBucket::RemoveOldFrames()
{
    if (m_frames.size() == 0)
    {
        return;
    }

    int64_t tick = ProGetTickCount64();

    /*
     * first frame
     */
    RTP_VIDEO_FRAME* frame = m_frames.front();
    if (tick - frame->tick <= m_redlineDelayMs)
    {
        return;
    }

    while (m_frames.size() > 0)
    {
        frame = m_frames.front();

        /*
         * find a good GOP
         */
        if (frame->keyFrame && tick - frame->tick <= m_redlineDelayMs)
        {
            m_needKeyFrame = false;
            break;
        }

        /*
         * remove old frames
         */
        m_frames.pop_front();
        m_totalBytes -= frame->bucket.GetTotalBytes();
        delete frame;
        --m_totalFrames;

        m_needKeyFrame = true; /* ===resynchronize=== */
    }
}

void
CRtpVideoBucket::PopFrontRelease(IRtpPacket* packet)
{
    if (packet == NULL || m_sendingFrame == NULL || packet != m_sendingFrame->bucket.GetFront())
    {
        return;
    }

    size_t size = packet->GetPayloadSize();
    m_flowStat.PopData(packet->GetMarker() ? 1 : 0, size);

    m_totalBytes -= size;
    m_sendingFrame->bucket.PopFrontRelease(packet);

    if (m_sendingFrame->bucket.GetFront() == NULL)
    {
        delete m_sendingFrame;
        m_sendingFrame = NULL;
        --m_totalFrames;
    }
}

void
CRtpVideoBucket::Reset()
{
    delete m_waitingFrame;

    int i = 0;
    int c = (int)m_frames.size();

    for (; i < c; ++i)
    {
        delete m_frames[i];
    }

    delete m_sendingFrame;

    m_totalBytes   = 0;
    m_totalFrames  = 0;
    m_waitingFrame = NULL;
    m_frames.clear();
    m_sendingFrame = NULL;
    m_needKeyFrame = true;

    m_flowStat.Reset();
}

void
CRtpVideoBucket::SetRedline(size_t redlineBytes,   /* = 0 */
                            size_t redlineFrames,  /* = 0 */
                            size_t redlineDelayMs) /* = 0 */
{
    if (redlineBytes > 0)
    {
        m_redlineBytes   = redlineBytes;
    }
    if (redlineFrames > 0)
    {
        m_redlineFrames  = redlineFrames;
    }
    if (redlineDelayMs > 0)
    {
        m_redlineDelayMs = redlineDelayMs;
    }
}

void
CRtpVideoBucket::GetRedline(size_t* redlineBytes,         /* = NULL */
                            size_t* redlineFrames,        /* = NULL */
                            size_t* redlineDelayMs) const /* = NULL */
{
    if (redlineBytes != NULL)
    {
        *redlineBytes   = m_redlineBytes;
    }
    if (redlineFrames != NULL)
    {
        *redlineFrames  = m_redlineFrames;
    }
    if (redlineDelayMs != NULL)
    {
        *redlineDelayMs = (size_t)m_redlineDelayMs;
    }
}

void
CRtpVideoBucket::GetFlowctrlInfo(float*  srcFrameRate,       /* = NULL */
                                 float*  srcBitRate,         /* = NULL */
                                 float*  outFrameRate,       /* = NULL */
                                 float*  outBitRate,         /* = NULL */
                                 size_t* cachedBytes,        /* = NULL */
                                 size_t* cachedFrames) const /* = NULL */
{
    m_flowStat.CalcInfo(srcFrameRate, srcBitRate, outFrameRate, outBitRate);

    if (cachedBytes != NULL)
    {
        *cachedBytes  = m_totalBytes;
    }
    if (cachedFrames != NULL)
    {
        *cachedFrames = m_totalFrames;
    }
}

void
CRtpVideoBucket::ResetFlowctrlInfo()
{
    m_flowStat.Reset();
}

/////////////////////////////////////////////////////////////////////////////
////

IRtpBucket*
CreateRtpBucket(RTP_MM_TYPE      mmType,
                RTP_SESSION_TYPE sessionType)
{
    assert(mmType != 0);
    if (mmType == 0)
    {
        return NULL;
    }

    IRtpBucket* bucket = NULL;

    if (mmType >= RTP_MMT_AUDIO_MIN && mmType <= RTP_MMT_AUDIO_MAX)
    {
        bucket = new CRtpAudioBucket;
    }
    else if (mmType >= RTP_MMT_VIDEO_MIN && mmType <= RTP_MMT_VIDEO_MAX)
    {
        switch (sessionType)
        {
        case RTP_ST_TCPCLIENT_EX:
        case RTP_ST_TCPSERVER_EX:
        case RTP_ST_SSLCLIENT_EX:
        case RTP_ST_SSLSERVER_EX:
            bucket = new CRtpVideoBucket;
            break;
        default:
            bucket = new CRtpBucket;
            break;
        }
    }
    else
    {
        bucket = new CRtpBucket;
    }

    return bucket;
}
