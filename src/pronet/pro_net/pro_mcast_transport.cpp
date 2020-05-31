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

#include "pro_mcast_transport.h"
#include "pro_net.h"
#include "pro_tp_reactor_task.h"
#include "pro_udp_transport.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define MULTICAST_TTL 32 /* 0 ~ 255 */

/////////////////////////////////////////////////////////////////////////////
////

CProMcastTransport*
CProMcastTransport::CreateInstance(size_t recvPoolSize)     /* = 0 */
{
    CProMcastTransport* const trans = new CProMcastTransport(recvPoolSize);

    return (trans);
}

CProMcastTransport::CProMcastTransport(size_t recvPoolSize) /* = 0 */
: CProUdpTransport(recvPoolSize)
{
}

CProMcastTransport::~CProMcastTransport()
{
    Fini();

    ProCloseSockId(m_sockId);
    m_sockId = -1;
}

bool
CProMcastTransport::Init(IProTransportObserver* observer,
                         CProTpReactorTask*     reactorTask,
                         const char*            mcastIp,
                         unsigned short         mcastPort,       /* = 0 */
                         const char*            localBindIp,     /* = NULL */
                         size_t                 sockBufSizeRecv, /* = 0 */
                         size_t                 sockBufSizeSend) /* = 0 */
{
    assert(observer != NULL);
    assert(reactorTask != NULL);
    assert(mcastIp != NULL);
    assert(mcastIp[0] != '\0');
    if (observer == NULL || reactorTask == NULL || mcastIp == NULL ||
        mcastIp[0] == '\0')
    {
        return (false);
    }

    const PRO_UINT32 mcastIp2 = pbsd_inet_aton(mcastIp);
    if (mcastIp2 == (PRO_UINT32)-1 || mcastIp2 == 0)
    {
        return (false);
    }

    const PRO_UINT32 minIp = pbsd_ntoh32(pbsd_inet_aton("224.0.0.0"));
    const PRO_UINT32 maxIp = pbsd_ntoh32(pbsd_inet_aton("239.255.255.255"));
    const PRO_UINT32 theIp = pbsd_ntoh32(mcastIp2);

    assert(theIp >= minIp);
    assert(theIp <= maxIp);
    if (theIp < minIp || theIp > maxIp)
    {
        return (false);
    }

    char anyIp[64] = "0.0.0.0";
    if (localBindIp == NULL || localBindIp[0] == '\0')
    {
        localBindIp = anyIp;
    }

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = pbsd_hton16(mcastPort);
    localAddr.sin_addr.s_addr = pbsd_inet_aton(localBindIp);

    pbsd_sockaddr_in remoteAddr;
    memset(&remoteAddr, 0, sizeof(pbsd_sockaddr_in));
    remoteAddr.sin_family      = AF_INET;
    remoteAddr.sin_port        = pbsd_hton16(mcastPort);
    remoteAddr.sin_addr.s_addr = mcastIp2;

    if (localAddr.sin_addr.s_addr == (PRO_UINT32)-1)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_observer == NULL);
        assert(m_reactorTask == NULL);
        if (m_observer != NULL || m_reactorTask != NULL)
        {
            return (false);
        }

        if (!m_recvPool.Resize(m_recvPoolSize))
        {
            return (false);
        }

        const PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
        if (sockId == -1)
        {
            return (false);
        }

        int option;
        option = (int)sockBufSizeRecv;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
        option = (int)sockBufSizeSend;
        pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

        if (pbsd_bind(sockId, &localAddr, false) != 0)
        {
            ProCloseSockId(sockId);

            return (false);
        }

        if (pbsd_getsockname(sockId, &localAddr) != 0)
        {
            ProCloseSockId(sockId);

            return (false);
        }

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = remoteAddr.sin_addr.s_addr;
        mreq.imr_interface.s_addr = localAddr.sin_addr.s_addr;

        pbsd_setsockopt(sockId,
            IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));

        option = MULTICAST_TTL;
        pbsd_setsockopt(
            sockId, IPPROTO_IP, IP_MULTICAST_TTL, &option, sizeof(int));
        option = 0;
        pbsd_setsockopt(
            sockId, IPPROTO_IP, IP_MULTICAST_LOOP, &option, sizeof(int));

        if (!reactorTask->AddHandler(sockId, this, PRO_MASK_READ))
        {
            pbsd_setsockopt(sockId,
                IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));
            ProCloseSockId(sockId);

            return (false);
        }

        observer->AddRef();
        m_observer          = observer;
        m_reactorTask       = reactorTask;
        m_sockId            = sockId;
        m_localAddr         = localAddr;
        m_defaultRemoteAddr = remoteAddr;
    }

    return (true);
}

void
CProMcastTransport::Fini()
{
    IProTransportObserver* observer = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        m_reactorTask->CancelTimer(m_timerId);
        m_timerId = 0;

        m_reactorTask->RemoveHandler(
            m_sockId, this, PRO_MASK_WRITE | PRO_MASK_READ);

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = m_defaultRemoteAddr.sin_addr.s_addr;
        mreq.imr_interface.s_addr = m_localAddr.sin_addr.s_addr;

        pbsd_setsockopt(m_sockId,
            IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));

        m_reactorTask = NULL;
        observer = m_observer;
        m_observer = NULL;
    }

    observer->Release();
}

PRO_TRANS_TYPE
PRO_CALLTYPE
CProMcastTransport::GetType() const
{
    return (PRO_TRANS_MCAST);
}

bool
PRO_CALLTYPE
CProMcastTransport::AddMcastReceiver(const char* mcastIp)
{
    assert(mcastIp != NULL);
    assert(mcastIp[0] != '\0');
    if (mcastIp == NULL || mcastIp[0] == '\0')
    {
        return (false);
    }

    const PRO_UINT32 mcastIp2 = pbsd_inet_aton(mcastIp);
    if (mcastIp2 == (PRO_UINT32)-1 || mcastIp2 == 0)
    {
        return (false);
    }

    const PRO_UINT32 minIp = pbsd_ntoh32(pbsd_inet_aton("224.0.0.0"));
    const PRO_UINT32 maxIp = pbsd_ntoh32(pbsd_inet_aton("239.255.255.255"));
    const PRO_UINT32 theIp = pbsd_ntoh32(mcastIp2);

    assert(theIp >= minIp);
    assert(theIp <= maxIp);
    if (theIp < minIp || theIp > maxIp)
    {
        return (false);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return (false);
        }

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = mcastIp2;
        mreq.imr_interface.s_addr = m_localAddr.sin_addr.s_addr;

        pbsd_setsockopt(m_sockId,
            IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));
    }

    return (true);
}

void
PRO_CALLTYPE
CProMcastTransport::RemoveMcastReceiver(const char* mcastIp)
{
    if (mcastIp == NULL || mcastIp[0] == '\0')
    {
        return;
    }

    const PRO_UINT32 mcastIp2 = pbsd_inet_aton(mcastIp);
    if (mcastIp2 == (PRO_UINT32)-1 || mcastIp2 == 0)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_observer == NULL || m_reactorTask == NULL)
        {
            return;
        }

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(struct ip_mreq));
        mreq.imr_multiaddr.s_addr = mcastIp2;
        mreq.imr_interface.s_addr = m_localAddr.sin_addr.s_addr;

        pbsd_setsockopt(m_sockId,
            IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));
    }
}
