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
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
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

        hubs_listen_ports.insert(3000);
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint    ("hubs_thread_count"     , hubs_thread_count);
        configStream.AddUint    ("hubs_handshake_timeout", hubs_handshake_timeout);

        CProStlSet<unsigned short>::const_iterator       itr = hubs_listen_ports.begin();
        CProStlSet<unsigned short>::const_iterator const end = hubs_listen_ports.end();

        for (; itr != end; ++itr)
        {
            configStream.AddUint("hubs_listen_port"      , *itr);
        }

        configStream.Get(configs);
    }

    unsigned int               hubs_thread_count; /* 1 ~ 100 */
    unsigned int               hubs_handshake_timeout;

    CProStlSet<unsigned short> hubs_listen_ports;

    DECLARE_SGI_POOL(0)
};

static CProThreadMutexCondition g_s_cond;

/////////////////////////////////////////////////////////////////////////////
////

#if !defined(_WIN32) && !defined(_WIN32_WCE)

static
void
SignalHandler_i(int sig)
{
    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

    switch (sig)
    {
    case SIGHUP:
        {
            printf(
                "\n"
                "%s \n"
                " pro_service_hub --- SIGHUP, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_cond.Signal();
            break;
        }
    case SIGINT:
        {
            printf(
                "\n"
                "%s \n"
                " pro_service_hub --- SIGINT, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_cond.Signal();
            break;
        }
    case SIGQUIT:
        {
            printf(
                "\n"
                "%s \n"
                " pro_service_hub --- SIGQUIT, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_cond.Signal();
            break;
        }
    case SIGTERM:
        {
            printf(
                "\n"
                "%s \n"
                " pro_service_hub --- SIGTERM, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_cond.Signal();
            break;
        }
    } /* end of switch (...) */
}

static
void
PRO_CALLTYPE
SetupSignalHandlers_i()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &SignalHandler_i;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGHUP , &sa, NULL);
    sigaction(SIGINT , &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

#endif /* _WIN32, _WIN32_WCE */

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();

    IProReactor*                   reactor    = NULL;
    CProStlString                  portString = "";
    SERVICE_HUB_CONFIG_INFO        configInfo;
    CProStlVector<IProServiceHub*> hubs;

    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

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
            printf(
                "\n"
                "%s \n"
                " pro_service_hub --- error! can't read the config file. \n"
                " [%s] \n"
                ,
                timeString.c_str(),
                configFileName.c_str()
                );

            goto EXIT;
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
                    configInfo.hubs_listen_ports.insert((unsigned short)value);
                }
            }
            else
            {
            }
        } /* end of for (...) */
    }

    if (configInfo.hubs_listen_ports.size() == 0)
    {
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- error! \"hubs_listen_port\" is not found. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

    reactor = ProCreateReactor(configInfo.hubs_thread_count);
    if (reactor == NULL)
    {
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- error! can't create reactor. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    SetupSignalHandlers_i();
#endif

    {
        CProStlSet<unsigned short>::iterator       itr = configInfo.hubs_listen_ports.begin();
        CProStlSet<unsigned short>::iterator const end = configInfo.hubs_listen_ports.end();

        for (; itr != end; ++itr)
        {
            const unsigned short port = *itr;

            IProServiceHub* const hub = ProCreateServiceHub(
                reactor, port, configInfo.hubs_handshake_timeout);
            if (hub == NULL)
            {
                printf(
                    "\n"
                    "%s \n"
                    " pro_service_hub --- error! can't create service hub on the port %u. \n"
                    " [maybe the port %u or the file \"/tmp/libpronet_127001_%u\" is busy.] \n"
                    ,
                    timeString.c_str(),
                    (unsigned int)port,
                    (unsigned int)port,
                    (unsigned int)port
                    );

                goto EXIT;
            }

            hubs.push_back(hub);

            char info[64] = "";
            sprintf(info, "%u", (unsigned int)port);
            if (portString.empty())
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
        "\n"
        "%s \n"
        " pro_service_hub [ver-%d.%d.%d] --- [ports : %s] --- ok! \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        portString.c_str()
        );
    g_s_cond.Wait(NULL);

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

    return (0);
}
