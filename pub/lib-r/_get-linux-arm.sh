#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

cp ${THIS_DIR}/../../build/linux-gcc-r/arm/mbedtls/libmbedtls.a            ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/pro_shared/libpro_shared.so     ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/pro_util/libpro_util.a          ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/pro_net/libpro_net.so           ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/pro_rtp/libpro_rtp.so           ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/pro_service_hub/pro_service_hub ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/rtp_msg_server/rtp_msg_server   ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/rtp_msg_c2s/rtp_msg_c2s         ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/test_msg_client/test_msg_client ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/test_rtp/test_rtp               ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/test_tcp_server/test_tcp_server ${THIS_DIR}/linux-gcc/arm/
cp ${THIS_DIR}/../../build/linux-gcc-r/arm/test_tcp_client/test_tcp_client ${THIS_DIR}/linux-gcc/arm/
