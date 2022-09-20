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

#if !defined(RTP_SESSION_UDPCLIENT_EX_H)
#define RTP_SESSION_UDPCLIENT_EX_H

#include "rtp_session_base.h"
#include "rtp_session_udpserver_ex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionUdpclientEx : public CRtpSessionBase
{
public:

    static CRtpSessionUdpclientEx* CreateInstance(const RTP_SESSION_INFO* localInfo);

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        const char*          remoteIp,
        unsigned short       remotePort,
        const char*          localIp,         /* = NULL */
        unsigned long        timeoutInSeconds /* = 0 */
        );

    virtual void Fini();

private:

    CRtpSessionUdpclientEx(const RTP_SESSION_INFO& localInfo);

    virtual ~CRtpSessionUdpclientEx();

    virtual void GetSyncId(unsigned char syncId[14]) const;

    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    bool DoHandshake1();

    void DoHandshake3();

private:

    RTP_UDPX_SYNC m_syncToPeer; /* network byte order */
    PRO_UINT64    m_syncTimerId;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_UDPCLIENT_EX_H */
