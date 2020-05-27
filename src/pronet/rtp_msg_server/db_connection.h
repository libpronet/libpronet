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

#if !defined(DB_CONNECTION_H)
#define DB_CONNECTION_H

#include "db_struct.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

struct sqlite3;

/////////////////////////////////////////////////////////////////////////////
////

class CDbConnection
{
public:

    CDbConnection();

    ~CDbConnection();

    bool Open(const char* fileName); /* UTF-8 */

    void Close();

    bool BeginTransaction();

    bool CommitTransaction();

    void RollbackTransaction();

    bool DoSelect(
        const char* sql,
        DB_ROW_SET& rows
        );

    bool DoOther(const char* sql);

private:

    sqlite3*        m_db;
    bool            m_transacting;
    CProThreadMutex m_lock;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* DB_CONNECTION_H */
