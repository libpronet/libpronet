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

/*         ______________________________________________________
 *        |                                                      |
 *        |                      Msg server                      |
 *        |______________________________________________________|
 *                 |             |        |              |
 *                 | ...         |        |          ... |
 *         ________|_______      |        |      ________|_______
 *        |                |     |        |     |                |
 *        |     Msg C2S    |     |        |     |     Msg C2S    |
 *        |________________|     |        |     |________________|
 *            |        |         |        |         |        |
 *            |  ...   |         |  ...   |         |  ...   |
 *         ___|___  ___|___   ___|___  ___|___   ___|___  ___|___
 *        |  Msg  ||  Msg  | |  Msg  ||  Msg  | |  Msg  ||  Msg  |
 *        | client|| client| | client|| client| | client|| client|
 *        |_______||_______| |_______||_______| |_______||_______|
 *                 Fig.1 Structure diagram of msg system
 */

/*
 * 1)  client ----->                 connect()                  -----> server
 * 2)  client <-----                  accept()                  <----- server
 * 3a) client <-----                 PRO_NONCE                  <----- server
 * 3b) client ----->    serviceId + serviceOpt + (r) + (r+1)    -----> server
 * 4]  client <<====              [SSL handshake]               ====>> server
 *     client::passwordHash
 * 5)  client -----> RTP(RTP_SESSION_INFO with RTP_MSG_HEADER0) -----> server
 *                                                       passwordHash::server
 * 6)  client <----- RTP(RTP_SESSION_ACK with RTP_MSG_HEADER0)  <----- server
 * 7)  client <<====        TCP4(RTP_MSG_HEADER + data)         ====>> server
 *                 Fig.2 Msg system handshake protocol flow chart
 */

/*
 * PMP-v2.0 (PRO Messaging Protocol version 2.0)
 */

#ifndef ____RTP_MSG_H____
#define ____RTP_MSG_H____

#include "rtp_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Message user ID. Format: 1-2-*, 1-3-*, ...; 2-1-*, 2-2-*, 2-3-*, ...; ...
 *
 * classId : 8 bits. Identifies user category for application management.
 * (cid)     0=Invalid, 1=Application server node, 2~254=Application client node, 255=C2S node
 *
 * userId  : 40 bits. Identifies user ID (e.g. phone number), allocated/permitted by msg server.
 * (uid)     0=Dynamically allocated, valid range [0xF000000000 ~ 0xFFFFFFFFFF];
 *           Otherwise statically allocated, valid range [1 ~ 0xEFFFFFFFFF]
 *
 * instId  : 16 bits. Identifies user instance ID (e.g. extension number),
 * (iid)     allocated/permitted by msg server. Valid range [0 ~ 65535]
 *
 * Note:
 * 1) cid-uid-iid 1-1-* are reserved for identifying the message server itself (ROOT)
 * 2) cid-uid-iid 255-*-* are reserved for identifying message C2S nodes
 * 3) cid-uid-* share the same password
 */
struct RTP_MSG_USER
{
    RTP_MSG_USER()
    {
        Zero();
    }

    RTP_MSG_USER(
        unsigned char __classId,
        uint64_t      __userId,
        uint16_t      __instId
        )
    {
        classId = __classId;
        UserId(__userId);
        instId  = __instId;
    }

    void UserId(uint64_t userId)
    {
        userId5 =  (unsigned char)userId;
        userId >>= 8;
        userId4 =  (unsigned char)userId;
        userId >>= 8;
        userId3 =  (unsigned char)userId;
        userId >>= 8;
        userId2 =  (unsigned char)userId;
        userId >>= 8;
        userId1 =  (unsigned char)userId;
    }

    /*
     * userId = userId1.userId2.userId3.userId4.userId5
     */
    uint64_t UserId() const
    {
        uint64_t userId = userId1;
        userId <<= 8;
        userId |=  userId2;
        userId <<= 8;
        userId |=  userId3;
        userId <<= 8;
        userId |=  userId4;
        userId <<= 8;
        userId |=  userId5;

        return userId;
    }

    void Zero()
    {
        classId = 0;
        userId1 = 0;
        userId2 = 0;
        userId3 = 0;
        userId4 = 0;
        userId5 = 0;
        instId  = 0;
    }

    bool IsRoot() const
    {
        return classId == 1 && UserId() == 1;
    }

    bool operator==(const RTP_MSG_USER& other) const
    {
        return classId == other.classId &&
               userId1 == other.userId1 &&
               userId2 == other.userId2 &&
               userId3 == other.userId3 &&
               userId4 == other.userId4 &&
               userId5 == other.userId5 &&
               instId  == other.instId;
    }

