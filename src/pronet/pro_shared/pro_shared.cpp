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

#if defined(_WIN32)
#if defined(PRO_FD_SETSIZE)
#undef  PRO_FD_SETSIZE
#endif
#define PRO_FD_SETSIZE 8
#endif

#include "pro_a.h"
#include "pro_shared.h"
#include "pro_bsd_wrapper.h"
#include "pro_version.h"
#include "pro_z.h"
#include "stl_alloc.h"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <pthread.h>
#if defined(PRO_HAS_MACH_ABSOLUTE_TIME)
#include <mach/mach_time.h>
#endif
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32)
#define PBSD_EINTR        WSAEINTR        /* 10004 */
#define PBSD_EINVAL       WSAEINVAL       /* 10022 */
#define PBSD_ENOTSOCK     WSAENOTSOCK     /* 10038 */
#define PBSD_ECONNREFUSED WSAECONNREFUSED /* 10061 */
#else
#define PBSD_EINTR        EINTR           /*   4 */
#define PBSD_EINVAL       EINVAL          /*  22 */
#define PBSD_ENOTSOCK     ENOTSOCK        /*   8 */
#define PBSD_ECONNREFUSED ECONNREFUSED    /* 111 */
#endif

#if defined(_WIN32)
static volatile bool                    g_s_tlsFlag       = false;
static unsigned long                    g_s_tlsKey0       = (unsigned long)-1;
static unsigned long                    g_s_tlsKey1       = (unsigned long)-1;
static int64_t                          g_s_globalTick    = 0;
#elif defined(PRO_HAS_MACH_ABSOLUTE_TIME)
static volatile bool                    g_s_timebaseFlag  = false;
static mach_timebase_info_data_t        g_s_timebaseInfo  = { 0 };
#endif
static volatile bool                    g_s_socketFlag    = false;
static int64_t                          g_s_sockId        = -1;
static uint64_t                         g_s_nextTimerId   = 1;
static uint64_t                         g_s_nextMmTimerId = 2;
static CProThreadMutex_i*               g_s_lock          = NULL;

/*
 * pool-0
 */
static std::__default_alloc_template<0> g_s_allocator0;
static CProThreadMutex_i*               g_s_lock0         = NULL;

/*
 * pool-1
 */
static std::__default_alloc_template<1> g_s_allocator1;
static CProThreadMutex_i*               g_s_lock1         = NULL;

/*
 * pool-2
 */
static std::__default_alloc_template<2> g_s_allocator2;
static CProThreadMutex_i*               g_s_lock2         = NULL;

/*
 * pool-3
 */
static std::__default_alloc_template<3> g_s_allocator3;
static CProThreadMutex_i*               g_s_lock3         = NULL;

/////////////////////////////////////////////////////////////////////////////
////

static
void
pbsd_startup_i()
{
    static bool s_flag = false;
    if (s_flag)
    {
        return;
    }
    s_flag = true;

#if defined(_WIN32)
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

static
int
pbsd_errno_i()
{
    int errcode = 0;

#if defined(_WIN32)
    errcode = ::WSAGetLastError();
#else
    errcode = errno;
#endif

    if (errcode == PBSD_ENOTSOCK)
    {
        errcode = PBSD_EBADF;
    }
    if (errcode == PBSD_ECONNREFUSED)
    {
        errcode = PBSD_ECONNRESET;
    }

    return errcode;
}

static
int
pbsd_ioctl_closexec_i(int64_t fd)
{
    int retc = -1;

#if !defined(_WIN32)

    do
    {
        retc = fcntl((int)fd, F_GETFD);
    }
    while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);

    if (retc >= 0)
    {
        int flags = retc | FD_CLOEXEC;

        do
        {
            retc = fcntl((int)fd, F_SETFD, flags);
        }
        while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);
    }

#endif /* _WIN32 */

    return retc;
}

static
int64_t
pbsd_socket_i(int af,
              int type,
              int protocol)
{
    int64_t fd = -1;

#if defined(_WIN32)

    if (sizeof(SOCKET) == 8)
    {
        fd = (int64_t)socket(af, type, protocol);
    }
    else
    {
        fd = (int32_t)socket(af, type, protocol);
    }

#else  /* _WIN32 */

#if defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        fd        = (int32_t)socket(af, type | SOCK_CLOEXEC, protocol);
        errorcode = pbsd_errno_i();
    }
