probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = test_rtp

test_rtp_SOURCES = ../../../../src/pronet/test_rtp/main.cpp \
                   ../../../../src/pronet/test_rtp/test.cpp

test_rtp_CPPFLAGS = -I../../../../src/pronet/pro_util \
                    -I../../../../src/pronet/pro_net

test_rtp_CFLAGS   =
test_rtp_CXXFLAGS =

test_rtp_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir} -Wl,--no-undefined
test_rtp_LDADD   =

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
