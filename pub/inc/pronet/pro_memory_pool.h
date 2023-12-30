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

template<typename __T, unsigned int __poolIndex = 0>
class pro_allocator : public allocator<__T>
{
public:

    template<typename __Other>
    struct rebind
    {
        typedef pro_allocator<__Other, __poolIndex> other;
    };

    pro_allocator() : allocator<__T>()
    {
    }

    pro_allocator(const pro_allocator<__T, __poolIndex>&) : allocator<__T>()
    {
    }

    template<typename __Other>
    pro_allocator(const pro_allocator<__Other, __poolIndex>&)
    {
        /*
         * do nothing
         */
    }

    template<typename __Other>
    pro_allocator<__T, __poolIndex>& operator=(const pro_allocator<__Other, __poolIndex>&)
    {
        /*
         * do nothing
         */
        return *this;
    }

    __T* allocate(size_t __n)
    {
        __T* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (__T*)ProAllocateSgiPoolBuffer(sizeof(__T) * __n, __poolIndex);
#else
        p = (__T*)::operator new(sizeof(__T) * __n);
#endif

        return p;
    }

    __T* allocate(size_t __n, const void*)
    {
        __T* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (__T*)ProAllocateSgiPoolBuffer(sizeof(__T) * __n, __poolIndex);
#else
        p = (__T*)::operator new(sizeof(__T) * __n);
#endif

        return p;
    }

    char* _Charalloc(size_t __n)
    {
        char* p = NULL;

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        p = (char*)ProAllocateSgiPoolBuffer(sizeof(char) * __n, __poolIndex);
#else
        p = (char*)::operator new(sizeof(char) * __n);
#endif

        return p;
    }

    void deallocate(void* __p, size_t)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        ProDeallocateSgiPoolBuffer(__p, __poolIndex);
#else
        ::operator delete(__p);
#endif
    }

    DECLARE_SGI_POOL(0)
};

template<>
class pro_allocator<void> : public allocator<void>
{
public:

    template<typename __Other>
    struct rebind
    {
        typedef pro_allocator<__Other> other;
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

    template<typename __Other>
    pro_allocator(const pro_allocator<__Other>&)
    {
        /*
         * do nothing
         */
    }

    template<typename __Other>
    pro_allocator<void>& operator=(const pro_allocator<__Other>&)
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
