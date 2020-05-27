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

#if !defined(PRO_SERVICE_PIPE_H)
#define PRO_SERVICE_PIPE_H

#include "pro_tcp_transport.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

static const char* const SERVICE_MAGIC = "********";

class CProServicePipe;

struct PRO_SERVICE_SOCK
{
    PRO_SERVICE_SOCK()
    {
        expireTick = 0;
        sockId     = -1;
        unixSocket = false;
    }

    bool operator<(const PRO_SERVICE_SOCK& ss) const
    {
        if (expireTick < ss.expireTick)
        {
            return (true);
        }
        if (expireTick > ss.expireTick)
        {
            return (false);
        }

        if (sockId < ss.sockId)
        {
            return (true);
        }

        return (false);
    }

    PRO_INT64 expireTick;
    PRO_INT64 sockId;
    bool      unixSocket;

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PACKET_C2S
{
    PRO_SERVICE_PACKET_C2S()
    {
        serviceId = 0;
        processId = 0;
    }

    unsigned char    serviceId;
    PRO_UINT64       processId;
    PRO_SERVICE_SOCK oldSock;

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PACKET_S2C
{
    PRO_SERVICE_PACKET_S2C()
    {
        serviceId  = 0;
        serviceOpt = 0;

        memset(&nonce, 0, sizeof(PRO_NONCE));
#if defined(_WIN32) && !defined(_WIN32_WCE)
        memset(&protocolInfo, 0, sizeof(WSAPROTOCOL_INFO));
#endif
    }

    unsigned char    serviceId;
    unsigned char    serviceOpt;
    PRO_NONCE        nonce;
#if defined(_WIN32) && !defined(_WIN32_WCE)
    WSAPROTOCOL_INFO protocolInfo;
#endif
    PRO_SERVICE_SOCK oldSock;

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PACKET
{
    PRO_SERVICE_PACKET()
    {
        memset(magic1, SERVICE_MAGIC[0], sizeof(magic1));
        memset(magic2, SERVICE_MAGIC[0], sizeof(magic2));
    }

    bool CheckMagic() const
    {
        return (
            memcmp(magic1, SERVICE_MAGIC, sizeof(magic1)) == 0 &&
            memcmp(magic2, SERVICE_MAGIC, sizeof(magic2)) == 0
            );
    }

    char                   magic1[8];
    PRO_SERVICE_PACKET_C2S c2s;
    PRO_SERVICE_PACKET_S2C s2c;
    char                   magic2[8];

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PIPE
{
    PRO_SERVICE_PIPE()
    {
        pipe       = NULL;
        pending    = true;
        expireTick = 0;
        serviceId  = 0;
        processId  = 0;
    }

    bool operator<(const PRO_SERVICE_PIPE& sp) const
    {
        if (pipe < sp.pipe)
        {
            return (true);
        }

        return (false);
    }

    CProServicePipe* pipe;
    bool             pending;
    PRO_INT64        expireTick;
    unsigned char    serviceId;
    PRO_UINT64       processId;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class IProServicePipeObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE OnRecv(
        CProServicePipe*          pipe,
        const PRO_SERVICE_PACKET& packet
        ) = 0;

    virtual void PRO_CALLTYPE OnRecvFd(
        CProServicePipe*          pipe,
        PRO_INT64                 fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        ) = 0;

    virtual void PRO_CALLTYPE OnClose(CProServicePipe* pipe) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

class CProServicePipe : public IProTransportObserverEx, public CProRefCount
{
public:

    static CProServicePipe* CreateInstance();

    bool Init(
        bool                     recvFdMode,
        IProServicePipeObserver* observer,
        IProReactor*             reactor,
        PRO_INT64                sockId,
        bool                     unixSocket
        );

    void Fini();

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    void SendData(const PRO_SERVICE_PACKET& packet);

    void SendFd(const PRO_SERVICE_PACKET& s2cPacket);

private:

    CProServicePipe();

    virtual ~CProServicePipe();

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void PRO_CALLTYPE OnRecvFd(
        IProTransport*            trans,
        PRO_INT64                 fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        );

    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        );

    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans)
    {
    }

private:

    IProServicePipeObserver*         m_observer;
    IProReactor*                     m_reactor;
    CProTcpTransport*                m_trans;
    CProStlDeque<PRO_SERVICE_PACKET> m_packets;
    CProStlDeque<PRO_SERVICE_PACKET> m_fdPackets;
    CProThreadMutex                  m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

CProServicePipe*
PRO_CALLTYPE
ProCreateServicePipe(bool                     recvFdMode,
                     IProServicePipeObserver* observer,
                     IProReactor*             reactor,
                     PRO_INT64                sockId,
                     bool                     unixSocket);

void
PRO_CALLTYPE
ProDeleteServicePipe(CProServicePipe* pipe);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SERVICE_PIPE_H */
