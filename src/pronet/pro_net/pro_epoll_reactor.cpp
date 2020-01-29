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

#include "pro_epoll_reactor.h"
#include "pro_base_reactor.h"
#include "pro_notify_pipe.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

#if defined(PRO_HAS_EPOLL)

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Within the Linux kernel source, we find the following definitions
 * which show the correspondence between the readable, writable,
 * and exceptional condition notifications of select() and the event
 * notifications provided by poll (and epoll):
 *
 * Ready for writing:
 * #define POLLOUT_SET (POLLWRNORM | POLLWRBAND | POLLOUT | POLLERR)
 * Ready for reading:
 * #define POLLIN_SET  (POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR)
 * Exceptional condition:
 * #define POLLEX_SET  (POLLPRI)
 */
#define PRO_EPOLLIN_SET  EPOLLIN
#define PRO_EPOLLOUT_SET EPOLLOUT
#define PRO_EPOLLEX_SET  EPOLLPRI
#define PRO_EPOLLHUP     EPOLLHUP
#define PRO_EPOLLERR     EPOLLERR

static char g_s_buffer[1024];

/////////////////////////////////////////////////////////////////////////////
////

CProEpollReactor::CProEpollReactor()
{
    m_epfd = -1;
}

CProEpollReactor::~CProEpollReactor()
{
    Fini();

    if (m_epfd != -1)
    {
        pbsd_epoll_event ev;
        memset(&ev, 0, sizeof(pbsd_epoll_event));

        const CProStlMap<PRO_INT64, PRO_HANDLER_INFO>& allHandlers =
            m_handlerMgr.GetAllHandlers();

        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::iterator       itr = allHandlers.begin();
        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::iterator const end = allHandlers.end();

        for (; itr != end; ++itr)
        {
            ev.data.fd = (int)itr->first;
            pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
        }

        close(m_epfd);
        m_epfd = -1;
    }

    delete m_notifyPipe;
    m_notifyPipe = NULL;
}

bool
PRO_CALLTYPE
CProEpollReactor::Init()
{
    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_epfd == -1);
        if (m_epfd != -1)
        {
            return (false);
        }

        m_epfd = pbsd_epoll_create();
        if (m_epfd == -1)
        {
            return (false);
        }

        m_notifyPipe->Init();

        const PRO_INT64 sockId = m_notifyPipe->GetReaderSockId();
        if (sockId == -1)
        {
            close(m_epfd);
            m_epfd = -1;

            return (false);
        }

        pbsd_epoll_event ev;
        memset(&ev, 0, sizeof(pbsd_epoll_event));
        ev.events  = PRO_EPOLLIN_SET;
        ev.data.fd = (int)sockId;

        if (pbsd_epoll_ctl(m_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) != 0)
        {
            close(m_epfd);
            m_epfd = -1;

            return (false);
        }

        if (!m_handlerMgr.AddHandler(sockId, this, PRO_MASK_READ))
        {
            pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
            close(m_epfd);
            m_epfd = -1;

            return (false);
        }
    }

    return (true);
}

void
PRO_CALLTYPE
CProEpollReactor::Fini()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_epfd == -1)
        {
            return;
        }

        m_wantExit = true;
        m_notifyPipe->Notify();
    }
}

