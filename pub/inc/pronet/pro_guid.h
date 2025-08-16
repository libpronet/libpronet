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

/*
 * RFC-4122
 */

#ifndef ____PRO_GUID_H____
#define ____PRO_GUID_H____

#include "pro_a.h"
#include "pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

CProStlString
ProMakeGuidVer1();

CProStlString
ProMakeGuidVer4();

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_GUID_H____ */
