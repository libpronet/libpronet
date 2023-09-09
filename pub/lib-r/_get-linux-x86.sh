#!/bin/sh
THIS_MOD=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_MOD}")

cp "${THIS_DIR}/../../build/linux-gcc-r/x86/mbedtls/libmbedtls.a"            "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/pro_shared/libpro_shared.so"     "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/pro_util/libpro_util.a"          "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/pro_net/libpro_net.so"           "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/pro_rtp/libpro_rtp.so"           "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/pro_service_hub/pro_service_hub" "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/rtp_msg_server/rtp_msg_server"   "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/rtp_msg_c2s/rtp_msg_c2s"         "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/test_msg_client/test_msg_client" "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/test_rtp/test_rtp"               "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/test_tcp_server/test_tcp_server" "${THIS_DIR}/linux-gcc/x86/"
cp "${THIS_DIR}/../../build/linux-gcc-r/x86/test_tcp_client/test_tcp_client" "${THIS_DIR}/linux-gcc/x86/"
