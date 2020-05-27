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

#if !defined(PRO_NOTIFY_PIPE_H)
#define PRO_NOTIFY_PIPE_H

#include "../pro_util/pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProNotifyPipe
{
public:

    CProNotifyPipe();

    ~CProNotifyPipe();

    void Init();

    void Fini();

    PRO_INT64 GetReaderSockId() const;

    PRO_INT64 GetWriterSockId() const;

    void EnableNotify();

    void Notify();

private:

    PRO_INT64 m_sockIds[2];
    bool      m_enableNotify;
    PRO_INT64 m_notifyTick;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* PRO_NOTIFY_PIPE_H */
