/*
 * Copyright (c) 1996-1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#if !defined(__SGI_STL_INTERNAL_ALLOC_H)
#define __SGI_STL_INTERNAL_ALLOC_H

#include <cstddef>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(_WINSOCK_DEPRECATED_NO_WARNINGS)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32)

class CProThreadMutex_i
{
public:

    CProThreadMutex_i()
    {
        ::InitializeCriticalSection(&m_cs);
    }

    ~CProThreadMutex_i()
    {
        ::DeleteCriticalSection(&m_cs);
    }

    void Lock()
    {
        ::EnterCriticalSection(&m_cs);
    }

    void Unlock()
    {
        ::LeaveCriticalSection(&m_cs);
    }

private:

    CRITICAL_SECTION m_cs;
};

#else  /* _WIN32 */

class CProThreadMutex_i
{
public:

    CProThreadMutex_i()
    {
        pthread_mutex_init(&m_mutext, NULL);
    }

    ~CProThreadMutex_i()
    {
        pthread_mutex_destroy(&m_mutext);
    }

    void Lock()
    {
        pthread_mutex_lock(&m_mutext);
    }

    void Unlock()
    {
        pthread_mutex_unlock(&m_mutext);
    }

private:

    pthread_mutex_t m_mutext;
};

#endif /* _WIN32 */

// This implements some standard node allocators.  These are
// NOT the same as the allocators in the C++ draft standard or in
// the original STL.  They do not encapsulate different pointer
// types; indeed we assume that there is only one pointer type.
// The allocation primitives are intended to allocate individual objects,
// not larger arenas as with the original STL allocators.

// Default node allocator.
// With a reasonable compiler, this should be roughly as fast as the
// original STL class-specific allocators, but with less fragmentation.
// Default_alloc_template parameters are experimental and MAY
// DISAPPEAR in the future.  Clients should just use alloc for now.
//
// Important implementation properties:
// 1. If the client request an object of size > __MAX_OBJ_BYTES, the resulting
//    object will be obtained directly from malloc.
// 2. In all other cases, we allocate an object of size exactly
//    _S_round_up(requested_size).  Thus the client has enough size
//    information that we can return the object to the proper free list
//    without permanently losing part of the object.

// It is safe to allocate an object from one instance of a default_alloc
// and deallocate it with another one.  This effectively transfers its
// ownership to the second one.  This may have undesirable effects
// on reference locality.
// The template parameter is unreferenced and serves only to allow the
// creation of multiple default_alloc instances.
// Node that containers built on different allocator instances have
// different types, limiting the utility of this approach.

namespace std {

/*
 * buf_obj = hdr + app_level_obj = 8 + app_level_obj,
 * please refer to "../pro_shared.cpp"
 */
enum { __MAX_OBJ_BYTES = 8 + 1024 * 128  }; /* sizeof(app_level_obj) <= 128K */
enum { __BIG_OBJ_BYTES = 8 + 4096        }; /* sizeof(app_level_big_obj) >= 4096 */
enum { __CHUNK_SIZE    = 1024 * 1024 * 4 }; /* sizeof(chunk) is 4M, >= glibc::M_MMAP_THRESHOLD */
enum { __NFREELISTS    = 57              }; /* 57 levels */

union __Obj
{
    union __Obj* _M_free_list_link;
    char         _M_client_data[1];
};

template<int inst>
class __default_alloc_template
{
public:

    static void* allocate(
        size_t             __n,
        CProThreadMutex_i* __lock /* = NULL */
        )
    {
        if (__n == 0)
        {
            return NULL;
        }

        void* __ret = NULL;

        if (__n > (size_t)__MAX_OBJ_BYTES)
        {
            __ret = malloc(__n);
        }
        else
        {
            if (__lock != NULL)
            {
                __lock->Lock();
            }

            int __index = _S_freelist_index(__n);
            __Obj** __my_free_list = _S_free_list + __index;
            __Obj* __result = *__my_free_list;

            if (__result == NULL)
            {
                __ret = _S_refill(_S_round_up(__n));
            }
            else
            {
                *__my_free_list = __result->_M_free_list_link;
                __ret = __result;
            }

            if (__ret != NULL)
            {
                ++_S_busy_obj_num[__index];
            }

            if (__lock != NULL)
            {
                __lock->Unlock();
            }
        }

        return __ret;
    }

