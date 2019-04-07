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

#include "test.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define CONFIG_FILE_NAME "test_tcp_client.cfg"

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();

    const char* server_ip   = NULL;
    int         server_port = 0;
    const char* local_ip    = NULL;

    if (argc >= 3)
    {
        server_ip   = argv[1];
        server_port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        local_ip    = argv[3];
    }
    if (server_port < 0 || server_port > 65535)
    {
        server_port = 0;
    }

    IProReactor*           reactor = NULL;
    CTest*                 tester  = NULL;
    TCP_CLIENT_CONFIG_INFO configInfo;

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
                " test_tcp_client --- warning! can't read the config file. \n"
                " [ %s ] \n"
                ,
                configFileName.c_str()
                );
        }

        configInfo.tcpc_ssl_cafile.clear();
        configInfo.tcpc_ssl_crlfile.clear();

        int       i = 0;
        const int c = (int)configs.size();

        for (; i < c; ++i)
        {
            CProStlString& configName  = configs[i].configName;
            CProStlString& configValue = configs[i].configValue;

            if (stricmp(configName.c_str(), "tcpc_thread_count") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 100)
                {
                    configInfo.tcpc_thread_count = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_server_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.tcpc_server_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_server_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.tcpc_server_port = (unsigned short)value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_local_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.tcpc_local_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_connection_count") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 60000)
                {
                    configInfo.tcpc_connection_count = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_max_pending_count") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 1000)
                {
                    configInfo.tcpc_max_pending_count = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_handshake_timeout") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.tcpc_handshake_timeout = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_heartbeat_interval") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.tcpc_heartbeat_interval = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_heartbeat_bytes") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value >= 0 && value <= 1024)
                {
                    configInfo.tcpc_heartbeat_bytes = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_sockbuf_size_recv") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value >= 1024)
                {
                    configInfo.tcpc_sockbuf_size_recv = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_sockbuf_size_send") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value >= 1024)
                {
                    configInfo.tcpc_sockbuf_size_send = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_recvpool_size") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value >= 1024)
                {
                    configInfo.tcpc_recvpool_size = value;
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_enable_ssl") == 0)
            {
                configInfo.tcpc_enable_ssl = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "tcpc_ssl_enable_sha1cert") == 0)
            {
                configInfo.tcpc_ssl_enable_sha1cert = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "tcpc_ssl_cafile") == 0)
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
                    configInfo.tcpc_ssl_cafile.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_ssl_crlfile") == 0)
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
                    configInfo.tcpc_ssl_crlfile.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "tcpc_ssl_sni") == 0)
            {
                configInfo.tcpc_ssl_sni = configValue;
            }
            else if (stricmp(configName.c_str(), "tcpc_ssl_aes256") == 0)
            {
                configInfo.tcpc_ssl_aes256 = atoi(configValue.c_str()) != 0;
            }
            else
            {
            }
        } /* end of for (...) */

        if (server_ip != NULL && server_port > 0)
        {
            configInfo.tcpc_server_ip   = server_ip;
            configInfo.tcpc_server_port = server_port;
        }
        if (local_ip != NULL)
        {
            configInfo.tcpc_local_ip    = local_ip;
        }
    }

    static char s_traceInfo[2048] = "";
    s_traceInfo[sizeof(s_traceInfo) - 1] = '\0';

    reactor = ProCreateReactor(configInfo.tcpc_thread_count);
    if (reactor == NULL)
    {
        printf("\n test_tcp_client --- error! can't create reactor. \n");

        goto EXIT;
    }
    else
    {
        reactor->UpdateHeartbeatTimers(configInfo.tcpc_heartbeat_interval);
    }

    tester = CTest::CreateInstance();
    if (tester == NULL || !tester->Init(reactor, configInfo))
    {
        printf("\n test_tcp_client --- error! can't create tester. \n");

        goto EXIT;
    }

    printf("\n test_tcp_client --- connecting... \n\n");

    printf(
        " help               : show this message \n"
        " htbttime <seconds> : set new heartbeat interval in seconds. \n"
        "                      for example, \"htbttime 200\" \n"
        " htbtsize <bytes>   : set new heartbeat data size in bytes. [0 ~ 1024] \n"
        "                      for example, \"htbtsize 0\" \n"
        "\n"
        );

    reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
    printf("%s", s_traceInfo);
    printf(" [ HTBT Size ] : %u \n", (unsigned int)tester->GetHeartbeatDataSize());

    while (1)
    {
        ProSleep(1);
#if defined(WIN32)
        printf("\nTCP-Cli:\\>");
#else
        printf("\nTCP-Cli:~$ ");
#endif
        fflush(stdout);

        static char s_msgText[1024]      = "";
        s_msgText[0]                     = '\0';
        s_msgText[sizeof(s_msgText) - 1] = '\0';

        /*
         * Bug!!! Don't include <iostream> for Android!!!
         */
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

        if (p[0] == '\0')
        {
            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf("%s", s_traceInfo);
            printf(" [ HTBT Size ] : %u \n", (unsigned int)tester->GetHeartbeatDataSize());
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
                "\n"
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

            reactor->UpdateHeartbeatTimers(seconds);
            printf("\n htbttime... \n\n");

            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf("%s", s_traceInfo);
            printf(" [ HTBT Size ] : %u \n", (unsigned int)tester->GetHeartbeatDataSize());
        }
        else if (strnicmp(p, "htbtsize ", 9) == 0)
        {
            p += 9;

            const int bytes = atoi(p);
            if (bytes < 0 || bytes > 1024)
            {
                continue;
            }

            tester->SetHeartbeatDataSize(bytes);
            printf("\n htbtsize... \n\n");

            reactor->GetTraceInfo(s_traceInfo, sizeof(s_traceInfo));
            printf("%s", s_traceInfo);
            printf(" [ HTBT Size ] : %u \n", (unsigned int)tester->GetHeartbeatDataSize());
        }
        else
        {
            tester->SendMsg(p);
        }
    } /* end of while (...) */

EXIT:

    if (tester != NULL)
    {
        tester->Fini();
        tester->Release();
    }

    ProDeleteReactor(reactor);
    ProSleep(3000);

    return (0);
}
