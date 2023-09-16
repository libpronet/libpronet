﻿/*
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
 *        |                      msg server                      |
 *        |______________________________________________________|
 *                 |             |        |              |
 *                 | ...         |        |          ... |
 *         ________|_______      |        |      ________|_______
 *        |                |     |        |     |                |
 *        |     msg c2s    |     |        |     |     msg c2s    |
 *        |________________|     |        |     |________________|
 *            |        |         |        |         |        |
 *            |  ...   |         |  ...   |         |  ...   |
 *         ___|___  ___|___   ___|___  ___|___   ___|___  ___|___
 *        |  msg  ||  msg  | |  msg  ||  msg  | |  msg  ||  msg  |
 *        | client|| client| | client|| client| | client|| client|
 *        |_______||_______| |_______||_______| |_______||_______|
 *                 Fig.1 structure diagram of msg system
 */

/*
 * 1)  client ----->                 connect()                  -----> server
 * 2)  client <-----                  accept()                  <----- server
 * 3a) client <-----                 PRO_NONCE                  <----- server
 * 3b) client ----->    serviceId + serviceOpt + (r) + (r+1)    -----> server
 * 4]  client <<====              [ssl handshake]               ====>> server
 *     client::passwordHash
 * 5)  client -----> rtp(RTP_SESSION_INFO with RTP_MSG_HEADER0) -----> server
 *                                                       passwordHash::server
 * 6)  client <----- rtp(RTP_SESSION_ACK with RTP_MSG_HEADER0)  <----- server
 * 7)  client <<====        tcp4(RTP_MSG_HEADER + data)         ====>> server
 *                 Fig.2 msg system handshake protocol flow chart
 */

/*
 * PMP-v2.0 (PRO Messaging Protocol version 2.0)
 */

#if !defined(____RTP_MSG_H____)
#define ____RTP_MSG_H____

#include "rtp_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息用户号. 1-2-*, 1-3-*, ...; 2-1-*, 2-2-*, 2-3-*, ...; ...
 *
 * classId : 8bits. 该字段用于标识用户类别, 以便于应用程序分类管理.
 * (cid)     0无效, 1应用服务器节点, 2~255应用客户端节点
 *
 * userId  : 40bits. 该字段用于标识用户id(如电话号码), 由消息服务器分配或许可.
 * (uid)     0动态分配, 有效范围为[0xF000000000 ~ 0xFFFFFFFFFF];
 *           否则静态分配, 有效范围为[1 ~ 0xEFFFFFFFFF]
 *
 * instId  : 16bits. 该字段用于标识用户实例id(如电话分机号), 由消息服务器分配
 * (iid)     或许可. 有效范围为[0 ~ 65535]
 *
 * 说明    : cid-uid-iid 之 1-1-* 保留, 用于标识消息服务器本身(root)
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
 * 消息握手头
 */
struct RTP_MSG_HEADER0
{
    uint16_t     version; /* the current protocol version is 02 */
    RTP_MSG_USER user;
    char         reserved1[2];
    union
    {
        char     reserved2[24];
        uint32_t publicIp;
    };
};

/*
 * 消息数据头
 */
struct RTP_MSG_HEADER
{
    uint16_t       charset;      /* ANSI, UTF-8, ... */
    RTP_MSG_USER   srcUser;
    char           reserved;
    unsigned char  dstUserCount; /* to 255 users at most */
    RTP_MSG_USER   dstUsers[1];  /* a variable-length array */
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息客户端
 */
class IRtpMsgClient
{
public:

    virtual ~IRtpMsgClient() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 获取媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * 获取用户号
     */
    virtual void GetUser(RTP_MSG_USER* myUser) const = 0;

    /*
     * 获取加密套件
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const = 0;

    /*
     * 获取本地ip地址
     */
    virtual const char* GetLocalIp(char localIp[64]) const = 0;

    /*
     * 获取本地端口号
     */
    virtual unsigned short GetLocalPort() const = 0;

    /*
     * 获取远端ip地址
     */
    virtual const char* GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * 获取远端端口号
     */
    virtual unsigned short GetRemotePort() const = 0;

