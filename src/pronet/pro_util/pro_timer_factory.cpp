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
#include "pro_timer_factory.h"
#include "pro_functor_command_task.h"
#include "pro_memory_pool.h"
#include "pro_shared.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_time_util.h"
#include "pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define DEFAULT_HEARTBEAT_INTERVAL 20

/////////////////////////////////////////////////////////////////////////////
////

CProTimerFactory::CProTimerFactory()
: m_cond(true) /* isSocketMode is true */
{
    m_task         = NULL;
    m_wantExit     = false;
    m_mmTimer      = false;
    m_mmResolution = 0;
    m_htbtPeriod   = DEFAULT_HEARTBEAT_INTERVAL * 1000;

    m_htbtTimerCounts.resize(1000); /* 1000 slots */
}

CProTimerFactory::~CProTimerFactory()
{
    Stop();
}

bool
CProTimerFactory::Start(bool mmTimer)
{{
    CProThreadMutexGuard mon(m_lockAtom);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_task == NULL);
        if (m_task != NULL)
        {
            return false;
        }

        m_task = new CProFunctorCommandTask;
        if (!m_task->Start(mmTimer))
        {
            delete m_task;
            m_task = NULL;

            return false;
        }

#if defined(_WIN32)
        TIMECAPS caps;
        if (::timeGetDevCaps(&caps, sizeof(TIMECAPS)) != TIMERR_NOERROR)
        {
            caps.wPeriodMin = 1;
        }
        if (::timeBeginPeriod(caps.wPeriodMin) == TIMERR_NOERROR)
        {
            m_mmResolution = caps.wPeriodMin;
        }
#endif

        m_mmTimer = mmTimer;

        int i = 0;
        int c = (int)m_htbtTimerCounts.size();

        for (; i < c; ++i)
        {
            m_htbtTimerCounts[i] = 0; /* clean all slots */
        }

        m_task->PostCall(*this, &CProTimerFactory::WorkerRun);
    }

    return true;
}}

void
CProTimerFactory::Stop()
{{
    CProThreadMutexGuard mon(m_lockAtom);

    CProStlSet<PRO_TIMER_NODE> timers;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL)
        {
            return;
        }

        m_timerId2ExpireTick.clear();
        timers = m_timers;
        m_timers.clear();

        m_wantExit = true;
        m_cond.Signal();
    }

    m_task->Stop();

    auto itr = timers.begin();
    auto end = timers.end();

    for (; itr != end; ++itr)
    {
        const PRO_TIMER_NODE& node = *itr;
        node.onTimer->Release();
    }

    {
        CProThreadMutexGuard mon(m_lock);

#if defined(_WIN32)
        if (m_mmResolution > 0)
        {
            ::timeEndPeriod(m_mmResolution);
        }
#endif

        delete m_task;

        m_task         = NULL;
        m_wantExit     = false;
        m_mmTimer      = false;
        m_mmResolution = 0;
        m_htbtPeriod   = DEFAULT_HEARTBEAT_INTERVAL * 1000;
    }
}}

uint64_t
CProTimerFactory::SetupTimer(IProOnTimer* onTimer,
                             uint64_t     firstDelay, /* [0, 0xFFFFFFFFFFFF] */
                             uint64_t     period,     /* [0, 0xFFFFFFFFFFFF] */
                             int64_t      userData)   /* = 0 */
{
    if (firstDelay > 0xFFFFFFFFFFFFULL)
    {
        firstDelay = 0xFFFFFFFFFFFFULL;
    }
    if (period > 0xFFFFFFFFFFFFULL)
    {
        period     = 0xFFFFFFFFFFFFULL;
    }

    assert(onTimer != NULL);
    if (onTimer == NULL)
    {
        return 0;
    }

    PRO_TIMER_NODE node;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return 0;
        }

        node.expireTick = ProGetTickCount64() + firstDelay;
        node.timerId    = m_mmTimer ? ProMakeMmTimerId() : ProMakeTimerId();
        node.onTimer    = onTimer;
        node.period     = period;
        node.userData   = userData;

        node.onTimer->AddRef();
        m_timers.insert(node);
        m_timerId2ExpireTick[node.timerId] = node.expireTick;
        assert(m_timers.size() == m_timerId2ExpireTick.size());

        m_cond.Signal();
    }

    return node.timerId;
}

