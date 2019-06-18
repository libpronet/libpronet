/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (http://www.libpro.org)
 */

/*         ______________________________________________________
 *        |                                                      |
 *        |                        ProNet                        |
 *        |______________________________________________________|
 *        |                    |                                 |
 *        |                    |             ProUtil             |
 *        |                    |_________________________________|
 *        |      MbedTLS                 |                       |
 *        |                              |       ProShared       |
 *        |______________________________|_______________________|
 *                     Fig.1 module hierarchy diagram
 */

/*         ________________   ________________   ________________
 *        |    Acceptor    | |   Connector    | |   Handshaker   |
 *        | (EventHandler) | | (EventHandler) | | (EventHandler) |
 *        |________________| |________________| |________________|
 *                 A                  A                  A
 *                 |       ...        |       ...        |
 *         ________|__________________|__________________|_______
 *        |                       Reactor                        |
 *        |   ____________     _____________     _____________   |
 *        |  A            |   A             |   A             |  |
 *        |  |     Acc    |   |     I/O     |   |   General   |  |
 *        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
 *        |  |____________V   |_____________V   |_____________V  |
 *        |                                                      |
 *        |   ____________     _____________     _____________   |
 *        |  A            |   A             |   A             |  |
 *        |  |    I/O     |   |     I/O     |   |     MM      |  |
 *        |  |   NetTask  |   |   NetTask   |   |  TimerTask  |  |
 *        |  |____________V   |_____________V   |_____________V  |
 *        |                                                      |
 *        |       ...               ...               ...        |
 *        |______________________________________________________|
 *                 |                  |                  |
 *                 |       ...        |       ...        |
 *         ________V_______   ________V_______   ________V_______
 *        |   Transport    | |   ServiceHub   | |  ServiceHost   |
 *        | (EventHandler) | | (EventHandler) | | (EventHandler) |
 *        |________________| |________________| |________________|
 *                   Fig.2 structure diagram of Reactor
 */

/*       __________________                     _________________
 *      |    ServiceHub    |<----------------->|   ServiceHost   |
 *      |   ____________   |    ServicePipe    |(Acceptor Shadow)| Audio-Process
 *      |  |            |  |                   |_________________|
 *      |  |  Acceptor  |  |        ...         _________________
 *      |  |____________|  |                   |   ServiceHost   |
 *      |                  |    ServicePipe    |(Acceptor Shadow)| Video-Process
 *      |__________________|<----------------->|_________________|
 *           Hub-Process
 *                Fig.3 structure diagram of ServiceHub
 */

/*
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 *       Fig.4 acceptor_ex/connector_ex handshake protocol flow chart
 */

#if !defined(____PRO_NET_H____)
#define ____PRO_NET_H____

#include <cstddef>

#if !defined(____PRO_A_H____)
#define ____PRO_A_H____
#define PRO_INT16_VCC    short
#define PRO_INT32_VCC    int
#if defined(_MSC_VER)
#define PRO_INT64_VCC    __int64
#define PRO_PRT64D_VCC   "%I64d"
#else
#define PRO_INT64_VCC    long long
#define PRO_PRT64D_VCC   "%lld"
#endif
#define PRO_UINT16_VCC   unsigned short
#define PRO_UINT32_VCC   unsigned int
#if defined(_MSC_VER)
#define PRO_UINT64_VCC   unsigned __int64
#define PRO_PRT64U_VCC   "%I64u"
#else
#define PRO_UINT64_VCC   unsigned long long
#define PRO_PRT64U_VCC   "%llu"
#endif
#define PRO_CALLTYPE_VCC __stdcall
#define PRO_EXPORT_VCC   __declspec(dllexport)
#define PRO_IMPORT_VCC
#define PRO_INT16_GCC    short
#define PRO_INT32_GCC    int
#define PRO_INT64_GCC    long long
#define PRO_PRT64D_GCC   "%lld"
#define PRO_UINT16_GCC   unsigned short
#define PRO_UINT32_GCC   unsigned int
#define PRO_UINT64_GCC   unsigned long long
#define PRO_PRT64U_GCC   "%llu"
#define PRO_CALLTYPE_GCC
#define PRO_EXPORT_GCC   __attribute__((visibility("default")))
#define PRO_IMPORT_GCC
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_VCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_VCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_VCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_VCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_VCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_VCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_VCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_VCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_VCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_VCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_VCC
#endif
#else  /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_GCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_GCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_GCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_GCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_GCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_GCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_GCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_GCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_GCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_GCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_GCC
#endif
#endif /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#endif /* ____PRO_A_H____ */

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_NET_LIB)
#define PRO_NET_API
#elif defined(PRO_NET_EXPORTS)
#if defined(_MSC_VER)
#define PRO_NET_API /* .def */
#else
#define PRO_NET_API PRO_EXPORT
#endif
#else
#define PRO_NET_API PRO_IMPORT
#endif

