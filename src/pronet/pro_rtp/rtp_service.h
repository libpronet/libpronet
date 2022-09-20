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

#if !defined(RTP_SERVICE_H)
#define RTP_SERVICE_H

#include "rtp_base.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_memory_pool.h"
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
        unsigned long        timeoutInSeconds /* = 0 */
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

private:

    CRtpService(
        const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
        RTP_MM_TYPE                  mmType
        );

    virtual ~CRtpService();

    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort
        )
    {
    }

    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,
        bool             unixSocket,
        const char*      localIp,
        const char*      remoteIp,
        unsigned short   remotePort,
        unsigned char    serviceId,
        unsigned char    serviceOpt,
        const PRO_NONCE* nonce
        );

    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,
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
        PRO_INT64          sockId,
        bool               unixSocket,
        const void*        buf,
        unsigned long      size
        );

    virtual void OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,
        long               sslCode
        );

private:

    const PRO_SSL_SERVER_CONFIG* const        m_sslConfig;
    const RTP_MM_TYPE                         m_mmType;
    IRtpServiceObserver*                      m_observer;
    IProReactor*                              m_reactor;
    IProServiceHost*                          m_serviceHost;
    unsigned long                             m_timeoutInSeconds;

    CProStlMap<IProTcpHandshaker*, PRO_NONCE> m_tcpHandshaker2Nonce;
    CProStlMap<IProSslHandshaker*, PRO_NONCE> m_sslHandshaker2Nonce;

    CProThreadMutex                           m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SERVICE_H */
