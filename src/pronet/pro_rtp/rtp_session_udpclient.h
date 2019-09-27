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
 * RFC-1889/1890, RFC-3550/3551
 */

#if !defined(RTP_SESSION_UDPCLIENT_H)
#define RTP_SESSION_UDPCLIENT_H

#include "rtp_session_base.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionUdpclient : public CRtpSessionBase
{
public:

    static CRtpSessionUdpclient* CreateInstance(
        const RTP_SESSION_INFO* localInfo
        );

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        const char*          localIp,  /* = NULL */
        unsigned short       localPort /* = 0 */
        );

    virtual void Fini();

protected:

    CRtpSessionUdpclient(const RTP_SESSION_INFO& localInfo);

    virtual ~CRtpSessionUdpclient();

private:

    virtual void PRO_CALLTYPE SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        );

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_UDPCLIENT_H */
