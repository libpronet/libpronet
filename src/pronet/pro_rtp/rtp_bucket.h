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

    virtual void PRO_CALLTYPE Destroy();

    virtual unsigned long PRO_CALLTYPE GetTotalBytes() const;

    virtual IRtpPacket* PRO_CALLTYPE GetFront();

    virtual bool PRO_CALLTYPE PushBackAddRef(IRtpPacket* packet);

    virtual void PRO_CALLTYPE PopFrontRelease(IRtpPacket* packet);

    virtual void PRO_CALLTYPE Reset();

    virtual void PRO_CALLTYPE SetRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        );

    virtual void PRO_CALLTYPE GetRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo();

protected:

    unsigned long             m_redlineBytes;
    unsigned long             m_totalBytes;
    CProStlDeque<IRtpPacket*> m_packets;

    mutable CRtpFlowStat      m_flowStat;

    DECLARE_SGI_POOL(0);
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

    virtual bool PRO_CALLTYPE PushBackAddRef(IRtpPacket* packet);
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpVideoBucket : public IRtpBucket
{
public:

    CRtpVideoBucket();

    virtual ~CRtpVideoBucket();

    virtual void PRO_CALLTYPE Destroy();

    virtual unsigned long PRO_CALLTYPE GetTotalBytes() const;

    virtual IRtpPacket* PRO_CALLTYPE GetFront();

    virtual bool PRO_CALLTYPE PushBackAddRef(IRtpPacket* packet);

    virtual void PRO_CALLTYPE PopFrontRelease(IRtpPacket* packet);

    virtual void PRO_CALLTYPE Reset();

    virtual void PRO_CALLTYPE SetRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        );

    virtual void PRO_CALLTYPE GetRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo();

private:

    unsigned long                  m_redlineBytes;
    unsigned long                  m_redlineFrames;
    unsigned long                  m_totalBytes;
    unsigned long                  m_totalFrames;
    RTP_VIDEO_FRAME*               m_waitingFrame;
    CProStlDeque<RTP_VIDEO_FRAME*> m_frames;
    RTP_VIDEO_FRAME*               m_sendingFrame;
    bool                           m_needKeyFrame;
    PRO_UINT16                     m_nextSeq;
    PRO_UINT32                     m_ssrc;

    mutable CRtpFlowStat           m_flowStat;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

IRtpBucket*
PRO_CALLTYPE
CreateRtpBucket(RTP_MM_TYPE      mmType,
                RTP_SESSION_TYPE sessionType);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_BUCKET_H */
