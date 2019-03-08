#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT       -= core gui

TARGET = pro_util
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG(debug  , debug | release) : DEFINES += _DEBUG _WIN32_WINNT=0x0501
CONFIG(release, debug | release) : DEFINES += NDEBUG _WIN32_WINNT=0x0501

INCLUDEPATH += ../../../src/mbedtls/include

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../../src/pro/pro_util/pro_bsd_wrapper.cpp \
    ../../../src/pro/pro_util/pro_buffer.cpp \
    ../../../src/pro/pro_util/pro_config_file.cpp \
    ../../../src/pro/pro_util/pro_config_stream.cpp \
    ../../../src/pro/pro_util/pro_file_monitor.cpp \
    ../../../src/pro/pro_util/pro_functor_command_task.cpp \
    ../../../src/pro/pro_util/pro_log_file.cpp \
    ../../../src/pro/pro_util/pro_memory_pool.cpp \
    ../../../src/pro/pro_util/pro_ref_count.cpp \
    ../../../src/pro/pro_util/pro_reorder.cpp \
    ../../../src/pro/pro_util/pro_shaper.cpp \
    ../../../src/pro/pro_util/pro_ssl_util.cpp \
    ../../../src/pro/pro_util/pro_stat.cpp \
    ../../../src/pro/pro_util/pro_thread.cpp \
    ../../../src/pro/pro_util/pro_thread_mutex.cpp \
    ../../../src/pro/pro_util/pro_time_util.cpp \
    ../../../src/pro/pro_util/pro_timer_factory.cpp \
    ../../../src/pro/pro_util/pro_unicode.cpp \
    ../../../src/pro/pro_util/pro_z.cpp

HEADERS += \
    ../../../src/pro/pro_util/pro_a.h \
    ../../../src/pro/pro_util/pro_bsd_wrapper.h \
    ../../../src/pro/pro_util/pro_buffer.h \
    ../../../src/pro/pro_util/pro_config_file.h \
    ../../../src/pro/pro_util/pro_config_stream.h \
    ../../../src/pro/pro_util/pro_file_monitor.h \
    ../../../src/pro/pro_util/pro_functor_command.h \
    ../../../src/pro/pro_util/pro_functor_command_task.h \
    ../../../src/pro/pro_util/pro_log_file.h \
    ../../../src/pro/pro_util/pro_memory_pool.h \
    ../../../src/pro/pro_util/pro_ref_count.h \
    ../../../src/pro/pro_util/pro_reorder.h \
    ../../../src/pro/pro_util/pro_shaper.h \
    ../../../src/pro/pro_util/pro_ssl_util.h \
    ../../../src/pro/pro_util/pro_stat.h \
    ../../../src/pro/pro_util/pro_stl.h \
    ../../../src/pro/pro_util/pro_thread.h \
    ../../../src/pro/pro_util/pro_thread_mutex.h \
    ../../../src/pro/pro_util/pro_time_util.h \
    ../../../src/pro/pro_util/pro_timer_factory.h \
    ../../../src/pro/pro_util/pro_unicode.h \
    ../../../src/pro/pro_util/pro_version.h \
    ../../../src/pro/pro_util/pro_z.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
