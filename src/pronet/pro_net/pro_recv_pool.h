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

#ifndef PRO_RECV_POOL_H
#define PRO_RECV_POOL_H

#include "pro_net.h"
#include "../pro_util/pro_buffer.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProRecvPool : public IProRecvPool
{
public:

    CProRecvPool()
    {
        m_begin    = NULL;
        m_end      = NULL;
        m_data     = NULL;
        m_dataSize = 0;
        m_idle     = NULL;
        m_idleSize = 0;
    }

    virtual ~CProRecvPool()
    {
        m_begin    = NULL;
        m_end      = NULL;
        m_data     = NULL;
        m_dataSize = 0;
        m_idle     = NULL;
        m_idleSize = 0;
    }

    bool Resize(size_t size)
    {
        if (size == 0)
        {
            return false;
        }

        m_begin    = NULL;
        m_end      = NULL;
        m_data     = NULL;
        m_dataSize = 0;
        m_idle     = NULL;
        m_idleSize = 0;

        if (!m_buf.Resize(size))
        {
            return false;
        }

        m_begin    = (char*)m_buf.Data();
        m_end      = m_begin + size;
        m_idle     = m_begin;
        m_idleSize = size;

        return true;
    }

    virtual size_t PeekDataSize() const
    {
        return m_dataSize;
    }

    virtual void PeekData(
        void*  buf,
        size_t size
        ) const
    {
        if (buf == NULL || size == 0 || size > m_dataSize)
        {
            return;
        }

        size_t continuousSize = ContinuousDataSize();
        if (size > continuousSize)
        {
            memcpy(buf, m_data, continuousSize);
            memcpy((char*)buf + continuousSize, m_begin, size - continuousSize);
        }
        else
        {
            memcpy(buf, m_data, size);
        }
    }

    virtual void Flush(size_t size)
    {
        if (size == 0 || size > m_dataSize)
        {
            return;
        }

        if (m_idle == NULL)
        {
            m_idle = m_data;
        }

        m_idleSize += size;

        if (m_dataSize - size == 0)
        {
            m_data = NULL;

            /*
             * for udp
             */
            m_idle     = m_begin;
            m_idleSize = m_end - m_begin;
        }
        else
        {
            size_t continuousSize = ContinuousDataSize();
            if (size > continuousSize)
            {
                m_data = m_begin + (size - continuousSize);
            }
            else
            {
                m_data += size;
                if (m_data == m_end)
                {
                    m_data = m_begin;
                }
            }
        }

        m_dataSize -= size;
    }

    virtual size_t GetFreeSize() const
    {
        return m_idleSize;
    }

    void* ContinuousIdleBuf()
    {
        return m_idle;
    }

    size_t ContinuousIdleSize() const
    {
        if (m_idle + m_idleSize > m_end)
        {
            return (size_t)(m_end - m_idle);
        }

        return (size_t)m_idleSize;
    }

    void Fill(size_t size)
    {
        if (size == 0 || size > ContinuousIdleSize())
        {
            return;
        }

        if (m_data == NULL)
        {
            m_data = m_idle;
        }

        m_dataSize += size;

        if (m_idleSize - size == 0)
        {
            m_idle = NULL;
        }
        else
        {
            m_idle += size;
            if (m_idle == m_end)
            {
                m_idle = m_begin;
            }
        }

        m_idleSize -= size;
    }

private:

    size_t ContinuousDataSize() const
    {
        if (m_data + m_dataSize > m_end)
        {
            return m_end - m_data;
        }

        return m_dataSize;
    }

private:

    CProBuffer m_buf;
    char*      m_begin; /* const */
    char*      m_end;   /* const */
    char*      m_data;
    size_t     m_dataSize;
    char*      m_idle;
    size_t     m_idleSize;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_RECV_POOL_H */
