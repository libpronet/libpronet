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

#if !defined(____PRO_FUNCTOR_COMMAND_TASK_H____)
#define ____PRO_FUNCTOR_COMMAND_TASK_H____

#include "pro_a.h"
#include "pro_stl.h"
#include "pro_thread.h"
#include "pro_thread_mutex.h"

/////////////////////////////////////////////////////////////////////////////
////

class IProFunctorCommand;

/////////////////////////////////////////////////////////////////////////////
////

class CProFunctorCommandTask : public CProThreadBase
{
public:

    CProFunctorCommandTask();

    virtual ~CProFunctorCommandTask();

    bool Start(
        bool          realtime    = false,
        unsigned long threadCount = 1
        );

    void Stop();

    bool Put(IProFunctorCommand* command, bool blocking = false);

    unsigned long GetSize() const;

    void SetUserData(const void* userData);

    const void* GetUserData() const;

private:

    void StopMe();

    virtual void Svc();

private:

    const void*                       m_userData;
    unsigned long                     m_threadCount;
    unsigned long                     m_curThreadCount;
    bool                              m_wantExit;
    CProStlSet<PRO_UINT64>            m_threadIds;
    CProStlDeque<IProFunctorCommand*> m_commands;
    CProThreadMutexCondition          m_commandCond;
    CProThreadMutexCondition          m_initCond;
    mutable CProThreadMutex           m_lock;
    CProThreadMutex                   m_lockAtom;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_FUNCTOR_COMMAND_TASK_H____ */
