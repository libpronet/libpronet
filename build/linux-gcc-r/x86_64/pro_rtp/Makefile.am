probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib
proincdir = ${prefix}/libpronet/include/pro

#############################################################################

prolib_PROGRAMS = libpro_rtp.so

proinc_HEADERS = ../../../../src/pro/pro_rtp/rtp_framework.h \
                 ../../../../src/pro/pro_rtp/rtp_foundation.h

libpro_rtp_so_SOURCES = ../../../../src/pro/pro_rtp/rtp_framework.cpp            \
                        ../../../../src/pro/pro_rtp/rtp_packet.cpp               \
                        ../../../../src/pro/pro_rtp/rtp_port_allocator.cpp       \
                        ../../../../src/pro/pro_rtp/rtp_service.cpp              \
                        ../../../../src/pro/pro_rtp/rtp_session_base.cpp         \
                        ../../../../src/pro/pro_rtp/rtp_session_mcast.cpp        \
                        ../../../../src/pro/pro_rtp/rtp_session_mcast_ex.cpp     \
                        ../../../../src/pro/pro_rtp/rtp_session_sslclient_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_session_sslserver_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_session_tcpclient.cpp    \
                        ../../../../src/pro/pro_rtp/rtp_session_tcpclient_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_session_tcpserver.cpp    \
                        ../../../../src/pro/pro_rtp/rtp_session_tcpserver_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_session_udpclient.cpp    \
                        ../../../../src/pro/pro_rtp/rtp_session_udpclient_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_session_udpserver.cpp    \
                        ../../../../src/pro/pro_rtp/rtp_session_udpserver_ex.cpp \
                        ../../../../src/pro/pro_rtp/rtp_bucket.cpp               \
                        ../../../../src/pro/pro_rtp/rtp_flow_stat.cpp            \
                        ../../../../src/pro/pro_rtp/rtp_foundation.cpp           \
                        ../../../../src/pro/pro_rtp/rtp_msg_c2s.cpp              \
                        ../../../../src/pro/pro_rtp/rtp_msg_client.cpp           \
                        ../../../../src/pro/pro_rtp/rtp_msg_server.cpp           \
                        ../../../../src/pro/pro_rtp/rtp_session_wrapper.cpp

libpro_rtp_so_CPPFLAGS = -DPRO_RTP_EXPORTS \
                         -I../../../../src/pro/pro_net

libpro_rtp_so_CFLAGS   = -fno-strict-aliasing -fPIC -fvisibility=hidden
libpro_rtp_so_CXXFLAGS = -fno-strict-aliasing -fPIC -fvisibility=hidden

libpro_rtp_so_LDFLAGS = -shared -Wl,-soname,libpro_rtp.so \
                        -Wl,-rpath,. -Wl,--no-undefined
libpro_rtp_so_LDADD   =

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
