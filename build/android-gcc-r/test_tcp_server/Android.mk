LOCAL_PATH := $(PRO_ROOT_DIR)/src/pronet/test_tcp_server
include $(CLEAR_VARS)

LOCAL_MODULE    := test_tcp_server
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
LOCAL_SHARED_LIBRARIES       := pro_net pro_shared

include $(BUILD_EXECUTABLE)
