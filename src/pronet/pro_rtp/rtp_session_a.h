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

#ifndef RTP_SESSION_A_H
#define RTP_SESSION_A_H

#include "rtp_base.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Function: Create a session of type RTP_ST_UDPCLIENT
 *
 * Parameters:
 * observer  : Callback target
 * reactor   : Reactor
 * localInfo : Session information
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort : Local port number to bind. If 0, system assigns randomly
 *
 * Return: Session object or NULL
 *
 * Note: Use IRtpSession::GetLocalPort() to get the actual local port number
 */
IRtpSession*
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * Function: Create a session of type RTP_ST_UDPSERVER
 *
 * Parameters:
 * observer  : Callback target
 * reactor   : Reactor
 * localInfo : Session information
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort : Local port number to bind. If 0, system assigns randomly
 *
 * Return: Session object or NULL
 *
 * Note: Use IRtpSession::GetLocalPort() to get the actual local port number
 */
IRtpSession*
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * Function: Create a session of type RTP_ST_TCPCLIENT
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
IRtpSession*
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp          = NULL,
                          unsigned int            timeoutInSeconds = 0,
                          bool                    suspendRecv      = false);

/*
 * Function: Create a session of type RTP_ST_TCPSERVER
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort        : Local port number to bind. If 0, system assigns randomly
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: Use IRtpSession::GetLocalPort() to get the actual local port number
 *
 *       suspendRecv is used for scenarios requiring precise control
 */
IRtpSession*
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp          = NULL,
                          unsigned short          localPort        = 0,
                          unsigned int            timeoutInSeconds = 0,
                          bool                    suspendRecv      = false);

/*
 * Function: Create a session of type RTP_ST_UDPCLIENT_EX
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 *
 * Return: Session object or NULL
 *
 * Note: None
 */
IRtpSession*
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp          = NULL,
                            unsigned int            timeoutInSeconds = 0);

/*
 * Function: Create a session of type RTP_ST_UDPSERVER_EX
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort        : Local port number to bind. If 0, system assigns randomly
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 *
 * Return: Session object or NULL
 *
 * Note: Use IRtpSession::GetLocalPort() to get the actual local port number
 */
IRtpSession*
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp          = NULL,
                            unsigned short          localPort        = 0,
                            unsigned int            timeoutInSeconds = 0);

/*
 * Function: Create a session of type RTP_ST_TCPCLIENT_EX
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * password         : Session password
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
IRtpSession*
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password         = NULL,
                            const char*             localIp          = NULL,
                            unsigned int            timeoutInSeconds = 0,
                            bool                    suspendRecv      = false);

/*
 * Function: Create a session of type RTP_ST_TCPSERVER_EX
 *
 * Parameters:
 * observer    : Callback target
 * reactor     : Reactor
 * localInfo   : Session information.
 *               Constructed from IRtpServiceObserver::OnAcceptSession()'s remoteInfo
 * sockId      : Socket ID. From IRtpServiceObserver::OnAcceptSession()
 * unixSocket  : Whether it's a Unix socket
 * useAckData  : Whether to use custom session acknowledgement data
 * ackData     : Custom session acknowledgement data
 * suspendRecv : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
IRtpSession*
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv = false);

/*
 * Function: Create a session of type RTP_ST_SSLCLIENT_EX
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localInfo        : Session information
 * sslConfig        : SSL configuration
 * sslSni           : SSL server name.
 *                    If valid, participates in server certificate verification
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * password         : Session password
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: The object specified by sslConfig must remain valid throughout
 *       the session's lifetime
 *
 *       suspendRecv is used for scenarios requiring precise control
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
                            unsigned int                 timeoutInSeconds = 0,
                            bool                         suspendRecv      = false);

/*
 * Function: Create a session of type RTP_ST_SSLSERVER_EX
 *
 * Parameters:
 * observer    : Callback target
 * reactor     : Reactor
 * localInfo   : Session information.
 *               Constructed from IRtpServiceObserver::OnAcceptSession()'s remoteInfo
 * sslCtx      : SSL context
 * sockId      : Socket ID. From IRtpServiceObserver::OnAcceptSession()
 * unixSocket  : Whether it's a Unix socket
 * useAckData  : Whether to use custom session acknowledgement data
 * ackData     : Custom session acknowledgement data
 * suspendRecv : Whether to suspend receive capability
 *
 * Return: Session object or NULL
 *
 * Note: If created successfully, session becomes owner of (sslCtx, sockId);
 *       Otherwise, caller should release resources for (sslCtx, sockId)
 *
 *       suspendRecv is used for scenarios requiring precise control
 */
IRtpSession*
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv = false);

/*
 * Function: Create a session of type RTP_ST_MCAST
 *
 * Parameters:
 * observer  : Callback target
 * reactor   : Reactor
 * localInfo : Session information
 * mcastIp   : Multicast IP address to bind
 * mcastPort : Multicast port number to bind. If 0, system assigns randomly
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 *
 * Return: Session object or NULL
 *
 * Note: Valid multicast addresses: [224.0.0.0 ~ 239.255.255.255],
 *       Recommended: [224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       Use IRtpSession::GetLocalPort() to get the actual multicast port number
 */
IRtpSession*
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort = 0,
                      const char*             localIp   = NULL);

/*
 * Function: Create a session of type RTP_ST_MCAST_EX
 *
 * Parameters:
 * observer  : Callback target
 * reactor   : Reactor
 * localInfo : Session information
 * mcastIp   : Multicast IP address to bind
 * mcastPort : Multicast port number to bind. If 0, system assigns randomly
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 *
 * Return: Session object or NULL
 *
 * Note: Valid multicast addresses: [224.0.0.0 ~ 239.255.255.255],
 *       Recommended: [224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       Use IRtpSession::GetLocalPort() to get the actual multicast port number
 */
IRtpSession*
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort = 0,
                        const char*             localIp   = NULL);

/*
 * Function: Delete a session
 *
 * Parameters:
 * session : Session object
 *
 * Return: None
 *
 * Note: None
 */
void
DeleteRtpSession(IRtpSession* session);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_A_H */
