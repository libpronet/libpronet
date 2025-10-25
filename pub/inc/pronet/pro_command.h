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

/*
 * This is an implementation of the "Command" pattern.
 */

#ifndef ____PRO_COMMAND_H____
#define ____PRO_COMMAND_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProCommand
{
public:

    static CProCommand* Create(const std::function<void()>& func)
    {
        assert(func);
        if (!func)
        {
            return NULL;
        }

        return new CProCommand(func);
    }

    void Destroy()
    {
        delete this;
    }

    void Execute()
    {
        m_func();
    }

    void SetUserData1(const void* userData1)
    {
        m_userData1 = userData1;
    }

    const void* GetUserData1() const
    {
        return m_userData1;
    }

    void SetUserData2(const void* userData2)
    {
        m_userData2 = userData2;
    }

    const void* GetUserData2() const
    {
        return m_userData2;
    }

private:

    CProCommand(const std::function<void()>& func)
    {
        m_func      = func;
        m_userData1 = NULL;
        m_userData2 = NULL;
    }

private:

    std::function<void()> m_func;
    const void*           m_userData1;
    const void*           m_userData2;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_COMMAND_H____ */
