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

#include "pro_tp_reactor_task.h"
#include "pro_base_reactor.h"
#include "pro_epoll_reactor.h"
#include "pro_event_handler.h"
#include "pro_net.h"
#include "pro_select_reactor.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_HAS_EPOLL)
typedef CProEpollReactor  CProReactorImpl;
#else
typedef CProSelectReactor CProReactorImpl;
#endif

/////////////////////////////////////////////////////////////////////////////
////

CProTpReactorTask::CProTpReactorTask()
{
    m_acceptReactor     = NULL;
    m_acceptThreadCount = 0;
    m_ioThreadCount     = 0;
    m_ioThreadPriority  = 0;
    m_curThreadCount    = 0;
    m_wantExit          = false;
}

CProTpReactorTask::~CProTpReactorTask()
{
    Stop();
}

bool
CProTpReactorTask::Start(unsigned long ioThreadCount,
                         long          ioThreadPriority) /* = 0, 1, 2 */
{{
    CProThreadMutexGuard mon(m_lockAtom);

    assert(ioThreadCount > 0);
    assert(
        ioThreadPriority == 0 ||
        ioThreadPriority == 1 ||
        ioThreadPriority == 2
        );
    if (
        ioThreadCount == 0
        ||
        (ioThreadPriority != 0 &&
         ioThreadPriority != 1 &&
         ioThreadPriority != 2)
       )
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_acceptThreadCount + m_ioThreadCount == 0);
        if (m_acceptThreadCount + m_ioThreadCount != 0)
        {
            return (false);
        }

        m_acceptThreadCount = 1;
        m_ioThreadCount     = ioThreadCount; /* for StopMe() */
        m_ioThreadPriority  = ioThreadPriority;

        /*
         * reactors
         */
        {
            m_acceptReactor = new CProReactorImpl;
            if (!m_acceptReactor->Init())
            {
                goto EXIT;
            }

            for (int i = 0; i < (int)m_ioThreadCount; ++i)
            {
                CProBaseReactor* const reactor = new CProReactorImpl;
                if (!reactor->Init())
                {
                    delete reactor;
                    break;
                }

                m_ioReactors.push_back(reactor);
            }

            assert(m_ioReactors.size() == m_ioThreadCount);
            if (m_ioReactors.size() != m_ioThreadCount)
            {
                goto EXIT;
            }
        }

        /*
         * threads
         */
        {
            int i;

            for (i = 0; i < (int)(m_acceptThreadCount + m_ioThreadCount); ++i)
            {
                if (!Spawn(false))
                {
                    break;
                }
            }

            assert(i == (int)(m_acceptThreadCount + m_ioThreadCount));
            if (i != (int)(m_acceptThreadCount + m_ioThreadCount))
            {
                goto EXIT;
            }
        }

        /*
         * timer factories
         */
        if (!m_timerFactory.Start(false) || !m_mmTimerFactory.Start(true))
        {
            goto EXIT;
        }

        while (m_curThreadCount < m_acceptThreadCount + m_ioThreadCount)
        {
            m_initCond.Wait(&m_lock);
        }
    }

    return (true);

EXIT:

    StopMe();

    return (false);
}}

void
CProTpReactorTask::Stop()
{{
    CProThreadMutexGuard mon(m_lockAtom);

    StopMe();
}}

void
CProTpReactorTask::StopMe()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0)
        {
            return;
        }

        assert(m_threadIds.find(ProGetThreadId()) == m_threadIds.end()); /* deadlock */

        m_wantExit = true;

        if (m_acceptReactor != NULL)
        {
            m_acceptReactor->Fini();
        }

        int       i = 0;
        const int c = (int)m_ioReactors.size();

        for (; i < c; ++i)
        {
            m_ioReactors[i]->Fini();
        }
    }

    WaitAll();
    m_timerFactory.Stop();
    m_mmTimerFactory.Stop();

    {
        CProThreadMutexGuard mon(m_lock);

        delete m_acceptReactor;

        int       i = 0;
        const int c = (int)m_ioReactors.size();

        for (; i < c; ++i)
        {
            delete m_ioReactors[i];
        }

        m_ioReactors.clear();

        m_acceptReactor     = NULL;
        m_acceptThreadCount = 0;
        m_ioThreadCount     = 0;
        m_ioThreadPriority  = 0;
        m_curThreadCount    = 0;
        m_wantExit          = false;
    }
}

