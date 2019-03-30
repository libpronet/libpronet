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
#include "../pro_rtp/rtp_foundation.h"
#include "../pro_rtp/rtp_framework.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define THREAD_COUNT     2
#define CONFIG_FILE_NAME "test_msg_client.cfg"

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();
    ProRtpInit();

    const char* server_ip   = NULL;
    int         server_port = 0;
    const char* local_ip    = NULL;

    if (argc >= 4)
    {
        server_ip   = argv[1];
        server_port = atoi(argv[2]);
        local_ip    = argv[3];

        if (server_ip[0] == '\0')
        {
            server_ip   = NULL;
        }
        if (server_port < 0 || server_port > 65535)
        {
            server_port = 0;
        }
        if (local_ip[0] == '\0')
        {
            local_ip    = NULL;
        }
    }

    IProReactor*           reactor        = NULL;
    CTest*                 tester         = NULL;
    CProStlString          configFileName = "";
    MSG_CLIENT_CONFIG_INFO configInfo;
    char                   bindString[64] = "";
    RTP_MSG_USER           bindUser;

    {
        char exeRoot[1024] = "";
        ProGetExeDir_(exeRoot);

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
                "\n"
                " test_msg_client --- warning! can't read the config file. \n"
                " [ %s ] \n"
                ,
                configFileName.c_str()
                );
        }

        configInfo.msgc_ssl_cafile.clear();
        configInfo.msgc_ssl_crlfile.clear();

        int       i = 0;
        const int c = (int)configs.size();

        for (; i < c; ++i)
        {
            CProStlString& configName  = configs[i].configName;
            CProStlString& configValue = configs[i].configValue;

            if (stricmp(configName.c_str(), "msgc_server_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.msgc_server_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "msgc_server_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.msgc_server_port = (unsigned short)value;
                }
            }
            else if (stricmp(configName.c_str(), "msgc_id") == 0)
            {
                if (!configValue.empty())
                {
                    RtpMsgString2User(configValue.c_str(), &configInfo.msgc_id);
                }
            }
            else if (stricmp(configName.c_str(), "msgc_password") == 0)
            {
                configInfo.msgc_password = configValue;

                if (!configValue.empty())
                {
                    ProZeroMemory(&configValue[0], configValue.length());
                    configValue = "";
                }
            }
            else if (stricmp(configName.c_str(), "msgc_local_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.msgc_local_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "msgc_handshake_timeout") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.msgc_handshake_timeout = value;
                }
            }
            else if (stricmp(configName.c_str(), "msgc_redline_bytes") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.msgc_redline_bytes = value;
                }
            }
            else if (stricmp(configName.c_str(), "msgc_enable_ssl") == 0)
            {
                configInfo.msgc_enable_ssl = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "msgc_ssl_enable_sha1cert") == 0)
            {
                configInfo.msgc_ssl_enable_sha1cert = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "msgc_ssl_cafile") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.msgc_ssl_cafile.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "msgc_ssl_crlfile") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.msgc_ssl_crlfile.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "msgc_ssl_sni") == 0)
            {
                configInfo.msgc_ssl_sni = configValue;
            }
            else if (stricmp(configName.c_str(), "msgc_ssl_aes256") == 0)
            {
                configInfo.msgc_ssl_aes256 = atoi(configValue.c_str()) != 0;
            }
            else
            {
            }
        } /* end of for (...) */

        if (server_ip != NULL && server_port > 0)
        {
            configInfo.msgc_server_ip   = server_ip;
            configInfo.msgc_server_port = server_port;
        }
        if (local_ip != NULL)
        {
            configInfo.msgc_local_ip    = local_ip;
        }
    }

    reactor = ProCreateReactor(THREAD_COUNT);
    if (reactor == NULL)
    {
        printf("\n test_msg_client --- error! can't create reactor. \n");

        goto EXIT;
    }

    tester = CTest::CreateInstance();
    if (tester == NULL || !tester->Init(reactor, configInfo))
    {
        printf("\n test_msg_client --- error! can't create tester. \n");

        goto EXIT;
    }

    printf("\n test_msg_client --- connecting... \n");

    printf(
        "\n"
        " help      : show this message \n"
        " bind <id> : bind to a destination to send to. \n"
        "             for example, \"bind 2-1-1\" \n"
        " unbind    : cancel the destination bound to \n"
        );

    while (1)
    {
        ProSleep(1);
        RtpMsgUser2String(&bindUser, bindString);
#if defined(WIN32)
        printf("\nMSG-Cli-To:%s\\>", bindString);
#else
        printf("\nMSG-Cli-To:%s~$ ", bindString);
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
            continue;
        }

        if (stricmp(p, "help") == 0 || stricmp(p, "--help") == 0 ||
            stricmp(p, "bind") == 0)
        {
            printf(
                "\n"
                " help      : show this message \n"
                " bind <id> : bind to a destination to send to. \n"
                "             for example, \"bind 2-1-1\" \n"
                " unbind    : cancel the destination bound to \n"
                );
        }
        else if (strnicmp(p, "bind ", 5) == 0)
        {
            p += 5;

            RTP_MSG_USER user;
            RtpMsgString2User(p, &user);
            if (user.classId == 0 || user.UserId() == 0)
            {
                continue;
            }

            bindUser = user;
            printf("\n binding... \n");
        }
        else if (stricmp(p, "unbind") == 0)
        {
            bindUser.Zero();
            printf("\n unbinding... \n");
        }
        else
        {
            tester->SendMsg(p, &bindUser);
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
