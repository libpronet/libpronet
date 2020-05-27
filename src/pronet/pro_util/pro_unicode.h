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

#if !defined(____PRO_UNICODE_H____)
#define ____PRO_UNICODE_H____

#include "pro_a.h"
#include "pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

#if defined(_WIN32) || defined(_WIN32_WCE)

void
PRO_CALLTYPE
ProAnsiToUnicode(CProStlWstring&      dst,
                 const CProStlString& src);

void
PRO_CALLTYPE
ProUnicodeToAnsi(CProStlString&        dst,
                 const CProStlWstring& src);

void
PRO_CALLTYPE
ProUtf8ToUnicode(CProStlWstring&      dst,
                 const CProStlString& src);

void
PRO_CALLTYPE
ProUnicodeToUtf8(CProStlString&        dst,
                 const CProStlWstring& src);

void
PRO_CALLTYPE
ProAnsiToUtf8(CProStlString&       dst,
              const CProStlString& src);

void
PRO_CALLTYPE
ProUtf8ToAnsi(CProStlString&       dst,
              const CProStlString& src);

#endif /* _WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_UNICODE_H____ */