bool
CProTpReactorTask::AddHandler(int64_t           sockId,
                              CProEventHandler* handler,
                              unsigned long     mask)
{
    mask &= (PRO_MASK_ACCEPT | PRO_MASK_CONNECT |
        PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);

    assert(sockId != -1);
    assert(handler != NULL);
    assert(mask != 0);
    if (sockId == -1 || handler == NULL || mask == 0)
    {
        return (false);
    }

    if (PRO_BIT_ENABLED(mask, PRO_MASK_ACCEPT))
    {
        assert(mask == PRO_MASK_ACCEPT);
        if (mask != PRO_MASK_ACCEPT)
        {
            return (false);
        }
    }

    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return (false);
        }

        CProBaseReactor* ioReactor = handler->GetReactor();
        if (ioReactor == NULL)
        {
            int       i = 0;
            const int c = (int)m_ioReactors.size();

            for (; i < c; ++i)
            {
                if (ioReactor == NULL)
                {
                    ioReactor = m_ioReactors[i];
                    continue;
                }

                if (m_ioReactors[i]->GetHandlerCount() < ioReactor->GetHandlerCount())
                {
                    ioReactor = m_ioReactors[i];
                }
            }
        }

        if (PRO_BIT_ENABLED(mask, PRO_MASK_ACCEPT))
        {
            ret = m_acceptReactor->AddHandler(sockId, handler, PRO_MASK_ACCEPT);
            if (!ret)
            {
                mask = 0;
            }
        }
        else
        {
            ret = ioReactor->AddHandler(sockId, handler, mask);
            if (ret)
            {
                handler->SetReactor(ioReactor);
            }
            else
            {
                mask = 0;
            }
        }

        handler->AddMask(mask);
    }

    return (ret);
}

void
CProTpReactorTask::RemoveHandler(int64_t           sockId,
                                 CProEventHandler* handler,
                                 unsigned long     mask)
{
    mask &= (PRO_MASK_ACCEPT | PRO_MASK_CONNECT |
        PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);

    if (sockId == -1 || handler == NULL || mask == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0 ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount)
        {
            return;
        }

        if (PRO_BIT_ENABLED(mask, PRO_MASK_ACCEPT))
        {
            m_acceptReactor->RemoveHandler(sockId, PRO_MASK_ACCEPT);
            handler->RemoveMask(PRO_MASK_ACCEPT);
        }

        if (PRO_BIT_ENABLED(mask, ~PRO_MASK_ACCEPT))
        {
            unsigned long ioMask = mask & ~PRO_MASK_ACCEPT;

            CProBaseReactor* const ioReactor = handler->GetReactor();
            if (ioReactor != NULL)
            {
                ioReactor->RemoveHandler(sockId, ioMask);
            }

            handler->RemoveMask(ioMask);

            ioMask = handler->GetMask() & ~PRO_MASK_ACCEPT;
            if (ioMask == 0)
            {
                handler->SetReactor(NULL);
            }
        }
    }
}

uint64_t
CProTpReactorTask::ScheduleTimer(IProOnTimer* onTimer,
                                 uint64_t     timeSpan,
                                 bool         recurring,
                                 int64_t      userData) /* = 0 */
{
    uint64_t timerId = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return (0);
        }

        timerId = m_timerFactory.ScheduleTimer(onTimer, timeSpan, recurring, userData);
    }

    return (timerId);
}

