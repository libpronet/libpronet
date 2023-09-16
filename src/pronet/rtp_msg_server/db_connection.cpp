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

#include "db_connection.h"
#include "db_struct.h"
#include "sqlite3.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

static
int
sqlite3_prepare_v2_i(sqlite3*       db,
                     const char*    zSql,
                     int            nByte,
                     sqlite3_stmt** ppStmt,
                     const char**   pzTail)
{
    int err = SQLITE_ERROR;

    for (int i = 0; i < 3000; ++i) /* 3s */
    {
        err = sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail);
        if (err != SQLITE_BUSY && err != SQLITE_LOCKED)
        {
            break;
        }

        ProSleep(1);
    }

    return err;
}

static
int
sqlite3_step_i(sqlite3_stmt* pStmt)
{
    int err = SQLITE_ERROR;

    for (int i = 0; i < 3000; ++i) /* 3s */
    {
        err = sqlite3_step(pStmt);
        if (err != SQLITE_BUSY && err != SQLITE_LOCKED)
        {
            break;
        }

        ProSleep(1);
    }

    return err;
}

static
int
sqlite3_finalize_i(sqlite3_stmt* pStmt)
{
    int err = SQLITE_ERROR;

    for (int i = 0; i < 3000; ++i) /* 3s */
    {
        err = sqlite3_finalize(pStmt);
        if (err != SQLITE_BUSY && err != SQLITE_LOCKED)
        {
            break;
        }

        ProSleep(1);
    }

    return err;
}

/////////////////////////////////////////////////////////////////////////////
////

CDbConnection::CDbConnection()
{
    m_db          = NULL;
    m_transacting = false;
}

CDbConnection::~CDbConnection()
{
    Close();
}

bool
CDbConnection::Open(const char* fileName) /* UTF-8 */
{
    assert(fileName != NULL);
    assert(fileName[0] != '\0');
    if (fileName == NULL || fileName[0] == '\0')
    {
        return false;
    }

    int err = SQLITE_ERROR;

    {
        CProThreadMutexGuard mon(m_lock);

        assert(m_db == NULL);
        if (m_db != NULL)
        {
            return false;
        }

        static bool s_flag = false;
        if (!s_flag)
        {
            err = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
            if (err != SQLITE_OK)
            {
                return false;
            }

            s_flag = true;
        }

        err = sqlite3_open_v2(
            fileName, &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, NULL);
        if (err != SQLITE_OK)
        {
            sqlite3_close_v2(m_db);
            m_db = NULL;
        }
    }

    return err == SQLITE_OK;
}

void
CDbConnection::Close()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_db == NULL)
    {
        return;
    }

    if (m_transacting)
    {
        sqlite3_stmt* stmt = NULL;
        const int err = sqlite3_prepare_v2_i(m_db, "ROLLBACK", -1, &stmt, NULL);
        if (err == SQLITE_OK)
        {
            assert(stmt != NULL);
            sqlite3_step_i(stmt);
        }
        sqlite3_finalize_i(stmt);

        m_transacting = false;
    }

    sqlite3_close_v2(m_db);
    m_db = NULL;
}

bool
CDbConnection::BeginTransaction()
{
    int err = SQLITE_ERROR;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_db == NULL)
        {
            return false;
        }

        assert(!m_transacting);
        if (m_transacting)
        {
            return false;
        }

        sqlite3_stmt* stmt = NULL;
        err = sqlite3_prepare_v2_i(m_db, "BEGIN", -1, &stmt, NULL);
        if (err == SQLITE_OK)
        {
            assert(stmt != NULL);
            err = sqlite3_step_i(stmt);
        }
        sqlite3_finalize_i(stmt);

        if (err == SQLITE_DONE)
        {
            m_transacting = true;
        }
    }

    return err == SQLITE_DONE;
}

