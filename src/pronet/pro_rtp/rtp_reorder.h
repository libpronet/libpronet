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

/////////////////////////////////////////////////////////////////////////////
////

class CRtpReorder : public IRtpReorder
{
public:

    CRtpReorder();

    virtual ~CRtpReorder();

    virtual void PRO_CALLTYPE SetWallHeightInPackets(
        unsigned char heightInPackets         /* = 3 */
        );

    virtual void PRO_CALLTYPE SetWallHeightInMilliseconds(
        unsigned long heightInMs              /* = 500 */
        );

    virtual void PRO_CALLTYPE SetMaxBrokenDuration(
        unsigned char brokenDurationInSeconds /* = 10 */
        );

    virtual unsigned long PRO_CALLTYPE GetTotalPackets() const;

    virtual void PRO_CALLTYPE PushBack(IRtpPacket* packet);

    virtual IRtpPacket* PRO_CALLTYPE PopFront(bool force);

    virtual void PRO_CALLTYPE Reset();

private:

    void Clean();

private:

    unsigned long                      m_heightInPackets;
    PRO_INT64                          m_heightInMs;
    PRO_INT64                          m_maxBrokenDuration;
    PRO_INT64                          m_minSeq64;
    PRO_INT64                          m_lastValidTick;
    CProStlMap<PRO_INT64, IRtpPacket*> m_seq64ToPacket;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_REORDER_H */
