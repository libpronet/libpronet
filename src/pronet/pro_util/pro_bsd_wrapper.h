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

#if !defined(____PRO_BSD_WRAPPER_H____)
#define ____PRO_BSD_WRAPPER_H____

#include "pro_a.h"
#include "pro_memory_pool.h"

#if defined(_WIN32) || defined(_WIN32_WCE)

#if !defined(PRO_FD_SETSIZE)
#define PRO_FD_SETSIZE 1024
#endif

#if defined(FD_SETSIZE)
#undef  FD_SETSIZE
#endif
#define FD_SETSIZE PRO_FD_SETSIZE

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(_WINSOCK_DEPRECATED_NO_WARNINGS)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <windows.h> /* for (_WIN32_WINNT>=0x0500) */
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

#else  /* _WIN32, _WIN32_WCE */

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#if defined(PRO_HAS_EPOLL)
#include <sys/epoll.h>
#endif
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#if defined(PRO_FD_SETSIZE)
#undef  PRO_FD_SETSIZE
#endif
#define PRO_FD_SETSIZE FD_SETSIZE

#endif /* _WIN32, _WIN32_WCE */

#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment(lib, "ws2.lib")
#elif defined(_WIN32)
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "ws2_32.lib")
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)

#if !defined(AF_UNIX)
#define AF_UNIX                1
#endif
#if !defined(AF_LOCAL)
#define AF_LOCAL               AF_UNIX
#endif

#if !defined(SO_EXCLUSIVEADDRUSE)
#define SO_EXCLUSIVEADDRUSE    ((int)(~SO_REUSEADDR))
#endif
#if !defined(SIO_UDP_CONNRESET)
#define SIO_UDP_CONNRESET      _WSAIOW(IOC_VENDOR, 12)
#endif

#define PBSD_EBADF             WSAEBADF       /* 10009 */
#define PBSD_EWOULDBLOCK       WSAEWOULDBLOCK /* 10035 */
#define PBSD_EINPROGRESS       WSAEINPROGRESS /* 10036 */
#define PBSD_EMSGSIZE          WSAEMSGSIZE    /* 10040 */
#define PBSD_ECONNRESET        WSAECONNRESET  /* 10054 */
#define PBSD_ETIMEDOUT         WSAETIMEDOUT   /* 10060 */

#define PBSD_FD_ZERO(set)      FD_ZERO(set)
#define PBSD_FD_SET(fd, set)   FD_SET(((SOCKET)(fd)), set)
#define PBSD_FD_CLR(fd, set)   FD_CLR(((SOCKET)(fd)), set)
#define PBSD_FD_ISSET(fd, set) FD_ISSET(((SOCKET)(fd)), set)

#else  /* _WIN32, _WIN32_WCE */

#define PBSD_EBADF             EBADF          /*   9 */
#define PBSD_EWOULDBLOCK       EAGAIN         /*  11 */
#define PBSD_EINPROGRESS       EINPROGRESS    /* 115 */
#define PBSD_EMSGSIZE          EMSGSIZE       /*  90 */
#define PBSD_ECONNRESET        ECONNRESET     /* 104 */
#define PBSD_ETIMEDOUT         ETIMEDOUT      /* 110 */

#define PBSD_FD_ZERO(set)      FD_ZERO(set)
#define PBSD_FD_SET(fd, set)   FD_SET(((int)(fd)), set)
#define PBSD_FD_CLR(fd, set)   FD_CLR(((int)(fd)), set)
#define PBSD_FD_ISSET(fd, set) FD_ISSET(((int)(fd)), set)

#endif /* _WIN32, _WIN32_WCE */

struct pbsd_fd_set : public fd_set
{
    DECLARE_SGI_POOL(0)
};

struct pbsd_sockaddr_in : public sockaddr_in
{
    DECLARE_SGI_POOL(0)
};

#if defined(_WIN32) || defined(_WIN32_WCE)

struct pbsd_sockaddr_un /* a dummy on Windows */
{
    char dummy;

    DECLARE_SGI_POOL(0)
};

struct pbsd_msghdr      /* a dummy on Windows */
{
    char dummy;

    DECLARE_SGI_POOL(0)
};

#else  /* _WIN32, _WIN32_WCE */

struct pbsd_sockaddr_un : public sockaddr_un
{
    DECLARE_SGI_POOL(0)
};

struct pbsd_msghdr : public msghdr
{
    DECLARE_SGI_POOL(0)
};

#endif /* _WIN32, _WIN32_WCE */

#if defined(PRO_HAS_EPOLL)

#if !defined(PRO_EPOLLFD_GETSIZE)
#define PRO_EPOLLFD_GETSIZE 1024
#endif

struct pbsd_epoll_event : public epoll_event
{
    DECLARE_SGI_POOL(0)
};

#endif /* PRO_HAS_EPOLL */

/////////////////////////////////////////////////////////////////////////////
////

void
pbsd_startup();

int
pbsd_errno(void* action); /* = NULL */

int
pbsd_gethostname(char* name,
                 int   namelen);

PRO_UINT32
pbsd_inet_aton(const char* ipornamestring);

