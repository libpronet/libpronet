#!/bin/sh

cp ../pub/cfg/ca.crt                           ./linux-x86-d/
cp ../pub/cfg/server.crt                       ./linux-x86-d/
cp ../pub/cfg/server.key                       ./linux-x86-d/
cp ../pub/cfg/*.cfg                            ./linux-x86-d/
cp ../pub/cfg/rtp_msg_server.db                ./linux-x86-d/
cp ../pub/cfg/pre_run.sh.txt                   ./linux-x86-d/

cp ../pub/lib-d/linux-gcc/x86/libpro_shared.so ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/libpro_net.so    ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/libpro_rtp.so    ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/pro_service_hub  ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/rtp_msg_server   ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/rtp_msg_c2s      ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/test_msg_client  ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/test_rtp         ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/test_tcp_server  ./linux-x86-d/
cp ../pub/lib-d/linux-gcc/x86/test_tcp_client  ./linux-x86-d/
