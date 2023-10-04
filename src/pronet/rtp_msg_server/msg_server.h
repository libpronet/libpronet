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

#ifndef MSG_SERVER_H
#define MSG_SERVER_H

#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_config_stream.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CDbConnection;
class CProLogFile;

struct MSG_SERVER_CONFIG_INFO
{
    MSG_SERVER_CONFIG_INFO()
    {
        msgs_thread_count        = 20;
        msgs_mm_type             = RTP_MMT_MSG;
        msgs_hub_port            = 3000;
        msgs_handshake_timeout   = 20;
        msgs_redline_bytes_c2s   = 8192000;
        msgs_redline_bytes_usr   = 1024000;
        msgs_db_readonly         = true;

        msgs_enable_ssl          = true;
        msgs_ssl_forced          = false;
        msgs_ssl_enable_sha1cert = true;
        msgs_ssl_keyfile         = "server.key";

        msgs_log_loop_bytes      = 50 * 1000 * 1000;
        msgs_log_level_green     = 0;

        msgs_ssl_cafiles.push_back("ca.crt");
        msgs_ssl_cafiles.push_back("");
        msgs_ssl_crlfiles.push_back("");
        msgs_ssl_crlfiles.push_back("");
        msgs_ssl_certfiles.push_back("server.crt");
        msgs_ssl_certfiles.push_back("");
    }

    void ToConfigs(CProStlVector<PRO_CONFIG_ITEM>& configs) const
    {
        CProConfigStream configStream;

        configStream.AddUint("msgs_thread_count"       , msgs_thread_count);
        configStream.AddUint("msgs_mm_type"            , msgs_mm_type);
        configStream.AddUint("msgs_hub_port"           , msgs_hub_port);
        configStream.AddUint("msgs_handshake_timeout"  , msgs_handshake_timeout);
        configStream.AddUint("msgs_redline_bytes_c2s"  , msgs_redline_bytes_c2s);
        configStream.AddUint("msgs_redline_bytes_usr"  , msgs_redline_bytes_usr);
        configStream.AddInt ("msgs_db_readonly"        , msgs_db_readonly);

        configStream.AddInt ("msgs_enable_ssl"         , msgs_enable_ssl);
        configStream.AddInt ("msgs_ssl_forced"         , msgs_ssl_forced);
        configStream.AddInt ("msgs_ssl_enable_sha1cert", msgs_ssl_enable_sha1cert);
        configStream.Add    ("msgs_ssl_cafile"         , msgs_ssl_cafiles);
        configStream.Add    ("msgs_ssl_crlfile"        , msgs_ssl_crlfiles);
        configStream.Add    ("msgs_ssl_certfile"       , msgs_ssl_certfiles);
        configStream.Add    ("msgs_ssl_keyfile"        , msgs_ssl_keyfile);

        configStream.AddUint("msgs_log_loop_bytes"     , msgs_log_loop_bytes);
        configStream.AddInt ("msgs_log_level_green"    , msgs_log_level_green);

        configStream.Get(configs);
    }

    unsigned int                 msgs_thread_count; /* 1 ~ 100 */
    RTP_MM_TYPE                  msgs_mm_type;      /* RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX */
    unsigned short               msgs_hub_port;
    unsigned int                 msgs_handshake_timeout;
    unsigned int                 msgs_redline_bytes_c2s;
    unsigned int                 msgs_redline_bytes_usr;
    bool                         msgs_db_readonly;

    bool                         msgs_enable_ssl;
    bool                         msgs_ssl_forced;
    bool                         msgs_ssl_enable_sha1cert;
    CProStlVector<CProStlString> msgs_ssl_cafiles;
    CProStlVector<CProStlString> msgs_ssl_crlfiles;
    CProStlVector<CProStlString> msgs_ssl_certfiles;
    CProStlString                msgs_ssl_keyfile;

    unsigned int                 msgs_log_loop_bytes;
    int                          msgs_log_level_green;

    DECLARE_SGI_POOL(0)
};

struct MSG_USER_CTX
{
    CProStlSet<uint16_t> iids;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CMsgServer : public IRtpMsgServerObserver, public CProRefCount
{
public:

    static CMsgServer* CreateInstance(
        CProLogFile&   logFile,
        CDbConnection& db
        );

    bool Init(
        IProReactor*                  reactor,
        const MSG_SERVER_CONFIG_INFO& configInfo
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    void KickoutUsers(const CProStlSet<RTP_MSG_USER>& users);

    void Reconfig(const MSG_SERVER_CONFIG_INFO& configInfo);

private:

    CMsgServer(
        CProLogFile&   logFile,
        CDbConnection& db
        );

    virtual ~CMsgServer();

    virtual bool OnCheckUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        const char*         userPublicIp,
        const RTP_MSG_USER* c2sUser, /* = NULL */
        const unsigned char hash[32],
        const unsigned char nonce[32],
        uint64_t*           userId,
        uint16_t*           instId,
        int64_t*            appData,
        bool*               isC2s
        );

    virtual void OnOkUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        const char*         userPublicIp,
        const RTP_MSG_USER* c2sUser, /* = NULL */
        int64_t             appData
        );

    virtual void OnCloseUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        int                 errorCode,
        int                 sslCode
        );

    virtual void OnHeartbeatUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        int64_t             peerAliveTick
        )
    {
    }

    virtual void OnRecvMsg(
        IRtpMsgServer*      msgServer,
        const void*         buf,
        size_t              size,
        uint16_t            charset,
        const RTP_MSG_USER* srcUser
        )
    {
    }

private:

    CProLogFile&                       m_logFile;
    CDbConnection&                     m_db;
    IProReactor*                       m_reactor;
    MSG_SERVER_CONFIG_INFO             m_configInfo;
    PRO_SSL_SERVER_CONFIG*             m_sslConfig;
    IRtpMsgServer*                     m_msgServer;

    CProStlMap<uint64_t, MSG_USER_CTX> m_uid2Ctx[256]; /* cid0 ~ cid255 */

    CProThreadMutex                    m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* MSG_SERVER_H */
