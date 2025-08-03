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
export CROSS_COMPILE=aarch64-linux-gnu-
./configure \
--host=aarch64-linux-gnu \
CPPFLAGS="-DNDEBUG          \
          -D_GNU_SOURCE     \
          -D_LIBC_REENTRANT \
          -D_REENTRANT"     \
CFLAGS="             -O2 -Wall -fno-strict-aliasing -fvisibility=hidden" \
CXXFLAGS="-std=c++11 -O2 -Wall -fno-strict-aliasing -fvisibility=hidden" \
LDFLAGS="" $@

rm -f ./configure
