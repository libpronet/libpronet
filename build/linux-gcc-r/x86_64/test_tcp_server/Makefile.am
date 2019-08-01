probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = test_tcp_server

test_tcp_server_SOURCES = ../../../../src/pronet/test_tcp_server/main.cpp \
                          ../../../../src/pronet/test_tcp_server/test.cpp

test_tcp_server_CPPFLAGS =

test_tcp_server_CFLAGS   = -fno-strict-aliasing
test_tcp_server_CXXFLAGS = -fno-strict-aliasing

test_tcp_server_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir} -Wl,--no-undefined
test_tcp_server_LDADD   =

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
