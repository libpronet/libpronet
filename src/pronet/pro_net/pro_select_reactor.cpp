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

#include "pro_select_reactor.h"
#include "pro_base_reactor.h"
#include "pro_notify_pipe.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

static char g_s_buffer[1024];

/////////////////////////////////////////////////////////////////////////////
////

static
inline
bool
PRO_CALLTYPE
pbsd_fd_preset_i(PRO_INT64    fd,
                 pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return (false);
    }

#if defined(WIN32) || defined(_WIN32_WCE)
    return (fds.fd_count < FD_SETSIZE);
#else
    return (fd < FD_SETSIZE);
#endif
}

static
inline
void
PRO_CALLTYPE
pbsd_fd_set_i(PRO_INT64    fd,
              pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return;
    }

#if defined(WIN32) || defined(_WIN32_WCE)
    if (fds.fd_count < FD_SETSIZE)
    {
        fds.fd_array[fds.fd_count] = (SOCKET)fd;
        ++fds.fd_count;
    }
#else
    if (fd < FD_SETSIZE)
    {
        PBSD_FD_SET(fd, &fds);
    }
#endif
}

static
inline
void
PRO_CALLTYPE
pbsd_fd_clr_i(PRO_INT64    fd,
              pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return;
    }

#if defined(WIN32) || defined(_WIN32_WCE)
    for (int i = 0; i < (int)fds.fd_count; ++i)
    {
        if (fds.fd_array[i] == (SOCKET)fd)
        {
            fds.fd_array[i] = fds.fd_array[fds.fd_count - 1];
            --fds.fd_count;
            break;
        }
    }
#else
    if (fd < FD_SETSIZE)
    {
        PBSD_FD_CLR(fd, &fds);
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////
////

CProSelectReactor::CProSelectReactor()
{
    PBSD_FD_ZERO(&m_fdsWr[0]);
    PBSD_FD_ZERO(&m_fdsRd[0]);
    PBSD_FD_ZERO(&m_fdsEx[0]);
}

CProSelectReactor::~CProSelectReactor()
{
    Fini();

    delete m_notifyPipe;
    m_notifyPipe = NULL;
}

bool
PRO_CALLTYPE
CProSelectReactor::Init()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_notifyPipe->Init();

        const PRO_INT64 sockId = m_notifyPipe->GetReaderSockId();
        if (sockId == -1)
        {
            return (false);
        }

        if (!pbsd_fd_preset_i(sockId, m_fdsRd[0]))
        {
            return (false);
        }

        if (!m_handlerMgr.AddHandler(sockId, this, PRO_MASK_READ))
        {
            return (false);
        }

        pbsd_fd_set_i(sockId, m_fdsRd[0]);
    }

    return (true);
}

void
PRO_CALLTYPE
CProSelectReactor::Fini()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_wantExit = true;
        m_notifyPipe->Notify();
    }
}

bool
PRO_CALLTYPE
CProSelectReactor::AddHandler(PRO_INT64         sockId,
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
        PRO_SET_BITS(mask, PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_wantExit)
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

        /*
         * preset
         */
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsWr[0]))
            {
                return (false);
            }
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsRd[0]))
            {
                return (false);
            }
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsEx[0]))
            {
                return (false);
            }
        }

        if (!m_handlerMgr.AddHandler(sockId, handler, mask))
        {
            return (false);
        }

        /*
         * set
         */
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            pbsd_fd_set_i(sockId, m_fdsWr[0]);
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            pbsd_fd_set_i(sockId, m_fdsRd[0]);
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            pbsd_fd_set_i(sockId, m_fdsEx[0]);
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
CProSelectReactor::RemoveHandler(PRO_INT64     sockId,
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
        PRO_SET_BITS(mask, PRO_MASK_WRITE | PRO_MASK_READ | PRO_MASK_EXCEPTION);
    }

    {
        CProThreadMutexGuard mon(m_lock);

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

        /*
         * clr
         */
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            pbsd_fd_clr_i(sockId, m_fdsWr[0]);
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            pbsd_fd_clr_i(sockId, m_fdsRd[0]);
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            pbsd_fd_clr_i(sockId, m_fdsEx[0]);
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
CProSelectReactor::WorkerRun()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_threadId = ProGetThreadId();
    }

    while (1)
    {
        PRO_INT64 maxSockId = -1;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_wantExit)
            {
                break;
            }

            maxSockId  = m_handlerMgr.GetMaxSockId();
            m_fdsWr[1] = m_fdsWr[0];
            m_fdsRd[1] = m_fdsRd[0];
            m_fdsEx[1] = m_fdsEx[0];
        }

        /*
         * select(...)
         */
        int retc = pbsd_select(maxSockId + 1, &m_fdsRd[1], &m_fdsWr[1], &m_fdsEx[1], NULL);
        if (retc == 0)
        {
            ProSleep(1);
            continue;
        }

        if (retc < 0)
        {
            if (pbsd_errno((void*)&pbsd_select) != PBSD_EBADF)
            {
                ProSleep(1);
                continue;
            }

            CProStlMap<PRO_INT64, PRO_HANDLER_INFO> allHandlers;

            {
                CProThreadMutexGuard mon(m_lock);

                if (m_wantExit)
                {
                    break;
                }

                allHandlers = m_handlerMgr.GetAllHandlers();

                CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator       itr = allHandlers.begin();
                CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator const end = allHandlers.end();

                for (; itr != end; ++itr)
                {
                    const PRO_HANDLER_INFO& info = itr->second;
                    info.handler->AddRef();
                }
            }

            CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator       itr = allHandlers.begin();
            CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator const end = allHandlers.end();

            for (; itr != end; ++itr)
            {
                const PRO_INT64         sockId = itr->first;
                const PRO_HANDLER_INFO& info   = itr->second;

                PBSD_FD_ZERO(&m_fdsRd[1]);
                pbsd_fd_set_i(sockId, m_fdsRd[1]);

                struct timeval tv;
                tv.tv_sec  = 0;
                tv.tv_usec = 0;

                retc = pbsd_select(sockId + 1, &m_fdsRd[1], NULL, NULL, &tv);
                if (retc >= 0)
                {
                    info.handler->Release();
                    continue;
                }

                if (pbsd_errno((void*)&pbsd_select) != PBSD_EBADF)
                {
                    info.handler->Release();
                    continue;
                }

                /*
                 * This descriptor is not a socket.
                 */
                info.handler->OnError(sockId, PBSD_EBADF);
                info.handler->Release();
            } /* end of for (...) */

            /*
             * I am fine now.
             */
            continue;
        }

        CProStlMap<PRO_INT64, PRO_HANDLER_INFO> handlers;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_wantExit)
            {
                break;
            }

