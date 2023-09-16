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

#include "db_connection.h"
#include "msg_db.h"
#include "msg_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_log_file.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define LOG_FILE_NAME    "rtp_msg_server.log"
#define DB_FILE_NAME     "rtp_msg_server.db"
#define CONFIG_FILE_NAME "rtp_msg_server.cfg"
#define LOG_LOOP_BYTES   (50 * 1000 * 1000)

/////////////////////////////////////////////////////////////////////////////
////

static
void
ReadConfig_i(const CProStlString&            exeRoot,
             CProStlVector<PRO_CONFIG_ITEM>& configs,
             MSG_SERVER_CONFIG_INFO&         configInfo)
{
    configInfo.msgs_ssl_cafiles.clear();
    configInfo.msgs_ssl_crlfiles.clear();
    configInfo.msgs_ssl_certfiles.clear();

    int       i = 0;
    const int c = (int)configs.size();

    for (; i < c; ++i)
    {
        CProStlString& configName  = configs[i].configName;
        CProStlString& configValue = configs[i].configValue;

        if (stricmp(configName.c_str(), "msgs_thread_count") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0 && value <= 100)
            {
                configInfo.msgs_thread_count = value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_mm_type") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value >= (int)RTP_MMT_MSG_MIN &&
                value <= (int)RTP_MMT_MSG_MAX)
            {
                configInfo.msgs_mm_type = (RTP_MM_TYPE)value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_hub_port") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0 && value <= 65535)
            {
                configInfo.msgs_hub_port = (unsigned short)value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_handshake_timeout") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.msgs_handshake_timeout = value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_redline_bytes_c2s") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.msgs_redline_bytes_c2s = value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_redline_bytes_usr") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.msgs_redline_bytes_usr = value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_db_readonly") == 0)
        {
            configInfo.msgs_db_readonly = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "msgs_enable_ssl") == 0)
        {
            configInfo.msgs_enable_ssl = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_forced") == 0)
        {
            configInfo.msgs_ssl_forced = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_enable_sha1cert") == 0)
        {
            configInfo.msgs_ssl_enable_sha1cert = atoi(configValue.c_str()) != 0;
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_cafile") == 0)
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
                configInfo.msgs_ssl_cafiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_crlfile") == 0)
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
                configInfo.msgs_ssl_crlfiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_certfile") == 0)
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
                configInfo.msgs_ssl_certfiles.push_back(configValue);
            }
        }
        else if (stricmp(configName.c_str(), "msgs_ssl_keyfile") == 0)
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

            configInfo.msgs_ssl_keyfile = configValue;
        }
        else if (stricmp(configName.c_str(), "msgs_log_loop_bytes") == 0)
        {
            const int value = atoi(configValue.c_str());
            if (value > 0)
            {
                configInfo.msgs_log_loop_bytes = value;
            }
        }
        else if (stricmp(configName.c_str(), "msgs_log_level_green") == 0)
        {
            configInfo.msgs_log_level_green    = atoi(configValue.c_str());
        }
        else
        {
        }
    } /* end of for () */
}

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();
    ProRtpInit();

    CProLogFile*   const   logFile = new CProLogFile;
    CDbConnection* const   db      = new CDbConnection;
    IProReactor*           reactor = NULL;
    CMsgServer*            server  = NULL;
    CProStlString          logFileName;
    CProStlString          dbFileName;
    CProStlString          configFileName;
    MSG_SERVER_CONFIG_INFO configInfo;

    CProStlString timeString;
    ProGetLocalTimeString(timeString);

    char exeRoot[1024] = "";
    ProGetExeDir_(exeRoot);

    {
        logFileName    =  exeRoot;
        logFileName    += LOG_FILE_NAME;
        dbFileName     =  exeRoot;
        dbFileName     += DB_FILE_NAME;
        configFileName =  exeRoot;
        configFileName += CONFIG_FILE_NAME;
    }

    static char s_traceInfo[4096] = "";

    logFile->Reinit(logFileName.c_str(), true); /* append mode */
    logFile->SetMaxSize(LOG_LOOP_BYTES);
    if (logFile->GetSize() > 0)
    {
        logFile->Log("\n\n", PRO_LL_MAX, false);
    }

    {
        CProConfigFile configFile;
        configFile.Init(configFileName.c_str());

        CProStlVector<PRO_CONFIG_ITEM> configs;
        if (!configFile.Read(configs))
        {
            sprintf(
                s_traceInfo,
                "\n"
                "%s \n"
                " rtp_msg_server --- error! can't read the config file. \n"
                " [%s] \n"
                ,
                timeString.c_str(),
                configFileName.c_str()
                );
            printf("%s", s_traceInfo);
            logFile->Log(s_traceInfo, PRO_LL_FATAL, false);

            goto EXIT;
        }

        ReadConfig_i(exeRoot, configs, configInfo);
    }

    logFile->SetMaxSize(configInfo.msgs_log_loop_bytes);
    logFile->SetGreenLevel(configInfo.msgs_log_level_green);

    if (!db->Open(dbFileName.c_str()))
    {
        sprintf(
            s_traceInfo,
            "\n"
            "%s \n"
            " rtp_msg_server --- error! can't open the database. \n"
            ,
            timeString.c_str()
            );
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo, PRO_LL_FATAL, false);

        goto EXIT;
    }
    if (!configInfo.msgs_db_readonly)
    {
        CleanMsgKickoutRows(*db);
        CleanMsgOnlineRows(*db);
    }

    reactor = ProCreateReactor(configInfo.msgs_thread_count);
    if (reactor == NULL)
    {
        sprintf(
            s_traceInfo,
            "\n"
            "%s \n"
            " rtp_msg_server --- error! can't create reactor. \n"
            ,
            timeString.c_str()
            );
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo, PRO_LL_FATAL, false);

        goto EXIT;
    }

    server = CMsgServer::CreateInstance(*logFile, *db);
    if (server == NULL)
    {
        sprintf(
            s_traceInfo,
            "\n"
            "%s \n"
            " rtp_msg_server --- error! can't create server. \n"
            ,
            timeString.c_str()
            );
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo, PRO_LL_FATAL, false);

        goto EXIT;
    }
    if (!server->Init(reactor, configInfo))
    {
        sprintf(
            s_traceInfo,
            "\n"
            "%s \n"
            " rtp_msg_server --- error! can't init server. \n"
            ,
            timeString.c_str()
            );
        printf("%s", s_traceInfo);
        logFile->Log(s_traceInfo, PRO_LL_FATAL, false);

        goto EXIT;
    }

    sprintf(
        s_traceInfo,
        "\n"
        "%s \n"
        " rtp_msg_server [ver-%d.%d.%d] --- [port : %u, mmType : %u] --- ok! \n"
        ,
        timeString.c_str(),
        PRO_VER_MAJOR,
        PRO_VER_MINOR,
        PRO_VER_PATCH,
        (unsigned int)configInfo.msgs_hub_port,
        (unsigned int)configInfo.msgs_mm_type
        );
    printf("%s", s_traceInfo);
    logFile->Log(s_traceInfo, PRO_LL_MAX, false);

    printf(
        "\n"
        " help                      : show this message \n"
        " kickout                   : kick out users listed in the table \"tbl_msg02_kickout\" \n"
        " kickout <id1>[, id2, ...] : kick out users listed in the command line. \n"
        "                             for example, \"kickout 2-1-1, 2-1-2\" \n"
        " reconfig                  : reload logging configs from the file \"rtp_msg_server.cfg\" \n"
        " exit                      : terminate the current process \n"
        );

    while (1)
    {
        ProSleep(1);

        printf("\nMSG-Svr>");
        fflush(stdout);

        static char s_cmdText[4096]      = "";
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

        ProGetLocalTimeString(timeString);

        if (stricmp(p, "help") == 0 || stricmp(p, "--help") == 0)
        {
            printf(
                "\n"
                " help                      : show this message \n"
                " kickout                   : kick out users listed in the table \"tbl_msg02_kickout\" \n"
                " kickout <id1>[, id2, ...] : kick out users listed in the command line. \n"
                "                             for example, \"kickout 2-1-1, 2-1-2\" \n"
                " reconfig                  : reload logging configs from the file \"rtp_msg_server.cfg\" \n"
                " exit                      : terminate the current process \n"
                );
        }
        else if (stricmp(p, "kickout") == 0)
        {
            CProStlVector<TBL_MSG_KICKOUT_ROW> rows;
            GetMsgKickoutRows(*db, rows);
            if (!configInfo.msgs_db_readonly)
            {
                CleanMsgKickoutRows(*db);
            }

            CProStlSet<RTP_MSG_USER> users;

            int       i = 0;
            const int c = (int)rows.size();

            for (; i < c; ++i)
            {
                const TBL_MSG_KICKOUT_ROW& row = rows[i];

                const RTP_MSG_USER user((unsigned char)row._cid_, row._uid_, (uint16_t)row._iid_);
                users.insert(user);
            }

            if (users.size() > 0)
            {
                sprintf(
                    s_traceInfo,
                    "\n"
                    "%s \n"
                    " kickout : kicking... \n"
                    ,
                    timeString.c_str()
                    );
                printf("%s", s_traceInfo);
                logFile->Log(s_traceInfo, PRO_LL_INFO, false);

                server->KickoutUsers(users);
            }
        }
        else if (strnicmp(p, "kickout ", 8) == 0)
        {
            CProStlSet<RTP_MSG_USER> users;

            p += 8;

            while (*p != '\0')
            {
                char*        q = NULL;
                RTP_MSG_USER user;

                for (; *p != '\0'; ++p)
                {
                    if (*p != ' ' && *p != '\t' && *p != ',')
                    {
                        break;
                    }
                }

                for (q = p; *q != '\0'; ++q)
                {
                    if (*q == ' ' || *q == '\t' || *q == ',')
                    {
                        break;
                    }
                }

                if (*q != '\0')
                {
                    *q = '\0';

                    RtpMsgString2User(p, &user);
                    p = q + 1;
                }
                else
                {
                    RtpMsgString2User(p, &user);
                    p = q;
                }

                if (user.classId > 0 && user.UserId() > 0)
                {
                    users.insert(user);
                }
            } /* end of while () */

            if (users.size() > 0)
            {
                sprintf(
                    s_traceInfo,
                    "\n"
                    "%s \n"
                    " kickout <id> : kicking... \n"
                    ,
                    timeString.c_str()
                    );
                printf("%s", s_traceInfo);
                logFile->Log(s_traceInfo, PRO_LL_INFO, false);

                server->KickoutUsers(users);
            }
        }
        else if (stricmp(p, "reconfig") == 0)
        {
            CProConfigFile configFile;
            configFile.Init(configFileName.c_str());

            CProStlVector<PRO_CONFIG_ITEM> configs;
            if (!configFile.Read(configs))
            {
                sprintf(
                    s_traceInfo,
                    "\n"
                    "%s \n"
                    " reconfig : error! can't read the config file. \n"
                    ,
                    timeString.c_str()
                    );
                printf("%s", s_traceInfo);
                logFile->Log(s_traceInfo, PRO_LL_WARN, false);
                continue;
            }

            int       i = 0;
            const int c = (int)configs.size();

            for (; i < c; ++i)
            {
                const CProStlString& configName  = configs[i].configName;
                const CProStlString& configValue = configs[i].configValue;

                if (stricmp(configName.c_str(), "msgs_log_loop_bytes") == 0)
                {
                    const int value = atoi(configValue.c_str());
                    if (value > 0)
                    {
                        configInfo.msgs_log_loop_bytes = value;
                    }
                }
                else if (stricmp(configName.c_str(), "msgs_log_level_green") == 0)
                {
                    configInfo.msgs_log_level_green    = atoi(configValue.c_str());
                }
                else
                {
                }
            }

            sprintf(
                s_traceInfo,
                "\n"
                "%s \n"
                " reconfig : reloading... \n"
                ,
                timeString.c_str()
                );
            printf("%s", s_traceInfo);
            logFile->Log(s_traceInfo, PRO_LL_INFO, false);

            server->Reconfig(configInfo);
        }
        else if (stricmp(p, "exit") == 0)
        {
            sprintf(
                s_traceInfo,
                "\n"
                "%s \n"
                " exit : exiting... \n"
                ,
                timeString.c_str()
                );
            printf("%s", s_traceInfo);
            logFile->Log(s_traceInfo, PRO_LL_MAX, false);
            break;
        }
        else
        {
        }
    } /* end of while () */

EXIT:

    if (server != NULL)
    {
        server->Fini();
        server->Release();
    }

    ProDeleteReactor(reactor);
    delete db;
    delete logFile;

    return (0);
}
