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

INCLUDEPATH += \
    ../../../src/mbedtls/include \
    ../../../src/pronet/pro_shared \
    ../../../src/pronet/pro_util

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../../src/pronet/pro_util/pro_bsd_wrapper.cpp \
    ../../../src/pronet/pro_util/pro_buffer.cpp \
    ../../../src/pronet/pro_util/pro_channel_task_pool.cpp \
    ../../../src/pronet/pro_util/pro_config_file.cpp \
    ../../../src/pronet/pro_util/pro_config_stream.cpp \
    ../../../src/pronet/pro_util/pro_file_monitor.cpp \
    ../../../src/pronet/pro_util/pro_functor_command_task.cpp \
    ../../../src/pronet/pro_util/pro_guid.cpp \
    ../../../src/pronet/pro_util/pro_log_file.cpp \
    ../../../src/pronet/pro_util/pro_memory_pool.cpp \
    ../../../src/pronet/pro_util/pro_notify_pipe.cpp \
    ../../../src/pronet/pro_util/pro_shaper.cpp \
    ../../../src/pronet/pro_util/pro_ssl_util.cpp \
    ../../../src/pronet/pro_util/pro_stat.cpp \
    ../../../src/pronet/pro_util/pro_thread.cpp \
    ../../../src/pronet/pro_util/pro_thread_mutex.cpp \
    ../../../src/pronet/pro_util/pro_time_util.cpp \
    ../../../src/pronet/pro_util/pro_timer_factory.cpp \
    ../../../src/pronet/pro_util/pro_unicode.cpp \
    ../../../src/pronet/pro_util/pro_z.cpp

HEADERS += \
    ../../../src/pronet/pro_util/pro_a.h \
    ../../../src/pronet/pro_util/pro_bsd_wrapper.h \
    ../../../src/pronet/pro_util/pro_buffer.h \
    ../../../src/pronet/pro_util/pro_channel_task_pool.h \
    ../../../src/pronet/pro_util/pro_config_file.h \
    ../../../src/pronet/pro_util/pro_config_stream.h \
    ../../../src/pronet/pro_util/pro_file_monitor.h \
    ../../../src/pronet/pro_util/pro_functor_command.h \
    ../../../src/pronet/pro_util/pro_functor_command_task.h \
    ../../../src/pronet/pro_util/pro_guid.h \
    ../../../src/pronet/pro_util/pro_log_file.h \
    ../../../src/pronet/pro_util/pro_memory_pool.h \
    ../../../src/pronet/pro_util/pro_notify_pipe.h \
    ../../../src/pronet/pro_util/pro_ref_count.h \
    ../../../src/pronet/pro_util/pro_shaper.h \
    ../../../src/pronet/pro_util/pro_ssl_util.h \
    ../../../src/pronet/pro_util/pro_stat.h \
    ../../../src/pronet/pro_util/pro_stl.h \
    ../../../src/pronet/pro_util/pro_thread.h \
    ../../../src/pronet/pro_util/pro_thread_mutex.h \
    ../../../src/pronet/pro_util/pro_time_util.h \
    ../../../src/pronet/pro_util/pro_timer_factory.h \
    ../../../src/pronet/pro_util/pro_unicode.h \
    ../../../src/pronet/pro_util/pro_version.h \
    ../../../src/pronet/pro_util/pro_z.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
