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
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_notify_pipe.h"
#include "../pro_util/pro_thread.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

static
inline
bool
pbsd_fd_preset_i(int64_t      fd,
                 pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return false;
    }

#if defined(_WIN32)
    return fds.fd_count < FD_SETSIZE;
#else
    return fd < FD_SETSIZE;
#endif
}

static
inline
void
pbsd_fd_set_i(int64_t      fd,
              pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32)
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
pbsd_fd_clr_i(int64_t      fd,
              pbsd_fd_set& fds)
{
    if (fd == -1)
    {
        return;
    }

#if defined(_WIN32)
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
}

bool
CProSelectReactor::Init()
{
    CProThreadMutexGuard mon(m_lock);

    m_notifyPipe->Init();

    int64_t sockId = m_notifyPipe->GetReaderSockId();
    if (sockId == -1)
    {
        return false;
    }

    if (!pbsd_fd_preset_i(sockId, m_fdsRd[0]))
    {
        return false;
    }

    if (!m_handlerMgr.AddHandler(sockId, this, PRO_MASK_READ))
    {
        return false;
    }

    pbsd_fd_set_i(sockId, m_fdsRd[0]);

    return true;
}

void
CProSelectReactor::Fini()
{
    CProThreadMutexGuard mon(m_lock);

    m_wantExit = true;
    m_notifyPipe->Notify();
}

bool
CProSelectReactor::AddHandler(int64_t           sockId,
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
        return false;
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
            return false;
        }

        PRO_HANDLER_INFO oldInfo = m_handlerMgr.FindHandler(sockId);
        if (oldInfo.handler != NULL && handler != oldInfo.handler)
        {
            return false;
        }

        mask &= ~oldInfo.mask;
        if (mask == 0)
        {
            return true;
        }

        /*
         * preset
         */
        if (PRO_BIT_ENABLED(mask, PRO_MASK_WRITE))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsWr[0]))
            {
                return false;
            }
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_READ))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsRd[0]))
            {
                return false;
            }
        }
        if (PRO_BIT_ENABLED(mask, PRO_MASK_EXCEPTION))
        {
            if (!pbsd_fd_preset_i(sockId, m_fdsEx[0]))
            {
                return false;
            }
        }

        if (!m_handlerMgr.AddHandler(sockId, handler, mask))
        {
            return false;
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

    return true;
}

void
CProSelectReactor::RemoveHandler(int64_t       sockId,
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

        PRO_HANDLER_INFO oldInfo = m_handlerMgr.FindHandler(sockId);
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
CProSelectReactor::WorkerRun()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_threadId = ProGetThreadId();
    }

    while (1)
    {
        int64_t maxSockId = -1;

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
         * select()
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

            CProStlMap<int64_t, PRO_HANDLER_INFO> sockId2HandlerInfo;

            {
                CProThreadMutexGuard mon(m_lock);

                if (m_wantExit)
                {
                    break;
                }

                sockId2HandlerInfo = m_handlerMgr.GetAllHandlers();

                auto itr = sockId2HandlerInfo.begin();
                auto end = sockId2HandlerInfo.end();

                for (; itr != end; ++itr)
                {
                    const PRO_HANDLER_INFO& info = itr->second;
                    info.handler->AddRef();
                }
            }

            auto itr = sockId2HandlerInfo.begin();
            auto end = sockId2HandlerInfo.end();

            for (; itr != end; ++itr)
            {
                int64_t                 sockId = itr->first;
                const PRO_HANDLER_INFO& info   = itr->second;

                PBSD_FD_ZERO(&m_fdsRd[1]);
                pbsd_fd_set_i(sockId, m_fdsRd[1]);

                struct timeval timeout = { 0 };

                retc = pbsd_select(sockId + 1, &m_fdsRd[1], NULL, NULL, &timeout);
                if (retc >= 0)
                {
                    info.handler->Release();
                    continue;
                }

                /*
                 * This descriptor is not a socket.
                 */
                if (pbsd_errno((void*)&pbsd_select) == PBSD_EBADF)
                {
                    info.handler->OnError(sockId, PBSD_EBADF);
                }

                info.handler->Release();
            } /* end of for () */

            /*
             * I am fine now.
             */
            continue;
        }

        CProStlMap<int64_t, PRO_HANDLER_INFO> handlers;

        {
            CProThreadMutexGuard mon(m_lock);

            if (m_wantExit)
            {
                break;
            }

#if defined(_WIN32)

            for (int i = 0; i < (int)m_fdsWr[1].fd_count; ++i)
            {
                int64_t          sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (int64_t)m_fdsWr[1].fd_array[i];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (int32_t)m_fdsWr[1].fd_array[i];
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
                int64_t          sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (int64_t)m_fdsRd[1].fd_array[j];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (int32_t)m_fdsRd[1].fd_array[j];
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
                int64_t          sockId = -1;
                PRO_HANDLER_INFO info;

                if (sizeof(SOCKET) == 8)
                {
                    sockId = (int64_t)m_fdsEx[1].fd_array[k];
                    info   = m_handlerMgr.FindHandler(sockId);
                }
                else
                {
                    sockId = (int32_t)m_fdsEx[1].fd_array[k];
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

#else  /* _WIN32 */

            const CProStlMap<int64_t, PRO_HANDLER_INFO>& sockId2HandlerInfo =
                m_handlerMgr.GetAllHandlers();

            auto itr = sockId2HandlerInfo.begin();
            auto end = sockId2HandlerInfo.end();

            for (; itr != end; ++itr)
            {
                int64_t                 sockId = itr->first;
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
            } /* end of for () */

#endif /* _WIN32 */
        }

        auto itr = handlers.begin();
        auto end = handlers.end();

        for (; itr != end; ++itr)
        {
            int64_t                 sockId = itr->first;
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
        } /* end of for () */
    } /* end of while () */
}

void
CProSelectReactor::OnInput(int64_t sockId)
{
    assert(sockId != -1);
    if (sockId == -1)
    {
        return;
    }

    if (m_notifyPipe->Roger())
    {
        return;
    }

    OnError(sockId, -1);
}

void
CProSelectReactor::OnError(int64_t sockId,
                           int     errorCode)
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

        CProNotifyPipe* newPipe = new CProNotifyPipe;
        newPipe->Init();

        int64_t newSockId = newPipe->GetReaderSockId();
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
