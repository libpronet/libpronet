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

#include "pro_notify_pipe.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define RECV_BUF_SIZE (1024 * 8)
#define SEND_BUF_SIZE (1024 * 8)

/////////////////////////////////////////////////////////////////////////////
////

CProNotifyPipe::CProNotifyPipe()
{
    m_sockIds[0]    = -1;
    m_sockIds[1]    = -1;
    m_notifyPending = false;
    m_notifyTick    = 0;
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

    const PRO_INT64 sockId = pbsd_socket(AF_INET, SOCK_DGRAM, 0);
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

    PRO_INT64 sockIds[2] = { -1, -1 };

    const int retc = pbsd_socketpair(sockIds); /* connected */
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

    m_sockIds[0]    = -1;
    m_sockIds[1]    = -1;
    m_notifyPending = false;
    m_notifyTick    = 0;
}

void
CProNotifyPipe::Notify()
{
    const PRO_INT64 sockId = GetWriterSockId();
    if (sockId == -1)
    {
        return;
    }

    const PRO_INT64 tick = ProGetTickCount64();
    if (m_notifyPending && tick > m_notifyTick)
    {
        m_notifyPending = false;
        m_notifyTick    = 0;
    }
    if (m_notifyPending)
    {
        return;
    }

    const char buf[] = { 0 };
    if (pbsd_send(sockId, buf, sizeof(buf), 0) > 0) /* connected */
    {
        m_notifyPending = true;
        m_notifyTick    = tick;
    }
}

void
CProNotifyPipe::Roger()
{
    m_notifyPending = false;
    m_notifyTick    = 0;
}
