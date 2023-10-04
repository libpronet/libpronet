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

#ifndef ____PRO_TIMER_FACTORY_H____
#define ____PRO_TIMER_FACTORY_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProFunctorCommandTask;
class IProOnTimer;

struct PRO_TIMER_NODE
{
    PRO_TIMER_NODE()
    {
        expireTick    = 0;
        timerId       = 0;

        onTimer       = NULL;
        period        = 0;
        heartbeat     = false;
        htbtSlotIndex = 0;
        userData      = 0;
    }

    bool operator<(const PRO_TIMER_NODE& other) const
    {
        if (expireTick < other.expireTick)
        {
            return true;
        }
        if (expireTick > other.expireTick)
        {
            return false;
        }

        return timerId < other.timerId;
    }

    int64_t      expireTick;
    uint64_t     timerId;

    IProOnTimer* onTimer;
    int64_t      period;
    bool         heartbeat;
    unsigned int htbtSlotIndex;
    int64_t      userData;

    DECLARE_SGI_POOL(0)
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

    virtual ~IProOnTimer() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
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

    uint64_t SetupTimer(
        IProOnTimer* onTimer,
        uint64_t     firstDelay, /* [0, 0xFFFFFFFFFFFF] */
        uint64_t     period,     /* [0, 0xFFFFFFFFFFFF] */
        int64_t      userData = 0
        );

    uint64_t SetupHeartbeatTimer(
        IProOnTimer* onTimer,
        int64_t      userData = 0
        );

    void CancelTimer(uint64_t timerId);

    size_t GetTimerCount() const;

    bool UpdateHeartbeatTimers(unsigned int htbtIntervalInSeconds);

    unsigned int GetHeartbeatInterval() const;

private:

    void WorkerRun(int64_t* args);

    bool Process();

private:

    CProFunctorCommandTask*       m_task;
    bool                          m_wantExit;
    bool                          m_mmTimer;
    unsigned int                  m_mmResolution;
    CProStlSet<PRO_TIMER_NODE>    m_timers;
    CProStlMap<uint64_t, int64_t> m_timerId2ExpireTick;
    int64_t                       m_htbtPeriod;
    CProStlVector<size_t>         m_htbtTimerCounts;
    CProThreadMutexCondition      m_cond;
    mutable CProThreadMutex       m_lock;
    CProThreadMutex               m_lockAtom;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_TIMER_FACTORY_H____ */
