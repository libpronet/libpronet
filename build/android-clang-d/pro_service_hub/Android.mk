LOCAL_PATH := $(MY_ROOT_DIR)/src/pronet/pro_service_hub
include $(CLEAR_VARS)

LOCAL_MODULE    := pro_service_hub
LOCAL_SRC_FILES := main.cpp

LOCAL_C_INCLUDES    := $(MY_ROOT_DIR)/src/pronet/pro_util
LOCAL_CFLAGS        :=
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_net pro_shared

include $(BUILD_EXECUTABLE)
