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

/*         ______________________________________________________
 *        |                                                      |
 *        |                        ProRtp                        |
 *        |______________________________________________________|
 *        |          |                              |            |
 *        |          |             ProNet           |            |
 *        |          |______________________________|            |
 *        |                    |                                 |
 *        |                    |             ProUtil             |
 *        |      MbedTLS       |_________________________________|
 *        |                              |                       |
 *        |                              |       ProShared       |
 *        |______________________________|_______________________|
 *                     Fig.1 module hierarchy diagram
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
 *                 Fig.2 structure diagram of msg system
 */

#if !defined(____RTP_FOUNDATION_H____)
#define ____RTP_FOUNDATION_H____

#include "rtp_framework.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

class IRtpBucket;

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_UDPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_UDPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_TCPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_TCPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_UDPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_UDPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_TCPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];       /* = "" */
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_TCPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * sslConfig指定的对象必须在会话的生命周期内一直有效;
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_SSLCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    const PRO_SSL_CLIENT_CONFIG* sslConfig;
    char                         sslServiceName[64]; /* = "" */
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];       /* = "" */
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_SSLSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_SSL_CTX*                 sslCtx;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_MCAST
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;          /* = 0 */
    char                         localIp[64];        /* = "" */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 *
 * 如果bucket为NULL,系统将自动分配一个bucket
 */
struct RTP_INIT_MCAST_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;          /* = 0 */
    char                         localIp[64];        /* = "" */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * 会话初始化参数
 */
struct RTP_INIT_COMMON
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
};

/*
 * 会话初始化参数集合
 */
struct RTP_INIT_ARGS
{
    union
    {
        RTP_INIT_UDPCLIENT       udpclient;
        RTP_INIT_UDPSERVER       udpserver;
        RTP_INIT_TCPCLIENT       tcpclient;
        RTP_INIT_TCPSERVER       tcpserver;
        RTP_INIT_UDPCLIENT_EX    udpclientEx;
        RTP_INIT_UDPSERVER_EX    udpserverEx;
        RTP_INIT_TCPCLIENT_EX    tcpclientEx;
        RTP_INIT_TCPSERVER_EX    tcpserverEx;
        RTP_INIT_SSLCLIENT_EX    sslclientEx;
        RTP_INIT_SSLSERVER_EX    sslserverEx;
        RTP_INIT_MCAST           mcast;
        RTP_INIT_MCAST_EX        mcastEx;
        RTP_INIT_COMMON          comm;
    };
};

/*
 * 消息用户号. 1-2-1, 1-2-2, 1-3-1, 1-3-2, ...; 2-1-1, 2-1-2, 2-2-1, 2-2-2, ...
 *
 * classId : 8bits.该字段用于标识用户类别,以便于应用程序分类管理.
 *           0无效, 1应用服务器节点, 2~255应用客户端节点
 *
 * userId  : 40bits.该字段用于标识用户id(如电话号码),由消息服务器分配或许可.
 *           0动态分配,范围为[0xF000000000 ~ 0xFFFFFFFFFF];
 *           否则静态分配,范围为[1 ~ 0xEFFFFFFFFF]
 *
 * instId  : 16bits.该字段用于标识用户实例id(如电话分机号),由消息服务器分配
 *           或许可.有效范围为[0 ~ 65535]
 *
 * 说明    : classId-userId 之 1-1 保留,用于标识消息服务器本身(root)
 */
struct RTP_MSG_USER
{
    RTP_MSG_USER()
    {
        Zero();
    }

    RTP_MSG_USER(
        unsigned char classId,
        PRO_UINT64    userId,
        PRO_UINT16    instId
        )
    {
        this->classId = classId;
        UserId(userId);
        this->instId  = instId;
    }

    void UserId(PRO_UINT64 userId)
    {
        userId5 = (unsigned char)userId;
        userId >>= 8;
        userId4 = (unsigned char)userId;
        userId >>= 8;
        userId3 = (unsigned char)userId;
        userId >>= 8;
        userId2 = (unsigned char)userId;
        userId >>= 8;
        userId1 = (unsigned char)userId;
    }

