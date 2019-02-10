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
#include "../pro_rtp/rtp_foundation.h"
#include "../pro_rtp/rtp_framework.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_log_file.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CC2sServer*
CC2sServer::CreateInstance(CProLogFile& logFile)
{
    CC2sServer* const server = new CC2sServer(logFile);

    return (server);
}

CC2sServer::CC2sServer(CProLogFile& logFile)
: m_logFile(logFile)
{
    m_reactor         = NULL;
    m_uplinkSslConfig = NULL;
    m_localSslConfig  = NULL;
    m_msgC2s          = NULL;
    m_onOkTick        = 0;
}

CC2sServer::~CC2sServer()
{
    Fini();
}

bool
CC2sServer::Init(IProReactor*                  reactor,
                 const C2S_SERVER_CONFIG_INFO& configInfo)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (false);
    }

    PRO_SSL_CLIENT_CONFIG* uplinkSslConfig = NULL;
    PRO_SSL_SERVER_CONFIG* localSslConfig  = NULL;
    IRtpMsgC2s*            msgC2s          = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_uplinkSslConfig == NULL);
        assert(m_localSslConfig == NULL);
        assert(m_msgC2s == NULL);
        if (m_reactor != NULL ||
            m_uplinkSslConfig != NULL || m_localSslConfig != NULL || m_msgC2s != NULL)
        {
            return (false);
        }

        if (configInfo.c2ss_enable_ssl && configInfo.c2ss_ssl_uplink_cafile.size() > 0)
        {
            uplinkSslConfig = ProSslClientConfig_Create();
            if (uplinkSslConfig == NULL)
            {
                goto EXIT;
            }

            ProSslClientConfig_EnableSha1Cert(
                uplinkSslConfig, configInfo.c2ss_ssl_enable_sha1cert);

            CProStlVector<const char*>      caFiles;
            CProStlVector<const char*>      crlFiles;
            CProStlVector<PRO_SSL_SUITE_ID> suites;

            int i = 0;
            int c = (int)configInfo.c2ss_ssl_uplink_cafile.size();

            for (; i < c; ++i)
            {
                caFiles.push_back(&configInfo.c2ss_ssl_uplink_cafile[i][0]);
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_uplink_crlfile.size();

            for (; i < c; ++i)
            {
                crlFiles.push_back(&configInfo.c2ss_ssl_uplink_crlfile[i][0]);
            }

            if (!ProSslClientConfig_SetCaList(
                uplinkSslConfig,
                &caFiles[0],
                caFiles.size(),
                crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                crlFiles.size()
                ))
            {
                goto EXIT;
            }

            if (configInfo.c2ss_ssl_uplink_aes256)
            {
                suites.push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384);
                suites.push_back(PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
                suites.push_back(PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384);
            }
            else
            {
                suites.push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
                suites.push_back(PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
                suites.push_back(PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256);
            }

            if (!ProSslClientConfig_SetSuiteList(uplinkSslConfig, &suites[0], suites.size()))
            {
                goto EXIT;
            }
        }

        if (configInfo.c2ss_enable_ssl                    &&
            configInfo.c2ss_ssl_local_cafile.size()   > 0 &&
            configInfo.c2ss_ssl_local_certfile.size() > 0)
        {
            localSslConfig = ProSslServerConfig_Create();
            if (localSslConfig == NULL)
            {
                goto EXIT;
            }

            ProSslServerConfig_EnableSha1Cert(
                localSslConfig, configInfo.c2ss_ssl_enable_sha1cert);

            CProStlVector<const char*> caFiles;
            CProStlVector<const char*> crlFiles;
            CProStlVector<const char*> certFiles;

            int i = 0;
            int c = (int)configInfo.c2ss_ssl_local_cafile.size();

            for (; i < c; ++i)
            {
                caFiles.push_back(&configInfo.c2ss_ssl_local_cafile[i][0]);
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_local_crlfile.size();

            for (; i < c; ++i)
            {
                crlFiles.push_back(&configInfo.c2ss_ssl_local_crlfile[i][0]);
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_local_certfile.size();

            for (; i < c; ++i)
            {
                certFiles.push_back(&configInfo.c2ss_ssl_local_certfile[i][0]);
            }

            if (!ProSslServerConfig_SetCaList(
                localSslConfig,
                &caFiles[0],
                caFiles.size(),
                crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                crlFiles.size()
                ))
            {
                goto EXIT;
            }

            if (!ProSslServerConfig_AppendCertChain(
                localSslConfig,
                &certFiles[0],
                certFiles.size(),
                configInfo.c2ss_ssl_local_keyfile.c_str(),
                NULL
                ))
            {
                goto EXIT;
            }
        }

        msgC2s = CreateRtpMsgC2s(
            this,
            reactor,
            RTP_MMT_MSG,
            uplinkSslConfig,
            configInfo.c2ss_ssl_uplink_sni.c_str(),
            configInfo.c2ss_uplink_ip.c_str(),
            configInfo.c2ss_uplink_port,
            &configInfo.c2ss_uplink_id,
            configInfo.c2ss_uplink_password.c_str(),
            configInfo.c2ss_uplink_local_ip.c_str(),
            configInfo.c2ss_uplink_timeout,
            localSslConfig,
            configInfo.c2ss_ssl_local_forced,
            configInfo.c2ss_local_hub_port,
            configInfo.c2ss_local_timeout
            );
        if (msgC2s == NULL)
        {
            goto EXIT;
        }

        m_reactor         = reactor;
        m_configInfo      = configInfo;
        m_uplinkSslConfig = uplinkSslConfig;
        m_localSslConfig  = localSslConfig;
        m_msgC2s          = msgC2s;

        if (!m_configInfo.c2ss_uplink_password.empty())
        {
            ProZeroMemory(
                &m_configInfo.c2ss_uplink_password[0],
                m_configInfo.c2ss_uplink_password.length()
                );
            m_configInfo.c2ss_uplink_password = "";
        }
    }

    return (true);

EXIT:

    DeleteRtpMsgC2s(msgC2s);
    ProSslServerConfig_Delete(localSslConfig);
    ProSslClientConfig_Delete(uplinkSslConfig);

    return (false);
}

void
CC2sServer::Fini()
{
    PRO_SSL_CLIENT_CONFIG* uplinkSslConfig = NULL;
    PRO_SSL_SERVER_CONFIG* localSslConfig  = NULL;
    IRtpMsgC2s*            msgC2s          = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        msgC2s = m_msgC2s;
        m_msgC2s = NULL;
        localSslConfig = m_localSslConfig;
        m_localSslConfig = NULL;
        uplinkSslConfig = m_uplinkSslConfig;
        m_uplinkSslConfig = NULL;
        m_reactor = NULL;
    }

    DeleteRtpMsgC2s(msgC2s);
    ProSslServerConfig_Delete(localSslConfig);
    ProSslClientConfig_Delete(uplinkSslConfig);
}

unsigned long
PRO_CALLTYPE
CC2sServer::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CC2sServer::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CC2sServer::Reconfig(const C2S_SERVER_CONFIG_INFO& configInfo)
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        m_configInfo.c2ss_log_loop_bytes    = configInfo.c2ss_log_loop_bytes;
        m_configInfo.c2ss_log_level_green   = configInfo.c2ss_log_level_green;
        m_configInfo.c2ss_log_level_status  = configInfo.c2ss_log_level_status;
        m_configInfo.c2ss_log_level_userin  = configInfo.c2ss_log_level_userin;
        m_configInfo.c2ss_log_level_userout = configInfo.c2ss_log_level_userout;

        m_logFile.SetGreenLevel(configInfo.c2ss_log_level_green);
    }

    {{{
        char traceInfo[1024] = "";
        traceInfo[sizeof(traceInfo) - 1] = '\0';
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            " CC2sServer::Reconfig(%u, %d, %d, %d, %d) \n"
            ,
            configInfo.c2ss_log_loop_bytes,
            configInfo.c2ss_log_level_green,
            configInfo.c2ss_log_level_status,
            configInfo.c2ss_log_level_userin,
            configInfo.c2ss_log_level_userout
            );
        printf("\n%s", traceInfo);
        strcat(traceInfo, "\n");
        if (m_logFile.GetPos() >= (long)configInfo.c2ss_log_loop_bytes)
        {
            m_logFile.Rewind();
        }
        m_logFile.Log(traceInfo, configInfo.c2ss_log_level_green); /* green */
    }}}
}