    bool operator!=(const RTP_MSG_USER& other) const
    {
        return !(*this == other);
    }

    bool operator<(const RTP_MSG_USER& other) const
    {
        if (classId < other.classId)
        {
            return true;
        }
        if (classId > other.classId)
        {
            return false;
        }

        if (userId1 < other.userId1)
        {
            return true;
        }
        if (userId1 > other.userId1)
        {
            return false;
        }

        if (userId2 < other.userId2)
        {
            return true;
        }
        if (userId2 > other.userId2)
        {
            return false;
        }

        if (userId3 < other.userId3)
        {
            return true;
        }
        if (userId3 > other.userId3)
        {
            return false;
        }

        if (userId4 < other.userId4)
        {
            return true;
        }
        if (userId4 > other.userId4)
        {
            return false;
        }

        if (userId5 < other.userId5)
        {
            return true;
        }
        if (userId5 > other.userId5)
        {
            return false;
        }

        return instId < other.instId;
    }

    bool operator>(const RTP_MSG_USER& other) const
    {
        return !(*this < other || *this == other);
    }

    unsigned char classId;
    unsigned char userId1;
    unsigned char userId2;
    unsigned char userId3;
    unsigned char userId4;
    unsigned char userId5;
    uint16_t      instId;
};

/*
 * Message handshake header
 */
struct RTP_MSG_HEADER0
{
    uint16_t     version; /* The current protocol version is 02 */
    RTP_MSG_USER user;
    char         reserved1[2];
    union
    {
        char     reserved2[24];
        uint32_t publicIp;
    };
};

/*
 * Message data header
 */
struct RTP_MSG_HEADER
{
    uint16_t       charset;      /* ANSI, UTF-8, ... */
    RTP_MSG_USER   srcUser;
    char           reserved;
    unsigned char  dstUserCount; /* Up to 255 users */
    RTP_MSG_USER   dstUsers[1];  /* Variable-length array */
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Message client
 */
class IRtpMsgClient
{
public:

    virtual ~IRtpMsgClient() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Get media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * Get user information
     */
    virtual void GetUser(RTP_MSG_USER* myUser) const = 0;

    /*
     * Get SSL cipher suite
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const = 0;

    /*
     * Get local IP address
     */
    virtual const char* GetLocalIp(char localIp[64]) const = 0;

    /*
     * Get local port number
     */
    virtual unsigned short GetLocalPort() const = 0;

    /*
     * Get remote IP address
     */
    virtual const char* GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * Get remote port number
     */
    virtual unsigned short GetRemotePort() const = 0;

    /*
     * Send message
     *
     * The system maintains an internal message queue
     */
    virtual bool SendMsg(
        const void*         buf,         /* Message content */
        size_t              size,        /* Message length */
        uint16_t            charset,     /* User-defined charset code */
        const RTP_MSG_USER* dstUsers,    /* Message recipients */
        unsigned char       dstUserCount /* Max 255 recipients */
        ) = 0;

    /*
     * Send message (buf1 + buf2)
     *
     * The system maintains an internal message queue
     */
    virtual bool SendMsg2(
        const void*         buf1,        /* Message content 1 */
        size_t              size1,       /* Message length 1 */
        const void*         buf2,        /* Message content 2 (can be NULL) */
        size_t              size2,       /* Message length 2 (can be 0) */
        uint16_t            charset,     /* User-defined message charset code */
        const RTP_MSG_USER* dstUsers,    /* Message recipients */
        unsigned char       dstUserCount /* Max 255 recipients */
        ) = 0;

    /*
     * Set output redline. Default (1024 * 1024) bytes
     *
     * Do nothing if redlineBytes is 0
     */
    virtual void SetOutputRedline(size_t redlineBytes) = 0;

    /*
     * Get output redline. Default (1024 * 1024) bytes
     */
    virtual size_t GetOutputRedline() const = 0;

    /*
     * Get unsent bytes in the queue
     */
    virtual size_t GetSendingBytes() const = 0;
};

/*
 * Message client callback target
 *
 * Users need implement this interface
 */
class IRtpMsgClientObserver
{
public:

    virtual ~IRtpMsgClientObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when login is successful
     */
    virtual void OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * Called when a message arrives
     */
    virtual void OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        size_t              size,
        uint16_t            charset,
        const RTP_MSG_USER* srcUser
        ) = 0;

    /*
     * Called on network error or timeout
     */
    virtual void OnCloseMsg(
        IRtpMsgClient* msgClient,
        int            errorCode,   /* System error code */
        int            sslCode,     /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        bool           tcpConnected /* Whether TCP connection is established */
        ) = 0;

