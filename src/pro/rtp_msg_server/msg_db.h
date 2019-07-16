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

/////////////////////////////////////////////////////////////////////////////
////

static const PRO_INT64 MAX_NODE_UID = ((PRO_INT64)0xFF << 32) | 0xFFFFFFFF;

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

    PRO_INT64     _cid_;
    PRO_INT64     _uid_;
    PRO_INT64     _maxiids_;
    PRO_INT64     _isc2s_;
    CProStlString _passwd_;
    CProStlString _bindedip_;

    DECLARE_SGI_POOL(0);
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

    PRO_INT64 _cid_;
    PRO_INT64 _uid_;
    PRO_INT64 _iid_;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

long
PRO_CALLTYPE
GetMsgUserRow(CDbConnection&      db,
              const RTP_MSG_USER& user,
              TBL_MSG_USER_ROW&   row);

void
PRO_CALLTYPE
GetMsgKickoutRows(CDbConnection&                      db,
                  CProStlVector<TBL_MSG_KICKOUT_ROW>& rows);

void
PRO_CALLTYPE
CleanMsgKickoutRows(CDbConnection& db);

void
PRO_CALLTYPE
AddMsgOnlineRow(CDbConnection&      db,
                const RTP_MSG_USER& user,
                CProStlString       userPublicIp,
                CProStlString       c2sIdString);

void
PRO_CALLTYPE
RemoveMsgOnlineRow(CDbConnection&      db,
                   const RTP_MSG_USER& user);

void
PRO_CALLTYPE
CleanMsgOnlineRows(CDbConnection& db);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* MSG_DB_H */
