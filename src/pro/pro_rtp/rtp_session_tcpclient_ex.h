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

/*
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 * 5) client::[password hash]
 * 6) client ----->          rtp(RTP_SESSION_INFO)          -----> server
 * 7)                                             [password hash]::server
 * 8) client <-----               rtp(version)              <----- server
 *                   TCP_EX handshake protocol flow chart
 */

#if !defined(RTP_SESSION_TCPCLIENT_EX_H)
#define RTP_SESSION_TCPCLIENT_EX_H

#include "rtp_session_base.h"
#include "../pro_util/pro_stl.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionTcpclientEx
:
public IProConnectorObserver,
public IProTcpHandshakerObserver,
public IProSslHandshakerObserver,
public CRtpSessionBase
{
public:

    static CRtpSessionTcpclientEx* CreateInstance(const RTP_SESSION_INFO* localInfo);

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        const char*          remoteIp,
        unsigned short       remotePort,
        const char*          password,        /* = NULL */
        const char*          localIp,         /* = NULL */
        unsigned long        timeoutInSeconds /* = 0 */
        );

    virtual void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

protected:

    CRtpSessionTcpclientEx(
        const RTP_SESSION_INFO&      localInfo,
        const PRO_SSL_CLIENT_CONFIG* sslConfig,     /* = NULL */
        const char*                  sslServiceName /* = NULL */
        );

    virtual ~CRtpSessionTcpclientEx();

    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector* connector,
        PRO_INT64      sockId,
        bool           unixSocket,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        PRO_UINT64     nonce
        );

    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
        );

    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void PRO_CALLTYPE OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode
        );

    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void PRO_CALLTYPE OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,
        long               sslCode
        );

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    bool Recv0(CRtpPacket*& packet);

    bool Recv2(CRtpPacket*& packet);

    bool Recv4(CRtpPacket*& packet);

    bool DoHandshake(
        PRO_INT64  sockId,
        bool       unixSocket,
        PRO_UINT64 nonce
        );

private:

    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslServiceName;
    CProStlString                      m_password;
    unsigned long                      m_timeoutInSeconds;

    IProConnector*                     m_connector;
    IProTcpHandshaker*                 m_tcpHandshaker;
    IProSslHandshaker*                 m_sslHandshaker;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_TCPCLIENT_EX_H */
