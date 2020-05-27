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

#if !defined(____PRO_REF_COUNT_H____)
#define ____PRO_REF_COUNT_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProRefCount
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

protected:

    CProRefCount()
    {
        m_refCount = 1;
    }

    virtual ~CProRefCount()
    {
    }

private:

#if defined(_WIN32) || defined(_WIN32_WCE) || defined(PRO_HAS_ATOMOP)
    volatile unsigned long m_refCount;
#else
    unsigned long          m_refCount;
    CProThreadMutex        m_lock;
#endif

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_REF_COUNT_H____ */
