LOCAL_PATH := $(MY_ROOT_DIR)/src/pronet/pro_net
include $(CLEAR_VARS)

LOCAL_MODULE    := pro_net
LOCAL_SRC_FILES := pro_acceptor.cpp        \
                   pro_base_reactor.cpp    \
                   pro_connector.cpp       \
                   pro_epoll_reactor.cpp   \
                   pro_handler_mgr.cpp     \
                   pro_mcast_transport.cpp \
                   pro_net.cpp             \
                   pro_select_reactor.cpp  \
                   pro_service_host.cpp    \
                   pro_service_hub.cpp     \
                   pro_service_pipe.cpp    \
                   pro_ssl.cpp             \
                   pro_ssl_handshaker.cpp  \
                   pro_ssl_transport.cpp   \
                   pro_tcp_handshaker.cpp  \
                   pro_tcp_transport.cpp   \
                   pro_tp_reactor_task.cpp \
                   pro_udp_transport.cpp

LOCAL_C_INCLUDES    := $(MY_ROOT_DIR)/src/mbedtls/include \
                       $(MY_ROOT_DIR)/src/pronet/pro_util
LOCAL_CFLAGS        := -DPRO_NET_EXPORTS
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_shared

include $(BUILD_SHARED_LIBRARY)
