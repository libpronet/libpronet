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

#if !defined(____PRO_THREAD_MUTEX_H____)
#define ____PRO_THREAD_MUTEX_H____

#include "pro_a.h"
#include "pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProThreadMutexCondition;
class CProThreadMutexConditionImpl;
class CProThreadMutexImpl;

/////////////////////////////////////////////////////////////////////////////
////

class CProThreadMutex
{
public:

    CProThreadMutex();

    ~CProThreadMutex();

    void Lock();

    void Unlock();

protected:

    CProThreadMutex(int);

private:

    CProThreadMutexImpl* m_impl;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CProNullMutex : public CProThreadMutex
{
public:

    CProNullMutex();
};

/////////////////////////////////////////////////////////////////////////////
////

#if defined(WIN32) || defined(_WIN32_WCE)

class CProRecursiveThreadMutex : public CProThreadMutex
{
};

#else  /* WIN32, _WIN32_WCE */

class CProRecursiveThreadMutex
{
public:

    CProRecursiveThreadMutex();

    ~CProRecursiveThreadMutex();

    void Lock();

    void Unlock();

private:

    PRO_UINT64                m_ownerThreadId;
    unsigned long             m_ownerNestingLevel;
    unsigned long             m_waiters;
    CProThreadMutexCondition* m_cond;
    CProThreadMutex*          m_mutex;

    DECLARE_SGI_POOL(0);
};

#endif /* WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

class CProRwThreadMutex
{
public:

    CProRwThreadMutex();

    ~CProRwThreadMutex();

    void Lock();

    void Unlock();

    void Lock_r();

    void Unlock_r();

private:

    unsigned long             m_readers;
    CProThreadMutex*          m_lock;
    CProThreadMutexCondition* m_cond;
    CProThreadMutex*          m_mutex;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CProThreadMutexGuard
{
public:

    CProThreadMutexGuard(CProThreadMutex& mutex);

    CProThreadMutexGuard(CProRecursiveThreadMutex& rcmutex);

    CProThreadMutexGuard(CProRwThreadMutex& rwmutex, bool readonly);

    ~CProThreadMutexGuard();

private:

    CProThreadMutex*          m_mutex;
    CProRecursiveThreadMutex* m_rcmutex;
    CProRwThreadMutex*        m_rwmutex;
    bool                      m_readonly;
};

/////////////////////////////////////////////////////////////////////////////
////

class CProThreadMutexCondition
{
public:

    CProThreadMutexCondition();

    ~CProThreadMutexCondition();

    /*
     * the "mutex" can be NULL
     */
    void Wait(CProThreadMutex* mutex);

    /*
     * the "rcmutex" can be NULL
     */
    void Waitrc(CProRecursiveThreadMutex* rcmutex);

    void Signal();

private:

    CProThreadMutexConditionImpl* m_impl;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_THREAD_MUTEX_H____ */
