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

/*
 * Usage1:
 *
 * class CTest
 * {
 * public:
 *
 *     void Action(int arg1, void* arg2, std::string arg3)
 *     {
 *     }
 * };
 *
 * CTest test;
 *
 * CProFunctorCommand* command = CProFunctorCommand::Create(
 *     test,
 *     &CTest::Action,
 *     1,
 *     (void*)&test,
 *     std::string("test")
 *     );
 * ... ...
 * command->Execute();
 * command->Destroy();
 */

/*
 * Usage2:
 *
 * void Action(int arg1, std::string arg2)
 * {
 * }
 *
 * CProFunctorCommand* command = CProFunctorCommand::Create(
 *     &Action,
 *     1,
 *     std::string("test")
 *     );
 * ... ...
 * command->Execute();
 * command->Destroy();
 */

#ifndef ____PRO_FUNCTOR_COMMAND_H____
#define ____PRO_FUNCTOR_COMMAND_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProFunctorCommand
{
public:

    /*
     * The 'action' is a non-const member function.
     */
    template<typename RECEIVER, typename... ARGS>
    static CProFunctorCommand* Create(
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...),
        ARGS...           args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return NULL;
        }

        CProFunctorCommand* command = new CProFunctorCommand;

        command->m_func = [&receiver, action, args...]() -> void
        {
            (receiver.*action)(args...);
        };

        return command;
    }

    /*
     * The 'action' is a const member function.
     */
    template<typename RECEIVER, typename... ARGS>
    static CProFunctorCommand* Create(
        const RECEIVER&   receiver,
        void (RECEIVER::* action)(ARGS...) const,
        ARGS...           args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return NULL;
        }

        CProFunctorCommand* command = new CProFunctorCommand;

        command->m_func = [&receiver, action, args...]() -> void
        {
            (receiver.*action)(args...);
        };

        return command;
    }

    /*
     * The 'action' is a static member function or a non-member function.
     */
    template<typename... ARGS>
    static CProFunctorCommand* Create(
        void (* action)(ARGS...),
        ARGS... args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return NULL;
        }

        CProFunctorCommand* command = new CProFunctorCommand;

        command->m_func = [action, args]() -> void
        {
            (*action)(args...);
        };

        return command;
    }

    /*
     * Its main purpose is to encapsulate lambda expressions.
     */
    static CProFunctorCommand* Create(const std::function<void()>& func)
    {
        assert(func);
        if (!func)
        {
            return NULL;
        }

        CProFunctorCommand* command = new CProFunctorCommand;
        command->m_func             = func;

        return command;
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

    CProFunctorCommand()
    {
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

#endif /* ____PRO_FUNCTOR_COMMAND_H____ */
