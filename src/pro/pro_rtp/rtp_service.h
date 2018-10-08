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

#if !defined(RTP_SERVICE_H)
#define RTP_SERVICE_H

#include "rtp_framework.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpService
:
public IProServiceHostObserver,
public IProTcpHandshakerObserver,
public IProSslHandshakerObserver,
public CProRefCount
{
public:

    static CRtpService* CreateInstance(
        const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
        RTP_MM_TYPE                  mmType
        );

    bool Init(
        IRtpServiceObserver* observer,
        IProReactor*         reactor,
        unsigned short       serviceHubPort,
        unsigned long        timeoutInSeconds   /* = 0 */
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

private:

    CRtpService(
        const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
        RTP_MM_TYPE                  mmType
        );

    virtual ~CRtpService();

    virtual void PRO_CALLTYPE OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        PRO_UINT64       nonce
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

private:

    const PRO_SSL_SERVER_CONFIG* const         m_sslConfig;
    const RTP_MM_TYPE                          m_mmType;
    IRtpServiceObserver*                       m_observer;
    IProReactor*                               m_reactor;
    IProServiceHost*                           m_serviceHost;
    unsigned long                              m_timeoutInSeconds;

    CProStlMap<IProTcpHandshaker*, PRO_UINT64> m_tcpHandshaker2Nonce;
    CProStlMap<IProSslHandshaker*, PRO_UINT64> m_sslHandshaker2Nonce;

    CProThreadMutex                            m_lock;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SERVICE_H */
