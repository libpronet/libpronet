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

#ifndef PRO_MCAST_TRANSPORT_H
#define PRO_MCAST_TRANSPORT_H

#include "pro_udp_transport.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProMcastTransport : public CProUdpTransport
{
public:

    static CProMcastTransport* CreateInstance(size_t recvPoolSize); /* = 0 */

    bool Init(
        IProTransportObserver* observer,
        CProTpReactorTask*     reactorTask,
        const char*            mcastIp,
        unsigned short         mcastPort,       /* = 0 */
        const char*            localBindIp,     /* = NULL */
        size_t                 sockBufSizeRecv, /* = 0 */
        size_t                 sockBufSizeSend  /* = 0 */
        );

    void Fini();

    virtual PRO_TRANS_TYPE GetType() const
    {
        return PRO_TRANS_MCAST;
    }

    virtual bool AddMcastReceiver(const char* mcastIp);

    virtual void RemoveMcastReceiver(const char* mcastIp);

private:

    CProMcastTransport(size_t recvPoolSize); /* = 0 */

    virtual ~CProMcastTransport();

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_MCAST_TRANSPORT_H */
