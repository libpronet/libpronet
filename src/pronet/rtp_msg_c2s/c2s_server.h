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

#if !defined(C2S_SERVER_H)
#define C2S_SERVER_H

#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProLogFile;

struct C2S_SERVER_CONFIG_INFO
{
    C2S_SERVER_CONFIG_INFO()
    {
        c2ss_thread_count               = 20;
        c2ss_mm_type                    = RTP_MMT_MSG;
        c2ss_uplink_ip                  = "a.b.c.d";
        c2ss_uplink_port                = 3000;
        c2ss_uplink_password            = "test";
        c2ss_uplink_local_ip            = "0.0.0.0";
        c2ss_uplink_timeout             = 20;
        c2ss_uplink_redline_bytes       = 8192000;
        c2ss_local_hub_port             = 3000;
        c2ss_local_timeout              = 20;
        c2ss_local_redline_bytes        = 1024000;

        c2ss_ssl_uplink                 = false;
        c2ss_ssl_local                  = true;
        c2ss_ssl_local_forced           = false;
        c2ss_ssl_uplink_enable_sha1cert = true;
        c2ss_ssl_uplink_sni             = "server.libpro.org";
        c2ss_ssl_uplink_aes256          = false;
        c2ss_ssl_local_enable_sha1cert  = true;
        c2ss_ssl_local_keyfile          = "./server.key";

        c2ss_log_loop_bytes             = 50 * 1000 * 1000;
        c2ss_log_level_green            = 0;

        RtpMsgString2User("1-10000001-1", &c2ss_uplink_id);

        c2ss_ssl_uplink_cafiles.push_back("./ca.crt");
        c2ss_ssl_uplink_cafiles.push_back("");
        c2ss_ssl_uplink_crlfiles.push_back("");
        c2ss_ssl_uplink_crlfiles.push_back("");
        c2ss_ssl_local_cafiles.push_back("./ca.crt");
        c2ss_ssl_local_cafiles.push_back("");
        c2ss_ssl_local_crlfiles.push_back("");
        c2ss_ssl_local_crlfiles.push_back("");
        c2ss_ssl_local_certfiles.push_back("./server.crt");
        c2ss_ssl_local_certfiles.push_back("");
    }

    ~C2S_SERVER_CONFIG_INFO()
    {
        if (!c2ss_uplink_password.empty())
        {
            ProZeroMemory(
                &c2ss_uplink_password[0], c2ss_uplink_password.length());
            c2ss_uplink_password = "";
        }
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        char idString[64] = "";
        RtpMsgUser2String(&c2ss_uplink_id, idString);

        CProConfigStream configStream;

        configStream.AddUint("c2ss_thread_count"              , c2ss_thread_count);
        configStream.AddUint("c2ss_mm_type"                   , c2ss_mm_type);
        configStream.Add    ("c2ss_uplink_ip"                 , c2ss_uplink_ip);
        configStream.AddUint("c2ss_uplink_port"               , c2ss_uplink_port);
        configStream.Add    ("c2ss_uplink_id"                 , idString);
        configStream.Add    ("c2ss_uplink_password"           , c2ss_uplink_password);
        configStream.Add    ("c2ss_uplink_local_ip"           , c2ss_uplink_local_ip);
        configStream.AddUint("c2ss_uplink_timeout"            , c2ss_uplink_timeout);
        configStream.AddUint("c2ss_uplink_redline_bytes"      , c2ss_uplink_redline_bytes);
        configStream.AddUint("c2ss_local_hub_port"            , c2ss_local_hub_port);
        configStream.AddUint("c2ss_local_timeout"             , c2ss_local_timeout);
        configStream.AddUint("c2ss_local_redline_bytes"       , c2ss_local_redline_bytes);

        configStream.AddInt ("c2ss_ssl_uplink"                , c2ss_ssl_uplink);
        configStream.AddInt ("c2ss_ssl_local"                 , c2ss_ssl_local);
        configStream.AddInt ("c2ss_ssl_local_forced"          , c2ss_ssl_local_forced);
        configStream.AddInt ("c2ss_ssl_uplink_enable_sha1cert", c2ss_ssl_uplink_enable_sha1cert);
        configStream.Add    ("c2ss_ssl_uplink_cafile"         , c2ss_ssl_uplink_cafiles);
        configStream.Add    ("c2ss_ssl_uplink_crlfile"        , c2ss_ssl_uplink_crlfiles);
        configStream.Add    ("c2ss_ssl_uplink_sni"            , c2ss_ssl_uplink_sni);
        configStream.AddInt ("c2ss_ssl_uplink_aes256"         , c2ss_ssl_uplink_aes256);
        configStream.AddInt ("c2ss_ssl_local_enable_sha1cert" , c2ss_ssl_local_enable_sha1cert);
        configStream.Add    ("c2ss_ssl_local_cafile"          , c2ss_ssl_local_cafiles);
        configStream.Add    ("c2ss_ssl_local_crlfile"         , c2ss_ssl_local_crlfiles);
        configStream.Add    ("c2ss_ssl_local_certfile"        , c2ss_ssl_local_certfiles);
        configStream.Add    ("c2ss_ssl_local_keyfile"         , c2ss_ssl_local_keyfile);

        configStream.AddUint("c2ss_log_loop_bytes"            , c2ss_log_loop_bytes);
        configStream.AddInt ("c2ss_log_level_green"           , c2ss_log_level_green);

        configStream.Get(configs);
    }

