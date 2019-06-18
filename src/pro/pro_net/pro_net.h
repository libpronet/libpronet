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
 *        |                        ProNet                        |
 *        |______________________________________________________|
 *        |                    |                                 |
 *        |                    |             ProUtil             |
 *        |                    |_________________________________|
 *        |      MbedTLS                 |                       |
 *        |                              |       ProShared       |
 *        |______________________________|_______________________|
 *                     Fig.1 module hierarchy diagram
 */

/*         ________________   ________________   ________________
 *        |    Acceptor    | |   Connector    | |   Handshaker   |
 *        | (EventHandler) | | (EventHandler) | | (EventHandler) |
 *        |________________| |________________| |________________|
 *                 A                  A                  A
 *                 |       ...        |       ...        |
 *         ________|__________________|__________________|_______
 *        |                       Reactor                        |
 *        |   ____________     _____________     _____________   |
 *        |  A            |   A             |   A             |  |
 *        |  |     Acc    |   |     I/O     |   |   General   |  |
 *        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
 *        |  |____________V   |_____________V   |_____________V  |
 *        |                                                      |
 *        |   ____________     _____________     _____________   |
 *        |  A            |   A             |   A             |  |
 *        |  |    I/O     |   |     I/O     |   |     MM      |  |
 *        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
 *        |  |____________V   |_____________V   |_____________V  |
 *        |                                                      |
 *        |       ...               ...               ...        |
 *        |______________________________________________________|
 *                 |                  |                  |
 *                 |       ...        |       ...        |
 *         ________V_______   ________V_______   ________V_______
 *        |   Transport    | |   ServiceHub   | |  ServiceHost   |
 *        | (EventHandler) | | (EventHandler) | | (EventHandler) |
 *        |________________| |________________| |________________|
 *                   Fig.2 structure diagram of Reactor
 */

/*       __________________                     _________________
 *      |    ServiceHub    |<----------------->|   ServiceHost   |
 *      |   ____________   |    ServicePipe    |(Acceptor Shadow)| Audio-Process
 *      |  |            |  |                   |_________________|
 *      |  |  Acceptor  |  |        ...         _________________
 *      |  |____________|  |                   |   ServiceHost   |
 *      |                  |    ServicePipe    |(Acceptor Shadow)| Video-Process
 *      |__________________|<----------------->|_________________|
 *           Hub-Process
 *                Fig.3 structure diagram of ServiceHub
 */

/*
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 *       Fig.4 acceptor_ex/connector_ex handshake protocol flow chart
 */

#if !defined(____PRO_NET_H____)
#define ____PRO_NET_H____

#include <cstddef>

#if !defined(____PRO_A_H____)
#define ____PRO_A_H____
#define PRO_INT16_VCC    short
#define PRO_INT32_VCC    int
#if defined(_MSC_VER)
#define PRO_INT64_VCC    __int64
#define PRO_PRT64D_VCC   "%I64d"
#else
#define PRO_INT64_VCC    long long
#define PRO_PRT64D_VCC   "%lld"
#endif
#define PRO_UINT16_VCC   unsigned short
#define PRO_UINT32_VCC   unsigned int
#if defined(_MSC_VER)
#define PRO_UINT64_VCC   unsigned __int64
#define PRO_PRT64U_VCC   "%I64u"
#else
#define PRO_UINT64_VCC   unsigned long long
#define PRO_PRT64U_VCC   "%llu"
#endif
#define PRO_CALLTYPE_VCC __stdcall
#define PRO_EXPORT_VCC   __declspec(dllexport)
#define PRO_IMPORT_VCC
#define PRO_INT16_GCC    short
#define PRO_INT32_GCC    int
#define PRO_INT64_GCC    long long
#define PRO_PRT64D_GCC   "%lld"
#define PRO_UINT16_GCC   unsigned short
#define PRO_UINT32_GCC   unsigned int
#define PRO_UINT64_GCC   unsigned long long
#define PRO_PRT64U_GCC   "%llu"
#define PRO_CALLTYPE_GCC
#define PRO_EXPORT_GCC   __attribute__((visibility("default")))
#define PRO_IMPORT_GCC
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_VCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_VCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_VCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_VCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_VCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_VCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_VCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_VCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_VCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_VCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_VCC
#endif
#else  /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_GCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_GCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_GCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_GCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_GCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_GCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_GCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_GCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_GCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_GCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_GCC
#endif
#endif /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#endif /* ____PRO_A_H____ */

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_NET_LIB)
#define PRO_NET_API
#elif defined(PRO_NET_EXPORTS)
#if defined(_MSC_VER)
#define PRO_NET_API /* .def */
#else
#define PRO_NET_API PRO_EXPORT
#endif
#else
#define PRO_NET_API PRO_IMPORT
#endif

