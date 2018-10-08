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
#include "pro_memory_pool.h"
#include "pro_z.h"
#include "../pro_shared/pro_shared.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include <cstddef>
#include <memory>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

void*
PRO_CALLTYPE
ProMalloc(size_t size)
{
    if (size == 0)
    {
        size = 1;
    }

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    void* const p = ProAllocateSgiPoolBuffer(size, 0);
#else
    void* const p = malloc(size);
#endif

    return (p);
}

void*
PRO_CALLTYPE
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

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    void* const p = ProAllocateSgiPoolBuffer(count * size, 0);
    if (p != NULL)
    {
        memset(p, 0, count * size);
    }
#else
    void* const p = calloc(count, size);
#endif

    return (p);
}

void*
PRO_CALLTYPE
ProRealloc(void*  p,
           size_t newSize)
{
    if (p == NULL && newSize == 0)
    {
        newSize = 1;
    }

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_MALLOC)
    void* const q = ProReallocateSgiPoolBuffer(p, newSize, 0);
#else
    void* const q = realloc(p, newSize);
#endif

    return (q);
}

void
PRO_CALLTYPE
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

void*
PRO_CALLTYPE
ProPoolMalloc(size_t        size,
              unsigned long poolIndex)  /* 0 ~ 9 */
{
    return (ProAllocateSgiPoolBuffer(size, poolIndex));
}

void*
PRO_CALLTYPE
ProPoolCalloc(size_t        count,
              size_t        size,
              unsigned long poolIndex)  /* 0 ~ 9 */
{
    void* const p = ProAllocateSgiPoolBuffer(count * size, poolIndex);
    if (p != NULL)
    {
        memset(p, 0, count * size);
    }

    return (p);
}

void*
PRO_CALLTYPE
ProPoolRealloc(void*         p,
               size_t        newSize,
               unsigned long poolIndex) /* 0 ~ 9 */
{
    return (ProReallocateSgiPoolBuffer(p, newSize, poolIndex));
}

void
PRO_CALLTYPE
ProPoolFree(void*         p,
            unsigned long poolIndex)    /* 0 ~ 9 */
{
    ProDeallocateSgiPoolBuffer(p, poolIndex);
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif
