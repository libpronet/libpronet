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

/*         ______________________________________________________
 *        |         |                                  |         |
 *        |         |              ProRtp              |         |
 *        |         |__________________________________|         |
 *        |                                   |                  |
 *        |               ProNet              |                  |
 *        |___________________________________|                  |
 *        |                    |                     ProUtil     |
 *        |                    |                                 |
 *        |                    |_________________________________|
 *        |      MbedTLS                 |                       |
 *        |                              |       ProShared       |
 *        |______________________________|_______________________|
 *                     Fig.1 module hierarchy diagram
 */

/*       __________________                     _________________
 *      |    ServiceHub    |<----------------->|   RtpService    |
 *      |   ____________   |    ServicePipe    |(Acceptor Shadow)| Audio-Process
 *      |  |            |  |                   |_________________|
 *      |  |  Acceptor  |  |        ...         _________________
 *      |  |____________|  |                   |   RtpService    |
 *      |                  |    ServicePipe    |(Acceptor Shadow)| Video-Process
 *      |__________________|<----------------->|_________________|
 *           Hub-Process
 *                Fig.2 structure diagram of RtpService
 */

/*
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 * 5) client::[password hash]
 * 6) client ----->          rtp(RTP_SESSION_INFO)          -----> server
 * 7)                                             [password hash]::server
 * 8) client <-----          rtp(RTP_SESSION_ACK)           <----- server
 *                 Fig.3 TCP_EX handshake protocol flow chart
 */

/*
 * 1) client ----->                connect()                -----> server
 * 2) client <-----                 accept()                <----- server
 * 3) client <-----                  nonce                  <----- server
 * 4) client ----->  serviceId + serviceOpt + (r) + (r+1)   -----> server
 * 5) client <<====              ssl handshake              ====>> server
 * 6) client::[password hash]
 * 7) client ----->          rtp(RTP_SESSION_INFO)          -----> server
 * 8)                                             [password hash]::server
 * 9) client <-----          rtp(RTP_SESSION_ACK)           <----- server
 *                 Fig.4 SSL_EX handshake protocol flow chart
 */

/*
 * RFC-1889/1890, RFC-3550/3551, RFC-4571
 *
 * PSP-v1.0 (PRO Session Protocol version 1.0)
 */

#if !defined(____RTP_BASE_H____)
#define ____RTP_BASE_H____

#include "pro_net.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_RTP_LIB)
#define PRO_RTP_API
#elif defined(PRO_RTP_EXPORTS)
#if defined(_MSC_VER)
#define PRO_RTP_API /* .def */
#else
#define PRO_RTP_API PRO_EXPORT
#endif
#else
#define PRO_RTP_API PRO_IMPORT
#endif

class IRtpBucket;          /* rtp����Ͱ */
class IRtpService;         /* rtp���� */
class IRtpSessionObserver; /* rtp�Ự�ص�Ŀ�� */

/*
 * [[[[ rtpý������. 0��Ч, 1~127����, 128~255�Զ���
 */
#if !defined(____RTP_MM_TYPE____)
#define ____RTP_MM_TYPE____
typedef unsigned char RTP_MM_TYPE;

static const RTP_MM_TYPE RTP_MMT_MSG       = 11; /* ��Ϣ[11 ~ 20] */
static const RTP_MM_TYPE RTP_MMT_MSGII     = 12;
static const RTP_MM_TYPE RTP_MMT_MSGIII    = 13;
static const RTP_MM_TYPE RTP_MMT_MSG_MIN   = 11;
static const RTP_MM_TYPE RTP_MMT_MSG_MAX   = 20;
static const RTP_MM_TYPE RTP_MMT_AUDIO     = 21; /* ��Ƶ[21 ~ 30] */
static const RTP_MM_TYPE RTP_MMT_AUDIO_MIN = 21;
static const RTP_MM_TYPE RTP_MMT_AUDIO_MAX = 30;
static const RTP_MM_TYPE RTP_MMT_VIDEO     = 31; /* ��Ƶ[31 ~ 40] */
static const RTP_MM_TYPE RTP_MMT_VIDEOII   = 32;
static const RTP_MM_TYPE RTP_MMT_VIDEO_MIN = 31;
static const RTP_MM_TYPE RTP_MMT_VIDEO_MAX = 40;
static const RTP_MM_TYPE RTP_MMT_CTRL      = 41; /* ����[41 ~ 50] */
static const RTP_MM_TYPE RTP_MMT_CTRL_MIN  = 41;
static const RTP_MM_TYPE RTP_MMT_CTRL_MAX  = 50;
#endif /* ____RTP_MM_TYPE____ */
/*
 * ]]]]
 */

/*
 * [[[[ rtp��չ���ģʽ
 */
#if !defined(____RTP_EXT_PACK_MODE____)
#define ____RTP_EXT_PACK_MODE____
typedef unsigned char RTP_EXT_PACK_MODE;

static const RTP_EXT_PACK_MODE RTP_EPM_DEFAULT = 0; /* ext8 + rfc12 + payload */
static const RTP_EXT_PACK_MODE RTP_EPM_TCP2    = 2; /* len2 + payload */
static const RTP_EXT_PACK_MODE RTP_EPM_TCP4    = 4; /* len4 + payload */
#endif /* ____RTP_EXT_PACK_MODE____ */
/*
 * ]]]]
 */

/*
 * [[[[ rtp�Ự����
 */
typedef unsigned char RTP_SESSION_TYPE;

static const RTP_SESSION_TYPE RTP_ST_UDPCLIENT    =  1; /* udp-��׼rtpЭ��ͻ��� */
static const RTP_SESSION_TYPE RTP_ST_UDPSERVER    =  2; /* udp-��׼rtpЭ������ */
static const RTP_SESSION_TYPE RTP_ST_TCPCLIENT    =  3; /* tcp-��׼rtpЭ��ͻ��� */
static const RTP_SESSION_TYPE RTP_ST_TCPSERVER    =  4; /* tcp-��׼rtpЭ������ */
static const RTP_SESSION_TYPE RTP_ST_UDPCLIENT_EX =  5; /* udp-��չЭ��ͻ��� */
static const RTP_SESSION_TYPE RTP_ST_UDPSERVER_EX =  6; /* udp-��չЭ������ */
static const RTP_SESSION_TYPE RTP_ST_TCPCLIENT_EX =  7; /* tcp-��չЭ��ͻ��� */
static const RTP_SESSION_TYPE RTP_ST_TCPSERVER_EX =  8; /* tcp-��չЭ������ */
static const RTP_SESSION_TYPE RTP_ST_SSLCLIENT_EX =  9; /* ssl-��չЭ��ͻ��� */
static const RTP_SESSION_TYPE RTP_ST_SSLSERVER_EX = 10; /* ssl-��չЭ������ */
static const RTP_SESSION_TYPE RTP_ST_MCAST        = 11; /* mcast-��׼rtpЭ��� */
static const RTP_SESSION_TYPE RTP_ST_MCAST_EX     = 12; /* mcast-��չЭ��� */
/*
 * ]]]]
 */

/*
 * rtp�Ự��Ϣ
 */
struct RTP_SESSION_INFO
{
    PRO_UINT16        localVersion;     /* ���ذ汾��===[      ��������      ], for tcp_ex, ssl_ex */
    PRO_UINT16        remoteVersion;    /* Զ�˰汾��===[c��������, s��������>, for tcp_ex, ssl_ex */
    RTP_SESSION_TYPE  sessionType;      /* �Ự����=====[      ��������      ] */
    RTP_MM_TYPE       mmType;           /* ý������=====<      ��������      > */
    RTP_EXT_PACK_MODE packMode;         /* ���ģʽ=====<      ��������      >, for tcp_ex, ssl_ex */
    char              reserved1;
    char              passwordHash[32]; /* ����hashֵ===[c��������, s��������>, for tcp_ex, ssl_ex */
    char              reserved2[40];

    PRO_UINT32        someId;           /* ĳ��id. ���緿��id, Ŀ��ڵ�id��,���ϲ㶨�� */
    PRO_UINT32        mmId;             /* �ڵ�id */
    PRO_UINT32        inSrcMmId;        /* ����ý������Դ�ڵ�id. ����Ϊ0 */
    PRO_UINT32        outSrcMmId;       /* ���ý������Դ�ڵ�id. ����Ϊ0 */

    char              userData[64];     /* �û��Զ������� */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * bucket    : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_UDPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * localPort : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * bucket    : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_UDPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_TCPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];      /* = "" */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * localPort        : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_TCPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_UDPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];      /* = "" */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * localPort        : Ҫ�󶨵ı��ض˿ں�. ���Ϊ0, ϵͳ���������һ��
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_UDPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * password         : �Ự����
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_TCPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];     /* = "" */
    char                         localIp[64];      /* = "" */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer   : �ص�Ŀ��
 * reactor    : ��Ӧ��
 * sockId     : �׽���id. ��Դ��IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket : �Ƿ�unix�׽���
 * bucket     : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 */
