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

/////////////////////////////////////////////////////////////////////////////
////

#define CONFIG_FILE_NAME "pro_service_hub.cfg"

struct SERVICE_HUB_CONFIG_INFO
{
    SERVICE_HUB_CONFIG_INFO()
    {
        hubs_thread_count      = 10;
        hubs_handshake_timeout = 10;

        hubs_tcpex_port_a.insert(3000);
        hubs_tcpex_port_b.insert(0);
        hubs_tcp_port_a.insert(0);
        hubs_tcp_port_b.insert(0);
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint    ("hubs_thread_count"     , hubs_thread_count);
        configStream.AddUint    ("hubs_handshake_timeout", hubs_handshake_timeout);

        auto itr = hubs_tcpex_port_a.begin();
        auto end = hubs_tcpex_port_a.end();

        for (; itr != end; ++itr)
        {
            configStream.AddUint("hubs_tcpex_port_a"     , *itr);
        }

        itr = hubs_tcpex_port_b.begin();
        end = hubs_tcpex_port_b.end();

        for (; itr != end; ++itr)
        {
            configStream.AddUint("hubs_tcpex_port_b"     , *itr);
        }

        itr = hubs_tcp_port_a.begin();
        end = hubs_tcp_port_a.end();

        for (; itr != end; ++itr)
        {
            configStream.AddUint("hubs_tcp_port_a"       , *itr);
        }

        itr = hubs_tcp_port_b.begin();
        end = hubs_tcp_port_b.end();

        for (; itr != end; ++itr)
        {
            configStream.AddUint("hubs_tcp_port_b"       , *itr);
        }

        configStream.Get(configs);
    }

    unsigned int               hubs_thread_count; /* 1 ~ 100 */
    unsigned int               hubs_handshake_timeout;

    CProStlSet<unsigned short> hubs_tcpex_port_a; /* class-a port with tcpex protocol, active-standby mode */
    CProStlSet<unsigned short> hubs_tcpex_port_b; /* class-b port with tcpex protocol, load-balance mode */
    CProStlSet<unsigned short> hubs_tcp_port_a;   /* class-a port with tcp protocol, active-standby mode */
    CProStlSet<unsigned short> hubs_tcp_port_b;   /* class-b port with tcp protocol, load-balance mode */

    DECLARE_SGI_POOL(0)
};

static CProThreadMutexCondition g_s_cond;
static CProThreadMutex          g_s_lock;

/////////////////////////////////////////////////////////////////////////////
////

static
void
ReadConfig_i(const CProStlVector<PRO_CONFIG_ITEM>& configs,
             SERVICE_HUB_CONFIG_INFO&              configInfo)
{
    configInfo.hubs_tcpex_port_a.clear();
    configInfo.hubs_tcpex_port_b.clear();
    configInfo.hubs_tcp_port_a.clear();
    configInfo.hubs_tcp_port_b.clear();

    int i = 0;
    int c = (int)configs.size();

    for (; i < c; ++i)
    {
        const CProStlString& configName  = configs[i].configName;
        const CProStlString& configValue = configs[i].configValue;

        if (stricmp(configName.c_str(), "hubs_thread_count") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0 && value <= 100)
            {
                configInfo.hubs_thread_count = value;
            }
        }
        else if (stricmp(configName.c_str(), "hubs_handshake_timeout") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.hubs_handshake_timeout = value;
            }
        }
        else if (stricmp(configName.c_str(), "hubs_tcpex_port_a") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.hubs_tcpex_port_a.insert((unsigned short)value);
            }
        }
        else if (stricmp(configName.c_str(), "hubs_tcpex_port_b") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.hubs_tcpex_port_b.insert((unsigned short)value);
            }
        }
        else if (stricmp(configName.c_str(), "hubs_tcp_port_a") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.hubs_tcp_port_a.insert((unsigned short)value);
            }
        }
        else if (stricmp(configName.c_str(), "hubs_tcp_port_b") == 0)
        {
            int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.hubs_tcp_port_b.insert((unsigned short)value);
            }
        }
        else
        {
        }
    } /* end of for () */
}

#if !defined(_WIN32)

static
void
SignalHandler_i(int sig)
{
    CProStlString timeString;
    ProGetLocalTimeString(timeString);

    switch (sig)
    {
    case SIGHUP:
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- SIGHUP, exiting... \n"
            ,
            timeString.c_str()
            );
        fflush(stdout);
        g_s_lock.Lock();
        g_s_cond.Signal();
        g_s_lock.Unlock();
        break;
    case SIGINT:
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- SIGINT, exiting... \n"
            ,
            timeString.c_str()
            );
        fflush(stdout);
        g_s_lock.Lock();
        g_s_cond.Signal();
        g_s_lock.Unlock();
        break;
    case SIGQUIT:
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- SIGQUIT, exiting... \n"
            ,
            timeString.c_str()
            );
        fflush(stdout);
        g_s_lock.Lock();
        g_s_cond.Signal();
        g_s_lock.Unlock();
        break;
    case SIGTERM:
        printf(
            "\n"
            "%s \n"
            " pro_service_hub --- SIGTERM, exiting... \n"
            ,
            timeString.c_str()
            );
        fflush(stdout);
        g_s_lock.Lock();
        g_s_cond.Signal();
        g_s_lock.Unlock();
        break;
    } /* end of switch () */
}

static
void
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

#endif /* _WIN32 */

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();

    IProReactor*                   reactor = NULL;
    CProStlString                  portString;
    SERVICE_HUB_CONFIG_INFO        configInfo;
    CProStlVector<IProServiceHub*> hubs;

    CProStlString timeString;
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
            fflush(stdout);

            goto EXIT;
        }

        ReadConfig_i(configs, configInfo);
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
        fflush(stdout);

        goto EXIT;
    }

#if !defined(_WIN32)
    SetupSignalHandlers_i();
#endif

    for (int i = 1; i <= 4; ++i)
    {
        CProStlSet<unsigned short>::iterator itr;
        CProStlSet<unsigned short>::iterator end;

        if (i == 1)
        {
            itr = configInfo.hubs_tcpex_port_a.begin();
            end = configInfo.hubs_tcpex_port_a.end();
        }
        else if (i == 2)
        {
            itr = configInfo.hubs_tcpex_port_b.begin();
            end = configInfo.hubs_tcpex_port_b.end();
        }
        else if (i== 3)
        {
            itr = configInfo.hubs_tcp_port_a.begin();
            end = configInfo.hubs_tcp_port_a.end();
        }
        else
        {
            itr = configInfo.hubs_tcp_port_b.begin();
            end = configInfo.hubs_tcp_port_b.end();
        }

        for (; itr != end; ++itr)
        {
            unsigned short port = *itr;

            IProServiceHub* hub = NULL;

            if (i == 1 || i == 2)
            {
                hub = ProCreateServiceHubEx(
                    reactor,
                    port,
                    i == 2, /* enableLoadBalance */
                    configInfo.hubs_handshake_timeout
                    );
            }
            else
            {
                hub = ProCreateServiceHub(
                    reactor,
                    port,
                    i == 4  /* enableLoadBalance */
                    );
            }

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
                fflush(stdout);

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
        } /* end of for () */
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
    fflush(stdout);

    g_s_lock.Lock();
    g_s_cond.Wait(&g_s_lock);
    g_s_lock.Unlock();

EXIT:

    {
        int i = 0;
        int c = (int)hubs.size();

        for (; i < c; ++i)
        {
            ProDeleteServiceHub(hubs[i]);
        }
    }

    ProDeleteReactor(reactor);

    return 0;
}
