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

static
TEST_MODE
PRO_CALLTYPE
String2Mode_i(const char* modeString)
{
    TEST_MODE mode = 0;

    assert(modeString != NULL);
    assert(modeString[0] != '\0');
    if (modeString == NULL || modeString[0] == '\0')
    {
        return (mode);
    }

    if (stricmp(modeString, "-udpecho")  == 0 ||
        stricmp(modeString, "--udpecho") == 0)
    {
        mode = TM_UDPE;
    }
    else if (
        stricmp(modeString, "-tcpecho")  == 0 ||
        stricmp(modeString, "--tcpecho") == 0
        )
    {
        mode = TM_TCPE;
    }
    else if (
        stricmp(modeString, "-udps")  == 0 ||
        stricmp(modeString, "--udps") == 0
        )
    {
        mode = TM_UDPS;
    }
    else if (
        stricmp(modeString, "-tcps")  == 0 ||
        stricmp(modeString, "--tcps") == 0
        )
    {
        mode = TM_TCPS;
    }
    else if (
        stricmp(modeString, "-udpc")  == 0 ||
        stricmp(modeString, "--udpc") == 0
        )
    {
        mode = TM_UDPC;
    }
    else if (
        stricmp(modeString, "-tcpc")  == 0 ||
        stricmp(modeString, "--tcpc") == 0
        )
    {
        mode = TM_TCPC;
    }
    else
    {
    }

    return (mode);
}

/////////////////////////////////////////////////////////////////////////////
////

