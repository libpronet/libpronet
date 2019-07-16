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
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 * 5) client::[password hash]
 * 6) client ----->          rtp(RTP_SESSION_INFO)          -----> server
 * 7)                                             [password hash]::server
 * 8) client <-----          rtp(RTP_SESSION_ACK)           <----- server
 *                   TCP_EX handshake protocol flow chart
 */

#if !defined(RTP_SESSION_TCPSERVER_EX_H)
#define RTP_SESSION_TCPSERVER_EX_H

#include "rtp_session_base.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionTcpserverEx : public CRtpSessionBase
{
public:

    static CRtpSessionTcpserverEx* CreateInstance(const RTP_SESSION_INFO* localInfo);

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        PRO_INT64            sockId,
        bool                 unixSocket
        );

    virtual void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

protected:

    CRtpSessionTcpserverEx(
        const RTP_SESSION_INFO& localInfo,
        PRO_SSL_CTX*            sslCtx /* = NULL */
        );

    virtual ~CRtpSessionTcpserverEx();

private:

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    bool Recv0(CRtpPacket*& packet);

    bool Recv2(CRtpPacket*& packet);

    bool Recv4(CRtpPacket*& packet);

    bool DoHandshake();

private:

    PRO_SSL_CTX* m_sslCtx;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_TCPSERVER_EX_H */