void
PRO_CALLTYPE
CC2sServer::OnOkC2s(IRtpMsgC2s*         msgC2s,
                    const RTP_MSG_USER* c2sUser,
                    const char*         c2sPublicIp)
{
    assert(msgC2s != NULL);
    assert(c2sUser != NULL);
    assert(c2sUser->classId > 0);
    assert(c2sUser->UserId() > 0);
    assert(c2sPublicIp != NULL);
    assert(c2sPublicIp[0] != '\0');
    if (msgC2s == NULL || c2sUser == NULL || c2sUser->classId == 0 || c2sUser->UserId() == 0 ||
        c2sPublicIp == NULL || c2sPublicIp[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        if (msgC2s != m_msgC2s)
        {
            return;
        }

        m_onOkTick = ProGetTickCount64();
    }

    {{{
        char traceInfo[1024] = "";
        traceInfo[sizeof(traceInfo) - 1] = '\0';
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            " CC2sServer::OnOkC2s(id : %u-" PRO_PRT64U "-%u, publicIp : %s, server : %s:%u) \n"
            ,
            (unsigned int)c2sUser->classId,
            c2sUser->UserId(),
            (unsigned int)c2sUser->instId,
            c2sPublicIp,
            m_configInfo.c2ss_uplink_ip.c_str(),
            (unsigned int)m_configInfo.c2ss_uplink_port
            );
        printf("\n%s", traceInfo);
        strcat(traceInfo, "\n");
        if (m_logFile.GetPos() >= (long)m_configInfo.c2ss_log_loop_bytes)
        {
            m_logFile.Rewind();
        }
        m_logFile.Log(traceInfo, m_configInfo.c2ss_log_level_status);
    }}}
}

