prolibdir = ${prefix}/libpronet/lib
proincdir = ${prefix}/libpronet/include/pronet

#############################################################################

prolib_PROGRAMS = libpro_rtp.so

proinc_HEADERS = ../../../../src/pronet/pro_rtp/rtp_base.h \
                 ../../../../src/pronet/pro_rtp/rtp_msg.h

libpro_rtp_so_SOURCES = ../../../../src/pronet/pro_rtp/rtp_base.cpp                 \
                        ../../../../src/pronet/pro_rtp/rtp_bucket.cpp               \
                        ../../../../src/pronet/pro_rtp/rtp_flow_stat.cpp            \
                        ../../../../src/pronet/pro_rtp/rtp_packet.cpp               \
                        ../../../../src/pronet/pro_rtp/rtp_port_allocator.cpp       \
                        ../../../../src/pronet/pro_rtp/rtp_reorder.cpp              \
                        ../../../../src/pronet/pro_rtp/rtp_service.cpp              \
                        ../../../../src/pronet/pro_rtp/rtp_session_a.cpp            \
                        ../../../../src/pronet/pro_rtp/rtp_session_base.cpp         \
                        ../../../../src/pronet/pro_rtp/rtp_session_mcast.cpp        \
                        ../../../../src/pronet/pro_rtp/rtp_session_mcast_ex.cpp     \
                        ../../../../src/pronet/pro_rtp/rtp_session_sslclient_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_sslserver_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_tcpclient.cpp    \
                        ../../../../src/pronet/pro_rtp/rtp_session_tcpclient_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_tcpserver.cpp    \
                        ../../../../src/pronet/pro_rtp/rtp_session_tcpserver_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_udpclient.cpp    \
                        ../../../../src/pronet/pro_rtp/rtp_session_udpclient_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_udpserver.cpp    \
                        ../../../../src/pronet/pro_rtp/rtp_session_udpserver_ex.cpp \
                        ../../../../src/pronet/pro_rtp/rtp_session_wrapper.cpp      \
                        ../../../../src/pronet/pro_rtp/rtp_msg.cpp                  \
                        ../../../../src/pronet/pro_rtp/rtp_msg_c2s.cpp              \
                        ../../../../src/pronet/pro_rtp/rtp_msg_client.cpp           \
                        ../../../../src/pronet/pro_rtp/rtp_msg_server.cpp

libpro_rtp_so_CPPFLAGS = -DPRO_RTP_EXPORTS                 \
                         -I../../../../src/pronet/pro_util \
                         -I../../../../src/pronet/pro_net

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
