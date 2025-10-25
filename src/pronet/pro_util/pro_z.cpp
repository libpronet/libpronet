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
#include "pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

static
inline
char
tolower_i(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        c = c - 'A' + 'a';
    }

    return c;
}

static
inline
char
toupper_i(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        c = c - 'a' + 'A';
    }

    return c;
}

/////////////////////////////////////////////////////////////////////////////
////

char*
strlwr_pro(char* str)
{
    if (str != NULL)
    {
        for (int i = 0; str[i] != '\0'; ++i)
        {
            str[i] = tolower_i(str[i]);
        }
    }

    return str;
}

char*
strupr_pro(char* str)
{
    if (str != NULL)
    {
        for (int i = 0; str[i] != '\0'; ++i)
        {
            str[i] = toupper_i(str[i]);
        }
    }

    return str;
}

char*
strncpy_pro(char*       dest,
            size_t      destSize,
            const char* src)
{
    if (dest == NULL || destSize == 0)
    {
        return NULL;
    }

    *dest = '\0';

    if (destSize == 1 || src == NULL)
    {
        return dest;
    }

    return strncat(dest, src, destSize - 1);
}

int
snprintf_pro(char*       dest,
             size_t      destSize,
             const char* format,
             ...)
{
    if (dest == NULL || destSize == 0)
    {
        return 0;
    }

    *dest = '\0';

    if (destSize == 1 || format == NULL)
    {
        return 0;
    }

    va_list ap;
    va_start(ap, format);
#if defined(_MSC_VER)
    int ret = _vsnprintf(dest, destSize, format, ap);
#else
    int ret = vsnprintf(dest, destSize, format, ap);
#endif
    va_end(ap);

    dest[destSize - 1] = '\0';

    if (ret > (int)destSize - 1)
    {
        ret = (int)destSize - 1;
    }

    return ret;
}

void
ProGetExeDir_(char        buf[1024],
              const char* argv0) /* = NULL */
{
    long size = 1024;

    buf[0]        = '\0';
    buf[size - 1] = '\0';

#if defined(_WIN32)
    ::GetModuleFileNameA(NULL, buf, size - 1);
    char* slash = strrchr(buf, '\\');
    if (slash != NULL)
    {
        *slash = '\0';
    }
    else
    {
        strcpy(buf, ".");
    }
    strncat(buf, "\\", size - 1);
#else
    long bytes = readlink("/proc/self/exe", buf, size - 1);
    if (bytes > 0 && bytes < size)
    {
        buf[bytes] = '\0';
    }
    else if (argv0 != NULL && argv0[0] != '\0')
    {
        strncpy_pro(buf, 1024, argv0);
    }
    else
    {
        strcpy(buf, "./a.out");
    }
    char* slash = strrchr(buf, '/');
    if (slash != NULL)
    {
        *slash = '\0';
    }
    else
    {
        strcpy(buf, "/usr/local/bin");
    }
    strncat(buf, "/", size - 1);
#endif
}

void
ProGetExePath(char        buf[1024],
              const char* argv0) /* = NULL */
{
    long size = 1024;

    buf[0]        = '\0';
    buf[size - 1] = '\0';

#if defined(_WIN32)
    ::GetModuleFileNameA(NULL, buf, size - 1);
#else
    long bytes = readlink("/proc/self/exe", buf, size - 1);
    if (bytes > 0 && bytes < size)
    {
        buf[bytes] = '\0';
    }
    else if (argv0 != NULL && argv0[0] != '\0')
    {
        strncpy_pro(buf, 1024, argv0);
    }
    else
    {
        buf[0] = '\0';
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
