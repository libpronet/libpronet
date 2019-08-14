#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

cp ${THIS_DIR}/../../src/mbedtls/include/mbedtls/*.h    ${THIS_DIR}/mbedtls/

cp ${THIS_DIR}/../../src/pronet/pro_shared/pro_shared.h ${THIS_DIR}/pronet/

cp ${THIS_DIR}/../../src/pronet/pro_util/*.h            ${THIS_DIR}/pronet/

cp ${THIS_DIR}/../../src/pronet/pro_net/pro_net.h       ${THIS_DIR}/pronet/
cp ${THIS_DIR}/../../src/pronet/pro_net/pro_ssl.h       ${THIS_DIR}/pronet/

cp ${THIS_DIR}/../../src/pronet/pro_rtp/rtp_base.h      ${THIS_DIR}/pronet/
cp ${THIS_DIR}/../../src/pronet/pro_rtp/rtp_msg.h       ${THIS_DIR}/pronet/
