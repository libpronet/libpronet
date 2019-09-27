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
#include "pro_functor_command.h"
#include "pro_functor_command_task.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_time_util.h"
#include "pro_z.h"
#include "../pro_shared/pro_shared.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <mmsystem.h>
#endif

#include <cassert>

#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment(lib, "mmtimer.lib")
#elif defined(WIN32)
#pragma comment(lib, "winmm.lib")
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(PRO_TIMER_UPCALL_COUNT)
#define PRO_TIMER_UPCALL_COUNT     1000
#endif

#define DEFAULT_HEARTBEAT_INTERVAL 20

typedef void (CProTimerFactory::* ACTION)(PRO_INT64*);

/////////////////////////////////////////////////////////////////////////////
////

CProTimerFactory::CProTimerFactory()
{
    m_task         = NULL;
    m_wantExit     = false;
    m_mmTimer      = false;
    m_mmResolution = 0;
    m_htbtTimeSpan = DEFAULT_HEARTBEAT_INTERVAL * 1000;

    m_htbtCounts.resize(1000); /* 1000 steps */
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
            return (false);
        }

        m_task = new CProFunctorCommandTask;
        if (!m_task->Start(mmTimer))
        {
            delete m_task;
            m_task = NULL;

            return (false);
        }

#if defined(WIN32) || defined(_WIN32_WCE)
        if (mmTimer)
        {
            TIMECAPS tc;
            if (::timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
            {
                tc.wPeriodMin = 1;
            }

            if (::timeBeginPeriod(tc.wPeriodMin) == TIMERR_NOERROR)
            {
                m_mmResolution = tc.wPeriodMin;
            }
        }
#endif

        m_mmTimer = mmTimer;

        int       i = 0;
        const int c = (int)m_htbtCounts.size();

        for (; i < c; ++i)
        {
            m_htbtCounts[i] = 0;
        }

        IProFunctorCommand* const command =
            CProFunctorCommand_cpp<CProTimerFactory, ACTION>::CreateInstance(
            *this,
            &CProTimerFactory::WorkerRun
            );
        m_task->Put(command);
    }

    return (true);
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

    CProStlSet<PRO_TIMER_NODE>::const_iterator       itr = timers.begin();
    CProStlSet<PRO_TIMER_NODE>::const_iterator const end = timers.end();

    for (; itr != end; ++itr)
    {
        const PRO_TIMER_NODE& node = *itr;
        node.onTimer->Release();
    }

    {
        CProThreadMutexGuard mon(m_lock);

#if defined(WIN32) || defined(_WIN32_WCE)
        if (m_mmTimer && m_mmResolution > 0)
        {
            ::timeEndPeriod(m_mmResolution);
        }
#endif

        delete m_task;

        m_task         = NULL;
        m_wantExit     = false;
        m_mmTimer      = false;
        m_mmResolution = 0;
        m_htbtTimeSpan = DEFAULT_HEARTBEAT_INTERVAL * 1000;
    }
}}

unsigned long
CProTimerFactory::ScheduleTimer(IProOnTimer* onTimer,
                                PRO_UINT64   timeSpan,
                                bool         recurring,
                                PRO_INT64    userData) /* = 0 */
{
    const unsigned long timeSpan2 = (unsigned long)timeSpan;

    assert(onTimer != NULL);
    assert(timeSpan2 > 0 || !recurring);
    if (
        onTimer == NULL
        ||
        (timeSpan2 == 0 && recurring)
       )
    {
        return (0);
    }

    PRO_TIMER_NODE node;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return (0);
        }

        node.expireTick = ProGetTickCount64() + timeSpan2;
        node.timerId    = m_mmTimer ? ProMakeMmTimerId() : ProMakeTimerId();
        node.onTimer    = onTimer;
        node.timeSpan   = timeSpan2;
        node.recurring  = recurring;
        node.userData   = userData;

        node.onTimer->AddRef();
        m_timers.insert(node);
        m_timerId2ExpireTick[node.timerId] = node.expireTick;
        assert(m_timers.size() == m_timerId2ExpireTick.size());
        m_cond.Signal();
    }

    return (node.timerId);
}

unsigned long
CProTimerFactory::ScheduleHeartbeatTimer(IProOnTimer* onTimer,
                                         PRO_INT64    userData) /* = 0 */
{
    assert(onTimer != NULL);
    if (onTimer == NULL)
    {
        return (0);
    }

    PRO_TIMER_NODE node;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return (0);
        }

        assert(!m_mmTimer);
        if (m_mmTimer)
        {
            return (0);
        }

        int           index = 0;
        unsigned long count = m_htbtCounts[0];

        int       i = 1;
        const int c = (int)m_htbtCounts.size();

        for (; i < c; ++i)
        {
            if (m_htbtCounts[i] < count)
            {
                index = i;
                count = m_htbtCounts[i];
            }
        }

        ++m_htbtCounts[index];

        const PRO_INT64 step = m_htbtTimeSpan / c;
        const PRO_INT64 tick = ProGetTickCount64();

        node.expireTick =  (tick + m_htbtTimeSpan - 1) / m_htbtTimeSpan * m_htbtTimeSpan;
        node.expireTick += step * index;
        node.expireTick += (PRO_INT64)(ProRand_0_1() * (step - 1));
        node.timerId    =  ProMakeTimerId();
        node.onTimer    =  onTimer;
        node.timeSpan   =  m_htbtTimeSpan;
        node.recurring  =  true;
        node.heartbeat  =  true;
        node.htbtIndex  =  index;
        node.userData   =  userData;

        node.onTimer->AddRef();
        m_timers.insert(node);
        m_timerId2ExpireTick[node.timerId] = node.expireTick;
        assert(m_timers.size() == m_timerId2ExpireTick.size());
        m_cond.Signal();
    }

    return (node.timerId);
}

