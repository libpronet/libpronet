#!/bin/sh

THIS_MOD=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_MOD}")

. "${THIS_DIR}/set2_proc.sh"

while [ 1 ]
do

  "${THIS_DIR}/pro_service_hub"
  sleep 1

done