    static void deallocate(
        void*              __p,
        size_t             __n,
        CProThreadMutex_i* __lock /* = NULL */
        )
    {
        if (__p == NULL || __n == 0)
        {
            return;
        }

        if (__n > (size_t)__MAX_OBJ_BYTES)
        {
            free(__p);
        }
        else
        {
            if (__lock != NULL)
            {
                __lock->Lock();
            }

            int __index = _S_freelist_index(__n);
            __Obj** __my_free_list = _S_free_list + __index;
            __Obj* __q = (__Obj*)__p;

            __q->_M_free_list_link = *__my_free_list;
            *__my_free_list = __q;

            --_S_busy_obj_num[__index];

            if (__lock != NULL)
            {
                __lock->Unlock();
            }
        }
    }

    static void* reallocate(
        void*              __p,
        size_t             __old_sz,
        size_t             __new_sz,
        CProThreadMutex_i* __lock /* = NULL */
        )
    {
        if (__p == NULL || __old_sz == 0)
        {
            return allocate(__new_sz, __lock);
        }

        if (__new_sz == 0)
        {
            deallocate(__p, __old_sz, __lock);

            return NULL;
        }

        if (__old_sz > (size_t)__MAX_OBJ_BYTES && __new_sz > (size_t)__MAX_OBJ_BYTES)
        {
            return realloc(__p, __new_sz);
        }

        if (_S_round_up(__old_sz) == _S_round_up(__new_sz))
        {
            return __p;
        }

        if (__lock != NULL)
        {
            __lock->Lock();
        }

        void* __result = allocate(__new_sz, NULL);
        if (__result != NULL)
        {
            size_t __copy_sz = __new_sz > __old_sz ? __old_sz : __new_sz;
            memcpy(__result, __p, __copy_sz);
            deallocate(__p, __old_sz, NULL);
        }

        if (__lock != NULL)
        {
            __lock->Unlock();
        }

        return __result;
    }

    static void get_info(
        void*              __free_list[__NFREELISTS],
        size_t             __obj_size[__NFREELISTS],
        size_t             __busy_obj_num[__NFREELISTS],
        size_t             __total_obj_num[__NFREELISTS],
        size_t*            __heap_size,
        CProThreadMutex_i* __lock /* = NULL */
        )
    {
        if (__lock != NULL)
        {
            __lock->Lock();
        }

        memcpy(__free_list    , (void*)_S_free_list    , sizeof(_S_free_list));
        memcpy(__obj_size     , (void*)_S_obj_size     , sizeof(_S_obj_size));
        memcpy(__busy_obj_num , (void*)_S_busy_obj_num , sizeof(_S_busy_obj_num));
        memcpy(__total_obj_num, (void*)_S_total_obj_num, sizeof(_S_total_obj_num));
        if (__heap_size != NULL)
        {
            *__heap_size = _S_heap_size;
        }

        if (__lock != NULL)
        {
            __lock->Unlock();
        }
    }

private:

    static int _S_freelist_index(size_t __bytes)
    {
        int __l = 0;
        int __r = __NFREELISTS - 1;

        if (__bytes <= _S_obj_size[__l])
        {
            return __l;
        }
        else if (__bytes >= _S_obj_size[__r])
        {
            return __r;
        }
        else
        {
        }

        while (__l < __r)
        {
            int __m = (__l + __r) / 2;
            if (__bytes == _S_obj_size[__m])
            {
                return __m;
            }
            else if (__bytes > _S_obj_size[__m])
            {
                __l = __m + 1;
            }
            else
            {
                __r = __m;
            }
        }

        return __l;
    }

