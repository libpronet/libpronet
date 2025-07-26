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
 *        |                                   |                  |
 *        |               ProNet              |                  |
 *        |___________________________________|                  |
 *        |                    |                     ProUtil     |
 *        |                    |                                 |
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
 * 1)  client ----->                 connect()                  -----> server
 * 2)  client <-----                  accept()                  <----- server
 * 3a) client <-----                 PRO_NONCE                  <----- server
 * 3b) client ----->    serviceId + serviceOpt + (r) + (r+1)    -----> server
 * 4]  client <<====              [ssl handshake]               ====>> server
 *          Fig.4 acceptor_ex/connector_ex handshake protocol flow chart
 */

#ifndef ____PRO_NET_H____
#define ____PRO_NET_H____

#include "pro_a.h"
#include "pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_NET_EXPORTS)
#if defined(_MSC_VER)
#define PRO_NET_API /* using xxx.def */
#elif defined(__MINGW32__) || defined(__CYGWIN__)
#define PRO_NET_API __declspec(dllexport)
#else
#define PRO_NET_API __attribute__((visibility("default")))
#endif
#else
#define PRO_NET_API
#endif

#include "pro_ssl.h"

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

/*
 * 会话随机数
 */
struct PRO_NONCE
{
    unsigned char nonce[32];
};

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

    virtual ~IProOnTimer() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        ) = 0;
};
#endif /* ____IProOnTimer____ */

/*
 * 反应器
 *
 * 这里只暴露了定时器相关的接口, 目的是为了在使用反应器的时候, 可以方便地使用其内部的定时器,
 * 免得还要创建额外的CProTimerFactory对象
 */
class IProReactor
{
public:

    virtual ~IProReactor() {}

    /*
     * 创建一个普通定时器
     *
     * 返回值为定时器id. 0无效
     */
    virtual uint64_t SetupTimer(
        IProOnTimer* onTimer,
        uint64_t     firstDelay, /* 首次触发延迟(ms) */
        uint64_t     period,     /* 定时周期(ms). 如果为0, 则该定时器只触发一次 */
        int64_t      userData = 0
        ) = 0;

    /*
     * 创建一个用于链路心跳的普通定时器(心跳定时器)
     *
     * 各个心跳定时器的心跳时间点由内部算法决定, 内部会进行均匀化处理. 心跳事件发生时, 上层
     * 可以在OnTimer()回调里发送心跳数据包
     *
     * 返回值为定时器id. 0无效
     */
    virtual uint64_t SetupHeartbeatTimer(
        IProOnTimer* onTimer,
        int64_t      userData = 0
        ) = 0;

    /*
     * 更新全体心跳定时器的心跳周期
     *
     * 默认的心跳周期为20秒
     */
    virtual bool UpdateHeartbeatTimers(unsigned int htbtIntervalInSeconds) = 0;

    /*
     * 删除一个普通定时器
     */
    virtual void CancelTimer(uint64_t timerId) = 0;

    /*
     * 创建一个多媒体定时器(高精度定时器)
     *
     * 返回值为定时器id. 0无效
     */
    virtual uint64_t SetupMmTimer(
        IProOnTimer* onTimer,
        uint64_t     firstDelay, /* 首次触发延迟(ms) */
        uint64_t     period,     /* 定时周期(ms). 如果为0, 则该定时器只触发一次 */
        int64_t      userData = 0
        ) = 0;

    /*
     * 删除一个多媒体定时器(高精度定时器)
     */
    virtual void CancelMmTimer(uint64_t timerId) = 0;

    /*
     * 获取状态信息字符串
     */
    virtual void GetTraceInfo(
        char*  buf,
        size_t size
        ) const = 0;
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

    virtual ~IProAcceptorObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 有原始连接进入时, 该函数将被回调
     *
     * 该回调适用于ProCreateAcceptor()创建的接受器. 使用者负责sockId的资源维护
     */
    virtual void OnAccept(
        IProAcceptor*  acceptor,
        int64_t        sockId,     /* 套接字id */
        bool           unixSocket, /* 是否unix套接字 */
        const char*    localIp,    /* 本地的ip地址. != NULL */
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort  /* 远端的端口号. > 0 */
        ) = 0;

