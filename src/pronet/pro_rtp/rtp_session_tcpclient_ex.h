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

    static CRtpSessionTcpclientEx* CreateInstance(
        const RTP_SESSION_INFO* localInfo,
        bool                    suspendRecv
        );

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

    virtual unsigned long AddRef();

    virtual unsigned long Release();

protected:

    CRtpSessionTcpclientEx(
        const RTP_SESSION_INFO&      localInfo,
        bool                         suspendRecv,
        const PRO_SSL_CLIENT_CONFIG* sslConfig, /* = NULL */
        const char*                  sslSni     /* = NULL */
        );

    virtual ~CRtpSessionTcpclientEx();

    virtual void OnConnectOk(
        IProConnector* connector,
        int64_t        sockId,
        bool           unixSocket,
        const char*    remoteIp,
        unsigned short remotePort
        )
    {
    }

    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        bool           timeout
        )
    {
    }

    virtual void OnConnectOk(
        IProConnector*   connector,
        int64_t          sockId,
        bool             unixSocket,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
        );

    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        int64_t            sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode
        );

    virtual void OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,
        int64_t            sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,
        long               sslCode
        );

    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    bool Recv0(
        CRtpPacket*& packet,
        bool&        leave
        );

    bool Recv2(
        CRtpPacket*& packet,
        bool&        leave
        );

    bool Recv4(
        CRtpPacket*& packet,
        bool&        leave
        );

    bool DoHandshake(
        int64_t          sockId,
        bool             unixSocket,
        const PRO_NONCE& nonce
        );

private:

    const PRO_SSL_CLIENT_CONFIG* const m_sslConfig;
    const CProStlString                m_sslSni;
    CProStlString                      m_password;
    unsigned long                      m_timeoutInSeconds;

    IProConnector*                     m_connector;
    IProTcpHandshaker*                 m_tcpHandshaker;
    IProSslHandshaker*                 m_sslHandshaker;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_TCPCLIENT_EX_H */
