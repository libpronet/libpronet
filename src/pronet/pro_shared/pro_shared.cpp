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

/*
 * use a smaller stack size, for ProSleep_s(...)
 */
#if defined(_WIN32) || defined(_WIN32_WCE)
#if defined(PRO_FD_SETSIZE)
#undef  PRO_FD_SETSIZE
#endif
#define PRO_FD_SETSIZE 64
#endif

#include "pro_a.h"
#include "pro_shared.h"
#include "pro_bsd_wrapper.h"
#include "pro_version.h"
#include "pro_z.h"
#include "stl_alloc.h"

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <mmsystem.h>
#else
#include <pthread.h>
#if defined(PRO_MACH_ABSOLUTE_TIME)
#include <mach/mach_time.h>
#endif
#endif

#include <cassert>
#include <cstddef>

#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment(lib, "mmtimer.lib")
#elif defined(_WIN32)
#pragma comment(lib, "winmm.lib")
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)

class CProThreadMutex_s
{
public:

    CProThreadMutex_s()
    {
        ::InitializeCriticalSection(&m_cs);
    }

    ~CProThreadMutex_s()
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

#else  /* _WIN32, _WIN32_WCE */

class CProThreadMutex_s
{
public:

    CProThreadMutex_s()
    {
        pthread_mutex_init(&m_mutext, NULL);
    }

    ~CProThreadMutex_s()
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

#endif /* _WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)
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

#if defined(_WIN32) || defined(_WIN32_WCE)
static volatile bool                    g_s_tlsFlag       = false;
static unsigned long                    g_s_tlsKey0       = (unsigned long)-1;
static unsigned long                    g_s_tlsKey1       = (unsigned long)-1;
static PRO_INT64                        g_s_globalTick    = 0;
#elif defined(PRO_MACH_ABSOLUTE_TIME)
static volatile bool                    g_s_timebaseFlag  = false;
static mach_timebase_info_data_t        g_s_timebaseInfo  = { 0, 0 };
#endif
static volatile bool                    g_s_socketFlag    = false;
static PRO_INT64                        g_s_sockId        = -1;
static PRO_UINT64                       g_s_nextTimerId   = 1;
static PRO_UINT64                       g_s_nextMmTimerId = 2;
static CProThreadMutex_s*               g_s_lock          = NULL;

/*
 * pool-0
 */
static std::__default_alloc_template<0> g_s_allocator0;
static CProThreadMutex_s*               g_s_lock0         = NULL;

/*
 * pool-1
 */
static std::__default_alloc_template<1> g_s_allocator1;
static CProThreadMutex_s*               g_s_lock1         = NULL;

/*
 * pool-2
 */
static std::__default_alloc_template<2> g_s_allocator2;
static CProThreadMutex_s*               g_s_lock2         = NULL;

/*
 * pool-3
 */
static std::__default_alloc_template<3> g_s_allocator3;
static CProThreadMutex_s*               g_s_lock3         = NULL;

/*
 * pool-4
 */
static std::__default_alloc_template<4> g_s_allocator4;
static CProThreadMutex_s*               g_s_lock4         = NULL;

/*
 * pool-5
 */
static std::__default_alloc_template<5> g_s_allocator5;
static CProThreadMutex_s*               g_s_lock5         = NULL;

/*
 * pool-6
 */
static std::__default_alloc_template<6> g_s_allocator6;
static CProThreadMutex_s*               g_s_lock6         = NULL;

/*
 * pool-7
 */
static std::__default_alloc_template<7> g_s_allocator7;
static CProThreadMutex_s*               g_s_lock7         = NULL;

/*
 * pool-8
 */
static std::__default_alloc_template<8> g_s_allocator8;
static CProThreadMutex_s*               g_s_lock8         = NULL;

/*
 * pool-9
 */
static std::__default_alloc_template<9> g_s_allocator9;
static CProThreadMutex_s*               g_s_lock9         = NULL;

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

#if defined(_WIN32) || defined(_WIN32_WCE)
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

static
int
pbsd_errno_i()
{
    int errcode = 0;

#if defined(_WIN32) || defined(_WIN32_WCE)
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

    return (errcode);
}

static
int
pbsd_ioctl_closexec_i(PRO_INT64 fd)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)

