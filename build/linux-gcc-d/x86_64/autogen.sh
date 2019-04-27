#!/bin/sh

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
CPPFLAGS="-D_DEBUG                          \
          -D_GNU_SOURCE                     \
          -D_LIBC_REENTRANT                 \
          -D_REENTRANT                      \
          -DPRO_HAS_ATOMOP                  \
          -DPRO_HAS_ACCEPT4                 \
          -DPRO_HAS_EPOLL                   \
          -DPRO_HAS_PTHREAD_EXPLICIT_SCHED" \
CFLAGS="  -g -O0 -Wall -march=nocona -m64"  \
CXXFLAGS="-g -O0 -Wall -march=nocona -m64"  \
LDFLAGS="" $@

rm -f ./configure
