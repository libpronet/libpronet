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

#include "msg_db.h"
#include "db_connection.h"
#include "db_struct.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_rtp/rtp_msg.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

long
GetMsgUserRow(CDbConnection&      db,
              const RTP_MSG_USER& user,
              TBL_MSG_USER_ROW&   row)
{
    char sql[1024] = "";
    snprintf_pro(
        sql,
        sizeof(sql),
        " SELECT _cid_, _uid_, _maxiids_, _isc2s_, _passwd_, _bindedip_ "
        " FROM tbl_msg01_user WHERE _cid_=%u AND _uid_=%llu ",
        (unsigned int)user.classId,
        (unsigned long long)user.UserId()
        );

    DB_ROW_SET dbrows;
    dbrows.types_in.push_back(DB_CT_I64); /* _cid_ */
    dbrows.types_in.push_back(DB_CT_I64); /* _uid_ */
    dbrows.types_in.push_back(DB_CT_I64); /* _maxiids_ */
    dbrows.types_in.push_back(DB_CT_I64); /* _isc2s_ */
    dbrows.types_in.push_back(DB_CT_TXT); /* _passwd_ */
    dbrows.types_in.push_back(DB_CT_TXT); /* _bindedip_ */

    if (!db.DoSelect(sql, dbrows))
    {
        return -1;
    }

    if (dbrows.rows_out.size() == 0)
    {
        return 0;
    }

    {
        const DB_ROW_UNIT& dbrow = dbrows.rows_out[0];

        row._cid_      = dbrow.cells[0].i64;
        row._uid_      = dbrow.cells[1].i64;
        row._maxiids_  = dbrow.cells[2].i64;
        row._isc2s_    = dbrow.cells[3].i64;
        row._passwd_   = dbrow.cells[4].txt;
        row._bindedip_ = dbrow.cells[5].txt;
    }

    int i = 0;
    int c = (int)dbrows.rows_out.size();

    for (; i < c; ++i)
    {
        DB_ROW_UNIT& dbrow = dbrows.rows_out[i];
        if (!dbrow.cells[4].txt.empty())
        {
            ProZeroMemory(&dbrow.cells[4].txt[0], dbrow.cells[4].txt.length());
            dbrow.cells[4].txt = "";
        }
    }

    row.Adjust();

    return 1;
}

void
GetMsgKickoutRows(CDbConnection&                      db,
                  CProStlVector<TBL_MSG_KICKOUT_ROW>& rows)
{
    rows.clear();

    const char* sql = " SELECT _cid_, _uid_, _iid_ FROM tbl_msg02_kickout ";

    DB_ROW_SET dbrows;
    dbrows.types_in.push_back(DB_CT_I64); /* _cid_ */
    dbrows.types_in.push_back(DB_CT_I64); /* _uid_ */
    dbrows.types_in.push_back(DB_CT_I64); /* _iid_ */

    if (!db.DoSelect(sql, dbrows))
    {
        return;
    }

    int i = 0;
    int c = (int)dbrows.rows_out.size();

    for (; i < c; ++i)
    {
        const DB_ROW_UNIT& dbrow = dbrows.rows_out[i];

        TBL_MSG_KICKOUT_ROW row;
        row._cid_ = dbrow.cells[0].i64;
        row._uid_ = dbrow.cells[1].i64;
        row._iid_ = dbrow.cells[2].i64;

        row.Adjust();
        rows.push_back(row);
    }
}

void
CleanMsgKickoutRows(CDbConnection& db)
{
    const char* sql = " DELETE FROM tbl_msg02_kickout ";

    db.DoOther(sql);
}

void
AddMsgOnlineRow(CDbConnection&      db,
                const RTP_MSG_USER& user,
                CProStlString       userPublicIp,
                CProStlString       c2sIdString,
                CProStlString       sslSuiteName)
{
    userPublicIp = userPublicIp.substr(0, 64);
    c2sIdString  = c2sIdString.substr (0, 64);
    sslSuiteName = sslSuiteName.substr(0, 64);

    CProStlString timeString;
    ProGetLocalTimeString(timeString);

    char sql[1024] = "";
    snprintf_pro(
        sql,
        sizeof(sql),
        " SELECT _cid_ FROM tbl_msg03_online "
        " WHERE _cid_=%u AND _uid_=%llu AND _iid_=%u ",
        (unsigned int)user.classId,
        (unsigned long long)user.UserId(),
        (unsigned int)user.instId
        );

    DB_ROW_SET dbrows;
    dbrows.types_in.push_back(DB_CT_I64); /* _cid_ */

    if (db.DoSelect(sql, dbrows) && dbrows.rows_out.size() > 0)
    {
        snprintf_pro(
            sql,
            sizeof(sql),
            " UPDATE tbl_msg03_online "
            " SET _fromip_='%s', _fromc2s_='%s', _sslsuite_='%s', _logontime_='%s' "
            " WHERE _cid_=%u AND _uid_=%llu AND _iid_=%u ",
            userPublicIp.c_str(),
            c2sIdString.c_str(),
            sslSuiteName.c_str(),
            timeString.c_str(),
            (unsigned int)user.classId,
            (unsigned long long)user.UserId(),
            (unsigned int)user.instId
            );
    }
    else
    {
        snprintf_pro(
            sql,
            sizeof(sql),
            " INSERT INTO tbl_msg03_online "
            " (_cid_, _uid_, _iid_, _fromip_, _fromc2s_, _sslsuite_, _logontime_) "
            " VALUES (%u, %llu, %u, '%s', '%s', '%s', '%s') ",
            (unsigned int)user.classId,
            (unsigned long long)user.UserId(),
            (unsigned int)user.instId,
            userPublicIp.c_str(),
            c2sIdString.c_str(),
            sslSuiteName.c_str(),
            timeString.c_str()
            );
    }

    db.DoOther(sql);
}

void
RemoveMsgOnlineRow(CDbConnection&      db,
                   const RTP_MSG_USER& user)
{
    char sql[1024] = "";
    snprintf_pro(
        sql,
        sizeof(sql),
        " DELETE FROM tbl_msg03_online "
        " WHERE _cid_=%u AND _uid_=%llu AND _iid_=%u ",
        (unsigned int)user.classId,
        (unsigned long long)user.UserId(),
        (unsigned int)user.instId
        );

    db.DoOther(sql);
}

void
CleanMsgOnlineRows(CDbConnection& db)
{
    const char* sql = " DELETE FROM tbl_msg03_online ";

    db.DoOther(sql);
}
