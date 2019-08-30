#!/bin/sh

THIS_MOD=$(readlink -f "$0")
THIS_DIR=$(dirname "${THIS_MOD}")

screen -d -m -S pro_service_hub "${THIS_DIR}/run-pro_service_hub.sh"