uint64_t
CProTimerFactory::SetupHeartbeatTimer(IProOnTimer* onTimer,
                                      int64_t      userData) /* = 0 */
{
    assert(onTimer != NULL);
    if (onTimer == NULL)
    {
        return 0;
    }

    PRO_TIMER_NODE node;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return 0;
        }

        assert(!m_mmTimer);
        if (m_mmTimer)
        {
            return 0;
        }

        int    index = 0;
        size_t count = m_htbtTimerCounts[0];

        int i = 1;
        int c = (int)m_htbtTimerCounts.size();

        for (; i < c; ++i)
        {
            if (m_htbtTimerCounts[i] < count)
            {
                index = i;
                count = m_htbtTimerCounts[i];
            }
        }

        /*
         * put it into the step
         */
        ++m_htbtTimerCounts[index];

        int64_t step = m_htbtPeriod / c;
        int64_t tick = ProGetTickCount64();

        node.expireTick    =  (tick + m_htbtPeriod - 1) / m_htbtPeriod * m_htbtPeriod;
        node.expireTick    += step * index;
        node.expireTick    += ProRand_0_32767() % step;
        node.timerId       =  ProMakeTimerId();
        node.onTimer       =  onTimer;
        node.period        =  m_htbtPeriod;
        node.heartbeat     =  true;
        node.htbtSlotIndex =  index;
        node.userData      =  userData;

        node.onTimer->AddRef();
        m_timers.insert(node);
        m_timerId2ExpireTick[node.timerId] = node.expireTick;
        assert(m_timers.size() == m_timerId2ExpireTick.size());

        m_cond.Signal();
    }

    return node.timerId;
}

void
CProTimerFactory::CancelTimer(uint64_t timerId)
{
    if (timerId == 0)
    {
        return;
    }

    PRO_TIMER_NODE node;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL)
        {
            return;
        }

        auto itr = m_timerId2ExpireTick.find(timerId);
        if (itr == m_timerId2ExpireTick.end())
        {
            return;
        }

        node.expireTick = itr->second;
        node.timerId    = timerId;

        auto itr2 = m_timers.find(node);
        if (itr2 == m_timers.end())
        {
            return;
        }

        node = *itr2;

        m_timers.erase(itr2);
        m_timerId2ExpireTick.erase(itr);
        assert(m_timers.size() == m_timerId2ExpireTick.size());

        if (node.heartbeat)
        {
            --m_htbtTimerCounts[node.htbtSlotIndex];
        }
    }

    node.onTimer->Release();
}

size_t
CProTimerFactory::GetTimerCount() const
{
    size_t count = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        count = m_timers.size();
    }

    return count;
}

