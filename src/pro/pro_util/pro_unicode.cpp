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

const CProStlWstring
PRO_CALLTYPE
ProAnsiToUnicode(const CProStlString& src)
{
    if (src.empty())
    {
        return (L"");
    }

    CProStlWstring dst(src.length() + 1, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, &dst[0], (int)dst.length());

    return (dst.c_str());
}

const CProStlString
PRO_CALLTYPE
ProUnicodeToAnsi(const CProStlWstring& src)
{
    if (src.empty())
    {
        return ("");
    }

    CProStlString dst(src.length() * 2 + 1, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, &dst[0], (int)dst.length(),
        NULL, NULL);

    return (dst.c_str());
}

const CProStlWstring
PRO_CALLTYPE
ProUtf8ToUnicode(const CProStlString& src)
{
    if (src.empty())
    {
        return (L"");
    }

    CProStlWstring dst(src.length() + 1, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &dst[0], (int)dst.length());

    return (dst.c_str());
}

const CProStlString
PRO_CALLTYPE
ProUnicodeToUtf8(const CProStlWstring& src)
{
    if (src.empty())
    {
        return ("");
    }

    CProStlString dst(src.length() * 4 + 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, &dst[0], (int)dst.length(),
        NULL, NULL);

    return (dst.c_str());
}

const CProStlString
PRO_CALLTYPE
ProAnsiToUtf8(const CProStlString& src)
{
    const CProStlWstring tmp = ProAnsiToUnicode(src);
    const CProStlString  dst = ProUnicodeToUtf8(tmp);

    return (dst.c_str());
}

const CProStlString
PRO_CALLTYPE
ProUtf8ToAnsi(const CProStlString& src)
{
    const CProStlWstring tmp = ProUtf8ToUnicode(src);
    const CProStlString  dst = ProUnicodeToAnsi(tmp);

    return (dst.c_str());
}

#endif /* WIN32, _WIN32_WCE */