    /*
     * Called on heartbeat occurrence
     *
     * Mainly for debugging
     */
    virtual void OnHeartbeatMsg(
        IRtpMsgClient* msgClient,
        int64_t        peerAliveTick
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Message server
 */
class IRtpMsgServer
{
public:

    virtual ~IRtpMsgServer() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Get media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * Get service port number
     */
    virtual unsigned short GetServicePort() const = 0;

    /*
     * Get SSL cipher suite
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const = 0;

    /*
     * Get user count statistics
     */
    virtual void GetUserCount(
        size_t* pendingUserCount, /* = NULL */
        size_t* baseUserCount,    /* = NULL */
        size_t* subUserCount      /* = NULL */
        ) const = 0;

    /*
     * Kick out a user
     */
    virtual void KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * Send message
     *
     * The system maintains an internal message queue
     */
    virtual bool SendMsg(
        const void*         buf,         /* Message content */
        size_t              size,        /* Message length */
        uint16_t            charset,     /* User-defined message charset code */
        const RTP_MSG_USER* dstUsers,    /* Message recipients */
        unsigned char       dstUserCount /* Max 255 recipients */
        ) = 0;

    /*
     * Send message (buf1 + buf2)
     *
     * The system maintains an internal message queue
     */
    virtual bool SendMsg2(
        const void*         buf1,        /* Message content 1 */
        size_t              size1,       /* Message length 1 */
        const void*         buf2,        /* Message content 2 (can be NULL) */
        size_t              size2,       /* Message length 2 (can be 0) */
        uint16_t            charset,     /* User-defined message charset code */
        const RTP_MSG_USER* dstUsers,    /* Message recipients */
        unsigned char       dstUserCount /* Max 255 recipients */
        ) = 0;

    /*
     * Set output redline for server->c2s link. Default (1024 * 1024 * 8) bytes
     *
     * Do nothing if redlineBytes is 0
     */
    virtual void SetOutputRedlineToC2s(size_t redlineBytes) = 0;

    /*
     * Get output redline for server->c2s link. Default (1024 * 1024 * 8) bytes
     */
    virtual size_t GetOutputRedlineToC2s() const = 0;

    /*
     * Set output redline for server->user link. Default (1024 * 1024) bytes
     *
     * Do nothing if redlineBytes is 0
     */
    virtual void SetOutputRedlineToUsr(size_t redlineBytes) = 0;

    /*
     * Get output redline for server->user link. Default (1024 * 1024) bytes
     */
    virtual size_t GetOutputRedlineToUsr() const = 0;

    /*
     * Get unsent bytes in the queue for the user
     */
    virtual size_t GetSendingBytes(const RTP_MSG_USER* user) const = 0;
};

/*
 * Message server callback target
 *
 * Users need implement this interface
 */
class IRtpMsgServerObserver
{
public:

    virtual ~IRtpMsgServerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a user requests login
     *
     * The implementation should find matching password based on user info,
     * then call CheckRtpServiceData() for verification
     *
     * Return value indicates whether to allow login
     */
    virtual bool OnCheckUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* Login user info from client */
        const char*         userPublicIp, /* User's IP address */
        const RTP_MSG_USER* c2sUser,      /* Via which C2S (can be NULL) */
        const unsigned char hash[32],     /* Password hash from client */
        const unsigned char nonce[32],    /* Session nonce. Used for CheckRtpServiceData()
                                             to verify password hash */
        uint64_t*           userId,       /* UID allocated/permitted by upper layer */
        uint16_t*           instId,       /* IID allocated/permitted by upper layer */
        int64_t*            appData,      /* Application-specific data by upper layer.
                                             Returned in subsequent OnOkUser() */
        bool*               isC2s         /* Whether this node is a C2S, set by upper layer */
        ) = 0;

    /*
     * Called when user login is successful
     */
    virtual void OnOkUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* User info allocated/permitted by upper layer */
        const char*         userPublicIp, /* User's IP address */
        const RTP_MSG_USER* c2sUser,      /* Via which C2S (can be NULL) */
        int64_t             appData       /* Application-specific data from OnCheckUser() */
        ) = 0;