#include "pro_mbedtls.h"

class  IProAcceptor;      /* ������ */
class  IProConnector;     /* ������ */
class  IProServiceHost;   /* ����host */
class  IProServiceHub;    /* ����hub */
class  IProSslHandshaker; /* ssl������ */
class  IProTcpHandshaker; /* tcp������ */
struct pbsd_sockaddr_in;  /* �׽��ֵ�ַ */

/*
 * [[[[ ����������
 */
typedef unsigned char PRO_TRANS_TYPE;

static const PRO_TRANS_TYPE PRO_TRANS_TCP   =  1;
static const PRO_TRANS_TYPE PRO_TRANS_UDP   =  2;
static const PRO_TRANS_TYPE PRO_TRANS_MCAST =  3;
static const PRO_TRANS_TYPE PRO_TRANS_SSL   = 11;
/*
 * ]]]]
 */

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ��ʱ���ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 *
 * please refer to "pro_util/pro_timer_factory.h"
 */
#if !defined(____IProOnTimer____)
#define ____IProOnTimer____
class IProOnTimer
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE OnTimer(
        unsigned long timerId,
        PRO_INT64     userData
        ) = 0;
};
#endif /* ____IProOnTimer____ */

/*
 * ��Ӧ��
 *
 * ����ֻ��¶�˶�ʱ����صĽӿ�,Ŀ����Ϊ����ʹ�÷�Ӧ����ʱ��,���Է����
 * ʹ�����ڲ��Ķ�ʱ������,��û�Ҫ���������CProTimerFactory����
 */
class IProReactor
{
public:

    /*
     * ����һ����ͨ��ʱ��
     *
     * ����ֵΪ��ʱ��id. 0��Ч
     */
    virtual unsigned long PRO_CALLTYPE ScheduleTimer(
        IProOnTimer* onTimer,
        PRO_UINT64   timeSpan,  /* ��ʱ����(ms) */
        bool         recurring, /* �Ƿ��ظ�. ���timeSpanΪ0, recurring����Ϊfalse */
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * ����һ��������·��������ͨ��ʱ��(������ʱ��)
     *
     * ����������ʱ��������ʱ������ڲ��㷨����,�ڲ�����о��Ȼ�����.
     * �����¼�����ʱ,�ϲ������OnTimer(...)�ص��﷢���������ݰ�
     *
     * ����ֵΪ��ʱ��id. 0��Ч
     */
    virtual unsigned long PRO_CALLTYPE ScheduleHeartbeatTimer(
        IProOnTimer* onTimer,
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * ����ȫ��������ʱ������������
     *
     * Ĭ�ϵ���������Ϊ25��
     */
    virtual bool PRO_CALLTYPE UpdateHeartbeatTimers(unsigned long htbtIntervalInSeconds) = 0;

    /*
     * ɾ��һ����ͨ��ʱ��
     */
    virtual void PRO_CALLTYPE CancelTimer(unsigned long timerId) = 0;

    /*
     * ����һ����ý�嶨ʱ��(�߾��ȶ�ʱ��)
     *
     * ����ֵΪ��ʱ��id. 0��Ч
     */
    virtual unsigned long PRO_CALLTYPE ScheduleMmTimer(
        IProOnTimer* onTimer,
        PRO_UINT64   timeSpan,  /* ��ʱ����(ms) */
        bool         recurring, /* �Ƿ��ظ�. ���timeSpanΪ0, recurring����Ϊfalse */
        PRO_INT64    userData = 0
        ) = 0;

    /*
     * ɾ��һ����ý�嶨ʱ��(�߾��ȶ�ʱ��)
     */
    virtual void PRO_CALLTYPE CancelMmTimer(unsigned long timerId) = 0;

    /*
     * ��ȡ״̬��Ϣ�ַ���
     */
    virtual void PRO_CALLTYPE GetTraceInfo(char* buf, size_t size) const = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * �������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProAcceptorObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * �����ӽ���ʱ,�ú��������ص�
     *
     * ʹ���߸���sockId����Դά��
     */
    virtual void PRO_CALLTYPE OnAccept(
        IProAcceptor*  acceptor,
        PRO_INT64      sockId,     /* �׽���id */
        bool           unixSocket, /* �Ƿ�unix�׽��� */
        const char*    remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        unsigned char  serviceId,  /* ���������չʱ,Զ������ķ���id */
        unsigned char  serviceOpt, /* ���������չʱ,Զ������ķ���ѡ�� */
        PRO_UINT64     nonce       /* ���������չʱ,�Ự����� */
        ) = 0;
};

/*
 * ����host�ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProServiceHostObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * �����ӽ���ʱ,�ú��������ص�
     *
     * ʹ���߸���sockId����Դά��
     */
    virtual void PRO_CALLTYPE OnServiceAccept(
        IProServiceHost* serviceHost,
        PRO_INT64        sockId,     /* �׽���id */
        bool             unixSocket, /* �Ƿ�unix�׽��� */
        const char*      remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short   remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        unsigned char    serviceId,  /* Զ������ķ���id */
        unsigned char    serviceOpt, /* Զ������ķ���ѡ�� */
        PRO_UINT64       nonce       /* �Ự����� */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * �������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProConnectorObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ���ӳɹ�ʱ,�ú��������ص�
     *
     * ʹ���߸���sockId����Դά��
     */
    virtual void PRO_CALLTYPE OnConnectOk(
        IProConnector* connector,
        PRO_INT64      sockId,     /* �׽���id */
        bool           unixSocket, /* �Ƿ�unix�׽��� */
        const char*    remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        unsigned char  serviceId,  /* ���������չʱ,����ķ���id */
        unsigned char  serviceOpt, /* ���������չʱ,����ķ���ѡ�� */
        PRO_UINT64     nonce       /* ���������չʱ,�Ự����� */
        ) = 0;

    /*
     * ���ӳ����ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        unsigned char  serviceId,  /* ���������չʱ,����ķ���id */
        unsigned char  serviceOpt, /* ���������չʱ,����ķ���ѡ�� */
        bool           timeout     /* �Ƿ����ӳ�ʱ */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * tcp�������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProTcpHandshakerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ���ֳɹ�ʱ,�ú��������ص�
     *
     * ʹ���߸���sockId����Դά��.
     * ������ɺ�,�ϲ�Ӧ�ð�sockId��װ��IProTransport,
     * ���ͷ�sockId��Ӧ����Դ
     */
    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        PRO_INT64          sockId,     /* �׽���id */
        bool               unixSocket, /* �Ƿ�unix�׽��� */
        const void*        buf,        /* ��������. ������NULL */
        unsigned long      size        /* ���ݳ���. ������0 */
        ) = 0;

    /*
     * ���ֳ����ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnHandshakeError(
        IProTcpHandshaker* handshaker,
        long               errorCode   /* ϵͳ������. �μ�"pro_util/pro_bsd_wrapper.h" */
        ) = 0;
};

