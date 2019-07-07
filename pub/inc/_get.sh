#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

cp ${THIS_DIR}/../../src/mbedtls/include/mbedtls/*.h ${THIS_DIR}/mbedtls/

cp ${THIS_DIR}/../../src/pro/pro_shared/pro_shared.h ${THIS_DIR}/pro/

cp ${THIS_DIR}/../../src/pro/pro_util/*.h            ${THIS_DIR}/pro/

cp ${THIS_DIR}/../../src/pro/pro_net/pro_net.h       ${THIS_DIR}/pro/
cp ${THIS_DIR}/../../src/pro/pro_net/pro_mbedtls.h   ${THIS_DIR}/pro/

cp ${THIS_DIR}/../../src/pro/pro_rtp/rtp_base.h      ${THIS_DIR}/pro/
cp ${THIS_DIR}/../../src/pro/pro_rtp/rtp_msg.h       ${THIS_DIR}/pro/
