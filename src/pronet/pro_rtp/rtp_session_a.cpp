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

#include "rtp_session_a.h"
#include "rtp_base.h"
#include "rtp_session_mcast.h"
#include "rtp_session_mcast_ex.h"
#include "rtp_session_sslclient_ex.h"
#include "rtp_session_sslserver_ex.h"
#include "rtp_session_tcpclient.h"
#include "rtp_session_tcpclient_ex.h"
#include "rtp_session_tcpserver.h"
#include "rtp_session_tcpserver_ex.h"
#include "rtp_session_udpclient.h"
#include "rtp_session_udpclient_ex.h"
#include "rtp_session_udpserver.h"
#include "rtp_session_udpserver_ex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

IRtpSession*
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    CRtpSessionUdpclient* const session = CRtpSessionUdpclient::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    CRtpSessionUdpserver* const session = CRtpSessionUdpserver::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp,          /* = NULL */
                          unsigned long           timeoutInSeconds, /* = 0 */
                          bool                    suspendRecv)      /* = false */
{
    CRtpSessionTcpclient* const session =
        CRtpSessionTcpclient::CreateInstance(localInfo, suspendRecv);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,          /* = NULL */
                          unsigned short          localPort,        /* = 0 */
                          unsigned long           timeoutInSeconds, /* = 0 */
                          bool                    suspendRecv)      /* = false */
{
    CRtpSessionTcpserver* const session =
        CRtpSessionTcpserver::CreateInstance(localInfo, suspendRecv);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionUdpclientEx* const session = CRtpSessionUdpclientEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp,          /* = NULL */
                            unsigned short          localPort,        /* = 0 */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionUdpserverEx* const session = CRtpSessionUdpserverEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password,         /* = NULL */
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds, /* = 0 */
                            bool                    suspendRecv)      /* = false */
{
    CRtpSessionTcpclientEx* const session =
        CRtpSessionTcpclientEx::CreateInstance(localInfo, suspendRecv);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, remoteIp, remotePort, password, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv) /* = false */
{
    CRtpSessionTcpserverEx* const session =
        CRtpSessionTcpserverEx::CreateInstance(localInfo, suspendRecv);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket, useAckData, ackData))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionSslclientEx(IRtpSessionObserver*         observer,
                            IProReactor*                 reactor,
                            const RTP_SESSION_INFO*      localInfo,
                            const PRO_SSL_CLIENT_CONFIG* sslConfig,
                            const char*                  sslSni,           /* = NULL */
                            const char*                  remoteIp,
                            unsigned short               remotePort,
                            const char*                  password,         /* = NULL */
                            const char*                  localIp,          /* = NULL */
                            unsigned long                timeoutInSeconds, /* = 0 */
                            bool                         suspendRecv)      /* = false */
{
    CRtpSessionSslclientEx* const session = CRtpSessionSslclientEx::CreateInstance(
        localInfo, suspendRecv, sslConfig, sslSni);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, remoteIp, remotePort, password, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            int64_t                 sockId,
                            bool                    unixSocket,
                            bool                    useAckData,
                            char                    ackData[64],
                            bool                    suspendRecv) /* = false */
{
    CRtpSessionSslserverEx* const session =
        CRtpSessionSslserverEx::CreateInstance(localInfo, suspendRecv, sslCtx);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket, useAckData, ackData))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort, /* = 0 */
                      const char*             localIp)   /* = NULL */
{
    CRtpSessionMcast* const session = CRtpSessionMcast::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, mcastIp, mcastPort, localIp))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort, /* = 0 */
                        const char*             localIp)   /* = NULL */
{
    CRtpSessionMcastEx* const session = CRtpSessionMcastEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, mcastIp, mcastPort, localIp))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

void
DeleteRtpSession(IRtpSession* session)
{
    if (session == NULL)
    {
        return;
    }

    RTP_SESSION_INFO sessionInfo;
    session->GetInfo(&sessionInfo);

    switch (sessionInfo.sessionType)
    {
    case RTP_ST_UDPCLIENT:
        {
            CRtpSessionUdpclient* const p = (CRtpSessionUdpclient*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_UDPSERVER:
        {
            CRtpSessionUdpserver* const p = (CRtpSessionUdpserver*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_TCPCLIENT:
        {
            CRtpSessionTcpclient* const p = (CRtpSessionTcpclient*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_TCPSERVER:
        {
            CRtpSessionTcpserver* const p = (CRtpSessionTcpserver*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_UDPCLIENT_EX:
        {
            CRtpSessionUdpclientEx* const p = (CRtpSessionUdpclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_UDPSERVER_EX:
        {
            CRtpSessionUdpserverEx* const p = (CRtpSessionUdpserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_TCPCLIENT_EX:
        {
            CRtpSessionTcpclientEx* const p = (CRtpSessionTcpclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_TCPSERVER_EX:
        {
            CRtpSessionTcpserverEx* const p = (CRtpSessionTcpserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_SSLCLIENT_EX:
        {
            CRtpSessionSslclientEx* const p = (CRtpSessionSslclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_SSLSERVER_EX:
        {
            CRtpSessionSslserverEx* const p = (CRtpSessionSslserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_MCAST:
        {
            CRtpSessionMcast* const p = (CRtpSessionMcast*)session;
            p->Fini();
            p->Release();
            break;
        }
    case RTP_ST_MCAST_EX:
        {
            CRtpSessionMcastEx* const p = (CRtpSessionMcastEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    } /* end of switch () */
}
