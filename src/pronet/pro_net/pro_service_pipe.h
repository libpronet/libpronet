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

    bool operator<(const PRO_SERVICE_SOCK& other) const
    {
        if (expireTick < other.expireTick)
        {
            return true;
        }
        if (expireTick > other.expireTick)
        {
            return false;
        }

        return sockId < other.sockId;
    }

    int64_t expireTick;
    int64_t sockId;
    bool    unixSocket;

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PACKET_C2S
{
    PRO_SERVICE_PACKET_C2S()
    {
        serviceId  = 0;
        processId  = 0;
        totalSocks = 0;
    }

    unsigned char    serviceId;
    uint64_t         processId;
    uint32_t         totalSocks;
    PRO_SERVICE_SOCK oldSock;

    DECLARE_SGI_POOL(0)
};

struct PRO_SERVICE_PACKET_S2C
{
    PRO_SERVICE_PACKET_S2C()
    {
        serviceOpt = 0;

        memset(&nonce, 0, sizeof(PRO_NONCE));
#if defined(_WIN32)
        memset(&protocolInfo, 0, sizeof(WSAPROTOCOL_INFO));
#endif
    }

    unsigned char    serviceOpt;
    PRO_NONCE        nonce;
#if defined(_WIN32)
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
        return memcmp(magic1, SERVICE_MAGIC, sizeof(magic1)) == 0 &&
               memcmp(magic2, SERVICE_MAGIC, sizeof(magic2)) == 0;
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
        processId  = 0;
        expireTick = 0;
    }

    PRO_SERVICE_PIPE(CProServicePipe* __pipe)
    {
        pipe       = __pipe;
        processId  = 0;
        expireTick = 0;
    }

    bool operator<(const PRO_SERVICE_PIPE& other) const
    {
        return pipe < other.pipe;
    }

    CProServicePipe* pipe;
    uint64_t         processId;
    int64_t          expireTick;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class IProServicePipeObserver
{
public:

    virtual ~IProServicePipeObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    virtual void OnRecv(
        CProServicePipe*          pipe,
        const PRO_SERVICE_PACKET& packet
        ) = 0;

    virtual void OnRecvFd(
        CProServicePipe*          pipe,
        int64_t                   fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        ) = 0;

    virtual void OnClose(CProServicePipe* pipe) = 0;
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
        int64_t                  sockId,
        bool                     unixSocket
        );

    void Fini();

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    void SendData(const PRO_SERVICE_PACKET& packet);

    void SendFd(const PRO_SERVICE_PACKET& s2cPacket);

private:

    CProServicePipe();

    virtual ~CProServicePipe();

    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void OnRecvFd(
        IProTransport*            trans,
        int64_t                   fd,
        bool                      unixSocket,
        const PRO_SERVICE_PACKET& s2cPacket
        );

    virtual void OnSend(
        IProTransport* trans,
        uint64_t       actionId
        );

    virtual void OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void OnHeartbeat(IProTransport* trans)
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
ProCreateServicePipe(bool                     recvFdMode,
                     IProServicePipeObserver* observer,
                     IProReactor*             reactor,
                     int64_t                  sockId,
                     bool                     unixSocket);

void
ProDeleteServicePipe(CProServicePipe* pipe);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SERVICE_PIPE_H */
