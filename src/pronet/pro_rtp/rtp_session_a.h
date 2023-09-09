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

#if !defined(RTP_SESSION_A_H)
#define RTP_SESSION_A_H

#include "rtp_base.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 功能: 创建一个RTP_ST_UDPCLIENT类型的会话
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localInfo : 会话信息
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 可以使用IRtpSession::GetLocalPort(...)获取本地端口号
 */
IRtpSession*
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * 功能: 创建一个RTP_ST_UDPSERVER类型的会话
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localInfo : 会话信息
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 可以使用IRtpSession::GetLocalPort(...)获取本地端口号
 */
IRtpSession*
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * 功能: 创建一个RTP_ST_TCPCLIENT类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时. 默认20秒
 * suspendRecv      : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp          = NULL,
                          unsigned long           timeoutInSeconds = 0,
                          bool                    suspendRecv      = false);

/*
 * 功能: 创建一个RTP_ST_TCPSERVER类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort        : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 * timeoutInSeconds : 握手超时. 默认20秒
 * suspendRecv      : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 可以使用IRtpSession::GetLocalPort(...)获取本地端口号
 *
 *       suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp          = NULL,
                          unsigned short          localPort        = 0,
                          unsigned long           timeoutInSeconds = 0,
                          bool                    suspendRecv      = false);

/*
 * 功能: 创建一个RTP_ST_UDPCLIENT_EX类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时. 默认10秒
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 无
 */
IRtpSession*
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp          = NULL,
                            unsigned long           timeoutInSeconds = 0);

/*
 * 功能: 创建一个RTP_ST_UDPSERVER_EX类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * localPort        : 要绑定的本地端口号. 如果为0, 系统将随机分配一个
 * timeoutInSeconds : 握手超时. 默认10秒
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 可以使用IRtpSession::GetLocalPort(...)获取本地端口号
 */
IRtpSession*
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp          = NULL,
                            unsigned short          localPort        = 0,
                            unsigned long           timeoutInSeconds = 0);

/*
 * 功能: 创建一个RTP_ST_TCPCLIENT_EX类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * password         : 会话口令
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时. 默认20秒
 * suspendRecv      : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password         = NULL,
                            const char*             localIp          = NULL,
                            unsigned long           timeoutInSeconds = 0,
                            bool                    suspendRecv      = false);

/*
 * 功能: 创建一个RTP_ST_TCPSERVER_EX类型的会话
 *
 * 参数:
 * observer    : 回调目标
 * reactor     : 反应器
 * localInfo   : 会话信息. 根据IRtpServiceObserver::OnAcceptSession(...)的remoteInfo构造
 * sockId      : 套接字id. 来源于IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket  : 是否unix套接字
 * useAckData  : 是否使用自定义的会话应答数据
 * ackData     : 自定义的会话应答数据
 * suspendRecv : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_INT64               sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv = false);

/*
 * 功能: 创建一个RTP_ST_SSLCLIENT_EX类型的会话
 *
 * 参数:
 * observer         : 回调目标
 * reactor          : 反应器
 * localInfo        : 会话信息
 * sslConfig        : ssl配置
 * sslSni           : ssl服务名. 如果有效, 则参与认证服务端证书
 * remoteIp         : 远端的ip地址或域名
 * remotePort       : 远端的端口号
 * password         : 会话口令
 * localIp          : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 * timeoutInSeconds : 握手超时. 默认20秒
 * suspendRecv      : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: sslConfig指定的对象必须在会话的生命周期内一直有效
 *
 *       suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionSslclientEx(IRtpSessionObserver*         observer,
                            IProReactor*                 reactor,
                            const RTP_SESSION_INFO*      localInfo,
                            const PRO_SSL_CLIENT_CONFIG* sslConfig,
                            const char*                  sslSni, /* = NULL */
                            const char*                  remoteIp,
                            unsigned short               remotePort,
                            const char*                  password         = NULL,
                            const char*                  localIp          = NULL,
                            unsigned long                timeoutInSeconds = 0,
                            bool                         suspendRecv      = false);

/*
 * 功能: 创建一个RTP_ST_SSLSERVER_EX类型的会话
 *
 * 参数:
 * observer    : 回调目标
 * reactor     : 反应器
 * localInfo   : 会话信息. 根据IRtpServiceObserver::OnAcceptSession(...)的remoteInfo构造
 * sslCtx      : ssl上下文
 * sockId      : 套接字id. 来源于IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket  : 是否unix套接字
 * useAckData  : 是否使用自定义的会话应答数据
 * ackData     : 自定义的会话应答数据
 * suspendRecv : 是否挂起接收能力
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 如果创建成功, 会话将成为(sslCtx, sockId)的属主; 否则, 调用者应该
 *       释放(sslCtx, sockId)对应的资源
 *
 *       suspendRecv用于一些需要精确控制时序的场景
 */
IRtpSession*
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            PRO_INT64               sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv = false);

/*
 * 功能: 创建一个RTP_ST_MCAST类型的会话
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localInfo : 会话信息
 * mcastIp   : 要绑定的多播地址
 * mcastPort : 要绑定的多播端口号. 如果为0, 系统将随机分配一个
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 合法的多播地址为[224.0.0.0 ~ 239.255.255.255],
 *       推荐的多播地址为[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       可以使用IRtpSession::GetLocalPort(...)获取多播端口号
 */
IRtpSession*
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort = 0,
                      const char*             localIp   = NULL);

/*
 * 功能: 创建一个RTP_ST_MCAST_EX类型的会话
 *
 * 参数:
 * observer  : 回调目标
 * reactor   : 反应器
 * localInfo : 会话信息
 * mcastIp   : 要绑定的多播地址
 * mcastPort : 要绑定的多播端口号. 如果为0, 系统将随机分配一个
 * localIp   : 要绑定的本地ip地址. 如果为NULL, 系统将使用0.0.0.0
 *
 * 返回值: 会话对象或NULL
 *
 * 说明: 合法的多播地址为[224.0.0.0 ~ 239.255.255.255],
 *       推荐的多播地址为[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       可以使用IRtpSession::GetLocalPort(...)获取多播端口号
 */
IRtpSession*
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort = 0,
                        const char*             localIp   = NULL);

/*
 * 功能: 删除一个会话
 *
 * 参数:
 * session : 会话对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
void
DeleteRtpSession(IRtpSession* session);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_A_H */
