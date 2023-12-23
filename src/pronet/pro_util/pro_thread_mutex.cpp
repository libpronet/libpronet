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
#include "pro_thread_mutex.h"
#include "pro_bsd_wrapper.h"
#include "pro_memory_pool.h"
#include "pro_notify_pipe.h"
#include "pro_thread.h"
#include "pro_time_util.h"
#include "pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(_WIN32) && !defined(PRO_HAS_PTHREAD_CONDATTR_SETCLOCK)

static
void
GetTimespec_i(struct timespec& ts,
              int              deltaMilliseconds) /* = 0 */
{
    memset(&ts, 0, sizeof(struct timespec));

#if !defined(PRO_LACKS_CLOCK_GETTIME)

    struct timespec now = { 0 };
    clock_gettime(CLOCK_REALTIME, &now);

    int64_t tt = (int64_t)now.tv_sec * 1000000000 + now.tv_nsec +
        (int64_t)deltaMilliseconds * 1000000;

#else  /* PRO_LACKS_CLOCK_GETTIME */

    struct timeval now = { 0 };
    if (gettimeofday(&now, NULL) != 0)
    {
        return;
    }

    int64_t tt = (int64_t)now.tv_sec * 1000000000 + (int64_t)now.tv_usec * 1000 +
        (int64_t)deltaMilliseconds * 1000000;

#endif /* PRO_LACKS_CLOCK_GETTIME */

    time_t seconds     = (time_t)(tt / 1000000000);
    time_t nanoseconds = (time_t)(tt % 1000000000);
    if (seconds <= 0) /* overflow */
    {
        return;
    }

    ts.tv_sec  = (long)seconds;
    ts.tv_nsec = (long)nanoseconds;
}

#endif /* _WIN32, PRO_HAS_PTHREAD_CONDATTR_SETCLOCK */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32)

class CProThreadMutexImpl
{
public:

    CProThreadMutexImpl()
    {
        ::InitializeCriticalSection(&m_cs);
    }

    ~CProThreadMutexImpl()
    {
        ::DeleteCriticalSection(&m_cs);
    }

    void Lock()
    {
        ::EnterCriticalSection(&m_cs);
    }

    void Unlock()
    {
        ::LeaveCriticalSection(&m_cs);
    }

    void* GetNativePthreadObj()
    {
        return NULL;
    }

private:

    CRITICAL_SECTION m_cs;

    DECLARE_SGI_POOL(0)
};

class CProThreadMutexConditionImpl
{
public:

    CProThreadMutexConditionImpl(bool isSocketMode) /* = false */
    {
        m_sem = ::CreateSemaphore(NULL, 0, 1, NULL);
    }

    ~CProThreadMutexConditionImpl()
    {
        if (m_sem != NULL)
        {
            ::CloseHandle(m_sem);
            m_sem = NULL;
        }
    }

    bool Wait(
        CProThreadMutex* mutex,
        unsigned int     milliseconds /* = 0xFFFFFFFF */
        )
    {
        assert(mutex != NULL);

        mutex->Unlock();
        unsigned long retc = ::WaitForSingleObject(m_sem, milliseconds);
        mutex->Lock();

        return retc == WAIT_OBJECT_0;
    }

    bool Wait_rc(
        CProRecursiveThreadMutex* mutex,
        unsigned int              milliseconds /* = 0xFFFFFFFF */
        )
    {
        assert(mutex != NULL);

        mutex->Unlock();
        unsigned long retc = ::WaitForSingleObject(m_sem, milliseconds);
        mutex->Lock();

        return retc == WAIT_OBJECT_0;
    }

    void Signal()
    {
        ::ReleaseSemaphore(m_sem, 1, NULL);
    }

private:

    HANDLE m_sem;

    DECLARE_SGI_POOL(0)
};

#else  /* _WIN32 */

class CProThreadMutexImpl
{
    friend class CProThreadMutexConditionImpl;

public:

    CProThreadMutexImpl()
    {
        pthread_mutex_init(&m_mutext, NULL);
    }

    ~CProThreadMutexImpl()
    {
        pthread_mutex_destroy(&m_mutext);
    }

    void Lock()
    {
        pthread_mutex_lock(&m_mutext);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&m_mutext);
    }

    void* GetNativePthreadObj()
    {
        return &m_mutext;
    }

private:

    pthread_mutex_t m_mutext;

    DECLARE_SGI_POOL(0)
};

class CProThreadMutexConditionImpl
{
public:

