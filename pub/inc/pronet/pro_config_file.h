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
 * "name1"    "abcd"
 * "name1"    "1234"
 * "name2"    "00"
 * "name2"    ""
 * "name3"    "*#"
 */

#if !defined(____PRO_CONFIG_FILE_H____)
#define ____PRO_CONFIG_FILE_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

struct PRO_CONFIG_ITEM
{
    PRO_CONFIG_ITEM()
    {
        configName  = "";
        configValue = "";
    }

    CProStlString configName;
    CProStlString configValue;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProConfigFile
{
public:

    CProConfigFile();

    void Init(const char* fileName);

    bool Read(
        CProStlVector<PRO_CONFIG_ITEM>& configs,
        char                            aroundCharL = '\"',
        char                            aroundCharR = '\"'
        ) const;

    bool Write(
        const CProStlVector<PRO_CONFIG_ITEM>& configs,
        char                                  aroundCharL = '\"',
        char                                  aroundCharR = '\"'
        );

private:

    CProStlString m_fileName;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_CONFIG_FILE_H____ */
