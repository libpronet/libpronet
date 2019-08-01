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

#if !defined(PRO_SEND_POOL_H)
#define PRO_SEND_POOL_H

#include "../pro_util/pro_buffer.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProSendPool
{
public:

    CProSendPool()
    {
        m_pendingPos = NULL;
    }

    ~CProSendPool()
    {
        int       i = 0;
        const int c = (int)m_bufs.size();

        for (; i < c; ++i)
        {
            delete m_bufs[i];
        }

        m_bufs.clear();
        m_pendingPos = NULL;
    }

    void Fill(const void* buf, size_t size, PRO_UINT64 actionId = 0)
    {
        if (buf == NULL || size == 0)
        {
            return;
        }

        CProBuffer* const buf2 = new CProBuffer;
        if (!buf2->Resize(size))
        {
            delete buf2;

            return;
        }

        memcpy(buf2->Data(), buf, size);
        buf2->SetMagic(actionId);
        m_bufs.push_back(buf2);

        if (m_bufs.size() == 1)
        {
            m_pendingPos = (char*)buf2->Data();
        }
    }

    const void* PreSend(unsigned long& size) const
    {
        size = 0;

        if (m_bufs.size() == 0)
        {
            return (NULL);
        }

        CProBuffer* const buf = m_bufs.front();
        size = (unsigned long)((char*)buf->Data() + buf->Size() - m_pendingPos);
        if (size == 0)
        {
            return (NULL);
        }

        return (m_pendingPos);
    }

    void Flush(size_t size)
    {
        if (size == 0 || m_bufs.size() == 0)
        {
            return;
        }

        CProBuffer* const buf = m_bufs.front();
        if (m_pendingPos + size > (char*)buf->Data() + buf->Size())
        {
            return;
        }

        m_pendingPos += size;
    }

    const CProBuffer* OnSendBuf() const
    {
        if (m_bufs.size() == 0)
        {
            return (NULL);
        }

        CProBuffer* const buf = m_bufs.front();
        if (m_pendingPos != (char*)buf->Data() + buf->Size())
        {
            return (NULL);
        }

        return (buf);
    }

    void PostSend()
    {
        if (m_bufs.size() == 0)
        {
            return;
        }

        CProBuffer* buf = m_bufs.front();
        if (m_pendingPos != (char*)buf->Data() + buf->Size())
        {
            return;
        }

        m_bufs.pop_front();
        delete buf;
        m_pendingPos = NULL;

        if (m_bufs.size() > 0)
        {
            buf = m_bufs.front();
            m_pendingPos = (char*)buf->Data();
        }
    }

private:

    CProStlDeque<CProBuffer*> m_bufs;
    const char*               m_pendingPos;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_SEND_POOL_H */
