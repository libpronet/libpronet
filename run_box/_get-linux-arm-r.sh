#!/bin/sh

cp ../pub/cfg/ca.crt                           ./
cp ../pub/cfg/server.crt                       ./
cp ../pub/cfg/server.key                       ./
cp ../pub/cfg/*.cfg                            ./
cp ../pub/cfg/rtp_msg_server.db                ./
cp ../pub/cfg/pre_run.sh.txt                   ./

cp ../pub/lib-r/linux-gcc/arm/libpro_shared.so ./
cp ../pub/lib-r/linux-gcc/arm/libpro_net.so    ./
cp ../pub/lib-r/linux-gcc/arm/libpro_rtp.so    ./
cp ../pub/lib-r/linux-gcc/arm/pro_service_hub  ./
cp ../pub/lib-r/linux-gcc/arm/rtp_msg_server   ./
cp ../pub/lib-r/linux-gcc/arm/rtp_msg_c2s      ./
cp ../pub/lib-r/linux-gcc/arm/test_msg_client  ./
cp ../pub/lib-r/linux-gcc/arm/test_rtp         ./
cp ../pub/lib-r/linux-gcc/arm/test_tcp_server  ./
cp ../pub/lib-r/linux-gcc/arm/test_tcp_client  ./