#include "pro_mbedtls.h"

class  IProAcceptor;      /* 接受器 */
class  IProConnector;     /* 连接器 */
class  IProServiceHost;   /* 服务host */
class  IProServiceHub;    /* 服务hub */
class  IProSslHandshaker; /* ssl握手器 */
class  IProTcpHandshaker; /* tcp握手器 */
struct pbsd_sockaddr_in;  /* 套接字地址 */

/*
 * [[[[ 传输器类型
 */
typedef unsigned char PRO_TRANS_TYPE;

static const PRO_TRANS_TYPE PRO_TRANS_TCP   =  1;
static const PRO_TRANS_TYPE PRO_TRANS_UDP   =  2;
static const PRO_TRANS_TYPE PRO_TRANS_MCAST =  3;
static const PRO_TRANS_TYPE PRO_TRANS_SSL   = 11;
/*
 * ]]]]
 */

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 定时器回调目标
 *
 * 使用者需要实现该接口
 *
 * please refer to "pro_util/pro_timer_factory.h"
 */
#if !defined(____IProOnTimer____)
#define ____IProOnTimer____
class IProOnTimer
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        ) = 0;
};
#endif /* ____IProOnTimer____ */

/*
 * 反应器
 *
 * 这里只暴露了定时器相关的接口,目的是为了在使用反应器的时候,可以方便地
 * 使用其内部的定时器功能,免得还要创建额外的CProTimerFactory对象
 */
class IProReactor
{
public:

    /*
     * 创建一个普通定时器
     *
     * 返回值为定时器id. 0无效
     */
    virtual unsigned long PRO_CALLTYPE ScheduleTimer(
        IProOnTimer* onTimer,
        PRO_UINT64   timeSpan,  /* 定时周期(ms) */
        bool         recurring, /* 是否重复. 如果timeSpan为0, recurring必须为false */
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * 创建一个用于链路心跳的普通定时器(心跳定时器)
     *
     * 各个心跳定时器的心跳时间点由内部算法决定,内部会进行均匀化处理.
     * 心跳事件发生时,上层可以在OnTimer(...)回调里发送心跳数据包
     *
     * 返回值为定时器id. 0无效
     */
    virtual unsigned long PRO_CALLTYPE ScheduleHeartbeatTimer(
        IProOnTimer* onTimer,
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * 更新全体心跳定时器的心跳周期
     *
     * 默认的心跳周期为25秒
     */
    virtual bool PRO_CALLTYPE UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds) = 0;

    /*
     * 删除一个普通定时器
     */
    virtual void PRO_CALLTYPE CancelTimer(unsigned long timerId) = 0;

    /*
     * 创建一个多媒体定时器(高精度定时器)
     *
     * 返回值为定时器id. 0无效
     */
    virtual unsigned long PRO_CALLTYPE ScheduleMmTimer(
        IProOnTimer* onTimer,
        PRO_UINT64   timeSpan,  /* 定时周期(ms) */
        bool         recurring, /* 是否重复. 如果timeSpan为0, recurring必须为false */
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * 删除一个多媒体定时器(高精度定时器)
     */
    virtual void PRO_CALLTYPE CancelMmTimer(unsigned long timerId) = 0;

    /*
     * 获取状态信息字符串
     */
    virtual void PRO_CALLTYPE GetTraceInfo(char* buf, size_t size) const = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 接受器回调目标
 *
 * 使用者需要实现该接口
 */
class IProAcceptorObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 有连接进入时,该函数将被回调
     *
     * 使用者负责sockId的资源维护
     */
    virtual void PRO_CALLTYPE OnAccept(
        IProAcceptor*  acceptor,
        PRO_INT64      sockId,     /* 套接字id */
        bool           unixSocket, /* 是否unix套接字 */
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort, /* 远端的端口号. > 0 */
        unsigned char  serviceId,  /* 允许服务扩展时,远端请求的服务id */
        unsigned char  serviceOpt, /* 允许服务扩展时,远端请求的服务选项 */
        PRO_UINT64     nonce       /* 允许服务扩展时,会话随机数 */
        ) = 0;
};

/*
 * 服务host回调目标
 *
 * 使用者需要实现该接口
 */
class IProServiceHostObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 有连接进入时,该函数将被回调
     *
     * 使用者负责sockId的资源维护
     */
    virtual void PRO_CALLTYPE OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,     /* 套接字id */
        bool             unixSocket, /* 是否unix套接字 */
        const char*      remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short   remotePort, /* 远端的端口号. > 0 */
        unsigned char    serviceId,  /* 远端请求的服务id */
        unsigned char    serviceOpt, /* 远端请求的服务选项 */
        PRO_UINT64       nonce       /* 会话随机数 */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 连接器回调目标
 *
 * 使用者需要实现该接口
 */
class IProConnectorObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 连接成功时,该函数将被回调
     *
     * 使用者负责sockId的资源维护
     */
    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector* connector,
        PRO_INT64      sockId,     /* 套接字id */
        bool           unixSocket, /* 是否unix套接字 */
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort, /* 远端的端口号. > 0 */
        unsigned char  serviceId,  /* 允许服务扩展时,请求的服务id */
        unsigned char  serviceOpt, /* 允许服务扩展时,请求的服务选项 */
        PRO_UINT64     nonce       /* 允许服务扩展时,会话随机数 */
        ) = 0;

