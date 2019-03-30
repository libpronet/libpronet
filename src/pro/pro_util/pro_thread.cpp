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

#include "pro_a.h"
#include "pro_thread.h"
#include "pro_bsd_wrapper.h" /* for <unistd.h> */
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_time_util.h"
#include "pro_z.h"
#include "../pro_shared/pro_shared.h"

#if defined(_WIN32_WCE)
#include <windows.h>
#elif defined(WIN32)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(STACK_SIZE_PARAM_IS_A_RESERVATION)
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif

#if !defined(PRO_THREAD_STACK_SIZE)
#define PRO_THREAD_STACK_SIZE             (1024 * 1024 - 8192)
#endif

/////////////////////////////////////////////////////////////////////////////
////

CProThreadBase::CProThreadBase()
{
    m_threadCount = 0;
}

bool
CProThreadBase::Spawn(bool realtime)
{
    {
        CProThreadMutexGuard mon(m_lock);

#if defined(_WIN32_WCE)

        const HANDLE threadHandle = ::CreateThread(
            NULL, PRO_THREAD_STACK_SIZE, &CProThreadBase::SvcRun, this,
            CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        if (threadHandle == NULL)
        {
            return (false);
        }

        if (realtime)
        {
            ::SetThreadPriority(threadHandle, THREAD_PRIORITY_TIME_CRITICAL);
        }

        ::ResumeThread(threadHandle);
        ::CloseHandle(threadHandle);

#elif defined(WIN32)

        const HANDLE threadHandle = (HANDLE)::_beginthreadex(
            NULL, PRO_THREAD_STACK_SIZE, &CProThreadBase::SvcRun, this,
            CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
        if (threadHandle == NULL)
        {
            return (false);
        }

        if (realtime)
        {
            ::SetThreadPriority(threadHandle, THREAD_PRIORITY_TIME_CRITICAL);
        }

        ::ResumeThread(threadHandle);
        ::CloseHandle(threadHandle);

#else

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, PRO_THREAD_STACK_SIZE);

#if defined(PRO_HAS_PTHREAD_EXPLICIT_SCHED)
        if (realtime)
        {
            struct sched_param sp;
            memset(&sp, 0, sizeof(struct sched_param));
            sp.sched_priority = sched_get_priority_max(SCHED_RR);

            pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy(&attr, SCHED_RR);
            pthread_attr_setschedparam(&attr, &sp);
        }
#endif

        pthread_t threadId = 0;
        const int retc = pthread_create(&threadId, &attr, &CProThreadBase::SvcRun, this);

        pthread_attr_destroy(&attr);

        if (retc != 0)
        {
            return (false);
        }

        m_threadId2Realtime[threadId] = realtime;

#endif

        ++m_threadCount;
    }

    return (true);
}

unsigned long
CProThreadBase::GetThreadCount() const
{
    unsigned long count = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        count = m_threadCount;
    }

    return (count);
}

void
CProThreadBase::Wait1()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_threadCount > 0)
        {
            m_cond.Wait(&m_lock);
        }
    }
}

void
CProThreadBase::WaitAll()
{
    {
        CProThreadMutexGuard mon(m_lock);

        while (m_threadCount > 0)
        {
            m_cond.Wait(&m_lock);
        }
    }
}

#if defined(_WIN32_WCE)
unsigned long
__stdcall
CProThreadBase::SvcRun(void* arg)
#elif defined(WIN32)
unsigned int
__stdcall
CProThreadBase::SvcRun(void* arg)
#else
void*
CProThreadBase::SvcRun(void* arg)
#endif
{
    CProThreadBase* const threadObj = (CProThreadBase*)arg;

    {
        CProThreadMutexGuard mon(threadObj->m_lock);
    }

    ProSrand(); /* TLS */

#if !defined(WIN32) && !defined(_WIN32_WCE)

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGTSTP);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGVTALRM);
    sigaddset(&mask, SIGPROF);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    const PRO_UINT64 threadId = ProGetThreadId();

    {
        CProThreadMutexGuard mon(threadObj->m_lock);

        if (threadObj->m_threadId2Realtime[threadId])
        {
            struct sched_param sp;
            memset(&sp, 0, sizeof(struct sched_param));
            sp.sched_priority = sched_get_priority_max(SCHED_RR);

            pthread_setschedparam((pthread_t)threadId, SCHED_RR, &sp);
        }

        threadObj->m_threadId2Realtime.erase(threadId);
    }

#endif /* WIN32, _WIN32_WCE */

    threadObj->Svc();

    {
        CProThreadMutexGuard mon(threadObj->m_lock);

        --threadObj->m_threadCount;
        threadObj->m_cond.Signal();
    }

#if !defined(WIN32) && !defined(_WIN32_WCE)
    pthread_detach(threadId);
#endif

    return (0);
}

/////////////////////////////////////////////////////////////////////////////
////

PRO_UINT64
PRO_CALLTYPE
ProGetThreadId()
{
#if defined(WIN32) || defined(_WIN32_WCE)
    const PRO_UINT64 tid = (PRO_UINT64)::GetCurrentThreadId();
#else
    const PRO_UINT64 tid = (PRO_UINT64)pthread_self();
#endif

    return (tid);
}

PRO_UINT64
PRO_CALLTYPE
ProGetProcessId()
{
#if defined(WIN32) || defined(_WIN32_WCE)
    const PRO_UINT64 pid = (PRO_UINT64)::GetCurrentProcessId();
#else
    const PRO_UINT64 pid = (PRO_UINT64)getpid();
#endif

    return (pid);
}