/*
 * ssl�������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProSslHandshakerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ���ֳɹ�ʱ,�ú��������ص�
     *
     * ʹ���߸���(ctx, sockId)����Դά��.
     * ������ɺ�,�ϲ�Ӧ�ð�(ctx, sockId)��װ��IProTransport,
     * ���ͷ�(ctx, sockId)��Ӧ����Դ
     *
     * ����ProSslCtx_GetSuite(...)���Ի�֪��ǰʹ�õļ����׼�
     */
    virtual void PRO_CALLTYPE OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,        /* ssl������ */
        PRO_INT64          sockId,     /* �׽���id */
        bool               unixSocket, /* �Ƿ�unix�׽��� */
        const void*        buf,        /* ��������. ������NULL */
        unsigned long      size        /* ���ݳ���. ������0 */
        ) = 0;

    /*
     * ���ֳ����ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnHandshakeError(
        IProSslHandshaker* handshaker,
        long               errorCode,  /* ϵͳ������. �μ�"pro_util/pro_bsd_wrapper.h" */
        long               sslCode     /* ssl������. �μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ���ճ�
 *
 * ������IProTransportObserver::OnRecv(...)���߳���������ʹ��,���򲻰�ȫ
 *
 * ����tcp/ssl������,ʹ�û��ͽ��ճ�.
 * ����tcp����ʽ������,�ն˶������뷢�˶�������һ����ͬ,����,��OnRecv(...)
 * ��Ӧ�þ���ȡ�߽��ճ��ڵ�����. ���û���µ����ݵ���,��Ӧ�������ٴα������
 * ���ڻ���ʣ�����������
 *
 * ����udp/mcast������,ʹ�����Խ��ճ�.
 * Ϊ��ֹ���ճؿռ䲻�㵼��EMSGSIZE����,��OnRecv(...)��Ӧ������ȫ������
 *
 * ����,����Ӧ�������׽����������ݿɶ�ʱ,������ճ��Ѿ�<<����>>,��ô���׽���
 * �����ر�!!! ��˵���ն˺ͷ��˵�����߼�������!!!
 */
class IProRecvPool
{
public:

    /*
     * ��ѯ���ճ��ڵ�������
     */
    virtual unsigned long PRO_CALLTYPE PeekDataSize() const = 0;

    /*
     * ��ȡ���ճ��ڵ�����
     */
    virtual void PRO_CALLTYPE PeekData(void* buf, size_t size) const = 0;

    /*
     * ˢ���Ѿ���ȡ������
     *
     * �ڳ��ռ�,�Ա������µ�����
     */
    virtual void PRO_CALLTYPE Flush(size_t size) = 0;

    /*
     * ��ѯ���ճ���ʣ��Ĵ洢�ռ�
     */
    virtual unsigned long PRO_CALLTYPE GetFreeSize() const = 0;
};

/*
 * ������
 */