    CProThreadMutexConditionImpl(bool isSocketMode) /* = false */
    : m_isSocketMode(isSocketMode)
    {
        m_signal  = false;
        m_waiters = 0;

        if (isSocketMode)
        {
            m_pipe.Init();
        }
        else
        {
#if defined(PRO_HAS_PTHREAD_CONDATTR_SETCLOCK)
            pthread_condattr_t attr;
            pthread_condattr_init(&attr);
            pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
            pthread_cond_init(&m_condt, &attr);
            pthread_condattr_destroy(&attr);
#else
            pthread_cond_init(&m_condt, NULL);
#endif
        }
    }

    ~CProThreadMutexConditionImpl()
    {
        if (m_isSocketMode)
        {
            m_pipe.Fini();
        }
        else
        {
            pthread_cond_destroy(&m_condt);
        }
    }

    bool Wait(
        CProThreadMutex* mutex,
        unsigned int     milliseconds /* = 0xFFFFFFFF */
        )
    {
        bool ret = false;

        if (m_isSocketMode)
        {
            ret = WaitInSocketMode(mutex, false, milliseconds);  /* recursive is false */
        }
        else
        {
            ret = WaitInPthreadMode(mutex, false, milliseconds); /* recursive is false */
        }

        return ret;
    }

    bool Wait_rc(
        CProRecursiveThreadMutex* mutex,
        unsigned int              milliseconds /* = 0xFFFFFFFF */
        )
    {
        bool ret = false;

        if (m_isSocketMode)
        {
            ret = WaitInSocketMode(mutex, true, milliseconds);  /* recursive is true */
        }
        else
        {
            ret = WaitInPthreadMode(mutex, true, milliseconds); /* recursive is true */
        }

        return ret;
    }

    void Signal()
    {
        m_signal = true;
        if (m_waiters == 0)
        {
            return;
        }

        if (m_isSocketMode)
        {
            m_pipe.Notify();
        }
        else
        {
            pthread_cond_signal(&m_condt);
        }
    }

private:

    bool WaitInSocketMode(
        void*        mutex,
        bool         recursive,
        unsigned int milliseconds /* = 0xFFFFFFFF */
        )
    {
        assert(mutex != NULL);

        if (m_signal)
        {
            m_signal = false;

            return true;
        }

        bool ret = true;

        while (!m_signal)
        {
            ++m_waiters;
            WaitSocket(mutex, recursive, milliseconds);
            --m_waiters;

            if (milliseconds != 0xFFFFFFFF)
            {
                ret = m_signal;
                break;
            }
        }

        m_signal = false;

        return ret;
    }

    void WaitSocket(
        void*        mutex,
        bool         recursive,
        unsigned int milliseconds /* = 0xFFFFFFFF */
        )
    {
        assert(mutex != NULL);

        if (milliseconds == 0)
        {
            milliseconds = 1;
        }

        int64_t sockId = m_pipe.GetReaderSockId();
        if (sockId == -1)
        {
            m_pipe.Fini();
            m_pipe.Init();

            Unlock(mutex, recursive);
            ProSleep(1);
            Lock(mutex, recursive);

            return;
        }

        pbsd_pollfd pfd;
        memset(&pfd, 0, sizeof(pbsd_pollfd));
        pfd.fd     = (int)sockId;
        pfd.events = POLLIN;

        Unlock(mutex, recursive);
        int retc = pbsd_poll(&pfd, 1, milliseconds);
        Lock(mutex, recursive);

        if (retc == 0)
        {
            return;
        }

        if (retc < 0 || !m_pipe.Roger())
        {
            m_pipe.Fini();
            m_pipe.Init();

            Unlock(mutex, recursive);
            ProSleep(1);
            Lock(mutex, recursive);
        }
    }