    /*
     * 有扩展协议连接进入时, 该函数将被回调
     *
     * 该回调适用于ProCreateAcceptorEx()创建的接受器. 使用者负责sockId的资源维护
     */
    virtual void OnAccept(
        IProAcceptor*    acceptor,
        int64_t          sockId,     /* 套接字id */
        bool             unixSocket, /* 是否unix套接字 */
        const char*      localIp,    /* 本地的ip地址. != NULL */
        const char*      remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short   remotePort, /* 远端的端口号. > 0 */
        unsigned char    serviceId,  /* 远端请求的服务id */
        unsigned char    serviceOpt, /* 远端请求的服务选项 */
        const PRO_NONCE* nonce       /* 会话随机数 */
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

    virtual ~IProServiceHostObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 有原始连接进入时, 该函数将被回调
     *
     * 该回调适用于ProCreateServiceHost()创建的服务host. 使用者负责sockId的资源维护
     */
    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        int64_t          sockId,     /* 套接字id */
        bool             unixSocket, /* 是否unix套接字 */
        const char*      localIp,    /* 本地的ip地址. != NULL */
        const char*      remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short   remotePort  /* 远端的端口号. > 0 */
        ) = 0;

    /*
     * 有扩展协议连接进入时, 该函数将被回调
     *
     * 该回调适用于ProCreateServiceHostEx()创建的服务host. 使用者负责sockId的资源维护
     */
    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        int64_t          sockId,     /* 套接字id */
        bool             unixSocket, /* 是否unix套接字 */
        const char*      localIp,    /* 本地的ip地址. != NULL */
        const char*      remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short   remotePort, /* 远端的端口号. > 0 */
        unsigned char    serviceId,  /* 远端请求的服务id */
        unsigned char    serviceOpt, /* 远端请求的服务选项 */
        const PRO_NONCE* nonce       /* 会话随机数. != NULL */
        ) = 0;
};

/*
 * 服务hub回调目标
 *
 * 使用者需要实现该接口
 */
class IProServiceHubObserver
{
public:

    virtual ~IProServiceHubObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 服务host上线时, 该函数将被回调
     */
    virtual void OnServiceHostConnected(
        IProServiceHub* serviceHub,
        unsigned short  servicePort,  /* 服务端口号. > 0 */
        unsigned char   serviceId,    /* 服务id. > 0 */
        uint64_t        hostProcessId /* 服务host的进程id */
        ) = 0;

    /*
     * 服务host下线时, 该函数将被回调
     */
    virtual void OnServiceHostDisconnected(
        IProServiceHub* serviceHub,
        unsigned short  servicePort,   /* 服务端口号. > 0 */
        unsigned char   serviceId,     /* 服务id. > 0 */
        uint64_t        hostProcessId, /* 服务host的进程id */
        bool            timeout        /* 是否超时 */
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

    virtual ~IProConnectorObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 原始连接成功时, 该函数将被回调
     *
     * 该回调适用于ProCreateConnector()创建的连接器. 使用者负责sockId的资源维护
     */
    virtual void OnConnectOk(
        IProConnector* connector,
        int64_t        sockId,     /* 套接字id */
        bool           unixSocket, /* 是否unix套接字 */
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort  /* 远端的端口号. > 0 */
        ) = 0;

    /*
     * 原始连接出错或超时时, 该函数将被回调
     *
     * 该回调适用于ProCreateConnector()创建的连接器
     */
    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort, /* 远端的端口号. > 0 */
        bool           timeout     /* 是否连接超时 */
        ) = 0;

    /*
     * 扩展协议连接成功时, 该函数将被回调
     *
     * 该回调适用于ProCreateConnectorEx()创建的连接器. 使用者负责sockId的资源维护
     */
    virtual void OnConnectOk(
        IProConnector*   connector,
        int64_t          sockId,     /* 套接字id */
        bool             unixSocket, /* 是否unix套接字 */
        const char*      remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short   remotePort, /* 远端的端口号. > 0 */
        unsigned char    serviceId,  /* 请求的服务id */
        unsigned char    serviceOpt, /* 请求的服务选项 */
        const PRO_NONCE* nonce       /* 会话随机数 */
        ) = 0;

    /*
     * 扩展协议连接出错或超时时, 该函数将被回调
     *
     * 该回调适用于ProCreateConnectorEx()创建的连接器
     */
    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* 远端的ip地址. != NULL */
        unsigned short remotePort, /* 远端的端口号. > 0 */
        unsigned char  serviceId,  /* 请求的服务id */
        unsigned char  serviceOpt, /* 请求的服务选项 */
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

