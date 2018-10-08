/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (http://www.libpro.org)
 */

#include "pro_a.h"
#include "pro_functor_command_task.h"
#include "pro_functor_command.h"
#include "pro_stl.h"
#include "pro_thread.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CProFunctorCommandTask::CProFunctorCommandTask()
{
    m_userData = NULL;
    m_threadId = 0;
    m_wantExit = false;
}

CProFunctorCommandTask::~CProFunctorCommandTask()
{
    Stop();
}

bool
CProFunctorCommandTask::Start(bool realtime) /* = false */
{{
    CProThreadMutexGuard mon(m_lockAtom);

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_threadId == 0);
        if (m_threadId != 0)
        {
            return (false);
        }

        if (!Spawn(realtime))
        {
            return (false);
        }

        while (m_threadId == 0)
        {
            m_initCond.Wait(&m_lock);
        }
    }

    return (true);
}}

void
CProFunctorCommandTask::Stop()
{{
    CProThreadMutexGuard mon(m_lockAtom);

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_threadId == 0)
        {
            return;
        }

        assert(ProGetThreadId() != m_threadId); /* deadlock */

        m_wantExit = true;
        m_commandCond.Signal();
    }

    Wait();

    {
        CProThreadMutexGuard mon(m_lock);

        m_userData = NULL;
        m_threadId = 0;
        m_wantExit = false;
    }
}}

bool
CProFunctorCommandTask::Put(IProFunctorCommand* command,
                            bool                blocking) /* = false */
{
    assert(command != NULL);
    if (command == NULL)
    {
        return (false);
    }

    CProThreadMutexCondition cond;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_threadId == 0 || m_wantExit)
        {
            return (false);
        }

        if (blocking)
        {
            command->SetUserData(&cond);
        }
        else
        {
            command->SetUserData(NULL);
        }

        m_commands.push_back(command);
        m_commandCond.Signal();
    }

    if (blocking)
    {
        cond.Wait(NULL);
    }

    return (true);
}

unsigned long
CProFunctorCommandTask::GetSize() const
{
    unsigned long size = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        size = (unsigned long)m_commands.size();
    }

    return (size);
}

PRO_UINT64
CProFunctorCommandTask::GetThreadId() const
{
    PRO_UINT64 threadId = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        threadId = m_threadId;
    }

    return (threadId);
}

void
CProFunctorCommandTask::SetUserData(const void* userData)
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_userData = userData;
    }
}

const void*
CProFunctorCommandTask::GetUserData() const
{
    const void* userData = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        userData = m_userData;
    }

    return (userData);
}

void
CProFunctorCommandTask::Svc()
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_threadId = ProGetThreadId();
        m_initCond.Signal();
    }

    CProStlDeque<IProFunctorCommand*> commands;

    while (1)
    {
        IProFunctorCommand* command = NULL;

        {
            CProThreadMutexGuard mon(m_lock);

            while (1)
            {
                if (m_wantExit || m_commands.size() > 0)
                {
                    break;
                }

                m_commandCond.Wait(&m_lock);
            }

            if (m_wantExit)
            {
                commands = m_commands;
                m_commands.clear();
                break;
            }

            command = m_commands.front();
            m_commands.pop_front();
        }

        command->Execute();

        CProThreadMutexCondition* const cond = (CProThreadMutexCondition*)command->GetUserData();
        if (cond != NULL)
        {
            cond->Signal();
        }

        command->Destroy();
    } /* end of while (...) */

    int       i = 0;
    const int c = (int)commands.size();

    for (; i < c; ++i)
    {
        IProFunctorCommand* const command = commands[i];
        command->Execute();

        CProThreadMutexCondition* const cond = (CProThreadMutexCondition*)command->GetUserData();
        if (cond != NULL)
        {
            cond->Signal();
        }

        command->Destroy();
    }
}
