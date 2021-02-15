prolibdir = ${prefix}/libpronet/lib
proincdir = ${prefix}/libpronet/include/pronet

#############################################################################

prolib_PROGRAMS = libpro_shared.so

proinc_HEADERS = ../../../../src/pronet/pro_shared/pro_shared.h

libpro_shared_so_SOURCES = ../../../../src/pronet/pro_shared/pro_shared.cpp

libpro_shared_so_CPPFLAGS = -DPRO_SHARED_EXPORTS \
                            -I../../../../src/pronet/pro_util

libpro_shared_so_CFLAGS   = -fPIC
libpro_shared_so_CXXFLAGS = -fPIC

libpro_shared_so_LDFLAGS = -shared -Wl,-soname,libpro_shared.so -Wl,--no-undefined
libpro_shared_so_LDADD   =

LIBS = -lstdc++  \
       -lrt      \
       -lpthread \
       -lm       \
       -lgcc     \
       -lc
