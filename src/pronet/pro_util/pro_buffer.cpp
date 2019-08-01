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

#include "pro_a.h"
#include "pro_buffer.h"
#include "pro_memory_pool.h"
#include "pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CProBuffer::CProBuffer()
{
    m_data  = NULL;
    m_size  = 0;
    m_magic = 0;
}

CProBuffer::~CProBuffer()
{
    Free();
}

void
CProBuffer::Free()
{
    ProFree(m_data);
    m_data = NULL;
    m_size = 0;
}

bool
CProBuffer::Resize(size_t size)
{
    assert(size > 0);
    if (size == 0)
    {
        return (false);
    }

    if (size == m_size)
    {
        return (true);
    }

    ProFree(m_data);
    m_data = NULL;
    m_size = 0;

    m_data = (char*)ProMalloc(size);
    if (m_data == NULL)
    {
        return (false);
    }

    m_size = size;

    return (true);
}
