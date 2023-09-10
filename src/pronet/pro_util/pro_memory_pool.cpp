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
#include "pro_memory_pool.h"
#include "pro_shared.h"
#include "pro_z.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

void*
ProMalloc(size_t size)
{
    if (size == 0)
    {
        size = 1;
    }

    void* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    p = ProAllocateSgiPoolBuffer(size, 0);
#else
    p = malloc(size);
#endif

    return p;
}

void*
ProCalloc(size_t count,
          size_t size)
{
    if (count == 0)
    {
        count = 1;
    }
    if (size == 0)
    {
        size  = 1;
    }

    void* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    p = ProAllocateSgiPoolBuffer(count * size, 0);
    if (p != NULL)
    {
        memset(p, 0, count * size);
    }
#else
    p = calloc(count, size);
#endif

    return p;
}

void*
ProRealloc(void*  p,
           size_t newSize)
{
    if (p == NULL && newSize == 0)
    {
        newSize = 1;
    }

    void* q = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    q = ProReallocateSgiPoolBuffer(p, newSize, 0);
#else
    q = realloc(p, newSize);
#endif

    return q;
}

void
ProFree(void* p)
{
    if (p == NULL)
    {
        return;
    }

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    ProDeallocateSgiPoolBuffer(p, 0);
#else
    free(p);
#endif
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