    virtual ~IProTcpHandshakerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 握手成功时, 该函数将被回调
     *
     * 使用者负责sockId的资源维护. 握手完成后, 上层应该把sockId包装成IProTransport,
     * 或者释放sockId对应的资源
     */
    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        int64_t            sockId,     /* 套接字id */
        bool               unixSocket, /* 是否unix套接字 */
        const void*        buf,        /* 握手数据. 可以是NULL */
        size_t             size        /* 数据长度. 可以是0 */
        ) = 0;

    /*
     * 握手出错或超时时, 该函数将被回调
     */
    virtual void OnHandshakeError(
        IProTcpHandshaker* handshaker,
        int                errorCode /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
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

    virtual ~IProSslHandshakerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 握手成功时, 该函数将被回调
     *
     * 使用者负责(ctx, sockId)的资源维护. 握手完成后, 上层应该把(ctx, sockId)包装成
     * IProTransport, 或者释放(ctx, sockId)对应的资源
     *
     * 调用ProSslCtx_GetSuite()可以获知当前使用的加密套件
     */
    virtual void OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,        /* ssl上下文 */
        int64_t            sockId,     /* 套接字id */
        bool               unixSocket, /* 是否unix套接字 */
        const void*        buf,        /* 握手数据. 可以是NULL */
        size_t             size        /* 数据长度. 可以是0 */
        ) = 0;

    /*
     * 握手出错或超时时, 该函数将被回调
     */
    virtual void OnHandshakeError(
        IProSslHandshaker* handshaker,
        int                errorCode, /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
        int                sslCode    /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 接收池
 *
 * 必须在IProTransportObserver::OnRecv()的线程上下文里使用, 否则不安全
 *
 * 对于tcp/ssl传输器, 使用环型接收池.
 * 由于tcp是流式工作的, 收端动力数与发端动力数不一定相同, 所以, 在OnRecv()里应该尽量取走
 * 接收池内的数据. 如果没有新的数据到来, 反应器不会再次报告接收池内还有剩余数据这件事
 *
 * 对于udp/mcast传输器, 使用线性接收池.
 * 为防止接收池空间不足导致EMSGSIZE错误, 在OnRecv()里应该收完全部数据
 *
 * 当反应器报告套接字内有数据可读时, 如果接收池已经<<满了>>, 那么该套接字将被关闭!!! 这说明
 * 收端和发端的配合逻辑有问题!!!
 */
class IProRecvPool
{
public:

    virtual ~IProRecvPool() {}

    /*
     * 查询接收池内的数据量
     */
    virtual size_t PeekDataSize() const = 0;

    /*
     * 读取接收池内的数据
     */
    virtual void PeekData(
        void*  buf,
        size_t size
        ) const = 0;

    /*
     * 刷掉已经读取的数据
     *
     * 腾出空间, 以便容纳新的数据
     */
    virtual void Flush(size_t size) = 0;

    /*
     * 查询接收池内剩余的存储空间
     */
    virtual size_t GetFreeSize() const = 0;
};

/*
 * 传输器
 */
class IProTransport
{
public:

    virtual ~IProTransport() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 获取传输器类型
     */
    virtual PRO_TRANS_TYPE GetType() const = 0;

    /*
     * 获取加密套件
     *
     * 仅用于PRO_TRANS_SSL类型的传输器
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const = 0;

    /*
     * 获取底层的套接字id
     *
     * 如非必需, 最好不要直接操作底层的套接字
     */
    virtual int64_t GetSockId() const = 0;

    /*
     * 获取套接字的本地ip地址
     */
    virtual const char* GetLocalIp(char localIp[64]) const = 0;

    /*
     * 获取套接字的本地端口号
     */
    virtual unsigned short GetLocalPort() const = 0;

    /*
     * 获取套接字的远端ip地址
     *
     * 对于tcp, 返回连接对端的ip地址;
     * 对于udp, 返回默认的远端ip地址
     */
    virtual const char* GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * 获取套接字的远端端口号
     *
     * 对于tcp, 返回连接对端的端口号;
     * 对于udp, 返回默认的远端端口号
     */
    virtual unsigned short GetRemotePort() const = 0;

    /*
     * 获取接收池
     *
     * 参见IProRecvPool的注释
     */
    virtual IProRecvPool* GetRecvPool() = 0;

    /*
     * 发送数据
     *
     * 对于tcp, 数据将放到发送池里, 忽略remoteAddr参数. actionId是上层分配的一个值, 用于
     * 标识这次发送动作, OnSend()回调时会带回该值;
     * 对于udp, 数据将直接发送. 如果remoteAddr参数无效, 则使用默认的远端地址
     *
     * 如果返回false, 表示发送忙, 上层应该缓冲数据以待OnSend()回调拉取
     */
    virtual bool SendData(
        const void*             buf,
        size_t                  size,
        uint64_t                actionId   = 0,   /* for tcp */
        const pbsd_sockaddr_in* remoteAddr = NULL /* for udp */
        ) = 0;

    /*
     * 请求回调一个OnSend事件(for CProTcpTransport & CProSslTransport)
     *
     * 网络发送能力缓解时, OnSend()回调才会过来
     */
    virtual void RequestOnSend() = 0;

    /*
     * 挂起接收能力
     */
    virtual void SuspendRecv() = 0;

    /*
     * 恢复接收能力
     */
    virtual void ResumeRecv() = 0;

    /*
     * 添加额外的多播地址(for CProMcastTransport only)
     */
    virtual bool AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * 删除额外的多播地址(for CProMcastTransport only)
     */
    virtual void RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * 启动心跳定时器
     *
     * 心跳事件发生时, OnHeartbeat()将被回调
     */
    virtual void StartHeartbeat() = 0;

    /*
     * 停止心跳定时器
     */
    virtual void StopHeartbeat() = 0;

    /*
     * udp数据不可达时, 作为错误对待(for CProUdpTransport only)
     *
     * 默认false. 设置之后不可逆
     */
    virtual void UdpConnResetAsError(const pbsd_sockaddr_in* remoteAddr = NULL) = 0;
};

/*
 * 传输器回调目标
 *
 * 使用者需要实现该接口
 */
class IProTransportObserver
{
public:

    virtual ~IProTransportObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * 数据抵达套接字的接收缓冲区时, 该函数将被回调
     *
     * 对于tcp/ssl传输器, 使用环型接收池.
     * 由于tcp是流式工作的, 收端动力数与发端动力数不一定相同, 所以, 在OnRecv()里应该尽量
     * 取走接收池内的数据. 如果没有新的数据到来, 反应器不会再次报告接收池内还有剩余数据这件事
     *
     * 对于udp/mcast传输器, 使用线性接收池.
     * 为防止接收池空间不足导致EMSGSIZE错误, 在OnRecv()里应该收完全部数据
     *
     * 当反应器报告套接字内有数据可读时, 如果接收池已经<<满了>>, 那么该套接字将被关闭!!! 这
     * 说明收端和发端的配合逻辑有问题!!!
     */
    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        ) = 0;

    /*
     * 数据被成功送入套接字的发送缓冲区时, 或上层调用过RequestOnSend(), 该函数将被回调
     *
     * 如果回调由RequestOnSend()触发, 则actionId为0
     */
    virtual void OnSend(
        IProTransport* trans,
        uint64_t       actionId
        ) = 0;

    /*
     * 套接字出现错误时, 该函数将被回调
     */
    virtual void OnClose(
        IProTransport* trans,
        int            errorCode, /* 系统错误码. 参见"pro_util/pro_bsd_wrapper.h" */
        int            sslCode    /* ssl错误码. 参见"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * 心跳事件发生时, 该函数将被回调
     */
    virtual void OnHeartbeat(IProTransport* trans) = 0;
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
ProNetVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * 功能: 创建一个反应器
 *
 * 参数:
 * ioThreadCount : 处理收发事件的线程数
 *
 * 返回值: 反应器对象或NULL
 *
 * 说明: 无
 */
PRO_NET_API
IProReactor*
ProCreateReactor(unsigned int ioThreadCount);

/*
 * 功能: 删除一个反应器
 *
 * 参数:
 * reactor : 反应器对象
 *
 * 返回值: 无
 *
 * 说明: 反应器是主动对象, 内部包含回调线程池. 该函数将保持阻塞, 直到所有的回调线程都结束运行.
 *      上层要确保删除动作由回调线程池之外的线程发起, 并在锁外执行, 否则会发生死锁
 */
PRO_NET_API
void
ProDeleteReactor(IProReactor* reactor);

/*
 * 功能: 创建一个原始的接受器
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localIp   : 要监听的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要监听的本地端口号. 如果为0, 系统将随机分配一个
 *
 * 返回值: 接受器对象或NULL
 *
 * 说明: 可以使用ProGetAcceptorPort()获取实际的端口号
 */
PRO_NET_API
IProAcceptor*
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp   = NULL,
                  unsigned short        localPort = 0);

/*
 * 功能: 创建一个扩展协议的接受器
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
 * 说明: 可以使用ProGetAcceptorPort()获取实际的端口号
 *
 *      扩展协议握手期间, 服务id用于服务端在握手初期识别客户端. 服务端发送nonce给客户端,
 *      客户端发送(serviceId, serviceOpt)给服务端, 服务端根据客户端请求的服务id, 将该
 *      连接派发给对应的处理者或服务进程
 */
PRO_NET_API
IProAcceptor*
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp          = NULL,
                    unsigned short        localPort        = 0,
                    unsigned int          timeoutInSeconds = 0);

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
ProDeleteAcceptor(IProAcceptor* acceptor);

