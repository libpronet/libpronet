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

#include "c2s_server.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
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
        if (m_reactor != NULL || m_uplinkSslConfig != NULL ||
            m_localSslConfig != NULL || m_msgC2s != NULL)
        {
            return (false);
        }

        if (configInfo.c2ss_ssl_uplink)
        {
            CProStlVector<const char*>      caFiles;
            CProStlVector<const char*>      crlFiles;
            CProStlVector<PRO_SSL_SUITE_ID> suites;

            int i = 0;
            int c = (int)configInfo.c2ss_ssl_uplink_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.c2ss_ssl_uplink_cafiles[i].empty())
                {
                    caFiles.push_back(
                        &configInfo.c2ss_ssl_uplink_cafiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_uplink_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.c2ss_ssl_uplink_crlfiles[i].empty())
                {
                    crlFiles.push_back(
                        &configInfo.c2ss_ssl_uplink_crlfiles[i][0]);
                }
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

            if (caFiles.size() > 0)
            {
                uplinkSslConfig = ProSslClientConfig_Create();
                if (uplinkSslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslClientConfig_EnableSha1Cert(
                    uplinkSslConfig,
                    configInfo.c2ss_ssl_uplink_enable_sha1cert
                    );

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

                if (!ProSslClientConfig_SetSuiteList(
                    uplinkSslConfig,
                    &suites[0],
                    suites.size()
                    ))
                {
                    goto EXIT;
                }
            }
        }

        if (configInfo.c2ss_ssl_local)
        {
            CProStlVector<const char*> caFiles;
            CProStlVector<const char*> crlFiles;
            CProStlVector<const char*> certFiles;

            int i = 0;
            int c = (int)configInfo.c2ss_ssl_local_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.c2ss_ssl_local_cafiles[i].empty())
                {
                    caFiles.push_back(
                        &configInfo.c2ss_ssl_local_cafiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_local_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.c2ss_ssl_local_crlfiles[i].empty())
                {
                    crlFiles.push_back(
                        &configInfo.c2ss_ssl_local_crlfiles[i][0]);
                }
            }

            i = 0;
            c = (int)configInfo.c2ss_ssl_local_certfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.c2ss_ssl_local_certfiles[i].empty())
                {
                    certFiles.push_back(
                        &configInfo.c2ss_ssl_local_certfiles[i][0]);
                }
            }

            if (caFiles.size() > 0 && certFiles.size() > 0)
            {
                localSslConfig = ProSslServerConfig_Create();
                if (localSslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslServerConfig_EnableSha1Cert(
                    localSslConfig,
                    configInfo.c2ss_ssl_local_enable_sha1cert
                    );

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
        }

        msgC2s = CreateRtpMsgC2s(
            this,
            reactor,
            configInfo.c2ss_mm_type,
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

        msgC2s->SetUplinkOutputRedline(configInfo.c2ss_uplink_redline_bytes);
        msgC2s->SetLocalOutputRedline(configInfo.c2ss_local_redline_bytes);

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
CC2sServer::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
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

        m_configInfo.c2ss_log_loop_bytes  = configInfo.c2ss_log_loop_bytes;
        m_configInfo.c2ss_log_level_green = configInfo.c2ss_log_level_green;

        m_logFile.SetMaxSize(configInfo.c2ss_log_loop_bytes);
        m_logFile.SetGreenLevel(configInfo.c2ss_log_level_green);
    }

    {{{
        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n"
            " CC2sServer::Reconfig(%u, %d) \n"
            ,
            configInfo.c2ss_log_loop_bytes,
            configInfo.c2ss_log_level_green
            );
        m_logFile.Log(traceInfo, PRO_LL_MAX, true);
    }}}
}