    /*
     * userId = userId1.userId2.userId3.userId4.userId5
     */
    PRO_UINT64 UserId() const
    {
        PRO_UINT64 userId = userId1;
        userId <<= 8;
        userId |= userId2;
        userId <<= 8;
        userId |= userId3;
        userId <<= 8;
        userId |= userId4;
        userId <<= 8;
        userId |= userId5;

        return (userId);
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
        return (classId == 1 && UserId() == 1);
    }

    bool operator==(const RTP_MSG_USER& user) const
    {
        return (
            classId == user.classId &&
            userId1 == user.userId1 &&
            userId2 == user.userId2 &&
            userId3 == user.userId3 &&
            userId4 == user.userId4 &&
            userId5 == user.userId5 &&
            instId  == user.instId
            );
    }

    bool operator!=(const RTP_MSG_USER& user) const
    {
        return (!(*this == user));
    }

    bool operator<(const RTP_MSG_USER& user) const
    {
        if (classId < user.classId)
        {
            return (true);
        }
        if (classId > user.classId)
        {
            return (false);
        }

        if (userId1 < user.userId1)
        {
            return (true);
        }
        if (userId1 > user.userId1)
        {
            return (false);
        }

        if (userId2 < user.userId2)
        {
            return (true);
        }
        if (userId2 > user.userId2)
        {
            return (false);
        }

        if (userId3 < user.userId3)
        {
            return (true);
        }
        if (userId3 > user.userId3)
        {
            return (false);
        }

        if (userId4 < user.userId4)
        {
            return (true);
        }
        if (userId4 > user.userId4)
        {
            return (false);
        }

        if (userId5 < user.userId5)
        {
            return (true);
        }
        if (userId5 > user.userId5)
        {
            return (false);
        }

        if (instId < user.instId)
        {
            return (true);
        }

        return (false);
    }

    unsigned char classId;
    unsigned char userId1;
    unsigned char userId2;
    unsigned char userId3;
    unsigned char userId4;
    unsigned char userId5;
    PRO_UINT16    instId;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * rtp桶
 */
class IRtpBucket
{
public:

    virtual void PRO_CALLTYPE Destroy() = 0;

    virtual unsigned long PRO_CALLTYPE GetTotalBytes() const = 0;

    virtual IRtpPacket* PRO_CALLTYPE GetFront() = 0;

    virtual bool PRO_CALLTYPE PushBackAddRef(IRtpPacket* packet) = 0;

    virtual void PRO_CALLTYPE PopFrontRelease(IRtpPacket* packet) = 0;

    virtual void PRO_CALLTYPE Reset() = 0;

    virtual void PRO_CALLTYPE SetRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        ) = 0;

    virtual void PRO_CALLTYPE GetRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo() = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息客户端
 */
class IRtpMsgClient
{
public:

    /*
     * 获取用户号
     */
    virtual void PRO_CALLTYPE GetUser(RTP_MSG_USER* user) const = 0;

    /*
     * 发送消息
     *
     * 系统内部有消息发送队列
     */
    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,         /* 消息内容 */
        PRO_UINT16          size,        /* 最多(1024 * 60)字节 */
        PRO_UINT16          charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * 消息客户端回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgClientObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 登录成功时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * 消息到来时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        ) = 0;

    /*
     * 网络错误或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,   /* 系统错误码 */
        long           sslCode,     /* ssl错误码.参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool           tcpConnected /* tcp连接是否已经建立 */
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

