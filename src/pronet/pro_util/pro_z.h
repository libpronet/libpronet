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

#ifndef ____PRO_Z_H____
#define ____PRO_Z_H____

#include "pro_a.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include <atomic>
#include <cassert>
#include <cctype>
#include <cerrno>
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#include <cinttypes>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>

#if defined(_WIN32)
#include <conio.h>
#include <tchar.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_MSC_VER)

#define stricmp   _stricmp
#define strnicmp  _strnicmp
#define snprintf  _snprintf
#define vsnprintf _vsnprintf

#else  /* _MSC_VER */

#define stricmp   strcasecmp
#define strnicmp  strncasecmp

#endif /* _MSC_VER */

/////////////////////////////////////////////////////////////////////////////
////

char*
strlwr_pro(char* str);

char*
strupr_pro(char* str);

char*
strncpy_pro(char*       dest,
            size_t      destSize,
            const char* src);

int
snprintf_pro(char*       dest,
             size_t      destSize,
             const char* format,
             ...);

void
ProGetExeDir_(char buf[1024]);

void
ProGetExePath(char buf[1024]);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_Z_H____ */