    /*
     * 连接出错或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort, /* 远端的端口号. > 0 */
        unsigned char  serviceId,  /* 允许服务扩展时,请求的服务id */
        unsigned char  serviceOpt, /* 允许服务扩展时,请求的服务选项 */
        bool           timeout     /* 是否连接超时 */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * tcp握手器回调目标
 *
 * 使用者需要实现该接口
 */
class IProTcpHandshakerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 握手成功时,该函数将被回调
     *
     * 使用者负责sockId的资源维护.
     * 握手完成后,上层应该把sockId包装成IProTransport,
     * 或释放sockId对应的资源
     */
    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,     /* 套接字id */
        bool               unixSocket, /* 是否unix套接字 */
        const void*        buf,        /* 握手数据. 可以是NULL */
        unsigned long      size        /* 数据长度. 可以是0 */
        ) = 0;

    /*
     * 握手出错或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode   /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
        ) = 0;
};

/*
 * ssl握手器回调目标
 *
 * 使用者需要实现该接口
 */
class IProSslHandshakerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 握手成功时,该函数将被回调
     *
     * 使用者负责(ctx, sockId)的资源维护.
     * 握手完成后,上层应该把(ctx, sockId)包装成IProTransport,
     * 或释放(ctx, sockId)对应的资源
     *
     * 调用ProSslCtx_GetSuite(...)可以获知当前使用的加密套件
     */
    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,        /* ssl上下文 */
        PRO_INT64          sockId,     /* 套接字id */
        bool               unixSocket, /* 是否unix套接字 */
        const void*        buf,        /* 握手数据. 可以是NULL */
        unsigned long      size        /* 数据长度. 可以是0 */
        ) = 0;

    /*
     * 握手出错或超时时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,  /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
        long               sslCode     /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 接收池
 *
 * 必须在IProTransportObserver::OnRecv(...)的线程上下文里使用,否则不安全
 *
 * 对于tcp/ssl传输器,使用环型接收池.
 * 由于tcp是流式工作的,收端动力数与发端动力数不一定相同,所以,在OnRecv(...)
 * 里应该尽量取走接收池内的数据. 如果没有新的数据到来,反应器不会再次报告接收
 * 池内还有剩余数据这件事
 *
 * 对于udp/mcast传输器,使用线性接收池.
 * 为防止接收池空间不足导致EMSGSIZE错误,在OnRecv(...)里应该收完全部数据
 *
 * 另外,当反应器报告套接字内有数据可读时,如果接收池已经<<满了>>,那么该套接字
 * 将被关闭!!! 这说明收端和发端的配合逻辑有问题!!!
 */
