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

#ifndef ____PRO_NOTIFY_PIPE_H____
#define ____PRO_NOTIFY_PIPE_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProNotifyPipe
{
public:

    CProNotifyPipe();

    ~CProNotifyPipe();

    void Init();

    void Fini();

    int64_t GetReaderSockId() const
    {
        return m_sockIds[0];
    }

    int64_t GetWriterSockId() const
    {
        return m_sockIds[1];
    }

    void Notify();

    bool Roger();

private:

    int64_t m_sockIds[2];
    bool    m_signal;
    int64_t m_notifyTick;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_NOTIFY_PIPE_H____ */
