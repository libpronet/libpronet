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

#include "msg_server.h"
#include "msg_db.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_log_file.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

static const unsigned char SERVER_CID = 1; /* 1-... */

/////////////////////////////////////////////////////////////////////////////
////

CMsgServer*
CMsgServer::CreateInstance(CProLogFile&   logFile,
                           CDbConnection& db)
{
    CMsgServer* const server = new CMsgServer(logFile, db);

    return (server);
}

CMsgServer::CMsgServer(CProLogFile&   logFile,
                       CDbConnection& db)
                       :
m_logFile(logFile),
m_db(db)
{
    m_reactor   = NULL;
    m_sslConfig = NULL;
    m_msgServer = NULL;
}

CMsgServer::~CMsgServer()
{
    Fini();
}

bool
CMsgServer::Init(IProReactor*                  reactor,
                 const MSG_SERVER_CONFIG_INFO& configInfo)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (false);
    }

    PRO_SSL_SERVER_CONFIG* sslConfig = NULL;
    IRtpMsgServer*         msgServer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_sslConfig == NULL);
        assert(m_msgServer == NULL);
        if (m_reactor != NULL || m_sslConfig != NULL || m_msgServer != NULL)
        {
            return (false);
        }

        if (configInfo.msgs_enable_ssl)
        {
            CProStlVector<const char*> caFiles;
            CProStlVector<const char*> crlFiles;
            CProStlVector<const char*> certFiles;

            int i = 0;
            int c = (int)configInfo.msgs_ssl_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.msgs_ssl_cafiles[i].empty())
                {
                    caFiles.push_back(&configInfo.msgs_ssl_cafiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.msgs_ssl_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.msgs_ssl_crlfiles[i].empty())
                {
                    crlFiles.push_back(&configInfo.msgs_ssl_crlfiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.msgs_ssl_certfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.msgs_ssl_certfiles[i].empty())
                {
                    certFiles.push_back(&configInfo.msgs_ssl_certfiles[i][0]);
                }
            }

            if (caFiles.size() > 0 && certFiles.size() > 0)
            {
                sslConfig = ProSslServerConfig_Create();
                if (sslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslServerConfig_EnableSha1Cert(sslConfig, configInfo.msgs_ssl_enable_sha1cert);

                if (!ProSslServerConfig_SetCaList(
                    sslConfig,
                    &caFiles[0],
                    caFiles.size(),
                    crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                    crlFiles.size()
                    ))
                {
                    goto EXIT;
                }

                if (!ProSslServerConfig_AppendCertChain(
                    sslConfig,
                    &certFiles[0],
                    certFiles.size(),
                    configInfo.msgs_ssl_keyfile.c_str(),
                    NULL
                    ))
                {
                    goto EXIT;
                }
            }
        }

        msgServer = CreateRtpMsgServer(
            this,
            reactor,
            configInfo.msgs_mm_type,
            sslConfig,
            configInfo.msgs_ssl_forced,
            configInfo.msgs_hub_port,
            configInfo.msgs_handshake_timeout
            );
        if (msgServer == NULL)
        {
            goto EXIT;
        }

        msgServer->SetOutputRedlineToC2s(configInfo.msgs_redline_bytes_c2s);
        msgServer->SetOutputRedlineToUsr(configInfo.msgs_redline_bytes_usr);

        m_reactor    = reactor;
        m_configInfo = configInfo;
        m_sslConfig  = sslConfig;
        m_msgServer  = msgServer;
    }

    return (true);

EXIT:

    DeleteRtpMsgServer(msgServer);
    ProSslServerConfig_Delete(sslConfig);

    return (false);
}

void
CMsgServer::Fini()
{
    PRO_SSL_SERVER_CONFIG* sslConfig = NULL;
    IRtpMsgServer*         msgServer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return;
        }

        msgServer = m_msgServer;
        m_msgServer = NULL;
        sslConfig = m_sslConfig;
        m_sslConfig = NULL;
        m_reactor = NULL;
    }

    DeleteRtpMsgServer(msgServer);
    ProSslServerConfig_Delete(sslConfig);
}

unsigned long
PRO_CALLTYPE
CMsgServer::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CMsgServer::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CMsgServer::KickoutUsers(const CProStlSet<RTP_MSG_USER>& users)
{
    if (users.size() == 0)
    {
        return;
    }

    CProStlString traceInfo = "";

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return;
        }

        traceInfo += '\n';
        traceInfo += " CMsgServer::KickoutUsers(...) \n";
        traceInfo += " [[[ begin \n";

        CProStlSet<RTP_MSG_USER>::const_iterator       itr = users.begin();
        CProStlSet<RTP_MSG_USER>::const_iterator const end = users.end();

        for (; itr != end; ++itr)
        {
            const RTP_MSG_USER& user = *itr;
            if (user.classId == 0 || user.UserId() == 0)
            {
                continue;
            }

            m_msgServer->KickoutUser(&user);

            char idString[64] = "";
            RtpMsgUser2String(&user, idString);
            traceInfo += "\t ";
            traceInfo += idString;
            traceInfo += " \n";
        }

        traceInfo += " ]]] end \n";
    }

    {{{
        m_logFile.Log(traceInfo.c_str());
    }}}
}