class IProRecvPool
{
public:

    /*
     * 查询接收池内的数据量
     */
    virtual unsigned long PRO_CALLTYPE PeekDataSize() const = 0;

    /*
     * 读取接收池内的数据
     */
    virtual void PRO_CALLTYPE PeekData(void* buf, size_t size) const = 0;

    /*
     * 刷掉已经读取的数据
     *
     * 腾出空间,以便容纳新的数据
     */
    virtual void PRO_CALLTYPE Flush(size_t size) = 0;

    /*
     * 查询接收池内剩余的存储空间
     */
    virtual unsigned long PRO_CALLTYPE GetFreeSize() const = 0;
};

/*
 * 传输器
 */
class IProTransport
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 获取传输器类型
     */
    virtual PRO_TRANS_TYPE PRO_CALLTYPE GetType() const = 0;

    /*
     * 获取加密套件
     *
     * 仅用于PRO_TRANS_SSL类型的传输器
     */
    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(char suiteName[64]) const = 0;

    /*
     * 获取底层的套接字id
     *
     * 如非必需,最好不要直接操作底层的套接字
     */
    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const = 0;

    /*
     * 获取套接字的本地ip地址
     */
    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const = 0;

    /*
     * 获取套接字的本地端口号
     */
    virtual unsigned short PRO_CALLTYPE GetLocalPort() const = 0;

    /*
     * 获取套接字的远端ip地址
     *
     * 对于tcp, 返回连接对端的ip地址;
     * 对于udp, 返回默认的远端ip地址
     */
    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * 获取套接字的远端端口号
     *
     * 对于tcp, 返回连接对端的端口号;
     * 对于udp, 返回默认的远端端口号
     */
    virtual unsigned short PRO_CALLTYPE GetRemotePort() const = 0;

    /*
     * 获取接收池
     *
     * 参见IProRecvPool的注释
     */
    virtual IProRecvPool* PRO_CALLTYPE GetRecvPool() = 0;

    /*
     * 发送数据
     *
     * 对于tcp, 数据将放到发送池里,忽略remoteAddr参数;
     * 对于udp, 数据将直接发送,如果remoteAddr参数无效,则使用默认的远端地址;
     * actionId是上层分配的一个值,用于标识这次发送动作, OnSend(...)回调时
     * 会带回该值
     *
     * 如果返回false, 表示发送忙,上层应该缓冲数据以待OnSend(...)回调拉取
     */
    virtual bool PRO_CALLTYPE SendData(
        const void*             buf,
        size_t                  size,
        PRO_UINT64              actionId   = 0,
        const pbsd_sockaddr_in* remoteAddr = NULL /* for udp */
        ) = 0;

    /*
     * 请求回调一个OnSend事件
     *
     * 网络发送能力缓解时, OnSend(...)回调才会过来
     */
    virtual void PRO_CALLTYPE RequestOnSend() = 0;

    /*
     * 挂起接收能力
     */
    virtual void PRO_CALLTYPE SuspendRecv() = 0;

    /*
     * 恢复接收能力
     */
    virtual void PRO_CALLTYPE ResumeRecv() = 0;

    /*
     * 添加额外的多播地址(for CProMcastTransport only)
     */
    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * 删除额外的多播地址(for CProMcastTransport only)
     */
    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * 启动心跳定时器
     *
     * 心跳事件发生时, OnHeartbeat(...)将被回调
     */
    virtual void PRO_CALLTYPE StartHeartbeat() = 0;

    /*
     * 停止心跳定时器
     */
    virtual void PRO_CALLTYPE StopHeartbeat() = 0;
};

/*
 * 传输器回调目标
 *
 * 使用者需要实现该接口
 */
class IProTransportObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * 数据抵达套接字的接收缓冲区时,该函数将被回调
     *
     * 对于tcp/ssl传输器,使用环型接收池.
     * 由于tcp是流式工作的,收端动力数与发端动力数不一定相同,所以,在OnRecv(...)
     * 里应该尽量取走接收池内的数据. 如果没有新的数据到来,反应器不会再次报告接收
     * 池内还有剩余数据这件事
     *
     * 对于udp/mcast传输器,使用线性接收池.
     * 为防止接收池空间不足导致EMSGSIZE错误,在OnRecv(...)里应该收完全部数据
     *
     * 另外,当反应器报告套接字内有数据可读时,如果接收池已经<<满了>>,那么该套接字
     * 将被关闭!!! 这说明收端和发端的配合逻辑有问题!!!
     */
    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        ) = 0;

    /*
     * 数据被成功送入套接字的发送缓冲区时,或上层调用过RequestOnSend(...),
     * 该函数将被回调
     *
     * 如果回调是RequestOnSend(...)触发的,则actionId为0
     */
    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        ) = 0;

    /*
     * 套接字出现错误时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode, /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
        long           sslCode    /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * 心跳事件发生时,该函数将被回调
     */
    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 功能: 初始化网络库
 *
 * 参数: 无
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProNetInit();

/*
 * 功能: 获取该库的版本号
 *
 * 参数:
 * major : 主版本号
 * minor : 次版本号
 * patch : 补丁号
 *
 * 返回值: 无
 *
 * 说明: 该函数格式恒定
 */
PRO_NET_API
void
PRO_CALLTYPE
ProNetVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * 功能: 创建一个反应器
 *
 * 参数:
 * ioThreadCount    : 处理收发事件的线程数
 * ioThreadPriority : 收发线程的优先级(0/1/2)
 *
 * 返回值: 反应器对象或NULL
 *
 * 说明: ioThreadPriority可能对一些特殊的应用场景有用处. 比如多媒体通信
 *       中,可以将视频链路和音频链路放到不同的reactor中,并将音频reactor
 *       的ioThreadPriority略微调高,这样在数据密集时可以改善音频的处理
 */
PRO_NET_API
IProReactor*
PRO_CALLTYPE
ProCreateReactor(unsigned long ioThreadCount,
                 long          ioThreadPriority = 0);

/*
 * 功能: 删除一个反应器
 *
 * 参数:
 * reactor : 反应器对象
 *
 * 返回值: 无
 *
 * 说明: 反应器是主动对象,内部包含回调线程池. 该函数将保持阻塞,直到所有的
 *       回调线程都结束运行. 上层要确保删除动作由回调线程池之外的线程发起,
 *       并在锁外执行,否则会发生死锁
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteReactor(IProReactor* reactor);

/*
 * 功能: 创建一个接受器
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localIp   : 要监听的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要监听的本地端口号. 如果为0, 系统将随机分配一个
 *
 * 返回值: 接受器对象或NULL
 *
 * 说明: 可以使用ProGetAcceptorPort(...)获取实际的端口号
 */
PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp   = NULL,
                  unsigned short        localPort = 0);

/*
 * 功能: 创建一个扩展协议接受器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localIp          : 要监听的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort        : 要监听的本地端口号. 如果为0, 系统将随机分配一个
 * timeoutInSeconds : 握手超时. 默认10秒
 *
 * 返回值: 接受器对象或NULL
 *
 * 说明: 可以使用ProGetAcceptorPort(...)获取实际的端口号
 *
 *       扩展协议握手期间,服务id用于服务端在握手初期识别客户端.
 *       服务端发送nonce给客户端,客户端发送(serviceId, serviceOpt)给服务端,
 *       服务端根据客户端请求的服务id, 将该连接派发给对应的处理者或服务进程
 */
PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp          = NULL,
                    unsigned short        localPort        = 0,
                    unsigned long         timeoutInSeconds = 0);

/*
 * 功能: 获取接受器监听的端口号
 *
 * 参数:
 * acceptor : 接受器对象
 *
 * 返回值: 端口号
 *
 * 说明: 主要用于随机监听的端口号的获取
 */
PRO_NET_API
unsigned short
PRO_CALLTYPE
ProGetAcceptorPort(IProAcceptor* acceptor);