uint64_t
CProTpReactorTask::ScheduleHeartbeatTimer(IProOnTimer* onTimer,
                                          int64_t      userData) /* = 0 */
{
    uint64_t timerId = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return (0);
        }

        timerId = m_timerFactory.ScheduleHeartbeatTimer(onTimer, userData);
    }

    return (timerId);
}

bool
CProTpReactorTask::UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds)
{
    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return (false);
        }

        ret = m_timerFactory.UpdateHeartbeatTimers(htbtIntervalInSeconds);
    }

    return (ret);
}

void
CProTpReactorTask::CancelTimer(uint64_t timerId)
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0 ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount)
        {
            return;
        }

        m_timerFactory.CancelTimer(timerId);
    }
}

uint64_t
CProTpReactorTask::ScheduleMmTimer(IProOnTimer* onTimer,
                                   uint64_t     timeSpan,
                                   bool         recurring,
                                   int64_t      userData) /* = 0 */
{
    uint64_t timerId = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return (0);
        }

        timerId = m_mmTimerFactory.ScheduleTimer(onTimer, timeSpan, recurring, userData);
    }

    return (timerId);
}

void
CProTpReactorTask::CancelMmTimer(uint64_t timerId)
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0 ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount)
        {
            return;
        }

        m_mmTimerFactory.CancelTimer(timerId);
    }
}

void
CProTpReactorTask::GetTraceInfo(char*  buf,
                                size_t size) const
{
    assert(buf != NULL);
    assert(size > 0);
    if (buf == NULL || size == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_acceptThreadCount + m_ioThreadCount == 0                ||
            m_curThreadCount != m_acceptThreadCount + m_ioThreadCount ||
            m_wantExit)
        {
            return;
        }

        CProStlString theInfo;
        char          theBuf[256] = "";
        int           theValue    = 0;

        for (int i = 0; i < (int)m_ioThreadCount; ++i)
        {
            theValue += (int)m_ioReactors[i]->GetHandlerCount();
            --theValue; /* exclude the signal socket */
        }

        sprintf(
            theBuf,
            " [I/O Threads] : %d \n"
            " [I/O Sockets] : %d \n"
            ,
            (int)m_ioThreadCount,
            theValue
            );
        theInfo += theBuf;

        sprintf(theBuf, " %d ", (int)m_ioReactors[0]->GetHandlerCount() - 1);
        theInfo += theBuf;

        for (int j = 1; j < (int)m_ioThreadCount; ++j)
        {
            sprintf(theBuf, "+ %d ", (int)m_ioReactors[j]->GetHandlerCount() - 1);
            theInfo += theBuf;
        }

        theInfo += "\n";

        theValue = (int)m_timerFactory.GetTimerCount();
        sprintf(theBuf, " [ ST Timers ] : %d \n", theValue);
        theInfo += theBuf;

        theValue = (int)m_mmTimerFactory.GetTimerCount();
        sprintf(theBuf, " [ MM Timers ] : %d \n", theValue);
        theInfo += theBuf;

        theValue = (int)m_timerFactory.GetHeartbeatInterval();
        sprintf(theBuf, " [ HTBT Time ] : %d ", theValue);
        theInfo += theBuf;

        strncpy_pro(buf, size, theInfo.c_str());
    }
}

void
CProTpReactorTask::Svc()
{
    const uint64_t threadId = ProGetThreadId();

    unsigned long threadCount = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        threadCount = ++m_curThreadCount;
        m_threadIds.insert(threadId);
        m_initCond.Signal();
    }

    if (threadCount <= m_acceptThreadCount)
    {
        m_acceptReactor->WorkerRun();
    }
    else
    {
#if defined(_WIN32)
        ::SetThreadPriority(::GetCurrentThread(), m_ioThreadPriority);
#endif

        m_ioReactors[threadCount - 2]->WorkerRun();
    }

    {
        CProThreadMutexGuard mon(m_lock);

        m_threadIds.erase(threadId);
    }
}
