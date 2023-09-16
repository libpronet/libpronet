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

/*
 * This is an implementation of the "Strategy" pattern.
 */

#if !defined(RTP_BUCKET_H)
#define RTP_BUCKET_H

#include "rtp_base.h"
#include "rtp_flow_stat.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

struct RTP_VIDEO_FRAME;

/////////////////////////////////////////////////////////////////////////////
////

class CRtpBucket : public IRtpBucket
{
public:

    CRtpBucket();

    virtual ~CRtpBucket();

    virtual void Destroy();

    virtual size_t GetTotalBytes() const
    {
        return m_totalBytes;
    }

    virtual IRtpPacket* GetFront();

    virtual bool PushBackAddRef(IRtpPacket* packet);

    virtual void PopFrontRelease(IRtpPacket* packet);

    virtual void Reset();

    virtual void SetRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        );

    virtual void GetRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const;

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const;

    virtual void ResetFlowctrlInfo();

protected:

    size_t                    m_redlineBytes;
    size_t                    m_redlineFrames;
    int64_t                   m_redlineDelayMs;
    size_t                    m_totalBytes;
    CProStlDeque<IRtpPacket*> m_packets;

    mutable CRtpFlowStat      m_flowStat;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpAudioBucket : public CRtpBucket
{
public:

    CRtpAudioBucket();

    virtual ~CRtpAudioBucket()
    {
    }

    virtual bool PushBackAddRef(IRtpPacket* packet);

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpVideoBucket : public IRtpBucket
{
public:

    CRtpVideoBucket();

    virtual ~CRtpVideoBucket();

    virtual void Destroy();

    virtual size_t GetTotalBytes() const
    {
        return m_totalBytes;
    }

    virtual IRtpPacket* GetFront();

    virtual bool PushBackAddRef(IRtpPacket* packet);

    virtual void PopFrontRelease(IRtpPacket* packet);

    virtual void Reset();

    virtual void SetRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        );

    virtual void GetRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const;

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const;

    virtual void ResetFlowctrlInfo();

private:

    void RemoveOldFrames();

private:

    size_t                         m_redlineBytes;
    size_t                         m_redlineFrames;
    int64_t                        m_redlineDelayMs;
    size_t                         m_totalBytes;
    size_t                         m_totalFrames;
    RTP_VIDEO_FRAME*               m_waitingFrame;
    CProStlDeque<RTP_VIDEO_FRAME*> m_frames;
    RTP_VIDEO_FRAME*               m_sendingFrame;
    bool                           m_needKeyFrame;

    mutable CRtpFlowStat           m_flowStat;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

IRtpBucket*
CreateRtpBucket(RTP_MM_TYPE      mmType,
                RTP_SESSION_TYPE sessionType);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_BUCKET_H */