    /*
     * 获取用户数
     */
    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* baseUserCount,    /* = NULL */
        unsigned long* subUserCount      /* = NULL */
        ) const = 0;

    /*
     * 踢出用户
     */
    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * 发送消息
     *
     * 系统内部有消息发送队列
     */
    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,         /* 消息内容 */
        PRO_UINT16          size,        /* 最多(1024 * 60)字节 */
        PRO_UINT16          charset,     /* 用户自定义的消息字符集代码 */
        const RTP_MSG_USER* dstUsers,    /* 消息接收者 */
        unsigned char       dstUserCount /* 最多255个目标 */
        ) = 0;

    /*
     * 设置server->c2s链路发送红线.默认(1024 * 1024 * 8)字节
     */
    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes) = 0;

    /*
     * 获取server->c2s链路发送红线.默认(1024 * 1024 * 8)字节
     */
    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * 消息服务器回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgServerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 用户请求登录时,该函数将被回调
     *
     * 上层应该根据用户号,找到匹配的用户口令,然后调用CheckRtpServiceData(...)
     * 进行校验
     *
     * 返回值表示是否允许该用户登录
     */
    virtual bool PRO_CALLTYPE OnCheckUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* 用户发来的登录用户号 */
        const char*         userPublicIp, /* 用户的ip地址 */
        const RTP_MSG_USER* c2sUser,      /* 经由哪个c2s而来.可以是NULL */
        const char          hash[32],     /* 用户发来的口令hash值 */
        PRO_UINT64          nonce,        /* 会话随机数.用于CheckRtpServiceData(...)校验口令hash值 */
        PRO_UINT64*         userId,       /* 上层分配或许可的用户id */
        PRO_UINT16*         instId,       /* 上层分配或许可的实例id */
        PRO_INT64*          appData,      /* 上层设置的标识数据.后续的OnOkUser(...)会带回来 */
        bool*               isC2s         /* 上层设置的是否该节点为c2s */
        ) = 0;

    /*
     * 用户登录成功时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnOkUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* 上层分配或许可的用户号 */
        const char*         userPublicIp, /* 用户的ip地址 */
        const RTP_MSG_USER* c2sUser,      /* 经由哪个c2s而来.可以是NULL */
        PRO_INT64           appData       /* OnCheckUser(...)时上层设置的标识数据 */
        ) = 0;

    /*
     * 用户网络错误或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnCloseUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        long                errorCode,    /* 系统错误码 */
        long                sslCode       /* ssl错误码.参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * 消息到来时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgServer*      msgServer,
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 消息c2s
 *
 * c2s位于client与server之间,它可以分散server的负载,还可以隐藏server的位置.
 * 对于client而言,c2s是透明的,client无法区分它连接的是server还是c2s
 */
class IRtpMsgC2s
{
public:

    /*
     * 获取c2s用户号
     */
    virtual void PRO_CALLTYPE GetC2sUser(RTP_MSG_USER* c2sUser) const = 0;

    /*
     * 获取用户数
     */
    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* userCount         /* = NULL */
        ) const = 0;

    /*
     * 踢出用户
     */
    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * 设置c2s->server链路发送红线.默认(1024 * 1024 * 8)字节
     */
    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes) = 0;

    /*
     * 获取c2s->server链路发送红线.默认(1024 * 1024 * 8)字节
     */
    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * 消息c2s回调目标
 *
 * 使用者需要实现该接口
 */
class IRtpMsgC2sObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * c2s登录成功时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnOkC2s(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* c2sUser,
        const char*         c2sPublicIp
        ) = 0;

    /*
     * c2s网络错误或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnCloseC2s(
        IRtpMsgC2s* msgC2s,
        long        errorCode,         /* 系统错误码 */
        long        sslCode,           /* ssl错误码.参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool        tcpConnected       /* tcp连接是否已经建立 */
        ) = 0;

    /*
     * 用户登录成功时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnOkUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        const char*         userPublicIp
        ) = 0;

    /*
     * 用户网络错误或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnCloseUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        long                errorCode, /* 系统错误码 */
        long                sslCode    /* ssl错误码.参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 功能: 创建一个会话包装
 *
 * 参数:
 * sessionType : 会话类型
 * initArgs    : 会话初始化参数
 * localInfo   : 会话信息
 *
 * 返回值: 会话包装对象或NULL
 *
 * 说明: 会话包装对象包含流控策略
 */
PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionWrapper(RTP_SESSION_TYPE        sessionType,
                        const RTP_INIT_ARGS*    initArgs,
                        const RTP_SESSION_INFO* localInfo);

