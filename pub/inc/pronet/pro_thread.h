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

#ifndef ____PRO_THREAD_H____
#define ____PRO_THREAD_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Instead of using CProThreadBase, you should use CProFunctorCommandTask.
 */
class CProThreadBase
{
public:

    unsigned int GetThreadCount() const;

protected:

    CProThreadBase();

    virtual ~CProThreadBase()
    {
    }

    bool Spawn(bool realtime);

    void Wait1();

    void WaitAll();

    virtual void Svc() = 0;

private:

#if defined(_WIN32)
    static unsigned int __stdcall SvcRun(void* arg);
#else
    static void* SvcRun(void* arg);
#endif

private:

    unsigned int               m_threadCount;
#if !defined(_WIN32)
    CProStlMap<uint64_t, bool> m_threadId2Realtime;
#endif
    CProThreadMutexCondition   m_cond;
    mutable CProThreadMutex    m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

uint64_t
ProGetThreadId();

uint64_t
ProGetProcessId();

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_THREAD_H____ */
