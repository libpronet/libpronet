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

#ifndef RTP_SESSION_TCPSERVER_EX_H
#define RTP_SESSION_TCPSERVER_EX_H

#include "rtp_session_base.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionTcpserverEx : public CRtpSessionBase
{
public:

    static CRtpSessionTcpserverEx* CreateInstance(
        const RTP_SESSION_INFO* localInfo,
        bool                    suspendRecv
        );

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        int64_t              sockId,
        bool                 unixSocket,
        bool                 useAckData,
        char                 ackData[64]
        );

    virtual void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

protected:

    CRtpSessionTcpserverEx(
        const RTP_SESSION_INFO& localInfo,
        bool                    suspendRecv,
        PRO_SSL_CTX*            sslCtx /* = NULL */
        );

    virtual ~CRtpSessionTcpserverEx();

private:

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
        bool useAckData,
        char ackData[64]
        );

private:

    PRO_SSL_CTX* m_sslCtx;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_TCPSERVER_EX_H */
