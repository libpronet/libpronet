probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = test_tcp_client

test_tcp_client_SOURCES = ../../../../src/pronet/test_tcp_client/main.cpp \
                          ../../../../src/pronet/test_tcp_client/test.cpp

test_tcp_client_CPPFLAGS = -I../../../../src/pronet/pro_util

test_tcp_client_CFLAGS   =
test_tcp_client_CXXFLAGS =

test_tcp_client_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir}
test_tcp_client_LDADD   =

LIBS = ../pro_net/libpro_net.so       \
       ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lpthread                      \
       -lc