struct RTP_INIT_TCPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * sslConfig        : ssl����
 * sslSni           : ssl������. �����Ч,�������֤�����֤��
 * remoteIp         : Զ�˵�ip��ַ������
 * remotePort       : Զ�˵Ķ˿ں�
 * password         : �Ự����
 * localIp          : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��20��
 * bucket           : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 *
 * ˵��: sslConfigָ���Ķ�������ڻỰ������������һֱ��Ч
 */
struct RTP_INIT_SSLCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    const PRO_SSL_CLIENT_CONFIG* sslConfig;
    char                         sslSni[64];       /* = "" */
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];     /* = "" */
    char                         localIp[64];      /* = "" */
    unsigned long                timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer   : �ص�Ŀ��
 * reactor    : ��Ӧ��
 * sslCtx     : ssl������
 * sockId     : �׽���id. ��Դ��IRtpServiceObserver::OnAcceptSession(...)
 * unixSocket : �Ƿ�unix�׽���
 * bucket     : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 *
 * ˵��: ��������ɹ�,�Ự����Ϊ(sslCtx, sockId)������;����,������Ӧ��
 *       �ͷ�(sslCtx, sockId)��Ӧ����Դ
 */
struct RTP_INIT_SSLSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_SSL_CTX*                 sslCtx;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * mcastIp   : Ҫ�󶨵Ķಥ��ַ
 * mcastPort : Ҫ�󶨵Ķಥ�˿ں�. ���Ϊ0, ϵͳ���������һ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * bucket    : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 *
 * ˵��: �Ϸ��Ķಥ��ַΪ[224.0.0.0 ~ 239.255.255.255],
 *       �Ƽ��Ķಥ��ַΪ[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 */
struct RTP_INIT_MCAST
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;        /* = 0 */
    char                         localIp[64];      /* = "" */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ������
 *
 * observer  : �ص�Ŀ��
 * reactor   : ��Ӧ��
 * mcastIp   : Ҫ�󶨵Ķಥ��ַ
 * mcastPort : Ҫ�󶨵Ķಥ�˿ں�. ���Ϊ0, ϵͳ���������һ��
 * localIp   : Ҫ�󶨵ı���ip��ַ. ���Ϊ"", ϵͳ��ʹ��0.0.0.0
 * bucket    : ����Ͱ. ���ΪNULL, ϵͳ���Զ�����һ��
 *
 * ˵��: �Ϸ��Ķಥ��ַΪ[224.0.0.0 ~ 239.255.255.255],
 *       �Ƽ��Ķಥ��ַΪ[224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 */
struct RTP_INIT_MCAST_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;        /* = 0 */
    char                         localIp[64];      /* = "" */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * rtp�Ự��ʼ��������������
 */
struct RTP_INIT_COMMON
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
};

/*
 * rtp�Ự��ʼ����������
 */
struct RTP_INIT_ARGS
{
    union
    {
        RTP_INIT_UDPCLIENT       udpclient;
        RTP_INIT_UDPSERVER       udpserver;
        RTP_INIT_TCPCLIENT       tcpclient;
        RTP_INIT_TCPSERVER       tcpserver;
        RTP_INIT_UDPCLIENT_EX    udpclientEx;
        RTP_INIT_UDPSERVER_EX    udpserverEx;
        RTP_INIT_TCPCLIENT_EX    tcpclientEx;
        RTP_INIT_TCPSERVER_EX    tcpserverEx;
        RTP_INIT_SSLCLIENT_EX    sslclientEx;
        RTP_INIT_SSLSERVER_EX    sslserverEx;
        RTP_INIT_MCAST           mcast;
        RTP_INIT_MCAST_EX        mcastEx;
        RTP_INIT_COMMON          comm;
    };
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * rtp��
 */
#if !defined(____IRtpPacket____)
#define ____IRtpPacket____
class IRtpPacket
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    virtual void PRO_CALLTYPE SetMarker(bool m) = 0;

    virtual bool PRO_CALLTYPE GetMarker() const = 0;

    virtual void PRO_CALLTYPE SetPayloadType(char pt) = 0;

    virtual char PRO_CALLTYPE GetPayloadType() const = 0;

    virtual void PRO_CALLTYPE SetSequence(PRO_UINT16 seq) = 0;

    virtual PRO_UINT16 PRO_CALLTYPE GetSequence() const = 0;

    virtual void PRO_CALLTYPE SetTimeStamp(PRO_UINT32 ts) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetTimeStamp() const = 0;

    virtual void PRO_CALLTYPE SetSsrc(PRO_UINT32 ssrc) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetSsrc() const = 0;

    virtual void PRO_CALLTYPE SetMmId(PRO_UINT32 mmId) = 0;

    virtual PRO_UINT32 PRO_CALLTYPE GetMmId() const = 0;

    virtual void PRO_CALLTYPE SetMmType(RTP_MM_TYPE mmType) = 0;

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const = 0;

    virtual void PRO_CALLTYPE SetKeyFrame(bool keyFrame) = 0;

    virtual bool PRO_CALLTYPE GetKeyFrame() const = 0;

    virtual void PRO_CALLTYPE SetFirstPacketOfFrame(bool firstPacket) = 0;

    virtual bool PRO_CALLTYPE GetFirstPacketOfFrame() const = 0;

    virtual const void* PRO_CALLTYPE GetPayloadBuffer() const = 0;

    virtual void* PRO_CALLTYPE GetPayloadBuffer() = 0;

    virtual unsigned long PRO_CALLTYPE GetPayloadSize() const = 0;

    virtual PRO_UINT16 PRO_CALLTYPE GetPayloadSize16() const = 0;

    virtual RTP_EXT_PACK_MODE PRO_CALLTYPE GetPackMode() const = 0;

    virtual void PRO_CALLTYPE SetMagic(PRO_INT64 magic) = 0;

    virtual PRO_INT64 PRO_CALLTYPE GetMagic() const = 0;
};
#endif /* ____IRtpPacket____ */

/*
 * rtp����Ͱ
 */
class IRtpBucket
{
public:

    virtual void PRO_CALLTYPE Destroy() = 0;

    virtual unsigned long PRO_CALLTYPE GetTotalBytes() const = 0;

    virtual IRtpPacket* PRO_CALLTYPE GetFront() = 0;

    virtual bool PRO_CALLTYPE PushBackAddRef(IRtpPacket* packet) = 0;

    virtual void PRO_CALLTYPE PopFrontRelease(IRtpPacket* packet) = 0;

    virtual void PRO_CALLTYPE Reset() = 0;

    virtual void PRO_CALLTYPE SetRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        ) = 0;

    virtual void PRO_CALLTYPE GetRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo() = 0;
};

