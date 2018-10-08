probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib
proincdir = ${prefix}/libpronet/include/pro

#############################################################################

prolib_PROGRAMS = libpro_net.so

proinc_HEADERS = ../../../../src/pro/pro_net/pro_net.h \
                 ../../../../src/pro/pro_net/pro_mbedtls.h

libpro_net_so_SOURCES = ../../../../src/pro/pro_net/pro_acceptor.cpp        \
                        ../../../../src/pro/pro_net/pro_base_reactor.cpp    \
                        ../../../../src/pro/pro_net/pro_connector.cpp       \
                        ../../../../src/pro/pro_net/pro_epoll_reactor.cpp   \
                        ../../../../src/pro/pro_net/pro_handler_mgr.cpp     \
                        ../../../../src/pro/pro_net/pro_mbedtls.cpp         \
                        ../../../../src/pro/pro_net/pro_mcast_transport.cpp \
                        ../../../../src/pro/pro_net/pro_net.cpp             \
                        ../../../../src/pro/pro_net/pro_notify_pipe.cpp     \
                        ../../../../src/pro/pro_net/pro_select_reactor.cpp  \
                        ../../../../src/pro/pro_net/pro_service_host.cpp    \
                        ../../../../src/pro/pro_net/pro_service_hub.cpp     \
                        ../../../../src/pro/pro_net/pro_service_pipe.cpp    \
                        ../../../../src/pro/pro_net/pro_ssl_handshaker.cpp  \
                        ../../../../src/pro/pro_net/pro_ssl_transport.cpp   \
                        ../../../../src/pro/pro_net/pro_tcp_handshaker.cpp  \
                        ../../../../src/pro/pro_net/pro_tcp_transport.cpp   \
                        ../../../../src/pro/pro_net/pro_tp_reactor_task.cpp \
                        ../../../../src/pro/pro_net/pro_udp_transport.cpp

libpro_net_so_CPPFLAGS = -DPRO_NET_EXPORTS \
                         -I../../../../src/mbedtls/include

libpro_net_so_CFLAGS   = -fno-strict-aliasing -fPIC -fvisibility=hidden
libpro_net_so_CXXFLAGS = -fno-strict-aliasing -fPIC -fvisibility=hidden

libpro_net_so_LDFLAGS = -shared -Wl,-soname,libpro_net.so -Wl,--no-undefined
libpro_net_so_LDADD   =

LIBS = ../pro_util/libpro_util.a      \
       ../mbedtls/libmbedtls.a        \
       ../pro_shared/libpro_shared.so \
       -lstdc++                       \
       -lrt                           \
       -lpthread                      \
       -lm                            \
       -lgcc                          \
       -lc
