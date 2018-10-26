Overview
====

LibProNet is a network communication engine in c++. It is composed of
a network library, a messaging framework as well as a toolset.

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

Please refer to "pro_net.h", "rtp_framework.h", and "rtp_foundation.h"
under the directory of "pub/inc/pro" for more.

         ______________________________________________________
        |                                                      |
        |                        ProRtp                        |
        |______________________________________________________|
        |          |                              |            |
        |          |             ProNet           |            |
        |          |______________________________|            |
        |                    |                                 |
        |                    |             ProUtil             |
        |      MbedTLS       |_________________________________|
        |                              |                       |
        |                              |       ProShared       |
        |______________________________|_______________________|
                     Fig.1 module hierarchy diagram

         ______________________________________________________
        |                                                      |
        |                      msg server                      |
        |______________________________________________________|
                 |             |        |              |
                 | ...         |        |          ... |
         ________|_______      |        |      ________|_______
        |                |     |        |     |                |
        |     msg c2s    |     |        |     |     msg c2s    |
        |________________|     |        |     |________________|
            |        |         |        |         |        |
            |  ...   |         |  ...   |         |  ...   |
         ___|___  ___|___   ___|___  ___|___   ___|___  ___|___
        |  msg  ||  msg  | |  msg  ||  msg  | |  msg  ||  msg  |
        | client|| client| | client|| client| | client|| client|
        |_______||_______| |_______||_______| |_______||_______|
                 Fig.2 structure diagram of msg system


How to compile LibProNet?
====

In some cases, you may need to define some macros. Please refer to
the file "build/DEFINE.txt" for more.

1. GCC on Linux

   To build the libraries you will need to have autoconf-2.65+
   and automake-1.11+ installed appropriately in your system.

   > cd build/linux-gcc-r/x86 (or "cd build/linux-gcc-r/x86_64")

   > chmod 777 ./autogen.sh

   > ./autogen.sh

   > make

   > make install

2. VS6 on Windows

   1)Apply patchs that is under the directory of "build/_sp_for_vs6"

   2)Open the workspace file "build/windows-vs6/pro.dsw"

3. VS2010 on Windows

   The solution file is "build/windows-vs2010/pro.sln"

4. Qt Creator on Windows

   Open the project file "build/windows-qt4.8.7/mbedtls/mbedtls.pro"

   Open the project file "build/windows-qt4.8.7/pro_util/pro_util.pro"

5. Android NDK on Windows

   Run the script "build/android-gcc-r/rebuil-all.bat"

6. Xcode on Mac OS X

   You will need to create project files manually.


How to run the messaging system?
====

1) Run the script "pub/lib-r/_get-linux-x86.sh", and
   run the script "run_box/_get-linux-x86-r.sh"

2) Adjust the config file "run_box/pro_service_hub.cfg", and
   start a "run_box/pro_service_hub" process

3) Adjust the config file "run_box/rtp_msg_server.cfg", and
   start a "run_box/rtp_msg_server" process

4) Adjust the config file "run_box/rtp_msg_client.cfg", and
   start some "run_box/rtp_msg_client" processes


How to run the tests?
====

1) Run the script "pub/lib-r/_get-linux-x86.sh", and
   run the script "run_box/_get-linux-x86-r.sh"

2) See the file "run_box/pre_run.sh.txt", and do something

3) Adjust the config file "run_box/test_tcp_server.cfg", and
   start a test_tcp_server process

   > cd run_box

   > ./test_tcp_server

4) Adjust the config file "run_box/test_tcp_client.cfg", and
   start some "run_box/test_tcp_client" processes

   > cd run_box

   > ./test_tcp_client [<server_ip> <server_port> <local_ip>]
