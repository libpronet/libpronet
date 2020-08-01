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

#if !defined(____PRO_Z_H____)
#define ____PRO_Z_H____

#include "pro_a.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include <cctype>
#if !defined(_WIN32_WCE)
#include <cerrno>
#endif
#include <clocale>
#include <cmath>
#if !defined(_WIN32_WCE)
#include <csignal>
#endif
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>

#if defined(_WIN32) || defined(_WIN32_WCE)

#if !defined(_WIN32_WCE)
#include <conio.h>
#endif
#include <tchar.h>

#else  /* _WIN32, _WIN32_WCE */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#endif /* _WIN32, _WIN32_WCE */

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)

#define strlwr    _strlwr
#define strupr    _strupr
#define stricmp   _stricmp
#define strnicmp  _strnicmp
#define snprintf  _snprintf
#define vsnprintf _vsnprintf
#if defined(_WIN32_WCE)
#define time      _time64
#endif

#else  /* _WIN32, _WIN32_WCE */

#define stricmp   strcasecmp
#define strnicmp  strncasecmp

#endif /* _WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(_WIN32) && !defined(_WIN32_WCE)

char*
strlwr(char* str);

char*
strupr(char* str);

#endif /* _WIN32, _WIN32_WCE */

char*
strncpy_pro(char*       dest,
            size_t      destSize,
            const char* src);

int
snprintf_pro(char*       dest,
             size_t      destSize,
             const char* format,
             ...);

#if !defined(_WIN32_WCE)

void
PRO_CALLTYPE
ProGetExeDir_(char buf[1024]);

void
PRO_CALLTYPE
ProGetExePath(char buf[1024]);

#endif /* _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_Z_H____ */
