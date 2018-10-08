probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = test_tcp_client

test_tcp_client_SOURCES = ../../../../src/pro/test_tcp_client/main.cpp \
                          ../../../../src/pro/test_tcp_client/test.cpp

test_tcp_client_CPPFLAGS =

test_tcp_client_CFLAGS   = -fno-strict-aliasing
test_tcp_client_CXXFLAGS = -fno-strict-aliasing

test_tcp_client_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir} -Wl,--no-undefined
test_tcp_client_LDADD   =

LIBS = ../pro_net/libpro_net.so       \
       ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lstdc++                       \
       -lrt                           \
       -lpthread                      \
       -lm                            \
       -lgcc                          \
       -lc
