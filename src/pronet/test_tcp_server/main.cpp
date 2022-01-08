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

#include "test.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define CONFIG_FILE_NAME "test_tcp_server.cfg"

static CTest* g_s_tester = NULL;

/////////////////////////////////////////////////////////////////////////////
////

static
void
PRO_CALLTYPE
ReadConfig_i(const CProStlString&            exeRoot,
             CProStlVector<PRO_CONFIG_ITEM>& configs,
             TCP_SERVER_CONFIG_INFO&         configInfo)
{
    configInfo.tcps_ssl_cafiles.clear();
    configInfo.tcps_ssl_crlfiles.clear();
    configInfo.tcps_ssl_certfiles.clear();

    int       i = 0;
    const int c = (int)configs.size();

    for (; i < c; ++i)
    {
        CProStlString& configName  = configs[i].configName;
        CProStlString& configValue = configs[i].configValue;

        if (stricmp(configName.c_str(), "tcps_thread_count") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0 && value <= 100)
            {
                configInfo.tcps_thread_count = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_using_hub") == 0)
        {
            configInfo.tcps_using_hub = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "tcps_port") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.tcps_port = (unsigned short)value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_handshake_timeout") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.tcps_handshake_timeout = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_heartbeat_interval") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.tcps_heartbeat_interval = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_heartbeat_bytes") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value >= 0 && value <= 1024)
            {
                configInfo.tcps_heartbeat_bytes = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_sockbuf_size_recv") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value >= 1024)
            {
                configInfo.tcps_sockbuf_size_recv = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_sockbuf_size_send") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value >= 1024)
            {
                configInfo.tcps_sockbuf_size_send = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_recvpool_size") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value >= 1024)
            {
                configInfo.tcps_recvpool_size = value;
            }
        }
        else if (stricmp(configName.c_str(), "tcps_enable_ssl") == 0)
        {
            configInfo.tcps_enable_ssl = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "tcps_ssl_enable_sha1cert") == 0)
        {
            configInfo.tcps_ssl_enable_sha1cert = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "tcps_ssl_cafile") == 0)
        {
            if (!configValue.empty())
            {
                if (configValue[0] == '.' ||
                    configValue.find_first_of("\\/") == CProStlString::npos)
                {
                    CProStlString fileName = exeRoot;
                    fileName += configValue;
                    configValue = fileName;
                }
            }

            if (!configValue.empty())
            {
                configInfo.tcps_ssl_cafiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "tcps_ssl_crlfile") == 0)
        {
            if (!configValue.empty())
            {
                if (configValue[0] == '.' ||
                    configValue.find_first_of("\\/") == CProStlString::npos)
                {
                    CProStlString fileName = exeRoot;
                    fileName += configValue;
                    configValue = fileName;
                }
            }

            if (!configValue.empty())
            {
                configInfo.tcps_ssl_crlfiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "tcps_ssl_certfile") == 0)
        {
            if (!configValue.empty())
            {
                if (configValue[0] == '.' ||
                    configValue.find_first_of("\\/") == CProStlString::npos)
                {
                    CProStlString fileName = exeRoot;
                    fileName += configValue;
                    configValue = fileName;
                }
            }

            if (!configValue.empty())
            {
                configInfo.tcps_ssl_certfiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "tcps_ssl_keyfile") == 0)
        {
            if (!configValue.empty())
            {
                if (configValue[0] == '.' ||
                    configValue.find_first_of("\\/") == CProStlString::npos)
                {
                    CProStlString fileName = exeRoot;
                    fileName += configValue;
                    configValue = fileName;
                }
            }

            configInfo.tcps_ssl_keyfile = configValue;
        }
        else
        {
        }
    } /* end of for (...) */
}

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
                " test_tcp_server --- SIGHUP, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_tester->Fini();
            _exit(0);
        }
    case SIGINT:
        {
            printf(
                "\n"
                "%s \n"
                " test_tcp_server --- SIGINT, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_tester->Fini();
            _exit(0);
        }
    case SIGQUIT:
        {
            printf(
                "\n"
                "%s \n"
                " test_tcp_server --- SIGQUIT, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_tester->Fini();
            _exit(0);
        }
    case SIGTERM:
        {
            printf(
                "\n"
                "%s \n"
                " test_tcp_server --- SIGTERM, exiting... \n"
                ,
                timeString.c_str()
                );
            g_s_tester->Fini();
            _exit(0);
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

    IProReactor*           reactor = NULL;
    TCP_SERVER_CONFIG_INFO configInfo;

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
            configInfo.ToConfigs(configs);
            configFile.Write(configs);

            printf(
                "\n"
                "%s \n"
                " test_tcp_server --- warning! can't read the config file. \n"
                " [%s] \n"
                ,
                timeString.c_str(),
                configFileName.c_str()
                );
        }

        ReadConfig_i(exeRoot, configs, configInfo);
    }

    static char s_traceInfo[2048] = "";
    s_traceInfo[sizeof(s_traceInfo) - 1] = '\0';

    reactor = ProCreateReactor(configInfo.tcps_thread_count);
    if (reactor == NULL)
    {
        printf(
            "\n"
            "%s \n"
            " test_tcp_server --- error! can't create reactor. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }
    else
    {
        reactor->UpdateHeartbeatTimers(configInfo.tcps_heartbeat_interval);
    }

    g_s_tester = CTest::CreateInstance();
    if (g_s_tester == NULL)
    {
        printf(
            "\n"
            "%s \n"
            " test_tcp_server --- error! can't create tester on port %u. \n"
            ,
            timeString.c_str(),
            (unsigned int)configInfo.tcps_port
            );

        goto EXIT;
    }

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    SetupSignalHandlers_i();
#endif

    if (!g_s_tester->Init(reactor, configInfo))
    {
        printf(
            "\n"
            "%s \n"
            " test_tcp_server --- error! can't init tester on port %u. \n"
            ,
            timeString.c_str(),
            (unsigned int)configInfo.tcps_port
            );

        goto EXIT;
    }

    printf(
        "\n"
        "%s \n"
        " test_tcp_server [ver-%d.%d.%d] --- [usingHub : %s, port : %u] --- ok! \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        configInfo.tcps_using_hub ? "true" : "false",
        (unsigned int)configInfo.tcps_port
        );

    printf(
        "\n"
        " help               : show this message \n"
        " htbttime <seconds> : set new heartbeat interval in seconds. \n"
        "                      for example, \"htbttime 200\" \n"
        " htbtsize <bytes>   : set new heartbeat data size in bytes. [0 ~ 1024] \n"
        "                      for example, \"htbtsize 0\" \n"
        );

    reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
    printf(
        "\n"
        "%s \n"
        "%s \n"
        ,
        timeString.c_str(),
        s_traceInfo
        );
    printf(" [ HTBT Size ] : %u \n",
        (unsigned int)g_s_tester->GetHeartbeatDataSize());

    while (1)
    {
        ProSleep(1);

        printf("\nTCP-Svr>");
        fflush(stdout);

        static char s_msgText[1024]      = "";
        s_msgText[0]                     = '\0';
        s_msgText[sizeof(s_msgText) - 1] = '\0';

        char* p = fgets(s_msgText, sizeof(s_msgText) - 1, stdin);
        if (p == NULL)
        {
            continue;
        }

        for (; *p != '\0'; ++p)
        {
            if (*p != ' ' && *p != '\t')
            {
                break;
            }
        }

        for (char* q = p + strlen(p) - 1; q >= p; --q)
        {
            if (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')
            {
                *q = '\0';
                continue;
            }
            break;
        }

        ProGetLocalTimeString(timeString);

        if (p[0] == '\0')
        {
            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf(
                "\n"
                "%s \n"
                "%s \n"
                ,
                timeString.c_str(),
                s_traceInfo
                );
            printf(" [ HTBT Size ] : %u \n",
                (unsigned int)g_s_tester->GetHeartbeatDataSize());
            continue;
        }

        if (stricmp(p, "help") == 0 || stricmp(p, "--help") == 0 ||
            stricmp(p, "htbttime") == 0 || stricmp(p, "htbtsize") == 0)
        {
            printf(
                "\n"
                " help               : show this message \n"
                " htbttime <seconds> : set new heartbeat interval in seconds. \n"
                "                      for example, \"htbttime 200\" \n"
                " htbtsize <bytes>   : set new heartbeat data size in bytes. [0 ~ 1024] \n"
                "                      for example, \"htbtsize 0\" \n"
                );
        }
        else if (strnicmp(p, "htbttime ", 9) == 0)
        {
            p += 9;

            const int seconds = atoi(p);
            if (seconds <= 0)
            {
                continue;
            }

            printf(
                "\n"
                "%s \n"
                " htbttime : htbttime... \n"
                ,
                timeString.c_str()
                );
            reactor->UpdateHeartbeatTimers(seconds);

            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf(
                "\n"
                "%s \n"
                "%s \n"
                ,
                timeString.c_str(),
                s_traceInfo
                );
            printf(" [ HTBT Size ] : %u \n",
                (unsigned int)g_s_tester->GetHeartbeatDataSize());
        }
        else if (strnicmp(p, "htbtsize ", 9) == 0)
        {
            p += 9;

            const int bytes = atoi(p);
            if (bytes < 0 || bytes > 1024)
            {
                continue;
            }

            printf(
                "\n"
                "%s \n"
                " htbtsize : htbtsize... \n"
                ,
                timeString.c_str()
                );
            g_s_tester->SetHeartbeatDataSize(bytes);

            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf(
                "\n"
                "%s \n"
                "%s \n"
                ,
                timeString.c_str(),
                s_traceInfo
                );
            printf(" [ HTBT Size ] : %u \n",
                (unsigned int)g_s_tester->GetHeartbeatDataSize());
        }
        else
        {
            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf(
                "\n"
                "%s \n"
                "%s \n"
                ,
                timeString.c_str(),
                s_traceInfo
                );
            printf(" [ HTBT Size ] : %u \n",
                (unsigned int)g_s_tester->GetHeartbeatDataSize());
        }
    } /* end of while (...) */

EXIT:

    if (g_s_tester != NULL)
    {
        g_s_tester->Fini();
        g_s_tester->Release();
    }

    ProDeleteReactor(reactor);
    ProSleep(3000);

    return (0);
}
