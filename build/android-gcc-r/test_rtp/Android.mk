LOCAL_PATH := $(PRO_ROOT_DIR)/src/pro/test_rtp
include $(CLEAR_VARS)

LOCAL_MODULE    := test_rtp
LOCAL_SRC_FILES := main.cpp \
                   test.cpp

LOCAL_C_INCLUDES    :=
LOCAL_CFLAGS        := -fno-strict-aliasing
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_rtp pro_net pro_shared

include $(BUILD_EXECUTABLE)
