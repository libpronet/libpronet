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

#include "rtp_port_allocator.h"
#include "../pro_shared/pro_shared.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_MIN_PORT 3000
#define DEFAULT_MAX_PORT 5999

/////////////////////////////////////////////////////////////////////////////
////

CRtpPortAllocator::CRtpPortAllocator()
{
    m_portBase = DEFAULT_MIN_PORT;
    m_portSpan = DEFAULT_MAX_PORT - DEFAULT_MIN_PORT;
    m_portItr  = (unsigned short)(ProRand_0_1() * m_portSpan) / 2 * 2;
}

bool
CRtpPortAllocator::SetPortRange(unsigned short minPort,
                                unsigned short maxPort)
{
    assert(minPort > 0);
    assert(minPort < maxPort);
    if (minPort == 0 || minPort >= maxPort)
    {
        return (false);
    }

    if (minPort % 2 != 0)
    {
        ++minPort;
    }
    if (maxPort % 2 == 0)
    {
        --maxPort;
    }

    assert(minPort < maxPort);
    if (minPort >= maxPort)
    {
        return (false);
    }

    m_lock.Lock();
    m_portBase = minPort;
    m_portSpan = maxPort - minPort;
    m_portItr  = (unsigned short)(ProRand_0_1() * m_portSpan) / 2 * 2;
    m_lock.Unlock();

    return (true);
}

void
CRtpPortAllocator::GetPortRange(unsigned short& minPort,
                                unsigned short& maxPort) const
{
    m_lock.Lock();
    minPort = m_portBase;
    maxPort = m_portBase + m_portSpan;
    m_lock.Unlock();
}

unsigned short
CRtpPortAllocator::AllocPort()
{
    m_lock.Lock();
    unsigned short port = m_portBase + m_portItr % m_portSpan;
    m_portItr += 2;
    m_lock.Unlock();

    if (port % 2 != 0)
    {
        --port;
    }

    return (port);
}