void
PRO_CALLTYPE
CC2sServer::OnCloseC2s(IRtpMsgC2s* msgC2s,
                       long        errorCode,
                       long        sslCode,
                       bool        tcpConnected)
{
    assert(msgC2s != NULL);
    if (msgC2s == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        if (msgC2s != m_msgC2s || m_onOkTick == -1)
        {
            return;
        }

        m_onOkTick = -1;
    }

    {{{
        RTP_MSG_USER c2sUser;
        msgC2s->GetC2sUser(&c2sUser);

        char traceInfo[1024] = "";
        traceInfo[sizeof(traceInfo) - 1] = '\0';
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            " CC2sServer::OnCloseC2s(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], tcpConnected : %d, server : %s:%u) \n"
            ,
            (unsigned int)c2sUser.classId,
            c2sUser.UserId(),
            (unsigned int)c2sUser.instId,
            (int)errorCode,
            (int)sslCode,
            (int)tcpConnected,
            m_configInfo.c2ss_uplink_ip.c_str(),
            (unsigned int)m_configInfo.c2ss_uplink_port
            );
        printf("\n%s", traceInfo);
        strcat(traceInfo, "\n");
        if (m_logFile.GetPos() >= (long)m_configInfo.c2ss_log_loop_bytes)
        {
            m_logFile.Rewind();
        }
        m_logFile.Log(traceInfo, m_configInfo.c2ss_log_level_status);
    }}}
}

void
PRO_CALLTYPE
CC2sServer::OnOkUser(IRtpMsgC2s*         msgC2s,
                     const RTP_MSG_USER* user,
                     const char*         userPublicIp)
{
    assert(msgC2s != NULL);
    assert(user != NULL);
    assert(user->classId > 0);
    assert(user->UserId() > 0);
    assert(userPublicIp != NULL);
    assert(userPublicIp[0] != '\0');
    if (msgC2s == NULL || user == NULL || user->classId == 0 || user->UserId() == 0 ||
        userPublicIp == NULL || userPublicIp[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        if (msgC2s != m_msgC2s)
        {
            return;
        }
    }

    {{{
        unsigned long userCount = 0;
        msgC2s->GetUserCount(NULL, &userCount);

        char traceInfo[1024] = "";
        traceInfo[sizeof(traceInfo) - 1] = '\0';
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            " CC2sServer::OnOkUser(id : %u-" PRO_PRT64U "-%u, fromIp : %s, users : %u) \n\n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            userPublicIp,
            (unsigned int)userCount
            );
        if (m_logFile.GetPos() >= (long)m_configInfo.c2ss_log_loop_bytes)
        {
            m_logFile.Rewind();
        }
        m_logFile.Log(traceInfo, m_configInfo.c2ss_log_level_userin);
    }}}
}

void
PRO_CALLTYPE
CC2sServer::OnCloseUser(IRtpMsgC2s*         msgC2s,
                        const RTP_MSG_USER* user,
                        long                errorCode,
                        long                sslCode)
{
    assert(msgC2s != NULL);
    assert(user != NULL);
    assert(user->classId > 0);
    assert(user->UserId() > 0);
    if (msgC2s == NULL || user == NULL || user->classId == 0 || user->UserId() == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgC2s == NULL)
        {
            return;
        }

        if (msgC2s != m_msgC2s)
        {
            return;
        }
    }

    {{{
        unsigned long userCount = 0;
        msgC2s->GetUserCount(NULL, &userCount);

        char traceInfo[1024] = "";
        traceInfo[sizeof(traceInfo) - 1] = '\0';
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            " CC2sServer::OnCloseUser(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], users : %u) \n\n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            (int)errorCode,
            (int)sslCode,
            (unsigned int)userCount
            );
        if (m_logFile.GetPos() >= (long)m_configInfo.c2ss_log_loop_bytes)
        {
            m_logFile.Rewind();
        }
        m_logFile.Log(traceInfo, m_configInfo.c2ss_log_level_userout);
    }}}
}