/*
 * rtp��������
 */
class IRtpReorder
{
public:

    virtual void PRO_CALLTYPE SetGatePacketCount(
        unsigned char gatePacketCount             /* = 5 */
        ) = 0;

    virtual void PRO_CALLTYPE SetMaxWaitingDuration(
        unsigned char maxWaitingDurationInSeconds /* = 1 */
        ) = 0;

    virtual void PRO_CALLTYPE SetMaxBrokenDuration(
        unsigned char maxBrokenDurationInSeconds  /* = 10 */
        ) = 0;

    virtual void PRO_CALLTYPE PushBack(IRtpPacket* packet) = 0;

    virtual IRtpPacket* PRO_CALLTYPE PopFront() = 0;

    virtual void PRO_CALLTYPE Reset() = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * rtp����ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IRtpServiceObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ��tcp�Ự����ʱ,�ú��������ص�
     *
     * �ϲ�Ӧ�õ���CheckRtpServiceData(...)����У��,֮��,����remoteInfo,
     * ��sockId��װ��RTP_ST_TCPSERVER_EX���͵�IRtpSession����,
     * ���ͷ�sockId��Ӧ����Դ
     */
    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_INT64               sockId,     /* �׽���id */
        bool                    unixSocket, /* �Ƿ�unix�׽��� */
        const char*             remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short          remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        const RTP_SESSION_INFO* remoteInfo, /* Զ�˵ĻỰ��Ϣ. != NULL */
        PRO_UINT64              nonce       /* �Ự����� */
        ) = 0;

    /*
     * ��ssl�Ự����ʱ,�ú��������ص�
     *
     * �ϲ�Ӧ�õ���CheckRtpServiceData(...)����У��,֮��,����remoteInfo,
     * ��(sslCtx, sockId)��װ��RTP_ST_SSLSERVER_EX���͵�IRtpSession����,
     * ���ͷ�(sslCtx, sockId)��Ӧ����Դ
     */
    virtual void PRO_CALLTYPE OnAcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,     /* ssl������ */
        PRO_INT64               sockId,     /* �׽���id */
        bool                    unixSocket, /* �Ƿ�unix�׽��� */
        const char*             remoteIp,   /* Զ�˵�ip��ַ. != NULL */
        unsigned short          remotePort, /* Զ�˵Ķ˿ں�. > 0 */
        const RTP_SESSION_INFO* remoteInfo, /* Զ�˵ĻỰ��Ϣ. != NULL */
        PRO_UINT64              nonce       /* �Ự����� */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * rtp�Ự
 */
