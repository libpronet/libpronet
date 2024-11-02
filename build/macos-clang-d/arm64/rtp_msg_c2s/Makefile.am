probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = rtp_msg_c2s

rtp_msg_c2s_SOURCES = ../../../../src/pronet/rtp_msg_c2s/c2s_server.cpp \
                      ../../../../src/pronet/rtp_msg_c2s/main.cpp

rtp_msg_c2s_CPPFLAGS = -I../../../../src/pronet/pro_util \
                       -I../../../../src/pronet/pro_net

rtp_msg_c2s_CFLAGS   =
rtp_msg_c2s_CXXFLAGS =

rtp_msg_c2s_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir}
rtp_msg_c2s_LDADD   =

LIBS = ../pro_rtp/libpro_rtp.so       \
       ../pro_net/libpro_net.so       \
       ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lpthread                      \
       -lc