#endif

    if (fd < 0)
    {
        fd = (int32_t)socket(af, type, protocol);

#if defined(SOCK_CLOEXEC)
        if (fd >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

#endif /* _WIN32 */

    if (fd >= 0)
    {
        pbsd_ioctl_closexec_i(fd);
    }
    else
    {
        fd = -1;
    }

    return fd;
}

#if defined(_WIN32)

static
int
pbsd_select_i(pbsd_fd_set*    readfds,
              pbsd_fd_set*    writefds,
              pbsd_fd_set*    exceptfds,
              struct timeval* timeout)
{
    return select(0, readfds, writefds, exceptfds, timeout);
}

#else  /* _WIN32 */

static
int
pbsd_poll_i(pbsd_pollfd* fds,
            size_t       nfds,
            int          timeout)
{
    int retc = -1;

    do
    {
        retc = poll(fds, nfds, timeout);
    }
    while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);

    return retc;
}

#endif /* _WIN32 */

static
void
pbsd_closesocket_i(int64_t fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32)
    closesocket((SOCKET)fd);
#else
    close((int)fd);
#endif
}

/*
 * Be sure to initialize important global data first to prevent crashes
 * caused by it being used before entering main()!!!
 */
static
void
Init_i()
{
    pbsd_startup_i();

    if (g_s_lock  == NULL)
    {
        g_s_lock  = new CProThreadMutex_i;
    }
    if (g_s_lock0 == NULL)
    {
        g_s_lock0 = new CProThreadMutex_i;
    }
    if (g_s_lock1 == NULL)
    {
        g_s_lock1 = new CProThreadMutex_i;
    }
    if (g_s_lock2 == NULL)
    {
        g_s_lock2 = new CProThreadMutex_i;
    }
    if (g_s_lock3 == NULL)
    {
        g_s_lock3 = new CProThreadMutex_i;
    }
}

static
void
InitSocket_i()
{
    g_s_sockId = pbsd_socket_i(AF_INET, SOCK_DGRAM, 0);
}

static
void
FiniSocket_i()
{
    pbsd_closesocket_i(g_s_sockId);
    g_s_sockId = -1;
}

static
void
Delay1ms_i()
{
    int64_t te = ProGetTickCount64() + 1;

    while (1)
    {
#if defined(_WIN32)
        ::Sleep(1);
#else
        usleep(1000);
#endif

        if (ProGetTickCount64() >= te)
        {
            break;
        }
    }
}

static
uint32_t
GetTickCount32_i()
{
    Init_i();

    int64_t ret = 0;

#if defined(_WIN32)

    ret = ::timeGetTime();

#elif defined(PRO_HAS_MACH_ABSOLUTE_TIME) /* for MacOS */

    if (!g_s_timebaseFlag)
    {
        g_s_lock->Lock();
        if (!g_s_timebaseFlag) /* double check */
        {
            mach_timebase_info(&g_s_timebaseInfo);
            g_s_timebaseFlag = true;
        }
        g_s_lock->Unlock();
    }

    ret =  mach_absolute_time();
    ret =  ret * g_s_timebaseInfo.numer / g_s_timebaseInfo.denom; /* ns_ticks ---> ns */
    ret /= 1000000;                                               /* ns       ---> ms */

#elif !defined(PRO_LACKS_CLOCK_GETTIME)   /* for non-MacOS */

    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);

    ret =  ts.tv_sec;
    ret *= 1000;
    ret += ts.tv_nsec / 1000000;

#else

    assert(0);

#endif

    return (uint32_t)ret;
}

/////////////////////////////////////////////////////////////////////////////
////

PRO_SHARED_API
void
ProSharedVersion(unsigned char* major, /* = NULL */
                 unsigned char* minor, /* = NULL */
                 unsigned char* patch) /* = NULL */
{
    if (major != NULL)
    {
        *major = PRO_VER_MAJOR;
    }
    if (minor != NULL)
    {
        *minor = PRO_VER_MINOR;
    }
    if (patch != NULL)
    {
        *patch = PRO_VER_PATCH;
    }
}

PRO_SHARED_API
void
ProSrand()
{
    int64_t tick = GetTickCount32_i(); /* Don't invoke ProGetTickCount64()!!! */
    tick %=  128;
    tick <<= 16;

    int64_t seed = time(NULL);
    seed += tick;

    srand((unsigned int)seed);
}

PRO_SHARED_API
double
ProRand_0_1()
{
    return (double)rand() / RAND_MAX;
}

