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
#include "pro_command_task.h"
#include "pro_command.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CProCommandTask::CProCommandTask()
: m_commandCond(true) /* isSocketMode is true */
{
    m_userData       = NULL;
    m_threadCount    = 0;
    m_curThreadCount = 0;
    m_wantExit       = false;
}

CProCommandTask::~CProCommandTask()
{
    Stop();
}

bool
CProCommandTask::Start(bool         realtime,    /* = false */
                       unsigned int threadCount) /* = 1 */
{{
    CProThreadMutexGuard mon(m_lockAtom);

    assert(threadCount > 0);
    if (threadCount == 0)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_threadCount == 0);
        if (m_threadCount != 0)
        {
            return false;
        }

        m_threadCount = threadCount; /* for StopMe() */

        for (int i = 0; i < (int)threadCount; ++i)
        {
            if (!Spawn(realtime))
            {
                goto EXIT;
            }
        }

        while (m_curThreadCount < threadCount)
        {
            m_initCond.Wait(&m_lock);
        }
    }

    return true;

EXIT:

    StopMe();

    return false;
}}

void
CProCommandTask::Stop()
{{
    CProThreadMutexGuard mon(m_lockAtom);

    StopMe();
}}

void
CProCommandTask::StopMe()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_threadCount == 0)
    {
        return;
    }

    assert(m_threadIds.find(ProGetThreadId()) == m_threadIds.end()); /* deadlock */

    m_wantExit = true;

    while (GetThreadCount() > 0)
    {
        m_commandCond.Signal();

        m_lock.Unlock();
        Wait1();
        m_lock.Lock();
    }

    m_userData       = NULL;
    m_threadCount    = 0;
    m_curThreadCount = 0;
    m_wantExit       = false;
}

bool
CProCommandTask::Put(CProCommand* command,
                     bool         blocking) /* = false */
{
    assert(command != NULL);
    if (command == NULL)
    {
        return false;
    }

    CProThreadMutexCondition* cond = NULL;
    CProThreadMutex*          lock = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_threadCount == 0 || m_curThreadCount == 0 || m_wantExit)
        {
            return false;
        }

        if (blocking)
        {
            cond = new CProThreadMutexCondition;
            lock = new CProThreadMutex;
        }

        command->SetUserData1(cond);
        command->SetUserData2(lock);

        m_commands.push_back(command);
        m_commandCond.Signal();
    }

    if (blocking)
    {
        lock->Lock();
        cond->Wait(lock);
        lock->Unlock();
    }

    return true;
}

size_t
CProCommandTask::GetSize() const
{
    size_t size = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        size = m_commands.size();
    }

    return size;
}

bool
CProCommandTask::IsCurrentThread() const
{
    CProThreadMutexGuard mon(m_lock);

    if (m_threadCount == 0 || m_curThreadCount == 0 || m_threadIds.size() == 0)
    {
        return false;
    }

    assert(m_threadCount == 1);

    uint64_t taskThreadId = *m_threadIds.begin();
    uint64_t currThreadId = ProGetThreadId();

    return taskThreadId == currThreadId;
}

void
CProCommandTask::SetUserData(const void* userData)
{
    CProThreadMutexGuard mon(m_lock);

    m_userData = userData;
}

const void*
CProCommandTask::GetUserData() const
{
    const void* userData = NULL;

    {
        CProThreadMutexGuard mon(m_lock);

        userData = m_userData;
    }

    return userData;
}

void
CProCommandTask::Svc()
{
    uint64_t threadId = ProGetThreadId();

    {
        CProThreadMutexGuard mon(m_lock);

        ++m_curThreadCount;
        m_threadIds.insert(threadId);
        m_initCond.Signal();
    }

    CProStlDeque<CProCommand*> commands;

    while (1)
    {
        CProCommand* command = NULL;

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

        CProThreadMutexCondition* cond = (CProThreadMutexCondition*)command->GetUserData1();
        CProThreadMutex*          lock = (CProThreadMutex*)         command->GetUserData2();
        if (cond != NULL && lock != NULL)
        {
            lock->Lock();
            cond->Signal();
            lock->Unlock();
        }

        command->Destroy();
    } /* end of while () */

    int i = 0;
    int c = (int)commands.size();

    for (; i < c; ++i)
    {
        CProCommand* command = commands[i];
        command->Execute();

        CProThreadMutexCondition* cond = (CProThreadMutexCondition*)command->GetUserData1();
        CProThreadMutex*          lock = (CProThreadMutex*)         command->GetUserData2();
        if (cond != NULL && lock != NULL)
        {
            lock->Lock();
            cond->Signal();
            lock->Unlock();
        }

        command->Destroy();
    }

    {
        CProThreadMutexGuard mon(m_lock);

        m_threadIds.erase(threadId);
    }
}
