procfgdir = ${prefix}/libpronet/bin

#############################################################################

procfg_DATA = ../../../../pub/cfg/ca.crt              \
              ../../../../pub/cfg/server.crt          \
              ../../../../pub/cfg/server.key          \
              ../../../../pub/cfg/pro_service_hub.cfg \
              ../../../../pub/cfg/rtp_msg_server.cfg  \
              ../../../../pub/cfg/rtp_msg_server.db   \
              ../../../../pub/cfg/rtp_msg_server.sql  \
              ../../../../pub/cfg/rtp_msg_c2s.cfg     \
              ../../../../pub/cfg/test_msg_client.cfg \
              ../../../../pub/cfg/test_tcp_server.cfg \
              ../../../../pub/cfg/test_tcp_client.cfg \
              ../../../../pub/cfg/set1_sys.sh         \
              ../../../../pub/cfg/set2_proc.sh        \
              ../../../../pub/cfg/pro_service_hub.service

install-data-hook:
	chmod +x  ${procfgdir}/set1_sys.sh
	chmod +x  ${procfgdir}/set2_proc.sh
	chmod 644 ${procfgdir}/pro_service_hub.service