void
CProTimerFactory::CancelTimer(unsigned long timerId)
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

        CProStlMap<unsigned long, PRO_INT64>::iterator const itr =
            m_timerId2ExpireTick.find(timerId);
        if (itr == m_timerId2ExpireTick.end())
        {
            return;
        }

        node.expireTick = itr->second;
        node.timerId    = timerId;

        CProStlSet<PRO_TIMER_NODE>::iterator const itr2 = m_timers.find(node);
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
            --m_htbtCounts[node.htbtIndex];
        }
    }

    node.onTimer->Release();
}

unsigned long
CProTimerFactory::GetTimerCount() const
{
    unsigned long count = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        count = (unsigned long)m_timers.size();
    }

    return (count);
}

bool
CProTimerFactory::UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds)
{
    assert(htbtIntervalInSeconds > 0);
    if (htbtIntervalInSeconds == 0)
    {
        return (false);
    }

    PRO_INT64 timeSpan = htbtIntervalInSeconds;
    timeSpan *= 1000;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task == NULL || m_wantExit)
        {
            return (false);
        }

        if (timeSpan == m_htbtTimeSpan)
        {
            return (true);
        }

        /*
         * set value
         */
        m_htbtTimeSpan = timeSpan;

        /*
         * todo...
         */
        CProStlVector<PRO_TIMER_NODE> timers;

        CProStlSet<PRO_TIMER_NODE>::iterator       itr = m_timers.begin();
        CProStlSet<PRO_TIMER_NODE>::iterator const end = m_timers.end();

        while (itr != end)
        {
            const PRO_TIMER_NODE& node = *itr;
            if (node.heartbeat)
            {
                timers.push_back(node);

                CProStlSet<PRO_TIMER_NODE>::iterator const oldItr = itr;
                ++itr;
                m_timers.erase(oldItr);
            }
            else
            {
                ++itr;
            }
        }

        int             index = 0;
        const int       steps = (int)m_htbtCounts.size();
        const PRO_INT64 step  = m_htbtTimeSpan / steps;
        const PRO_INT64 tick  = ProGetTickCount64();

        int       i = 0;
        const int c = (int)timers.size();

        for (; i < c; ++i)
        {
            if (index < steps)
            {
                m_htbtCounts[index] = 1;
            }
            else
            {
                ++m_htbtCounts[index % steps];
            }

            PRO_TIMER_NODE& node = timers[i];
            node.expireTick =  (tick + m_htbtTimeSpan - 1) / m_htbtTimeSpan * m_htbtTimeSpan;
            node.expireTick += step * index;
            node.expireTick += (PRO_INT64)(ProRand_0_1() * (step - 1));
            node.timeSpan   =  m_htbtTimeSpan;
            node.htbtIndex  =  index++;

            m_timers.insert(node);
            m_timerId2ExpireTick[node.timerId] = node.expireTick;
            assert(m_timers.size() == m_timerId2ExpireTick.size());
        }
    }

    return (true);
}

unsigned long
CProTimerFactory::GetHeartbeatInterval() const
{
    PRO_INT64 timeSpan = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        timeSpan = m_htbtTimeSpan;
    }

    return ((unsigned long)(timeSpan / 1000));
}

void
CProTimerFactory::WorkerRun(PRO_INT64* args)
{
    while (1)
    {
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
                break;
            }

            const PRO_INT64 tick = ProGetTickCount64();

            CProStlSet<PRO_TIMER_NODE>::const_iterator       itr = m_timers.begin();
            CProStlSet<PRO_TIMER_NODE>::const_iterator const end = m_timers.end();

            for (; itr != end; ++itr)
            {
                const PRO_TIMER_NODE& node = *itr;
                if (node.expireTick > tick)
                {
                    break;
                }

                timers.push_back(node);
            }

            int       i = 0;
            const int c = (int)timers.size();

            for (; i < c; ++i)
            {
                PRO_TIMER_NODE& node = timers[i];
                if (node.recurring || node.heartbeat)
                {
                    m_timers.erase(node);

                    if (node.heartbeat)
                    {
                        const PRO_INT64 offset =
                            node.expireTick % node.timeSpan;
                        node.expireTick = (tick + node.timeSpan - 1) /
                            node.timeSpan * node.timeSpan + offset;
                        if (node.expireTick == tick)
                        {
                            node.expireTick += node.timeSpan; /* !!! */
                        }
                    }
                    else
                    {
                        node.expireTick = tick + node.timeSpan;
                    }

                    node.onTimer->AddRef();                   /* !!! */
                    m_timers.insert(node);
                    m_timerId2ExpireTick[node.timerId] = node.expireTick;
                }
                else
                {
                    m_timers.erase(node);
                    m_timerId2ExpireTick.erase(node.timerId);
                }

                assert(m_timers.size() == m_timerId2ExpireTick.size());
            } /* end of for (...) */
        }

        if (timers.size() == 0)
        {
            ProSleep(1); /* 1ms */
            continue;
        }

        int       i = 0;
        const int c = (int)timers.size();

        for (int j = 0; i < c; ++i)
        {
            const PRO_TIMER_NODE& node = timers[i];
            node.onTimer->OnTimer(node.timerId, node.userData);
            node.onTimer->Release();

            ++j;
            if (j == PRO_TIMER_UPCALL_COUNT)
            {
                j = 0;
                ProSleep(1); /* 1ms */
            }
        }
    } /* end of while (...) */
}
