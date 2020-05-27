LOCAL_PATH := $(MY_ROOT_DIR)/src/pronet/pro_shared
include $(CLEAR_VARS)

LOCAL_MODULE    := pro_shared
LOCAL_SRC_FILES := pro_shared.cpp

LOCAL_C_INCLUDES    :=
LOCAL_CFLAGS        := -DPRO_SHARED_EXPORTS \
                       -fno-strict-aliasing -fvisibility=hidden
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       :=
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       :=

include $(BUILD_SHARED_LIBRARY)
