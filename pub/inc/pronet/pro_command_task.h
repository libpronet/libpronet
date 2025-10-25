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
 * This is an implementation of the "Active Object" pattern.
 */

/*
 * Usage:
 *
 * class CTest
 * {
 * public:
 *
 *     void Action1(int arg1, std::string arg2)
 *     {
 *         printf(" Action1(%d, %s) \n", arg1, arg2.c_str());
 *     }
 *
 *     int Action2(int arg1, std::string arg2)
 *     {
 *         printf(" Action2(%d, %s) \n", arg1, arg2.c_str());
 *
 *         return arg1;
 *     }
 *
 *     void Action3(int arg1, const std::string& arg2)
 *     {
 *         printf(" Action3(%d, %s) \n", arg1, arg2.c_str());
 *     }
 *
 *     static void Action4(int arg1, std::string arg2)
 *     {
 *         printf(" Action4(%d, %s) \n", arg1, arg2.c_str());
 *     }
 * };
 *
 * void Test()
 * {
 *     CProCommandTask task;
 *     task.Start();
 *
 *     CTest test;
 *
 *     task.PostCall(test, &CTest::Action1, 1, std::string("PostCall -> Action1"));
 *     task.SendCall(test, &CTest::Action1, 2, std::string("SendCall -> Action1"));
 *     task.SendCall(test, &CTest::Action1, 3, "SendCall -> Action1");
 *
 *     task.PostCall(test, &CTest::Action2, 4, std::string("PostCall -> Action2"));
 *     task.SendCall(test, &CTest::Action2, 5, "SendCall -> Action2");
 *     int ret = 0;
 *     task.SendCall([&]()
 *     {
 *         ret = test.Action2(6, "SendCall -> Action2");
 *     });
 *     printf(" ret = %d \n", ret);
 *
 *     task.SendCall(test, &CTest::Action3, 7, "SendCall -> Action3");
 *
 *     task.PostCall(&CTest::Action4, 8, std::string("PostCall -> Action4"));
 *     task.SendCall(&CTest::Action4, 9, "SendCall -> Action4");
 * }
 */

#ifndef ____PRO_COMMAND_TASK_H____
#define ____PRO_COMMAND_TASK_H____

#include "pro_a.h"
#include "pro_command.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_thread.h"
#include "pro_thread_mutex.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProChannelTaskPool;

/////////////////////////////////////////////////////////////////////////////
////

class CProCommandTask : public CProThreadBase
{
    friend class CProChannelTaskPool;

public:

    CProCommandTask();

    virtual ~CProCommandTask();

    bool Start(
        bool         realtime    = false,
        unsigned int threadCount = 1
        );

    void Stop();

    /*
     * The 'action' is a non-const member function
     */
    template<typename RECEIVER, typename RET, typename... ARGS>
    bool PostCall(
        RECEIVER&        receiver,
        RET (RECEIVER::* action)(ARGS...),
        ARGS...          args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [=, &receiver]() -> void
        {
            (receiver.*action)(args...);
        };

        return DoCall(false, func); /* blocking is false */
    }

    /*
     * The 'action' is a const member function
     */
    template<typename RECEIVER, typename RET, typename... ARGS>
    bool PostCall(
        const RECEIVER&  receiver,
        RET (RECEIVER::* action)(ARGS...) const,
        ARGS...          args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [=, &receiver]() -> void
        {
            (receiver.*action)(args...);
        };

        return DoCall(false, func); /* blocking is false */
    }

    /*
     * The 'action' is a static member function or a non-member function
     */
    template<typename RET, typename... ARGS>
    bool PostCall(
        RET (*  action)(ARGS...),
        ARGS... args
        )
    {
        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [=]() -> void
        {
            (*action)(args...);
        };

        return DoCall(false, func); /* blocking is false */
    }

    /*
     * Its main purpose is to encapsulate lambda expressions
     */
    bool PostCall(const std::function<void()>& func)
    {
        return DoCall(false, func); /* blocking is false */
    }

    /*
     * The 'action' is a non-const member function
     */
    template<typename RECEIVER, typename RET, typename... ARGS1, typename... ARGS2>
    bool SendCall(
        RECEIVER&        receiver,
        RET (RECEIVER::* action)(ARGS1...),
        ARGS2&&...       args
        )
    {
        static_assert(sizeof...(ARGS1) == sizeof...(ARGS2), "");

        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [&]() -> void
        {
            (receiver.*action)(std::forward<ARGS2>(args)...);
        };

        return DoCall(true, func); /* blocking is true */
    }

    /*
     * The 'action' is a const member function
     */
    template<typename RECEIVER, typename RET, typename... ARGS1, typename... ARGS2>
    bool SendCall(
        const RECEIVER&  receiver,
        RET (RECEIVER::* action)(ARGS1...) const,
        ARGS2&&...       args
        )
    {
        static_assert(sizeof...(ARGS1) == sizeof...(ARGS2), "");

        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [&]() -> void
        {
            (receiver.*action)(std::forward<ARGS2>(args)...);
        };

        return DoCall(true, func); /* blocking is true */
    }

    /*
     * The 'action' is a static member function or a non-member function
     */
    template<typename RET, typename... ARGS1, typename... ARGS2>
    bool SendCall(
        RET (*     action)(ARGS1...),
        ARGS2&&... args
        )
    {
        static_assert(sizeof...(ARGS1) == sizeof...(ARGS2), "");

        assert(action != NULL);
        if (action == NULL)
        {
            return false;
        }

        auto func = [&]() -> void
        {
            (*action)(std::forward<ARGS2>(args)...);
        };

        return DoCall(true, func); /* blocking is true */
    }

    /*
     * Its main purpose is to encapsulate lambda expressions
     */
    bool SendCall(const std::function<void()>& func)
    {
        return DoCall(true, func); /* blocking is true */
    }

    size_t GetSize() const;

    /*
     * This is only relevant in single-threaded scenarios
     */
    bool IsCurrentThread() const;

    void SetUserData(const void* userData);

    const void* GetUserData() const;

private:

    void StopMe();

    bool DoCall(
        bool                         blocking,
        const std::function<void()>& func
        )
    {
        CProCommand* command = CProCommand::Create(func);
        if (command == NULL)
        {
            return false;
        }

        if (!Put(command, blocking))
        {
            command->Destroy();

            return false;
        }

        return true;
    }

    bool Put(
        CProCommand* command,
        bool         blocking = false
        );

    virtual void Svc();

private:

    const void*                m_userData;
    unsigned int               m_threadCount;
    unsigned int               m_curThreadCount;
    bool                       m_wantExit;
    CProStlSet<uint64_t>       m_threadIds;
    CProStlDeque<CProCommand*> m_commands;
    CProThreadMutexCondition   m_commandCond;
    CProThreadMutexCondition   m_initCond;
    mutable CProThreadMutex    m_lock;
    CProThreadMutex            m_lockAtom;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_COMMAND_TASK_H____ */
