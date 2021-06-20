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
 *     void Action(PRO_INT64* args)
 *     {
 *     }
 * };
 *
 * typedef void (CTest::* ACTION)(PRO_INT64*);
 *
 * CTest test;
 *
 * IProFunctorCommand* command =
 *     CProFunctorCommand_cpp<CTest, ACTION>::CreateInstance(
 *     test,
 *     &CTest::Action,
 *     0,
 *     1,
 *     2
 *     );
 */

/*
 * Usage2:
 *
 * void Action(PRO_INT64* args)
 * {
 * }
 *
 * typedef void (* ACTION)(PRO_INT64*);
 *
 * IProFunctorCommand* command =
 *     CProFunctorCommand_c<ACTION>::CreateInstance(
 *     &Action,
 *     0,
 *     1,
 *     2
 *     );
 */

#if !defined(____PRO_FUNCTOR_COMMAND_H____)
#define ____PRO_FUNCTOR_COMMAND_H____

#include "pro_a.h"
#include "pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class IProFunctorCommand
{
public:

    virtual ~IProFunctorCommand() {}

    virtual void PRO_CALLTYPE Destroy() = 0;

    virtual void PRO_CALLTYPE Execute() = 0;

    virtual void PRO_CALLTYPE SetUserData(const void* userData) = 0;

    virtual const void* PRO_CALLTYPE GetUserData() const = 0;
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
        PRO_INT64 arg0 = 0,
        PRO_INT64 arg1 = 0,
        PRO_INT64 arg2 = 0,
        PRO_INT64 arg3 = 0,
        PRO_INT64 arg4 = 0,
        PRO_INT64 arg5 = 0,
        PRO_INT64 arg6 = 0,
        PRO_INT64 arg7 = 0,
        PRO_INT64 arg8 = 0,
        PRO_INT64 arg9 = 0
        )
    {
        if (action == NULL)
        {
            return (NULL);
        }

        CProFunctorCommand_cpp* const command = new CProFunctorCommand_cpp(
            receiver, action,
            arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

        return (command);
    }

private:

    CProFunctorCommand_cpp(
        RECEIVER& receiver,
        ACTION    action,
        PRO_INT64 arg0,
        PRO_INT64 arg1,
        PRO_INT64 arg2,
        PRO_INT64 arg3,
        PRO_INT64 arg4,
        PRO_INT64 arg5,
        PRO_INT64 arg6,
        PRO_INT64 arg7,
        PRO_INT64 arg8,
        PRO_INT64 arg9
        ) : m_receiver(receiver)
    {
        m_userData = NULL;
        m_action   = action;

        const PRO_INT64 args[10] =
        { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 };

        for (int i = 0; i < 10; ++i)
        {
            m_args[i] = args[i];
        }
    }

    virtual ~CProFunctorCommand_cpp()
    {
    }

    virtual void PRO_CALLTYPE Destroy()
    {
        delete this;
    }

    virtual void PRO_CALLTYPE Execute()
    {
        (m_receiver.*m_action)((PRO_INT64*)m_args);
    }

    virtual void PRO_CALLTYPE SetUserData(const void* userData)
    {
        m_userData = userData;
    }

    virtual const void* PRO_CALLTYPE GetUserData() const
    {
        return (m_userData);
    }

private:

    const void* m_userData;
    RECEIVER&   m_receiver;
    ACTION      m_action;
    PRO_INT64   m_args[10];

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

template<typename ACTION>
class CProFunctorCommand_c : public IProFunctorCommand
{
public:

    static CProFunctorCommand_c* CreateInstance(
        ACTION    action,
        PRO_INT64 arg0 = 0,
        PRO_INT64 arg1 = 0,
        PRO_INT64 arg2 = 0,
        PRO_INT64 arg3 = 0,
        PRO_INT64 arg4 = 0,
        PRO_INT64 arg5 = 0,
        PRO_INT64 arg6 = 0,
        PRO_INT64 arg7 = 0,
        PRO_INT64 arg8 = 0,
        PRO_INT64 arg9 = 0
        )
    {
        if (action == NULL)
        {
            return (NULL);
        }

        CProFunctorCommand_c* const command = new CProFunctorCommand_c(
            action,
            arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

        return (command);
    }

private:

    CProFunctorCommand_c(
        ACTION    action,
        PRO_INT64 arg0,
        PRO_INT64 arg1,
        PRO_INT64 arg2,
        PRO_INT64 arg3,
        PRO_INT64 arg4,
        PRO_INT64 arg5,
        PRO_INT64 arg6,
        PRO_INT64 arg7,
        PRO_INT64 arg8,
        PRO_INT64 arg9
        )
    {
        m_userData = NULL;
        m_action   = action;

        const PRO_INT64 args[10] =
        { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 };

        for (int i = 0; i < 10; ++i)
        {
            m_args[i] = args[i];
        }
    }

    virtual ~CProFunctorCommand_c()
    {
    }

    virtual void PRO_CALLTYPE Destroy()
    {
        delete this;
    }

    virtual void PRO_CALLTYPE Execute()
    {
        (*m_action)((PRO_INT64*)m_args);
    }

    virtual void PRO_CALLTYPE SetUserData(const void* userData)
    {
        m_userData = userData;
    }

    virtual const void* PRO_CALLTYPE GetUserData() const
    {
        return (m_userData);
    }

private:

    const void* m_userData;
    ACTION      m_action;
    PRO_INT64   m_args[10];

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_FUNCTOR_COMMAND_H____ */
