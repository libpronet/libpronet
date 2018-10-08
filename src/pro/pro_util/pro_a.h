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

#if !defined(____PRO_A_H____)
#define ____PRO_A_H____

/////////////////////////////////////////////////////////////////////////////
////

#define PRO_INT16_VC    short
#define PRO_INT32_VC    int
#if defined(_MSC_VER)
#define PRO_INT64_VC    __int64
#else
#define PRO_INT64_VC    long long
#endif
#define PRO_UINT16_VC   unsigned short
#define PRO_UINT32_VC   unsigned int
#if defined(_MSC_VER)
#define PRO_UINT64_VC   unsigned __int64
#else
#define PRO_UINT64_VC   unsigned long long
#endif
#define PRO_CALLTYPE_VC __stdcall
#define PRO_EXPORT_VC   __declspec(dllexport)
#define PRO_IMPORT_VC

#define PRO_INT16_GCC   short
#define PRO_INT32_GCC   int
#define PRO_INT64_GCC   long long
#define PRO_UINT16_GCC  unsigned short
#define PRO_UINT32_GCC  unsigned int
#define PRO_UINT64_GCC  unsigned long long
#define PRO_CALLTYPE_GCC
#define PRO_EXPORT_GCC  __attribute__((visibility("default")))
#define PRO_IMPORT_GCC

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)

#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_VC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_VC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_VC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_VC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_VC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_VC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_VC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_VC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_VC
#endif

#else  /* _MSC_VER, __MINGW32__, __CYGWIN__ */

#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_GCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_GCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_GCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_GCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_GCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_GCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_GCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_GCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_GCC
#endif

#endif /* _MSC_VER, __MINGW32__, __CYGWIN__ */

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_A_H____ */
