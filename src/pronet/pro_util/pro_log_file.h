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

#if !defined(____PRO_LOG_FILE_H____)
#define ____PRO_LOG_FILE_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread_mutex.h"
#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
////

#define PRO_LL_MIN   -9
#define PRO_LL_DEBUG -1
#define PRO_LL_INFO   0
#define PRO_LL_WARN   1
#define PRO_LL_ERROR  2
#define PRO_LL_FATAL  3
#define PRO_LL_MAX    9

/////////////////////////////////////////////////////////////////////////////
////

class CProLogFile
{
public:

    CProLogFile();

    ~CProLogFile();

    void Reinit(
        const char* fileName,
        bool        append = false
        );

    void SetGreenLevel(long level);

    long GetGreenLevel() const;

    /*
     * the "size" can be <= 0
     */
    void SetMaxSize(PRO_INT32 size);

    PRO_INT32 GetMaxSize() const;

    PRO_INT32 GetSize() const;

    void Log(
        const char* text,
        long        level,
        bool        showTime
        );

private:

    void Reopen(bool append);

    void Move_1();

private:

    CProStlString           m_fileName;
    FILE*                   m_file;
    PRO_INT64               m_reopenTick;
    long                    m_greenLevel;
    PRO_INT32               m_maxSize;
    mutable CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_LOG_FILE_H____ */