PRO_SHARED_API
int
ProRand_0_32767()
{
    return rand();
}

PRO_SHARED_API
int64_t
ProGetTickCount64()
{
    Init_i();

    int64_t ret = 0;

#if defined(_WIN32)

    if (!g_s_tlsFlag)
    {
        g_s_lock->Lock();
        if (!g_s_tlsFlag) /* double check */
        {
            g_s_tlsKey0    = ::TlsAlloc(); /* dynamic TLS */
            g_s_tlsKey1    = ::TlsAlloc(); /* dynamic TLS */
            g_s_globalTick = ::timeGetTime();

            g_s_tlsFlag = true;
        }
        g_s_lock->Unlock();
    }

    bool updateGlobalTick = false;

    uint32_t tick0 = (uint32_t)(uint64_t)::TlsGetValue(g_s_tlsKey0);
    uint32_t tick1 = (uint32_t)(uint64_t)::TlsGetValue(g_s_tlsKey1);
    if (tick0 == 0 && tick1 == 0)
    {
        g_s_lock->Lock();
        tick0 = (uint32_t)g_s_globalTick;
        tick1 = (uint32_t)(g_s_globalTick >> 32);
        g_s_lock->Unlock();

        ::TlsSetValue(g_s_tlsKey0, (void*)(uint64_t)tick0);
        ::TlsSetValue(g_s_tlsKey1, (void*)(uint64_t)tick1);

        updateGlobalTick = true;
    }

    uint32_t tick = ::timeGetTime();
    if (tick > tick0)
    {
        tick0 = tick;
        ::TlsSetValue(g_s_tlsKey0, (void*)(uint64_t)tick0);
    }
    else if (tick < tick0)
    {
        tick0 = tick;
        ++tick1;
        ::TlsSetValue(g_s_tlsKey0, (void*)(uint64_t)tick0);
        ::TlsSetValue(g_s_tlsKey1, (void*)(uint64_t)tick1);

        updateGlobalTick = true;
    }
    else
    {
    }

    ret =   tick1;
    ret <<= 32;
    ret |=  tick0;

    if (updateGlobalTick)
    {
        g_s_lock->Lock();
        if (ret > g_s_globalTick)
        {
            g_s_globalTick = ret;
        }
        g_s_lock->Unlock();
    }

#elif defined(PRO_HAS_MACH_ABSOLUTE_TIME) /* for MacOS */

    if (!g_s_timebaseFlag)
    {
        g_s_lock->Lock();
        if (!g_s_timebaseFlag) /* double check */
        {
            mach_timebase_info(&g_s_timebaseInfo);
            g_s_timebaseFlag = true;
        }
        g_s_lock->Unlock();
    }

    ret =  mach_absolute_time();
    ret =  ret * g_s_timebaseInfo.numer / g_s_timebaseInfo.denom; /* ns_ticks ---> ns */
    ret /= 1000000;                                               /* ns       ---> ms */

#elif !defined(PRO_LACKS_CLOCK_GETTIME)   /* for non-MacOS */

    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);

    ret =  ts.tv_sec;
    ret *= 1000;
    ret += ts.tv_nsec / 1000000;

#else

    assert(0);

#endif

    return ret;
}

PRO_SHARED_API
int64_t
ProGetNtpTickCount64()
{
    Init_i();

    int64_t tick1970 = 0;

#if defined(_WIN32)

    SYSTEMTIME st;
    ::GetLocalTime(&st);

    FILETIME ft;
    if (::SystemTimeToFileTime(&st, &ft))
    {
        int64_t ft64 = ft.dwHighDateTime;
        ft64 <<= 32;
        ft64 |=  ft.dwLowDateTime;
        ft64 /=  10000;

        const int64_t TIME_DIFF_WINDOWS_UNIX = 11644473600LL; /* [1601 ~ 1970] */

        tick1970 = ft64 - TIME_DIFF_WINDOWS_UNIX * 1000;
    }

#elif !defined(PRO_LACKS_CLOCK_GETTIME)

    struct timespec ts = { 0 };
    clock_gettime(CLOCK_REALTIME, &ts);

    tick1970 =  ts.tv_sec;
    tick1970 *= 1000;
    tick1970 += ts.tv_nsec / 1000000;

#else

    struct timeval tv = { 0 };
    if (gettimeofday(&tv, NULL) == 0)
    {
        tick1970 =  tv.tv_sec;
        tick1970 *= 1000;
        tick1970 += tv.tv_usec / 1000;
    }

#endif

    const int64_t TIME_DIFF_NTP_UNIX = 2208988800LL; /* [1900 ~ 1970] */

    return tick1970 + TIME_DIFF_NTP_UNIX * 1000;
}