bool
PRO_CALLTYPE
CProEpollReactor::AddHandler(PRO_INT64         sockId,
                             CProEventHandler* handler,
                             unsigned long     mask)
{
    mask &= (PRO_MASK_ACCEPT | PRO_MASK_CONNECT |
        PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);

    assert(sockId != -1);
    assert(handler != NULL);
    assert(mask != 0);
    if (sockId == -1 || handler == NULL || mask == 0)
    {
        return (false);
    }

    if (PRO_BIT_ENABLED(mask, PRO_MASK_ACCEPT))
    {
        PRO_CLR_BITS(mask, PRO_MASK_ACCEPT);
        PRO_SET_BITS(mask, PRO_MASK_READ);
    }
    if (PRO_BIT_ENABLED(mask, PRO_MASK_CONNECT))
    {
        PRO_CLR_BITS(mask, PRO_MASK_CONNECT);
        PRO_SET_BITS(mask,
            PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_epfd == -1 || m_wantExit)
        {
            return (false);
        }

        const PRO_HANDLER_INFO oldInfo = m_handlerMgr.FindHandler(sockId);
        if (oldInfo.handler != NULL && handler != oldInfo.handler)
        {
            return (false);
        }

        mask &= ~oldInfo.mask;
        if (mask == 0)
        {
            return (true);
        }

        short oldEvents = 0;
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_WRITE))
        {
            oldEvents |= PRO_EPOLLOUT_SET;
        }
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_READ))
        {
            oldEvents |= PRO_EPOLLIN_SET;
        }
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_EXCEPTION))
        {
            oldEvents |= PRO_EPOLLEX_SET;
        }

        short events = oldEvents;
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            events |= PRO_EPOLLOUT_SET;
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            events |= PRO_EPOLLIN_SET;
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            events |= PRO_EPOLLEX_SET;
        }

        pbsd_epoll_event ev;
        memset(&ev, 0, sizeof(pbsd_epoll_event));
        ev.events  = events;
        ev.data.fd = (int)sockId;

        int retc = -1;
        if (oldEvents == 0)
        {
            retc = pbsd_epoll_ctl(m_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
        }
        else
        {
            retc = pbsd_epoll_ctl(m_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
        }
        if (retc != 0)
        {
            return (false);
        }

        if (!m_handlerMgr.AddHandler(sockId, handler, mask))
        {
            /*
             * rollback
             */
            if (oldEvents == 0)
            {
                pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
            }
            else
            {
                ev.events = oldEvents;
                pbsd_epoll_ctl(m_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
            }

            return (false);
        }

        if (ProGetThreadId() != m_threadId)
        {
            m_notifyPipe->Notify();
        }
    }

    return (true);
}

void
PRO_CALLTYPE
CProEpollReactor::RemoveHandler(PRO_INT64     sockId,
                                unsigned long mask)
{
    mask &= (PRO_MASK_ACCEPT | PRO_MASK_CONNECT |
        PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);

    if (sockId == -1 || mask == 0)
    {
        return;
    }

    if (PRO_BIT_ENABLED(mask, PRO_MASK_ACCEPT))
    {
        PRO_CLR_BITS(mask, PRO_MASK_ACCEPT);
        PRO_SET_BITS(mask, PRO_MASK_READ);
    }
    if (PRO_BIT_ENABLED(mask, PRO_MASK_CONNECT))
    {
        PRO_CLR_BITS(mask, PRO_MASK_CONNECT);
        PRO_SET_BITS(mask,
            PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_epfd == -1)
        {
            return;
        }

        const PRO_HANDLER_INFO oldInfo = m_handlerMgr.FindHandler(sockId);
        if (oldInfo.handler == NULL)
        {
            return;
        }

        mask &= oldInfo.mask;
        if (mask == 0)
        {
            return;
        }

        short oldEvents = 0;
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_WRITE))
        {
            oldEvents |= PRO_EPOLLOUT_SET;
        }
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_READ))
        {
            oldEvents |= PRO_EPOLLIN_SET;
        }
        if (PRO_BIT_ENABLED(oldInfo.mask, PRO_MASK_EXCEPTION))
        {
            oldEvents |= PRO_EPOLLEX_SET;
        }

        short events = oldEvents;
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            events &= ~PRO_EPOLLOUT_SET;
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            events &= ~PRO_EPOLLIN_SET;
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            events &= ~PRO_EPOLLEX_SET;
        }

        pbsd_epoll_event ev;
        memset(&ev, 0, sizeof(pbsd_epoll_event));
        ev.events  = events;
        ev.data.fd = (int)sockId;

        if (events == 0)
        {
            pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
        }
        else
        {
            pbsd_epoll_ctl(m_epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
        }

        m_handlerMgr.RemoveHandler(sockId, mask);

        if (ProGetThreadId() != m_threadId)
        {
            m_notifyPipe->Notify();
        }
    }
}

