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
#include "pro_unicode.h"
#include "pro_stl.h"
#include "pro_z.h"

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)

void
ProAnsiToUnicode(const CProStlString& src,
                 CProStlWstring&      dst)
{
    dst = L"";

    if (src.empty())
    {
        return;
    }

    const size_t len = src.length() + 1;
    dst.resize(len);

    wchar_t* const p = &dst[0];
    p[0]       = L'\0';
    p[len - 1] = L'\0';

    ::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, p, (int)len);
}

void
ProUnicodeToAnsi(const CProStlWstring& src,
                 CProStlString&        dst)
{
    dst = "";

    if (src.empty())
    {
        return;
    }

    const size_t len = src.length() * 4 + 1;
    dst.resize(len);

    char* const p = &dst[0];
    p[0]       = '\0';
    p[len - 1] = '\0';

    ::WideCharToMultiByte(
        CP_ACP, 0, src.c_str(), -1, p, (int)len, NULL, NULL);
}

void
ProUtf8ToUnicode(const CProStlString& src,
                 CProStlWstring&      dst)
{
    dst = L"";

    if (src.empty())
    {
        return;
    }

    const size_t len = src.length() + 1;
    dst.resize(len);

    wchar_t* const p = &dst[0];
    p[0]       = L'\0';
    p[len - 1] = L'\0';

    ::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, p, (int)len);
}

void
ProUnicodeToUtf8(const CProStlWstring& src,
                 CProStlString&        dst)
{
    dst = "";

    if (src.empty())
    {
        return;
    }

    const size_t len = src.length() * 4 + 1;
    dst.resize(len);

    char* const p = &dst[0];
    p[0]       = '\0';
    p[len - 1] = '\0';

    ::WideCharToMultiByte(
        CP_UTF8, 0, src.c_str(), -1, p, (int)len, NULL, NULL);
}

void
ProAnsiToUtf8(const CProStlString& src,
              CProStlString&       dst)
{
    CProStlWstring uni = L"";
    ProAnsiToUnicode(src, uni);
    ProUnicodeToUtf8(uni, dst);
}

void
ProUtf8ToAnsi(const CProStlString& src,
              CProStlString&       dst)
{
    CProStlWstring uni = L"";
    ProUtf8ToUnicode(src, uni);
    ProUnicodeToAnsi(uni, dst);
}

#endif /* _WIN32, _WIN32_WCE */
