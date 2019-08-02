#!/bin/sh

rm -rf ./mbedtls/.deps
rm -rf ./pro_shared/.deps
rm -rf ./pro_util/.deps
rm -rf ./pro_net/.deps
rm -rf ./pro_rtp/.deps
rm -rf ./pro_service_hub/.deps
rm -rf ./rtp_msg_server/.deps
rm -rf ./rtp_msg_c2s/.deps
rm -rf ./test_msg_client/.deps
rm -rf ./test_rtp/.deps
rm -rf ./test_tcp_server/.deps
rm -rf ./test_tcp_client/.deps
rm -rf ./cfg/.deps

#
# configure.ac ---> aclocal.m4
#
aclocal --force

#
# configure.ac + aclocal.m4 ---> configure
#
autoconf --force

#
# configure.ac + Makefile.am ---> Makefile.in
#
automake --add-missing --force-missing --foreign

#
# Makefile.in ---> Makefile
#
./configure                                 \
CPPFLAGS="-DNDEBUG                          \
          -D_GNU_SOURCE                     \
          -D_LIBC_REENTRANT                 \
          -D_REENTRANT                      \
          -DPRO_HAS_ATOMOP                  \
          -DPRO_HAS_ACCEPT4                 \
          -DPRO_HAS_EPOLL                   \
          -DPRO_HAS_PTHREAD_EXPLICIT_SCHED" \
CFLAGS="  -O2 -Wall -march=nocona -m64"     \
CXXFLAGS="-O2 -Wall -march=nocona -m64"     \
LDFLAGS="" $@

rm -f ./configure
