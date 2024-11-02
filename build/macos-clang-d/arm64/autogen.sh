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
./configure \
CPPFLAGS="-D_DEBUG          \
          -D_GNU_SOURCE     \
          -D_LIBC_REENTRANT \
          -D_REENTRANT"     \
CFLAGS="             -g -O0 -Wall -fno-strict-aliasing -fvisibility=hidden -march=armv8-a -m64" \
CXXFLAGS="-std=c++11 -g -O0 -Wall -fno-strict-aliasing -fvisibility=hidden -march=armv8-a -m64" \
LDFLAGS="" $@

rm -f ./configure
