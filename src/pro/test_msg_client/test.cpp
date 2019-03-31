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
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_unicode.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance()
{
    CTest* const tester = new CTest;

    return (tester);
}

CTest::CTest()
{
    m_reactor   = NULL;
    m_sslConfig = NULL;
    m_msgClient = NULL;
}

CTest::~CTest()
{
    Fini();
}

bool
CTest::Init(IProReactor*                  reactor,
            const MSG_CLIENT_CONFIG_INFO& configInfo)
{
    assert(reactor != NULL);
    if (reactor == NULL)
    {
        return (false);
    }

    PRO_SSL_CLIENT_CONFIG* sslConfig = NULL;
    IRtpMsgClient*         msgClient = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_reactor == NULL);
        assert(m_sslConfig == NULL);
        assert(m_msgClient == NULL);
        if (m_reactor != NULL || m_sslConfig != NULL || m_msgClient != NULL)
        {
            return (false);
        }

        if (configInfo.msgc_enable_ssl && configInfo.msgc_ssl_cafile.size() > 0)
        {
            sslConfig = ProSslClientConfig_Create();
            if (sslConfig == NULL)
            {
                goto EXIT;
            }

            ProSslClientConfig_EnableSha1Cert(sslConfig, configInfo.msgc_ssl_enable_sha1cert);

            CProStlVector<const char*>      caFiles;
            CProStlVector<const char*>      crlFiles;
            CProStlVector<PRO_SSL_SUITE_ID> suites;

            int i = 0;
            int c = (int)configInfo.msgc_ssl_cafile.size();

            for (; i < c; ++i)
            {
                caFiles.push_back(&configInfo.msgc_ssl_cafile[i][0]);
            }

            i = 0;
            c = (int)configInfo.msgc_ssl_crlfile.size();

            for (; i < c; ++i)
            {
                crlFiles.push_back(&configInfo.msgc_ssl_crlfile[i][0]);
            }

            if (!ProSslClientConfig_SetCaList(
                sslConfig,
                &caFiles[0],
                caFiles.size(),
                crlFiles.size() > 0 ? &crlFiles[0] : NULL,
                crlFiles.size()
                ))
            {
                goto EXIT;
            }

            if (configInfo.msgc_ssl_aes256)
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

            if (!ProSslClientConfig_SetSuiteList(sslConfig, &suites[0], suites.size()))
            {
                goto EXIT;
            }
        }

        msgClient = CreateRtpMsgClient(
            this,
            reactor,
            RTP_MMT_MSG,
            sslConfig,
            configInfo.msgc_ssl_sni.c_str(),
            configInfo.msgc_server_ip.c_str(),
            configInfo.msgc_server_port,
            &configInfo.msgc_id,
            configInfo.msgc_password.c_str(),
            configInfo.msgc_local_ip.c_str(),
            configInfo.msgc_handshake_timeout
            );
        if (msgClient == NULL)
        {
            goto EXIT;
        }

        msgClient->SetOutputRedline(configInfo.msgc_redline_bytes);

        m_reactor    = reactor;
        m_configInfo = configInfo;
        m_sslConfig  = sslConfig;
        m_msgClient  = msgClient;
    }

    return (true);

EXIT:

    DeleteRtpMsgClient(msgClient);
    ProSslClientConfig_Delete(sslConfig);

    return (false);
}

void
CTest::Fini()
{
    PRO_SSL_CLIENT_CONFIG* sslConfig = NULL;
    IRtpMsgClient*         msgClient = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        msgClient = m_msgClient;
        m_msgClient = NULL;
        sslConfig = m_sslConfig;
        m_sslConfig = NULL;
        m_reactor = NULL;
    }

    DeleteRtpMsgClient(msgClient);
    ProSslClientConfig_Delete(sslConfig);
}

unsigned long
PRO_CALLTYPE
CTest::AddRef()
{
    const unsigned long refCount = CProRefCount::AddRef();

    return (refCount);
}

unsigned long
PRO_CALLTYPE
CTest::Release()
{
    const unsigned long refCount = CProRefCount::Release();

    return (refCount);
}