    do
    {
        retc = fcntl((int)fd, F_GETFD);
    }
    while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);

    if (retc >= 0)
    {
        const int flags = retc | FD_CLOEXEC;

        do
        {
            retc = fcntl((int)fd, F_SETFD, flags);
        }
        while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);
    }

#endif /* _WIN32, _WIN32_WCE */

    return (retc);
}

static
PRO_INT64
pbsd_socket_i(int af,
              int type,
              int protocol)
{
    PRO_INT64 fd = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)

    if (sizeof(SOCKET) == 8)
    {
        fd = (PRO_INT64)socket(af, type, protocol);
    }
    else
    {
        fd = (PRO_INT32)socket(af, type, protocol);
    }

#else  /* _WIN32, _WIN32_WCE */

#if defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        fd        = (PRO_INT32)socket(af, type | SOCK_CLOEXEC, protocol);
        errorcode = pbsd_errno_i();
    }
#endif

    if (fd < 0)
    {
        fd = (PRO_INT32)socket(af, type, protocol);

#if defined(SOCK_CLOEXEC)
        if (fd >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

#endif /* _WIN32, _WIN32_WCE */

    if (fd >= 0)
    {
        pbsd_ioctl_closexec_i(fd);
    }
    else
    {
        fd = -1;
    }

    return (fd);
}

static
int
pbsd_select_i(PRO_INT64       nfds,
              pbsd_fd_set*    readfds,
              pbsd_fd_set*    writefds,
              pbsd_fd_set*    exceptfds,
              struct timeval* timeout)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        retc = select(0, readfds, writefds, exceptfds, timeout);
    }
    while (0);
#else
    do
    {
        retc = select((int)nfds, readfds, writefds, exceptfds, timeout);
    }
    while (retc < 0 && pbsd_errno_i() == PBSD_EINTR);
#endif

    return (retc);
}

