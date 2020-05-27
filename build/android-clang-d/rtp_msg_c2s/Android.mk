LOCAL_PATH := $(MY_ROOT_DIR)/src/pronet/rtp_msg_c2s
include $(CLEAR_VARS)

LOCAL_MODULE    := rtp_msg_c2s
LOCAL_SRC_FILES := c2s_server.cpp \
                   main.cpp

LOCAL_C_INCLUDES    := $(MY_ROOT_DIR)/src/pronet/pro_net
LOCAL_CFLAGS        := -fno-strict-aliasing
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_rtp pro_net pro_shared

include $(BUILD_EXECUTABLE)
