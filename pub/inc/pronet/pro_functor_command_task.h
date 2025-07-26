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

#ifndef ____PRO_FUNCTOR_COMMAND_TASK_H____
#define ____PRO_FUNCTOR_COMMAND_TASK_H____

#include "pro_a.h"
#include "pro_functor_command.h"
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

class CProFunctorCommandTask : public CProThreadBase
{
    friend class CProChannelTaskPool;

public:

    CProFunctorCommandTask();

    virtual ~CProFunctorCommandTask();

    bool Start(
        bool         realtime    = false,
        unsigned int threadCount = 1
        );

    void Stop();

    /*
     * 'action' is a non-const member function
     */
    template<typename RECEIVER, typename... ARGS>
    bool PostCall(
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...),
        ARGS...           args
        )
    {
        return DoCall(false, receiver, action, args...); /* blocking is false */
    }

    /*
     * 'action' is a const member function
     */
    template<typename RECEIVER, typename... ARGS>
    bool PostCall(
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...) const,
        ARGS...           args
        )
    {
        return DoCall(false, receiver, action, args...); /* blocking is false */
    }

    /*
     * 'action' is a static member function or a non-member function
     */
    template<typename... ARGS>
    bool PostCall(
        void (* action)(ARGS...),
        ARGS... args
        )
    {
        return DoCall(false, action, args...); /* blocking is false */
    }

    /*
     * 'action' is a non-const member function
     */
    template<typename RECEIVER, typename... ARGS>
    bool SendCall(
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...),
        ARGS...           args
        )
    {
        return DoCall(true, receiver, action, args...); /* blocking is true */
    }

    /*
     * 'action' is a const member function
     */
    template<typename RECEIVER, typename... ARGS>
    bool SendCall(
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...) const,
        ARGS...           args
        )
    {
        return DoCall(true, receiver, action, args...); /* blocking is true */
    }

    /*
     * 'action' is a static member function or a non-member function
     */
    template<typename... ARGS>
    bool SendCall(
        void (* action)(ARGS...),
        ARGS... args
        )
    {
        return DoCall(true, action, args...); /* blocking is true */
    }

    size_t GetSize() const;

    /*
     * This is only relevant in single-threaded scenarios.
     */
    bool IsCurrentThread() const;

    void SetUserData(const void* userData);

    const void* GetUserData() const;

private:

    void StopMe();

    template<typename RECEIVER, typename... ARGS>
    bool DoCall(
        bool              blocking,
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...),
        ARGS...           args
        )
    {
        CProFunctorCommand* command = CProFunctorCommand::Create(receiver, action, args...);
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

    template<typename RECEIVER, typename... ARGS>
    bool DoCall(
        bool              blocking,
        RECEIVER&         receiver,
        void (RECEIVER::* action)(ARGS...) const,
        ARGS...           args
        )
    {
        CProFunctorCommand* command = CProFunctorCommand::Create(receiver, action, args...);
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

    template<typename... ARGS>
    bool DoCall(
        bool    blocking,
        void (* action)(ARGS...),
        ARGS... args
        )
    {
        CProFunctorCommand* command = CProFunctorCommand::Create(action, args...);
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
        CProFunctorCommand* command,
        bool                blocking = false
        );

    virtual void Svc();

private:

    const void*                       m_userData;
    unsigned int                      m_threadCount;
    unsigned int                      m_curThreadCount;
    bool                              m_wantExit;
    CProStlSet<uint64_t>              m_threadIds;
    CProStlDeque<CProFunctorCommand*> m_commands;
    CProThreadMutexCondition          m_commandCond;
    CProThreadMutexCondition          m_initCond;
    mutable CProThreadMutex           m_lock;
    CProThreadMutex                   m_lockAtom;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_FUNCTOR_COMMAND_TASK_H____ */
