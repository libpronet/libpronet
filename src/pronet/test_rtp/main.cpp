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

#include "test.h"
#include "../pro_net/pro_net.h"
#include "../pro_rtp/rtp_base.h"
#include "../pro_util/pro_time_util.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define THREAD_COUNT 2

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    ProNetInit();
    ProRtpInit();

    const char*  remote_ip   = NULL;
    int          remote_port = 0;
    const char*  local_ip    = NULL;
    int          local_port  = 0;
    int          kbps        = 0;
    IProReactor* reactor     = NULL;
    CTest*       tester      = NULL;

    if (argc < 6)
    {
        printf(
            " test_rtp --- error! too few arguments. \n"
            "\n"
            " usage: \n"
            " test_rtp <remote_ip> <remote_port> <local_ip> <local_port> <kbps> \n"
            "\n"
            " for example: \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       4000 1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 4000 1024 \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       0    1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 0    1024 \n"
            " test_rtp 0.0.0.0       0    0.0.0.0       0    0   \n"
            "\n"
            );

        goto EXIT;
    }

    remote_ip   = argv[1];
    remote_port = atoi(argv[2]);
    local_ip    = argv[3];
    local_port  = atoi(argv[4]);
    kbps        = atoi(argv[5]);

    /*
     * remote_port
     */
    assert(remote_port >= 0);
    assert(remote_port <= 65535);
    if (remote_port < 0 || remote_port > 65535)
    {
        printf(
            " test_rtp --- error! invalid argument <remote_port>. \n"
            "\n"
            " usage: \n"
            " test_rtp <remote_ip> <remote_port> <local_ip> <local_port> <kbps> \n"
            "\n"
            " for example: \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       4000 1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 4000 1024 \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       0    1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 0    1024 \n"
            " test_rtp 0.0.0.0       0    0.0.0.0       0    0   \n"
            "\n"
            );

        goto EXIT;
    }

    /*
     * local_port
     */
    assert(local_port >= 0);
    assert(local_port <= 65535);
    if (local_port < 0 || local_port > 65535)
    {
        printf(
            " test_rtp --- error! invalid argument <local_port>. \n"
            "\n"
            " usage: \n"
            " test_rtp <remote_ip> <remote_port> <local_ip> <local_port> <kbps> \n"
            "\n"
            " for example: \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       4000 1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 4000 1024 \n"
            " test_rtp 192.168.0.101 4000 0.0.0.0       0    1024 \n"
            " test_rtp 192.168.0.101 4000 192.168.0.102 0    1024 \n"
            " test_rtp 0.0.0.0       0    0.0.0.0       0    0   \n"
            "\n"
            );

        goto EXIT;
    }

    /*
     * kbps
     */
    if (kbps < 0)
    {
        kbps = 0;
    }
    if (kbps > 2048000)
    {
        kbps = 2048000;
    }

    reactor = ProCreateReactor(THREAD_COUNT);
    if (reactor == NULL)
    {
        printf(" test_rtp --- error! can't create reactor. \n\n");

        goto EXIT;
    }

    tester = CTest::CreateInstance();
    if (tester == NULL)
    {
        printf(" test_rtp --- error! can't create tester. \n\n");

        goto EXIT;
    }
    if (!tester->Init(
        reactor,
        remote_ip,
        (unsigned short)remote_port,
        local_ip,
        (unsigned short)local_port,
        kbps * 1000
        ))
    {
        printf(" test_rtp --- error! can't init tester. \n\n");

        goto EXIT;
    }

    ProSleep(-1);

EXIT:

    if (tester != NULL)
    {
        tester->Fini();
        tester->Release();
    }

    ProDeleteReactor(reactor);
    ProSleep(3000);

    return (0);
}
