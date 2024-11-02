prolibdir = ${prefix}/libpronet/lib
propkgdir = ${prolibdir}/pkgconfig
proincdir = ${prefix}/libpronet/include/pronet

#############################################################################

prolib_PROGRAMS = libpro_net.so

propkg_DATA = pro_net.pc

proinc_HEADERS = ../../../../src/pronet/pro_net/pro_net.h \
                 ../../../../src/pronet/pro_net/pro_ssl.h

libpro_net_so_SOURCES = ../../../../src/pronet/pro_net/pro_acceptor.cpp        \
                        ../../../../src/pronet/pro_net/pro_base_reactor.cpp    \
                        ../../../../src/pronet/pro_net/pro_connector.cpp       \
                        ../../../../src/pronet/pro_net/pro_epoll_reactor.cpp   \
                        ../../../../src/pronet/pro_net/pro_handler_mgr.cpp     \
                        ../../../../src/pronet/pro_net/pro_mcast_transport.cpp \
                        ../../../../src/pronet/pro_net/pro_net.cpp             \
                        ../../../../src/pronet/pro_net/pro_select_reactor.cpp  \
                        ../../../../src/pronet/pro_net/pro_service_host.cpp    \
                        ../../../../src/pronet/pro_net/pro_service_hub.cpp     \
                        ../../../../src/pronet/pro_net/pro_service_pipe.cpp    \
                        ../../../../src/pronet/pro_net/pro_ssl.cpp             \
                        ../../../../src/pronet/pro_net/pro_ssl_handshaker.cpp  \
                        ../../../../src/pronet/pro_net/pro_ssl_transport.cpp   \
                        ../../../../src/pronet/pro_net/pro_tcp_handshaker.cpp  \
                        ../../../../src/pronet/pro_net/pro_tcp_transport.cpp   \
                        ../../../../src/pronet/pro_net/pro_tp_reactor_task.cpp \
                        ../../../../src/pronet/pro_net/pro_udp_transport.cpp

libpro_net_so_CPPFLAGS = -DPRO_NET_EXPORTS                 \
                         -I../../../../src/mbedtls/include \
                         -I../../../../src/pronet/pro_util

libpro_net_so_CFLAGS   = -fPIC
libpro_net_so_CXXFLAGS = -fPIC

libpro_net_so_LDFLAGS = -shared -Wl,-rpath,.:${prolibdir}
libpro_net_so_LDADD   =

LIBS = ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lpthread                      \
       -lc
