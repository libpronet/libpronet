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
#include "pro_shared.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define PBSD_GOOGLE_DNS   "8.8.8.8"
#define PBSD_EPOLL_SIZE   20000

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

/////////////////////////////////////////////////////////////////////////////
////

static
uint32_t
pbsd_inet_addr_i(const char* ipstring)
{
    assert(ipstring != NULL);
    assert(ipstring[0] != '\0');
    if (ipstring == NULL || ipstring[0] == '\0')
    {
        return (uint32_t)-1;
    }

    CProStlString ipstring2 = ipstring;
    if (ipstring2.find_first_not_of("0123456789.") != CProStlString::npos)
    {
        return (uint32_t)-1;
    }

    const char* p[4] = { 0 };
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
    } /* end of for () */

    if (p[0] == NULL || p[0][0] == '\0' || p[1] == NULL || p[1][0] == '\0' ||
        p[2] == NULL || p[2][0] == '\0' || p[3] == NULL || p[3][0] == '\0')
    {
        return (uint32_t)-1;
    }

    uint32_t b[4];
    b[0] = (uint32_t)atoi(p[0]);
    b[1] = (uint32_t)atoi(p[1]);
    b[2] = (uint32_t)atoi(p[2]);
    b[3] = (uint32_t)atoi(p[3]);

    if (b[0] > 255 || b[1] > 255 || b[2] > 255 || b[3] > 255)
    {
        return (uint32_t)-1;
    }

    uint32_t ip = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]; /* host byte order */
    ip = pbsd_hton32(ip);                                           /* network byte order */

    return ip;
}

static
void
pbsd_getaddrinfo_i(const char*              name,
                   CProStlVector<uint32_t>& ips)
{
    ips.clear();

    assert(name != NULL);
    assert(name[0] != '\0');
    if (name == NULL || name[0] == '\0')
    {
        return;
    }

    uint32_t loopmin = pbsd_ntoh32(pbsd_inet_addr_i("127.0.0.0"));
    uint32_t loopmax = pbsd_ntoh32(pbsd_inet_addr_i("127.255.255.255"));

    struct addrinfo hints = { 0 };
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo*        res  = NULL;
    struct addrinfo*        itr  = NULL;
    const pbsd_sockaddr_in* addr = NULL;

    if (getaddrinfo(name, NULL, &hints, &res) != 0)
    {
        return;
    }

    CProStlSet<uint32_t> ips2;

    for (itr = res; itr != NULL; itr = itr->ai_next)
    {
        if (itr->ai_family != AF_INET || itr->ai_socktype != SOCK_DGRAM ||
            itr->ai_addrlen != sizeof(pbsd_sockaddr_in) || itr->ai_addr == NULL)
        {
            continue;
        }

        addr = (pbsd_sockaddr_in*)itr->ai_addr;

        uint32_t iphost = pbsd_ntoh32(addr->sin_addr.s_addr);
        if (
            (iphost < loopmin || iphost > loopmax)
            &&
            ips2.find(iphost) == ips2.end()
           )
        {
            ips.push_back(addr->sin_addr.s_addr);
            ips2.insert(iphost);
        }
    }

    freeaddrinfo(res);
}

static
void
GetLocalFirstIpWithGW_i(char        localFirstIp[64],
                        const char* peerIpOrName)
{
    localFirstIp[0]  = '\0';
    localFirstIp[63] = '\0';

    assert(peerIpOrName != NULL);
    assert(peerIpOrName[0] != '\0');
    if (peerIpOrName == NULL || peerIpOrName[0] == '\0')
    {
        return;
    }

    int64_t sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockId == -1)
    {
        return;
    }

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(80);
    remoteAddr.sin_addr.s_addr = pbsd_inet_aton(peerIpOrName);
    if (remoteAddr.sin_addr.s_addr == (uint32_t)-1 || remoteAddr.sin_addr.s_addr == 0)
    {
        remoteAddr.sin_addr.s_addr = pbsd_inet_addr_i(PBSD_GOOGLE_DNS);
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

    CProStlVector<uint32_t> ips;
    pbsd_getaddrinfo_i(name, ips);

    if (ips.size() > 0)
    {
        pbsd_inet_ntoa(ips[0], localFirstIp);
    }
}