PRO_SHARED_API
void
ProSleep(unsigned int milliseconds)
{
    Init_i();

    if (milliseconds == 0)
    {
#if defined(_WIN32)
        ::Sleep(0);
#else
        usleep(0);
#endif

        return;
    }

    if (milliseconds == (unsigned int)-1)
    {
        while (1)
        {
#if defined(_WIN32)
            ::Sleep(-1);
#else
            usleep(999999);
#endif
        }
    }

    if (!g_s_socketFlag)
    {
        g_s_lock->Lock();
        if (!g_s_socketFlag) /* double check */
        {
            InitSocket_i();
            g_s_socketFlag = true;
        }
        g_s_lock->Unlock();
    }

    int64_t t0 = ProGetTickCount64();
    int64_t te = t0 + milliseconds;
    int64_t t  = 0;

    while (1)
    {
        t = t == 0 ? t0 : ProGetTickCount64();
        if (t >= te)
        {
            break;
        }

        int64_t sockId = g_s_sockId;
        if (sockId == -1)
        {
            g_s_lock->Lock();
            FiniSocket_i();
            InitSocket_i();
            g_s_lock->Unlock();

            Delay1ms_i();
            continue;
        }

#if defined(_WIN32)

        pbsd_fd_set fds;
        PBSD_FD_ZERO(&fds);
        PBSD_FD_SET(sockId, &fds);

        struct timeval timeout = { 0 };
        timeout.tv_sec  = (long)((te - t) / 1000);
        timeout.tv_usec = (long)((te - t) % 1000 * 1000);

        int retc = pbsd_select_i(NULL, NULL, &fds, &timeout);

        assert(retc == 0);
        if (retc != 0)
        {
            g_s_lock->Lock();
            FiniSocket_i();
            InitSocket_i();
            g_s_lock->Unlock();

            Delay1ms_i();
        }

#elif defined(PRO_HAS_NANOSLEEP) /* for Linux */

        struct timespec req = { 0 };
        req.tv_sec  = (time_t)((te - t) / 1000);
        req.tv_nsec = (long)  ((te - t) % 1000 * 1000000);

        nanosleep(&req, NULL);

#else                            /* for non-Linux */

    pbsd_pollfd pfd;
    memset(&pfd, 0, sizeof(pbsd_pollfd));
    pfd.fd = (int)sockId;

    int retc = pbsd_poll_i(&pfd, 1, (int)(te - t));

    assert(retc == 0);
    if (retc != 0)
    {
        g_s_lock->Lock();
        FiniSocket_i();
        InitSocket_i();
        g_s_lock->Unlock();

        Delay1ms_i();
    }

#endif
    } /* end of while () */
}

PRO_SHARED_API
uint64_t
ProMakeTimerId()
{
    Init_i();

    g_s_lock->Lock();

    uint64_t timerId = g_s_nextTimerId;
    g_s_nextTimerId += 2;

    g_s_lock->Unlock();

    return timerId;
}

PRO_SHARED_API
uint64_t
ProMakeMmTimerId()
{
    Init_i();

    g_s_lock->Lock();

    uint64_t timerId = g_s_nextMmTimerId;
    g_s_nextMmTimerId += 2;
    if (g_s_nextMmTimerId == 0)
    {
        g_s_nextMmTimerId += 2;
    }

    g_s_lock->Unlock();

    return timerId;
}

PRO_SHARED_API
void*
ProAllocateSgiPoolBuffer(size_t       size,
                         unsigned int poolIndex) /* [0, 3] */
{
    Init_i();

    assert(poolIndex <= 3);
    if (poolIndex > 3)
    {
        return NULL;
    }

    if (size == 0)
    {
        size = sizeof(uint32_t) + sizeof(uint32_t) + 1;
    }
    else
    {
        size = sizeof(uint32_t) + sizeof(uint32_t) + size;
    }

    uint32_t* p = NULL;

    switch (poolIndex)
    {
    case 0:
        p = (uint32_t*)g_s_allocator0.allocate(size, g_s_lock0);
        break;
    case 1:
        p = (uint32_t*)g_s_allocator1.allocate(size, g_s_lock1);
        break;
    case 2:
        p = (uint32_t*)g_s_allocator2.allocate(size, g_s_lock2);
        break;
    case 3:
        p = (uint32_t*)g_s_allocator3.allocate(size, g_s_lock3);
        break;
    }

    if (p == NULL)
    {
        return NULL;
    }
    else
    {
        *p = (uint32_t)size;

        return p + 2;
    }
}

