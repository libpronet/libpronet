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

#include "c2s_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_log_file.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define LOG_FILE_NAME    "rtp_msg_c2s.log"
#define CONFIG_FILE_NAME "rtp_msg_c2s.cfg"

static const unsigned char SERVER_CID    = 1;                                     /* 1-... */
static const PRO_UINT64    NODE_UID_MIN  = 1;                                     /* 1 ~ 0xFFFFFFFFFF */
static const PRO_UINT64    NODE_UID_MAXX = ((PRO_UINT64)0xFF << 32) | 0xFFFFFFFF; /* 1 ~ 0xFFFFFFFFFF */

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();
    ProRtpInit();

    CProLogFile* const     logFile        = new CProLogFile;
    IProReactor*           reactor        = NULL;
    CC2sServer*            server         = NULL;
    CProStlString          logFileName    = "";
    CProStlString          configFileName = "";
    C2S_SERVER_CONFIG_INFO configInfo;

    char exeRoot[1024] = "";
    ProGetExeDir_(exeRoot);

    {
        logFileName    =  exeRoot;
        logFileName    += LOG_FILE_NAME;
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
                " rtp_msg_c2s --- warning! can't read the config file. \n"
                " [ %s ] \n\n"
                ,
                configFileName.c_str()
                );
        }

        configInfo.c2ss_ssl_uplink_cafiles.clear();
        configInfo.c2ss_ssl_uplink_crlfiles.clear();
        configInfo.c2ss_ssl_local_cafiles.clear();
        configInfo.c2ss_ssl_local_crlfiles.clear();
        configInfo.c2ss_ssl_local_certfiles.clear();

        int       i = 0;
        const int c = (int)configs.size();

        for (; i < c; ++i)
        {
            CProStlString& configName  = configs[i].configName;
            CProStlString& configValue = configs[i].configValue;

            if (stricmp(configName.c_str(), "c2ss_thread_count") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 100)
                {
                    configInfo.c2ss_thread_count = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.c2ss_uplink_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.c2ss_uplink_port = (unsigned short)value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_id") == 0)
            {
                if (!configValue.empty())
                {
                    RTP_MSG_USER uplinkId;
                    RtpMsgString2User(configValue.c_str(), &uplinkId);

                    if (uplinkId.classId == SERVER_CID &&
                        uplinkId.UserId() >= NODE_UID_MIN && uplinkId.UserId() <= NODE_UID_MAXX)
                    {
                        configInfo.c2ss_uplink_id = uplinkId;
                    }
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_password") == 0)
            {
                configInfo.c2ss_uplink_password = configValue;

                if (!configValue.empty())
                {
                    ProZeroMemory(&configValue[0], configValue.length());
                    configValue = "";
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_local_ip") == 0)
            {
                if (!configValue.empty())
                {
                    configInfo.c2ss_uplink_local_ip = configValue;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_timeout") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.c2ss_uplink_timeout = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_uplink_redline_bytes") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.c2ss_uplink_redline_bytes = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_local_hub_port") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0 && value <= 65535)
                {
                    configInfo.c2ss_local_hub_port = (unsigned short)value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_local_timeout") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.c2ss_local_timeout = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_local_redline_bytes") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.c2ss_local_redline_bytes = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_mm_type") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value >= (int)RTP_MMT_MSG_MIN && value <= (int)RTP_MMT_MSG_MAX)
                {
                    configInfo.c2ss_mm_type = (RTP_MM_TYPE)value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_enable_ssl") == 0)
            {
                configInfo.c2ss_enable_ssl = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_enable_sha1cert") == 0)
            {
                configInfo.c2ss_ssl_enable_sha1cert = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_uplink_cafile") == 0)
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
                    configInfo.c2ss_ssl_uplink_cafiles.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_uplink_crlfile") == 0)
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
                    configInfo.c2ss_ssl_uplink_crlfiles.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_uplink_sni") == 0)
            {
                configInfo.c2ss_ssl_uplink_sni = configValue;
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_uplink_aes256") == 0)
            {
                configInfo.c2ss_ssl_uplink_aes256 = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_local_forced") == 0)
            {
                configInfo.c2ss_ssl_local_forced = atoi(configValue.c_str()) != 0;
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_local_cafile") == 0)
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
                    configInfo.c2ss_ssl_local_cafiles.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_local_crlfile") == 0)
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
                    configInfo.c2ss_ssl_local_crlfiles.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_local_certfile") == 0)
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
                    configInfo.c2ss_ssl_local_certfiles.push_back(configValue);
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_ssl_local_keyfile") == 0)
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

                configInfo.c2ss_ssl_local_keyfile = configValue;
            }
            else if (stricmp(configName.c_str(), "c2ss_log_loop_bytes") == 0)
            {
                const int value = atoi(configValue.c_str());
                if (value > 0)
                {
                    configInfo.c2ss_log_loop_bytes = value;
                }
            }
            else if (stricmp(configName.c_str(), "c2ss_log_level_green") == 0)
            {
                configInfo.c2ss_log_level_green    = atoi(configValue.c_str());
            }
            else if (stricmp(configName.c_str(), "c2ss_log_level_status") == 0)
            {
                configInfo.c2ss_log_level_status   = atoi(configValue.c_str());
            }
            else if (stricmp(configName.c_str(), "c2ss_log_level_userin") == 0)
            {
                configInfo.c2ss_log_level_userin   = atoi(configValue.c_str());
            }
            else if (stricmp(configName.c_str(), "c2ss_log_level_userout") == 0)
            {
                configInfo.c2ss_log_level_userout  = atoi(configValue.c_str());
            }
            else
            {
            }
        } /* end of for (...) */
    }

    static char s_traceInfo[1024] = "";

    logFile->Init(logFileName.c_str(), true);
    if (logFile->GetPos() > 0)
    {
        printf(" rtp_msg_c2s --- error! please delete the log file first. \n\n");

        goto EXIT;
    }

    reactor = ProCreateReactor(configInfo.c2ss_thread_count);
    if (reactor == NULL)
    {
        strcpy(s_traceInfo, " rtp_msg_c2s --- error! can't create reactor. \n\n");
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo);

        goto EXIT;
    }

    server = CC2sServer::CreateInstance(*logFile);
    if (server == NULL || !server->Init(reactor, configInfo))
    {
        strcpy(s_traceInfo, " rtp_msg_c2s --- error! can't create server. \n\n");
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo);

        goto EXIT;
    }

    if (!configInfo.c2ss_uplink_password.empty())
    {
        ProZeroMemory(
            &configInfo.c2ss_uplink_password[0], configInfo.c2ss_uplink_password.length());
        configInfo.c2ss_uplink_password = "";
    }

    snprintf_pro(
        s_traceInfo,
        sizeof(s_traceInfo),
        " rtp_msg_c2s --- [servicePort : %u, server : %s:%u, mmType : %u] --- ok! \n\n"
        ,
        (unsigned int)configInfo.c2ss_local_hub_port,
        configInfo.c2ss_uplink_ip.c_str(),
        (unsigned int)configInfo.c2ss_uplink_port,
        (unsigned int)configInfo.c2ss_mm_type
        );
    printf("%s", s_traceInfo);
    logFile->Log(s_traceInfo);

    printf(
        " help     : show this message \n"
        " reconfig : reload logging configs from the file \"rtp_msg_c2s.cfg\" \n"
        " exit     : terminate the current process \n"
        );

    while (1)
    {
        ProSleep(1);
#if defined(WIN32)
        printf("\nMSG-C2S:\\>");
#else
        printf("\nMSG-C2S:~$ ");
#endif
        fflush(stdout);

        static char s_cmdText[1024]      = "";
        s_cmdText[0]                     = '\0';
        s_cmdText[sizeof(s_cmdText) - 1] = '\0';

        char* p = fgets(s_cmdText, sizeof(s_cmdText) - 1, stdin);
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

        if (stricmp(p, "help") == 0 || stricmp(p, "--help") == 0)
        {
            printf(
                "\n"
                " help     : show this message \n"
                " reconfig : reload logging configs from the file \"rtp_msg_c2s.cfg\" \n"
                " exit     : terminate the current process \n"
                );
        }
        else if (stricmp(p, "reconfig") == 0)
        {
            CProConfigFile configFile;
            configFile.Init(configFileName.c_str());

            CProStlVector<PRO_CONFIG_ITEM> configs;
            if (!configFile.Read(configs))
            {
                printf("\n error! can't read the config file. \n");
                continue;
            }

            int       i = 0;
            const int c = (int)configs.size();

            for (; i < c; ++i)
            {
                const CProStlString& configName  = configs[i].configName;
                const CProStlString& configValue = configs[i].configValue;

                if (stricmp(configName.c_str(), "c2ss_log_loop_bytes") == 0)
                {
                    const int value = atoi(configValue.c_str());
                    if (value > 0)
                    {
                        configInfo.c2ss_log_loop_bytes = value;
                    }
                }
                else if (stricmp(configName.c_str(), "c2ss_log_level_green") == 0)
                {
                    configInfo.c2ss_log_level_green    = atoi(configValue.c_str());
                }
                else if (stricmp(configName.c_str(), "c2ss_log_level_status") == 0)
                {
                    configInfo.c2ss_log_level_status   = atoi(configValue.c_str());
                }
                else if (stricmp(configName.c_str(), "c2ss_log_level_userin") == 0)
                {
                    configInfo.c2ss_log_level_userin   = atoi(configValue.c_str());
                }
                else if (stricmp(configName.c_str(), "c2ss_log_level_userout") == 0)
                {
                    configInfo.c2ss_log_level_userout  = atoi(configValue.c_str());
                }
                else
                {
                }
            } /* end of for (...) */

            printf("\n reloading... \n");
            server->Reconfig(configInfo);
        }
        else if (stricmp(p, "exit") == 0)
        {
            strcpy(s_traceInfo, " exiting... \n");
            printf("\n%s", s_traceInfo);
            strcat(s_traceInfo, "\n");
            logFile->Log(s_traceInfo);
            break;
        }
        else
        {
        }
    } /* end of while (...) */

EXIT:

    if (server != NULL)
    {
        server->Fini();
        server->Release();
    }

    ProDeleteReactor(reactor);
    delete logFile;
    ProSleep(3000);

    return (0);
}