/////////////////////////////////////////////////////////////////////////////
////

void
pbsd_startup()
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
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, NULL); /* SIGPIPE!!! */
#endif

    setlocale(LC_ALL, "");
    ProSrand();
}

int
pbsd_errno(const void* action) /* = NULL */
{
    int errcode = 0;

#if defined(_WIN32)

    errcode = ::WSAGetLastError();

    if (errcode == PBSD_EWOULDBLOCK && action == &pbsd_connect)
    {
        errcode = PBSD_EINPROGRESS;
    }

#else  /* _WIN32 */

    errcode = errno;

#endif /* _WIN32 */

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

int
pbsd_gethostname(char*  name,
                 size_t len)
{
    return gethostname(name, (int)len);
}

uint32_t
pbsd_inet_aton(const char* ipornamestring) /* = NULL */
{
    if (ipornamestring == NULL || ipornamestring[0] == '\0')
    {
        ipornamestring = "0.0.0.0";
    }
    if (stricmp(ipornamestring, "localhost") == 0)
    {
        ipornamestring = "127.0.0.1";
    }

    uint32_t ip = pbsd_inet_addr_i(ipornamestring);

    if (ip == (uint32_t)-1)
    {
        CProStlVector<uint32_t> ips;
        pbsd_getaddrinfo_i(ipornamestring, ips);

        if (ips.size() > 0)
        {
            ip = ips[0];
        }
    }

    return ip;
}

const char*
pbsd_inet_ntoa(uint32_t ip,
               char     ipstring[64])
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

    return ipstring;
}

uint16_t
pbsd_hton16(uint16_t host16)
{
    uint16_t net16 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net16 = host16;
#else
    net16 |= (host16 >> 8) & 0x00FF;
    net16 |= (host16 << 8) & 0xFF00;
#endif

    return net16;
}

uint16_t
pbsd_ntoh16(uint16_t net16)
{
    return pbsd_hton16(net16);
}

uint32_t
pbsd_hton32(uint32_t host32)
{
    uint32_t net32 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net32 = host32;
#else
    net32 |= (host32 >> 24) & 0x000000FF;
    net32 |= (host32 >>  8) & 0x0000FF00;
    net32 |= (host32 <<  8) & 0x00FF0000;
    net32 |= (host32 << 24) & 0xFF000000;
#endif

    return net32;
}

uint32_t
pbsd_ntoh32(uint32_t net32)
{
    return pbsd_hton32(net32);
}

uint64_t
pbsd_hton64(uint64_t host64)
{
    uint64_t net64 = 0;

#if defined(PRO_WORDS_BIGENDIAN)
    net64 = host64;
#else
    uint32_t high = pbsd_hton32((uint32_t)(host64 >> 32));
    uint32_t low  = pbsd_hton32((uint32_t)host64);
    net64 =   low;
    net64 <<= 32;
    net64 |=  high;
#endif

    return net64;
}

uint64_t
pbsd_ntoh64(uint64_t net64)
{
    return pbsd_hton64(net64);
}

int64_t
pbsd_socket(int af,
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
        errorcode = pbsd_errno((void*)&pbsd_socket);
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
        pbsd_ioctl_nonblock(fd);
        pbsd_ioctl_closexec(fd);

        if (af == AF_INET && type == SOCK_DGRAM)
        {
            int option = 1;
            pbsd_setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &option, sizeof(int));

#if defined(_WIN32)
            long          value         = 0;
            unsigned long bytesReturned = 0;
            ::WSAIoctl((SOCKET)fd, (unsigned long)SIO_UDP_CONNRESET, &value, sizeof(long),
                NULL, 0, &bytesReturned, NULL, NULL);
#endif
        }
    }
    else
    {
        fd = -1;
    }

    return fd;
}

