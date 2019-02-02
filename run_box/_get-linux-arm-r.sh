#!/bin/sh

cp ../pub/cfg/ca.crt                           ./linux-arm-r/
cp ../pub/cfg/server.crt                       ./linux-arm-r/
cp ../pub/cfg/server.key                       ./linux-arm-r/
cp ../pub/cfg/*.cfg                            ./linux-arm-r/
cp ../pub/cfg/rtp_msg_server.db                ./linux-arm-r/
cp ../pub/cfg/pre_run.sh.txt                   ./linux-arm-r/

cp ../pub/lib-r/linux-gcc/arm/libpro_shared.so ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/libpro_net.so    ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/libpro_rtp.so    ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/pro_service_hub  ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/rtp_msg_server   ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/rtp_msg_c2s      ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/test_msg_client  ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/test_rtp         ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/test_tcp_server  ./linux-arm-r/
cp ../pub/lib-r/linux-gcc/arm/test_tcp_client  ./linux-arm-r/