/*
 * 功能: 创建一个原始的连接器
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
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp      = NULL,
                   unsigned int           timeoutInSeconds = 0);

/*
 * 功能: 创建一个扩展协议的连接器
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
 * 说明: 扩展协议握手期间, 服务id用于服务端在握手初期识别客户端. 服务端发送nonce给客户端,
 *      客户端发送(serviceId, serviceOpt)给服务端, 服务端根据客户端请求的服务id, 将该
 *      连接派发给对应的处理者或服务进程
 */
PRO_NET_API
IProConnector*
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp      = NULL,
                     unsigned int           timeoutInSeconds = 0);

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
ProDeleteConnector(IProConnector* connector);

/*
 * 功能: 创建一个tcp握手器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * sockId           : 套接字id. 来源于OnAccept()或OnConnectOk()
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
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned int               timeoutInSeconds = 0);

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
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker);

/*
 * 功能: 创建一个ssl握手器
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * ctx              : ssl上下文. 通过ProSslCtx_Creates()或ProSslCtx_Createc()创建
 * sockId           : 套接字id. 来源于OnAccept()或OnConnectOk()
 * unixSocket       : 是否unix套接字
 * sendData         : 希望发送的数据
 * sendDataSize     : 希望发送的数据长度
 * recvDataSize     : 希望接收的数据长度
 * recvFirst        : 接收优先
 * timeoutInSeconds : 握手超时. 默认20秒
 *
 * 返回值: 握手器对象或NULL
 *
 * 说明: 如果ctx尚未完成ssl/tls协议的握手过程, 则握手器首先执行ssl/tls协议的握手, 在此基础
 *      上进一步执行收发用户数据的高层握手. 如果recvFirst为true, 则高层握手数据先接收后发送
 */
