#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

. ${THIS_DIR}/set2_proc.sh

while [ 1 ]
do

${THIS_DIR}/pro_service_hub

sleep 1
done
