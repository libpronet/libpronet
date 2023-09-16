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
#include "pro_channel_task_pool.h"
#include "pro_functor_command_task.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CProChannelTaskPool::~CProChannelTaskPool()
{
    Stop();
}

bool
CProChannelTaskPool::Start(size_t threadCount)
{
    assert(threadCount > 0);
    if (threadCount == 0)
    {
        return false;
    }

    CProStlMap<CProFunctorCommandTask*, size_t> task2Channels;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_task2Channels.size() == 0);
        if (m_task2Channels.size() != 0)
        {
            return false;
        }

        for (int i = 0; i < (int)threadCount; ++i)
        {
            CProFunctorCommandTask* const task = new CProFunctorCommandTask;
            task2Channels[task] = 0;
            if (!task->Start())
            {
                goto EXIT;
            }
        }

        m_task2Channels = task2Channels;
    }

    return true;

EXIT:

    auto itr = task2Channels.begin();
    auto end = task2Channels.end();

    for (; itr != end; ++itr)
    {
        delete itr->first;
    }

    return false;
}

void
CProChannelTaskPool::Stop()
{
    CProStlMap<CProFunctorCommandTask*, size_t> task2Channels;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task2Channels.size() == 0)
        {
            return;
        }

        m_channelId2Task.clear();
        task2Channels = m_task2Channels;
        m_task2Channels.clear();
    }

    auto itr = task2Channels.begin();
    auto end = task2Channels.end();

    for (; itr != end; ++itr)
    {
        delete itr->first;
    }
}

bool
CProChannelTaskPool::AddChannel(uint64_t channelId)
{
    CProThreadMutexGuard mon(m_lock);

    if (m_task2Channels.size() == 0)
    {
        return false;
    }

    if (m_channelId2Task.find(channelId) != m_channelId2Task.end())
    {
        return true;
    }

    CProFunctorCommandTask* task     = NULL;
    size_t                  channels = 0;

    auto itr = m_task2Channels.begin();
    auto end = m_task2Channels.end();

    for (; itr != end; ++itr)
    {
        if (task == NULL || itr->second < channels)
        {
            task     = itr->first;
            channels = itr->second;
        }
    }

    ++m_task2Channels[task];
    m_channelId2Task[channelId] = task;

    return true;
}

void
CProChannelTaskPool::RemoveChannel(uint64_t channelId)
{
    CProThreadMutexGuard mon(m_lock);

    if (m_task2Channels.size() == 0)
    {
        return;
    }

    auto itr = m_channelId2Task.find(channelId);
    if (itr == m_channelId2Task.end())
    {
        return;
    }

    --m_task2Channels[itr->second];
    m_channelId2Task.erase(itr);
}

bool
CProChannelTaskPool::Put(uint64_t            channelId,
                         IProFunctorCommand* command)
{
    assert(command != NULL);
    if (command == NULL)
    {
        return false;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_task2Channels.size() == 0)
        {
            return false;
        }

        auto itr = m_channelId2Task.find(channelId);
        if (itr == m_channelId2Task.end())
        {
            return false;
        }

        CProFunctorCommandTask* const task = itr->second;
        task->Put(command);
    }

    return true;
}

size_t
CProChannelTaskPool::GetSize() const
{
    size_t size = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        auto itr = m_task2Channels.begin();
        auto end = m_task2Channels.end();

        for (; itr != end; ++itr)
        {
            CProFunctorCommandTask* const task = itr->first;
            size += task->GetSize();
        }
    }

    return size;
}
