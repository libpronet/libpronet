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

#if !defined(MSG_DB_H)
#define MSG_DB_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

static const int64_t MAX_NODE_UID = 0xFFFFFFFFFFULL;

class  CDbConnection;
struct RTP_MSG_USER;

struct TBL_MSG_USER_ROW
{
    TBL_MSG_USER_ROW()
    {
        _cid_      = 0;
        _uid_      = 0;
        _maxiids_  = 0;
        _isc2s_    = 0;
        _passwd_   = "";
        _bindedip_ = "";
    }

    ~TBL_MSG_USER_ROW()
    {
        if (!_passwd_.empty())
        {
            ProZeroMemory(&_passwd_[0], _passwd_.length());
            _passwd_ = "";
        }
    }

    void Adjust()
    {
        if (_cid_ < 0 || _cid_ > 255)
        {
            _cid_     = 0;
        }
        if (_uid_ < 0 || _uid_ > MAX_NODE_UID)
        {
            _uid_     = 0;
        }
        if (_maxiids_ < 1 || _maxiids_ > 65535)
        {
            _maxiids_ = 1;
        }
    }

    int64_t       _cid_;
    int64_t       _uid_;
    int64_t       _maxiids_;
    int64_t       _isc2s_;
    CProStlString _passwd_;
    CProStlString _bindedip_;

    DECLARE_SGI_POOL(0)
};

struct TBL_MSG_KICKOUT_ROW
{
    TBL_MSG_KICKOUT_ROW()
    {
        _cid_ = 0;
        _uid_ = 0;
        _iid_ = 0;
    }

    void Adjust()
    {
        if (_cid_ < 0 || _cid_ > 255)
        {
            _cid_ = 0;
        }
        if (_uid_ < 0 || _uid_ > MAX_NODE_UID)
        {
            _uid_ = 0;
        }
        if (_iid_ < 0 || _iid_ > 65535)
        {
            _iid_ = 0;
        }
    }

    int64_t _cid_;
    int64_t _uid_;
    int64_t _iid_;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

long
GetMsgUserRow(CDbConnection&      db,
              const RTP_MSG_USER& user,
              TBL_MSG_USER_ROW&   row);

void
GetMsgKickoutRows(CDbConnection&                      db,
                  CProStlVector<TBL_MSG_KICKOUT_ROW>& rows);

void
CleanMsgKickoutRows(CDbConnection& db);

void
AddMsgOnlineRow(CDbConnection&      db,
                const RTP_MSG_USER& user,
                CProStlString       userPublicIp,
                CProStlString       c2sIdString,
                CProStlString       sslSuiteName);

void
RemoveMsgOnlineRow(CDbConnection&      db,
                   const RTP_MSG_USER& user);

void
CleanMsgOnlineRows(CDbConnection& db);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* MSG_DB_H */