void
CC2sServer::OnOkC2s(IRtpMsgC2s*         msgC2s,
                    const RTP_MSG_USER* myUser,
                    const char*         myPublicIp)
{
    assert(msgC2s != NULL);
    assert(myUser != NULL);
    assert(myPublicIp != NULL);
    assert(myPublicIp[0] != '\0');
    if (msgC2s == NULL || myUser == NULL || myPublicIp == NULL ||
        myPublicIp[0] == '\0')
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
        char           suiteName[64] = "";
        char           remoteIp[64]  = "";
        unsigned short remotePort    = 0;
        msgC2s->GetUplinkSslSuite(suiteName);
        msgC2s->GetUplinkRemoteIp(remoteIp);
        remotePort = msgC2s->GetUplinkRemotePort();

        CProStlString timeString = "";
        ProGetLocalTimeString(timeString);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n"
            "%s \n"
            " CC2sServer::OnOkC2s(id : %u-" PRO_PRT64U "-%u, publicIp : %s,"
            " sslSuite : %s, server : %s:%u) \n"
            ,
            timeString.c_str(),
            (unsigned int)myUser->classId,
            myUser->UserId(),
            (unsigned int)myUser->instId,
            myPublicIp,
            suiteName,
            remoteIp,
            (unsigned int)remotePort
            );
        printf("%s", traceInfo);
        m_logFile.Log(traceInfo, PRO_LL_INFO, false);
    }}}
}

void
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
        RTP_MSG_USER myUser;
        msgC2s->GetUplinkUser(&myUser);

        char           remoteIp[64] = "";
        unsigned short remotePort   = 0;
        msgC2s->GetUplinkRemoteIp(remoteIp);
        remotePort = msgC2s->GetUplinkRemotePort();

        CProStlString timeString = "";
        ProGetLocalTimeString(timeString);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n"
            "%s \n"
            " CC2sServer::OnCloseC2s(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], tcpConnected : %d, server : %s:%u) \n"
            ,
            timeString.c_str(),
            (unsigned int)myUser.classId,
            myUser.UserId(),
            (unsigned int)myUser.instId,
            (int)errorCode,
            (int)sslCode,
            (int)tcpConnected,
            remoteIp,
            (unsigned int)remotePort
            );
        printf("%s", traceInfo);
        m_logFile.Log(traceInfo, PRO_LL_ERROR, false);
    }}}
}

void
CC2sServer::OnOkUser(IRtpMsgC2s*         msgC2s,
                     const RTP_MSG_USER* user,
                     const char*         userPublicIp)
{
    assert(msgC2s != NULL);
    assert(user != NULL);
    assert(userPublicIp != NULL);
    assert(userPublicIp[0] != '\0');
    if (msgC2s == NULL || user == NULL || userPublicIp == NULL ||
        userPublicIp[0] == '\0')
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
        char          suiteName[64] = "";
        unsigned long userCount     = 0;
        msgC2s->GetLocalSslSuite(user, suiteName);
        msgC2s->GetLocalUserCount(NULL, &userCount);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n"
            " CC2sServer::OnOkUser(id : %u-" PRO_PRT64U "-%u, fromIp : %s,"
            " sslSuite : %s, users : %u) \n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            userPublicIp,
            suiteName,
            (unsigned int)userCount
            );
        m_logFile.Log(traceInfo, PRO_LL_INFO, true);
    }}}
}

void
CC2sServer::OnCloseUser(IRtpMsgC2s*         msgC2s,
                        const RTP_MSG_USER* user,
                        long                errorCode,
                        long                sslCode)
{
    assert(msgC2s != NULL);
    assert(user != NULL);
    if (msgC2s == NULL || user == NULL)
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
        msgC2s->GetLocalUserCount(NULL, &userCount);

        char traceInfo[1024] = "";
        snprintf_pro(
            traceInfo,
            sizeof(traceInfo),
            "\n"
            " CC2sServer::OnCloseUser(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], users : %u) \n"
            ,
            (unsigned int)user->classId,
            user->UserId(),
            (unsigned int)user->instId,
            (int)errorCode,
            (int)sslCode,
            (unsigned int)userCount
            );
        m_logFile.Log(traceInfo, PRO_LL_INFO, true);
    }}}
}
