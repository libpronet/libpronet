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

#include "../pro_net/pro_net.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define CONFIG_FILE_NAME "pro_service_hub.cfg"

struct SERVICE_HUB_CONFIG_INFO
{
    SERVICE_HUB_CONFIG_INFO()
    {
        hubs_thread_count      = 10;
        hubs_handshake_timeout = 10;

        hubs_listen_ports.push_back(3000);
        hubs_listen_ports.push_back(4000);
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint("hubs_thread_count"     , hubs_thread_count);
        configStream.AddUint("hubs_handshake_timeout", hubs_handshake_timeout);

        configStream.AddUint("hubs_listen_port"      , hubs_listen_ports);

        configStream.Get(configs);
    }

    unsigned int                hubs_thread_count; /* 1 ~ 100 */
    unsigned int                hubs_handshake_timeout;

    CProStlVector<unsigned int> hubs_listen_ports;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();

    IProReactor*                   reactor    = NULL;
    CProStlString                  portString = "";
    SERVICE_HUB_CONFIG_INFO        configInfo;
    CProStlVector<IProServiceHub*> hubs;

    char exeRoot[1024] = "";
    ProGetExeDir_(exeRoot);

    {
        CProStlString configFileName = exeRoot;
        configFileName += CONFIG_FILE_NAME;

        CProConfigFile configFile;
        configFile.Init(configFileName.c_str());

        CProStlVector<PRO_CONFIG_ITEM> configs;
        if (!configFile.Read(configs))
        {
            configInfo.ToConfigs(configs);
            configFile.Write(configs);

            printf(
                " pro_service_hub --- warning! can't read the config file. \n"
                " [ %s ] \n\n"
                ,
                configFileName.c_str()
                );
        }

        configInfo.hubs_listen_ports.clear();

        int       i = 0;
        const int c = (int)configs.size();

        for (; i < c; ++i)
        {
            const CProStlString& configName  = configs[i].configName;
            const CProStlString& configValue = configs[i].configValue;

            if (stricmp(configName.c_str(), "hubs_thread_count") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 100)
                {
                    configInfo.hubs_thread_count = value;
                }
            }
            else if (stricmp(configName.c_str(), "hubs_handshake_timeout") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.hubs_handshake_timeout = value;
                }
            }
            else if (stricmp(configName.c_str(), "hubs_listen_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.hubs_listen_ports.push_back(value);
                }
            }
            else
            {
            }
        } /* end of for (...) */
    }

    if (configInfo.hubs_listen_ports.size() == 0)
    {
        printf(" pro_service_hub --- error! \"hubs_listen_port\" is not found. \n\n");

        goto EXIT;
    }

    reactor = ProCreateReactor(configInfo.hubs_thread_count);
    if (reactor == NULL)
    {
        printf(" pro_service_hub --- error! can't create reactor. \n\n");

        goto EXIT;
    }

    {
        int       i = 0;
        const int c = (int)configInfo.hubs_listen_ports.size();

        for (; i < c; ++i)
        {
            IProServiceHub* const hub = ProCreateServiceHub(
                reactor, (unsigned short)configInfo.hubs_listen_ports[i],
                configInfo.hubs_handshake_timeout);
            if (hub == NULL)
            {
                printf(
                    " pro_service_hub --- error! can't create service hub on the port %u. \n"
                    " [ maybe the port %u or the file \"/tmp/libpronet_127001_%u\" is busy. ] \n\n"
                    ,
                    configInfo.hubs_listen_ports[i],
                    configInfo.hubs_listen_ports[i],
                    configInfo.hubs_listen_ports[i]
                    );

                goto EXIT;
            }

            hubs.push_back(hub);

            char info[64] = "";
            sprintf(info, "%u", configInfo.hubs_listen_ports[i]);
            if (i == 0)
            {
                portString =  info;
            }
            else
            {
                portString += ", ";
                portString += info;
            }
        } /* end of for (...) */
    }

    printf(
        " pro_service_hub --- [listenPorts : %s] --- ok! \n\n"
        ,
        portString.c_str()
        );
    ProSleep(-1);

EXIT:

    {
        int       i = 0;
        const int c = (int)hubs.size();

        for (; i < c; ++i)
        {
            ProDeleteServiceHub(hubs[i]);
        }
    }

    ProDeleteReactor(reactor);
    ProSleep(3000);

    return (0);
}
