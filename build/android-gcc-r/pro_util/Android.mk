LOCAL_PATH := $(PRO_ROOT_DIR)/src/pronet/pro_util
include $(CLEAR_VARS)

LOCAL_MODULE    := pro_util
LOCAL_SRC_FILES := pro_bsd_wrapper.cpp          \
                   pro_buffer.cpp               \
                   pro_config_file.cpp          \
                   pro_config_stream.cpp        \
                   pro_file_monitor.cpp         \
                   pro_functor_command_task.cpp \
                   pro_log_file.cpp             \
                   pro_memory_pool.cpp          \
                   pro_ref_count.cpp            \
                   pro_shaper.cpp               \
                   pro_ssl_util.cpp             \
                   pro_stat.cpp                 \
                   pro_thread.cpp               \
                   pro_thread_mutex.cpp         \
                   pro_time_util.cpp            \
                   pro_timer_factory.cpp        \
                   pro_unicode.cpp              \
                   pro_z.cpp

LOCAL_C_INCLUDES    := $(PRO_ROOT_DIR)/src/mbedtls/include
LOCAL_CFLAGS        := -fno-strict-aliasing -fvisibility=hidden
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       :=
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       :=

include $(BUILD_STATIC_LIBRARY)
