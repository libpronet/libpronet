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
 * This is an implementation of the "Reactor" pattern.
 */

#if !defined(PRO_TP_REACTOR_TASK_H)
#define PRO_TP_REACTOR_TASK_H

#include "pro_net.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProBaseReactor;
class CProEventHandler;

/////////////////////////////////////////////////////////////////////////////
////

class CProTpReactorTask : public IProReactor, public CProThreadBase
{
public:

    CProTpReactorTask();

    virtual ~CProTpReactorTask();

    bool Start(
        unsigned long ioThreadCount,
        long          ioThreadPriority /* = 0, 1, 2 */
        );

    void Stop();

    bool AddHandler(
        int64_t           sockId,
        CProEventHandler* handler,
        unsigned long     mask
        );

    void RemoveHandler(
        int64_t           sockId,
        CProEventHandler* handler,
        unsigned long     mask
        );

    virtual uint64_t ScheduleTimer(
        IProOnTimer* onTimer,
        uint64_t     timeSpan,
        bool         recurring,
        int64_t      userData /* = 0 */
        );

    virtual uint64_t ScheduleHeartbeatTimer(
        IProOnTimer* onTimer,
        int64_t      userData /* = 0 */
        );

    virtual bool UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds);

    virtual void CancelTimer(uint64_t timerId);

    virtual uint64_t ScheduleMmTimer(
        IProOnTimer* onTimer,
        uint64_t     timeSpan,
        bool         recurring,
        int64_t      userData /* = 0 */
        );

    virtual void CancelMmTimer(uint64_t timerId);

    virtual void GetTraceInfo(
        char*  buf,
        size_t size
        ) const;

private:

    void StopMe();

    virtual void Svc();

private:

    CProBaseReactor*                m_acceptReactor;
    CProStlVector<CProBaseReactor*> m_ioReactors;
    CProTimerFactory                m_timerFactory;
    CProTimerFactory                m_mmTimerFactory;
    unsigned long                   m_acceptThreadCount;
    unsigned long                   m_ioThreadCount;
    long                            m_ioThreadPriority;
    unsigned long                   m_curThreadCount;
    bool                            m_wantExit;
    CProStlSet<uint64_t>            m_threadIds;
    CProThreadMutexCondition        m_initCond;
    mutable CProThreadMutex         m_lock;
    CProThreadMutex                 m_lockAtom;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_TP_REACTOR_TASK_H */
