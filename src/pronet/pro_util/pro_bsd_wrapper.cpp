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
#include "pro_bsd_wrapper.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_time_util.h"
#include "pro_z.h"
#include "../pro_shared/pro_shared.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define PBSD_EPOLL_SIZE   10000

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

#if !defined(PRO_LACKS_GETADDRINFO)
#define PBSD_GET_IPS      pbsd_getaddrinfo_i
#else
#define PBSD_GET_IPS      pbsd_gethostbyname_r_i
#endif

/////////////////////////////////////////////////////////////////////////////
////

static
PRO_UINT32
PRO_CALLTYPE
pbsd_inet_addr_i(const char* ipstring)
{
    if (ipstring == NULL || ipstring[0] == '\0')
    {
        return (0);
    }

    CProStlString ipstring2 = ipstring;

    if (ipstring2.find_first_not_of("0123456789.") != CProStlString::npos)
    {
        return ((PRO_UINT32)-1);
    }

    const char* p[4] = { NULL, NULL, NULL, NULL };
    const char* q1   = &ipstring2[0];
    char*       q2   = (char*)q1;

    for (int i = 0; ; )
    {
        if (i > 3)
        {
            p[0] = NULL;
            p[1] = NULL;
            p[2] = NULL;
            p[3] = NULL;
            break;
        }

        if (*q2 == '\0')
        {
            p[i] = q1;
            break;
        }

        if (*q2 == '.')
        {
            *q2++  = '\0';
            p[i++] = q1;
            q1     = q2;
        }
        else
        {
            ++q2;
            if (q2 - q1 > 3) /* xxx.xxx.xxx.xxx */
            {
                break;
            }
        }
    } /* end of for (...) */

    if (p[0] == NULL || p[0][0] == '\0' ||
        p[1] == NULL || p[1][0] == '\0' ||
        p[2] == NULL || p[2][0] == '\0' ||
        p[3] == NULL || p[3][0] == '\0')
    {
        return ((PRO_UINT32)-1);
    }

    PRO_UINT32 b[4];
    b[0] = (PRO_UINT32)atoi(p[0]);
    b[1] = (PRO_UINT32)atoi(p[1]);
    b[2] = (PRO_UINT32)atoi(p[2]);
    b[3] = (PRO_UINT32)atoi(p[3]);

    if (b[0] > 255 || b[1] > 255 || b[2] > 255 || b[3] > 255)
    {
        return ((PRO_UINT32)-1);
    }

    PRO_UINT32 ip = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]; /* host byte order */
    ip = pbsd_hton32(ip);                                             /* network byte order */

    return (ip);
}

#if !defined(PRO_LACKS_GETADDRINFO)

static
unsigned long
PRO_CALLTYPE
pbsd_getaddrinfo_i(const char* name,
                   PRO_UINT32  ips[8])
{
    if (name == NULL || name[0] == '\0')
    {
        return (0);
    }

    const PRO_UINT32 loopmin = pbsd_ntoh32(pbsd_inet_addr_i("127.0.0.0"));
    const PRO_UINT32 loopmax = pbsd_ntoh32(pbsd_inet_addr_i("127.255.255.255"));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo*        res  = NULL;
    struct addrinfo*        itr  = NULL;
    const pbsd_sockaddr_in* addr = NULL;

    if (getaddrinfo(name, NULL, &hints, &res) != 0)
    {
        return (0);
    }

    unsigned long count = 0;

    itr = res;

    for (int i = 0; i < 8 && itr != NULL; ++i, itr = itr->ai_next)
    {
        if (itr->ai_family != AF_INET || itr->ai_socktype != SOCK_DGRAM ||
            itr->ai_addrlen != sizeof(pbsd_sockaddr_in) ||
            itr->ai_addr == NULL)
        {
            continue;
        }

        addr       = (pbsd_sockaddr_in*)itr->ai_addr;
        ips[count] = addr->sin_addr.s_addr;

        const PRO_UINT32 iphost = pbsd_ntoh32(ips[count]);
        if (iphost < loopmin || iphost > loopmax)
        {
            ++count;
        }
    }

    freeaddrinfo(res);

    return (count);
}

