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
PRO_CALLTYPE
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    CRtpSessionUdpclient* const session =
        CRtpSessionUdpclient::CreateInstance(localInfo);
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
PRO_CALLTYPE
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    CRtpSessionUdpserver* const session =
        CRtpSessionUdpserver::CreateInstance(localInfo);
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
PRO_CALLTYPE
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp,          /* = NULL */
                          unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionTcpclient* const session =
        CRtpSessionTcpclient::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,          /* = NULL */
                          unsigned short          localPort,        /* = 0 */
                          unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionTcpserver* const session =
        CRtpSessionTcpserver::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionUdpclientEx* const session =
        CRtpSessionUdpclientEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp,          /* = NULL */
                            unsigned short          localPort,        /* = 0 */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionUdpserverEx* const session =
        CRtpSessionUdpserverEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(
        observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password,         /* = NULL */
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    CRtpSessionTcpclientEx* const session =
        CRtpSessionTcpclientEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor,
        remoteIp, remotePort, password, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_INT64               sockId,
                            bool                    unixSocket)
{
    CRtpSessionTcpserverEx* const session =
        CRtpSessionTcpserverEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslclientEx(IRtpSessionObserver*         observer,
                            IProReactor*                 reactor,
                            const RTP_SESSION_INFO*      localInfo,
                            const PRO_SSL_CLIENT_CONFIG* sslConfig,
                            const char*                  sslSni,           /* = NULL */
                            const char*                  remoteIp,
                            unsigned short               remotePort,
                            const char*                  password,         /* = NULL */
                            const char*                  localIp,          /* = NULL */
                            unsigned long                timeoutInSeconds) /* = 0 */
{
    CRtpSessionSslclientEx* const session =
        CRtpSessionSslclientEx::CreateInstance(localInfo, sslConfig, sslSni);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor,
        remoteIp, remotePort, password, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            PRO_INT64               sockId,
                            bool                    unixSocket)
{
    CRtpSessionSslserverEx* const session =
        CRtpSessionSslserverEx::CreateInstance(localInfo, sslCtx);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

IRtpSession*
PRO_CALLTYPE
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort, /* = 0 */
                      const char*             localIp)   /* = NULL */
{
    CRtpSessionMcast* const session =
        CRtpSessionMcast::CreateInstance(localInfo);
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
PRO_CALLTYPE
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort, /* = 0 */
                        const char*             localIp)   /* = NULL */
{
    CRtpSessionMcastEx* const session =
        CRtpSessionMcastEx::CreateInstance(localInfo);
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
PRO_CALLTYPE
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
    } /* end of switch (...) */
}
