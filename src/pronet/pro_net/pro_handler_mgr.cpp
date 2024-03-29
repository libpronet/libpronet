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

#include "pro_handler_mgr.h"
#include "pro_event_handler.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

CProHandlerMgr::~CProHandlerMgr()
{
    auto itr = m_sockId2HandlerInfo.begin();
    auto end = m_sockId2HandlerInfo.end();

    for (; itr != end; ++itr)
    {
        const PRO_HANDLER_INFO& info = itr->second;
        info.handler->Release();
    }

    m_sockId2HandlerInfo.clear();
}

bool
CProHandlerMgr::AddHandler(int64_t           sockId,
                           CProEventHandler* handler,
                           unsigned long     mask)
{
    assert(sockId != -1);
    assert(handler != NULL);
    assert(mask != 0);
    if (sockId == -1 || handler == NULL || mask == 0)
    {
        return false;
    }

    auto itr = m_sockId2HandlerInfo.find(sockId);
    if (itr == m_sockId2HandlerInfo.end())
    {
        PRO_HANDLER_INFO info;
        info.handler = handler;
        info.mask    = mask;

        info.handler->AddRef();
        m_sockId2HandlerInfo[sockId] = info;

        return true;
    }

    PRO_HANDLER_INFO& info = itr->second;

    if (handler == info.handler)
    {
        PRO_SET_BITS(info.mask, mask);

        return true;
    }
    else
    {
        return false;
    }
}

void
CProHandlerMgr::RemoveHandler(int64_t       sockId,
                              unsigned long mask)
{
    if (sockId == -1 || mask == 0)
    {
        return;
    }

    auto itr = m_sockId2HandlerInfo.find(sockId);
    if (itr == m_sockId2HandlerInfo.end())
    {
        return;
    }

    PRO_HANDLER_INFO& info = itr->second;
    PRO_CLR_BITS(info.mask, mask);

    if (info.mask == 0)
    {
        info.handler->Release();
        m_sockId2HandlerInfo.erase(itr);
    }
}

int64_t
CProHandlerMgr::GetMaxSockId() const
{
    int64_t sockId = -1;

    auto itr = m_sockId2HandlerInfo.rbegin();
    if (itr != m_sockId2HandlerInfo.rend())
    {
        sockId = itr->first;
    }

    return sockId;
}

PRO_HANDLER_INFO
CProHandlerMgr::FindHandler(int64_t sockId) const
{
    PRO_HANDLER_INFO info;

    assert(sockId != -1);
    if (sockId == -1)
    {
        return info;
    }

    auto itr = m_sockId2HandlerInfo.find(sockId);
    if (itr != m_sockId2HandlerInfo.end())
    {
        info = itr->second;
    }

    return info;
}
