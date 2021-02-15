LOCAL_PATH := $(MY_ROOT_DIR)/src/pronet/test_msg_client
include $(CLEAR_VARS)

LOCAL_MODULE    := test_msg_client
LOCAL_SRC_FILES := main.cpp \
                   test.cpp

LOCAL_C_INCLUDES    := $(MY_ROOT_DIR)/src/pronet/pro_util \
                       $(MY_ROOT_DIR)/src/pronet/pro_net
LOCAL_CFLAGS        :=
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_rtp pro_net pro_shared

include $(BUILD_EXECUTABLE)
