probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = rtp_msg_c2s

rtp_msg_c2s_SOURCES = ../../../../src/pronet/rtp_msg_c2s/c2s_server.cpp \
                      ../../../../src/pronet/rtp_msg_c2s/main.cpp

rtp_msg_c2s_CPPFLAGS = -I../../../../src/pronet/pro_util \
                       -I../../../../src/pronet/pro_net

rtp_msg_c2s_CFLAGS   = -fno-strict-aliasing
rtp_msg_c2s_CXXFLAGS = -fno-strict-aliasing

rtp_msg_c2s_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir} -Wl,--no-undefined
rtp_msg_c2s_LDADD   =

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
