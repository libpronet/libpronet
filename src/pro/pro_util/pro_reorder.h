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

#if !defined(____PRO_REORDER_H____)
#define ____PRO_REORDER_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

class IRtpPacket;

/////////////////////////////////////////////////////////////////////////////
////

class CProReorder
{
public:

    CProReorder();

    ~CProReorder();

    void SetMaxPacketCount(unsigned char maxPacketCount);                  /* = 5 */

    void SetMaxWaitingDuration(unsigned char maxWaitingDurationInSeconds); /* = 1 */

    void SetMaxBrokenDuration(unsigned char maxBrokenDurationInSeconds);   /* = 10 */

    void PushBack(IRtpPacket* packet);

    IRtpPacket* PopFront();

    void Reset();

private:

    unsigned long                      m_maxPacketCount;
    PRO_INT64                          m_maxWaitingDuration;
    PRO_INT64                          m_maxBrokenDuration;
    PRO_INT64                          m_minSeq64;
    PRO_INT64                          m_lastValidTick;
    CProStlMap<PRO_INT64, IRtpPacket*> m_seq64ToPacket;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_REORDER_H____ */