/*
 * 功能: 删除一个接受器
 *
 * 参数:
 * acceptor : 接受器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteAcceptor(IProAcceptor* acceptor);

/*
 * 功能: 创建一个连接器
 *
 * 参数:
 * enableUnixSocket : 是否允许unix套接字. 仅用于Linux家族的127.0.0.1连接
 * observer         : 回调目标
 * reactor          : 反应器
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * localBindIp      : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 连接超时. 默认20秒
 *
 * 返回值: 连接器对象或NULL
 *
 * 说明: 无
 */
PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp      = NULL,
                   unsigned long          timeoutInSeconds = 0);

/*
 * 功能: 创建一个扩展协议连接器
 *
 * 参数:
 * enableUnixSocket : 是否允许unix套接字. 仅用于Linux家族的127.0.0.1连接
 * serviceId        : 服务id
 * serviceOpt       : 服务选项
 * observer         : 回调目标
 * reactor          : 反应器
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * localBindIp      : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 连接超时. 默认20秒
 *
 * 返回值: 连接器对象或NULL
 *
 * 说明: 扩展协议握手期间,服务id用于服务端在握手初期识别客户端.
 *       服务端发送nonce给客户端,客户端发送(serviceId, serviceOpt)给服务端,
 *       服务端根据客户端请求的服务id, 将该连接派发给对应的处理者或服务进程
 */
PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp      = NULL,
                     unsigned long          timeoutInSeconds = 0);

/*
 * 功能: 删除一个连接器
 *
 * 参数:
 * connector : 连接器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteConnector(IProConnector* connector);

/*
 * 功能: 创建一个tcp握手器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * sockId           : 套接字id. 来源于OnAccept(...)或OnConnectOk(...)
 * unixSocket       : 是否unix套接字
 * sendData         : 希望发送的数据
 * sendDataSize     : 希望发送的数据长度
 * recvDataSize     : 希望接收的数据长度
 * recvFirst        : 接收优先
 * timeoutInSeconds : 握手超时. 默认20秒
 *
 * 返回值: 握手器对象或NULL
 *
 * 说明: 如果recvFirst为true, 则握手数据先接收后发送
 */
PRO_NET_API
IProTcpHandshaker*
PRO_CALLTYPE
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned long              timeoutInSeconds = 0);

/*
 * 功能: 删除一个tcp握手器
 *
 * 参数:
 * handshaker : 握手器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker);

/*
 * 功能: 创建一个ssl握手器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * ctx              : ssl上下文. 通过ProSslCtx_Creates(...)或ProSslCtx_Createc(...)创建
 * sockId           : 套接字id. 来源于OnAccept(...)或OnConnectOk(...)
 * unixSocket       : 是否unix套接字
 * sendData         : 希望发送的数据
 * sendDataSize     : 希望发送的数据长度
 * recvDataSize     : 希望接收的数据长度
 * recvFirst        : 接收优先
 * timeoutInSeconds : 握手超时. 默认20秒
 *
 * 返回值: 握手器对象或NULL
 *
 * 说明: 如果ctx尚未完成ssl/tls协议的握手过程,则握手器首先完成ssl/tls
 *       协议的握手过程,在此基础上进一步执行收发用户数据的高层握手动作.
 *       如果recvFirst为true, 则高层握手数据先接收后发送
 */
PRO_NET_API
IProSslHandshaker*
PRO_CALLTYPE
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned long              timeoutInSeconds = 0);

