/*
 * Copyright (C) 2018-2019 Eric Tung <libpronet@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of LibProNet (https://github.com/libpronet/libpronet)
 */

#if !defined(RTP_SESSION_A_H)
#define RTP_SESSION_A_H

#include "rtp_base.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ����: ����һ��RTP_ST_UDPCLIENT���͵ĻỰ
 *
 * ����:
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localInfo : �Ự��Ϣ
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ����ʹ��IRtpSession::GetLocalPort(...)��ȡ���ض˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * ����: ����һ��RTP_ST_UDPSERVER���͵ĻỰ
 *
 * ����:
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localInfo : �Ự��Ϣ
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ����ʹ��IRtpSession::GetLocalPort(...)��ȡ���ض˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp   = NULL,
                          unsigned short          localPort = 0);

/*
 * ����: ����һ��RTP_ST_TCPCLIENT���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ��
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp          = NULL,
                          unsigned long           timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_TCPSERVER���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort        : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ����ʹ��IRtpSession::GetLocalPort(...)��ȡ���ض˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp          = NULL,
                          unsigned short          localPort        = 0,
                          unsigned long           timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_UDPCLIENT_EX���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ��
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp          = NULL,
                            unsigned long           timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_UDPSERVER_EX���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort        : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ����ʹ��IRtpSession::GetLocalPort(...)��ȡ���ض˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp          = NULL,
                            unsigned short          localPort        = 0,
                            unsigned long           timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_TCPCLIENT_EX���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * password         : �Ự����
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ��
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password         = NULL,
                            const char*             localIp          = NULL,
                            unsigned long           timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_TCPSERVER_EX���͵ĻỰ
 *
 * ����:
 * observer   : �ص�Ŀ��
 * reactor    : ��Ӧ��
 * localInfo  : �Ự��Ϣ. ����IRtpServiceObserver::OnAcceptSession(...)��remoteInfo����
 * sockId     : �׽���id. ��Դ��IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket : �Ƿ�unix�׽���
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ��
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_INT64               sockId,
                            bool                    unixSocket);

/*
 * ����: ����һ��RTP_ST_SSLCLIENT_EX���͵ĻỰ
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localInfo        : �Ự��Ϣ
 * sslConfig        : ssl����
 * sslSni           : ssl������. �����Ч, �������֤�����֤��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * password         : �Ự����
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: sslConfigָ���Ķ�������ڻỰ������������һֱ��Ч
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslclientEx(IRtpSessionObserver*         observer,
                            IProReactor*                 reactor,
                            const RTP_SESSION_INFO*      localInfo,
                            const PRO_SSL_CLIENT_CONFIG* sslConfig,
                            const char*                  sslSni, /* = NULL */
                            const char*                  remoteIp,
                            unsigned short               remotePort,
                            const char*                  password         = NULL,
                            const char*                  localIp          = NULL,
                            unsigned long                timeoutInSeconds = 0);

/*
 * ����: ����һ��RTP_ST_SSLSERVER_EX���͵ĻỰ
 *
 * ����:
 * observer   : �ص�Ŀ��
 * reactor    : ��Ӧ��
 * localInfo  : �Ự��Ϣ. ����IRtpServiceObserver::OnAcceptSession(...)��remoteInfo����
 * sslCtx     : ssl������
 * sockId     : �׽���id. ��Դ��IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket : �Ƿ�unix�׽���
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: ��������ɹ�, �Ự����Ϊ(sslCtx, sockId)������; ����, ������Ӧ��
 *       �ͷ�(sslCtx, sockId)��Ӧ����Դ
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            PRO_INT64               sockId,
                            bool                    unixSocket);

/*
 * ����: ����һ��RTP_ST_MCAST���͵ĻỰ
 *
 * ����:
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localInfo : �Ự��Ϣ
 * mcastIp   : Ҫ�󶨵Ķಥ��ַ
 * mcastPort : Ҫ�󶨵Ķಥ�˿ں�. ���Ϊ0, ϵͳ���������һ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: �Ϸ��Ķಥ��ַΪ[224.0.0.0 ~ 239.255.255.255],
 *       �Ƽ��Ķಥ��ַΪ[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       ����ʹ��IRtpSession::GetLocalPort(...)��ȡ�ಥ�˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort = 0,
                      const char*             localIp   = NULL);

/*
 * ����: ����һ��RTP_ST_MCAST_EX���͵ĻỰ
 *
 * ����:
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localInfo : �Ự��Ϣ
 * mcastIp   : Ҫ�󶨵Ķಥ��ַ
 * mcastPort : Ҫ�󶨵Ķಥ�˿ں�. ���Ϊ0, ϵͳ���������һ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 *
 * ����ֵ: �Ự�����NULL
 *
 * ˵��: �Ϸ��Ķಥ��ַΪ[224.0.0.0 ~ 239.255.255.255],
 *       �Ƽ��Ķಥ��ַΪ[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       ����ʹ��IRtpSession::GetLocalPort(...)��ȡ�ಥ�˿ں�
 */
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort = 0,
                        const char*             localIp   = NULL);

/*
 * ����: ɾ��һ���Ự
 *
 * ����:
 * session : �Ự����
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
void
PRO_CALLTYPE
DeleteRtpSession(IRtpSession* session);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_A_H */