bool
CDbConnection::CommitTransaction()
{
    int err = SQLITE_ERROR;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_db == NULL)
        {
            return false;
        }

        assert(m_transacting);
        if (!m_transacting)
        {
            return false;
        }

        sqlite3_stmt* stmt = NULL;
        err = sqlite3_prepare_v2_i(m_db, "COMMIT", -1, &stmt, NULL);
        if (err == SQLITE_OK)
        {
            assert(stmt != NULL);
            err = sqlite3_step_i(stmt);
        }
        sqlite3_finalize_i(stmt);

        if (err == SQLITE_DONE)
        {
            m_transacting = false;
        }
    }

    return err == SQLITE_DONE;
}

void
CDbConnection::RollbackTransaction()
{
    CProThreadMutexGuard mon(m_lock);

    if (m_db == NULL)
    {
        return;
    }

    if (m_transacting)
    {
        sqlite3_stmt* stmt = NULL;
        const int err = sqlite3_prepare_v2_i(m_db, "ROLLBACK", -1, &stmt, NULL);
        if (err == SQLITE_OK)
        {
            assert(stmt != NULL);
            sqlite3_step_i(stmt);
        }
        sqlite3_finalize_i(stmt);

        m_transacting = false;
    }
}

bool
CDbConnection::DoSelect(const char* sql,
                        DB_ROW_SET& rows)
{
    assert(sql != NULL);
    assert(sql[0] != '\0');
    if (sql == NULL || sql[0] == '\0')
    {
        return false;
    }

    const int columns = (int)rows.types_in.size();

    assert(columns > 0);
    if (columns <= 0)
    {
        return false;
    }

    /*
     * check the input parameters
     */
    for (int i = 0; i < columns; ++i)
    {
        const DB_COLUMN_TYPE type = rows.types_in[i];

        assert(type == DB_CT_I64 || type == DB_CT_DBL || type == DB_CT_TXT);
        if (type != DB_CT_I64 && type != DB_CT_DBL && type != DB_CT_TXT)
        {
            return false;
        }
    }

    rows.rows_out.clear();

    bool ret = false;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_db == NULL)
        {
            return false;
        }

        sqlite3_stmt* stmt = NULL;
        int err = sqlite3_prepare_v2_i(m_db, sql, -1, &stmt, NULL);
        if (err != SQLITE_OK)
        {
            sqlite3_finalize_i(stmt);

            return false;
        }

        while (1)
        {
            assert(stmt != NULL);
            err = sqlite3_step_i(stmt);

            if (err == SQLITE_ROW)
            {
                if (sqlite3_data_count(stmt) != columns)
                {
                    break;
                }

                DB_ROW_UNIT row;
                row.cells.resize(columns);

                for (int j = 0; j < columns; ++j)
                {
                    const DB_COLUMN_TYPE type = rows.types_in[j];
                    if (type == DB_CT_I64)
                    {
                        row.cells[j].i64 = sqlite3_column_int64(stmt, j);
                    }
                    else if (type == DB_CT_DBL)
                    {
                        row.cells[j].dbl = sqlite3_column_double(stmt, j);
                    }
                    else
                    {
                        const char* const txt = (char*)sqlite3_column_text(stmt, j);
                        row.cells[j].txt = txt != NULL ? txt : "";
                    }
                }

                rows.rows_out.push_back(row);
            }
            else if (err == SQLITE_DONE)
            {
                ret = true; /* !!! */
                break;
            }
            else
            {
                break;
            }
        } /* end of while () */

        sqlite3_finalize_i(stmt);
    }

    if (!ret)
    {
        rows.rows_out.clear();
    }

    return ret;
}

bool
CDbConnection::DoOther(const char* sql)
{
    assert(sql != NULL);
    assert(sql[0] != '\0');
    if (sql == NULL || sql[0] == '\0')
    {
        return false;
    }

    int err = SQLITE_ERROR;

    {
        CProThreadMutexGuard mon(m_lock);

        if (m_db == NULL)
        {
            return false;
        }

        sqlite3_stmt* stmt = NULL;
        err = sqlite3_prepare_v2_i(m_db, sql, -1, &stmt, NULL);
        if (err == SQLITE_OK)
        {
            assert(stmt != NULL);
            err = sqlite3_step_i(stmt);
        }
        sqlite3_finalize_i(stmt);
    }

    return err == SQLITE_DONE;
}
