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

#include "pro_base_reactor.h"
#include "pro_event_handler.h"
#include "pro_handler_mgr.h"
#include "pro_notify_pipe.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

CProBaseReactor::CProBaseReactor()
{
    m_threadId   = 0;
    m_wantExit   = false;
    m_notifyPipe = new CProNotifyPipe;
}

unsigned long
CProBaseReactor::GetHandlerCount() const
{
    unsigned long count = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        count = m_handlerMgr.GetHandlerCount();
    }

    return (count);
}
