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

#ifndef ____PRO_CHANNEL_TASK_POOL_H____
#define ____PRO_CHANNEL_TASK_POOL_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProFunctorCommandTask;
class IProFunctorCommand;

/////////////////////////////////////////////////////////////////////////////
////

class CProChannelTaskPool
{
public:

    CProChannelTaskPool()
    {
    }

    ~CProChannelTaskPool();

    bool Start(unsigned int threadCount);

    void Stop();

    /*
     * the 'channelId' can be 0
     */
    bool AddChannel(uint64_t channelId);

    void RemoveChannel(uint64_t channelId);

    bool Put(
        uint64_t            channelId,
        IProFunctorCommand* command
        );

    size_t GetSize() const;

private:

    CProStlMap<CProFunctorCommandTask*, size_t>   m_task2Channels;
    CProStlMap<uint64_t, CProFunctorCommandTask*> m_channelId2Task;
    mutable CProThreadMutex                       m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_CHANNEL_TASK_POOL_H____ */
