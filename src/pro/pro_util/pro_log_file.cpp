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

CProLogFile::CProLogFile()
{
    m_file       = NULL;
    m_greenLevel = 0;
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
CProLogFile::Init(const char* fileName,
                  bool        increment) /* = false */
{
    assert(fileName != NULL);
    assert(fileName[0] != '\0');
    if (fileName == NULL || fileName[0] == '\0')
    {
        return;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_file == NULL);
        if (m_file != NULL)
        {
            return;
        }

        if (increment)
        {
            m_file = fopen(fileName, "r+b");
            if (m_file == NULL)
            {
                m_file = fopen(fileName, "wb");
            }

            if (m_file != NULL)
            {
                fseek(m_file, 0, SEEK_END);
            }
        }
        else
        {
            m_file = fopen(fileName, "wb");
        }

#if !defined(WIN32) && !defined(_WIN32_WCE)
        if (m_file != NULL)
        {
            const int fd = fileno(m_file);
            if (fd >= 0)
            {
                pbsd_ioctl_closexec(fd);
            }
        }
#endif
    }
}

void
CProLogFile::SetGreenLevel(long level)
{
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
CProLogFile::Log(const char* text,
                 long        level,    /* = 100 */
                 bool        showTime) /* = true */
{
    CProStlString totalText = "";

    if (showTime)
    {
        ProGetLocalTimeString(totalText);
        totalText += "\n";
    }

    if (text != NULL)
    {
        totalText += text;
    }

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_file != NULL && level >= m_greenLevel)
        {
            fwrite(totalText.c_str(), 1, totalText.length(), m_file);
            fflush(m_file);
        }
    }
}

long
CProLogFile::GetPos() const
{
    long pos = 0;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_file != NULL)
        {
            pos = ftell(m_file);
        }
    }

    return (pos);
}

void
CProLogFile::Rewind()
{
    {
        CProThreadMutexGuard mon(m_lock);

        if (m_file != NULL)
        {
            fseek(m_file, 0, SEEK_SET);
        }
    }
}
