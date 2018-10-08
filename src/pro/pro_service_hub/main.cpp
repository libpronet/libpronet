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

#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h" /* for <unistd.h> */
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
        hubs_thread_count      = 20;
        hubs_listen_port       = 3000;
        hubs_handshake_timeout = 10;
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint("hubs_thread_count"     , hubs_thread_count);
        configStream.AddUint("hubs_listen_port"      , hubs_listen_port);
        configStream.AddUint("hubs_handshake_timeout", hubs_handshake_timeout);

        configStream.Get(configs);
    }

    unsigned int   hubs_thread_count; /* 1 ~ 100 */
    unsigned short hubs_listen_port;
    unsigned int   hubs_handshake_timeout;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();

    IProReactor*            reactor        = NULL;
    IProServiceHub*         serviceHub     = NULL;
    CProStlString           configFileName = "";
    SERVICE_HUB_CONFIG_INFO configInfo;

    {
        char exeRoot[1024] = "";
        exeRoot[sizeof(exeRoot) - 1] = '\0';
#if defined(WIN32)
        ::GetModuleFileName(NULL, exeRoot, sizeof(exeRoot) - 1);
        ::GetLongPathName(exeRoot, exeRoot, sizeof(exeRoot) - 1);
        char* const exeSlash = strrchr(exeRoot, '\\');
        if (exeSlash != NULL)
        {
            *exeSlash = '\0';
        }
        else
        {
            strcpy(exeRoot, ".");
        }
        strcat(exeRoot, "\\");
#else
        const int bytes = readlink("/proc/self/exe", exeRoot, sizeof(exeRoot) - 1);
        if (bytes > 0)
        {
            exeRoot[bytes] = '\0';
        }
        else
        {
            strncpy_pro(exeRoot, sizeof(exeRoot), argv[0]);
        }
        char* const exeSlash = strrchr(exeRoot, '/');
        if (exeSlash != NULL)
        {
            *exeSlash = '\0';
        }
        else
        {
            strcpy(exeRoot, "/usr/local/bin");
        }
        strcat(exeRoot, "/");
#endif

        configFileName =  exeRoot;
        configFileName += CONFIG_FILE_NAME;
    }

    {
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
            else if (stricmp(configName.c_str(), "hubs_listen_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.hubs_listen_port = (unsigned short)value;
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
            else
            {
            }
        }
    }

    reactor = ProCreateReactor(configInfo.hubs_thread_count);
    if (reactor == NULL)
    {
        printf(" pro_service_hub --- error! can't create reactor. \n\n");

        goto EXIT;
    }

    serviceHub = ProCreateServiceHub(
        reactor, configInfo.hubs_listen_port, configInfo.hubs_handshake_timeout);
    if (serviceHub == NULL)
    {
        printf(" pro_service_hub --- error! can't create service hub. \n\n");

        goto EXIT;
    }

    printf(
        " pro_service_hub --- [listenPort : %u] --- ok! \n\n"
        ,
        (unsigned int)configInfo.hubs_listen_port
        );
    ProSleep(-1);

EXIT:

    ProDeleteServiceHub(serviceHub);
    ProDeleteReactor(reactor);
    ProSleep(3000);

    return (0);
}
