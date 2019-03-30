probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = test_msg_client

test_msg_client_SOURCES = ../../../../src/pro/test_msg_client/main.cpp \
                          ../../../../src/pro/test_msg_client/test.cpp

test_msg_client_CPPFLAGS = -I../../../../src/pro/pro_net

test_msg_client_CFLAGS   = -fno-strict-aliasing
test_msg_client_CXXFLAGS = -fno-strict-aliasing

test_msg_client_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir} -Wl,--no-undefined
test_msg_client_LDADD   =

LIBS = ../pro_rtp/libpro_rtp.so       \
       ../pro_net/libpro_net.so       \
       ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lstdc++                       \
       -lrt                           \
       -lpthread                      \
       -lm                            \
       -lgcc                          \
       -lc