#else  /* PRO_LACKS_GETADDRINFO */

static
unsigned long
PRO_CALLTYPE
pbsd_gethostbyname_r_i(const char* name,
                       PRO_UINT32  ips[8])
{
    if (name == NULL || name[0] == '\0')
    {
        return (0);
    }

    const PRO_UINT32 loopmin = pbsd_ntoh32(pbsd_inet_addr_i("127.0.0.0"));
    const PRO_UINT32 loopmax = pbsd_ntoh32(pbsd_inet_addr_i("127.255.255.255"));

    struct hostent* phe = NULL;

#if defined(_WIN32) || defined(_WIN32_WCE)
    phe = gethostbyname(name);
#elif !defined(PRO_LACKS_GETHOSTBYNAME_R) /* for non-MacOS */
    struct hostent he;
    char           buf[1024];
    int            errcode = 0;
    if (gethostbyname_r(name, &he, buf, sizeof(buf), &phe, &errcode) != 0 ||
        phe != &he)
    {
        phe = NULL;
    }
#else                                     /* for MacOS */
    sethostent(0);
    phe = gethostent();
    endhostent();
#endif

    if (phe == NULL || phe->h_addrtype != AF_INET || phe->h_addr_list == NULL)
    {
        return (0);
    }

    unsigned long count = 0;

    for (int i = 0; i < 8; ++i)
    {
        if (phe->h_addr_list[i] == NULL)
        {
            break;
        }

        ips[count] = ((struct in_addr*)phe->h_addr_list[i])->s_addr;

        const PRO_UINT32 iphost = pbsd_ntoh32(ips[count]);
        if (iphost < loopmin || iphost > loopmax)
        {
            ++count;
        }
    }

    return (count);
}

#endif /* PRO_LACKS_GETADDRINFO */

static
void
PRO_CALLTYPE
GetLocalFirstIpWithGW_i(char        localFirstIp[64],
                        const char* peerIpOrName) /* = NULL */
{
    localFirstIp[0]  = '\0';
    localFirstIp[63] = '\0';

    const PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockId == -1)
    {
        return;
    }

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(80);
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(peerIpOrName);
    if (remoteAddr.sin_addr.s_addr == (PRO_UINT32)-1 ||
        remoteAddr.sin_addr.s_addr == 0)
    {
        remoteAddr.sin_addr.s_addr = pbsd_inet_addr_i("8.8.8.8");
    }

    int retc = pbsd_connect(sockId, &remoteAddr);
    if (retc != 0 && pbsd_errno((void*)&pbsd_connect) != PBSD_EINPROGRESS)
    {
        pbsd_closesocket(sockId);

        return;
    }

    pbsd_sockaddr_in localAddr;
    retc = pbsd_getsockname(sockId, &localAddr);
    pbsd_closesocket(sockId);

    if (retc == 0)
    {
        pbsd_inet_ntoa(localAddr.sin_addr.s_addr, localFirstIp);
    }
}

static
void
PRO_CALLTYPE
GetLocalFirstIpWithoutGW_i(char localFirstIp[64])
{
    localFirstIp[0]  = '\0';
    localFirstIp[63] = '\0';

    char name[256] = "";
    name[sizeof(name) - 1] = '\0';

    if (pbsd_gethostname(name, sizeof(name) - 1) != 0)
    {
        return;
    }

    PRO_UINT32          ips[8];
    const unsigned long count = PBSD_GET_IPS(name, ips);

    if (count > 0)
    {
        pbsd_inet_ntoa(ips[0], localFirstIp);
    }
}

/////////////////////////////////////////////////////////////////////////////
////

void
PRO_CALLTYPE
pbsd_startup()
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
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, NULL); /* SIGPIPE!!! */
#endif

    setlocale(LC_ALL, "");
    ProSrand();
    ProSleep(1);
}