static
void
pbsd_closesocket_i(PRO_INT64 fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32) || defined(_WIN32_WCE)
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
        g_s_lock  = new CProThreadMutex_s;
    }
    if (g_s_lock0 == NULL)
    {
        g_s_lock0 = new CProThreadMutex_s;
    }
    if (g_s_lock1 == NULL)
    {
        g_s_lock1 = new CProThreadMutex_s;
    }
    if (g_s_lock2 == NULL)
    {
        g_s_lock2 = new CProThreadMutex_s;
    }
    if (g_s_lock3 == NULL)
    {
        g_s_lock3 = new CProThreadMutex_s;
    }
    if (g_s_lock4 == NULL)
    {
        g_s_lock4 = new CProThreadMutex_s;
    }
    if (g_s_lock5 == NULL)
    {
        g_s_lock5 = new CProThreadMutex_s;
    }
    if (g_s_lock6 == NULL)
    {
        g_s_lock6 = new CProThreadMutex_s;
    }
    if (g_s_lock7 == NULL)
    {
        g_s_lock7 = new CProThreadMutex_s;
    }
    if (g_s_lock8 == NULL)
    {
        g_s_lock8 = new CProThreadMutex_s;
    }
    if (g_s_lock9 == NULL)
    {
        g_s_lock9 = new CProThreadMutex_s;
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
Delay_i(unsigned long milliseconds)
{
    const PRO_INT64 te = ProGetTickCount64_s() + milliseconds;

    while (1)
    {
#if defined(_WIN32) || defined(_WIN32_WCE)
        ::Sleep(1);
#else
        usleep(500);
#endif

        if (ProGetTickCount64_s() >= te)
        {
            break;
        }
    }
}

static
PRO_UINT32
GetTickCount32_i()
{
    Init_i();

    PRO_INT64 ret = 0;

#if defined(_WIN32) || defined(_WIN32_WCE)

    ret = ::timeGetTime();

#elif defined(PRO_MACH_ABSOLUTE_TIME)

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

#elif !defined(PRO_LACKS_CLOCK_GETTIME)

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    ret =  ts.tv_sec;
    ret *= 1000;
    ret += ts.tv_nsec / 1000000;

#endif

    return ((PRO_UINT32)ret);
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
    PRO_INT64 seconds = GetTickCount32_i();
    seconds /=  1000;
    seconds <<= 24;

    PRO_INT64 seed = time(NULL);
    seed += seconds;

    srand((unsigned int)seed);
}

PRO_SHARED_API
double
ProRand_0_1()
{
    return ((double)rand() / RAND_MAX);
}

PRO_SHARED_API
int
ProRand_0_32767()
{
    return (rand());
}

PRO_SHARED_API
PRO_INT64
ProGetTickCount64_s()
{
    Init_i();

#if defined(_WIN32) || defined(_WIN32_WCE)

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

    PRO_UINT32 tick0 = (PRO_UINT32)(PRO_UINT64)::TlsGetValue(g_s_tlsKey0);
    PRO_UINT32 tick1 = (PRO_UINT32)(PRO_UINT64)::TlsGetValue(g_s_tlsKey1);
    if (tick0 == 0 && tick1 == 0)
    {
        g_s_lock->Lock();
        tick0 = (PRO_UINT32)g_s_globalTick;
        tick1 = (PRO_UINT32)(g_s_globalTick >> 32);
        g_s_lock->Unlock();

        ::TlsSetValue(g_s_tlsKey0, (void*)(PRO_UINT64)tick0);
        ::TlsSetValue(g_s_tlsKey1, (void*)(PRO_UINT64)tick1);

        updateGlobalTick = true;
    }

    const PRO_UINT32 tick = ::timeGetTime();
    if (tick > tick0)
    {
        tick0 = tick;
        ::TlsSetValue(g_s_tlsKey0, (void*)(PRO_UINT64)tick0);
    }
    else if (tick < tick0)
    {
        tick0 = tick;
        ++tick1;
        ::TlsSetValue(g_s_tlsKey0, (void*)(PRO_UINT64)tick0);
        ::TlsSetValue(g_s_tlsKey1, (void*)(PRO_UINT64)tick1);

        updateGlobalTick = true;
    }
    else
    {
    }

    PRO_INT64 ret = tick1;
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

    return (ret);

#elif defined(PRO_MACH_ABSOLUTE_TIME)   /* for MacOS */

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

    PRO_INT64 ret = mach_absolute_time();
    ret =  ret * g_s_timebaseInfo.numer / g_s_timebaseInfo.denom; /* ns_ticks ---> ns */
    ret /= 1000000;                                               /* ns       ---> ms */

    return (ret);

#elif !defined(PRO_LACKS_CLOCK_GETTIME) /* for non-MacOS */

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    PRO_INT64 ret = ts.tv_sec;
    ret *= 1000;
    ret += ts.tv_nsec / 1000000;

    return (ret);

#else

    return (0);

#endif
}

PRO_SHARED_API
void
ProSleep_s(PRO_UINT32 milliseconds)
{
    Init_i();

    if (milliseconds == 0)
    {
#if defined(_WIN32) || defined(_WIN32_WCE)
        ::Sleep(0);
#else
        usleep(0);
#endif

        return;
    }

    if (milliseconds == (PRO_UINT32)-1)
    {
        while (1)
        {
#if defined(_WIN32) || defined(_WIN32_WCE)
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

    const PRO_INT64 te = ProGetTickCount64_s() + milliseconds;

    while (1)
    {
        const PRO_INT64 t = ProGetTickCount64_s();
        if (t >= te)
        {
            break;
        }

        PRO_INT64 sockId = g_s_sockId;
        if (sockId == -1)
        {
            g_s_lock->Lock();
            FiniSocket_i();
            InitSocket_i();
            g_s_lock->Unlock();

            Delay_i(1);
            continue;
        }

#if !defined(_WIN32) && !defined(_WIN32_WCE)
        /*
         * the descriptor value of the STDIN is less than FD_SETSIZE
         */
        if (sockId >= FD_SETSIZE)
        {
            sockId = 0; /* we use STDIN */
        }
#endif

        pbsd_fd_set fds;
        PBSD_FD_ZERO(&fds);
        PBSD_FD_SET(sockId, &fds);

        struct timeval tv;
        if (te - t <= 5) /* 5ms */
        {
            tv.tv_sec  = 0;
            tv.tv_usec = 500;
        }
        else
        {
            tv.tv_sec  = (long)((te - t - 5) / 1000);        /* assert(0 <= tv_sec  <= 0x7FFFFFFF) */
            tv.tv_usec = (long)((te - t - 5) % 1000 * 1000); /* assert(0 <= tv_usec <= 0x7FFFFFFF) */
        }

        const int retc = pbsd_select_i(sockId + 1, NULL, NULL, &fds, &tv);

        assert(retc == 0);
        if (retc != 0)
        {
            g_s_lock->Lock();
            FiniSocket_i();
            InitSocket_i();
            g_s_lock->Unlock();

            Delay_i(1);
        }
    } /* end of while (...) */
}

PRO_SHARED_API
PRO_UINT64
ProMakeTimerId()
{
    Init_i();

    g_s_lock->Lock();

    const PRO_UINT64 timerId = g_s_nextTimerId;
    g_s_nextTimerId += 2;

    g_s_lock->Unlock();

    return (timerId);
}

PRO_SHARED_API
PRO_UINT64
ProMakeMmTimerId()
{
    Init_i();

    g_s_lock->Lock();

    const PRO_UINT64 timerId = g_s_nextMmTimerId;
    g_s_nextMmTimerId += 2;
    if (g_s_nextMmTimerId == 0)
    {
        g_s_nextMmTimerId += 2;
    }

    g_s_lock->Unlock();

    return (timerId);
}

PRO_SHARED_API
void*
ProAllocateSgiPoolBuffer(size_t        size,
                         unsigned long poolIndex) /* 0 ~ 9 */
{
    Init_i();

    assert(poolIndex <= 9);
    if (poolIndex > 9)
    {
        return (NULL);
    }

    if (size == 0)
    {
        size = sizeof(PRO_UINT32) + sizeof(PRO_UINT32) + 1;
    }
    else
    {
        size = sizeof(PRO_UINT32) + sizeof(PRO_UINT32) + size;
    }

    PRO_UINT32* p = NULL;

    switch (poolIndex)
    {
    case 0:
        {
            g_s_lock0->Lock();
            p = (PRO_UINT32*)g_s_allocator0.allocate(size);
            g_s_lock0->Unlock();
            break;
        }
    case 1:
        {
            g_s_lock1->Lock();
            p = (PRO_UINT32*)g_s_allocator1.allocate(size);
            g_s_lock1->Unlock();
            break;
        }
    case 2:
        {
            g_s_lock2->Lock();
            p = (PRO_UINT32*)g_s_allocator2.allocate(size);
            g_s_lock2->Unlock();
            break;
        }
    case 3:
        {
            g_s_lock3->Lock();
            p = (PRO_UINT32*)g_s_allocator3.allocate(size);
            g_s_lock3->Unlock();
            break;
        }
    case 4:
        {
            g_s_lock4->Lock();
            p = (PRO_UINT32*)g_s_allocator4.allocate(size);
            g_s_lock4->Unlock();
            break;
        }
    case 5:
        {
            g_s_lock5->Lock();
            p = (PRO_UINT32*)g_s_allocator5.allocate(size);
            g_s_lock5->Unlock();
            break;
        }
    case 6:
        {
            g_s_lock6->Lock();
            p = (PRO_UINT32*)g_s_allocator6.allocate(size);
            g_s_lock6->Unlock();
            break;
        }
    case 7:
        {
            g_s_lock7->Lock();
            p = (PRO_UINT32*)g_s_allocator7.allocate(size);
            g_s_lock7->Unlock();
            break;
        }
    case 8:
        {
            g_s_lock8->Lock();
            p = (PRO_UINT32*)g_s_allocator8.allocate(size);
            g_s_lock8->Unlock();
            break;
        }
    case 9:
        {
            g_s_lock9->Lock();
            p = (PRO_UINT32*)g_s_allocator9.allocate(size);
            g_s_lock9->Unlock();
            break;
        }
    } /* end of switch (...) */

    if (p == NULL)
    {
        return (NULL);
    }

    *p = (PRO_UINT32)size;

    return (p + 2);
}

PRO_SHARED_API
void*
ProReallocateSgiPoolBuffer(void*         buf,
                           size_t        newSize,
                           unsigned long poolIndex) /* 0 ~ 9 */
{
    Init_i();

    assert(poolIndex <= 9);
    if (poolIndex > 9)
    {
        return (NULL);
    }

    if (buf == NULL)
    {
        return (ProAllocateSgiPoolBuffer(newSize, poolIndex));
    }

    if (newSize == 0)
    {
        ProDeallocateSgiPoolBuffer(buf, poolIndex);

        return (NULL);
    }

    PRO_UINT32* const p = (PRO_UINT32*)buf - 2;
    if (*p < sizeof(PRO_UINT32) + sizeof(PRO_UINT32) + 1)
    {
        return (NULL);
    }

    newSize = sizeof(PRO_UINT32) + sizeof(PRO_UINT32) + newSize;

    PRO_UINT32* q = NULL;

    switch (poolIndex)
    {
    case 0:
        {
            g_s_lock0->Lock();
            q = (PRO_UINT32*)g_s_allocator0.reallocate(p, *p, newSize);
            g_s_lock0->Unlock();
            break;
        }
    case 1:
        {
            g_s_lock1->Lock();
            q = (PRO_UINT32*)g_s_allocator1.reallocate(p, *p, newSize);
            g_s_lock1->Unlock();
            break;
        }
    case 2:
        {
            g_s_lock2->Lock();
            q = (PRO_UINT32*)g_s_allocator2.reallocate(p, *p, newSize);
            g_s_lock2->Unlock();
            break;
        }
    case 3:
        {
            g_s_lock3->Lock();
            q = (PRO_UINT32*)g_s_allocator3.reallocate(p, *p, newSize);
            g_s_lock3->Unlock();
            break;
        }
    case 4:
        {
            g_s_lock4->Lock();
            q = (PRO_UINT32*)g_s_allocator4.reallocate(p, *p, newSize);
            g_s_lock4->Unlock();
            break;
        }
    case 5:
        {
            g_s_lock5->Lock();
            q = (PRO_UINT32*)g_s_allocator5.reallocate(p, *p, newSize);
            g_s_lock5->Unlock();
            break;
        }
    case 6:
        {
            g_s_lock6->Lock();
            q = (PRO_UINT32*)g_s_allocator6.reallocate(p, *p, newSize);
            g_s_lock6->Unlock();
            break;
        }
    case 7:
        {
            g_s_lock7->Lock();
            q = (PRO_UINT32*)g_s_allocator7.reallocate(p, *p, newSize);
            g_s_lock7->Unlock();
            break;
        }
    case 8:
        {
            g_s_lock8->Lock();
            q = (PRO_UINT32*)g_s_allocator8.reallocate(p, *p, newSize);
            g_s_lock8->Unlock();
            break;
        }
    case 9:
        {
            g_s_lock9->Lock();
            q = (PRO_UINT32*)g_s_allocator9.reallocate(p, *p, newSize);
            g_s_lock9->Unlock();
            break;
        }
    } /* end of switch (...) */

    if (q == NULL)
    {
        return (NULL);
    }

    *q = (PRO_UINT32)newSize;

    return (q + 2);
}

PRO_SHARED_API
void
ProDeallocateSgiPoolBuffer(void*         buf,
                           unsigned long poolIndex) /* 0 ~ 9 */
{
    Init_i();

    assert(poolIndex <= 9);
    if (buf == NULL || poolIndex > 9)
    {
        return;
    }

    PRO_UINT32* const p = (PRO_UINT32*)buf - 2;
    if (*p < sizeof(PRO_UINT32) + sizeof(PRO_UINT32) + 1)
    {
        return;
    }

    switch (poolIndex)
    {
    case 0:
        {
            g_s_lock0->Lock();
            g_s_allocator0.deallocate(p, *p);
            g_s_lock0->Unlock();
            break;
        }
    case 1:
        {
            g_s_lock1->Lock();
            g_s_allocator1.deallocate(p, *p);
            g_s_lock1->Unlock();
            break;
        }
    case 2:
        {
            g_s_lock2->Lock();
            g_s_allocator2.deallocate(p, *p);
            g_s_lock2->Unlock();
            break;
        }
    case 3:
        {
            g_s_lock3->Lock();
            g_s_allocator3.deallocate(p, *p);
            g_s_lock3->Unlock();
            break;
        }
    case 4:
        {
            g_s_lock4->Lock();
            g_s_allocator4.deallocate(p, *p);
            g_s_lock4->Unlock();
            break;
        }
    case 5:
        {
            g_s_lock5->Lock();
            g_s_allocator5.deallocate(p, *p);
            g_s_lock5->Unlock();
            break;
        }
    case 6:
        {
            g_s_lock6->Lock();
            g_s_allocator6.deallocate(p, *p);
            g_s_lock6->Unlock();
            break;
        }
    case 7:
        {
            g_s_lock7->Lock();
            g_s_allocator7.deallocate(p, *p);
            g_s_lock7->Unlock();
            break;
        }
    case 8:
        {
            g_s_lock8->Lock();
            g_s_allocator8.deallocate(p, *p);
            g_s_lock8->Unlock();
            break;
        }
    case 9:
        {
            g_s_lock9->Lock();
            g_s_allocator9.deallocate(p, *p);
            g_s_lock9->Unlock();
            break;
        }
    } /* end of switch (...) */
}

PRO_SHARED_API
void
ProGetSgiPoolInfo(void*         freeList[64],
                  size_t        objSize[64],
                  size_t        busyObjNum[64],
                  size_t        totalObjNum[64],
                  size_t*       heapBytes, /* = NULL */
                  unsigned long poolIndex) /* 0 ~ 9 */
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

    assert(poolIndex <= 9);
    if (poolIndex > 9)
    {
        return;
    }

    switch (poolIndex)
    {
    case 0:
        {
            g_s_lock0->Lock();
            g_s_allocator0.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock0->Unlock();
            break;
        }
    case 1:
        {
            g_s_lock1->Lock();
            g_s_allocator1.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock1->Unlock();
            break;
        }
    case 2:
        {
            g_s_lock2->Lock();
            g_s_allocator2.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock2->Unlock();
            break;
        }
    case 3:
        {
            g_s_lock3->Lock();
            g_s_allocator3.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock3->Unlock();
            break;
        }
    case 4:
        {
            g_s_lock4->Lock();
            g_s_allocator4.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock4->Unlock();
            break;
        }
    case 5:
        {
            g_s_lock5->Lock();
            g_s_allocator5.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock5->Unlock();
            break;
        }
    case 6:
        {
            g_s_lock6->Lock();
            g_s_allocator6.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock6->Unlock();
            break;
        }
    case 7:
        {
            g_s_lock7->Lock();
            g_s_allocator7.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock7->Unlock();
            break;
        }
    case 8:
        {
            g_s_lock8->Lock();
            g_s_allocator8.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock8->Unlock();
            break;
        }
    case 9:
        {
            g_s_lock9->Lock();
            g_s_allocator9.get_info(
                freeList, objSize, busyObjNum, totalObjNum, heapBytes);
            g_s_lock9->Unlock();
            break;
        }
    } /* end of switch (...) */
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