    bool WaitInPthreadMode(
        void*        mutex,
        bool         recursive,
        unsigned int milliseconds /* = 0xFFFFFFFF */
        )
    {
        assert(mutex != NULL);

        if (milliseconds != 0xFFFFFFFF && milliseconds > 0x7FFFFFFF)
        {
            milliseconds = 0x7FFFFFFF;
        }

        if (m_signal)
        {
            m_signal = false;

            return true;
        }

        pthread_mutex_t* mutext = NULL;
        if (recursive)
        {
            CProRecursiveThreadMutex* mutex2 = (CProRecursiveThreadMutex*)mutex;
            mutext = (pthread_mutex_t*)mutex2->GetNativePthreadObj();
        }
        else
        {
            CProThreadMutex* mutex2 = (CProThreadMutex*)mutex;
            mutext = (pthread_mutex_t*)mutex2->GetNativePthreadObj();
        }
        if (mutext == NULL)
        {
            return false;
        }

        struct timespec abstime = { 0 };

#if defined(PRO_HAS_PTHREAD_CONDATTR_SETCLOCK)
        int64_t tt = ProGetTickCount64() + milliseconds;

        abstime.tv_sec  = (time_t)(tt / 1000);
        abstime.tv_nsec = (long)  (tt % 1000 * 1000000);
#else
        GetTimespec_i(abstime, milliseconds);
#endif

        bool ret = true;

        while (!m_signal)
        {
            if (milliseconds == 0xFFFFFFFF)
            {
                ++m_waiters;
                pthread_cond_wait(&m_condt, mutext);
                --m_waiters;
            }
            else
            {
                ++m_waiters;
                int retc = pthread_cond_timedwait(&m_condt, mutext, &abstime);
                --m_waiters;

                if (retc != 0)
                {
                    ret = m_signal;
                    break;
                }
            }
        }

        m_signal = false;

        return ret;
    }

    void Lock(
        void* mutex,
        bool  recursive
        )
    {
        assert(mutex != NULL);

        if (recursive)
        {
            CProRecursiveThreadMutex* mutex2 = (CProRecursiveThreadMutex*)mutex;
            mutex2->Lock();
        }
        else
        {
            CProThreadMutex* mutex2 = (CProThreadMutex*)mutex;
            mutex2->Lock();
        }
    }

    void Unlock(
        void* mutex,
        bool  recursive
        )
    {
        assert(mutex != NULL);

        if (recursive)
        {
            CProRecursiveThreadMutex* mutex2 = (CProRecursiveThreadMutex*)mutex;
            mutex2->Unlock();
        }
        else
        {
            CProThreadMutex* mutex2 = (CProThreadMutex*)mutex;
            mutex2->Unlock();
        }
    }

private:

    const bool     m_isSocketMode;
    bool           m_signal;
    size_t         m_waiters;
    CProNotifyPipe m_pipe;
    pthread_cond_t m_condt;

    DECLARE_SGI_POOL(0)
};

#endif /* _WIN32 */

/////////////////////////////////////////////////////////////////////////////
////

CProThreadMutex::CProThreadMutex()
{
    m_impl = new CProThreadMutexImpl;
}

CProThreadMutex::CProThreadMutex(int)
{
    m_impl = NULL;
}

CProThreadMutex::~CProThreadMutex()
{
    delete m_impl;
    m_impl = NULL;
}

void
CProThreadMutex::Lock()
{
    if (m_impl != NULL)
    {
        m_impl->Lock();
    }
}

void
CProThreadMutex::Unlock()
{
    if (m_impl != NULL)
    {
        m_impl->Unlock();
    }
}

void*
CProThreadMutex::GetNativePthreadObj()
{
    if (m_impl == NULL)
    {
        return NULL;
    }
    else
    {
        return m_impl->GetNativePthreadObj();
    }
}

/////////////////////////////////////////////////////////////////////////////
////

CProNullMutex::CProNullMutex() : CProThreadMutex(0)
{
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32)

void
CProRecursiveThreadMutex::Lock()
{
    m_mutex.Lock();
}

void
CProRecursiveThreadMutex::Unlock()
{
    m_mutex.Unlock();
}

void*
CProRecursiveThreadMutex::GetNativePthreadObj()
{
    return m_mutex.GetNativePthreadObj();
}

#else  /* _WIN32 */

CProRecursiveThreadMutex::CProRecursiveThreadMutex()
{
    m_ownerThreadId     = 0;
    m_ownerNestingLevel = 0;
    m_waiters           = 0;
    m_cond              = new CProThreadMutexCondition;
    m_mutex             = new CProThreadMutex;
}

CProRecursiveThreadMutex::~CProRecursiveThreadMutex()
{
    delete m_cond;
    delete m_mutex;
    m_cond  = NULL;
    m_mutex = NULL;
}

void
CProRecursiveThreadMutex::Lock()
{
    uint64_t threadId = ProGetThreadId();

    m_mutex->Lock();   /* [[[[ */

    while (m_ownerNestingLevel > 0 && m_ownerThreadId != threadId)
    {
        ++m_waiters;
        m_cond->Wait(m_mutex);
        --m_waiters;
    }

    m_ownerThreadId = threadId;
    ++m_ownerNestingLevel;

    m_mutex->Unlock(); /* ]]]] */
}

