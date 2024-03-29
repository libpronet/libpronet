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

#ifndef ____PRO_BUFFER_H____
#define ____PRO_BUFFER_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProBuffer
{
public:

    CProBuffer();

    ~CProBuffer();

    void Free();

    bool Resize(size_t size);

    void* Data()
    {
        return m_data;
    }

    const void* Data() const
    {
        return m_data;
    }

    size_t Size() const
    {
        return m_size;
    }

    void SetMagic(int64_t magic)
    {
        m_magic = magic;
    }

    int64_t GetMagic() const
    {
        return m_magic;
    }

private:

    char*   m_data;
    size_t  m_size;
    int64_t m_magic;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_BUFFER_H____ */