class IProTransport
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ��ȡ����������
     */
    virtual PRO_TRANS_TYPE PRO_CALLTYPE GetType() const = 0;

    /*
     * ��ȡ�����׼�
     *
     * ������PRO_TRANS_SSL���͵Ĵ�����
     */
    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(char suiteName[64]) const = 0;

    /*
     * ��ȡ�ײ���׽���id
     *
     * ��Ǳ���,��ò�Ҫֱ�Ӳ����ײ���׽���
     */
    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const = 0;

    /*
     * ��ȡ�׽��ֵı���ip��ַ
     */
    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const = 0;

    /*
     * ��ȡ�׽��ֵı��ض˿ں�
     */
    virtual unsigned short PRO_CALLTYPE GetLocalPort() const = 0;

    /*
     * ��ȡ�׽��ֵ�Զ��ip��ַ
     *
     * ����tcp, �������ӶԶ˵�ip��ַ;
     * ����udp, ����Ĭ�ϵ�Զ��ip��ַ
     */
    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * ��ȡ�׽��ֵ�Զ�˶˿ں�
     *
     * ����tcp, �������ӶԶ˵Ķ˿ں�;
     * ����udp, ����Ĭ�ϵ�Զ�˶˿ں�
     */
    virtual unsigned short PRO_CALLTYPE GetRemotePort() const = 0;

    /*
     * ��ȡ���ճ�
     *
     * �μ�IProRecvPool��ע��
     */
    virtual IProRecvPool* PRO_CALLTYPE GetRecvPool() = 0;

    /*
     * ��������
     *
     * ����tcp, ���ݽ��ŵ����ͳ���,����remoteAddr����;
     * ����udp, ���ݽ�ֱ�ӷ���,���remoteAddr������Ч,��ʹ��Ĭ�ϵ�Զ�˵�ַ;
     * actionId���ϲ�����һ��ֵ,���ڱ�ʶ��η��Ͷ���, OnSend(...)�ص�ʱ
     * ����ظ�ֵ
     *
     * �������false, ��ʾ����æ,�ϲ�Ӧ�û��������Դ�OnSend(...)�ص���ȡ
     */
    virtual bool PRO_CALLTYPE SendData(
        const void*             buf,
        size_t                  size,
        PRO_UINT64              actionId   = 0,
        const pbsd_sockaddr_in* remoteAddr = NULL /* for udp */
        ) = 0;

    /*
     * ����ص�һ��OnSend�¼�
     *
     * ���緢����������ʱ, OnSend(...)�ص��Ż����
     */
    virtual void PRO_CALLTYPE RequestOnSend() = 0;

    /*
     * �����������
     */
    virtual void PRO_CALLTYPE SuspendRecv() = 0;

    /*
     * �ָ���������
     */
    virtual void PRO_CALLTYPE ResumeRecv() = 0;

    /*
     * ��Ӷ���Ķಥ��ַ(for CProMcastTransport only)
     */
    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * ɾ������Ķಥ��ַ(for CProMcastTransport only)
     */
    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * ����������ʱ��
     *
     * �����¼�����ʱ, OnHeartbeat(...)�����ص�
     */
    virtual void PRO_CALLTYPE StartHeartbeat() = 0;

    /*
     * ֹͣ������ʱ��
     */
    virtual void PRO_CALLTYPE StopHeartbeat() = 0;
};

/*
 * �������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IProTransportObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ���ݵִ��׽��ֵĽ��ջ�����ʱ,�ú��������ص�
     *
     * ����tcp/ssl������,ʹ�û��ͽ��ճ�.
     * ����tcp����ʽ������,�ն˶������뷢�˶�������һ����ͬ,����,��OnRecv(...)
     * ��Ӧ�þ���ȡ�߽��ճ��ڵ�����. ���û���µ����ݵ���,��Ӧ�������ٴα������
     * ���ڻ���ʣ�����������
     *
     * ����udp/mcast������,ʹ�����Խ��ճ�.
     * Ϊ��ֹ���ճؿռ䲻�㵼��EMSGSIZE����,��OnRecv(...)��Ӧ������ȫ������
     *
     * ����,����Ӧ�������׽����������ݿɶ�ʱ,������ճ��Ѿ�<<����>>,��ô���׽���
     * �����ر�!!! ��˵���ն˺ͷ��˵�����߼�������!!!
     */
    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        ) = 0;

    /*
     * ���ݱ��ɹ������׽��ֵķ��ͻ�����ʱ,���ϲ���ù�RequestOnSend(...),
     * �ú��������ص�
     *
     * ����ص���RequestOnSend(...)������,��actionIdΪ0
     */
    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        ) = 0;

    /*
     * �׽��ֳ��ִ���ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode, /* ϵͳ������. �μ�"pro_util/pro_bsd_wrapper.h" */
        long           sslCode    /* ssl������. �μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * �����¼�����ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ����: ��ʼ�������
 *
 * ����: ��
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProNetInit();

/*
 * ����: ��ȡ�ÿ�İ汾��
 *
 * ����:
 * major : ���汾��
 * minor : �ΰ汾��
 * patch : ������
 *
 * ����ֵ: ��
 *
 * ˵��: �ú�����ʽ�㶨
 */
PRO_NET_API
void
PRO_CALLTYPE
ProNetVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * ����: ����һ����Ӧ��
 *
 * ����:
 * ioThreadCount    : �����շ��¼����߳���
 * ioThreadPriority : �շ��̵߳����ȼ�(0/1/2)
 *
 * ����ֵ: ��Ӧ�������NULL
 *
 * ˵��: ioThreadPriority���ܶ�һЩ�����Ӧ�ó������ô�. �����ý��ͨ��
 *       ��,���Խ���Ƶ��·����Ƶ��·�ŵ���ͬ��reactor��,������Ƶreactor
 *       ��ioThreadPriority��΢����,�����������ܼ�ʱ���Ը�����Ƶ�Ĵ���
 */
PRO_NET_API
IProReactor*
PRO_CALLTYPE
ProCreateReactor(unsigned long ioThreadCount,
                 long          ioThreadPriority = 0);

/*
 * ����: ɾ��һ����Ӧ��
 *
 * ����:
 * reactor : ��Ӧ������
 *
 * ����ֵ: ��
 *
 * ˵��: ��Ӧ������������,�ڲ������ص��̳߳�. �ú�������������,ֱ�����е�
 *       �ص��̶߳���������. �ϲ�Ҫȷ��ɾ�������ɻص��̳߳�֮����̷߳���,
 *       ��������ִ��,����ᷢ������
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteReactor(IProReactor* reactor);

/*
 * ����: ����һ��������
 *
 * ����:
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localIp   : Ҫ�����ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�����ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ����ʹ��ProGetAcceptorPort(...)��ȡʵ�ʵĶ˿ں�
 */
PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp   = NULL,
                  unsigned short        localPort = 0);

/*
 * ����: ����һ����չЭ�������
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localIp          : Ҫ�����ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort        : Ҫ�����ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��10��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ����ʹ��ProGetAcceptorPort(...)��ȡʵ�ʵĶ˿ں�
 *
 *       ��չЭ�������ڼ�,����id���ڷ���������ֳ���ʶ��ͻ���.
 *       ����˷���nonce���ͻ���,�ͻ��˷���(serviceId, serviceOpt)�������,
 *       ����˸��ݿͻ�������ķ���id, ���������ɷ�����Ӧ�Ĵ����߻�������
 */
PRO_NET_API
IProAcceptor*
PRO_CALLTYPE
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp          = NULL,
                    unsigned short        localPort        = 0,
                    unsigned long         timeoutInSeconds = 0);

/*
 * ����: ��ȡ�����������Ķ˿ں�
 *
 * ����:
 * acceptor : ����������
 *
 * ����ֵ: �˿ں�
 *
 * ˵��: ��Ҫ������������Ķ˿ںŵĻ�ȡ
 */
PRO_NET_API
unsigned short
PRO_CALLTYPE
ProGetAcceptorPort(IProAcceptor* acceptor);

/*
 * ����: ɾ��һ��������
 *
 * ����:
 * acceptor : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteAcceptor(IProAcceptor* acceptor);

/*
 * ����: ����һ��������
 *
 * ����:
 * enableUnixSocket : �Ƿ�����unix�׽���. ������Linux�����127.0.0.1����
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localBindIp      : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ӳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ��
 */
PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp      = NULL,
                   unsigned long          timeoutInSeconds = 0);

/*
 * ����: ����һ����չЭ��������
 *
 * ����:
 * enableUnixSocket : �Ƿ�����unix�׽���. ������Linux�����127.0.0.1����
 * serviceId        : ����id
 * serviceOpt       : ����ѡ��
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localBindIp      : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ӳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ��չЭ�������ڼ�,����id���ڷ���������ֳ���ʶ��ͻ���.
 *       ����˷���nonce���ͻ���,�ͻ��˷���(serviceId, serviceOpt)�������,
 *       ����˸��ݿͻ�������ķ���id, ���������ɷ�����Ӧ�Ĵ����߻�������
 */
PRO_NET_API
IProConnector*
PRO_CALLTYPE
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp      = NULL,
                     unsigned long          timeoutInSeconds = 0);