int
PRO_CALLTYPE
pbsd_errno(void* action) /* = NULL */
{
    int errcode = 0;

#if defined(_WIN32) || defined(_WIN32_WCE)

    errcode = ::WSAGetLastError();

    if (errcode == PBSD_EWOULDBLOCK && action == &pbsd_connect)
    {
        errcode = PBSD_EINPROGRESS;
    }

#else  /* _WIN32, _WIN32_WCE */

    errcode = errno;

#endif /* _WIN32, _WIN32_WCE */

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

int
PRO_CALLTYPE
pbsd_gethostname(char* name,
                 int   namelen)
{
    return (gethostname(name, namelen));
}

PRO_UINT32
PRO_CALLTYPE
pbsd_inet_aton(const char* ipornamestring)
{
    const char* const loopipstring = "127.0.0.1";
    if (ipornamestring != NULL && stricmp(ipornamestring, "localhost") == 0)
    {
        ipornamestring = loopipstring;
    }

    const PRO_UINT32 ip = pbsd_inet_addr_i(ipornamestring);
    if (ip != (PRO_UINT32)-1)
    {
        return (ip);
    }

    PRO_UINT32          ips[8];
    const unsigned long count = PBSD_GET_IPS(ipornamestring, ips);
    if (count > 0)
    {
        return (ips[0]);
    }

    return ((PRO_UINT32)-1);
}

const char*
PRO_CALLTYPE
pbsd_inet_ntoa(PRO_UINT32 ip,
               char       ipstring[64])
{
    ip = pbsd_ntoh32(ip);

    sprintf(
        ipstring,
        "%u.%u.%u.%u",
        (unsigned int)((ip >> 24) & 0xFF),
        (unsigned int)((ip >> 16) & 0xFF),
        (unsigned int)((ip >>  8) & 0xFF),
        (unsigned int)((ip >>  0) & 0xFF)
        );

    return (ipstring);
}

PRO_UINT16
PRO_CALLTYPE
pbsd_hton16(PRO_UINT16 host16)
{
    PRO_UINT16 net16 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net16 = host16;
#else
    net16 |= (host16 >> 8) & 0x00FF;
    net16 |= (host16 << 8) & 0xFF00;
#endif

    return (net16);
}

PRO_UINT16
PRO_CALLTYPE
pbsd_ntoh16(PRO_UINT16 net16)
{
    return (pbsd_hton16(net16));
}

PRO_UINT32
PRO_CALLTYPE
pbsd_hton32(PRO_UINT32 host32)
{
    PRO_UINT32 net32 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net32 = host32;
#else
    net32 |= (host32 >> 24) & 0x000000FF;
    net32 |= (host32 >>  8) & 0x0000FF00;
    net32 |= (host32 <<  8) & 0x00FF0000;
    net32 |= (host32 << 24) & 0xFF000000;
#endif

    return (net32);
}

PRO_UINT32
PRO_CALLTYPE
pbsd_ntoh32(PRO_UINT32 net32)
{
    return (pbsd_hton32(net32));
}

PRO_UINT64
PRO_CALLTYPE
pbsd_hton64(PRO_UINT64 host64)
{
    PRO_UINT64 net64 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net64 = host64;
#else
    const PRO_UINT32 high = pbsd_hton32((PRO_UINT32)(host64 >> 32));
    const PRO_UINT32 low  = pbsd_hton32((PRO_UINT32)host64);
    net64 =   low;
    net64 <<= 32;
    net64 |=  high;
#endif

    return (net64);
}

PRO_UINT64
PRO_CALLTYPE
pbsd_ntoh64(PRO_UINT64 net64)
{
    return (pbsd_hton64(net64));
}

PRO_INT64
PRO_CALLTYPE
pbsd_socket(int af,
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
        errorcode = pbsd_errno((void*)&pbsd_socket);
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
        pbsd_ioctl_nonblock(fd);
        pbsd_ioctl_closexec(fd);

        if (af == AF_INET && type == SOCK_DGRAM)
        {
            const int option = 1;
            pbsd_setsockopt(
                fd, SOL_SOCKET, SO_BROADCAST, &option, sizeof(int));

#if defined(_WIN32) || defined(_WIN32_WCE)
            long          arg           = 0;
            unsigned long bytesReturned = 0;
            ::WSAIoctl((SOCKET)fd, (unsigned long)SIO_UDP_CONNRESET,
                &arg, sizeof(long), NULL, 0, &bytesReturned, NULL, NULL);
#endif
        }
    }
    else
    {
        fd = -1;
    }

    return (fd);
}

