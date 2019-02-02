#!/bin/sh

cp ../pub/cfg/ca.crt                              ./linux-x86_64-d/
cp ../pub/cfg/server.crt                          ./linux-x86_64-d/
cp ../pub/cfg/server.key                          ./linux-x86_64-d/
cp ../pub/cfg/*.cfg                               ./linux-x86_64-d/
cp ../pub/cfg/rtp_msg_server.db                   ./linux-x86_64-d/
cp ../pub/cfg/pre_run.sh.txt                      ./linux-x86_64-d/

cp ../pub/lib-d/linux-gcc/x86_64/libpro_shared.so ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/libpro_net.so    ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/libpro_rtp.so    ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/pro_service_hub  ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/rtp_msg_server   ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/rtp_msg_c2s      ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/test_msg_client  ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/test_rtp         ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/test_tcp_server  ./linux-x86_64-d/
cp ../pub/lib-d/linux-gcc/x86_64/test_tcp_client  ./linux-x86_64-d/
