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
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_unicode.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

static const RTP_MSG_USER ROOT_ID(1, 1, 0); /* 1-1 */

/////////////////////////////////////////////////////////////////////////////
////

CTest*
CTest::CreateInstance()
{
    return new CTest;
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
        return false;
    }

    char serverIpByDNS[64] = "";

    /*
     * DNS, for reconnecting
     */
    {
        uint32_t serverIp = pbsd_inet_aton(configInfo.msgc_server_ip.c_str());
        if (serverIp == (uint32_t)-1 || serverIp == 0)
        {
            return false;
        }

        pbsd_inet_ntoa(serverIp, serverIpByDNS);
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
            return false;
        }

        if (configInfo.msgc_enable_ssl)
        {
            CProStlVector<const char*>      caFiles;
            CProStlVector<const char*>      crlFiles;
            CProStlVector<PRO_SSL_SUITE_ID> suites;

            int i = 0;
            int c = (int)configInfo.msgc_ssl_cafiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.msgc_ssl_cafiles[i].empty())
                {
                    caFiles.push_back(configInfo.msgc_ssl_cafiles[i].c_str());
                }
            }

            i = 0;
            c = (int)configInfo.msgc_ssl_crlfiles.size();

            for (; i < c; ++i)
            {
                if (!configInfo.msgc_ssl_crlfiles[i].empty())
                {
                    crlFiles.push_back(configInfo.msgc_ssl_crlfiles[i].c_str());
                }
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

            if (caFiles.size() > 0)
            {
                sslConfig = ProSslClientConfig_Create();
                if (sslConfig == NULL)
                {
                    goto EXIT;
                }

                ProSslClientConfig_EnableSha1Cert(sslConfig, configInfo.msgc_ssl_enable_sha1cert);

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

                if (!ProSslClientConfig_SetSuiteList(sslConfig, &suites[0], suites.size()))
                {
                    goto EXIT;
                }
            }
        }

        msgClient = CreateRtpMsgClient(
            this,
            reactor,
            configInfo.msgc_mm_type,
            sslConfig,
            configInfo.msgc_ssl_sni.c_str(),
            serverIpByDNS,
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

        m_reactor                   = reactor;
        m_configInfo                = configInfo;
        m_configInfo.msgc_server_ip = serverIpByDNS;
        m_sslConfig                 = sslConfig;
        m_msgClient                 = msgClient;
    }

    return true;

EXIT:

    DeleteRtpMsgClient(msgClient);
    ProSslClientConfig_Delete(sslConfig);

    return false;
}

void
CTest::Fini()
{
    PRO_SSL_CLIENT_CONFIG* sslConfig = NULL;
    IRtpMsgClient*         msgClient = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
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
CTest::AddRef()
{
    return CProRefCount::AddRef();
}

unsigned long
CTest::Release()
{
    return CProRefCount::Release();
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

#if defined(_WIN32)
    CProStlString tmp;
    ProAnsiToUtf8(msg, tmp);
    msg = tmp.c_str();
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        RTP_MSG_USER myUser;
        m_msgClient->GetUser(&myUser);
        if (myUser.classId == 0 || myUser.UserId() == 0)
        {
            return;
        }

        CProStlSet<RTP_MSG_USER>    users;
        CProStlVector<RTP_MSG_USER> dstUsers;

        /*
         * root
         */
        users.insert(ROOT_ID);

        /*
         * static users, [1 ~ 50]
         */
        {
            RTP_MSG_USER dstUser;
            dstUser.classId = myUser.classId;

            for (int i = 1; i <= 50; ++i)
            {
                dstUser.UserId(i);
                users.insert(dstUser);
            }
        }

        /*
         * neighbours, [u-100 ~ u+100]
         */
        {
            RTP_MSG_USER dstUser;
            dstUser.classId = myUser.classId;

            int64_t myUid = myUser.UserId();

            for (int i = 1; i <= 100; ++i)
            {
                if (myUid - i > 0)
                {
                    dstUser.UserId(myUid - i);
                    users.insert(dstUser);
                }
                if (myUid + i > 0)
                {
                    dstUser.UserId(myUid + i);
                    users.insert(dstUser);
                }
            }
        }

        /*
         * exclude me
         */
        users.erase(myUser);

        auto itr = users.begin();
        auto end = users.end();

        for (; itr != end; ++itr)
        {
            dstUsers.push_back(*itr);
        }

        m_msgClient->SendMsg(msg, strlen(msg), 0, &dstUsers[0], (unsigned char)dstUsers.size());
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

#if defined(_WIN32)
    CProStlString tmp;
    ProAnsiToUtf8(msg, tmp);
    msg = tmp.c_str();
#endif

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL || m_msgClient == NULL)
        {
            return;
        }

        RTP_MSG_USER myUser;
        m_msgClient->GetUser(&myUser);
        if (myUser.classId == 0 || myUser.UserId() == 0)
        {
            return;
        }

        m_msgClient->SendMsg(msg, strlen(msg), 0, dstUser, 1);
    }
}