const char*
pbsd_inet_ntoa(PRO_UINT32 ip,
               char       ipstring[64]);

PRO_UINT16
pbsd_hton16(PRO_UINT16 host16);

PRO_UINT16
pbsd_ntoh16(PRO_UINT16 net16);

PRO_UINT32
pbsd_hton32(PRO_UINT32 host32);

PRO_UINT32
pbsd_ntoh32(PRO_UINT32 net32);

PRO_UINT64
pbsd_hton64(PRO_UINT64 host64);

PRO_UINT64
pbsd_ntoh64(PRO_UINT64 net64);

PRO_INT64
pbsd_socket(int af,
            int type,
            int protocol);

int
pbsd_socketpair(PRO_INT64 fds[2]);

int
pbsd_ioctl_nonblock(PRO_INT64 fd,
                    long      on = true);

int
pbsd_ioctl_closexec(PRO_INT64 fd,
                    long      on = true);

int
pbsd_setsockopt(PRO_INT64   fd,
                int         level,
                int         optname,
                const void* optval,
                int         optlen);

int
pbsd_getsockopt(PRO_INT64 fd,
                int       level,
                int       optname,
                void*     optval,
                int*      optlen);

int
pbsd_getsockname(PRO_INT64         fd,
                 pbsd_sockaddr_in* addr);

int
pbsd_getsockname_un(PRO_INT64         fd,
                    pbsd_sockaddr_un* addr);

int
pbsd_getpeername(PRO_INT64         fd,
                 pbsd_sockaddr_in* addr);

int
pbsd_getpeername_un(PRO_INT64         fd,
                    pbsd_sockaddr_un* addr);

int
pbsd_bind(PRO_INT64               fd,
          const pbsd_sockaddr_in* addr,
          bool                    reuseaddr);

int
pbsd_bind_un(PRO_INT64               fd,
             const pbsd_sockaddr_un* addr);

int
pbsd_listen(PRO_INT64 fd);

PRO_INT64
pbsd_accept(PRO_INT64         fd,
            pbsd_sockaddr_in* addr);

PRO_INT64
pbsd_accept_un(PRO_INT64         fd,
               pbsd_sockaddr_un* addr);

int
pbsd_connect(PRO_INT64               fd,
             const pbsd_sockaddr_in* addr);

int
pbsd_connect_un(PRO_INT64               fd,
                const pbsd_sockaddr_un* addr);

int
pbsd_send(PRO_INT64   fd,
          const void* buf,
          int         buflen,
          int         flags);

int
pbsd_sendto(PRO_INT64               fd,
            const void*             buf,
            int                     buflen,
            int                     flags,
            const pbsd_sockaddr_in* addr);

int
pbsd_sendmsg(PRO_INT64          fd,
             const pbsd_msghdr* msg,
             int                flags);

int
pbsd_recv(PRO_INT64 fd,
          void*     buf,
          int       buflen,
          int       flags);

int
pbsd_recvfrom(PRO_INT64         fd,
              void*             buf,
              int               buflen,
              int               flags,
              pbsd_sockaddr_in* addr);

int
pbsd_recvmsg(PRO_INT64    fd,
             pbsd_msghdr* msg,
             int          flags);

int
pbsd_select(PRO_INT64       nfds,
            pbsd_fd_set*    readfds,
            pbsd_fd_set*    writefds,
            pbsd_fd_set*    exceptfds,
            struct timeval* timeout);

#if defined(PRO_HAS_EPOLL)

int
pbsd_epoll_create();

int
pbsd_epoll_ctl(int               epfd,
               int               op,
               PRO_INT64         fd,
               pbsd_epoll_event* event);

int
pbsd_epoll_wait(int               epfd,
                pbsd_epoll_event* events,
                int               maxevents,
                int               timeout);

#endif /* PRO_HAS_EPOLL */

void
pbsd_shutdown_send(PRO_INT64 fd);

void
pbsd_shutdown_recv(PRO_INT64 fd);

void
pbsd_closesocket(PRO_INT64 fd,
                 bool      linger = false);

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 功能: 校验给定的ip字符串是否有效
 *
 * 参数:
 * ipString : 待校验的ip字符串
 *
 * 返回值: true有效, false无效
 *
 * 说明: 无
 */
bool
ProCheckIpString(const char* ipString);

/*
 * 功能: 获取本地的首选ip地址
 *
 * 参数:
 * localFirstIp : 输出结果
 * peerIpOrName : 参考的远端地址或域名
 *
 * 返回值: localFirstIp参数本身
 *
 * 说明: 参考地址用于本地多ip的情况下选择最匹配的一个
 */
const char*
ProGetLocalFirstIp(char        localFirstIp[64],
                   const char* peerIpOrName = NULL);

/*
 * 功能: 获取本地的ip地址列表
 *
 * 参数:
 * localIpList : 输出结果
 *
 * 返回值: 列表元素个数
 *
 * 说明: 无
 */
unsigned long
ProGetLocalIpList(char localIpList[8][64]);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_BSD_WRAPPER_H____ */