class IRtpSession
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ��ȡ�Ự��Ϣ
     */
    virtual void PRO_CALLTYPE GetInfo(RTP_SESSION_INFO* info) const = 0;

    /*
     * ��ȡ�Ự�ļ����׼�
     *
     * ������RTP_ST_SSLCLIENT_EX, RTP_ST_SSLSERVER_EX���͵ĻỰ
     */
    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(char suiteName[64]) const = 0;

    /*
     * ��ȡ�Ự���׽���id
     *
     * ��Ǳ���,��ò�Ҫֱ�Ӳ����ײ���׽���
     */
    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const = 0;

    /*
     * ��ȡ�Ự�ı���ip��ַ
     */
    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const = 0;

    /*
     * ��ȡ�Ự�ı��ض˿ں�
     */
    virtual unsigned short PRO_CALLTYPE GetLocalPort() const = 0;

    /*
     * ��ȡ�Ự��Զ��ip��ַ
     */
    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * ��ȡ�Ự��Զ�˶˿ں�
     */
    virtual unsigned short PRO_CALLTYPE GetRemotePort() const = 0;

    /*
     * ���ûỰ��Զ��ip��ַ�Ͷ˿ں�
     *
     * ������RTP_ST_UDPCLIENT, RTP_ST_UDPSERVER���͵ĻỰ
     */
    virtual void PRO_CALLTYPE SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        ) = 0;

    /*
     * ���Ự�Ƿ�������
     *
     * ������tcpЭ�����ĻỰ
     */
    virtual bool PRO_CALLTYPE IsTcpConnected() const = 0;

    /*
     * ���Ự�Ƿ����(�������)
     */
    virtual bool PRO_CALLTYPE IsReady() const = 0;

    /*
     * ֱ�ӷ���rtp��
     *
     * �������false, ��ʾ���ͳ�����,�ϲ�Ӧ�û��������Դ�
     * OnSendSession(...)�ص���ȡ
     */
    virtual bool PRO_CALLTYPE SendPacket(IRtpPacket* packet) = 0;

    /*
     * ͨ����ʱ��ƽ������rtp��(for CRtpSessionWrapper only)
     *
     * sendDurationMsΪƽ������. Ĭ��1����
     */
    virtual bool PRO_CALLTYPE SendPacketByTimer(
        IRtpPacket*   packet,
        unsigned long sendDurationMs = 0
        ) = 0;

    /*
     * ��ȡ�Ự�ķ���ʱ����Ϣ
     *
     * ���ڴ��Ե��ж�tcp��·�ķ����ӳ�,�ж�tcp��·�Ƿ��ϻ�
     */
    virtual void PRO_CALLTYPE GetSendOnSendTick(
        PRO_INT64* sendTick,  /* = NULL */
        PRO_INT64* onSendTick /* = NULL */
        ) const = 0;

    /*
     * ����Ự�ص�һ��OnSend�¼�
     */
    virtual void PRO_CALLTYPE RequestOnSend() = 0;

    /*
     * ����Ự�Ľ�������
     */
    virtual void PRO_CALLTYPE SuspendRecv() = 0;

    /*
     * �ָ��Ự�Ľ�������
     */
    virtual void PRO_CALLTYPE ResumeRecv() = 0;

    /*
     * Ϊ�Ự��Ӷ���Ķಥ���յ�ַ
     *
     * ������RTP_ST_MCAST, RTP_ST_MCAST_EX���͵ĻỰ
     */
    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * Ϊ�Ựɾ������Ķಥ���յ�ַ
     *
     * ������RTP_ST_MCAST, RTP_ST_MCAST_EX���͵ĻỰ
     */
    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * rtp��·ʹ��(for CRtpSessionWrapper only)------------------------------
     */

    virtual void PRO_CALLTYPE EnableInput(bool enable) = 0;

    virtual void PRO_CALLTYPE EnableOutput(bool enable) = 0;

    /*
     * rtp��������(for CRtpSessionWrapper only)------------------------------
     */

    virtual void PRO_CALLTYPE SetOutputRedline(
        unsigned long redlineBytes,  /* = 0 */
        unsigned long redlineFrames  /* = 0 */
        ) = 0;

    virtual void PRO_CALLTYPE GetOutputRedline(
        unsigned long* redlineBytes, /* = NULL */
        unsigned long* redlineFrames /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         inFrameRate,  /* = NULL */
        float*         inBitRate,    /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE ResetFlowctrlInfo() = 0;

    /*
     * rtp����ͳ��(for CRtpSessionWrapper only)------------------------------
     */

    virtual void PRO_CALLTYPE GetInputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE GetOutputStat(
        float* frameRate, /* = NULL */
        float* bitRate,   /* = NULL */
        float* lossRate,  /* = NULL */
        float* lossCount  /* = NULL */
        ) const = 0;

    virtual void PRO_CALLTYPE ResetInputStat() = 0;

    virtual void PRO_CALLTYPE ResetOutputStat() = 0;
};