void
PRO_CALLTYPE
CProEpollReactor::WorkerRun()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_threadId = ProGetThreadId();
    }

    while (1)
    {
        {
            CProThreadMutexGuard mon(m_lock);

            if (m_epfd == -1 || m_wantExit)
            {
                break;
            }
        }

        /*
         * epoll_wait(...)
         */
        const int retc = pbsd_epoll_wait(
            m_epfd, m_events, PRO_EPOLLFD_GETSIZE, -1);
        if (retc <= 0)
        {
            ProSleep(1);
            continue;
        }

        CProStlMap<PRO_INT64, PRO_HANDLER_INFO> handlers;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_epfd == -1 || m_wantExit)
            {
                break;
            }

            for (int i = 0; i < retc; ++i)
            {
                const pbsd_epoll_event& ev = m_events[i];
                if (ev.events == 0)
                {
                    continue;
                }

                const PRO_HANDLER_INFO info =
                    m_handlerMgr.FindHandler(ev.data.fd);
                if (info.handler == NULL)
                {
                    continue;
                }

                if ((ev.events & PRO_EPOLLERR) != 0)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[ev.data.fd]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_ERROR);
                    continue;
                }

                if ((ev.events & PRO_EPOLLOUT_SET) != 0)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[ev.data.fd]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_WRITE);
                }

                if ((ev.events & (PRO_EPOLLIN_SET | PRO_EPOLLHUP)) != 0)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[ev.data.fd]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_READ);
                }

                if ((ev.events & PRO_EPOLLEX_SET) != 0)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[ev.data.fd]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_EXCEPTION);
                }
            } /* end of for (...) */
        }

        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::iterator       itr = handlers.begin();
        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::iterator const end = handlers.end();

        for (; itr != end; ++itr)
        {
            const PRO_INT64         sockId = itr->first;
            const PRO_HANDLER_INFO& info   = itr->second;

            if (PRO_BIT_ENABLED(info.mask, PRO_MASK_ERROR))
            {
                info.handler->OnError(sockId, -1);
                info.handler->Release();
                continue;
            }

            if (PRO_BIT_ENABLED(info.mask, PRO_MASK_WRITE))
            {
                info.handler->OnOutput(sockId);
                info.handler->Release();
            }

            if (PRO_BIT_ENABLED(info.mask, PRO_MASK_READ))
            {
                info.handler->OnInput(sockId);
                info.handler->Release();
            }

            if (PRO_BIT_ENABLED(info.mask, PRO_MASK_EXCEPTION))
            {
                info.handler->OnException(sockId);
                info.handler->Release();
            }
        } /* end of for (...) */
    } /* end of while (...) */
}

void
PRO_CALLTYPE
CProEpollReactor::OnInput(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    const int recvSize = pbsd_recv(sockId, g_s_buffer, sizeof(g_s_buffer), 0); /* connected */
    if (
        (recvSize > 0 && recvSize <= (int)sizeof(g_s_buffer))
        ||
        (recvSize < 0 && pbsd_errno((void*)&pbsd_recv) == PBSD_EWOULDBLOCK)
       )
    {
        m_notifyPipe->EnableNotify();
    }
    else
    {
        OnError(sockId, -1);
    }
}

void
PRO_CALLTYPE
CProEpollReactor::OnError(PRO_INT64 sockId,
                          long      errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_epfd == -1 || m_wantExit ||
            sockId != m_notifyPipe->GetReaderSockId())
        {
            return;
        }

        CProNotifyPipe* const newPipe = new CProNotifyPipe;
        newPipe->Init();

        const PRO_INT64 newSockId = newPipe->GetReaderSockId();
        if (newSockId == -1)
        {
            delete newPipe;

            return;
        }

        pbsd_epoll_event ev;
        memset(&ev, 0, sizeof(pbsd_epoll_event));
        ev.events  = PRO_EPOLLIN_SET;
        ev.data.fd = (int)newSockId;

        if (pbsd_epoll_ctl(m_epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) != 0)
        {
            delete newPipe;

            return;
        }

        if (!m_handlerMgr.AddHandler(newSockId, this, PRO_MASK_READ))
        {
            pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
            delete newPipe;

            return;
        }

        /*
         * unregister old
         */
        ev.data.fd = (int)sockId;
        pbsd_epoll_ctl(m_epfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
        m_handlerMgr.RemoveHandler(sockId, PRO_MASK_READ);
        delete m_notifyPipe;
        m_notifyPipe = NULL;

        /*
         * register new
         */
        m_notifyPipe = newPipe;
    }
}

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_HAS_EPOLL */