int main(int argc, char* argv[])
{
    printf(
        "\n"
        " usage [server-side]: \n"
        " test_rtp -udpecho [ <local_ip> <local_port> ] \n"
        " test_rtp -tcpecho [ <local_ip> <local_port> ] \n"
        " test_rtp -udps    [ <local_ip> <local_port> <kbps> [packet_size] ] \n"
        " test_rtp -tcps    [ <local_ip> <local_port> <kbps> [packet_size] ] \n"
        "\n"
        " usage [client-side]: \n"
        " test_rtp -udpc    <remote_ip> <remote_port> <kbps> [ <packet_size> [local_ip] ] \n"
        " test_rtp -tcpc    <remote_ip> <remote_port> <kbps> [ <packet_size> [local_ip] ] \n"
        "\n"
        " for example [server-side on host 192.168.0.101]: \n"
        " test_rtp -udpecho                            \n"
        " test_rtp -udpecho 0.0.0.0       1234         \n"
        " test_rtp -udpecho 192.168.0.101 1234         \n"
        " test_rtp -tcpecho                            \n"
        " test_rtp -tcpecho 0.0.0.0       1234         \n"
        " test_rtp -tcpecho 192.168.0.101 1234         \n"
        " test_rtp -udps                               \n"
        " test_rtp -udps    0.0.0.0       1234 512     \n"
        " test_rtp -udps    192.168.0.101 1234 512 850 \n"
        " test_rtp -tcps                               \n"
        " test_rtp -tcps    0.0.0.0       1234 512 850 \n"
        " test_rtp -tcps    192.168.0.101 1234 512     \n"
        "\n"
        " for example [client-side on host 192.168.0.102]: \n"
        " test_rtp -udpc    192.168.0.101 1234 512                   \n"
        " test_rtp -udpc    192.168.0.101 1234 512 850               \n"
        " test_rtp -udpc    192.168.0.101 1234 512 850 192.168.0.102 \n"
        " test_rtp -tcpc    192.168.0.101 1234 512                   \n"
        " test_rtp -tcpc    192.168.0.101 1234 512 850               \n"
        " test_rtp -tcpc    192.168.0.101 1234 512 850 0.0.0.0       \n"
        );

    ProNetInit();
    ProRtpInit();

    TEST_MODE    mode        = 0;
    const char*  local_ip    = "0.0.0.0";
    int          local_port  = 1234;
    const char*  remote_ip   = NULL;
    int          remote_port = 0;
    int          kbps        = 512;
    int          packet_size = 850;
    IProReactor* reactor     = NULL;
    CTest*       tester      = NULL;

    CProStlString timeString = "";
    ProGetLocalTimeString(timeString);

    if (argc < 2)
    {
        printf(
            "\n"
            "%s \n"
            " test_rtp --- error! too few arguments. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

    mode = String2Mode_i(argv[1]);
    if (mode == 0)
    {
        printf(
            "\n"
            "%s \n"
            " test_rtp --- error! invalid argument <-mode>. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

    TEST_PARAMS params;
    memset(&params, 0, sizeof(TEST_PARAMS));

    switch (mode)
    {
    case TM_UDPE:
    case TM_TCPE:
    case TM_UDPS:
    case TM_TCPS:
        {
            TEST_SERVER& prm = params.server;

            if (
                (mode == TM_UDPE || mode == TM_TCPE)
                &&
                argc > 2
               )
            {
                if (argc < 4)
                {
                    printf(
                        "\n"
                        "%s \n"
                        " test_rtp --- error! too few arguments. \n"
                        ,
                        timeString.c_str()
                        );

                    goto EXIT;
                }

                local_ip   = argv[2];
                local_port = atoi(argv[3]);
            }
            else if (
                (mode == TM_UDPS || mode == TM_TCPS)
                &&
                argc > 2
               )
            {
                if (argc < 5)
                {
                    printf(
                        "\n"
                        "%s \n"
                        " test_rtp --- error! too few arguments. \n"
                        ,
                        timeString.c_str()
                        );

                    goto EXIT;
                }

                local_ip        = argv[2];
                local_port      = atoi(argv[3]);
                kbps            = atoi(argv[4]);
                if (argc >= 6)
                {
                    packet_size = atoi(argv[5]);
                }
            }
            else
            {
            }

            /*
             * local_ip
             */
            strncpy_pro(prm.local_ip, sizeof(prm.local_ip), local_ip);

            /*
             * local_port
             */
            assert(local_port > 0);
            assert(local_port <= 65535);
            if (local_port <= 0 || local_port > 65535)
            {
                printf(
                    "\n"
                    "%s \n"
                    " test_rtp --- error! invalid argument <local_port>. \n"
                    ,
                    timeString.c_str()
                    );

                goto EXIT;
            }
            else
            {
                prm.local_port = (unsigned short)local_port;
            }

            /*
             * kbps
             */
            if (kbps < 1)
            {
                kbps = 1;
            }
            if (kbps > 8192000)
            {
                kbps = 8192000;
            }
            prm.kbps = kbps;

            /*
             * packet_size
             */
            if (packet_size < 1)
            {
                packet_size = 1;
            }
            if (packet_size > 1500)
            {
                packet_size = 1500;
            }
            prm.packet_size = packet_size;
            break;
        }
    case TM_UDPC:
    case TM_TCPC:
        {
            TEST_CLIENT& prm = params.client;

            if (argc < 5)
            {
                printf(
                    "\n"
                    "%s \n"
                    " test_rtp --- error! too few arguments. \n"
                    ,
                    timeString.c_str()
                    );

                goto EXIT;
            }

            remote_ip       = argv[2];
            remote_port     = atoi(argv[3]);
            kbps            = atoi(argv[4]);
            if (argc >= 6)
            {
                packet_size = atoi(argv[5]);
            }
            if (argc >= 7)
            {
                local_ip    = argv[6];
            }

            /*
             * remote_ip
             */
            strncpy_pro(prm.remote_ip, sizeof(prm.remote_ip), remote_ip);

            /*
             * remote_port
             */
            assert(remote_port > 0);
            assert(remote_port <= 65535);
            if (remote_port <= 0 || remote_port > 65535)
            {
                printf(
                    "\n"
                    "%s \n"
                    " test_rtp --- error! invalid argument <remote_port>. \n"
                    ,
                    timeString.c_str()
                    );

                goto EXIT;
            }
            else
            {
                prm.remote_port = (unsigned short)remote_port;
            }

            /*
             * kbps
             */
            if (kbps < 1)
            {
                kbps = 1;
            }
            if (kbps > 8192000)
            {
                kbps = 8192000;
            }
            prm.kbps = kbps;

            /*
             * packet_size
             */
            if (packet_size < 1)
            {
                packet_size = 1;
            }
            if (packet_size > 1500)
            {
                packet_size = 1500;
            }
            prm.packet_size = packet_size;

            /*
             * local_ip
             */
            strncpy_pro(prm.local_ip, sizeof(prm.local_ip), local_ip);
            break;
        }
    } /* end of switch (...) */

    reactor = ProCreateReactor(THREAD_COUNT);
    if (reactor == NULL)
    {
        printf(
            "\n"
            "%s \n"
            " test_rtp --- error! can't create reactor. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

    tester = CTest::CreateInstance(mode);
    if (tester == NULL)
    {
        printf(
            "\n"
            "%s \n"
            " test_rtp --- error! can't create tester. \n"
            ,
            timeString.c_str()
            );

        goto EXIT;
    }

    if (CTest::IsServerMode(mode))
    {
        if (!tester->Init(reactor, params.server))
        {
            printf(
                "\n"
                "%s \n"
                " test_rtp --- error! can't init tester. \n"
                ,
                timeString.c_str()
                );

            goto EXIT;
        }
    }
    else
    {
        if (!tester->Init(reactor, params.client))
        {
            printf(
                "\n"
                "%s \n"
                " test_rtp --- error! can't init tester. \n"
                ,
                timeString.c_str()
                );

            goto EXIT;
        }
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
