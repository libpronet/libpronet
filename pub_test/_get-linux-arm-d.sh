#!/bin/sh

cp ../pub/cfg/ca.crt                           ./linux-arm-d/
cp ../pub/cfg/server.crt                       ./linux-arm-d/
cp ../pub/cfg/server.key                       ./linux-arm-d/
cp ../pub/cfg/*.cfg                            ./linux-arm-d/
cp ../pub/cfg/rtp_msg_server.db                ./linux-arm-d/
cp ../pub/cfg/set1_sys.sh                      ./linux-arm-d/
cp ../pub/cfg/set2_proc.sh                     ./linux-arm-d/

cp ../pub/lib-d/linux-gcc/arm/libpro_shared.so ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/libpro_net.so    ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/libpro_rtp.so    ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/pro_service_hub  ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/rtp_msg_server   ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/rtp_msg_c2s      ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/test_msg_client  ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/test_rtp         ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/test_tcp_server  ./linux-arm-d/
cp ../pub/lib-d/linux-gcc/arm/test_tcp_client  ./linux-arm-d/
