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
 *     void Action(int64_t* args)
 *     {
 *     }
 * };
 *
 * typedef void (CTest::* ACTION)(int64_t*);
 *
 * CTest test;
 *
 * IProFunctorCommand* command =
 *     CProFunctorCommand_cpp<CTest, ACTION>::CreateInstance(
 *         test,
 *         &CTest::Action,
 *         0,
 *         1,
 *         2
 *         );
 */

/*
 * Usage2:
 *
 * void Action(int64_t* args)
 * {
 * }
 *
 * typedef void (* ACTION)(int64_t*);
 *
 * IProFunctorCommand* command =
 *     CProFunctorCommand_c<ACTION>::CreateInstance(
 *         &Action,
 *         0,
 *         1,
 *         2
 *         );
 */

#if !defined(____PRO_FUNCTOR_COMMAND_H____)
#define ____PRO_FUNCTOR_COMMAND_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class IProFunctorCommand
{
public:

    virtual ~IProFunctorCommand() {}

    virtual void Destroy() = 0;

    virtual void Execute() = 0;

    virtual void SetUserData1(const void* userData1) = 0;

    virtual const void* GetUserData1() const = 0;

    virtual void SetUserData2(const void* userData2) = 0;

    virtual const void* GetUserData2() const = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

template<typename RECEIVER, typename ACTION>
class CProFunctorCommand_cpp : public IProFunctorCommand
{
public:

    static CProFunctorCommand_cpp* CreateInstance(
        RECEIVER& receiver,
        ACTION    action,
        int64_t   arg0 = 0,
        int64_t   arg1 = 0,
        int64_t   arg2 = 0,
        int64_t   arg3 = 0,
        int64_t   arg4 = 0,
        int64_t   arg5 = 0,
        int64_t   arg6 = 0,
        int64_t   arg7 = 0,
        int64_t   arg8 = 0,
        int64_t   arg9 = 0
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return NULL;
        }

        return new CProFunctorCommand_cpp(
            receiver, action, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }

private:

    CProFunctorCommand_cpp(
        RECEIVER& receiver,
        ACTION    action,
        int64_t   arg0,
        int64_t   arg1,
        int64_t   arg2,
        int64_t   arg3,
        int64_t   arg4,
        int64_t   arg5,
        int64_t   arg6,
        int64_t   arg7,
        int64_t   arg8,
        int64_t   arg9
        ) : m_receiver(receiver)
    {
        m_userData1 = NULL;
        m_userData2 = NULL;
        m_action    = action;

        const int64_t args[10] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 };

        for (int i = 0; i < 10; ++i)
        {
            m_args[i] = args[i];
        }
    }

    virtual ~CProFunctorCommand_cpp()
    {
    }

    virtual void Destroy()
    {
        delete this;
    }

    virtual void Execute()
    {
        (m_receiver.*m_action)((int64_t*)m_args);
    }

    virtual void SetUserData1(const void* userData1)
    {
        m_userData1 = userData1;
    }

    virtual const void* GetUserData1() const
    {
        return m_userData1;
    }

    virtual void SetUserData2(const void* userData2)
    {
        m_userData2 = userData2;
    }

    virtual const void* GetUserData2() const
    {
        return m_userData2;
    }

private:

    const void* m_userData1;
    const void* m_userData2;
    RECEIVER&   m_receiver;
    ACTION      m_action;
    int64_t     m_args[10];

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

template<typename ACTION>
class CProFunctorCommand_c : public IProFunctorCommand
{
public:

    static CProFunctorCommand_c* CreateInstance(
        ACTION  action,
        int64_t arg0 = 0,
        int64_t arg1 = 0,
        int64_t arg2 = 0,
        int64_t arg3 = 0,
        int64_t arg4 = 0,
        int64_t arg5 = 0,
        int64_t arg6 = 0,
        int64_t arg7 = 0,
        int64_t arg8 = 0,
        int64_t arg9 = 0
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return NULL;
        }

        return new CProFunctorCommand_c(
            action, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
    }

private:

    CProFunctorCommand_c(
        ACTION  action,
        int64_t arg0,
        int64_t arg1,
        int64_t arg2,
        int64_t arg3,
        int64_t arg4,
        int64_t arg5,
        int64_t arg6,
        int64_t arg7,
        int64_t arg8,
        int64_t arg9
        )
    {
        m_userData1 = NULL;
        m_userData2 = NULL;
        m_action    = action;

        const int64_t args[10] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 };

        for (int i = 0; i < 10; ++i)
        {
            m_args[i] = args[i];
        }
    }

    virtual ~CProFunctorCommand_c()
    {
    }

    virtual void Destroy()
    {
        delete this;
    }

    virtual void Execute()
    {
        (*m_action)((int64_t*)m_args);
    }

    virtual void SetUserData1(const void* userData1)
    {
        m_userData1 = userData1;
    }

    virtual const void* GetUserData1() const
    {
        return m_userData1;
    }

    virtual void SetUserData2(const void* userData2)
    {
        m_userData2 = userData2;
    }

    virtual const void* GetUserData2() const
    {
        return m_userData2;
    }

private:

    const void* m_userData1;
    const void* m_userData2;
    ACTION      m_action;
    int64_t     m_args[10];

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_FUNCTOR_COMMAND_H____ */