int
PRO_CALLTYPE
pbsd_socketpair(PRO_INT64 fds[2])
{
    fds[0] = -1;
    fds[1] = -1;

    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)

    int fds2[2] = { -1, -1 };

#if defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        retc      = socketpair(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0, fds2);
        errorcode = pbsd_errno((void*)&pbsd_socketpair);
    }
#endif

    if (retc != 0)
    {
        retc = socketpair(AF_LOCAL, SOCK_STREAM, 0, fds2);

#if defined(SOCK_CLOEXEC)
        if (retc == 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

    if (retc == 0 && fds2[0] >= 0)
    {
        pbsd_ioctl_nonblock(fds2[0]);
        pbsd_ioctl_closexec(fds2[0]);
    }
    else
    {
        fds2[0] = -1;
    }

    if (retc == 0 && fds2[1] >= 0)
    {
        pbsd_ioctl_nonblock(fds2[1]);
        pbsd_ioctl_closexec(fds2[1]);
    }
    else
    {
        fds2[1] = -1;
    }

    fds[0] = fds2[0];
    fds[1] = fds2[1];

#endif /* _WIN32, _WIN32_WCE */

    return (retc);
}

int
PRO_CALLTYPE
pbsd_ioctl_nonblock(PRO_INT64 fd,
                    long      on) /* = true */
{
    if (on != 0)
    {
        on = (long)((unsigned long)-1 >> 1);
    }

    int retc1 = -1;
    int retc2 = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)

    do
    {
        unsigned long bytesReturned = 0;
        retc1 = ::WSAIoctl((SOCKET)fd, (unsigned long)FIONBIO,
            &on, sizeof(long), NULL, 0, &bytesReturned, NULL, NULL);
    }
    while (0);

#else  /* _WIN32, _WIN32_WCE */

    do
    {
        retc1 = ioctl((int)fd, (unsigned long)FIONBIO, &on);
    }
    while (retc1 < 0 && pbsd_errno((void*)&pbsd_ioctl_nonblock) == PBSD_EINTR);

    do
    {
        retc2 = fcntl((int)fd, F_GETFL);
    }
    while (retc2 < 0 && pbsd_errno((void*)&pbsd_ioctl_nonblock) == PBSD_EINTR);

    if (retc2 >= 0)
    {
        int flags = retc2;
        if (on != 0)
        {
            flags |= O_NONBLOCK;
        }
        else
        {
            flags &= ~O_NONBLOCK;
        }

        do
        {
            retc2 = fcntl((int)fd, F_SETFL, flags);
        }
        while (retc2 < 0 &&
            pbsd_errno((void*)&pbsd_ioctl_nonblock) == PBSD_EINTR);
    }

#endif /* _WIN32, _WIN32_WCE */

    if (retc1 < 0 && retc2 < 0)
    {
        return (-1);
    }
    else
    {
        return (0);
    }
}

int
PRO_CALLTYPE
pbsd_ioctl_closexec(PRO_INT64 fd,
                    long      on) /* = true */
{
    if (on != 0)
    {
        on = (long)((unsigned long)-1 >> 1);
    }

    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)

    do
    {
        retc = fcntl((int)fd, F_GETFD);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_ioctl_closexec) == PBSD_EINTR);

    if (retc >= 0)
    {
        int flags = retc;
        if (on != 0)
        {
            flags |= FD_CLOEXEC;
        }
        else
        {
            flags &= ~FD_CLOEXEC;
        }

        do
        {
            retc = fcntl((int)fd, F_SETFD, flags);
        }
        while (retc < 0 &&
            pbsd_errno((void*)&pbsd_ioctl_closexec) == PBSD_EINTR);
    }

#endif /* _WIN32, _WIN32_WCE */

    return (retc);
}

int
PRO_CALLTYPE
pbsd_setsockopt(PRO_INT64   fd,
                int         level,
                int         optname,
                const void* optval,
                int         optlen)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    retc = setsockopt((SOCKET)fd, level, optname, (char*)optval, optlen);