PRO_NET_API
IProSslHandshaker*
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned int               timeoutInSeconds = 0);

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
ProDeleteSslHandshaker(IProSslHandshaker* handshaker);

/*
 * 功能: 创建一个tcp传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * sockId          : 套接字id. 来源于OnAccept()或OnConnectOk()
 * unixSocket      : 是否unix套接字
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认auto
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认auto
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 * suspendRecv     : 是否挂起接收能力
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: suspendRecv用于一些需要精确控制时序的场景
 */
PRO_NET_API
IProTransport*
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0,
                      bool                   suspendRecv     = false);

/*
 * 功能: 创建一个udp传输器
 *
 * 参数:
 * observer        : 回调目标
 * reactor         : 反应器
 * bindToLocal     : 是否绑定套接字到本地地址
 * localIp         : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort       : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认auto
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认auto
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 * remoteIp        : 默认的远端ip地址或域名
 * remotePort      : 默认的远端端口号
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 可以使用IProTransport::GetLocalPort()获取实际的端口号
 */
PRO_NET_API
IProTransport*
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      bool                   bindToLocal       = false,
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
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认auto
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认auto
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 合法的多播地址为[224.0.0.0 ~ 239.255.255.255],
 *      推荐的多播地址为[224.0.1.0 ~ 238.255.255.255],
 *      RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *      可以使用IProTransport::GetLocalPort()获取实际的端口号
 */
