#!/bin/sh

cp ../pub/cfg/ca.crt                              ./linux-x86_64-r/
cp ../pub/cfg/server.crt                          ./linux-x86_64-r/
cp ../pub/cfg/server.key                          ./linux-x86_64-r/
cp ../pub/cfg/*.cfg                               ./linux-x86_64-r/
cp ../pub/cfg/rtp_msg_server.db                   ./linux-x86_64-r/
cp ../pub/cfg/set1_sys.sh                         ./linux-x86_64-r/
cp ../pub/cfg/set2_proc.sh                        ./linux-x86_64-r/

cp ../pub/lib-r/linux-gcc/x86_64/libpro_shared.so ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/libpro_net.so    ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/libpro_rtp.so    ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/pro_service_hub  ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/rtp_msg_server   ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/rtp_msg_c2s      ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/test_msg_client  ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/test_rtp         ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/test_tcp_server  ./linux-x86_64-r/
cp ../pub/lib-r/linux-gcc/x86_64/test_tcp_client  ./linux-x86_64-r/
