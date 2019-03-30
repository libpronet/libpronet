LOCAL_PATH := $(PRO_ROOT_DIR)/src/pro/pro_rtp
include $(CLEAR_VARS)

LOCAL_MODULE    := pro_rtp
LOCAL_SRC_FILES := rtp_framework.cpp            \
                   rtp_packet.cpp               \
                   rtp_port_allocator.cpp       \
                   rtp_service.cpp              \
                   rtp_session_base.cpp         \
                   rtp_session_mcast.cpp        \
                   rtp_session_mcast_ex.cpp     \
                   rtp_session_sslclient_ex.cpp \
                   rtp_session_sslserver_ex.cpp \
                   rtp_session_tcpclient.cpp    \
                   rtp_session_tcpclient_ex.cpp \
                   rtp_session_tcpserver.cpp    \
                   rtp_session_tcpserver_ex.cpp \
                   rtp_session_udpclient.cpp    \
                   rtp_session_udpclient_ex.cpp \
                   rtp_session_udpserver.cpp    \
                   rtp_session_udpserver_ex.cpp \
                   rtp_bucket.cpp               \
                   rtp_flow_stat.cpp            \
                   rtp_foundation.cpp           \
                   rtp_msg_c2s.cpp              \
                   rtp_msg_client.cpp           \
                   rtp_msg_server.cpp           \
                   rtp_session_wrapper.cpp

LOCAL_C_INCLUDES    := $(PRO_ROOT_DIR)/src/pro/pro_net
LOCAL_CFLAGS        := -DPRO_RTP_EXPORTS \
                       -fno-strict-aliasing -fvisibility=hidden
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       := pro_util mbedtls
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       := pro_net pro_shared

include $(BUILD_SHARED_LIBRARY)
