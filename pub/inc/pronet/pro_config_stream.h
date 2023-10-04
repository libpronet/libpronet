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

#ifndef ____PRO_CONFIG_STREAM_H____
#define ____PRO_CONFIG_STREAM_H____

#include "pro_a.h"
#include "pro_config_file.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProConfigStream
{
public:

    static bool BufToConfigs(
        const void*                     buf,
        size_t                          size,
        CProStlVector<PRO_CONFIG_ITEM>& configs,
        char                            aroundCharL = '\"',
        char                            aroundCharR = '\"'
        );

    static bool StringToConfigs(
        const CProStlString&            str,
        CProStlVector<PRO_CONFIG_ITEM>& configs,
        char                            aroundCharL = '\"',
        char                            aroundCharR = '\"'
        );

    static void ConfigsToString(
        const CProStlVector<PRO_CONFIG_ITEM>& configs,
        CProStlString&                        str,
        char                                  aroundCharL = '\"',
        char                                  aroundCharR = '\"'
        );

    CProConfigStream();

    void Add(
        const CProStlString& configName,
        const CProStlString& configValue
        );

    void Add(
        const CProStlString&                configName,
        const CProStlVector<CProStlString>& configValues
        );

    void AddInt(
        const CProStlString& configName,
        int                  configValue
        );

    void AddInt(
        const CProStlString&      configName,
        const CProStlVector<int>& configValues
        );

    void AddUint(
        const CProStlString& configName,
        unsigned int         configValue
        );

    void AddUint(
        const CProStlString&               configName,
        const CProStlVector<unsigned int>& configValues
        );

    void AddInt64(
        const CProStlString& configName,
        int64_t              configValue
        );

    void AddInt64(
        const CProStlString&          configName,
        const CProStlVector<int64_t>& configValues
        );

    void AddUint64(
        const CProStlString& configName,
        uint64_t             configValue
        );

    void AddUint64(
        const CProStlString&           configName,
        const CProStlVector<uint64_t>& configValues
        );

    void AddFloat(
        const CProStlString& configName,
        float                configValue
        );

    void AddFloat(
        const CProStlString&        configName,
        const CProStlVector<float>& configValues
        );

    void AddFloat64(
        const CProStlString& configName,
        double               configValue
        );

    void AddFloat64(
        const CProStlString&         configName,
        const CProStlVector<double>& configValues
        );

    void Add(const PRO_CONFIG_ITEM& config);

    void Add(const CProStlVector<PRO_CONFIG_ITEM>& configs);

    void Remove(const CProStlString& configName);

    void Clear();

    void Get(
        const CProStlString& configName,
        CProStlString&       configValue
        ) const;

    void Get(
        const CProStlString&          configName,
        CProStlVector<CProStlString>& configValues
        ) const;

    void GetInt(
        const CProStlString& configName,
        int&                 configValue
        ) const;

    void GetInt(
        const CProStlString& configName,
        CProStlVector<int>&  configValues
        ) const;

    void GetUint(
        const CProStlString& configName,
        unsigned int&        configValue
        ) const;

    void GetUint(
        const CProStlString&         configName,
        CProStlVector<unsigned int>& configValues
        ) const;

    void GetInt64(
        const CProStlString& configName,
        int64_t&             configValue
        ) const;

    void GetInt64(
        const CProStlString&    configName,
        CProStlVector<int64_t>& configValues
        ) const;

    void GetUint64(
        const CProStlString& configName,
        uint64_t&            configValue
        ) const;

    void GetUint64(
        const CProStlString&     configName,
        CProStlVector<uint64_t>& configValues
        ) const;

    void GetFloat(
        const CProStlString& configName,
        float&               configValue
        ) const;

    void GetFloat(
        const CProStlString&  configName,
        CProStlVector<float>& configValues
        ) const;

    void GetFloat64(
        const CProStlString& configName,
        double&              configValue
        ) const;

    void GetFloat64(
        const CProStlString&   configName,
        CProStlVector<double>& configValues
        ) const;

    void Get(CProStlVector<PRO_CONFIG_ITEM>& configs) const;

    void ToString(
        CProStlString& str,
        char           aroundCharL = '\"',
        char           aroundCharR = '\"'
        ) const;

private:

    void Get_i(
        const CProStlString&            configName,
        CProStlVector<PRO_CONFIG_ITEM>& configs
        ) const;

private:

    CProStlMap<CProStlString, CProStlVector<PRO_CONFIG_ITEM> > m_name2Configs;
    CProStlMap<CProStlString, unsigned int>                    m_name2Index;
    unsigned int                                               m_nextIndex;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_CONFIG_STREAM_H____ */