/*
 * rtp�Ự�ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IRtpSessionObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * �������ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnOkSession(IRtpSession* session) = 0;

    /*
     * rtp������ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        ) = 0;

    /*
     * ������������ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnSendSession(
        IRtpSession* session,
        bool         packetErased /* �Ƿ���rtp�������ض��в��� */
        ) = 0;

    /*
     * ��������ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnCloseSession(
        IRtpSession* session,
        long         errorCode,   /* ϵͳ������ */
        long         sslCode,     /* ssl������. �μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool         tcpConnected /* tcp�����Ƿ��Ѿ����� */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ����: ��ʼ��rtp��
 *
 * ����: ��
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
ProRtpInit();

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
PRO_RTP_API
void
PRO_CALLTYPE
ProRtpVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * ����: ����һ��rtp��
 *
 * ����:
 * payloadBuffer : ý������ָ��
 * payloadSize   : ý�����ݳ���
 * packMode      : ���ģʽ
 *
 * ����ֵ: rtp�������NULL
 *
 * ˵��: ������Ӧ�ü�����ʼ��rtp����ͷ���ֶ�
 *
 *       ���packModeΪRTP_EPM_DEFAULT��RTP_EPM_TCP2, ��ôpayloadSize���
 *       (1024 * 63)�ֽ�;
 *       ���packModeΪRTP_EPM_TCP4, ��ôpayloadSize���
 *       (1024 * 1024 * 96)�ֽ�
 */
PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CreateRtpPacket(const void*       payloadBuffer,
                unsigned long     payloadSize,
                RTP_EXT_PACK_MODE packMode = RTP_EPM_DEFAULT);

/*
 * ����: ����һ��rtp��
 *
 * ����:
 * payloadSize : ý�����ݳ���
 * packMode    : ���ģʽ
 *
 * ����ֵ: rtp�������NULL
 *
 * ˵��: �ð汾��Ҫ���ڼ����ڴ濽������.
 *       ����,��Ƶ����������ͨ��IRtpPacket::GetPayloadBuffer(...)�õ�ý��
 *       ����ָ��,Ȼ��ֱ�ӽ���ý�����ݵĳ�ʼ���Ȳ���
 *
 *       ���packModeΪRTP_EPM_DEFAULT��RTP_EPM_TCP2, ��ôpayloadSize���
 *       (1024 * 63)�ֽ�;
 *       ���packModeΪRTP_EPM_TCP4, ��ôpayloadSize���
 *       (1024 * 1024 * 96)�ֽ�
 */
PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CreateRtpPacketSpace(unsigned long     payloadSize,
                     RTP_EXT_PACK_MODE packMode = RTP_EPM_DEFAULT);

/*
 * ����: ��¡һ��rtp��
 *
 * ����:
 * packet : ԭʼ��rtp������
 *
 * ����ֵ: ��¡��rtp�������NULL
 *
 * ˵��: ��
 */
PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CloneRtpPacket(const IRtpPacket* packet);

/*
 * ����: ����һ�α�׼��rtp��
 *
 * ����:
 * streamBuffer : ��ָ��
 * streamSize   : ������
 *
 * ����ֵ: rtp�������NULL
 *
 * ˵��: �������̻����IRtpPacket��֧�ֵ��ֶ�
 */
PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
ParseRtpStreamToPacket(const void* streamBuffer,
                       PRO_UINT16  streamSize);

/*
 * ����: ��һ��rtp���в���һ�α�׼��rtp��
 *
 * ����:
 * packet     : Ҫ�ҵ�rtp������
 * streamSize : �ҵ���������
 *
 * ����ֵ: �ҵ�����ָ���NULL
 *
 * ˵��: ����ʱ, (*streamSize)������������
 */
PRO_RTP_API
const void*
PRO_CALLTYPE
FindRtpStreamFromPacket(const IRtpPacket* packet,
                        PRO_UINT16*       streamSize);

