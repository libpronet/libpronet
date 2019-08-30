#!/bin/sh

THIS_MOD=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_MOD}")

cp "${THIS_DIR}/../pub/cfg/ca.crt"                           "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/server.crt"                       "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/server.key"                       "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/*.cfg"                            "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/rtp_msg_server.db"                "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/set1_sys.sh"                      "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/set2_proc.sh"                     "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/run-pro_service_hub.sh"           "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/cfg/screen-pro_service_hub.sh"        "${THIS_DIR}/linux-x86-d/"

cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/libpro_shared.so" "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/libpro_net.so"    "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/libpro_rtp.so"    "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/pro_service_hub"  "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/rtp_msg_server"   "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/rtp_msg_c2s"      "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/test_msg_client"  "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/test_rtp"         "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/test_tcp_server"  "${THIS_DIR}/linux-x86-d/"
cp "${THIS_DIR}/../pub/lib-d/linux-gcc/x86/test_tcp_client"  "${THIS_DIR}/linux-x86-d/"