void
CMsgServer::Reconfig(const MSG_SERVER_CONFIG_INFO& configInfo)
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return;
        }

        m_configInfo.msgs_log_loop_bytes    = configInfo.msgs_log_loop_bytes;
        m_configInfo.msgs_log_level_green   = configInfo.msgs_log_level_green;
        m_configInfo.msgs_log_level_userchk = configInfo.msgs_log_level_userchk;
        m_configInfo.msgs_log_level_userin  = configInfo.msgs_log_level_userin;
        m_configInfo.msgs_log_level_userout = configInfo.msgs_log_level_userout;

        m_logFile.SetGreenLevel(configInfo.msgs_log_level_green);
        m_logFile.SetMaxSize(configInfo.msgs_log_loop_bytes);
    }

    {{{
        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n CMsgServer::Reconfig(%u, %d, %d, %d, %d) \n"
            ,
            configInfo.msgs_log_loop_bytes,
            configInfo.msgs_log_level_green,
            configInfo.msgs_log_level_userchk,
            configInfo.msgs_log_level_userin,
            configInfo.msgs_log_level_userout
            );
        printf("%s", traceInfo);
        m_logFile.Log(traceInfo, configInfo.msgs_log_level_green); /* green */
    }}}
}

bool
PRO_CALLTYPE
CMsgServer::OnCheckUser(IRtpMsgServer*      msgServer,
                        const RTP_MSG_USER* user,
                        const char*         userPublicIp,
                        const RTP_MSG_USER* c2sUser, /* = NULL */
                        const char          hash[32],
                        PRO_UINT64          nonce,
                        PRO_UINT64*         userId,
                        PRO_UINT16*         instId,
                        PRO_INT64*          appData,
                        bool*               isC2s)
{
    assert(msgServer != NULL);
    assert(user != NULL);
    assert(user->classId > 0);
    assert(user->UserId() > 0);
    assert(userPublicIp != NULL);
    assert(userPublicIp[0] != '\0');
    assert(userId != NULL);
    assert(instId != NULL);
    assert(appData != NULL);
    assert(isC2s != NULL);
    if (msgServer == NULL || user == NULL || user->classId == 0 || user->UserId() == 0 ||
        userPublicIp == NULL || userPublicIp[0] == '\0' ||
        userId == NULL || instId == NULL || appData == NULL || isC2s == NULL)
    {
        return (false);
    }

    char c2sIdString[64] = "NULL";
    if (c2sUser != NULL)
    {
        RtpMsgUser2String(c2sUser, c2sIdString);
    }

    bool          ret         = false;
    CProStlString errorString = "";

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return (false);
        }

        if (msgServer != m_msgServer)
        {
            return (false);
        }

        const MSG_USER_CTX* ctx = NULL;

        CProStlMap<PRO_UINT64, MSG_USER_CTX>::const_iterator const itr =
            m_uid2Ctx[user->classId].find(user->UserId());
        if (itr != m_uid2Ctx[user->classId].end())
        {
            ctx = &itr->second;
        }

        TBL_MSG_USER_ROW userRow;

        {
            long ret2 = GetMsgUserRow(m_db, *user, userRow); /* uid */
            if (ret2 == 0)
            {
                const RTP_MSG_USER user0(user->classId, 0, 0);
                ret2 = GetMsgUserRow(m_db, user0, userRow);  /* uid0 */
            }

            if (ret2 < 0)
            {
                errorString = "Internal Error";

                goto EXIT;
            }
            else if (ret2 == 0)
            {
                errorString = "Invalid ID";

                goto EXIT;
            }
            else
            {
                const PRO_UINT32 bindedIp = pbsd_inet_aton(userRow._bindedip_.c_str());
                if (bindedIp != (PRO_UINT32)-1 && bindedIp != 0 &&
                    pbsd_inet_aton(userPublicIp) != bindedIp)
                {
                    errorString = "Mismatched IP";

                    goto EXIT;
                }
            }
        }

        if (ctx != NULL)
        {
            if (ctx->iids.size() >= (size_t)userRow._maxiids_ &&
                ctx->iids.find(user->instId) == ctx->iids.end())
            {
                errorString = "Too Many Insts";

                goto EXIT;
            }

            if (user->classId == SERVER_CID &&
                ctx->iids.find(user->instId) != ctx->iids.end())
            {
                errorString = "Buzy ID";

                goto EXIT;
            }
        }

        if (!CheckRtpServiceData(nonce, userRow._passwd_.c_str(), hash))
        {
            errorString = "Wrong Password";

            goto EXIT;
        }

        *userId  = user->UserId();
        *instId  = user->instId;
        *appData = 0; /* You can do something. */
        *isC2s   = userRow._isc2s_ != 0;
    }

    ret = true;

