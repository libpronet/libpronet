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

#if !defined(____PRO_TIMER_FACTORY_H____)
#define ____PRO_TIMER_FACTORY_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProFunctorCommandTask;
class IProOnTimer;

struct PRO_TIMER_NODE
{
    PRO_TIMER_NODE()
    {
        expireTick = 0;
        timerId    = 0;

        onTimer    = NULL;
        timeSpan   = 0;
        recurring  = false;
        heartbeat  = false;
        htbtIndex  = 0;
        userData   = 0;
    }

    bool operator<(const PRO_TIMER_NODE& node) const
    {
        if (expireTick < node.expireTick)
        {
            return (true);
        }
        if (expireTick > node.expireTick)
        {
            return (false);
        }

        if (timerId < node.timerId)
        {
            return (true);
        }

        return (false);
    }

    PRO_INT64     expireTick;
    unsigned long timerId;

    IProOnTimer*  onTimer;
    PRO_INT64     timeSpan;
    bool          recurring;
    bool          heartbeat;
    unsigned long htbtIndex;
    PRO_INT64     userData;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * please refer to "pro_net/pro_net.h"
 */
#if !defined(____IProOnTimer____)
#define ____IProOnTimer____
class IProOnTimer
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        ) = 0;
};
#endif /* ____IProOnTimer____ */

/////////////////////////////////////////////////////////////////////////////
////

class CProTimerFactory
{
public:

    CProTimerFactory();

    ~CProTimerFactory();

    bool Start(bool mmTimer);

    void Stop();

    unsigned long ScheduleTimer(
        IProOnTimer* onTimer,
        PRO_UINT64   timeSpan,
        bool         recurring,
        PRO_INT64    userData = 0
        );

    unsigned long ScheduleHeartbeatTimer(
        IProOnTimer* onTimer,
        PRO_INT64    userData = 0
        );

    void CancelTimer(unsigned long timerId);

    unsigned long GetTimerCount() const;

    bool UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds);

    unsigned long GetHeartbeatInterval() const;

private:

    void WorkerRun(PRO_INT64* args);

private:

    CProFunctorCommandTask*              m_task;
    bool                                 m_wantExit;
    bool                                 m_mmTimer;
    unsigned long                        m_mmResolution;
    CProStlSet<PRO_TIMER_NODE>           m_timers;
    CProStlMap<unsigned long, PRO_INT64> m_timerId2ExpireTick;
    PRO_INT64                            m_htbtTimeSpan;
    CProStlVector<unsigned long>         m_htbtCounts;
    CProThreadMutexCondition             m_cond;
    mutable CProThreadMutex              m_lock;
    CProThreadMutex                      m_lockAtom;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_TIMER_FACTORY_H____ */
