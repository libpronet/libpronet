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

/*
 * RFC-1889/1890, RFC-3550/3551, RFC-4571
 */

#if !defined(RTP_SESSION_TCPCLIENT_H)
#define RTP_SESSION_TCPCLIENT_H

#include "rtp_session_base.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionTcpclient
:
public IProConnectorObserver,
public CRtpSessionBase
{
public:

    static CRtpSessionTcpclient* CreateInstance(
        const RTP_SESSION_INFO* localInfo,
        bool                    suspendRecv
        );

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        const char*          remoteIp,
        unsigned short       remotePort,
        const char*          localIp,         /* = NULL */
        unsigned long        timeoutInSeconds /* = 0 */
        );

    virtual void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CRtpSessionTcpclient(
        const RTP_SESSION_INFO& localInfo,
        bool                    suspendRecv
        );

    virtual ~CRtpSessionTcpclient();

    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector* connector,
        PRO_INT64      sockId,
        bool           unixSocket,
        const char*    remoteIp,
        unsigned short remotePort
        );

    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        bool           timeout
        );

    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector*   connector,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        )
    {
    }

    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,
        unsigned short remotePort,
        unsigned char  serviceId,
        unsigned char  serviceOpt,
        bool           timeout
        )
    {
    }

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    bool Recv(
        CRtpPacket*& packet,
        bool&        leave
        );

private:

    IProConnector* m_connector;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_TCPCLIENT_H */