#if defined(WIN32) || defined(_WIN32_WCE)

            for (int i = 0; i < (int)m_fdsWr[1].fd_count; ++i)
            {
                PRO_INT64        sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (PRO_INT64)m_fdsWr[1].fd_array[i];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (PRO_INT32)m_fdsWr[1].fd_array[i];
                    info   = m_handlerMgr.FindHandler(sockId);
                }

                if (info.handler != NULL)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_WRITE);
                }
            }

            for (int j = 0; j < (int)m_fdsRd[1].fd_count; ++j)
            {
                PRO_INT64        sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (PRO_INT64)m_fdsRd[1].fd_array[j];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (PRO_INT32)m_fdsRd[1].fd_array[j];
                    info   = m_handlerMgr.FindHandler(sockId);
                }

                if (info.handler != NULL)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_READ);
                }
            }

            for (int k = 0; k < (int)m_fdsEx[1].fd_count; ++k)
            {
                PRO_INT64        sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (PRO_INT64)m_fdsEx[1].fd_array[k];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (PRO_INT32)m_fdsEx[1].fd_array[k];
                    info   = m_handlerMgr.FindHandler(sockId);
                }

                if (info.handler != NULL)
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_EXCEPTION);
                }
            }

#else  /* WIN32, _WIN32_WCE */

            const CProStlMap<PRO_INT64, PRO_HANDLER_INFO>& allHandlers =
                m_handlerMgr.GetAllHandlers();

            CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator       itr = allHandlers.begin();
            CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator const end = allHandlers.end();

            for (; itr != end; ++itr)
            {
                const PRO_INT64         sockId = itr->first;
                const PRO_HANDLER_INFO& info   = itr->second;

                if (PBSD_FD_ISSET(sockId, &m_fdsWr[1]))
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_WRITE);
                }

                if (PBSD_FD_ISSET(sockId, &m_fdsRd[1]))
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_READ);
                }

                if (PBSD_FD_ISSET(sockId, &m_fdsEx[1]))
                {
                    info.handler->AddRef();
                    PRO_HANDLER_INFO& info2 = handlers[sockId]; /* insert */
                    info2.handler = info.handler;
                    PRO_SET_BITS(info2.mask, PRO_MASK_EXCEPTION);
                }
            } /* end of for (...) */

#endif /* WIN32, _WIN32_WCE */
        }

        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator       itr = handlers.begin();
        CProStlMap<PRO_INT64, PRO_HANDLER_INFO>::const_iterator const end = handlers.end();

        for (; itr != end; ++itr)
        {
            const PRO_INT64         sockId = itr->first;
            const PRO_HANDLER_INFO& info   = itr->second;

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
CProSelectReactor::OnInput(PRO_INT64 sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    const int recvSize = pbsd_recv(sockId, g_s_buffer, sizeof(g_s_buffer), 0); /* connected */
    if (recvSize > 0 && recvSize <= (int)sizeof(g_s_buffer)
        ||
        recvSize < 0 && pbsd_errno((void*)&pbsd_recv) == PBSD_EWOULDBLOCK)
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
CProSelectReactor::OnError(PRO_INT64 sockId,
                           long      errorCode)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_wantExit || sockId != m_notifyPipe->GetReaderSockId())
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

        if (!pbsd_fd_preset_i(newSockId, m_fdsRd[0]))
        {
            delete newPipe;

            return;
        }

        if (!m_handlerMgr.AddHandler(newSockId, this, PRO_MASK_READ))
        {
            delete newPipe;

            return;
        }

        pbsd_fd_set_i(newSockId, m_fdsRd[0]);

        /*
         * unregister old
         */
        pbsd_fd_clr_i(sockId, m_fdsRd[0]);
        m_handlerMgr.RemoveHandler(sockId, PRO_MASK_READ);
        delete m_notifyPipe;
        m_notifyPipe = NULL;

        /*
         * register new
         */
        m_notifyPipe = newPipe;
    }
}
