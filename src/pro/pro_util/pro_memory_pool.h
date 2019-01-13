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

#if !defined(____PRO_MEMORY_POOL_H____)
#define ____PRO_MEMORY_POOL_H____

#include "pro_a.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include <cstddef>
#include <memory>

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(____STD____)
#define ____STD____
#define ____STD_BEGIN namespace std {
#define ____STD_END   };
#endif

#if defined(DECLARE_SGI_POOL)
#undef DECLARE_SGI_POOL
#endif

#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_OPNEW)
#define DECLARE_SGI_POOL(a)                                                                         \
public:                                                                                             \
    static void* operator new(size_t size)        { return (ProAllocateSgiPoolBuffer(size, (a))); } \
    static void  operator delete(void* p)         { ProDeallocateSgiPoolBuffer(p, (a)); }           \
    static void* operator new(size_t, void* p0)   { return (p0); }                                  \
    static void  operator delete(void*, void* p0) { (void)p0; }                                     \
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
PRO_CALLTYPE
ProMalloc(size_t size);

void*
PRO_CALLTYPE
ProCalloc(size_t count,
          size_t size);

void*
PRO_CALLTYPE
ProRealloc(void*  p,
           size_t newSize);

void
PRO_CALLTYPE
ProFree(void* p);

void*
PRO_CALLTYPE
ProPoolMalloc(size_t        size,
              unsigned long poolIndex);  /* 0 ~ 9 */

void*
PRO_CALLTYPE
ProPoolCalloc(size_t        count,
              size_t        size,
              unsigned long poolIndex);  /* 0 ~ 9 */

void*
PRO_CALLTYPE
ProPoolRealloc(void*         p,
               size_t        newSize,
               unsigned long poolIndex); /* 0 ~ 9 */

void
PRO_CALLTYPE
ProPoolFree(void*         p,
            unsigned long poolIndex);    /* 0 ~ 9 */

extern
void*
PRO_CALLTYPE
ProAllocateSgiPoolBuffer(size_t        size,
                         unsigned long poolIndex);   /* 0 ~ 9 */

extern
void
PRO_CALLTYPE
ProDeallocateSgiPoolBuffer(void*         buf,
                           unsigned long poolIndex); /* 0 ~ 9 */

#if defined(__cplusplus)
}
#endif

/////////////////////////////////////////////////////////////////////////////
////

____STD_BEGIN

template<typename ____Ty, unsigned long ____poolIndex = 0>
class pro_allocator : public allocator<____Ty>
{
public:

    template<class ____Other> struct rebind
    {
        typedef pro_allocator<____Other, ____poolIndex> other;
    };

    pro_allocator()
    {
    }

    pro_allocator(const pro_allocator<____Ty, ____poolIndex>&)
        : allocator<____Ty>()
    {
    }

#if !defined(_MSC_VER) || (_MSC_VER > 1200) /* 1200 is 6.0 */
    template<class ____Other>
        pro_allocator(const pro_allocator<____Other, ____poolIndex>&)
        : allocator<____Ty>()
    {
    }

    template<class ____Other> pro_allocator<____Ty, ____poolIndex>&
        operator=(const pro_allocator<____Other, ____poolIndex>&)
    {
        return (*this);
    }
#endif

    ____Ty* allocate(size_t ____N)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        ____Ty* const p = (____Ty*)ProPoolMalloc(sizeof(____Ty) * ____N, ____poolIndex);
#else
        ____Ty* const p = (____Ty*)::operator new(sizeof(____Ty) * ____N);
#endif

        return (p);
    }

    ____Ty* allocate(size_t ____N, const void*)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        ____Ty* const p = (____Ty*)ProPoolMalloc(sizeof(____Ty) * ____N, ____poolIndex);
#else
        ____Ty* const p = (____Ty*)::operator new(sizeof(____Ty) * ____N);
#endif

        return (p);
    }

    char* _Charalloc(size_t ____N)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        char* const p = (char*)ProPoolMalloc(sizeof(char) * ____N, ____poolIndex);
#else
        char* const p = (char*)::operator new(sizeof(char) * ____N);
#endif

        return (p);
    }

    void deallocate(void* ____P, size_t)
    {
#if !defined(PRO_LACKS_SGI_POOL) && !defined(PRO_LACKS_SGI_POOL_STL)
        ProPoolFree(____P, ____poolIndex);
#else
        ::operator delete(____P);
#endif
    }

    DECLARE_SGI_POOL(0);
};

template<>
class pro_allocator<void> : public allocator<void>
{
public:

    template<class ____Other> struct rebind
    {
        typedef pro_allocator<____Other> other;
    };

    pro_allocator()
    {
    }

    pro_allocator(const pro_allocator<void>&)
    {
    }

#if !defined(_MSC_VER) || (_MSC_VER > 1200) /* 1200 is 6.0 */
    template<class ____Other> pro_allocator(const pro_allocator<____Other>&)
    {
    }

    template<class ____Other> pro_allocator<void>& operator=(const pro_allocator<____Other>&)
    {
        return (*this);
    }
#endif

    DECLARE_SGI_POOL(0);
};

____STD_END

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_MEMORY_POOL_H____ */