int
pbsd_socketpair(int64_t fds[2])
{
    fds[0] = -1;
    fds[1] = -1;

    int retc = -1;

#if !defined(_WIN32)

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

#endif /* _WIN32 */

    return retc;
}

int
pbsd_ioctl_nonblock(int64_t fd,
                    bool    on) /* = true */
{
    long value = 0;
    if (on)
    {
        value = (long)((unsigned long)-1 >> 1);
    }

    int retc1 = -1;
    int retc2 = -1;

#if defined(_WIN32)

    do
    {
        unsigned long bytesReturned = 0;
        retc1 = ::WSAIoctl((SOCKET)fd, (unsigned long)FIONBIO, &value, sizeof(long),
            NULL, 0, &bytesReturned, NULL, NULL);
    }
    while (0);

#else  /* _WIN32 */

    do
    {
        retc1 = ioctl((int)fd, (unsigned long)FIONBIO, &value);
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
        if (on)
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
        while (retc2 < 0 && pbsd_errno((void*)&pbsd_ioctl_nonblock) == PBSD_EINTR);
    }

#endif /* _WIN32 */

    if (retc1 < 0 && retc2 < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int
pbsd_ioctl_closexec(int64_t fd,
                    bool    on) /* = true */
{
    int retc = -1;

#if !defined(_WIN32)

    do
    {
        retc = fcntl((int)fd, F_GETFD);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_ioctl_closexec) == PBSD_EINTR);

    if (retc >= 0)
    {
        int flags = retc;
        if (on)
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
        while (retc < 0 && pbsd_errno((void*)&pbsd_ioctl_closexec) == PBSD_EINTR);
    }

#endif /* _WIN32 */

    return retc;
}

int
pbsd_setsockopt(int64_t     fd,
                int         level,
                int         optname,
                const void* optval,
                int         optlen)
{
    while (
        level == SOL_SOCKET
        &&
        (optname == SO_RCVBUF || optname == SO_SNDBUF)
       )
    {
        if (optval == NULL || optlen <= 0)
        {
            break;
        }

        unsigned char* p   = (unsigned char*)optval;
        int            sum = 0;

        for (int i = 0; i < optlen; ++i)
        {
            sum += p[i];
        }

        if (sum == 0)
        {
            return 0; /* do nothing */
        }
        break;
    }

    int retc = -1;

#if defined(_WIN32)
    retc = setsockopt((SOCKET)fd, level, optname, (char*)optval, optlen);
#else
    retc = setsockopt((int)fd, level, optname, optval, optlen);
#endif

    return retc;
}

int
pbsd_getsockopt(int64_t fd,
                int     level,
                int     optname,
                void*   optval,
                int*    optlen)
{
    int retc = -1;

#if defined(_WIN32)
    retc = getsockopt((SOCKET)fd, level, optname, (char*)optval, optlen);
#else
    socklen_t optlen2 = 0;
    if (optlen != NULL)
    {
        optlen2 = *optlen;
    }
    retc = getsockopt((int)fd, level, optname, optval, optlen != NULL ? &optlen2 : NULL);
    if (optlen != NULL)
    {
        *optlen = optlen2;
    }
#endif

    return retc;
}

int
pbsd_getsockname(int64_t           fd,
                 pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32)
    int addrlen = sizeof(pbsd_sockaddr_in);
    retc = getsockname((SOCKET)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#else
    socklen_t addrlen = sizeof(pbsd_sockaddr_in);
    retc = getsockname((int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return retc;
}

int
pbsd_getsockname_un(int64_t           fd,
                    pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32)
    socklen_t addrlen = sizeof(pbsd_sockaddr_un);
    retc = getsockname((int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return retc;
}

int
pbsd_getpeername(int64_t           fd,
                 pbsd_sockaddr_in* addr)
{
    int retc = -1;

#if defined(_WIN32)
    int addrlen = sizeof(pbsd_sockaddr_in);
    retc = getpeername((SOCKET)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#else
    socklen_t addrlen = sizeof(pbsd_sockaddr_in);
    retc = getpeername((int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return retc;
}

int
pbsd_getpeername_un(int64_t           fd,
                    pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32)
    socklen_t addrlen = sizeof(pbsd_sockaddr_un);
    retc = getpeername((int)fd, (struct sockaddr*)addr, addr != NULL ? &addrlen : NULL);
#endif

    return retc;
}

int
pbsd_bind(int64_t                 fd,
          const pbsd_sockaddr_in* addr,
          bool                    reuseaddr)
{
    if (reuseaddr)
    {
        int option = 1;
        pbsd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
    }
    else
    {
#if defined(_WIN32)
        int option = 1;
        pbsd_setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &option, sizeof(int));
#endif
    }

    int retc = -1;

#if defined(_WIN32)
    retc = bind((SOCKET)fd, (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
#else
    retc = bind((int)fd, (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
#endif

    return retc;
}

int
pbsd_bind_un(int64_t                 fd,
             const pbsd_sockaddr_un* addr)
{
    int retc = -1;

#if !defined(_WIN32)
    retc = bind((int)fd, (struct sockaddr*)addr, addr != NULL ? sizeof(pbsd_sockaddr_un) : 0);
#endif

    return retc;
}

int
pbsd_listen(int64_t fd)
{
    int retc = -1;

#if defined(_WIN32)
    retc = listen((SOCKET)fd, SOMAXCONN);
#else
    retc = listen((int)fd, SOMAXCONN);
#endif

    return retc;
}

int64_t
pbsd_accept(int64_t           fd,
            pbsd_sockaddr_in* srcaddr)
{
    int64_t newfd = -1;

#if defined(_WIN32)

    do
    {
        int addrlen = sizeof(pbsd_sockaddr_in);
        if (sizeof(SOCKET) == 8)
        {
            newfd = (int64_t)accept((SOCKET)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
        }
        else
        {
            newfd = (int32_t)accept((SOCKET)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
        }
    }
    while (0);

#else  /* _WIN32 */

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_in);
            newfd     = (int32_t)accept4((int)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL, SOCK_CLOEXEC);
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
            newfd = (int32_t)accept((int)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
        }
        while (newfd < 0 && pbsd_errno((void*)&pbsd_accept) == PBSD_EINTR);

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
        if (newfd >= 0 && errorcode == PBSD_EINVAL)
        {
            s_hasclose = false;
        }
#endif
    }

#endif /* _WIN32 */

    if (newfd >= 0)
    {
        pbsd_ioctl_nonblock(newfd);
        pbsd_ioctl_closexec(newfd);
    }
    else
    {
        newfd = -1;
    }

    return newfd;
}

int64_t
pbsd_accept_un(int64_t           fd,
               pbsd_sockaddr_un* srcaddr)
{
    int64_t newfd = -1;

#if !defined(_WIN32)

#if defined(PRO_HAS_ACCEPT4) && defined(SOCK_CLOEXEC)
    static bool s_hasclose = true;
    int         errorcode  = 0;
    if (s_hasclose)
    {
        do
        {
            socklen_t addrlen = sizeof(pbsd_sockaddr_un);
            newfd     = (int32_t)accept4((int)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL, SOCK_CLOEXEC);
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
            newfd = (int32_t)accept((int)fd,
                (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
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

#endif /* _WIN32 */

    return newfd;
}

int
pbsd_connect(int64_t                 fd,
             const pbsd_sockaddr_in* dstaddr)
{
    int retc = -1;

#if defined(_WIN32)
    do
    {
        retc = connect((SOCKET)fd,
            (struct sockaddr*)dstaddr, dstaddr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (0);
#else
    do
    {
        retc = connect((int)fd,
            (struct sockaddr*)dstaddr, dstaddr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (retc != 0 && pbsd_errno((void*)&pbsd_connect) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_connect_un(int64_t                 fd,
                const pbsd_sockaddr_un* dstaddr)
{
    int retc = -1;

#if !defined(_WIN32)
    do
    {
        retc = connect((int)fd,
            (struct sockaddr*)dstaddr, dstaddr != NULL ? sizeof(pbsd_sockaddr_un) : 0);
    }
    while (retc != 0 && pbsd_errno((void*)&pbsd_connect_un) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_send(int64_t     fd,
          const void* buf,
          size_t      len,
          int         flags)
{
    int retc = -1;

#if defined(_WIN32)
    do
    {
        retc = send((SOCKET)fd, (char*)buf, (int)len, flags);
    }
    while (0);
#else
    do
    {
        retc = send((int)fd, buf, len, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_send) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_sendto(int64_t                 fd,
            const void*             buf,
            size_t                  len,
            int                     flags,
            const pbsd_sockaddr_in* dstaddr)
{
    int retc = -1;

#if defined(_WIN32)
    do
    {
        retc = sendto((SOCKET)fd, (char*)buf, (int)len, flags,
            (struct sockaddr*)dstaddr, dstaddr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (0);
#else
    do
    {
        retc = sendto((int)fd, buf, len, flags,
            (struct sockaddr*)dstaddr, dstaddr != NULL ? sizeof(pbsd_sockaddr_in) : 0);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_sendto) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_sendmsg(int64_t            fd,
             const pbsd_msghdr* msg,
             int                flags)
{
    int retc = -1;

#if !defined(_WIN32)
    do
    {
        retc = sendmsg((int)fd, msg, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_sendmsg) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_recv(int64_t fd,
          void*   buf,
          size_t  len,
          int     flags)
{
    int retc = -1;

#if defined(_WIN32)
    do
    {
        retc = recv((SOCKET)fd, (char*)buf, (int)len, flags);
    }
    while (0);
#else
    do
    {
        retc = recv((int)fd, buf, len, flags);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_recv) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_recvfrom(int64_t           fd,
              void*             buf,
              size_t            len,
              int               flags,
              pbsd_sockaddr_in* srcaddr)
{
    int retc = -1;

#if defined(_WIN32)
    do
    {
        int addrlen = sizeof(pbsd_sockaddr_in);
        retc = recvfrom((SOCKET)fd, (char*)buf, (int)len, flags,
            (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
    }
    while (0);
#else
    do
    {
        socklen_t addrlen = sizeof(pbsd_sockaddr_in);
        retc = recvfrom((int)fd, buf, len, flags,
            (struct sockaddr*)srcaddr, srcaddr != NULL ? &addrlen : NULL);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_recvfrom) == PBSD_EINTR);
#endif

    return retc;
}

int
pbsd_recvmsg(int64_t      fd,
             pbsd_msghdr* msg,
             int          flags)
{
    int retc = -1;

#if !defined(_WIN32)

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

#endif /* _WIN32 */

    return retc;
}

int
pbsd_select(int64_t         nfds,
            pbsd_fd_set*    readfds,
            pbsd_fd_set*    writefds,
            pbsd_fd_set*    exceptfds,
            struct timeval* timeout)
{
    int retc = -1;

#if defined(_WIN32)
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

    return retc;
}

int
pbsd_poll(pbsd_pollfd* fds,
          size_t       nfds,
          int          timeout)
{
    int retc = -1;

#if !defined(_WIN32)
    do
    {
        retc = poll(fds, nfds, timeout);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_poll) == PBSD_EINTR);
#endif

    return retc;
}

#if defined(PRO_HAS_EPOLL)

int
pbsd_epoll_create()
{
    int epfd = epoll_create(PBSD_EPOLL_SIZE);
    if (epfd < 0)
    {
        return -1;
    }

    pbsd_ioctl_closexec(epfd);

    return epfd;
}

int
pbsd_epoll_ctl(int               epfd,
               int               op,
               int64_t           fd,
               pbsd_epoll_event* event)
{
    int retc = -1;

    do
    {
        retc = epoll_ctl(epfd, op, (int)fd, event);
    }
    while (retc < 0 && pbsd_errno((void*)&pbsd_epoll_ctl) == PBSD_EINTR);

    return retc;
}

int
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

    return retc;
}

#endif /* PRO_HAS_EPOLL */

void
pbsd_shutdown_send(int64_t fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32)
    shutdown((SOCKET)fd, SD_SEND);
#else
    shutdown((int)fd, SHUT_WR);
#endif
}

void
pbsd_shutdown_recv(int64_t fd)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32)
    shutdown((SOCKET)fd, SD_RECEIVE);
#else
    shutdown((int)fd, SHUT_RD);
#endif
}

void
pbsd_closesocket(int64_t fd,
                 bool    linger) /* = false */
{
    if (fd == -1)
    {
        return;
    }

    /*
     * The state 'TIME_WAIT' will be skipped.
     * WARNING: You will not be able to use WSADuplicateSocket on Windows!!!
     */
    if (!linger)
    {
        struct linger option = { 0 };
        option.l_onoff  = 1;
        option.l_linger = 0;
        pbsd_setsockopt(fd, SOL_SOCKET, SO_LINGER, &option, sizeof(struct linger));
    }

#if defined(_WIN32)
    closesocket((SOCKET)fd);
#else
    close((int)fd);
#endif
}

/////////////////////////////////////////////////////////////////////////////
////

bool
ProCheckIpString(const char* ipString)
{
    if (ipString == NULL || ipString[0] == '\0')
    {
        return false;
    }

    if (strcmp(ipString, "0.0.0.0") == 0)
    {
        return true;
    }

    return pbsd_inet_addr_i(ipString) != (uint32_t)-1;
}

const char*
ProGetLocalFirstIp(char        localFirstIp[64],
                   const char* peerIpOrName) /* = NULL */
{
    if (peerIpOrName == NULL || peerIpOrName[0] == '\0')
    {
        peerIpOrName = PBSD_GOOGLE_DNS;
    }

    GetLocalFirstIpWithGW_i(localFirstIp, peerIpOrName);
    if (localFirstIp[0] == '\0')
    {
        GetLocalFirstIpWithoutGW_i(localFirstIp);
    }

    return localFirstIp;
}

void
ProGetLocalIpList(CProStlVector<CProStlString>& localIpList)
{
    localIpList.clear();

    char name[256] = "";
    name[sizeof(name) - 1] = '\0';

    if (pbsd_gethostname(name, sizeof(name) - 1) != 0)
    {
        return;
    }

    CProStlVector<uint32_t> ips;
    pbsd_getaddrinfo_i(name, ips);

    int c = (int)ips.size();
    if (c == 0)
    {
        return;
    }

    uint32_t ip           = 0;
    char     ipString[64] = "";

    GetLocalFirstIpWithGW_i(ipString, PBSD_GOOGLE_DNS);
    if (ipString[0] != '\0')
    {
        ip = pbsd_inet_addr_i(ipString);
    }
    else
    {
        ip = ips[0];
        pbsd_inet_ntoa(ip, ipString);
    }

    localIpList.push_back(ipString);

    for (int i = 0; i < c; ++i)
    {
        if (ips[i] != ip)
        {
            pbsd_inet_ntoa(ips[i], ipString);
            localIpList.push_back(ipString);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
////

class CProBsdWrapperDotCpp
{
public:

    CProBsdWrapperDotCpp()
    {
        pbsd_startup();
    }
};

static volatile CProBsdWrapperDotCpp g_s_initiator;
