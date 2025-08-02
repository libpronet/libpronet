Overview
====

LibProNet(https://github.com/libpronet/libpronet) is a network
communication engine in C++. It is composed of a network library,
a messaging framework as well as a toolset.

LibProNet is designed to be a reliable, easy-to-use and cross-platform
solution, which is object-oriented and has a layered structure.

LibProNet has 3 parts: a basic toolset, a network abstract layer and
an application layer.

The toolset layer provides some commonly used utilities in a network
communication application, including threads, tasks, locks, timers,
memory pool etc.

The network abstract layer encapsulates the socket I/O model and
abstracts a communication as a set of objects, such as reactor,
transporter, receiver, connector, handshaker, SSL/TLS objects,
and the sharing socket.

The application layer, incorporating the encapsulation of the RTP
protocol and sorts of RTP sessions, is an infrastructure of real-time
messaging.

LibProNet is capable of handling millions of concurrent connections
with ease, qualifying as a underlying communication engine of a large
service cluster system, a multimedia communication system or
a messaging system.

Please refer to "pro_net.h", "rtp_base.h", and "rtp_msg.h" under the
directory of "pub/inc/pronet" for more.

         ______________________________________________________
        |         |                                  |         |
        |         |              ProRtp              |         |
        |         |__________________________________|         |
        |                                   |                  |
        |               ProNet              |                  |
        |___________________________________|                  |
        |                    |                     ProUtil     |
        |                    |                                 |
        |                    |_________________________________|
        |      MbedTLS                 |                       |
        |                              |       ProShared       |
        |______________________________|_______________________|
                     Fig.1 Module hierarchy diagram

         ________________   ________________   ________________
        |    Acceptor    | |   Connector    | |   Handshaker   |
        | (EventHandler) | | (EventHandler) | | (EventHandler) |
        |________________| |________________| |________________|
                 A                  A                  A
                 |       ...        |       ...        |
         ________|__________________|__________________|_______
        |                       Reactor                        |
        |   ____________     _____________     _____________   |
        |  A            |   A             |   A             |  |
        |  |     Acc    |   |     I/O     |   |   General   |  |
        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
        |  |____________V   |_____________V   |_____________V  |
        |                                                      |
        |   ____________     _____________     _____________   |
        |  A            |   A             |   A             |  |
        |  |    I/O     |   |     I/O     |   |     MM      |  |
        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
        |  |____________V   |_____________V   |_____________V  |
        |                                                      |
        |       ...               ...               ...        |
        |______________________________________________________|
                 |                  |                  |
                 |       ...        |       ...        |
         ________V_______   ________V_______   ________V_______
        |   Transport    | |   ServiceHub   | |  ServiceHost   |
        | (EventHandler) | | (EventHandler) | | (EventHandler) |
        |________________| |________________| |________________|
                   Fig.2 Structure diagram of Reactor

        __________________                     _________________
       |    ServiceHub    |<----------------->|   ServiceHost   |
       |   ____________   |    ServicePipe    |(Acceptor Shadow)| Audio-Process
       |  |            |  |                   |_________________|
       |  |  Acceptor  |  |        ...         _________________
       |  |____________|  |                   |   ServiceHost   |
       |                  |    ServicePipe    |(Acceptor Shadow)| Video-Process
       |__________________|<----------------->|_________________|
            Hub-Process
                Fig.3-1 Structure diagram of ServiceHub

        __________________                     _________________
       |    ServiceHub    |<----------------->|   RtpService    |
       |   ____________   |    ServicePipe    |(Acceptor Shadow)| Audio-Process
       |  |            |  |                   |_________________|
       |  |  Acceptor  |  |        ...         _________________
       |  |____________|  |                   |   RtpService    |
       |                  |    ServicePipe    |(Acceptor Shadow)| Video-Process
       |__________________|<----------------->|_________________|
            Hub-Process
                Fig.3-2 Structure diagram of RtpService

                ______________________________
        ====>>in__________ o                  |__________
                          | o                  __________out====>>
                          |..ooooooooooooooooo|
                          |ooooooooooooooooooo|
                          |ooooooooooooooooooo|
                          |ooooooooooooooooooo|
                          |ooooooooooooooooooo|
                 Fig.4 Structure diagram of RtpReorder

         ______________________________________________________
        |                                                      |
        |                      Msg server                      |
        |______________________________________________________|
                 |             |        |              |
                 | ...         |        |          ... |
         ________|_______      |        |      ________|_______
        |                |     |        |     |                |
        |     Msg c2s    |     |        |     |     Msg c2s    |
        |________________|     |        |     |________________|
            |        |         |        |         |        |
            |  ...   |         |  ...   |         |  ...   |
         ___|___  ___|___   ___|___  ___|___   ___|___  ___|___
        |  Msg  ||  Msg  | |  Msg  ||  Msg  | |  Msg  ||  Msg  |
        | client|| client| | client|| client| | client|| client|
        |_______||_______| |_______||_______| |_______||_______|
                 Fig.5 Structure diagram of msg system


How to compile LibProNet?
====

In some cases, you may need to define some macros. Please refer to
the file "build/DEFINE.txt" for more.

1. GCC on Linux

   To build the libraries you will need to have autoconf-2.63+
   and automake-1.11+ installed appropriately in your system.

   On Ubuntu:
   > apt-get install autoconf automake gcc g++ make

   On CentOS:
   > yum install autoconf automake gcc gcc-c++ make

   > cd build/linux-gcc-r/x86_64 (or "cd build/linux-gcc-r/x86")

   > ./autogen.sh

   > make

   > make install

2. VS2022 on Windows

   The solution file is "build/windows-vs2022/pronet.sln"

3. Qt Creator on Windows

   Open the project file "build/windows-qt-gcc/mbedtls/mbedtls.pro"

   Open the project file "build/windows-qt-gcc/pro_util/pro_util.pro"

4. Android NDK on Windows

   Run the script "build/android-clang-r/rebuil-all.bat"

5. Clang on Mac OS

   > cd build/macos-clang-r/arm64

   > ./autogen.sh

   > make

   > make install


How to run the messaging system on Linux?
====

By default, the installation location is "/usr/local/libpronet",
which we call "${install}".

1) Execute the scripts "${install}/bin/set1_sys.sh" and
   "${install}/bin/set2_proc.sh" in the current shell

   > cd ${install}/bin

   > . ./set1_sys.sh

   > . ./set2_proc.sh

