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

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(WIN32) || defined(_WIN32_WCE)

void
PRO_CALLTYPE
ProAnsiToUnicode(CProStlWstring&      dst,
                 const CProStlString& src)
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
PRO_CALLTYPE
ProUnicodeToAnsi(CProStlString&        dst,
                 const CProStlWstring& src)
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
PRO_CALLTYPE
ProUtf8ToUnicode(CProStlWstring&      dst,
                 const CProStlString& src)
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
PRO_CALLTYPE
ProUnicodeToUtf8(CProStlString&        dst,
                 const CProStlWstring& src)
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
PRO_CALLTYPE
ProAnsiToUtf8(CProStlString&       dst,
              const CProStlString& src)
{
    CProStlWstring uni = L"";
    ProAnsiToUnicode(uni, src);
    ProUnicodeToUtf8(dst, uni);
}

void
PRO_CALLTYPE
ProUtf8ToAnsi(CProStlString&       dst,
              const CProStlString& src)
{
    CProStlWstring uni = L"";
    ProUtf8ToUnicode(uni, src);
    ProUnicodeToAnsi(dst, uni);
}

#endif /* WIN32, _WIN32_WCE */
