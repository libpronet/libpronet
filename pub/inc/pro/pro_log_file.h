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

#if !defined(____PRO_LOG_FILE_H____)
#define ____PRO_LOG_FILE_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_thread_mutex.h"
#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
////

class CProLogFile
{
public:

    CProLogFile();

    ~CProLogFile();

    void Init(const char* fileName, bool append = false);

    void SetGreenLevel(long level);

    long GetGreenLevel() const;

    void SetMaxSize(PRO_INT32 size);

    PRO_INT32 GetMaxSize() const;

    PRO_INT32 GetPos() const;

    void Rewind();

    void Log(const char* text, long level = 0, bool showTime = true);

private:

    FILE*                   m_file;
    long                    m_greenLevel;
    PRO_INT32               m_maxSize;
    mutable CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_LOG_FILE_H____ */