#else
    retc = setsockopt((int)fd, level, optname, optval, optlen);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_getsockopt(PRO_INT64 fd,
                int       level,
                int       optname,
                void*     optval,
                int*      optlen)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    retc = getsockopt((SOCKET)fd, level, optname, (char*)optval, optlen);
#else
    socklen_t optlen2 = 0;
    if (optlen != NULL)
    {
        optlen2 = *optlen;
    }
    retc = getsockopt(
        (int)fd, level, optname, optval, optlen != NULL ? &optlen2 : NULL);
    if (optlen != NULL)
    {
        *optlen = optlen2;
    }
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_getsockname(PRO_INT64         fd,
                 pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    int addrlen = sizeof(pbsd_sockaddr_in);
    retc = getsockname(
        (SOCKET)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#else
    socklen_t addrlen = sizeof(pbsd_sockaddr_in);
    retc = getsockname(
        (int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_getsockname_un(PRO_INT64         fd,
                    pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    socklen_t addrlen = sizeof(pbsd_sockaddr_un);
    retc = getsockname(
        (int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_getpeername(PRO_INT64         fd,
                 pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    int addrlen = sizeof(pbsd_sockaddr_in);
    retc = getpeername(
        (SOCKET)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#else
    socklen_t addrlen = sizeof(pbsd_sockaddr_in);
    retc = getpeername(
        (int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_getpeername_un(PRO_INT64         fd,
                    pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    socklen_t addrlen = sizeof(pbsd_sockaddr_un);
    retc = getpeername(
        (int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_bind(PRO_INT64               fd,
          const pbsd_sockaddr_in* addr,
          bool                    reuseaddr)
{
    if (reuseaddr)
    {
        const int option = 1;
        pbsd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
    }
    else
    {
#if defined(_WIN32) || defined(_WIN32_WCE)
        const int option = 1;
        pbsd_setsockopt(
            fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &option, sizeof(int));
#endif
    }

    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    retc = bind((SOCKET)fd,
        (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
#else
    retc = bind((int)fd,
        (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_bind_un(PRO_INT64               fd,
             const pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    retc = bind((int)fd,
        (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_un) : 0);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_listen(PRO_INT64 fd)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    retc = listen((SOCKET)fd, SOMAXCONN);
#else
    retc = listen((int)fd, SOMAXCONN);
#endif

    return (retc);
}

PRO_INT64
PRO_CALLTYPE
pbsd_accept(PRO_INT64         fd,
            pbsd_sockaddr_in* addr)
{
    PRO_INT64 newfd = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)

    do
    {
        int addrlen = sizeof(pbsd_sockaddr_in);
        if (sizeof(SOCKET) == 8)
        {
            newfd = (PRO_INT64)accept((SOCKET)fd,
                (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
        }
        else
        {
            newfd = (PRO_INT32)accept((SOCKET)fd,
                (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
        }
    }
    while (0);

#else  /* _WIN32, _WIN32_WCE */

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_in);
            newfd     = (PRO_INT32)accept4((int)fd, (struct sockaddr*)addr,
                addr != NULL ? &addrlen : NULL, SOCK_CLOEXEC);
            errorcode = pbsd_errno((void*)&pbsd_accept);
        }
        while (newfd < 0 && errorcode == PBSD_EINTR);
    }
#endif

    if (newfd < 0)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_in);
            newfd = (PRO_INT32)accept((int)fd,
                (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
        }
        while (newfd < 0 && pbsd_errno((void*)&pbsd_accept) == PBSD_EINTR);

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
        if (newfd >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

#endif /* _WIN32, _WIN32_WCE */

    if (newfd >= 0)
    {
        pbsd_ioctl_nonblock(newfd);
        pbsd_ioctl_closexec(newfd);
    }
    else
    {
        newfd = -1;
    }

    return (newfd);
}

PRO_INT64
PRO_CALLTYPE
pbsd_accept_un(PRO_INT64         fd,
               pbsd_sockaddr_un* addr)
{
    PRO_INT64 newfd = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_un);
            newfd     = (PRO_INT32)accept4((int)fd, (struct sockaddr*)addr,
                addr != NULL ? &addrlen : NULL, SOCK_CLOEXEC);
            errorcode = pbsd_errno((void*)&pbsd_accept_un);
        }
        while (newfd < 0 && errorcode == PBSD_EINTR);
    }
#endif

    if (newfd < 0)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_un);
            newfd = (PRO_INT32)accept((int)fd,
                (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
        }
        while (newfd < 0 && pbsd_errno((void*)&pbsd_accept_un) == PBSD_EINTR);

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
        if (newfd >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

    if (newfd >= 0)
    {
        pbsd_ioctl_nonblock(newfd);
        pbsd_ioctl_closexec(newfd);
    }
    else
    {
        newfd = -1;
    }

#endif /* _WIN32, _WIN32_WCE */

    return (newfd);
}

int
PRO_CALLTYPE
pbsd_connect(PRO_INT64               fd,
             const pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        retc = connect((SOCKET)fd, (struct sockaddr*)addr,
            addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (0);
#else
    do
    {
        retc = connect((int)fd, (struct sockaddr*)addr,
            addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (retc != 0 && pbsd_errno((void*)&pbsd_connect) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_connect_un(PRO_INT64               fd,
                const pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    do
    {
        retc = connect((int)fd, (struct sockaddr*)addr,
            addr != NULL ? sizeof(pbsd_sockaddr_un) : 0);
    }
    while (retc != 0 && pbsd_errno((void*)&pbsd_connect_un) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_send(PRO_INT64   fd,
          const void* buf,
          int         buflen,
          int         flags)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        retc = send((SOCKET)fd, (char*)buf, buflen, flags);
    }
    while (0);
#else
    do
    {
        retc = send((int)fd, buf, buflen, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_send) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_sendto(PRO_INT64               fd,
            const void*             buf,
            int                     buflen,
            int                     flags,
            const pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        retc = sendto(
            (SOCKET)fd, (char*)buf, buflen, flags, (struct sockaddr*)addr,
            addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (0);
#else
    do
    {
        retc = sendto(
            (int)fd, buf, buflen, flags, (struct sockaddr*)addr,
            addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_sendto) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_sendmsg(PRO_INT64          fd,
             const pbsd_msghdr* msg,
             int                flags)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    do
    {
        retc = sendmsg((int)fd, msg, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_sendmsg) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_recv(PRO_INT64 fd,
          void*     buf,
          int       buflen,
          int       flags)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        retc = recv((SOCKET)fd, (char*)buf, buflen, flags);
    }
    while (0);
#else
    do
    {
        retc = recv((int)fd, buf, buflen, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_recv) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_recvfrom(PRO_INT64         fd,
              void*             buf,
              int               buflen,
              int               flags,
              pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32) || defined(_WIN32_WCE)
    do
    {
        int addrlen = sizeof(pbsd_sockaddr_in);
        retc = recvfrom((SOCKET)fd, (char*)buf, buflen, flags,
            (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
    }
    while (0);
#else
    do
    {
        socklen_t addrlen = sizeof(pbsd_sockaddr_in);
        retc = recvfrom((int)fd, buf, buflen, flags,
            (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_recvfrom) == PBSD_EINTR);
#endif

    return (retc);
}

int
PRO_CALLTYPE
pbsd_recvmsg(PRO_INT64    fd,
             pbsd_msghdr* msg,
             int          flags)
{
    int retc = -1;

#if !defined(_WIN32) && !defined(_WIN32_WCE)

#if defined(MSG_CMSG_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        do
        {
            retc      = recvmsg((int)fd, msg, flags | MSG_CMSG_CLOEXEC);
            errorcode = pbsd_errno((void*)&pbsd_recvmsg);
        }
        while (retc < 0 && errorcode == PBSD_EINTR);
    }
#endif

    if (retc < 0)
    {
        do
        {
            retc = recvmsg((int)fd, msg, flags);
        }
        while (retc < 0 && pbsd_errno((void*)&pbsd_recvmsg) == PBSD_EINTR);

#if defined(MSG_CMSG_CLOEXEC)
        if (retc >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

#endif /* _WIN32, _WIN32_WCE */

    return (retc);
}

int
PRO_CALLTYPE
pbsd_select(PRO_INT64       nfds,
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
    while (retc < 0 && pbsd_errno((void*)&pbsd_select) == PBSD_EINTR);
#endif

    return (retc);
}

#if defined(PRO_HAS_EPOLL)

int
PRO_CALLTYPE
pbsd_epoll_create()
{
    const int epfd = epoll_create(PBSD_EPOLL_SIZE);
    if (epfd < 0)
    {
        return (-1);
    }

    pbsd_ioctl_closexec(epfd);

    return (epfd);
}

int
PRO_CALLTYPE
pbsd_epoll_ctl(int               epfd,
               int               op,
               PRO_INT64         fd,
               pbsd_epoll_event* event)
{
    int retc = -1;

    do
    {
        retc = epoll_ctl(epfd, op, (int)fd, event);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_epoll_ctl) == PBSD_EINTR);

    return (retc);
}

int
PRO_CALLTYPE
pbsd_epoll_wait(int               epfd,
                pbsd_epoll_event* events,
                int               maxevents,
                int               timeout)
{
    int retc = -1;

    do
    {
        retc = epoll_wait(epfd, events, maxevents, timeout);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_epoll_wait) == PBSD_EINTR);

    return (retc);
}

#endif /* PRO_HAS_EPOLL */

void
PRO_CALLTYPE
pbsd_shutdown_send(PRO_INT64 fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32) || defined(_WIN32_WCE)
    shutdown((SOCKET)fd, SD_SEND);
#else
    shutdown((int)fd, SHUT_WR);
#endif
}

void
PRO_CALLTYPE
pbsd_shutdown_recv(PRO_INT64 fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32) || defined(_WIN32_WCE)
    shutdown((SOCKET)fd, SD_RECEIVE);
#else
    shutdown((int)fd, SHUT_RD);
#endif
}

void
PRO_CALLTYPE
pbsd_closesocket(PRO_INT64 fd,
                 bool      linger) /* = false */
{
    if (fd == -1)
    {
        return;
    }

    /*
     * This will avoid the TIME_WAIT state.
     * WARNING: You will not be able to use WSADuplicateSocket on Windows!!!
     */
    if (!linger)
    {
        struct linger option;
        option.l_onoff  = 1;
        option.l_linger = 0;
        pbsd_setsockopt(
            fd, SOL_SOCKET, SO_LINGER, &option, sizeof(struct linger));
    }

#if defined(_WIN32) || defined(_WIN32_WCE)
    closesocket((SOCKET)fd);
#else
    close((int)fd);
#endif
}

/////////////////////////////////////////////////////////////////////////////
////

const char*
PRO_CALLTYPE
ProGetLocalFirstIp(char        localFirstIp[64],
                   const char* peerIpOrName) /* = NULL */
{
    GetLocalFirstIpWithGW_i(localFirstIp, peerIpOrName);
    if (localFirstIp[0] == '\0')
    {
        GetLocalFirstIpWithoutGW_i(localFirstIp);
    }

    return (localFirstIp);
}

unsigned long
PRO_CALLTYPE
ProGetLocalIpList(char localIpList[8][64])
{
    int i = 0;
    int j = 0;

    for (i = 0; i < 8; ++i)
    {
        localIpList[i][0]  = '\0';
        localIpList[i][63] = '\0';
    }

    char name[256] = "";
    name[sizeof(name) - 1] = '\0';

    if (pbsd_gethostname(name, sizeof(name) - 1) != 0)
    {
        return (0);
    }

    PRO_UINT32          ips[1 + 8];
    const unsigned long count = PBSD_GET_IPS(name, ips + 1);

    char ipString[64] = "";
    GetLocalFirstIpWithGW_i(ipString, NULL);

    if (count == 0 && ipString[0] == '\0')
    {
        return (0);
    }

    if (ipString[0] != '\0')
    {
        ips[0] = pbsd_inet_addr_i(ipString);
        strcpy(localIpList[0], ipString);
    }
    else
    {
        ips[0] = ips[1];
        pbsd_inet_ntoa(ips[0], localIpList[0]);
    }

    for (i = 1, j = 1; i < 8 && j <= (int)count; ++j)
    {
        if (ips[j] == ips[0])
        {
            continue;
        }

        pbsd_inet_ntoa(ips[j], localIpList[i]);
        ++i;
    }

    return (i);
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif
