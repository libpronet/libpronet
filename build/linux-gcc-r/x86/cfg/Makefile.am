procfgdir = ${prefix}/libpronet/bin

#############################################################################

procfg_DATA = ../../../../pub/cfg/ca.crt                 \
              ../../../../pub/cfg/server.crt             \
              ../../../../pub/cfg/server.key             \
              ../../../../pub/cfg/pro_service_hub.cfg    \
              ../../../../pub/cfg/rtp_msg_server.cfg     \
              ../../../../pub/cfg/rtp_msg_server.db      \
              ../../../../pub/cfg/rtp_msg_server.sql     \
              ../../../../pub/cfg/rtp_msg_c2s.cfg        \
              ../../../../pub/cfg/test_msg_client.cfg    \
              ../../../../pub/cfg/test_tcp_server.cfg    \
              ../../../../pub/cfg/test_tcp_client.cfg    \
              ../../../../pub/cfg/set1_sys.sh            \
              ../../../../pub/cfg/set2_proc.sh           \
              ../../../../pub/cfg/run-pro_service_hub.sh \
              ../../../../pub/cfg/screen-pro_service_hub.sh

install-data-hook:
	chmod +x ${procfgdir}/set1_sys.sh
	chmod +x ${procfgdir}/set2_proc.sh
	chmod +x ${procfgdir}/run-pro_service_hub.sh
	chmod +x ${procfgdir}/screen-pro_service_hub.sh