    /*
     * Called on network error or timeout
     */
    virtual void OnCloseUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        int                 errorCode, /* System error code */
        int                 sslCode    /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * Called on user heartbeat occurrence
     *
     * Mainly for debugging
     */
    virtual void OnHeartbeatUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        int64_t             peerAliveTick
        ) = 0;

    /*
     * Called when a message arrives
     */
    virtual void OnRecvMsg(
        IRtpMsgServer*      msgServer,
        const void*         buf,
        size_t              size,
        uint16_t            charset,
        const RTP_MSG_USER* srcUser
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Message C2S (Client-to-Server proxy)
 *
 * C2S sits between client and server. It can distribute server load and hide server location.
 * To the client, C2S is transparent - the client cannot distinguish whether it's connected to
 * the server or a C2S
 *
 * Currently, multi-level C2S cascading is not supported
 */
class IRtpMsgC2s
{
public:

    virtual ~IRtpMsgC2s() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Get media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * Get user information for the c2s<->server link
     */
    virtual void GetUplinkUser(RTP_MSG_USER* myUser) const = 0;

    /*
     * Get encryption suite for the c2s<->server link
     */
    virtual PRO_SSL_SUITE_ID GetUplinkSslSuite(char suiteName[64]) const = 0;

    /*
     * Get local IP address for the c2s<->server link
     */
    virtual const char* GetUplinkLocalIp(char localIp[64]) const = 0;

    /*
     * Get local port number for the c2s<->server link
     */
    virtual unsigned short GetUplinkLocalPort() const = 0;

    /*
     * Get remote IP address for the c2s<->server link
     */
    virtual const char* GetUplinkRemoteIp(char remoteIp[64]) const = 0;

    /*
     * Get remote port number for the c2s<->server link
     */
    virtual unsigned short GetUplinkRemotePort() const = 0;

    /*
     * Set output redline for c2s->server link. Default (1024 * 1024 * 8) bytes
     *
     * Do nothing if redlineBytes is 0
     */
    virtual void SetUplinkOutputRedline(size_t redlineBytes) = 0;

    /*
     * Get output redline for c2s->server link. Default (1024 * 1024 * 8) bytes
     */
    virtual size_t GetUplinkOutputRedline() const = 0;

    /*
     * Get unsent bytes in uplink queue
     */
    virtual size_t GetUplinkSendingBytes() const = 0;

    /*
     * Get local service port number
     */
    virtual unsigned short GetLocalServicePort() const = 0;

    /*
     * Get local SSL cipher suite
     */
    virtual PRO_SSL_SUITE_ID GetLocalSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const = 0;

    /*
     * Get local user count statistics
     */
    virtual void GetLocalUserCount(
        size_t* pendingUserCount, /* = NULL */
        size_t* userCount         /* = NULL */
        ) const = 0;

    /*
     * Kick out a local user
     */
    virtual void KickoutLocalUser(const RTP_MSG_USER* user) = 0;

    /*
     * Set output redline for c2s->user link. Default (1024 * 1024) bytes
     *
     * Do nothing if redlineBytes is 0
     */
    virtual void SetLocalOutputRedline(size_t redlineBytes) = 0;

    /*
     * Get output redline for c2s->user link. Default (1024 * 1024) bytes
     */
    virtual size_t GetLocalOutputRedline() const = 0;

    /*
     * Get unsent bytes in local queue for the user
     */
    virtual size_t GetLocalSendingBytes(const RTP_MSG_USER* user) const = 0;
};

/*
 * Message C2S callback target
 *
 * Users need implement this interface
 */
class IRtpMsgC2sObserver
{
public:

    virtual ~IRtpMsgC2sObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when C2S login is successful
     */
    virtual void OnOkC2s(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * Called on C2S network error or timeout
     */
    virtual void OnCloseC2s(
        IRtpMsgC2s* msgC2s,
        int         errorCode,   /* System error code */
        int         sslCode,     /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        bool        tcpConnected /* Whether TCP connection is established */
        ) = 0;

    /*
     * Called on C2S heartbeat occurrence
     *
     * Mainly for debugging
     */
    virtual void OnHeartbeatC2s(
        IRtpMsgC2s* msgC2s,
        int64_t     peerAliveTick
        ) = 0;

    /*
     * Called when a user login is successful
     */
    virtual void OnOkUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        const char*         userPublicIp
        ) = 0;

    /*
     * Called on user network error or timeout
     */
    virtual void OnCloseUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        int                 errorCode, /* System error code */
        int                 sslCode    /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * Called on user heartbeat occurrence
     *
     * Mainly for debugging
     */
    virtual void OnHeartbeatUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        int64_t             peerAliveTick
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Function: Create a message client
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * mmType           : Media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : SSL configuration. NULL for plaintext transmission
 * sslSni           : SSL server name.
 *                    If valid, participates in server certificate verification
 * remoteIp         : Server IP address or domain name
 * remotePort       : Server port number
 * user             : User information
 * password         : User password
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 *
 * Return: Message client object or NULL
 *
 * Note: The object specified by sslConfig must remain valid throughout
 *       the message client's lifetime
 */
PRO_RTP_API
IRtpMsgClient*
CreateRtpMsgClient(IRtpMsgClientObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_CLIENT_CONFIG* sslConfig,         /* = NULL */
                   const char*                  sslSni,            /* = NULL */
                   const char*                  remoteIp,
                   unsigned short               remotePort,
                   const RTP_MSG_USER*          user,
                   const char*                  password,          /* = NULL */
                   const char*                  localIp,           /* = NULL */
                   unsigned int                 timeoutInSeconds); /* = 0 */

/*
 * Function: Delete a message client
 *
 * Parameters:
 * msgClient : Message client object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpMsgClient(IRtpMsgClient* msgClient);

/*
 * Function: Create a message server
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * mmType           : Media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : SSL configuration. NULL for plaintext transmission
 * sslForced        : Whether to enforce SSL transmission. Ignored if sslConfig is NULL
 * serviceHubPort   : Service hub port number
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 *
 * Return: Message server object or NULL
 *
 * Note: The object specified by sslConfig must remain valid throughout
 *       the message server's lifetime
 */
PRO_RTP_API
IRtpMsgServer*
CreateRtpMsgServer(IRtpMsgServerObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_SERVER_CONFIG* sslConfig,         /* = NULL */
                   bool                         sslForced,         /* = false */
                   unsigned short               serviceHubPort,
                   unsigned int                 timeoutInSeconds); /* = 0 */

/*
 * Function: Delete a message server
 *
 * Parameters:
 * msgServer : Message server object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpMsgServer(IRtpMsgServer* msgServer);

/*
 * Function: Create a message C2S
 *
 * Parameters:
 * observer               : Callback target
 * reactor                : Reactor
 * mmType                 : Media type. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * uplinkSslConfig        : Cascaded SSL configuration. NULL for plaintext between c2s<->server
 * uplinkSslSni           : Cascaded SSL server name.
 *                          If valid, participates in server certificate verification
 * uplinkIp               : Server IP address or domain name
 * uplinkPort             : Server port number
 * uplinkUser             : C2S user information
 * uplinkPassword         : C2S password
 * uplinkLocalIp          : Local IP address to bind for cascading. If NULL, system uses 0.0.0.0
 * uplinkTimeoutInSeconds : Cascaded handshake timeout. Default 20 seconds
 * localSslConfig         : Local SSL configuration. NULL for plaintext transmission
 * localSslForced         : Whether to enforce SSL locally. Ignored if localSslConfig is NULL
 * localServiceHubPort    : Local service hub port number
 * localTimeoutInSeconds  : Local handshake timeout. Default 20 seconds
 *
 * Return: Message C2S object or NULL
 *
 * Note: The object specified by localSslConfig must remain valid throughout
 *       the message C2S's lifetime
 */
PRO_RTP_API
IRtpMsgC2s*
CreateRtpMsgC2s(IRtpMsgC2sObserver*          observer,
                IProReactor*                 reactor,
                RTP_MM_TYPE                  mmType,
                const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,        /* = NULL */
                const char*                  uplinkSslSni,           /* = NULL */
                const char*                  uplinkIp,
                unsigned short               uplinkPort,
                const RTP_MSG_USER*          uplinkUser,
                const char*                  uplinkPassword,
                const char*                  uplinkLocalIp,          /* = NULL */
                unsigned int                 uplinkTimeoutInSeconds, /* = 0 */
                const PRO_SSL_SERVER_CONFIG* localSslConfig,         /* = NULL */
                bool                         localSslForced,         /* = false */
                unsigned short               localServiceHubPort,
                unsigned int                 localTimeoutInSeconds); /* = 0 */

/*
 * Function: Delete a message C2S
 *
 * Parameters:
 * msgC2s : Message C2S object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpMsgC2s(IRtpMsgC2s* msgC2s);

/*
 * Function: Convert RTP_MSG_USER structure to "cid-uid-iid" format string
 *
 * Parameters:
 * user     : Input data
 * idString : Output result
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
RtpMsgUser2String(const RTP_MSG_USER* user,
                  char                idString[64]);

/*
 * Function: Convert "cid-uid" or "cid-uid-iid" format string to RTP_MSG_USER structure
 *
 * Parameters:
 * idString : Input data
 * user     : Output result
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
RtpMsgString2User(const char*   idString,
                  RTP_MSG_USER* user);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____RTP_MSG_H____ */
