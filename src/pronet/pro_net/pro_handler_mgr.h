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

#if !defined(PRO_HANDLER_MGR_H)
#define PRO_HANDLER_MGR_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProEventHandler;

struct PRO_HANDLER_INFO
{
    PRO_HANDLER_INFO()
    {
        handler = NULL;
        mask    = 0;
    }

    CProEventHandler* handler;
    unsigned long     mask;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CProHandlerMgr
{
public:

    CProHandlerMgr()
    {
    }

    ~CProHandlerMgr();

    bool AddHandler(
        PRO_INT64         sockId,
        CProEventHandler* handler,
        unsigned long     mask
        );

    void RemoveHandler(
        PRO_INT64     sockId,
        unsigned long mask
        );

    PRO_INT64 GetMaxSockId() const;

    unsigned long GetHandlerCount() const;

    const PRO_HANDLER_INFO FindHandler(PRO_INT64 sockId) const;

    const CProStlMap<PRO_INT64, PRO_HANDLER_INFO>& GetAllHandlers() const;

private:

    CProStlMap<PRO_INT64, PRO_HANDLER_INFO> m_sockId2HandlerInfo;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_HANDLER_MGR_H */