    unsigned int                 c2ss_thread_count; /* 1 ~ 100 */
    RTP_MM_TYPE                  c2ss_mm_type;      /* RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX */
    CProStlString                c2ss_uplink_ip;
    unsigned short               c2ss_uplink_port;
    RTP_MSG_USER                 c2ss_uplink_id;
    CProStlString                c2ss_uplink_password;
    CProStlString                c2ss_uplink_local_ip;
    unsigned int                 c2ss_uplink_timeout;
    unsigned int                 c2ss_uplink_redline_bytes;
    unsigned short               c2ss_local_hub_port;
    unsigned int                 c2ss_local_timeout;
    unsigned int                 c2ss_local_redline_bytes;

    bool                         c2ss_ssl_uplink;
    bool                         c2ss_ssl_local;
    bool                         c2ss_ssl_local_forced;
    bool                         c2ss_ssl_uplink_enable_sha1cert;
    CProStlVector<CProStlString> c2ss_ssl_uplink_cafiles;
    CProStlVector<CProStlString> c2ss_ssl_uplink_crlfiles;
    CProStlString                c2ss_ssl_uplink_sni;
    bool                         c2ss_ssl_uplink_aes256;
    bool                         c2ss_ssl_local_enable_sha1cert;
    CProStlVector<CProStlString> c2ss_ssl_local_cafiles;
    CProStlVector<CProStlString> c2ss_ssl_local_crlfiles;
    CProStlVector<CProStlString> c2ss_ssl_local_certfiles;
    CProStlString                c2ss_ssl_local_keyfile;

    unsigned int                 c2ss_log_loop_bytes;
    int                          c2ss_log_level_green;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CC2sServer : public IRtpMsgC2sObserver, public CProRefCount
{
public:

    static CC2sServer* CreateInstance(CProLogFile& logFile);

    bool Init(
        IProReactor*                  reactor,
        const C2S_SERVER_CONFIG_INFO& configInfo
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    void Reconfig(const C2S_SERVER_CONFIG_INFO& configInfo);

private:

    CC2sServer(CProLogFile& logFile);

    virtual ~CC2sServer();

    virtual void PRO_CALLTYPE OnOkC2s(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        );

    virtual void PRO_CALLTYPE OnCloseC2s(
        IRtpMsgC2s* msgC2s,
        long        errorCode,
        long        sslCode,
        bool        tcpConnected
        );

    virtual void PRO_CALLTYPE OnHeartbeatC2s(
        IRtpMsgC2s* msgC2s,
        PRO_INT64   peerAliveTick
        )
    {
    }

    virtual void PRO_CALLTYPE OnOkUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        const char*         userPublicIp
        );

    virtual void PRO_CALLTYPE OnCloseUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        long                errorCode,
        long                sslCode
        );

    virtual void PRO_CALLTYPE OnHeartbeatUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        PRO_INT64           peerAliveTick
        )
    {
    }

private:

    CProLogFile&           m_logFile;
    IProReactor*           m_reactor;
    C2S_SERVER_CONFIG_INFO m_configInfo;
    PRO_SSL_CLIENT_CONFIG* m_uplinkSslConfig;
    PRO_SSL_SERVER_CONFIG* m_localSslConfig;
    IRtpMsgC2s*            m_msgC2s;
    PRO_INT64              m_onOkTick;
    CProThreadMutex        m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* C2S_SERVER_H */