void
CTest::Reconnect()
{
    IRtpMsgClient* oldMsgClient = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_reactor == NULL)
        {
            return;
        }

        IRtpMsgClient* msgClient = CreateRtpMsgClient(
            this,
            m_reactor,
            m_configInfo.msgc_mm_type,
            m_sslConfig,
            m_configInfo.msgc_ssl_sni.c_str(),
            m_configInfo.msgc_server_ip.c_str(),
            m_configInfo.msgc_server_port,
            &m_configInfo.msgc_id,
            m_configInfo.msgc_password.c_str(),
            m_configInfo.msgc_local_ip.c_str(),
            m_configInfo.msgc_handshake_timeout
            );
        if (msgClient == NULL)
        {
            return;
        }

        msgClient->SetOutputRedline(m_configInfo.msgc_redline_bytes);

        oldMsgClient = m_msgClient;
        m_msgClient  = msgClient;
    }

    DeleteRtpMsgClient(oldMsgClient);
}

void
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

        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnOkMsg(id : %u-%llu-%u, publicIp : %s,"
            " sslSuite : %s, server : %s:%u) \n"
            ,
            timeString.c_str(),
            (unsigned int)myUser->classId,
            (unsigned long long)myUser->UserId(),
            (unsigned int)myUser->instId,
            myPublicIp,
            suiteName,
            m_configInfo.msgc_server_ip.c_str(),
            (unsigned int)m_configInfo.msgc_server_port
            );
    }}}
}

void
CTest::OnRecvMsg(IRtpMsgClient*      msgClient,
                 const void*         buf,
                 size_t              size,
                 uint16_t            charset,
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

    CProStlString msg((char*)buf, size);
#if defined(_WIN32)
    CProStlString tmp;
    ProUtf8ToAnsi(msg, tmp);
    msg = tmp;
#endif

    {{{
        RTP_MSG_USER myUser;
        msgClient->GetUser(&myUser);

        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnRecvMsg(from : %u-%llu-%u, me : %u-%llu-%u) \n"
            "\t %s \n"
            ,
            timeString.c_str(),
            (unsigned int)srcUser->classId,
            (unsigned long long)srcUser->UserId(),
            (unsigned int)srcUser->instId,
            (unsigned int)myUser.classId,
            (unsigned long long)myUser.UserId(),
            (unsigned int)myUser.instId,
            msg.c_str()
            );
    }}}
}

void
CTest::OnCloseMsg(IRtpMsgClient* msgClient,
                  int            errorCode,
                  int            sslCode,
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
        RTP_MSG_USER myUser;
        msgClient->GetUser(&myUser);

        CProStlString timeString;
        ProGetLocalTimeString(timeString);

        printf(
            "\n"
            "%s \n"
            " CTest::OnCloseMsg(id : %u-%llu-%u,"
            " errorCode : [%d, %d], tcpConnected : %d, server : %s:%u) \n"
            ,
            timeString.c_str(),
            (unsigned int)myUser.classId,
            (unsigned long long)myUser.UserId(),
            (unsigned int)myUser.instId,
            errorCode,
            sslCode,
            (int)tcpConnected,
            m_configInfo.msgc_server_ip.c_str(),
            (unsigned int)m_configInfo.msgc_server_port
            );
    }}}
}
