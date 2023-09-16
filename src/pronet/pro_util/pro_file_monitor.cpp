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
#include "pro_file_monitor.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_time_util.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define MONITOR_INTERVAL 10

/////////////////////////////////////////////////////////////////////////////
////

CProFileMonitor::CProFileMonitor()
{
    m_updateTick = 0;
    m_exist      = false;
}

void
CProFileMonitor::Init(const char* fileName)
{
    assert(fileName != NULL);
    assert(fileName[0] != '\0');
    if (fileName == NULL || fileName[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_fileName.empty());
        if (!m_fileName.empty())
        {
            return;
        }

        m_fileName = fileName;
    }
}

void
CProFileMonitor::UpdateFileExist()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_fileName.empty())
        {
            return;
        }

        const int64_t tick = ProGetTickCount64();
        if (tick - m_updateTick < MONITOR_INTERVAL * 1000)
        {
            return;
        }

        m_updateTick = tick;
    }

    FILE* const file = fopen(m_fileName.c_str(), "rb");
    if (file != NULL)
    {
        fclose(file);
    }

    {
        CProThreadMutexGuard mon(m_lock);

        m_exist = file != NULL;
    }
}

bool
CProFileMonitor::QueryFileExist() const
{
    bool exist = false;

    {
        CProThreadMutexGuard mon(m_lock);

        exist = m_exist;
    }

    return exist;
}
