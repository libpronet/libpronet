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

#if !defined(TEST_H)
#define TEST_H

#include "../pro_rtp/rtp_foundation.h"
#include "../pro_rtp/rtp_framework.h"
#include "../pro_util/pro_config_file.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

struct MSG_CLIENT_CONFIG_INFO
{
    MSG_CLIENT_CONFIG_INFO()
    {
        msgc_server_ip           = "127.0.0.1";
        msgc_server_port         = 3000;
        msgc_password            = "test";
        msgc_local_ip            = "0.0.0.0";
        msgc_handshake_timeout   = 20;
        msgc_redline_bytes       = 1024000;

        msgc_enable_ssl          = false;
        msgc_ssl_enable_sha1cert = true;
        msgc_ssl_sni             = "server.libpro.org";
        msgc_ssl_aes256          = false;

        RtpMsgString2User("2-0-0", &msgc_id);

        msgc_ssl_cafile.push_back("./ca.crt");
        msgc_ssl_cafile.push_back("");
        msgc_ssl_crlfile.push_back("");
        msgc_ssl_crlfile.push_back("");
    }

    ~MSG_CLIENT_CONFIG_INFO()
    {
        if (!msgc_password.empty())
        {
            ProZeroMemory(&msgc_password[0], msgc_password.length());
            msgc_password = "";
        }
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        char idString[64] = "";
        RtpMsgUser2String(&msgc_id, idString);

        CProConfigStream configStream;

        configStream.Add    ("msgc_server_ip"          , msgc_server_ip);
        configStream.AddUint("msgc_server_port"        , msgc_server_port);
        configStream.Add    ("msgc_id"                 , idString);
        configStream.Add    ("msgc_password"           , msgc_password);
        configStream.Add    ("msgc_local_ip"           , msgc_local_ip);
        configStream.AddUint("msgc_handshake_timeout"  , msgc_handshake_timeout);
        configStream.AddUint("msgc_redline_bytes"      , msgc_redline_bytes);

        configStream.AddInt ("msgc_enable_ssl"         , msgc_enable_ssl);
        configStream.AddInt ("msgc_ssl_enable_sha1cert", msgc_ssl_enable_sha1cert);
        configStream.Add    ("msgc_ssl_cafile"         , msgc_ssl_cafile);
        configStream.Add    ("msgc_ssl_crlfile"        , msgc_ssl_crlfile);
        configStream.Add    ("msgc_ssl_sni"            , msgc_ssl_sni);
        configStream.AddInt ("msgc_ssl_aes256"         , msgc_ssl_aes256);

        configStream.Get(configs);
    }

    CProStlString                msgc_server_ip;
    unsigned short               msgc_server_port;
    RTP_MSG_USER                 msgc_id;
    CProStlString                msgc_password;
    CProStlString                msgc_local_ip;
    unsigned int                 msgc_handshake_timeout;
    unsigned int                 msgc_redline_bytes;

    bool                         msgc_enable_ssl;
    bool                         msgc_ssl_enable_sha1cert;
    CProStlVector<CProStlString> msgc_ssl_cafile;
    CProStlVector<CProStlString> msgc_ssl_crlfile;
    CProStlString                msgc_ssl_sni;
    bool                         msgc_ssl_aes256;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CTest : public IRtpMsgClientObserver, public CProRefCount
{
public:

    static CTest* CreateInstance();

    bool Init(
        IProReactor*                  reactor,
        const MSG_CLIENT_CONFIG_INFO& configInfo
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    void SendMsg(const char* msg);

    void SendMsg(
        const char*         msg,
        const RTP_MSG_USER* dstUser /* = NULL */
        );

private:

    CTest();

    virtual ~CTest();

    virtual void PRO_CALLTYPE OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        );

    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        );

    virtual void PRO_CALLTYPE OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,
        long           sslCode,
        bool           tcpConnected
        );

private:

    IProReactor*           m_reactor;
    MSG_CLIENT_CONFIG_INFO m_configInfo;
    PRO_SSL_CLIENT_CONFIG* m_sslConfig;
    IRtpMsgClient*         m_msgClient;
    CProThreadMutex        m_lock;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* TEST_H */