void
CTest::SendMsg(const char* msg)
{
    assert(msg != NULL);
    assert(msg[0] != '\0');
    if (msg == NULL || msg[0] == '\0')
    {
        return;
    }

#if defined(WIN32)
    const CProStlString msg2 = ProAnsiToUtf8(msg);
    msg = msg2.c_str();
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        RTP_MSG_USER srcUser;
        m_msgClient->GetUser(&srcUser);
        if (srcUser.classId == 0 || srcUser.UserId() == 0)
        {
            return;
        }

        PRO_INT64                   uid = 0;
        RTP_MSG_USER                dstUser;
        CProStlVector<RTP_MSG_USER> dstUsers;
        CProStlSet<RTP_MSG_USER>    users;

        uid = srcUser.UserId();

        dstUser.classId = srcUser.classId;
        dstUser.instId  = 1;

        /*
         * static users, [1 ~ 50]
         */
        for (int i = 1; i <= 50; ++i)
        {
            dstUser.UserId(i);
            users.insert(dstUser);
        }

        /*
         * neighbours, [-100 ~ +100]
         */
        for (int j = 1; j <= 100; ++j)
        {
            if (uid - j > 0)
            {
                dstUser.UserId(uid - j);
                users.insert(dstUser);
            }
            if (uid + j > 0)
            {
                dstUser.UserId(uid + j);
                users.insert(dstUser);
            }
        }

        users.erase(srcUser);

        CProStlSet<RTP_MSG_USER>::const_iterator       itr = users.begin();
        CProStlSet<RTP_MSG_USER>::const_iterator const end = users.end();

        for (; itr != end; ++itr)
        {
            dstUsers.push_back(*itr);
        }

        m_msgClient->SendMsg(msg, (unsigned long)strlen(msg),
            0, &dstUsers[0], (unsigned char)dstUsers.size());
    }
}

void
CTest::SendMsg(const char*         msg,
               const RTP_MSG_USER* dstUser) /* = NULL */
{
    assert(msg != NULL);
    assert(msg[0] != '\0');
    if (msg == NULL || msg[0] == '\0')
    {
        return;
    }

    if (dstUser == NULL || dstUser->classId == 0 || dstUser->UserId() == 0)
    {
        SendMsg(msg);

        return;
    }

#if defined(WIN32)
    const CProStlString msg2 = ProAnsiToUtf8(msg);
    msg = msg2.c_str();
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        RTP_MSG_USER srcUser;
        m_msgClient->GetUser(&srcUser);
        if (srcUser.classId == 0 || srcUser.UserId() == 0)
        {
            return;
        }

        m_msgClient->SendMsg(msg, (unsigned long)strlen(msg), 0, dstUser, 1);
    }
}

void
PRO_CALLTYPE
CTest::OnOkMsg(IRtpMsgClient*      msgClient,
               const RTP_MSG_USER* myUser,
               const char*         myPublicIp)
{
    assert(msgClient != NULL);
    assert(myUser != NULL);
    assert(myPublicIp != NULL);
    assert(myPublicIp[0] != '\0');
    if (msgClient == NULL || myUser == NULL || myPublicIp == NULL || myPublicIp[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }
    }

    {{{
        char suiteName[64] = "";
        msgClient->GetSslSuite(suiteName);

        printf(
            "\n"
            " CTest::OnOkMsg(id : %u-" PRO_PRT64U "-%u, publicIp : %s, sslSuite : %s,"
            " server : %s:%u) \n"
            ,
            (unsigned int)myUser->classId,
            myUser->UserId(),
            (unsigned int)myUser->instId,
            myPublicIp,
            suiteName,
            m_configInfo.msgc_server_ip.c_str(),
            (unsigned int)m_configInfo.msgc_server_port
            );
    }}}
}

void
PRO_CALLTYPE
CTest::OnRecvMsg(IRtpMsgClient*      msgClient,
                 const void*         buf,
                 unsigned long       size,
                 PRO_UINT16          charset,
                 const RTP_MSG_USER* srcUser)
{
    assert(msgClient != NULL);
    assert(buf != NULL);
    assert(size > 0);
    assert(srcUser != NULL);
    if (msgClient == NULL || buf == NULL || size == 0 || srcUser == NULL)
    {
        return;
    }

    CProStlString msg((char*)buf, size);
#if defined(WIN32)
    msg = ProUtf8ToAnsi(msg);
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }
    }

    {{{
        RTP_MSG_USER user;
        msgClient->GetUser(&user);

        printf(
            "\n"
            " CTest::OnRecvMsg(from : %u-" PRO_PRT64U "-%u, me : %u-" PRO_PRT64U "-%u) \n"
            "\t %s \n"
            ,
            (unsigned int)srcUser->classId,
            srcUser->UserId(),
            (unsigned int)srcUser->instId,
            (unsigned int)user.classId,
            user.UserId(),
            (unsigned int)user.instId,
            msg.c_str()
            );
    }}}
}

void
PRO_CALLTYPE
CTest::OnCloseMsg(IRtpMsgClient* msgClient,
                  long           errorCode,
                  long           sslCode,
                  bool           tcpConnected)
{
    assert(msgClient != NULL);
    if (msgClient == NULL)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        if (msgClient != m_msgClient)
        {
            return;
        }
    }

    {{{
        RTP_MSG_USER user;
        msgClient->GetUser(&user);

        printf(
            "\n"
            " CTest::OnCloseMsg(id : %u-" PRO_PRT64U "-%u,"
            " errorCode : [%d, %d], tcpConnected : %d, server : %s:%u) \n"
            ,
            (unsigned int)user.classId,
            user.UserId(),
            (unsigned int)user.instId,
            (int)errorCode,
            (int)sslCode,
            (int)tcpConnected,
            m_configInfo.msgc_server_ip.c_str(),
            (unsigned int)m_configInfo.msgc_server_port
            );
    }}}
}
