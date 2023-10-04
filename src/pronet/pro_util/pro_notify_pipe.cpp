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

#include "pro_a.h"
#include "pro_notify_pipe.h"
#include "pro_bsd_wrapper.h"
#include "pro_memory_pool.h"
#include "pro_time_util.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RECV_BUF_SIZE (1024 * 8)
#define SEND_BUF_SIZE (1024 * 8)

static char g_s_buffer[1024];

/////////////////////////////////////////////////////////////////////////////
////

CProNotifyPipe::CProNotifyPipe()
{
    m_sockIds[0] = -1;
    m_sockIds[1] = -1;
    m_signal     = false;
    m_notifyTick = 0;
}

CProNotifyPipe::~CProNotifyPipe()
{
    Fini();
}

void
CProNotifyPipe::Init()
{
    assert(m_sockIds[0] == -1);
    assert(m_sockIds[1] == -1);
    if (m_sockIds[0] != -1 || m_sockIds[1] != -1)
    {
        return;
    }

#if defined(_WIN32)

    int64_t sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockId == -1)
    {
        return;
    }

    int option;
    option = RECV_BUF_SIZE;
    pbsd_setsockopt(sockId, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
    option = SEND_BUF_SIZE;
    pbsd_setsockopt(sockId, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));

    pbsd_sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(pbsd_sockaddr_in));
    localAddr.sin_family      = AF_INET;
    localAddr.sin_addr.s_addr = pbsd_inet_aton("127.0.0.1");

    int retc = pbsd_bind(sockId, &localAddr, false);
    if (retc != 0)
    {
        pbsd_closesocket(sockId);

        return;
    }

    retc = pbsd_getsockname(sockId, &localAddr);
    if (retc != 0)
    {
        pbsd_closesocket(sockId);

        return;
    }

    retc = pbsd_connect(sockId, &localAddr); /* connected */
    if (retc != 0 && pbsd_errno((void*)&pbsd_connect) != PBSD_EINPROGRESS)
    {
        pbsd_closesocket(sockId);

        return;
    }

    m_sockIds[0] = sockId;
    m_sockIds[1] = sockId;

#else  /* _WIN32 */

    int64_t sockIds[2] = { -1, -1 };

    int retc = pbsd_socketpair(sockIds); /* connected */
    if (retc != 0)
    {
        return;
    }

    m_sockIds[0] = sockIds[0];
    m_sockIds[1] = sockIds[1];

#endif /* _WIN32 */
}

void
CProNotifyPipe::Fini()
{
#if defined(_WIN32)
    pbsd_closesocket(m_sockIds[0]);
#else
    pbsd_closesocket(m_sockIds[0]);
    pbsd_closesocket(m_sockIds[1]);
#endif

    m_sockIds[0] = -1;
    m_sockIds[1] = -1;
    m_signal     = false;
    m_notifyTick = 0;
}

void
CProNotifyPipe::Notify()
{
    int64_t sockId = GetWriterSockId();
    if (sockId == -1)
    {
        return;
    }

    int64_t tick = ProGetTickCount64();
    if (tick > m_notifyTick)
    {
        m_signal     = false;
        m_notifyTick = 0;
    }

    if (m_signal)
    {
        return;
    }

    char buf[] = { 0 };
    if (pbsd_send(sockId, buf, sizeof(buf), 0) > 0) /* connected */
    {
        m_signal     = true;
        m_notifyTick = tick;
    }
}

bool
CProNotifyPipe::Roger()
{
    int64_t sockId = GetReaderSockId();
    if (sockId == -1)
    {
        return false;
    }

    int recvSize = pbsd_recv(sockId, g_s_buffer, sizeof(g_s_buffer), 0); /* connected */
    if (
        (recvSize > 0 && recvSize <= (int)sizeof(g_s_buffer))
        ||
        (recvSize < 0 && pbsd_errno((void*)&pbsd_recv) == PBSD_EWOULDBLOCK)
       )
    {
        m_signal     = false;
        m_notifyTick = 0;

        return true;
    }
    else
    {
        return false;
    }
}
