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

#ifndef PRO_EPOLL_REACTOR_H
#define PRO_EPOLL_REACTOR_H

#include "pro_base_reactor.h"

#if defined(PRO_HAS_EPOLL)

/////////////////////////////////////////////////////////////////////////////
////

class CProEpollReactor : public CProBaseReactor
{
public:

    CProEpollReactor();

    virtual ~CProEpollReactor();

    virtual bool Init();

    virtual void Fini();

    virtual bool AddHandler(
        int64_t           sockId,
        CProEventHandler* handler,
        unsigned long     mask
        );

    virtual void RemoveHandler(
        int64_t       sockId,
        unsigned long mask
        );

    virtual void WorkerRun();

private:

    virtual void OnInput(int64_t sockId);

    virtual void OnError(
        int64_t sockId,
        int     errorCode
        );

private:

    int              m_epfd;
    pbsd_epoll_event m_events[PRO_EPOLLFD_GETSIZE]; /* sizeof(epoll_event) is 16 */

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_HAS_EPOLL */

#endif /* PRO_EPOLL_REACTOR_H */