/*
 * ����: ����rtp�˿ںŵķ��䷶Χ
 *
 * ����:
 * minUdpPort : ��Сudp�˿ں�
 * maxUdpPort : ���udp�˿ں�
 * minTcpPort : ��Сtcp�˿ں�
 * maxTcpPort : ���tcp�˿ں�
 *
 * ����ֵ: ��
 *
 * ˵��: Ĭ�ϵĶ˿ںŷ��䷶ΧΪ[3000 ~ 5999]
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpPortRange(unsigned short minUdpPort,
                unsigned short maxUdpPort,
                unsigned short minTcpPort,
                unsigned short maxTcpPort);

/*
 * ����: ��ȡrtp�˿ںŵķ��䷶Χ
 *
 * ����:
 * minUdpPort : ���ص���Сudp�˿ں�
 * maxUdpPort : ���ص����udp�˿ں�
 * minTcpPort : ���ص���Сtcp�˿ں�
 * maxTcpPort : ���ص����tcp�˿ں�
 *
 * ����ֵ: ��
 *
 * ˵��: Ĭ�ϵĶ˿ںŷ��䷶ΧΪ[3000 ~ 5999]
 */
PRO_RTP_API
void
PRO_CALLTYPE
GetRtpPortRange(unsigned short* minUdpPort,  /* = NULL */
                unsigned short* maxUdpPort,  /* = NULL */
                unsigned short* minTcpPort,  /* = NULL */
                unsigned short* maxTcpPort); /* = NULL */

/*
 * ����: �Զ�����һ��udp�˿ں�
 *
 * ����: ��
 *
 * ����ֵ: udp�˿ں�. [ż��]
 *
 * ˵��: ���صĶ˿ںŲ�һ������,Ӧ�ö�η��䳢��
 */
PRO_RTP_API
unsigned short
PRO_CALLTYPE
AllocRtpUdpPort();

/*
 * ����: �Զ�����һ��tcp�˿ں�
 *
 * ����: ��
 *
 * ����ֵ: tcp�˿ں�. [ż��]
 *
 * ˵��: ���صĶ˿ںŲ�һ������,Ӧ�ö�η��䳢��
 */
PRO_RTP_API
unsigned short
PRO_CALLTYPE
AllocRtpTcpPort();

/*
 * ����: ���ûỰ�ı��ʱ
 *
 * ����:
 * keepaliveInSeconds : ���ʱ. Ĭ��60��
 *
 * ����ֵ: ��
 *
 * ˵��: ���reactor��������ʱ��ʹ��. ���ʱӦ�ô����������ڵ�2��
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpKeepaliveTimeout(unsigned long keepaliveInSeconds); /* = 60 */

/*
 * ����: ��ȡ�Ự�ı��ʱ
 *
 * ����: ��
 *
 * ����ֵ: ���ʱ. Ĭ��60��
 *
 * ˵��: ���reactor��������ʱ��ʹ��. ���ʱӦ�ô����������ڵ�2��
 */
PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpKeepaliveTimeout();

/*
 * ����: ���ûỰ������ʱ�䴰
 *
 * ����:
 * flowctrlInSeconds : ����ʱ�䴰. Ĭ��1��
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpFlowctrlTimeSpan(unsigned long flowctrlInSeconds); /* = 1 */

/*
 * ����: ��ȡ�Ự������ʱ�䴰
 *
 * ����: ��
 *
 * ����ֵ: ����ʱ�䴰. Ĭ��1��
 *
 * ˵��: ��
 */
PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpFlowctrlTimeSpan();

/*
 * ����: ���ûỰ��ͳ��ʱ�䴰
 *
 * ����:
 * statInSeconds : ͳ��ʱ�䴰. Ĭ��5��
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpStatTimeSpan(unsigned long statInSeconds); /* = 5 */

/*
 * ����: ��ȡ�Ự��ͳ��ʱ�䴰
 *
 * ����: ��
 *
 * ����ֵ: ͳ��ʱ�䴰. Ĭ��5��
 *
 * ˵��: ��
 */
PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpStatTimeSpan();

/*
 * ����: ���õײ�udp�׽��ֵ�ϵͳ����
 *
 * ����:
 * mmType          : ý������
 * sockBufSizeRecv : �ײ��׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �ײ��׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 56)
 * recvPoolSize    : �ײ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: ��
 *
 * ˵��: ĳ��Ϊ0ʱ,��ʾ���ı���������
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpUdpSocketParams(RTP_MM_TYPE   mmType,
                      unsigned long sockBufSizeRecv, /* = 0 */
                      unsigned long sockBufSizeSend, /* = 0 */
                      unsigned long recvPoolSize);   /* = 0 */

/*
 * ����: ��ȡ�ײ�udp�׽��ֵ�ϵͳ����
 *
 * ����:
 * mmType          : ý������
 * sockBufSizeRecv : ���صĵײ��׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : ���صĵײ��׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 56)
 * recvPoolSize    : ���صĵײ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
GetRtpUdpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize);   /* = NULL */

/*
 * ����: ���õײ�tcp�׽��ֵ�ϵͳ����
 *
 * ����:
 * mmType          : ý������
 * sockBufSizeRecv : �ײ��׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : �ײ��׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 8)
 * recvPoolSize    : �ײ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: ��
 *
 * ˵��: ĳ��Ϊ0ʱ,��ʾ���ı���������
 */
