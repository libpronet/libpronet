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

#if !defined(RTP_PORT_ALLOCATOR_H)
#define RTP_PORT_ALLOCATOR_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpPortAllocator
{
public:

    CRtpPortAllocator();

    bool SetPortRange(
        unsigned short minPort,
        unsigned short maxPort
        );

    void GetPortRange(
        unsigned short& minPort,
        unsigned short& maxPort
        ) const;

    unsigned short AllocPort(bool rfc);

private:

    unsigned short          m_portBase;
    unsigned short          m_portSpan;
    unsigned short          m_portItr;
    mutable CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_PORT_ALLOCATOR_H */