PRO_NET_API
IProTransport*
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
 * sockId          : 套接字id. 来源于OnAccept()或OnConnectOk()
 * unixSocket      : 是否unix套接字
 * sockBufSizeRecv : 套接字的系统接收缓冲区字节数. 默认auto
 * sockBufSizeSend : 套接字的系统发送缓冲区字节数. 默认auto
 * recvPoolSize    : 接收池的字节数. 默认(1024 * 65)
 * suspendRecv     : 是否挂起接收能力
 *
 * 返回值: 传输器对象或NULL
 *
 * 说明: 这里是安全访问ctx的最后时刻!!! 此后, ctx将交由IProTransport管理
 *
 *      suspendRecv用于一些需要精确控制时序的场景
 */
PRO_NET_API
IProTransport*
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0,
                      bool                   suspendRecv     = false);

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
ProDeleteTransport(IProTransport* trans);

/*
 * 功能: 创建一个原始服务hub
 *
 * 参数:
 * observer          : 回调目标
 * reactor           : 反应器
 * servicePort       : 服务端口号. 该端口工作在原始模式
 * enableLoadBalance : 该端口是否开启负载均衡. 如果开启, 该端口支持多个服务host
 *
 * 返回值: 服务hub对象或NULL
 *
 * 说明: 服务hub可以将连接请求派发给对应的一个或多个服务host, 并允许跨越进程边界(WinCE不能跨
 *      进程). 服务hub与服务host配合, 可以将接受器的功能"延伸"到不同的位置
 *
 *      注意, 对原始服务hub来说, 回环地址(127.0.0.1)专用于IPC通道, 不能对外提供服务; 扩展
 *      协议的服务hub无此限制
 */
PRO_NET_API
IProServiceHub*
ProCreateServiceHub(IProServiceHubObserver* observer,
                    IProReactor*            reactor,
                    unsigned short          servicePort,
                    bool                    enableLoadBalance = false);

/*
 * 功能: 创建一个扩展协议的服务hub
 *
 * 参数:
 * observer          : 回调目标
 * reactor           : 反应器
 * servicePort       : 服务端口号. 该端口工作在扩展协议模式
 * enableLoadBalance : 该端口是否开启负载均衡. 如果开启, 该端口支持多个服务host
 * timeoutInSeconds  : 握手超时. 默认10秒
 *
 * 返回值: 服务hub对象或NULL
 *
 * 说明: 服务hub可以将连接请求派发给对应的一个或多个服务host, 并允许跨越进程边界(WinCE不能跨
 *      进程). 服务hub与服务host配合, 可以将接受器的功能"延伸"到不同的位置
 *
 *      注意, 对原始服务hub来说, 回环地址(127.0.0.1)专用于IPC通道, 不能对外提供服务; 扩展
 *      协议的服务hub无此限制
 */
PRO_NET_API
IProServiceHub*
ProCreateServiceHubEx(IProServiceHubObserver* observer,
                      IProReactor*            reactor,
                      unsigned short          servicePort,
                      bool                    enableLoadBalance = false,
                      unsigned int            timeoutInSeconds  = 0);

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
ProDeleteServiceHub(IProServiceHub* hub);

/*
 * 功能: 创建一个原始服务host
 *
 * 参数:
 * observer    : 回调目标
 * reactor     : 反应器
 * servicePort : 服务端口号
 *
 * 返回值: 服务host对象或NULL
 *
 * 说明: 服务host可以看作是一个服务接受器
 */
PRO_NET_API
IProServiceHost*
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort);

/*
 * 功能: 创建一个扩展协议的服务host
 *
 * 参数:
 * observer    : 回调目标
 * reactor     : 反应器
 * servicePort : 服务端口号
 * serviceId   : 服务id. 0无效
 *
 * 返回值: 服务host对象或NULL
 *
 * 说明: 服务host可以看作是一个服务接受器
 */
PRO_NET_API
IProServiceHost*
ProCreateServiceHostEx(IProServiceHostObserver* observer,
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
int64_t
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
int64_t
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
 * 说明: OnAccept()或OnConnectOk()会产生套接字, 当产生的套接字没有包装成IProTransport
*       时, 应该使用该函数释放sockId对应的套接字资源
 */
PRO_NET_API
void
ProCloseSockId(int64_t sockId,
               bool    linger = false);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_NET_H____ */
