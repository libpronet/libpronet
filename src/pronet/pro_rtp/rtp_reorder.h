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

#if !defined(RTP_REORDER_H)
#define RTP_REORDER_H

#include "rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpReorder : public IRtpReorder
{
public:

    CRtpReorder();

    virtual ~CRtpReorder();

    virtual void SetWallHeightInPackets(size_t heightInPackets); /* = 100 */

    virtual void SetWallHeightInMilliseconds(size_t heightInMs); /* = 500 */

    virtual void SetMaxBrokenDuration(size_t brokenDurationInSeconds); /* = 10 */

    virtual size_t GetTotalPackets() const;

    virtual void PushBackAddRef(IRtpPacket* packet);

    virtual IRtpPacket* PopFront(bool force);

    virtual void Reset();

private:

    void Clean();

private:

    size_t                           m_heightInPackets;
    int64_t                          m_heightInMs;
    int64_t                          m_maxBrokenDuration;
    int64_t                          m_minSeq64;
    int64_t                          m_lastValidTick;
    CProStlMap<int64_t, IRtpPacket*> m_seq64ToPacket;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_REORDER_H */