    /*
     * 发送消息
     *
     * 系统内部有消息发送队列
     */
    virtual bool SendMsg(
        const void*         buf,         /* 消息内容 */
        size_t              size,        /* 消息长度 */
        uint16_t            charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    /*
     * 发送消息(buf1 + buf2)
     *
     * 系统内部有消息发送队列
     */
    virtual bool SendMsg2(
        const void*         buf1,        /* 消息内容1 */
        size_t              size1,       /* 消息长度1 */
        const void*         buf2,        /* 消息内容2. 可以是NULL */
        size_t              size2,       /* 消息长度2. 可以是0 */
        uint16_t            charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    /*
     * 设置链路发送红线. 默认(1024 * 1024)字节
     *
     * 如果redlineBytes为0, 则直接返回, 什么都不做
     */
    virtual void SetOutputRedline(size_t redlineBytes) = 0;

    /*
     * 获取链路发送红线. 默认(1024 * 1024)字节
     */
    virtual size_t GetOutputRedline() const = 0;

    /*
     * 获取链路缓存的尚未发送的字节数
     */
    virtual size_t GetSendingBytes() const = 0;
};

/*
 * 消息客户端回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgClientObserver
{
public:

    virtual ~IRtpMsgClientObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 登录成功时, 该函数将被回调
     */
    virtual void OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * 消息到来时, 该函数将被回调
     */
    virtual void OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        unsigned long       size,
        uint16_t            charset,
        const RTP_MSG_USER* srcUser
        ) = 0;

    /*
     * 网络错误或超时时, 该函数将被回调
     */
    virtual void OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,   /* 系统错误码 */
        long           sslCode,     /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool           tcpConnected /* tcp连接是否已经建立 */
        ) = 0;

    /*
     * 心跳发生时, 该函数将被回调
     *
     * 主要用于调试
     */
    virtual void OnHeartbeatMsg(
        IRtpMsgClient* msgClient,
        int64_t        peerAliveTick
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息服务器
 */
class IRtpMsgServer
{
public:

    virtual ~IRtpMsgServer() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 获取媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * 获取服务端口号
     */
    virtual unsigned short GetServicePort() const = 0;

    /*
     * 获取链路加密套件
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const = 0;

    /*
     * 获取用户数
     */
    virtual void GetUserCount(
        size_t* pendingUserCount, /* = NULL */
        size_t* baseUserCount,    /* = NULL */
        size_t* subUserCount      /* = NULL */
        ) const = 0;

    /*
     * 踢出用户
     */
    virtual void KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * 发送消息
     *
     * 系统内部有消息发送队列
     */
    virtual bool SendMsg(
        const void*         buf,         /* 消息内容 */
        size_t              size,        /* 消息长度 */
        uint16_t            charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    /*
     * 发送消息(buf1 + buf2)
     *
     * 系统内部有消息发送队列
     */
    virtual bool SendMsg2(
        const void*         buf1,        /* 消息内容1 */
        size_t              size1,       /* 消息长度1 */
        const void*         buf2,        /* 消息内容2. 可以是NULL */
        size_t              size2,       /* 消息长度2. 可以是0 */
        uint16_t            charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    /*
     * 设置server->c2s链路的发送红线. 默认(1024 * 1024 * 8)字节
     *
     * 如果redlineBytes为0, 则直接返回, 什么都不做
     */
    virtual void SetOutputRedlineToC2s(size_t redlineBytes) = 0;

    /*
     * 获取server->c2s链路的发送红线. 默认(1024 * 1024 * 8)字节
     */
    virtual size_t GetOutputRedlineToC2s() const = 0;

    /*
     * 设置server->user链路的发送红线. 默认(1024 * 1024)字节
     *
     * 如果redlineBytes为0, 则直接返回, 什么都不做
     */
    virtual void SetOutputRedlineToUsr(size_t redlineBytes) = 0;

    /*
     * 获取server->user链路的发送红线. 默认(1024 * 1024)字节
     */
    virtual size_t GetOutputRedlineToUsr() const = 0;

    /*
     * 获取链路缓存的尚未发送的字节数
     */
    virtual size_t GetSendingBytes(const RTP_MSG_USER* user) const = 0;
};

/*
 * 消息服务器回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgServerObserver
{
public:

    virtual ~IRtpMsgServerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 用户请求登录时, 该函数将被回调
     *
     * 上层应该根据用户号, 找到匹配的用户口令, 然后调用CheckRtpServiceData()
     * 进行校验
     *
     * 返回值表示是否允许该用户登录
     */
    virtual bool OnCheckUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* 用户发来的登录用户号 */
        const char*         userPublicIp, /* 用户的ip地址 */
        const RTP_MSG_USER* c2sUser,      /* 经由哪个c2s而来. 可以是NULL */
        const char          hash[32],     /* 用户发来的口令hash值 */
        const char          nonce[32],    /* 会话随机数. 用于CheckRtpServiceData()校验口令hash值 */
        uint64_t*           userId,       /* 上层分配或许可的uid */
        uint16_t*           instId,       /* 上层分配或许可的iid */
        int64_t*            appData,      /* 上层设置的标识数据. 后续的OnOkUser()会带回来 */
        bool*               isC2s         /* 上层设置的是否该节点为c2s */
        ) = 0;

    /*
     * 用户登录成功时, 该函数将被回调
     */
    virtual void OnOkUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* 上层分配或许可的用户号 */
        const char*         userPublicIp, /* 用户的ip地址 */
        const RTP_MSG_USER* c2sUser,      /* 经由哪个c2s而来. 可以是NULL */
        int64_t             appData       /* OnCheckUser()时上层设置的标识数据 */
        ) = 0;

    /*
     * 用户网络错误或超时时, 该函数将被回调
     */
    virtual void OnCloseUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        long                errorCode, /* 系统错误码 */
        long                sslCode    /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * 用户心跳发生时, 该函数将被回调
     *
     * 主要用于调试
     */
    virtual void OnHeartbeatUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        int64_t             peerAliveTick
        ) = 0;