void
CProRecursiveThreadMutex::Unlock()
{
    uint64_t threadId = ProGetThreadId();

    m_mutex->Lock();   /* [[[[ */

    if (m_ownerNestingLevel > 0 && m_ownerThreadId == threadId)
    {
        --m_ownerNestingLevel;
        if (m_ownerNestingLevel == 0)
        {
            m_ownerThreadId = 0;
            if (m_waiters > 0)
            {
                m_cond->Signal();
            }
        }
    }

    m_mutex->Unlock(); /* ]]]] */
}

void*
CProRecursiveThreadMutex::GetNativePthreadObj()
{
    return m_mutex->GetNativePthreadObj();
}

#endif /* _WIN32 */

/////////////////////////////////////////////////////////////////////////////
////

CProRwThreadMutex::CProRwThreadMutex()
{
    m_readers = 0;
    m_lock    = new CProThreadMutex;
    m_cond    = new CProThreadMutexCondition;
    m_mutex   = new CProThreadMutex;
}

CProRwThreadMutex::~CProRwThreadMutex()
{
    delete m_lock;
    delete m_cond;
    delete m_mutex;
    m_lock  = NULL;
    m_cond  = NULL;
    m_mutex = NULL;
}

void
CProRwThreadMutex::Lock()
{
    m_mutex->Lock();   /* [[[[ */

    m_lock->Lock();
    while (m_readers > 0)
    {
        m_cond->Wait(m_lock);
    }
    m_lock->Unlock();
}

void
CProRwThreadMutex::Unlock()
{
    m_mutex->Unlock(); /* ]]]] */
}

void
CProRwThreadMutex::Lock_r()
{
    m_mutex->Lock();   /* [[[[ */

    m_lock->Lock();
    ++m_readers;
    m_lock->Unlock();

    m_mutex->Unlock(); /* ]]]] */
}

void
CProRwThreadMutex::Unlock_r()
{
    m_lock->Lock();
    if (--m_readers == 0)
    {
        m_cond->Signal();
    }
    m_lock->Unlock();
}

/////////////////////////////////////////////////////////////////////////////
////

CProThreadMutexGuard::CProThreadMutexGuard(CProThreadMutex& mutex)
{
    m_mutex    = &mutex;
    m_rcmutex  = NULL;
    m_rwmutex  = NULL;
    m_readonly = false;

    mutex.Lock();
}

CProThreadMutexGuard::CProThreadMutexGuard(CProRecursiveThreadMutex& mutex)
{
    m_mutex    = NULL;
    m_rcmutex  = &mutex;
    m_rwmutex  = NULL;
    m_readonly = false;

    mutex.Lock();
}

CProThreadMutexGuard::CProThreadMutexGuard(CProRwThreadMutex& mutex,
                                           bool               readonly) /* = false */
{
    m_mutex    = NULL;
    m_rcmutex  = NULL;
    m_rwmutex  = &mutex;
    m_readonly = readonly;

    if (readonly)
    {
        mutex.Lock_r();
    }
    else
    {
        mutex.Lock();
    }
}

CProThreadMutexGuard::~CProThreadMutexGuard()
{
    if (m_mutex != NULL)
    {
        m_mutex->Unlock();
    }
    if (m_rcmutex != NULL)
    {
        m_rcmutex->Unlock();
    }
    if (m_rwmutex != NULL)
    {
        if (m_readonly)
        {
            m_rwmutex->Unlock_r();
        }
        else
        {
            m_rwmutex->Unlock();
        }
    }

    m_mutex   = NULL;
    m_rcmutex = NULL;
    m_rwmutex = NULL;
}

/////////////////////////////////////////////////////////////////////////////
////

CProThreadMutexCondition::CProThreadMutexCondition(bool isSocketMode) /* = false */
{
    m_impl = new CProThreadMutexConditionImpl(isSocketMode);
}

CProThreadMutexCondition::~CProThreadMutexCondition()
{
    delete m_impl;
    m_impl = NULL;
}

/*
 * The caller should hold the mutex!!!
 */
bool
CProThreadMutexCondition::Wait(CProThreadMutex* mutex,
                               unsigned int     milliseconds) /* = 0xFFFFFFFF */
{
    return m_impl->Wait(mutex, milliseconds);
}

/*
 * The caller should hold the mutex!!!
 */
bool
CProThreadMutexCondition::Wait_rc(CProRecursiveThreadMutex* mutex,
                                  unsigned int              milliseconds) /* = 0xFFFFFFFF */
{
    return m_impl->Wait_rc(mutex, milliseconds);
}

/*
 * The caller should hold the mutex!!!
 */
void
CProThreadMutexCondition::Signal()
{
    m_impl->Signal();
}