bool
CProTimerFactory::UpdateHeartbeatTimers(unsigned int htbtIntervalInSeconds)
{
    assert(htbtIntervalInSeconds > 0);
    if (htbtIntervalInSeconds == 0)
    {
        return false;
    }

    int64_t period = (int64_t)htbtIntervalInSeconds * 1000;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return false;
        }

        if (period == m_htbtPeriod)
        {
            return true;
        }

        /*
         * set value
         */
        m_htbtPeriod = period;

        /*
         * todo...
         */
        CProStlVector<PRO_TIMER_NODE> timers;

        auto itr = m_timers.begin();
        auto end = m_timers.end();

        while (itr != end)
        {
            const PRO_TIMER_NODE& node = *itr;
            if (node.heartbeat)
            {
                timers.push_back(node);
                m_timers.erase(itr++);
            }
            else
            {
                ++itr;
            }
        }

        int     slots = (int)m_htbtTimerCounts.size();
        int64_t step  = m_htbtPeriod / slots;
        int64_t tick  = ProGetTickCount64();

        int i = 0;
        int c = (int)timers.size();

        for (; i < c; ++i)
        {
            /*
             * put it into the slot
             */
            if (i < slots)
            {
                m_htbtTimerCounts[i] = 1;
            }
            else
            {
                ++m_htbtTimerCounts[i % slots];
            }

            PRO_TIMER_NODE& node = timers[i];
            node.expireTick    =  (tick + m_htbtPeriod - 1) / m_htbtPeriod * m_htbtPeriod;
            node.expireTick    += step * (i % slots);
            node.expireTick    += ProRand_0_32767() % step;
            node.period        =  m_htbtPeriod;
            node.htbtSlotIndex =  i % slots;

            m_timers.insert(node);
            m_timerId2ExpireTick[node.timerId] = node.expireTick;
            assert(m_timers.size() == m_timerId2ExpireTick.size());
        }

        m_cond.Signal();
    }

    return true;
}

unsigned int
CProTimerFactory::GetHeartbeatInterval() const
{
    int64_t period = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        period = m_htbtPeriod;
    }

    return (unsigned int)(period / 1000);
}

void
CProTimerFactory::WorkerRun()
{
    while (Process())
    {
    }
}

bool
CProTimerFactory::Process()
{
    int64_t                       tick = 0;
    CProStlVector<PRO_TIMER_NODE> timers;

    {
        CProThreadMutexGuard mon(m_lock);

        while (1)
        {
            if (m_wantExit || m_timers.size() > 0)
            {
                break;
            }

            m_cond.Wait(&m_lock);
        }

        if (m_wantExit)
        {
            return false;
        }

        tick = ProGetTickCount64();

        unsigned int timeout = 0xFFFFFFFF;

        auto itr = m_timers.begin();
        auto end = m_timers.end();

        for (; itr != end; ++itr)
        {
            const PRO_TIMER_NODE& node = *itr;
            if (node.expireTick > tick)
            {
                uint64_t delta = node.expireTick - tick;
                if (delta > 0xFFFFFFFF)
                {
                    delta = 0xFFFFFFFF;
                }

                timeout = (unsigned int)delta;
                break;
            }

            timers.push_back(node);
        }

        int i = 0;
        int c = (int)timers.size();

        for (; i < c; ++i)
        {
            PRO_TIMER_NODE& node = timers[i];
            if (node.period > 0)
            {
                m_timers.erase(node);

                if (node.heartbeat)
                {
                    int64_t offset = node.expireTick % node.period;
                    node.expireTick =
                        (tick + node.period - 1) / node.period * node.period + offset;
                    if (node.expireTick == tick)
                    {
                        node.expireTick += node.period; /* !!! */
                    }
                }
                else
                {
                    node.expireTick = tick + node.period;
                }

                node.onTimer->AddRef(); /* !!! */
                m_timers.insert(node);
                m_timerId2ExpireTick[node.timerId] = node.expireTick;
            }
            else
            {
                m_timers.erase(node);
                m_timerId2ExpireTick.erase(node.timerId);
            }

            assert(m_timers.size() == m_timerId2ExpireTick.size());
        } /* end of for () */

        if (c == 0)
        {
            m_cond.Wait(&m_lock, timeout);
        }
    }

    int i = 0;
    int j = 0;
    int c = (int)timers.size();

    for (; i < c; ++i)
    {
        const PRO_TIMER_NODE& node = timers[i];
        node.onTimer->OnTimer(this, node.timerId, tick, node.userData);
        node.onTimer->Release();

        ++j;
        if (j == PRO_TIMER_UPCALL_COUNT)
        {
            j = 0;

            ProSleep(1); /* 1ms */
            tick = ProGetTickCount64();
        }
    }

    return true;
}
