probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib

#############################################################################

probin_PROGRAMS = pro_service_hub

pro_service_hub_SOURCES = ../../../../src/pronet/pro_service_hub/main.cpp

pro_service_hub_CPPFLAGS = -I../../../../src/pronet/pro_util

pro_service_hub_CFLAGS   =
pro_service_hub_CXXFLAGS =

pro_service_hub_LDFLAGS = -Wl,-rpath,.:../lib:${prolibdir}
pro_service_hub_LDADD   =

LIBS = ../pro_net/libpro_net.so       \
       ../pro_util/libpro_util.a      \
       ../pro_shared/libpro_shared.so \
       ../mbedtls/libmbedtls.a        \
       -lpthread                      \
       -lc