    static size_t _S_round_up(size_t __bytes)
    {
        return _S_obj_size[_S_freelist_index(__bytes)];
    }

    // Returns an object of size __n, and optionally adds to size __n free list.
    static void* _S_refill(size_t __n);

    // Allocates a chunk for __nobjs of size __size.  __nobjs may be reduced
    // if it is inconvenient to allocate the requested number.
    static char* _S_chunk_alloc(
        size_t __size,
        int&   __nobjs
        );

private:

    static __Obj* _S_free_list[__NFREELISTS];
    static size_t _S_obj_size[__NFREELISTS];
    static size_t _S_busy_obj_num[__NFREELISTS];
    static size_t _S_total_obj_num[__NFREELISTS];

    // Chunk allocation state.
    static char*  _S_start_free;
    static char*  _S_end_free;
    static size_t _S_heap_size;
};

template<int __inst>
__Obj* __default_alloc_template<__inst>::_S_free_list[__NFREELISTS] = { 0 };

/*
 * refer to the "jemalloc/tcmalloc"
 */
template<int __inst>
size_t __default_alloc_template<__inst>::_S_obj_size[__NFREELISTS] =
{
    8 +    8, 8 +   16, 8 +   24, 8 +   32, 8 +  40, 8 +  48, 8 +  56, 8 +  64, /* x, x +   8 */
    8 +   72, 8 +   80, 8 +   88, 8 +   96, 8 + 104, 8 + 112, 8 + 120, 8 + 128, /* x, x +   8 */
    8 +  144, 8 +  160,                                                         /* x, x +  16 */
    8 +  192, 8 +  224, 8 +  256,                                               /* x, x +  32 */
    8 +  320, 8 +  384, 8 +  448, 8 +  512,                                     /* x, x +  64 */
    8 +  640, 8 +  768, 8 +  896, 8 + 1024,                                     /* x, x + 128 */
    8 + 1280, 8 + 1536, 8 + 1792, 8 + 2048,                                     /* x, x + 256 */
    8 + 2560, 8 + 3072, 8 + 3584, 8 + 4096,                                     /* x, x + 512 */
    8 + 1024 *  5, 8 + 1024 *  6, 8 + 1024 *   7, 8 + 1024 *   8,               /* x, x +  1K */
    8 + 1024 * 10, 8 + 1024 * 12, 8 + 1024 *  14, 8 + 1024 *  16,               /* x, x +  2K */
    8 + 1024 * 20, 8 + 1024 * 24, 8 + 1024 *  28, 8 + 1024 *  32,               /* x, x +  4K */
    8 + 1024 * 40, 8 + 1024 * 48, 8 + 1024 *  56, 8 + 1024 *  64,               /* x, x +  8K */
    8 + 1024 * 80, 8 + 1024 * 96, 8 + 1024 * 112, 8 + 1024 * 128                /* x, x + 16K */
};

template<int __inst>
size_t __default_alloc_template<__inst>::_S_busy_obj_num[__NFREELISTS] = { 0 };

template<int __inst>
size_t __default_alloc_template<__inst>::_S_total_obj_num[__NFREELISTS] = { 0 };

template<int __inst>
char* __default_alloc_template<__inst>::_S_start_free = NULL;

template<int __inst>
char* __default_alloc_template<__inst>::_S_end_free = NULL;

template<int __inst>
size_t __default_alloc_template<__inst>::_S_heap_size = 0;

// Returns an object of size __n, and optionally adds to size __n free list.
// We assume that __n is properly aligned.
template<int __inst>
void*
__default_alloc_template<__inst>::_S_refill(size_t __n)
{
    int     __nobjs        = 20;
    char*   __chunk        = _S_chunk_alloc(__n, __nobjs); // (__n * __nobjs)
    __Obj** __my_free_list = NULL;
    __Obj*  __result       = NULL;
    __Obj*  __current_obj  = NULL;
    __Obj*  __next_obj     = NULL;
    int     __index        = _S_freelist_index(__n);

    if (__chunk == NULL)
    {
        return NULL;
    }

    if (__nobjs == 1)
    {
        ++_S_total_obj_num[__index];

        return __chunk;
    }

    __my_free_list = _S_free_list + __index;

    // Build free list in chunk.
    __result = (__Obj*)__chunk;
    __next_obj = (__Obj*)(__chunk + __n);
    *__my_free_list = __next_obj;

    for (int __i = 1; ; ++__i)
    {
        __current_obj = __next_obj;
        __next_obj = (__Obj*)((char*)__next_obj + __n);

        if (__i == __nobjs - 1)
        {
            __current_obj->_M_free_list_link = NULL;
            break;
        }
        else
        {
            __current_obj->_M_free_list_link = __next_obj;
        }
    }

    _S_total_obj_num[__index] += __nobjs;

    return __result;
}

// We allocate memory in large chunks in order to avoid fragmenting
// the malloc heap too much.
// We assume that __size is properly aligned.
template<int __inst>
char*
__default_alloc_template<__inst>::_S_chunk_alloc(size_t __size,
                                                 int&   __nobjs)
{
    if (__size >= (size_t)__BIG_OBJ_BYTES)
    {
        __nobjs = 1;
    }
    else if (__size * __nobjs >= (size_t)__BIG_OBJ_BYTES)
    {
        __nobjs = (int)(__BIG_OBJ_BYTES / __size);
    }
    else
    {
    }

    char*  __result      = NULL;
    size_t __total_bytes = __size * __nobjs;
    size_t __bytes_left  = _S_end_free - _S_start_free;

    if (__bytes_left >= __total_bytes)
    {
        __result = _S_start_free;
        _S_start_free += __total_bytes;

        return __result;
    }
    else if (__bytes_left >= __size)
    {
        __nobjs = (int)(__bytes_left / __size);
        __total_bytes = __size * __nobjs;

        __result = _S_start_free;
        _S_start_free += __total_bytes;

        return __result;
    }
    else
    {
        for (int __i = _S_freelist_index(__bytes_left); __i >= 0; --__i)
        {
            if (__bytes_left == 0)
            {
                break;
            }

            size_t  __obj_size     = _S_obj_size[__i];
            __Obj** __my_free_list = _S_free_list + __i;

            while (__bytes_left >= __obj_size)
            {
                ((__Obj*)_S_start_free)->_M_free_list_link = *__my_free_list;
                *__my_free_list = (__Obj*)_S_start_free;

                _S_start_free += __obj_size;
                __bytes_left  -= __obj_size;

                ++_S_total_obj_num[__i];
            }
        }

        size_t __bytes_to_get = __CHUNK_SIZE;

        _S_start_free = (char*)malloc(__bytes_to_get);
        _S_end_free   = NULL;
        if (_S_start_free == NULL)
        {
            __bytes_to_get = __CHUNK_SIZE / 2;
            _S_start_free  = (char*)malloc(__bytes_to_get); // retry
        }

        if (_S_start_free != NULL)
        {
            _S_end_free  =  _S_start_free + __bytes_to_get;
            _S_heap_size += __bytes_to_get;

            return _S_chunk_alloc(__size, __nobjs);
        }

        // Try to make do with what we have.  That can't
        // hurt.  We do not try smaller requests, since that tends
        // to result in disaster on multi-process machines.
        for (int __j = _S_freelist_index(__size); __j < __NFREELISTS; ++__j)
        {
            __Obj** __my_free_list = _S_free_list + __j;
            if (*__my_free_list != NULL)
            {
                __Obj* __p = *__my_free_list;
                *__my_free_list = __p->_M_free_list_link;

                _S_start_free = (char*)__p;
                _S_end_free   = _S_start_free + _S_obj_size[__j];

                --_S_total_obj_num[__j];

                return _S_chunk_alloc(__size, __nobjs);
            }
        }

        return NULL;
    }
}

} /* namespace std */

#endif /* __SGI_STL_INTERNAL_ALLOC_H */
