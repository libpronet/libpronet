#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

cp ${THIS_DIR}/../pub/cfg/ca.crt                           ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/server.crt                       ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/server.key                       ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/*.cfg                            ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/rtp_msg_server.db                ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/set1_sys.sh                      ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/set2_proc.sh                     ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/run-pro_service_hub.sh           ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/cfg/screen-pro_service_hub.sh        ${THIS_DIR}/linux-arm-d/

cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/libpro_shared.so ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/libpro_net.so    ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/libpro_rtp.so    ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/pro_service_hub  ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/rtp_msg_server   ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/rtp_msg_c2s      ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/test_msg_client  ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/test_rtp         ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/test_tcp_server  ${THIS_DIR}/linux-arm-d/
cp ${THIS_DIR}/../pub/lib-d/linux-gcc/arm/test_tcp_client  ${THIS_DIR}/linux-arm-d/
