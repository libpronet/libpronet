prolibdir = ${prefix}/libpronet/lib
propkgdir = ${prolibdir}/pkgconfig
proincdir = ${prefix}/libpronet/include/pronet

#############################################################################

prolib_LIBRARIES = libpro_util.a

propkg_DATA = pro_util.pc

proinc_HEADERS = ../../../../src/pronet/pro_util/pro_a.h                 \
                 ../../../../src/pronet/pro_util/pro_bsd_wrapper.h       \
                 ../../../../src/pronet/pro_util/pro_buffer.h            \
                 ../../../../src/pronet/pro_util/pro_channel_task_pool.h \
                 ../../../../src/pronet/pro_util/pro_command.h           \
                 ../../../../src/pronet/pro_util/pro_command_task.h      \
                 ../../../../src/pronet/pro_util/pro_config_file.h       \
                 ../../../../src/pronet/pro_util/pro_config_stream.h     \
                 ../../../../src/pronet/pro_util/pro_file_monitor.h      \
                 ../../../../src/pronet/pro_util/pro_guid.h              \
                 ../../../../src/pronet/pro_util/pro_log_file.h          \
                 ../../../../src/pronet/pro_util/pro_memory_pool.h       \
                 ../../../../src/pronet/pro_util/pro_notify_pipe.h       \
                 ../../../../src/pronet/pro_util/pro_ref_count.h         \
                 ../../../../src/pronet/pro_util/pro_shaper.h            \
                 ../../../../src/pronet/pro_util/pro_ssl_util.h          \
                 ../../../../src/pronet/pro_util/pro_stat.h              \
                 ../../../../src/pronet/pro_util/pro_stl.h               \
                 ../../../../src/pronet/pro_util/pro_thread.h            \
                 ../../../../src/pronet/pro_util/pro_thread_mutex.h      \
                 ../../../../src/pronet/pro_util/pro_time_util.h         \
                 ../../../../src/pronet/pro_util/pro_timer_factory.h     \
                 ../../../../src/pronet/pro_util/pro_unicode.h           \
                 ../../../../src/pronet/pro_util/pro_version.h           \
                 ../../../../src/pronet/pro_util/pro_z.h

libpro_util_a_SOURCES = ../../../../src/pronet/pro_util/pro_bsd_wrapper.cpp       \
                        ../../../../src/pronet/pro_util/pro_buffer.cpp            \
                        ../../../../src/pronet/pro_util/pro_channel_task_pool.cpp \
                        ../../../../src/pronet/pro_util/pro_command_task.cpp      \
                        ../../../../src/pronet/pro_util/pro_config_file.cpp       \
                        ../../../../src/pronet/pro_util/pro_config_stream.cpp     \
                        ../../../../src/pronet/pro_util/pro_file_monitor.cpp      \
                        ../../../../src/pronet/pro_util/pro_guid.cpp              \
                        ../../../../src/pronet/pro_util/pro_log_file.cpp          \
                        ../../../../src/pronet/pro_util/pro_memory_pool.cpp       \
                        ../../../../src/pronet/pro_util/pro_notify_pipe.cpp       \
                        ../../../../src/pronet/pro_util/pro_shaper.cpp            \
                        ../../../../src/pronet/pro_util/pro_ssl_util.cpp          \
                        ../../../../src/pronet/pro_util/pro_stat.cpp              \
                        ../../../../src/pronet/pro_util/pro_thread.cpp            \
                        ../../../../src/pronet/pro_util/pro_thread_mutex.cpp      \
                        ../../../../src/pronet/pro_util/pro_time_util.cpp         \
                        ../../../../src/pronet/pro_util/pro_timer_factory.cpp     \
                        ../../../../src/pronet/pro_util/pro_unicode.cpp           \
                        ../../../../src/pronet/pro_util/pro_z.cpp

libpro_util_a_CPPFLAGS = -I../../../../src/mbedtls/include   \
                         -I../../../../src/pronet/pro_shared \
                         -I../../../../src/pronet/pro_util

libpro_util_a_CFLAGS   = -fPIC
libpro_util_a_CXXFLAGS = -fPIC
