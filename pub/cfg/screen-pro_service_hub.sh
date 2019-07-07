#!/bin/sh

THIS_DIR=$(dirname $(readlink -f "$0"))

screen -d -m -S pro_service_hub ${THIS_DIR}/run-pro_service_hub.sh
