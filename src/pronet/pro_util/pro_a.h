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

#if !defined(____PRO_A_H____)
#define ____PRO_A_H____

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)

#if !defined(PRO_INT8)
#define PRO_INT8   char
#endif
#if !defined(PRO_INT16)
#define PRO_INT16  short
#endif
#if !defined(PRO_INT32)
#define PRO_INT32  int
#endif
#if !defined(PRO_INT64)
#if defined(_MSC_VER)
#define PRO_INT64  __int64
#define PRO_PRT64D "%I64d"
#else
#define PRO_INT64  long long
#define PRO_PRT64D "%lld"
#endif
#endif

#if !defined(PRO_UINT8)
#define PRO_UINT8  unsigned char
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16 unsigned short
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32 unsigned int
#endif
#if !defined(PRO_UINT64)
#if defined(_MSC_VER)
#define PRO_UINT64 unsigned __int64
#define PRO_PRT64U "%I64u"
#else
#define PRO_UINT64 unsigned long long
#define PRO_PRT64U "%llu"
#endif
#endif

#else  /* _MSC_VER, __MINGW32__, __CYGWIN__ */

#if !defined(PRO_INT8)
#define PRO_INT8   char
#endif
#if !defined(PRO_INT16)
#define PRO_INT16  short
#endif
#if !defined(PRO_INT32)
#define PRO_INT32  int
#endif
#if !defined(PRO_INT64)
#define PRO_INT64  long long
#define PRO_PRT64D "%lld"
#endif

#if !defined(PRO_UINT8)
#define PRO_UINT8  unsigned char
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16 unsigned short
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32 unsigned int
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64 unsigned long long
#define PRO_PRT64U "%llu"
#endif

#endif /* _MSC_VER, __MINGW32__, __CYGWIN__ */

/*-------------------------------------------------------------------------*/

/*
 * definitions for Linux, see the "build/DEFINE.txt"
 */
#if defined(__linux__) && !defined(ANDROID)
#if !defined(PRO_HAS_ATOMOP)
#define PRO_HAS_ATOMOP
#endif
#if !defined(PRO_HAS_ACCEPT4)
#define PRO_HAS_ACCEPT4
#endif
#if !defined(PRO_HAS_EPOLL)
#define PRO_HAS_EPOLL
#endif
#if !defined(PRO_HAS_PTHREAD_EXPLICIT_SCHED)
#define PRO_HAS_PTHREAD_EXPLICIT_SCHED
#endif
#endif /* __linux__, ANDROID */

/*
 * definitions for MacOS/iOS, see the "build/DEFINE.txt"
 */
#if defined(__APPLE__)
#if !defined(PRO_LACKS_GETHOSTBYNAME_R)
#define PRO_LACKS_GETHOSTBYNAME_R
#endif
#if !defined(PRO_LACKS_CLOCK_GETTIME)
#define PRO_LACKS_CLOCK_GETTIME
#endif
#if !defined(PRO_MACH_ABSOLUTE_TIME)
#define PRO_MACH_ABSOLUTE_TIME
#endif
#if !defined(PRO_LACKS_UDP_CONNECT)
#define PRO_LACKS_UDP_CONNECT
#endif
#endif /* __APPLE__ */

/*
 * optional definitions, see the "build/DEFINE.txt"
 */
#if !defined(PRO_FD_SETSIZE)
#define PRO_FD_SETSIZE         1024
#endif
#if defined(PRO_HAS_EPOLL) && !defined(PRO_EPOLLFD_GETSIZE)
#define PRO_EPOLLFD_GETSIZE    1024
#endif
#if !defined(PRO_THREAD_STACK_SIZE)
#define PRO_THREAD_STACK_SIZE  (1024 * 1024 - 8192)
#endif
#if !defined(PRO_TIMER_UPCALL_COUNT)
#define PRO_TIMER_UPCALL_COUNT 1000
#endif
#if !defined(PRO_ACCEPTOR_LENGTH)
#define PRO_ACCEPTOR_LENGTH    10000
#endif
#if !defined(PRO_SERVICER_LENGTH)
#define PRO_SERVICER_LENGTH    10000
#endif
#if !defined(PRO_TCP4_PAYLOAD_SIZE)
#define PRO_TCP4_PAYLOAD_SIZE  (1024 * 1024 * 96)
#endif

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_A_H____ */