2) Adjust the config file "${install}/bin/pro_service_hub.cfg", and
   start a "${install}/bin/pro_service_hub" process

   > ./pro_service_hub

3) Adjust the config file "${install}/bin/rtp_msg_server.cfg", and
   start a "${install}/bin/rtp_msg_server" process

   > ./rtp_msg_server

4) Adjust the config file "${install}/bin/test_msg_client.cfg", and
   start some "${install}/bin/test_msg_client" processes

   > ./test_msg_client


How to run the tests on Linux?
====

By default, the installation location is "/usr/local/libpronet",
which we call "${install}".

1) Execute the scripts "${install}/bin/set1_sys.sh" and
   "${install}/bin/set2_proc.sh" in the current shell

   > cd ${install}/bin

   > . ./set1_sys.sh

   > . ./set2_proc.sh

2) Adjust the config file "${install}/bin/test_tcp_server.cfg", and
   start a "${install}/bin/test_tcp_server" process

   > ./test_tcp_server

3) Adjust the config file "${install}/bin/test_tcp_client.cfg", and
   start some "${install}/bin/test_tcp_client" processes

   > ./test_tcp_client
   or
   > ./test_tcp_client <server_ip> <server_port>
   or
   > ./test_tcp_client <server_ip> <server_port> <local_ip>


How to run the messaging system on Windows(64bit)?
====

1) Run the script "build/windows-vs2022/_release64/_get.bat"

2) Adjust the config file "build/windows-vs2022/_release64/pro_service_hub.cfg",
   and start a "build/windows-vs2022/_release64/pro_service_hub.exe" process

   > cd build\windows-vs2022\_release64

   > pro_service_hub

3) Adjust the config file "build/windows-vs2022/_release64/rtp_msg_server.cfg",
   and start a "build/windows-vs2022/_release64/rtp_msg_server.exe" process

   > rtp_msg_server

4) Adjust the config file "build/windows-vs2022/_release64/test_msg_client.cfg",
   and start some "build/windows-vs2022/_release64/test_msg_client.exe" processes

   > test_msg_client


How to run the tests on Windows(64bit)?
====

1) Run the script "build/windows-vs2022/_release64/_get.bat"

2) Adjust the config file "build/windows-vs2022/_release64/test_tcp_server.cfg",
   and start a "build/windows-vs2022/_release64/test_tcp_server.exe" process

   > cd build\windows-vs2022\_release64

   > test_tcp_server

3) Adjust the config file "build/windows-vs2022/_release64/test_tcp_client.cfg",
   and start some "build/windows-vs2022/_release64/test_tcp_client.exe" processes

   > test_tcp_client
   or
   > test_tcp_client <server_ip> <server_port>
   or
   > test_tcp_client <server_ip> <server_port> <local_ip>