/*
 * 功能: 删除一个会话包装
 *
 * 参数:
 * sessionWrapper : 会话包装对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpSessionWrapper(IRtpSession* sessionWrapper);

/*
 * 功能: 创建一个消息客户端
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * mmType           : 消息媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl配置. NULL表示明文传输
 * sslServiceName   : ssl服务名.如果有效,则参与认证服务端证书
 * remoteIp         : 服务器的ip地址或域名
 * remotePort       : 服务器的端口号
 * user             : 用户号
 * password         : 用户口令
 * localIp          : 要绑定的本地ip地址.如果为NULL,系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时.默认20秒
 *
 * 返回值: 消息客户端对象或NULL
 *
 * 说明: sslConfig指定的对象必须在消息客户端的生命周期内一直有效
 */
PRO_RTP_API
IRtpMsgClient*
PRO_CALLTYPE
CreateRtpMsgClient(IRtpMsgClientObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_CLIENT_CONFIG* sslConfig,         /* = NULL */
                   const char*                  sslServiceName,    /* = NULL */
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
PRO_CALLTYPE
DeleteRtpMsgClient(IRtpMsgClient* msgClient);

/*
 * 功能: 创建一个消息服务器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * mmType           : 消息媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl配置. NULL表示明文传输
 * sslForced        : 是否强制使用ssl传输. sslConfig为NULL时该参数忽略
 * serviceHubPort   : 服务hub监听的端口号
 * timeoutInSeconds : 握手超时.默认20秒
 *
 * 返回值: 消息服务器对象或NULL
 *
 * 说明: sslConfig指定的对象必须在消息服务器的生命周期内一直有效
 */
PRO_RTP_API
IRtpMsgServer*
PRO_CALLTYPE
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
PRO_CALLTYPE
DeleteRtpMsgServer(IRtpMsgServer* msgServer);

/*
 * 功能: 创建一个消息c2s
 *
 * 参数:
 * observer               : 回调目标
 * reactor                : 反应器
 * mmType                 : 消息媒体类型. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * uplinkSslConfig        : 级联的ssl配置. NULL表示c2s<->server之间明文传输
 * uplinkSslServiceName   : 级联的ssl服务名.如果有效,则参与认证服务端证书
 * uplinkIp               : 服务器的ip地址或域名
 * uplinkPort             : 服务器的端口号
 * uplinkUser             : c2s的用户号
 * uplinkPassword         : c2s的用户口令
 * uplinkLocalIp          : 级联时要绑定的本地ip地址.如果为NULL,系统将使用0.0.0.0
 * uplinkTimeoutInSeconds : 级联的握手超时.默认20秒
 * localSslConfig         : 近端的ssl配置. NULL表示明文传输
 * localSslForced         : 近端是否强制使用ssl传输. localSslConfig为NULL时该参数忽略
 * localServiceHubPort    : 近端的服务hub监听的端口号
 * localTimeoutInSeconds  : 近端的握手超时.默认20秒
 *
 * 返回值: 消息c2s对象或NULL
 *
 * 说明: localSslConfig指定的对象必须在消息c2s的生命周期内一直有效
 */
PRO_RTP_API
IRtpMsgC2s*
PRO_CALLTYPE
CreateRtpMsgC2s(IRtpMsgC2sObserver*          observer,
                IProReactor*                 reactor,
                RTP_MM_TYPE                  mmType,
                const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,        /* = NULL */
                const char*                  uplinkSslServiceName,   /* = NULL */
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
PRO_CALLTYPE
DeleteRtpMsgC2s(IRtpMsgC2s* msgC2s);

/*
 * 功能: 创建一个内部实现的基础类型的rtp桶
 *
 * 参数: 无
 *
 * 返回值: rtp桶对象或NULL
 *
 * 说明: rtp桶的线程安全性由调用者保证
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpBaseBucket();

/*
 * 功能: 创建一个内部实现的音频类型的rtp桶
 *
 * 参数: 无
 *
 * 返回值: rtp桶对象或NULL
 *
 * 说明: rtp桶的线程安全性由调用者保证
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpAudioBucket();

/*
 * 功能: 创建一个内部实现的视频类型的rtp桶
 *
 * 参数: 无
 *
 * 返回值: rtp桶对象或NULL
 *
 * 说明: rtp桶的线程安全性由调用者保证
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpVideoBucket();

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
PRO_CALLTYPE
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
PRO_CALLTYPE
RtpMsgString2User(const char*   idString,
                  RTP_MSG_USER* user);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____RTP_FOUNDATION_H____ */
