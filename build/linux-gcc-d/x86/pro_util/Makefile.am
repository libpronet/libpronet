probindir = ${prefix}/libpronet/bin
prolibdir = ${prefix}/libpronet/lib
proincdir = ${prefix}/libpronet/include/pro

#############################################################################

prolib_LIBRARIES = libpro_util.a

proinc_HEADERS = ../../../../src/pro/pro_util/pro_a.h                    \
                 ../../../../src/pro/pro_util/pro_bsd_wrapper.h          \
                 ../../../../src/pro/pro_util/pro_buffer.h               \
                 ../../../../src/pro/pro_util/pro_config_file.h          \
                 ../../../../src/pro/pro_util/pro_config_stream.h        \
                 ../../../../src/pro/pro_util/pro_file_monitor.h         \
                 ../../../../src/pro/pro_util/pro_functor_command.h      \
                 ../../../../src/pro/pro_util/pro_functor_command_task.h \
                 ../../../../src/pro/pro_util/pro_log_file.h             \
                 ../../../../src/pro/pro_util/pro_memory_pool.h          \
                 ../../../../src/pro/pro_util/pro_ref_count.h            \
                 ../../../../src/pro/pro_util/pro_reorder.h              \
                 ../../../../src/pro/pro_util/pro_shaper.h               \
                 ../../../../src/pro/pro_util/pro_ssl_util.h             \
                 ../../../../src/pro/pro_util/pro_stat.h                 \
                 ../../../../src/pro/pro_util/pro_stl.h                  \
                 ../../../../src/pro/pro_util/pro_thread.h               \
                 ../../../../src/pro/pro_util/pro_thread_mutex.h         \
                 ../../../../src/pro/pro_util/pro_time_util.h            \
                 ../../../../src/pro/pro_util/pro_timer_factory.h        \
                 ../../../../src/pro/pro_util/pro_unicode.h              \
                 ../../../../src/pro/pro_util/pro_version.h              \
                 ../../../../src/pro/pro_util/pro_z.h

libpro_util_a_SOURCES = ../../../../src/pro/pro_util/pro_bsd_wrapper.cpp          \
                        ../../../../src/pro/pro_util/pro_buffer.cpp               \
                        ../../../../src/pro/pro_util/pro_config_file.cpp          \
                        ../../../../src/pro/pro_util/pro_config_stream.cpp        \
                        ../../../../src/pro/pro_util/pro_file_monitor.cpp         \
                        ../../../../src/pro/pro_util/pro_functor_command_task.cpp \
                        ../../../../src/pro/pro_util/pro_log_file.cpp             \
                        ../../../../src/pro/pro_util/pro_memory_pool.cpp          \
                        ../../../../src/pro/pro_util/pro_ref_count.cpp            \
                        ../../../../src/pro/pro_util/pro_reorder.cpp              \
                        ../../../../src/pro/pro_util/pro_shaper.cpp               \
                        ../../../../src/pro/pro_util/pro_ssl_util.cpp             \
                        ../../../../src/pro/pro_util/pro_stat.cpp                 \
                        ../../../../src/pro/pro_util/pro_thread.cpp               \
                        ../../../../src/pro/pro_util/pro_thread_mutex.cpp         \
                        ../../../../src/pro/pro_util/pro_time_util.cpp            \
                        ../../../../src/pro/pro_util/pro_timer_factory.cpp        \
                        ../../../../src/pro/pro_util/pro_unicode.cpp              \
                        ../../../../src/pro/pro_util/pro_z.cpp

libpro_util_a_CPPFLAGS = -I../../../../src/mbedtls/include

libpro_util_a_CFLAGS   = -fno-strict-aliasing -fPIC -fvisibility=hidden
libpro_util_a_CXXFLAGS = -fno-strict-aliasing -fPIC -fvisibility=hidden
