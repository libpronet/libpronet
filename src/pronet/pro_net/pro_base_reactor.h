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

#if !defined(PRO_BASE_REACTOR_H)
#define PRO_BASE_REACTOR_H

#include "pro_event_handler.h"
#include "pro_handler_mgr.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProNotifyPipe;

/////////////////////////////////////////////////////////////////////////////
////

class CProBaseReactor : public CProEventHandler
{
public:

    CProBaseReactor();

    virtual ~CProBaseReactor()
    {
    }

    virtual bool PRO_CALLTYPE Init() = 0;

    virtual void PRO_CALLTYPE Fini() = 0;

    virtual bool PRO_CALLTYPE AddHandler(
        PRO_INT64         sockId,
        CProEventHandler* handler,
        unsigned long     mask
        ) = 0;

    virtual void PRO_CALLTYPE RemoveHandler(
        PRO_INT64     sockId,
        unsigned long mask
        ) = 0;

    virtual unsigned long PRO_CALLTYPE GetHandlerCount() const;

    virtual void PRO_CALLTYPE WorkerRun() = 0;

protected:

    virtual unsigned long PRO_CALLTYPE AddRef()
    {
        return (1);
    }

    virtual unsigned long PRO_CALLTYPE Release()
    {
        return (1);
    }

protected:

    PRO_UINT64              m_threadId;
    bool                    m_wantExit;
    CProHandlerMgr          m_handlerMgr;
    CProNotifyPipe*         m_notifyPipe;
    mutable CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_BASE_REACTOR_H */