    /*
     * 消息到来时, 该函数将被回调
     */
    virtual void OnRecvMsg(
        IRtpMsgServer*      msgServer,
        const void*         buf,
        unsigned long       size,
        uint16_t            charset,
        const RTP_MSG_USER* srcUser
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息c2s
 *
 * c2s位于client与server之间, 它可以分散server的负载, 还可以隐藏server的位置.
 * 对于client而言, c2s是透明的, client无法区分它连接的是server还是c2s
 */
class IRtpMsgC2s
{
public:

    virtual ~IRtpMsgC2s() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 获取媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
     */
    virtual RTP_MM_TYPE GetMmType() const = 0;

    /*
     * 获取c2s<->server链路的用户号
     */
    virtual void GetUplinkUser(RTP_MSG_USER* myUser) const = 0;

    /*
     * 获取c2s<->server链路的加密套件
     */
    virtual PRO_SSL_SUITE_ID GetUplinkSslSuite(char suiteName[64]) const = 0;

    /*
     * 获取c2s<->server链路的本地ip地址
     */
    virtual const char* GetUplinkLocalIp(char localIp[64]) const = 0;

    /*
     * 获取c2s<->server链路的本地端口号
     */
    virtual unsigned short GetUplinkLocalPort() const = 0;

    /*
     * 获取c2s<->server链路的远端ip地址
     */
    virtual const char* GetUplinkRemoteIp(char remoteIp[64]) const = 0;

    /*
     * 获取c2s<->server链路的远端端口号
     */
    virtual unsigned short GetUplinkRemotePort() const = 0;

    /*
     * 设置c2s->server链路的发送红线. 默认(1024 * 1024 * 8)字节
     *
     * 如果redlineBytes为0, 则直接返回, 什么都不做
     */
    virtual void SetUplinkOutputRedline(size_t redlineBytes) = 0;

    /*
     * 获取c2s->server链路的发送红线. 默认(1024 * 1024 * 8)字节
     */
    virtual size_t GetUplinkOutputRedline() const = 0;

    /*
     * 获取c2s->server链路缓存的尚未发送的字节数
     */
    virtual size_t GetUplinkSendingBytes() const = 0;

    /*
     * 获取本地服务端口号
     */
    virtual unsigned short GetLocalServicePort() const = 0;

    /*
     * 获取本地链路加密套件
     */
    virtual PRO_SSL_SUITE_ID GetLocalSslSuite(
        const RTP_MSG_USER* user,
        char                suiteName[64]
        ) const = 0;

    /*
     * 获取本地用户数
     */
    virtual void GetLocalUserCount(
        size_t* pendingUserCount, /* = NULL */
        size_t* userCount         /* = NULL */
        ) const = 0;

    /*
     * 踢出本地用户
     */
    virtual void KickoutLocalUser(const RTP_MSG_USER* user) = 0;

    /*
     * 设置c2s->user链路的发送红线. 默认(1024 * 1024)字节
     *
     * 如果redlineBytes为0, 则直接返回, 什么都不做
     */
    virtual void SetLocalOutputRedline(size_t redlineBytes) = 0;

    /*
     * 获取c2s->user链路的发送红线. 默认(1024 * 1024)字节
     */
    virtual size_t GetLocalOutputRedline() const = 0;

    /*
     * 获取c2s->user链路缓存的尚未发送的字节数
     */
    virtual size_t GetLocalSendingBytes(const RTP_MSG_USER* user) const = 0;
};

/*
 * 消息c2s回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgC2sObserver
{
public:

    virtual ~IRtpMsgC2sObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * c2s登录成功时, 该函数将被回调
     */
    virtual void OnOkC2s(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * c2s网络错误或超时时, 该函数将被回调
     */
    virtual void OnCloseC2s(
        IRtpMsgC2s* msgC2s,
        long        errorCode,   /* 系统错误码 */
        long        sslCode,     /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool        tcpConnected /* tcp连接是否已经建立 */
        ) = 0;

    /*
     * c2s心跳发生时, 该函数将被回调
     *
     * 主要用于调试
     */
    virtual void OnHeartbeatC2s(
        IRtpMsgC2s* msgC2s,
        int64_t     peerAliveTick
        ) = 0;

    /*
     * 用户登录成功时, 该函数将被回调
     */
    virtual void OnOkUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        const char*         userPublicIp
        ) = 0;

    /*
     * 用户网络错误或超时时, 该函数将被回调
     */
    virtual void OnCloseUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        long                errorCode, /* 系统错误码 */
        long                sslCode    /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * 用户心跳发生时, 该函数将被回调
     *
     * 主要用于调试
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
 * 功能: 创建一个消息客户端
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * mmType           : 媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl配置. NULL表示明文传输
 * sslSni           : ssl服务名. 如果有效, 则参与认证服务端证书
 * remoteIp         : 服务器的ip地址或域名
 * remotePort       : 服务器的端口号
 * user             : 用户号
 * password         : 用户口令
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时. 默认20秒
 *
 * 返回值: 消息客户端对象或NULL
 *
 * 说明: sslConfig指定的对象必须在消息客户端的生命周期内一直有效
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
                   unsigned long                timeoutInSeconds); /* = 0 */

/*
 * 功能: 删除一个消息客户端
 *
 * 参数:
 * msgClient : 消息客户端对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_RTP_API
void
DeleteRtpMsgClient(IRtpMsgClient* msgClient);

/*
 * 功能: 创建一个消息服务器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * mmType           : 媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl配置. NULL表示明文传输
 * sslForced        : 是否强制使用ssl传输. sslConfig为NULL时该参数忽略
 * serviceHubPort   : 服务hub的端口号
 * timeoutInSeconds : 握手超时. 默认20秒
 *
 * 返回值: 消息服务器对象或NULL
 *
 * 说明: sslConfig指定的对象必须在消息服务器的生命周期内一直有效
 */
PRO_RTP_API
IRtpMsgServer*
CreateRtpMsgServer(IRtpMsgServerObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_SERVER_CONFIG* sslConfig,         /* = NULL */
                   bool                         sslForced,         /* = false */
                   unsigned short               serviceHubPort,
                   unsigned long                timeoutInSeconds); /* = 0 */

/*
 * 功能: 删除一个消息服务器
 *
 * 参数:
 * msgServer : 消息服务器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_RTP_API
void
DeleteRtpMsgServer(IRtpMsgServer* msgServer);

/*
 * 功能: 创建一个消息c2s
 *
 * 参数:
 * observer               : 回调目标
 * reactor                : 反应器
 * mmType                 : 媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * uplinkSslConfig        : 级联的ssl配置. NULL表示c2s<->server之间明文传输
 * uplinkSslSni           : 级联的ssl服务名. 如果有效, 则参与认证服务端证书
 * uplinkIp               : 服务器的ip地址或域名
 * uplinkPort             : 服务器的端口号
 * uplinkUser             : c2s的用户号
 * uplinkPassword         : c2s的用户口令
 * uplinkLocalIp          : 级联时要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * uplinkTimeoutInSeconds : 级联的握手超时. 默认20秒
 * localSslConfig         : 近端的ssl配置. NULL表示明文传输
 * localSslForced         : 近端是否强制使用ssl传输. localSslConfig为NULL时该参数忽略
 * localServiceHubPort    : 近端的服务hub的端口号
 * localTimeoutInSeconds  : 近端的握手超时. 默认20秒
 *
 * 返回值: 消息c2s对象或NULL
 *
 * 说明: localSslConfig指定的对象必须在消息c2s的生命周期内一直有效
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
                unsigned long                uplinkTimeoutInSeconds, /* = 0 */
                const PRO_SSL_SERVER_CONFIG* localSslConfig,         /* = NULL */
                bool                         localSslForced,         /* = false */
                unsigned short               localServiceHubPort,
                unsigned long                localTimeoutInSeconds); /* = 0 */

/*
 * 功能: 删除一个消息c2s
 *
 * 参数:
 * msgC2s : 消息c2s对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_RTP_API
void
DeleteRtpMsgC2s(IRtpMsgC2s* msgC2s);

/*
 * 功能: 将RTP_MSG_USER结构转换为"cid-uid-iid"格式的串
 *
 * 参数:
 * user     : 输入数据
 * idString : 输出结果
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_RTP_API
void
RtpMsgUser2String(const RTP_MSG_USER* user,
                  char                idString[64]);

/*
 * 功能: 将"cid-uid"或"cid-uid-iid"格式的串转换为RTP_MSG_USER结构
 *
 * 参数:
 * idString : 输入数据
 * user     : 输出结果
 *
 * 返回值: 无
 *
 * 说明: 无
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