/*
 * 功能: 删除一个ssl握手器
 *
 * 参数:
 * handshaker : 握手器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteSslHandshaker(IProSslHandshaker* handshaker);

/*
 * 功能: 创建一个tcp传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * sockId          : 套接字id. 来源于OnAccept(...)或OnConnectOk(...)
 * unixSocket      : 是否unix套接字
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认(1024 * 56)
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认(1024 * 8)
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 无
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0);

/*
 * 功能: 创建一个udp传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * localIp         : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort       : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认(1024 * 56)
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认(1024 * 56)
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 * remoteIp        : 默认的远端ip地址或域名
 * remotePort      : 默认的远端端口号
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 可以使用IProTransport::GetLocalPort(...)获取实际的端口号
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      const char*            localIp           = NULL,
                      unsigned short         localPort         = 0,
                      size_t                 sockBufSizeRecv   = 0,
                      size_t                 sockBufSizeSend   = 0,
                      size_t                 recvPoolSize      = 0,
                      const char*            defaultRemoteIp   = NULL,
                      unsigned short         defaultRemotePort = 0);

/*
 * 功能: 创建一个多播传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * mcastIp         : 要绑定的多播地址
 * mcastPort       : 要绑定的多播端口号. 如果为0, 系统将随机分配一个
 * localBindIp     : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认(1024 * 56)
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认(1024 * 56)
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 合法的多播地址为[224.0.0.0 ~ 239.255.255.255],
 *       推荐的多播地址为[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       可以使用IProTransport::GetLocalPort(...)获取实际的端口号
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateMcastTransport(IProTransportObserver* observer,
                        IProReactor*           reactor,
                        const char*            mcastIp,
                        unsigned short         mcastPort       = 0,
                        const char*            localBindIp     = NULL,
                        size_t                 sockBufSizeRecv = 0,
                        size_t                 sockBufSizeSend = 0,
                        size_t                 recvPoolSize    = 0);

/*
 * 功能: 创建一个ssl传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * ctx             : ssl上下文. 来自于IProSslHandshaker的握手结果回调
 * sockId          : 套接字id. 来源于OnAccept(...)或OnConnectOk(...)
 * unixSocket      : 是否unix套接字
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认(1024 * 56)
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认(1024 * 8)
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 这里是安全访问ctx的最后时刻!!! 此后, ctx将交由IProTransport管理
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0);

/*
 * 功能: 删除一个传输器
 *
 * 参数:
 * trans : 传输器对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTransport(IProTransport* trans);

/*
 * 功能: 创建一个服务hub
 *
 * 参数:
 * reactor          : 反应器
 * servicePort      : 服务端口号
 * timeoutInSeconds : 握手超时. 默认10秒
 *
 * 返回值: 服务hub对象或NULL
 *
 * 说明: 服务hub可以将不同服务id的连接请求派发给对应的服务host,
 *       并允许跨越进程边界(WinCE不能跨进程)!!!
 *       服务hub与服务host配合,可以将接受器的功能延伸到不同的位置
 */
PRO_NET_API
IProServiceHub*
PRO_CALLTYPE
ProCreateServiceHub(IProReactor*   reactor,
                    unsigned short servicePort,
                    unsigned long  timeoutInSeconds = 0);

/*
 * 功能: 删除一个服务hub
 *
 * 参数:
 * hub : 服务hub对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHub(IProServiceHub* hub);

/*
 * 功能: 创建一个服务host
 *
 * 参数:
 * observer    : 回调目标
 * reactor     : 反应器
 * servicePort : 服务端口号
 * serviceId   : 服务id. 0无效
 *
 * 返回值: 服务host对象或NULL
 *
 * 说明: 服务host可以看作是一个特定服务id的接受器
 */
PRO_NET_API
IProServiceHost*
PRO_CALLTYPE
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort,
                     unsigned char            serviceId);

/*
 * 功能: 删除一个服务host
 *
 * 参数:
 * host : 服务host对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHost(IProServiceHost* host);

/*
 * 功能: 打开一个tcp套接字
 *
 * 参数:
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要绑定的本地端口号
 *
 * 返回值: 套接字id或-1
 *
 * 说明: 主要用于保留rtcp端口
 */
PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenTcpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * 功能: 打开一个udp套接字
 *
 * 参数:
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要绑定的本地端口号
 *
 * 返回值: 套接字id或-1
 *
 * 说明: 主要用于保留rtcp端口
 */
PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenUdpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * 功能: 关闭一个套接字
 *
 * 参数:
 * sockId : 套接字id
 * linger : 是否拖延关闭
 *
 * 返回值: 无
 *
 * 说明: OnAccept(...)或OnConnectOk(...)会产生套接字,当产生的套接字没有成功
 *       包装成IProTransport时,应该使用该函数释放sockId对应的套接字资源
 */
PRO_NET_API
void
PRO_CALLTYPE
ProCloseSockId(PRO_INT64 sockId,
               bool      linger = false);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____PRO_NET_H____ */