PRO_RTP_API
void
PRO_CALLTYPE
SetRtpTcpSocketParams(RTP_MM_TYPE   mmType,
                      unsigned long sockBufSizeRecv, /* = 0 */
                      unsigned long sockBufSizeSend, /* = 0 */
                      unsigned long recvPoolSize);   /* = 0 */

/*
 * ����: ��ȡ�ײ�tcp�׽��ֵ�ϵͳ����
 *
 * ����:
 * mmType          : ý������
 * sockBufSizeRecv : ���صĵײ��׽��ֵ�ϵͳ���ջ������ֽ���. Ĭ��(1024 * 56)
 * sockBufSizeSend : ���صĵײ��׽��ֵ�ϵͳ���ͻ������ֽ���. Ĭ��(1024 * 8)
 * recvPoolSize    : ���صĵײ���ճص��ֽ���. Ĭ��(1024 * 65)
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
GetRtpTcpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize);   /* = NULL */

/*
 * ����: ����һ��rtp����
 *
 * ����:
 * sslConfig        : ssl����. ����ΪNULL
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * mmType           : �����ý������
 * serviceHubPort   : ����hub�Ķ˿ں�
 * timeoutInSeconds : ���ֳ�ʱ. Ĭ��10��
 *
 * ����ֵ: rtp��������NULL
 *
 * ˵��: ���sslConfigΪNULL, ��ʾ�÷���֧��ssl
 */
PRO_RTP_API
IRtpService*
PRO_CALLTYPE
CreateRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                 IRtpServiceObserver*         observer,
                 IProReactor*                 reactor,
                 RTP_MM_TYPE                  mmType,
                 unsigned short               serviceHubPort,
                 unsigned long                timeoutInSeconds = 0);

/*
 * ����: ɾ��һ��rtp����
 *
 * ����:
 * service : rtp�������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpService(IRtpService* service);

/*
 * ����: У��rtp�����յ��Ŀ���hashֵ
 *
 * ����:
 * serviceNonce       : ����˵ĻỰ�����
 * servicePassword    : ���������ĻỰ����
 * clientPasswordHash : �ͻ��˷����Ŀ���hashֵ
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: �����ʹ�øú���У��ͻ��˵ķ��ʺϷ���
 */
PRO_RTP_API
bool
PRO_CALLTYPE
CheckRtpServiceData(PRO_UINT64  serviceNonce,
                    const char* servicePassword,
                    const char  clientPasswordHash[32]);

/*
 * ����: ����һ���Ự��װ
 *
 * ����:
 * sessionType : �Ự����
 * initArgs    : �Ự��ʼ������
 * localInfo   : �Ự��Ϣ
 *
 * ����ֵ: �Ự��װ�����NULL
 *
 * ˵��: �Ự��װ����������ز���
 */
PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionWrapper(RTP_SESSION_TYPE        sessionType,
                        const RTP_INIT_ARGS*    initArgs,
                        const RTP_SESSION_INFO* localInfo);

/*
 * ����: ɾ��һ���Ự��װ
 *
 * ����:
 * sessionWrapper : �Ự��װ����
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpSessionWrapper(IRtpSession* sessionWrapper);

/*
 * ����: ����һ���������͵�����Ͱ
 *
 * ����: ��
 *
 * ����ֵ: ����Ͱ�����NULL
 *
 * ˵��: ����Ͱ���̰߳�ȫ���ɵ����߸���
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpBaseBucket();

/*
 * ����: ����һ����Ƶ���͵�����Ͱ
 *
 * ����: ��
 *
 * ����ֵ: ����Ͱ�����NULL
 *
 * ˵��: ����Ͱ���̰߳�ȫ���ɵ����߸���
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpAudioBucket();

/*
 * ����: ����һ����Ƶ���͵�����Ͱ
 *
 * ����: ��
 *
 * ����ֵ: ����Ͱ�����NULL
 *
 * ˵��: ����Ͱ���̰߳�ȫ���ɵ����߸���
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpVideoBucket();

/*
 * ����: ����һ����������
 *
 * ����: ��
 *
 * ����ֵ: �������������NULL
 *
 * ˵��: �����������̰߳�ȫ���ɵ����߸���
 */
PRO_RTP_API
IRtpReorder*
PRO_CALLTYPE
CreateRtpReorder();

/*
 * ����: ɾ��һ����������
 *
 * ����:
 * reorder : ������������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpReorder(IRtpReorder* reorder);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____RTP_BASE_H____ */