PRO_SHARED_API
void*
ProReallocateSgiPoolBuffer(void*        buf,
                           size_t       newSize,
                           unsigned int poolIndex) /* [0, 3] */
{
    Init_i();

    assert(poolIndex <= 3);
    if (poolIndex > 3)
    {
        return NULL;
    }

    if (buf == NULL)
    {
        return ProAllocateSgiPoolBuffer(newSize, poolIndex);
    }

    if (newSize == 0)
    {
        ProDeallocateSgiPoolBuffer(buf, poolIndex);

        return NULL;
    }

    uint32_t* p = (uint32_t*)buf - 2;
    if (*p < sizeof(uint32_t) + sizeof(uint32_t) + 1)
    {
        return NULL;
    }

    newSize = sizeof(uint32_t) + sizeof(uint32_t) + newSize;

    uint32_t* q = NULL;

    switch (poolIndex)
    {
    case 0:
        q = (uint32_t*)g_s_allocator0.reallocate(p, *p, newSize, g_s_lock0);
        break;
    case 1:
        q = (uint32_t*)g_s_allocator1.reallocate(p, *p, newSize, g_s_lock1);
        break;
    case 2:
        q = (uint32_t*)g_s_allocator2.reallocate(p, *p, newSize, g_s_lock2);
        break;
    case 3:
        q = (uint32_t*)g_s_allocator3.reallocate(p, *p, newSize, g_s_lock3);
        break;
    }

    if (q == NULL)
    {
        return NULL;
    }
    else
    {
        *q = (uint32_t)newSize;

        return q + 2;
    }
}

PRO_SHARED_API
void
ProDeallocateSgiPoolBuffer(void*        buf,
                           unsigned int poolIndex) /* [0, 3] */
{
    Init_i();

    assert(poolIndex <= 3);
    if (buf == NULL || poolIndex > 3)
    {
        return;
    }

    uint32_t* p = (uint32_t*)buf - 2;
    if (*p < sizeof(uint32_t) + sizeof(uint32_t) + 1)
    {
        return;
    }

    switch (poolIndex)
    {
    case 0:
        g_s_allocator0.deallocate(p, *p, g_s_lock0);
        break;
    case 1:
        g_s_allocator1.deallocate(p, *p, g_s_lock1);
        break;
    case 2:
        g_s_allocator2.deallocate(p, *p, g_s_lock2);
        break;
    case 3:
        g_s_allocator3.deallocate(p, *p, g_s_lock3);
        break;
    }
}

PRO_SHARED_API
void
ProGetSgiPoolInfo(void*        freeList[64],
                  size_t       objSize[64],
                  size_t       busyObjNum[64],
                  size_t       totalObjNum[64],
                  size_t*      heapBytes, /* = NULL */
                  unsigned int poolIndex) /* [0, 3] */
{
    Init_i();

    memset(freeList   , 0, sizeof(void*)  * 64);
    memset(objSize    , 0, sizeof(size_t) * 64);
    memset(busyObjNum , 0, sizeof(size_t) * 64);
    memset(totalObjNum, 0, sizeof(size_t) * 64);
    if (heapBytes != NULL)
    {
        *heapBytes = 0;
    }

    assert(poolIndex <= 3);
    if (poolIndex > 3)
    {
        return;
    }

    switch (poolIndex)
    {
    case 0:
        g_s_allocator0.get_info(freeList, objSize, busyObjNum, totalObjNum, heapBytes, g_s_lock0);
        break;
    case 1:
        g_s_allocator1.get_info(freeList, objSize, busyObjNum, totalObjNum, heapBytes, g_s_lock1);
        break;
    case 2:
        g_s_allocator2.get_info(freeList, objSize, busyObjNum, totalObjNum, heapBytes, g_s_lock2);
        break;
    case 3:
        g_s_allocator3.get_info(freeList, objSize, busyObjNum, totalObjNum, heapBytes, g_s_lock3);
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
////

class CProSharedDotCpp
{
public:

    CProSharedDotCpp()
    {
        Init_i();
    }
};

static volatile CProSharedDotCpp g_s_initiator;

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
