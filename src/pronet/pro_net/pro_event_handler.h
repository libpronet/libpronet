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

#if !defined(PRO_EVENT_HANDLER_H)
#define PRO_EVENT_HANDLER_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_timer_factory.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

#define PRO_SET_BITS(ULONG, BITS)   ((ULONG) |= (BITS))
#define PRO_CLR_BITS(ULONG, BITS)   ((ULONG) &= ~(BITS))
#define PRO_BIT_ENABLED(ULONG, BIT) (((ULONG) & (BIT)) != 0)

static const unsigned long PRO_MASK_ACCEPT    = 1 << 0;
static const unsigned long PRO_MASK_CONNECT   = 1 << 1;
static const unsigned long PRO_MASK_WRITE     = 1 << 2;
static const unsigned long PRO_MASK_READ      = 1 << 3;
static const unsigned long PRO_MASK_EXCEPTION = 1 << 4;
static const unsigned long PRO_MASK_ERROR     = 1 << 5;

class CProBaseReactor;

/////////////////////////////////////////////////////////////////////////////
////

class CProEventHandler : public IProOnTimer, public CProRefCount
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef()
    {
        const unsigned long refCount = CProRefCount::AddRef();

        return (refCount);
    }

    virtual unsigned long PRO_CALLTYPE Release()
    {
        const unsigned long refCount = CProRefCount::Release();

        return (refCount);
    }

    virtual void PRO_CALLTYPE OnInput(PRO_INT64 sockId)
    {
    }

    virtual void PRO_CALLTYPE OnOutput(PRO_INT64 sockId)
    {
    }

    virtual void PRO_CALLTYPE OnException(PRO_INT64 sockId)
    {
    }

    virtual void PRO_CALLTYPE OnError(
        PRO_INT64 sockId,
        long      errorCode
        )
    {
    }

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        )
    {
    }

    void SetReactor(CProBaseReactor* reactor)
    {
        m_reactor = reactor;
    }

    CProBaseReactor* GetReactor() const
    {
        return (m_reactor);
    }

    void AddMask(unsigned long mask)
    {
        PRO_SET_BITS(m_mask, mask);
    }

    void RemoveMask(unsigned long mask)
    {
        PRO_CLR_BITS(m_mask, mask);
    }

    unsigned long GetMask() const
    {
        return (m_mask);
    }

protected:

    CProEventHandler()
    {
        m_reactor = NULL;
        m_mask    = 0;
    }

    virtual ~CProEventHandler()
    {
    }

private:

    CProBaseReactor* m_reactor;
    unsigned long    m_mask;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_EVENT_HANDLER_H */
