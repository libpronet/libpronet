LOCAL_PATH := $(PRO_ROOT_DIR)/src/pro/rtp_msg_server
include $(CLEAR_VARS)

LOCAL_MODULE    := rtp_msg_server
LOCAL_SRC_FILES := main.cpp          \
                   msg_db.cpp        \
                   msg_server.cpp    \
                   db_connection.cpp \
                   sqlite3.c

LOCAL_C_INCLUDES    := $(PRO_ROOT_DIR)/src/pro/pro_net
LOCAL_CFLAGS        := -DSQLITE_THREADSAFE=1 -fno-strict-aliasing
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_rtp pro_net pro_shared

include $(BUILD_EXECUTABLE)
