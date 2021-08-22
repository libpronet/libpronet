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
#include "pro_log_file.h"
#include "pro_bsd_wrapper.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include "pro_time_util.h"
#include "pro_z.h"
#include <cassert>
#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
////

#define REOPEN_INTERVAL_MS 1000

/////////////////////////////////////////////////////////////////////////////
////

CProLogFile::CProLogFile()
{
    m_fileName   = "";
    m_file       = NULL;
    m_reopenTick = 0;
    m_greenLevel = 0;
    m_maxSize    = 0;
}

CProLogFile::~CProLogFile()
{
    if (m_file != NULL)
    {
        fclose(m_file);
        m_file = NULL;
    }
}

void
CProLogFile::Reinit(const char* fileName,
                    bool        append) /* = false */
{
    assert(fileName != NULL);
    assert(fileName[0] != '\0');
    if (fileName == NULL || fileName[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        m_fileName = fileName;
        Reopen(append); /* open the file at this moment */
    }
}

void
CProLogFile::SetGreenLevel(long level)
{
    level = level < PRO_LL_MIN ? PRO_LL_MIN : level;
    level = level > PRO_LL_MAX ? PRO_LL_MAX : level;

    {
        CProThreadMutexGuard mon(m_lock);

        m_greenLevel = level;
    }
}

long
CProLogFile::GetGreenLevel() const
{
    long level = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        level = m_greenLevel;
    }

    return (level);
}

void
CProLogFile::SetMaxSize(PRO_INT32 size)
{
    {
        CProThreadMutexGuard mon(m_lock);

        m_maxSize = size;
    }
}

PRO_INT32
CProLogFile::GetMaxSize() const
{
    PRO_INT32 size = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        size = m_maxSize;
    }

    return (size);
}

PRO_INT32
CProLogFile::GetSize() const
{
    PRO_INT32 size = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_file != NULL)
        {
            size = (PRO_INT32)ftell(m_file);
        }
    }

    return (size);
}

void
CProLogFile::Log(const char* text,
                 long        level,
                 bool        showTime)
{
    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    CProStlString totalString = "";

    if (showTime)
    {
        CProStlString timeString = "";
        ProGetLocalTimeString(timeString);

        totalString += "\n";
        totalString += timeString;
        totalString += " ";
    }

    totalString += text;

    {
        CProThreadMutexGuard mon(m_lock);

        if (level < m_greenLevel)
        {
            return;
        }

        if (m_file == NULL &&
            ProGetTickCount64() - m_reopenTick >= REOPEN_INTERVAL_MS)
        {
            Reopen(true); /* reopen the file at this moment */
        }
        if (m_file == NULL)
        {
            return;
        }

        const PRO_INT32 pos = (PRO_INT32)ftell(m_file);
        if (
            pos < 0
            ||
            (m_maxSize > 0 && pos >= m_maxSize)
           )
        {
            fclose(m_file);
            m_file = NULL;

            Move_1();
            Reopen(false); /* remake the file at this moment */
        }
        if (m_file == NULL)
        {
            return;
        }

        const size_t ret = fwrite(
            totalString.c_str(), 1, totalString.length(), m_file);
        if (ret != totalString.length())
        {
            fclose(m_file);
            m_file = NULL;
        }
        else
        {
            fflush(m_file);
        }
    }
}

void
CProLogFile::Reopen(bool append)
{
    if (m_file != NULL)
    {
        fclose(m_file);
        m_file = NULL;
    }

    if (append)
    {
        m_file = fopen(m_fileName.c_str(), "r+b");
        if (m_file == NULL)
        {
            m_file = fopen(m_fileName.c_str(), "wb");
        }

        if (m_file != NULL)
        {
            fseek(m_file, 0, SEEK_END);
        }
    }
    else
    {
        m_file = fopen(m_fileName.c_str(), "wb");
    }

#if !defined(_WIN32) && !defined(_WIN32_WCE)
    if (m_file != NULL)
    {
        /*
         * Allow other users to access the file. "Write" permissions are required.
         */
        chmod(m_fileName.c_str(),
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        const int fd = fileno(m_file);
        if (fd >= 0)
        {
            pbsd_ioctl_closexec(fd);
        }
    }
#endif

    m_reopenTick = ProGetTickCount64();
}

void
CProLogFile::Move_1()
{
    if (m_fileName.empty())
    {
        return;
    }

    CProStlString fileName1 = m_fileName;
    fileName1 += ".1"; /* xxx.log to xxx.log.1 */

    remove(fileName1.c_str());
    rename(m_fileName.c_str(), fileName1.c_str());
}
