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

#if !defined(____PRO_THREAD_MUTEX_H____)
#define ____PRO_THREAD_MUTEX_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_z.h"

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

    void* GetNativePthreadObj();

protected:

    CProThreadMutex(int);

private:

    CProThreadMutexImpl* m_impl;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProNullMutex : public CProThreadMutex
{
public:

    CProNullMutex();

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32)

class CProRecursiveThreadMutex
{
public:

    CProRecursiveThreadMutex()
    {
    }

    void Lock();

    void Unlock();

    void* GetNativePthreadObj();

private:

    CProThreadMutex m_mutex;

    DECLARE_SGI_POOL(0)
};

#else  /* _WIN32 */

class CProRecursiveThreadMutex
{
public:

    CProRecursiveThreadMutex();

    ~CProRecursiveThreadMutex();

    void Lock();

    void Unlock();

    void* GetNativePthreadObj();

private:

    uint64_t                  m_ownerThreadId;
    size_t                    m_ownerNestingLevel;
    size_t                    m_waiters;
    CProThreadMutexCondition* m_cond;
    CProThreadMutex*          m_mutex;

    DECLARE_SGI_POOL(0)
};

#endif /* _WIN32 */

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

    size_t                    m_readers;
    CProThreadMutex*          m_lock;
    CProThreadMutexCondition* m_cond;
    CProThreadMutex*          m_mutex;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProThreadMutexGuard
{
public:

    CProThreadMutexGuard(CProThreadMutex& mutex);

    CProThreadMutexGuard(CProRecursiveThreadMutex& mutex);

    CProThreadMutexGuard(
        CProRwThreadMutex& mutex,
        bool               readonly = false
        );

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

    /*
     * On Linux, the performence in socket-mode is higher than that in pthread-mode.
     */
    CProThreadMutexCondition(bool isSocketMode = false);

    ~CProThreadMutexCondition();

    /*
     * The caller should hold the mutex!!!
     */
    bool Wait(
        CProThreadMutex* mutex,
        unsigned int     milliseconds = 0xFFFFFFFF
        );

    /*
     * The caller should hold the mutex!!!
     */
    bool Wait_rc(
        CProRecursiveThreadMutex* mutex,
        unsigned int              milliseconds = 0xFFFFFFFF
        );

    /*
     * The caller should hold the mutex!!!
     *
     * The object will be holding the signal, and this is different from the POSIX standard!!!
     */
    void Signal();

private:

    CProThreadMutexConditionImpl* m_impl;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_THREAD_MUTEX_H____ */
