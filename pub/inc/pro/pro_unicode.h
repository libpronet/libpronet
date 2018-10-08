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

#if !defined(____PRO_UNICODE_H____)
#define ____PRO_UNICODE_H____

#include "pro_a.h"
#include "pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

#if defined(WIN32) || defined(_WIN32_WCE)

const CProStlWstring
PRO_CALLTYPE
ProAnsiToUnicode(const CProStlString& src);

const CProStlString
PRO_CALLTYPE
ProUnicodeToAnsi(const CProStlWstring& src);

const CProStlWstring
PRO_CALLTYPE
ProUtf8ToUnicode(const CProStlString& src);

const CProStlString
PRO_CALLTYPE
ProUnicodeToUtf8(const CProStlWstring& src);

const CProStlString
PRO_CALLTYPE
ProAnsiToUtf8(const CProStlString& src);

const CProStlString
PRO_CALLTYPE
ProUtf8ToAnsi(const CProStlString& src);

#endif /* WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_UNICODE_H____ */