EXIT:

    {{{
        char traceInfo[1024] = "";
        if (ret)
        {
            snprintf_pro(
                traceInfo,
                sizeof(traceInfo),
                "\n CMsgServer::OnCheckUser(id : %u-" PRO_PRT64U "-%u,"
                " fromIp : %s, fromC2s : %s) ok! \n"
                ,
                (unsigned int)user->classId,
                user->UserId(),
                (unsigned int)user->instId,
                userPublicIp,
                c2sIdString
                );
        }
        else
        {
            snprintf_pro(
                traceInfo,
                sizeof(traceInfo),
                "\n CMsgServer::OnCheckUser(id : %u-" PRO_PRT64U "-%u,"
                " fromIp : %s, fromC2s : %s) failed! [%s] \n"
                ,
                (unsigned int)user->classId,
                user->UserId(),
                (unsigned int)user->instId,
                userPublicIp,
                c2sIdString,
                errorString.c_str()
                );
        }
        m_logFile.Log(traceInfo, m_configInfo.msgs_log_level_userchk);
    }}}

    return (ret);
}

void
PRO_CALLTYPE
CMsgServer::OnOkUser(IRtpMsgServer*      msgServer,
                     const RTP_MSG_USER* user,
                     const char*         userPublicIp,
                     const RTP_MSG_USER* c2sUser, /* = NULL */
                     PRO_INT64           appData)
{
    assert(msgServer != NULL);
    assert(user != NULL);
    assert(userPublicIp != NULL);
    assert(userPublicIp[0] != '\0');
    if (msgServer == NULL || user == NULL || userPublicIp == NULL || userPublicIp[0] == '\0')
    {
        return;
    }

    char c2sIdString[64] = "NULL";
    if (c2sUser != NULL)
    {
        RtpMsgUser2String(c2sUser, c2sIdString);
    }

    char suiteName[64] = "";
    msgServer->GetSslSuite(user, suiteName);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return;
        }

        if (msgServer != m_msgServer)
        {
            return;
        }

        MSG_USER_CTX& ctx = m_uid2Ctx[user->classId][user->UserId()];
        ctx.iids.insert(user->instId);

        if (!m_configInfo.msgs_db_readonly)
        {
            AddMsgOnlineRow(m_db, *user, userPublicIp, c2sIdString, suiteName);
        }
    }

    {{{
        unsigned long baseUserCount = 0;
        unsigned long subUserCount  = 0;
        msgServer->GetUserCount(NULL, &baseUserCount, &subUserCount);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n CMsgServer::OnOkUser(id : %u-" PRO_PRT64U "-%u,"
            " fromIp : %s, fromC2s : %s, sslSuite : %s, users : %u+%u) \n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            userPublicIp,
            c2sIdString,
            suiteName,
            (unsigned int)baseUserCount,
            (unsigned int)subUserCount
            );
        m_logFile.Log(traceInfo, m_configInfo.msgs_log_level_userin);
    }}}
}

void
PRO_CALLTYPE
CMsgServer::OnCloseUser(IRtpMsgServer*      msgServer,
                        const RTP_MSG_USER* user,
                        long                errorCode,
                        long                sslCode)
{
    assert(msgServer != NULL);
    assert(user != NULL);
    if (msgServer == NULL || user == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgServer == NULL)
        {
            return;
        }

        if (msgServer != m_msgServer)
        {
            return;
        }

        CProStlMap<PRO_UINT64, MSG_USER_CTX>::iterator const itr =
            m_uid2Ctx[user->classId].find(user->UserId());
        if (itr == m_uid2Ctx[user->classId].end())
        {
            return;
        }

        MSG_USER_CTX& ctx = itr->second;
        ctx.iids.erase(user->instId);
        if (ctx.iids.size() == 0)
        {
            m_uid2Ctx[user->classId].erase(itr);
        }

        if (!m_configInfo.msgs_db_readonly)
        {
            RemoveMsgOnlineRow(m_db, *user);
        }
    }

    {{{
        unsigned long baseUserCount = 0;
        unsigned long subUserCount  = 0;
        msgServer->GetUserCount(NULL, &baseUserCount, &subUserCount);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n CMsgServer::OnCloseUser(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], users : %u+%u) \n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            (int)errorCode,
            (int)sslCode,
            (unsigned int)baseUserCount,
            (unsigned int)subUserCount
            );
        m_logFile.Log(traceInfo, m_configInfo.msgs_log_level_userout);
    }}}
}
