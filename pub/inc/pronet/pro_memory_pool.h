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

#ifndef ____PRO_MEMORY_POOL_H____
#define ____PRO_MEMORY_POOL_H____

#include "pro_a.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#if defined(DECLARE_SGI_POOL)
#undef DECLARE_SGI_POOL
#endif

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_OPNEW)
#define DECLARE_SGI_POOL(a)                                                                       \
public:                                                                                           \
    static void* operator new(size_t size)        { return ProAllocateSgiPoolBuffer(size, (a)); } \
    static void  operator delete(void* p)         { ProDeallocateSgiPoolBuffer(p, (a)); }         \
    static void* operator new(size_t, void* p0)   { return p0; }                                  \
    static void  operator delete(void*, void* p0) { (void)p0; }                                   \
protected:
#else
#define DECLARE_SGI_POOL(a)
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
extern "C" {
#endif

void*
ProMalloc(size_t size);

void*
ProCalloc(size_t count,
          size_t size);

void*
ProRealloc(void*  p,
           size_t newSize);

void
ProFree(void* p);

extern
void*
ProAllocateSgiPoolBuffer(size_t       size,
                         unsigned int poolIndex); /* [0, 3] */

extern
void*
ProReallocateSgiPoolBuffer(void*        buf,
                           size_t       newSize,
                           unsigned int poolIndex); /* [0, 3] */

extern
void
ProDeallocateSgiPoolBuffer(void*        buf,
                           unsigned int poolIndex); /* [0, 3] */

#if defined(__cplusplus)
} /* extern "C" */
#endif

/////////////////////////////////////////////////////////////////////////////
////

namespace std {

template<typename ____Ty, unsigned int ____poolIndex = 0>
class pro_allocator : public allocator<____Ty>
{
public:

    template<class ____Other>
    struct rebind
    {
        typedef pro_allocator<____Other, ____poolIndex> other;
    };

    pro_allocator() : allocator<____Ty>()
    {
    }

    pro_allocator(const pro_allocator<____Ty, ____poolIndex>&) : allocator<____Ty>()
    {
    }

    template<class ____Other>
    pro_allocator(const pro_allocator<____Other, ____poolIndex>&)
    {
        /*
         * do nothing
         */
    }

    template<class ____Other>
    pro_allocator<____Ty, ____poolIndex>& operator=(const pro_allocator<____Other, ____poolIndex>&)
    {
        /*
         * do nothing
         */
        return *this;
    }

    ____Ty* allocate(size_t ____N)
    {
        ____Ty* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (____Ty*)ProAllocateSgiPoolBuffer(sizeof(____Ty) * ____N, ____poolIndex);
#else
        p = (____Ty*)::operator new(sizeof(____Ty) * ____N);
#endif

        return p;
    }

    ____Ty* allocate(size_t ____N, const void*)
    {
        ____Ty* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (____Ty*)ProAllocateSgiPoolBuffer(sizeof(____Ty) * ____N, ____poolIndex);
#else
        p = (____Ty*)::operator new(sizeof(____Ty) * ____N);
#endif

        return p;
    }

    char* _Charalloc(size_t ____N)
    {
        char* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (char*)ProAllocateSgiPoolBuffer(sizeof(char) * ____N, ____poolIndex);
#else
        p = (char*)::operator new(sizeof(char) * ____N);
#endif

        return p;
    }

    void deallocate(void* ____P, size_t)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        ProDeallocateSgiPoolBuffer(____P, ____poolIndex);
#else
        ::operator delete(____P);
#endif
    }

    DECLARE_SGI_POOL(0)
};

template<>
class pro_allocator<void> : public allocator<void>
{
public:

    template<class ____Other>
    struct rebind
    {
        typedef pro_allocator<____Other> other;
    };

    pro_allocator()
    {
        /*
         * do nothing
         */
    }

    pro_allocator(const pro_allocator<void>&)
    {
        /*
         * do nothing
         */
    }

    template<class ____Other>
    pro_allocator(const pro_allocator<____Other>&)
    {
        /*
         * do nothing
         */
    }

    template<class ____Other>
    pro_allocator<void>& operator=(const pro_allocator<____Other>&)
    {
        /*
         * do nothing
         */
        return *this;
    }

    DECLARE_SGI_POOL(0)
};

} /* namespace std */

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_MEMORY_POOL_H____ */
