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

#ifndef ____PRO_REF_COUNT_H____
#define ____PRO_REF_COUNT_H____

#include "pro_a.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProRefCount
{
public:

    virtual unsigned long AddRef()
    {
        return ++m_count;
    }

    virtual unsigned long Release()
    {
        unsigned long count = --m_count;
        if (count == 0)
        {
            delete this;
        }

        return count;
    }

protected:

    CProRefCount()
    {
        m_count = 1;
    }

    virtual ~CProRefCount()
    {
    }

private:

    std::atomic<unsigned int> m_count;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_REF_COUNT_H____ */
