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
#include "pro_memory_pool.h"
#include "pro_thread.h"
#include "pro_z.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#else
#include <pthread.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(WIN32) || defined(_WIN32_WCE)

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

private:

    CRITICAL_SECTION m_cs;

    DECLARE_SGI_POOL(0);
};

class CProThreadMutexConditionImpl
{
public:

    CProThreadMutexConditionImpl()
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

    void Wait(CProThreadMutex* mutex)
    {
        if (mutex != NULL)
        {
            mutex->Unlock();
        }

        ::WaitForSingleObject(m_sem, INFINITE);

        if (mutex != NULL)
        {
            mutex->Lock();
        }
    }

    void Waitrc(CProRecursiveThreadMutex* rcmutex)
    {
        if (rcmutex != NULL)
        {
            rcmutex->Unlock();
        }

        ::WaitForSingleObject(m_sem, INFINITE);

        if (rcmutex != NULL)
        {
            rcmutex->Lock();
        }
    }

    void Signal()
    {
        ::ReleaseSemaphore(m_sem, 1, NULL);
    }

private:

    HANDLE m_sem;

    DECLARE_SGI_POOL(0);
};

#else  /* WIN32, _WIN32_WCE */

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

private:

    pthread_mutex_t m_mutext;

    DECLARE_SGI_POOL(0);
};

class CProThreadMutexConditionImpl
{
public:

    CProThreadMutexConditionImpl()
    {
        m_signal  = false;
        m_waiters = 0;
        pthread_cond_init(&m_condt, NULL);
    }

    ~CProThreadMutexConditionImpl()
    {
        pthread_cond_destroy(&m_condt);
    }

    void Wait(CProThreadMutex* mutex)
    {
        if (mutex != NULL)
        {
            mutex->Unlock();
        }

        m_mutex.Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

        while (!m_signal)
        {
            ++m_waiters;
            pthread_cond_wait(&m_condt, &m_mutex.m_mutext);
            --m_waiters;
        }

        m_signal = false;

        m_mutex.Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */

        if (mutex != NULL)
        {
            mutex->Lock();
        }
    }

    void Waitrc(CProRecursiveThreadMutex* rcmutex)
    {
        if (rcmutex != NULL)
        {
            rcmutex->Unlock();
        }

        m_mutex.Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

        while (!m_signal)
        {
            ++m_waiters;
            pthread_cond_wait(&m_condt, &m_mutex.m_mutext);
            --m_waiters;
        }

        m_signal = false;

        m_mutex.Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */

        if (rcmutex != NULL)
        {
            rcmutex->Lock();
        }
    }

    void Signal()
    {
        m_mutex.Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

        m_signal = true;
        if (m_waiters > 0)
        {
            pthread_cond_signal(&m_condt);
        }

        m_mutex.Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */
    }

private:

    bool                m_signal;
    unsigned long       m_waiters;
    pthread_cond_t      m_condt;
    CProThreadMutexImpl m_mutex;

    DECLARE_SGI_POOL(0);
};

#endif /* WIN32, _WIN32_WCE */

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

/////////////////////////////////////////////////////////////////////////////
////

CProNullMutex::CProNullMutex()
: CProThreadMutex(0)
{
}

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(WIN32) && !defined(_WIN32_WCE)

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
    const PRO_UINT64 threadId = ProGetThreadId();

    m_mutex->Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

    while (m_ownerNestingLevel > 0 && m_ownerThreadId != threadId)
    {
        ++m_waiters;
        m_cond->Wait(m_mutex);
        --m_waiters;
    }

    m_ownerThreadId = threadId;
    ++m_ownerNestingLevel;

    m_mutex->Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */
}

void
CProRecursiveThreadMutex::Unlock()
{
    const PRO_UINT64 threadId = ProGetThreadId();

    m_mutex->Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

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

    m_mutex->Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */
}

#endif /* WIN32, _WIN32_WCE */

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
    m_mutex->Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

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
    m_mutex->Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */
}

void
CProRwThreadMutex::Lock_r()
{
    m_mutex->Lock();   /* [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ */

    m_lock->Lock();
    ++m_readers;
    m_lock->Unlock();

    m_mutex->Unlock(); /* ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]] */
}

void
CProRwThreadMutex::Unlock_r()
{
    m_lock->Lock();
    const unsigned long readers = --m_readers;
    m_lock->Unlock();

    if (readers == 0)
    {
        m_cond->Signal();
    }
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

CProThreadMutexGuard::CProThreadMutexGuard(CProRecursiveThreadMutex& rcmutex)
{
    m_mutex    = NULL;
    m_rcmutex  = &rcmutex;
    m_rwmutex  = NULL;
    m_readonly = false;

    rcmutex.Lock();
}

CProThreadMutexGuard::CProThreadMutexGuard(CProRwThreadMutex& rwmutex,
                                           bool               readonly)
{
    m_mutex    = NULL;
    m_rcmutex  = NULL;
    m_rwmutex  = &rwmutex;
    m_readonly = readonly;

    if (readonly)
    {
        rwmutex.Lock_r();
    }
    else
    {
        rwmutex.Lock();
    }
}

CProThreadMutexGuard::~CProThreadMutexGuard()
{
    if (m_mutex != NULL)
    {
        m_mutex->Unlock();
    }
    else if (m_rcmutex != NULL)
    {
        m_rcmutex->Unlock();
    }
    else if (m_rwmutex != NULL)
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
    else
    {
    }

    m_mutex   = NULL;
    m_rcmutex = NULL;
    m_rwmutex = NULL;
}

/////////////////////////////////////////////////////////////////////////////
////

CProThreadMutexCondition::CProThreadMutexCondition()
{
    m_impl = new CProThreadMutexConditionImpl;
}

CProThreadMutexCondition::~CProThreadMutexCondition()
{
    delete m_impl;
    m_impl = NULL;
}

void
CProThreadMutexCondition::Wait(CProThreadMutex* mutex)
{
    m_impl->Wait(mutex);
}

void
CProThreadMutexCondition::Waitrc(CProRecursiveThreadMutex* rcmutex)
{
    m_impl->Waitrc(rcmutex);
}

void
CProThreadMutexCondition::Signal()
{
    m_impl->Signal();
}