/*
 * ����: ɾ��һ��������
 *
 * ����:
 * connector : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteConnector(IProConnector* connector);

/*
 * ����: ����һ��tcp������
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * sockId           : �׽���id. ��Դ��OnAccept(...)��OnConnectOk(...)
 * unixSocket       : �Ƿ�unix�׽���
 * sendData         : ϣ�����͵�����
 * sendDataSize     : ϣ�����͵����ݳ���
 * recvDataSize     : ϣ�����յ����ݳ���
 * recvFirst        : ��������
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ���recvFirstΪtrue, �����������Ƚ��պ���
 */
PRO_NET_API
IProTcpHandshaker*
PRO_CALLTYPE
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned long              timeoutInSeconds = 0);

/*
 * ����: ɾ��һ��tcp������
 *
 * ����:
 * handshaker : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker);

/*
 * ����: ����һ��ssl������
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * ctx              : ssl������. ͨ��ProSslCtx_Creates(...)��ProSslCtx_Createc(...)����
 * sockId           : �׽���id. ��Դ��OnAccept(...)��OnConnectOk(...)
 * unixSocket       : �Ƿ�unix�׽���
 * sendData         : ϣ�����͵�����
 * sendDataSize     : ϣ�����͵����ݳ���
 * recvDataSize     : ϣ�����յ����ݳ���
 * recvFirst        : ��������
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ���ctx��δ���ssl/tlsЭ������ֹ���,���������������ssl/tls
 *       Э������ֹ���,�ڴ˻����Ͻ�һ��ִ���շ��û����ݵĸ߲����ֶ���.
 *       ���recvFirstΪtrue, ��߲����������Ƚ��պ���
 */
PRO_NET_API
IProSslHandshaker*
PRO_CALLTYPE
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       PRO_INT64                  sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned long              timeoutInSeconds = 0);

/*
 * ����: ɾ��һ��ssl������
 *
 * ����:
 * handshaker : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteSslHandshaker(IProSslHandshaker* handshaker);

/*
 * ����: ����һ��tcp������
 *
 * ����:
 * observer        : �ص�Ŀ��
 * reactor         : ��Ӧ��
 * sockId          : �׽���id. ��Դ��OnAccept(...)��OnConnectOk(...)
 * unixSocket      : �Ƿ�unix�׽���
 * sockBufSizeRecv : �׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 8)
 * recvPoolSize    : ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ��
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0);

/*
 * ����: ����һ��udp������
 *
 * ����:
 * observer        : �ص�Ŀ��
 * reactor         : ��Ӧ��
 * localIp         : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort       : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * sockBufSizeRecv : �׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 56)
 * recvPoolSize    : ���ճص��ֽ���. Ĭ��(1024 * 65)
 * remoteIp        : Ĭ�ϵ�Զ��ip��ַ������
 * remotePort      : Ĭ�ϵ�Զ�˶˿ں�
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: ����ʹ��IProTransport::GetLocalPort(...)��ȡʵ�ʵĶ˿ں�
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      const char*            localIp           = NULL,
                      unsigned short         localPort         = 0,
                      size_t                 sockBufSizeRecv   = 0,
                      size_t                 sockBufSizeSend   = 0,
                      size_t                 recvPoolSize      = 0,
                      const char*            defaultRemoteIp   = NULL,
                      unsigned short         defaultRemotePort = 0);

/*
 * ����: ����һ���ಥ������
 *
 * ����:
 * observer        : �ص�Ŀ��
 * reactor         : ��Ӧ��
 * mcastIp         : Ҫ�󶨵Ķಥ��ַ
 * mcastPort       : Ҫ�󶨵Ķಥ�˿ں�. ���Ϊ0, ϵͳ���������һ��
 * localBindIp     : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * sockBufSizeRecv : �׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 56)
 * recvPoolSize    : ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: �Ϸ��Ķಥ��ַΪ[224.0.0.0 ~ 239.255.255.255],
 *       �Ƽ��Ķಥ��ַΪ[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       ����ʹ��IProTransport::GetLocalPort(...)��ȡʵ�ʵĶ˿ں�
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateMcastTransport(IProTransportObserver* observer,
                        IProReactor*           reactor,
                        const char*            mcastIp,
                        unsigned short         mcastPort       = 0,
                        const char*            localBindIp     = NULL,
                        size_t                 sockBufSizeRecv = 0,
                        size_t                 sockBufSizeSend = 0,
                        size_t                 recvPoolSize    = 0);

/*
 * ����: ����һ��ssl������
 *
 * ����:
 * observer        : �ص�Ŀ��
 * reactor         : ��Ӧ��
 * ctx             : ssl������. ������IProSslHandshaker�����ֽ���ص�
 * sockId          : �׽���id. ��Դ��OnAccept(...)��OnConnectOk(...)
 * unixSocket      : �Ƿ�unix�׽���
 * sockBufSizeRecv : �׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 8)
 * recvPoolSize    : ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: �����������NULL
 *
 * ˵��: �����ǰ�ȫ����ctx�����ʱ��!!! �˺�, ctx������IProTransport����
 */
