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
#include "pro_ref_count.h"
#include "pro_memory_pool.h"
#include "pro_thread_mutex.h"

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

unsigned long
PRO_CALLTYPE
CProRefCount::AddRef()
{
#if defined(_WIN32) || defined(_WIN32_WCE)
    const unsigned long refCount = ::InterlockedIncrement((long*)&m_refCount);
#elif defined(PRO_HAS_ATOMOP)
    const unsigned long refCount = __sync_add_and_fetch(&m_refCount, 1);
#else
    m_lock.Lock();
    const unsigned long refCount = ++m_refCount;
    m_lock.Unlock();
#endif

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CProRefCount::Release()
{
#if defined(_WIN32) || defined(_WIN32_WCE)
    const unsigned long refCount = ::InterlockedDecrement((long*)&m_refCount);
#elif defined(PRO_HAS_ATOMOP)
    const unsigned long refCount = __sync_sub_and_fetch(&m_refCount, 1);
#else
    m_lock.Lock();
    const unsigned long refCount = --m_refCount;
    m_lock.Unlock();
#endif

    if (refCount == 0)
    {
        delete this;
    }

    return (refCount);
}