PRO_NET_API
IProTransport*
PRO_CALLTYPE
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      PRO_INT64              sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0);

/*
 * ����: ɾ��һ��������
 *
 * ����:
 * trans : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteTransport(IProTransport* trans);

/*
 * ����: ����һ������hub
 *
 * ����:
 * reactor          : ��Ӧ��
 * servicePort      : ����˿ں�
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��10��
 *
 * ����ֵ: ����hub�����NULL
 *
 * ˵��: ����hub���Խ���ͬ����id�����������ɷ�����Ӧ�ķ���host,
 *       �������Խ���̱߽�(WinCE���ܿ����)!!!
 *       ����hub�����host���,���Խ��������Ĺ������쵽��ͬ��λ��
 */
PRO_NET_API
IProServiceHub*
PRO_CALLTYPE
ProCreateServiceHub(IProReactor*   reactor,
                    unsigned short servicePort,
                    unsigned long  timeoutInSeconds = 0);

/*
 * ����: ɾ��һ������hub
 *
 * ����:
 * hub : ����hub����
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHub(IProServiceHub* hub);

/*
 * ����: ����һ������host
 *
 * ����:
 * observer    : �ص�Ŀ��
 * reactor     : ��Ӧ��
 * servicePort : ����˿ں�
 * serviceId   : ����id. 0��Ч
 *
 * ����ֵ: ����host�����NULL
 *
 * ˵��: ����host���Կ�����һ���ض�����id�Ľ�����
 */
PRO_NET_API
IProServiceHost*
PRO_CALLTYPE
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort,
                     unsigned char            serviceId);

/*
 * ����: ɾ��һ������host
 *
 * ����:
 * host : ����host����
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProDeleteServiceHost(IProServiceHost* host);

/*
 * ����: ��һ��tcp�׽���
 *
 * ����:
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�
 *
 * ����ֵ: �׽���id��-1
 *
 * ˵��: ��Ҫ���ڱ���rtcp�˿�
 */
PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenTcpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * ����: ��һ��udp�׽���
 *
 * ����:
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���ΪNULL, ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�
 *
 * ����ֵ: �׽���id��-1
 *
 * ˵��: ��Ҫ���ڱ���rtcp�˿�
 */
PRO_NET_API
PRO_INT64
PRO_CALLTYPE
ProOpenUdpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * ����: �ر�һ���׽���
 *
 * ����:
 * sockId : �׽���id
 * linger : �Ƿ����ӹر�
 *
 * ����ֵ: ��
 *
 * ˵��: OnAccept(...)��OnConnectOk(...)������׽���,���������׽���û�гɹ�
 *       ��װ��IProTransportʱ,Ӧ��ʹ�øú����ͷ�sockId��Ӧ���׽�����Դ
 */
PRO_NET_API
void
PRO_CALLTYPE
ProCloseSockId(PRO_INT64 sockId,
               bool      linger = false);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____PRO_NET_H____ */
