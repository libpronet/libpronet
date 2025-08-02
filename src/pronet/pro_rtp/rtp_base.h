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
 *                     Fig.1 Module hierarchy diagram
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
 *                Fig.2 Structure diagram of RtpService
 */

/*               ______________________________
 *       ====>>in__________ o                  |__________
 *                         | o                  __________out====>>
 *                         |..ooooooooooooooooo|
 *                         |ooooooooooooooooooo|
 *                         |ooooooooooooooooooo|
 *                         |ooooooooooooooooooo|
 *                         |ooooooooooooooooooo|
 *                Fig.3 Structure diagram of RtpReorder
 */

/*
 * 1)  client ----->                 connect()                  -----> server
 * 2)  client <-----                  accept()                  <----- server
 * 3a) client <-----                 PRO_NONCE                  <----- server
 * 3b) client ----->    serviceId + serviceOpt + (r) + (r+1)    -----> server
 * 4]  client <<====              [SSL handshake]               ====>> server
 *     client::passwordHash
 * 5)  client ----->            RTP(RTP_SESSION_INFO)           -----> server
 *                                                       passwordHash::server
 * 6)  client <-----            RTP(RTP_SESSION_ACK)            <----- server
 *               Fig.4 TCP_EX/SSL_EX handshake protocol flow chart
 */

/*
 * RFC-1889/1890, RFC-3550/3551, RFC-4571
 *
 * PSP-v2.0 (PRO Session Protocol version 2.0)
 */

#ifndef ____RTP_BASE_H____
#define ____RTP_BASE_H____

#include "pro_net.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_RTP_EXPORTS)
#if defined(_MSC_VER)
#define PRO_RTP_API /* using xxx.def */
#elif defined(__MINGW32__) || defined(__CYGWIN__)
#define PRO_RTP_API __declspec(dllexport)
#else
#define PRO_RTP_API __attribute__((visibility("default")))
#endif
#else
#define PRO_RTP_API
#endif

class IRtpBucket;          /* RTP flow control bucket */
class IRtpService;         /* RTP service */
class IRtpSessionObserver; /* RTP session callback target */

/*
 * [[[[ RTP media types. 0 invalid, 1~127 reserved, 128~255 custom
 */
typedef unsigned char RTP_MM_TYPE;

static const RTP_MM_TYPE RTP_MMT_MSG       = 11; /* Message [10 ~ 69] */
static const RTP_MM_TYPE RTP_MMT_MSGII     = 12;
static const RTP_MM_TYPE RTP_MMT_MSGIII    = 13;
static const RTP_MM_TYPE RTP_MMT_MSG_MIN   = 10;
static const RTP_MM_TYPE RTP_MMT_MSG_MAX   = 69;
static const RTP_MM_TYPE RTP_MMT_AUDIO     = 71; /* Audio [70 ~ 79] */
static const RTP_MM_TYPE RTP_MMT_AUDIO_MIN = 70;
static const RTP_MM_TYPE RTP_MMT_AUDIO_MAX = 79;
static const RTP_MM_TYPE RTP_MMT_VIDEO     = 81; /* Video [80 ~ 89] */
static const RTP_MM_TYPE RTP_MMT_VIDEOII   = 82;
static const RTP_MM_TYPE RTP_MMT_VIDEO_MIN = 80;
static const RTP_MM_TYPE RTP_MMT_VIDEO_MAX = 89;
static const RTP_MM_TYPE RTP_MMT_CTRL      = 91; /* Control [90 ~ 99] */
static const RTP_MM_TYPE RTP_MMT_CTRL_MIN  = 90;
static const RTP_MM_TYPE RTP_MMT_CTRL_MAX  = 99;
/*
 * ]]]]
 */

/*
 * [[[[ RTP extended packing modes
 */
typedef unsigned char RTP_EXT_PACK_MODE;

static const RTP_EXT_PACK_MODE RTP_EPM_DEFAULT = 0; /* ext8 + rfc12 + payload */
static const RTP_EXT_PACK_MODE RTP_EPM_TCP2    = 2; /* len2 + payload */
static const RTP_EXT_PACK_MODE RTP_EPM_TCP4    = 4; /* len4 + payload */
/*
 * ]]]]
 */

/*
 * [[[[ RTP session types
 */
typedef unsigned char RTP_SESSION_TYPE;

static const RTP_SESSION_TYPE RTP_ST_UDPCLIENT    =  1; /* UDP - standard RTP protocol client */
static const RTP_SESSION_TYPE RTP_ST_UDPSERVER    =  2; /* UDP - standard RTP protocol server */
static const RTP_SESSION_TYPE RTP_ST_TCPCLIENT    =  3; /* TCP - standard RTP protocol client */
static const RTP_SESSION_TYPE RTP_ST_TCPSERVER    =  4; /* TCP - standard RTP protocol server */
static const RTP_SESSION_TYPE RTP_ST_UDPCLIENT_EX =  5; /* UDP - extended protocol client */
static const RTP_SESSION_TYPE RTP_ST_UDPSERVER_EX =  6; /* UDP - extended protocol server */
static const RTP_SESSION_TYPE RTP_ST_TCPCLIENT_EX =  7; /* TCP - extended protocol client */
static const RTP_SESSION_TYPE RTP_ST_TCPSERVER_EX =  8; /* TCP - extended protocol server */
static const RTP_SESSION_TYPE RTP_ST_SSLCLIENT_EX =  9; /* SSL - extended protocol client */
static const RTP_SESSION_TYPE RTP_ST_SSLSERVER_EX = 10; /* SSL - extended protocol server */
static const RTP_SESSION_TYPE RTP_ST_MCAST        = 11; /* Multicast - standard RTP protocol endpoint */
static const RTP_SESSION_TYPE RTP_ST_MCAST_EX     = 12; /* Multicast - extended protocol endpoint */
/*
 * ]]]]
 */

/*
 * RTP session information
 */
struct RTP_SESSION_INFO
{
    uint16_t          localVersion;     /* Local version  ==== [ No need to set, current value is 02  ], for tcp_ex, ssl_ex */
    uint16_t          remoteVersion;    /* Remote version ==== [Client no need to set, server must set], for tcp_ex, ssl_ex */
    RTP_SESSION_TYPE  sessionType;      /* Session type   ==== [            No need to set            ] */
    RTP_MM_TYPE       mmType;           /* Media type     ==== <               Must set               > */
    RTP_EXT_PACK_MODE packMode;         /* Packing mode   ==== <               Must set               >, for tcp_ex, ssl_ex */
    char              reserved1;
    unsigned char     passwordHash[32]; /* Password hash  ==== [Client no need to set, server must set], for tcp_ex, ssl_ex */
    char              reserved2[40];

    uint32_t          someId;           /* Some ID, such as room ID, target node ID, etc.,
                                           defined by upper layer */
    uint32_t          mmId;             /* Node ID */
    uint32_t          inSrcMmId;        /* Input media stream source node ID. Can be 0 */
    uint32_t          outSrcMmId;       /* Output media stream source node ID. Can be 0 */

    char              userData[64];     /* User-defined data */
};

/*
 * RTP session acknowledgment
 */
struct RTP_SESSION_ACK
{
    uint16_t version;      /* The current protocol version is 02 */
    char     reserved[30]; /* Zero value */

    char     userData[64];
};

/*
 * RTP session initialization parameters
 *
 * observer  : Callback target
 * reactor   : Reactor
 * localIp   : Local IP address to bind. If "", system uses 0.0.0.0
 * localPort : Local port number to bind. If 0, system assigns randomly
 * bucket    : Flow control bucket. If NULL, system allocates automatically
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
 * RTP session initialization parameters
 *
 * observer  : Callback target
 * reactor   : Reactor
 * localIp   : Local IP address to bind. If "", system uses 0.0.0.0
 * localPort : Local port number to bind. If 0, system assigns randomly
 * bucket    : Flow control bucket. If NULL, system allocates automatically
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
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
struct RTP_INIT_TCPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];      /* = "" */
    unsigned int                 timeoutInSeconds; /* = 0 */
    bool                         suspendRecv;      /* = false */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * localPort        : Local port number to bind. If 0, system assigns randomly
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
struct RTP_INIT_TCPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    unsigned int                 timeoutInSeconds; /* = 0 */
    bool                         suspendRecv;      /* = false */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 */
struct RTP_INIT_UDPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];      /* = "" */
    unsigned int                 timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * localPort        : Local port number to bind. If 0, system assigns randomly
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 */
struct RTP_INIT_UDPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];      /* = "" */
    unsigned short               localPort;        /* = 0 */
    unsigned int                 timeoutInSeconds; /* = 0 */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * password         : Session password
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
struct RTP_INIT_TCPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];     /* = "" */
    char                         localIp[64];      /* = "" */
    unsigned int                 timeoutInSeconds; /* = 0 */
    bool                         suspendRecv;      /* = false */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer    : Callback target
 * reactor     : Reactor
 * sockId      : Socket ID (from IRtpServiceObserver::OnAcceptSession())
 * unixSocket  : Whether it's a Unix socket
 * suspendRecv : Whether to suspend receive capability
 * useAckData  : Whether to use custom session acknowledgment data
 * ackData     : Custom session acknowledgment data
 * bucket      : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
struct RTP_INIT_TCPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    int64_t                      sockId;
    bool                         unixSocket;
    bool                         suspendRecv;      /* = false */
    bool                         useAckData;
    char                         ackData[64];
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer         : Callback target
 * reactor          : Reactor
 * sslConfig        : SSL configuration
 * sslSni           : SSL server name.
 *                    If valid, participates in server certificate verification
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * password         : Session password
 * localIp          : Local IP address to bind. If "", system uses 0.0.0.0
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 * suspendRecv      : Whether to suspend receive capability
 * bucket           : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: The object specified by sslConfig must remain valid throughout
 *       the session's lifetime
 *
 *       suspendRecv is used for scenarios requiring precise control
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
    unsigned int                 timeoutInSeconds; /* = 0 */
    bool                         suspendRecv;      /* = false */
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer    : Callback target
 * reactor     : Reactor
 * sslCtx      : SSL context
 * sockId      : Socket ID (from IRtpServiceObserver::OnAcceptSession())
 * unixSocket  : Whether it's a Unix socket
 * suspendRecv : Whether to suspend receive capability
 * useAckData  : Whether to use custom session acknowledgment data
 * ackData     : Custom session acknowledgment data
 * bucket      : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: If created successfully, session becomes owner of (sslCtx, sockId);
 *       Otherwise, caller should release resources for (sslCtx, sockId)
 *
 *       suspendRecv is used for scenarios requiring precise control
 */
struct RTP_INIT_SSLSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_SSL_CTX*                 sslCtx;
    int64_t                      sockId;
    bool                         unixSocket;
    bool                         suspendRecv;      /* = false */
    bool                         useAckData;
    char                         ackData[64];
    IRtpBucket*                  bucket;           /* = NULL */
};

/*
 * RTP session initialization parameters
 *
 * observer  : Callback target
 * reactor   : Reactor
 * mcastIp   : Multicast IP address to bind
 * mcastPort : Multicast port number to bind. If 0, system assigns randomly
 * localIp   : Local IP address to bind. If "", system uses 0.0.0.0
 * bucket    : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: Valid multicast addresses: [224.0.0.0 ~ 239.255.255.255],
 *       Recommended: [224.0.1.0 ~ 238.255.255.255],
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
 * RTP session initialization parameters
 *
 * observer  : Callback target
 * reactor   : Reactor
 * mcastIp   : Multicast IP address to bind
 * mcastPort : Multicast port number to bind. If 0, system assigns randomly
 * localIp   : Local IP address to bind. If "", system uses 0.0.0.0
 * bucket    : Flow control bucket. If NULL, system allocates automatically
 *
 * Note: Valid multicast addresses: [224.0.0.0 ~ 239.255.255.255],
 *       Recommended: [224.0.1.0 ~ 238.255.255.255],
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
 * Common part of RTP session initialization parameters
 */
struct RTP_INIT_COMMON
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
};

/*
 * Union of RTP session initialization parameters
 */
union RTP_INIT_ARGS
{
    RTP_INIT_UDPCLIENT           udpclient;
    RTP_INIT_UDPSERVER           udpserver;
    RTP_INIT_TCPCLIENT           tcpclient;
    RTP_INIT_TCPSERVER           tcpserver;
    RTP_INIT_UDPCLIENT_EX        udpclientEx;
    RTP_INIT_UDPSERVER_EX        udpserverEx;
    RTP_INIT_TCPCLIENT_EX        tcpclientEx;
    RTP_INIT_TCPSERVER_EX        tcpserverEx;
    RTP_INIT_SSLCLIENT_EX        sslclientEx;
    RTP_INIT_SSLSERVER_EX        sslserverEx;
    RTP_INIT_MCAST               mcast;
    RTP_INIT_MCAST_EX            mcastEx;
    RTP_INIT_COMMON              comm;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * RTP packet
 */
class IRtpPacket
{
public:

    virtual ~IRtpPacket() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    virtual void SetMarker(bool m) = 0;

    virtual bool GetMarker() const = 0;

    virtual void SetPayloadType(char pt) = 0;

    virtual char GetPayloadType() const = 0;

    virtual void SetSequence(uint16_t seq) = 0;

    virtual uint16_t GetSequence() const = 0;

    virtual void SetTimeStamp(uint32_t ts) = 0;

    virtual uint32_t GetTimeStamp() const = 0;

    virtual void SetSsrc(uint32_t ssrc) = 0;

    virtual uint32_t GetSsrc() const = 0;

    virtual void SetMmId(uint32_t mmId) = 0;

    virtual uint32_t GetMmId() const = 0;

    virtual void SetMmType(RTP_MM_TYPE mmType) = 0;

    virtual RTP_MM_TYPE GetMmType() const = 0;

    virtual void SetKeyFrame(bool keyFrame) = 0;

    virtual bool GetKeyFrame() const = 0;

    virtual void SetFirstPacketOfFrame(bool firstPacket) = 0;

    virtual bool GetFirstPacketOfFrame() const = 0;

    virtual const void* GetPayloadBuffer() const = 0;

    virtual void* GetPayloadBuffer() = 0;

    virtual size_t GetPayloadSize() const = 0;

    virtual uint16_t GetPayloadSize16() const = 0;

    virtual RTP_EXT_PACK_MODE GetPackMode() const = 0;

    virtual void SetMagic(int64_t magic) = 0;

    virtual int64_t GetMagic() const = 0;
};

/*
 * RTP flow control bucket
 */
class IRtpBucket
{
public:

    virtual ~IRtpBucket() {}

    virtual void Destroy() = 0;

    virtual size_t GetTotalBytes() const = 0;

    virtual IRtpPacket* GetFront() = 0;

    virtual bool PushBackAddRef(IRtpPacket* packet) = 0;

    virtual void PopFrontRelease(IRtpPacket* packet) = 0;

    virtual void Reset() = 0;

    virtual void SetRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        ) = 0;

    virtual void GetRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const = 0;

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const = 0;

    virtual void ResetFlowctrlInfo() = 0;
};

/*
 * RTP reorderer
 *
 * Typically used for playback side <<A/V over UDP>>, can also be used for deduplication
 */
class IRtpReorder
{
public:

    virtual ~IRtpReorder() {}

    virtual void SetWallHeightInPackets(size_t heightInPackets /* = 100 */) = 0;

    virtual void SetWallHeightInMilliseconds(size_t heightInMs /* = 500 */) = 0;

    virtual void SetMaxBrokenDuration(size_t brokenDurationInSeconds /* = 5 */) = 0;

    virtual size_t GetTotalPackets() const = 0;

    virtual void PushBackAddRef(IRtpPacket* packet) = 0;

    virtual IRtpPacket* PopFront(bool force) = 0;

    virtual void Reset() = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * RTP service callback target
 *
 * Users need to implement this interface
 */
class IRtpServiceObserver
{
public:

    virtual ~IRtpServiceObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a TCP session arrives
     *
     * Upper layer should call CheckRtpServiceData() for validation, then
     * based on remoteInfo, wrap sockId into IRtpSession object of type RTP_ST_TCPSERVER_EX,
     * or release sockId resources
     */
    virtual void OnAcceptSession(
        IRtpService*            service,
        int64_t                 sockId,     /* Socket ID */
        bool                    unixSocket, /* Whether it's a Unix socket */
        const char*             remoteIp,   /* Remote IP address. != NULL */
        unsigned short          remotePort, /* Remote port number. > 0 */
        const RTP_SESSION_INFO* remoteInfo, /* Remote session info. != NULL */
        const PRO_NONCE*        nonce       /* Session nonce. != NULL */
        ) = 0;

    /*
     * Called when an SSL session arrives
     *
     * Upper layer should call CheckRtpServiceData() for validation, then based on remoteInfo,
     * wrap (sslCtx, sockId) into IRtpSession object of type RTP_ST_SSLSERVER_EX,
     * or release (sslCtx, sockId) resources
     */
    virtual void OnAcceptSession(
        IRtpService*            service,
        PRO_SSL_CTX*            sslCtx,     /* SSL context */
        int64_t                 sockId,     /* Socket ID */
        bool                    unixSocket, /* Whether it's a Unix socket */
        const char*             remoteIp,   /* Remote IP address. != NULL */
        unsigned short          remotePort, /* Remote port number. > 0 */
        const RTP_SESSION_INFO* remoteInfo, /* Remote session info. != NULL */
        const PRO_NONCE*        nonce       /* Session nonce. != NULL */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * RTP session
 */
class IRtpSession
{
public:

    virtual ~IRtpSession() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Get session information
     */
    virtual void GetInfo(RTP_SESSION_INFO* info) const = 0;

    /*
     * Get session acknowledgment
     *
     * Only for RTP_ST_TCPCLIENT_EX, RTP_ST_SSLCLIENT_EX type sessions.
     * Meaningless before OnOkSession() callback
     */
    virtual void GetAck(RTP_SESSION_ACK* ack) const = 0;

    /*
     * Get session synchronization ID
     *
     * Only for RTP_ST_UDPCLIENT_EX, RTP_ST_UDPSERVER_EX type sessions.
     * Meaningless before OnOkSession() callback
     *
     * Upper layer can use sync ID for useful purposes, e.g., negotiating initial sequence
     * for reliable UDP links
     */
    virtual void GetSyncId(unsigned char syncId[14]) const = 0;

    /*
     * Get session cipher suite
     *
     * Only for RTP_ST_SSLCLIENT_EX, RTP_ST_SSLSERVER_EX type sessions
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const = 0;

    /*
     * Get session socket ID
     *
     * Avoid directly operating the underlying socket unless necessary
     */
    virtual int64_t GetSockId() const = 0;

    /*
     * Get local IP address of session
     */
    virtual const char* GetLocalIp(char localIp[64]) const = 0;

    /*
     * Get local port number of session
     */
    virtual unsigned short GetLocalPort() const = 0;

    /*
     * Get remote IP address of session
     */
    virtual const char* GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * Get remote port number of session
     */
    virtual unsigned short GetRemotePort() const = 0;

    /*
     * Set remote IP address and port number
     *
     * Only for RTP_ST_UDPCLIENT, RTP_ST_UDPSERVER type sessions
     */
    virtual void SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        ) = 0;

    /*
     * Check if session is connected
     *
     * Only for connection-oriented sessions
     */
    virtual bool IsTcpConnected() const = 0;

    /*
     * Check if session is ready (handshake completed)
     */
    virtual bool IsReady() const = 0;

    /*
     * Send RTP packet directly
     *
     * Return false and (*tryAgain) value true means underlying layer temporarily
     * cannot send, upper layer should buffer data for later sending
     * or wait for OnSendSession() callback
     */
    virtual bool SendPacket(
        IRtpPacket* packet,
        bool*       tryAgain = NULL
        ) = 0;

    /*
     * Send RTP packet smoothly via timer (for CRtpSessionWrapper only)
     *
     * sendDurationMs is smoothing period. Default 1 millisecond
     */
    virtual bool SendPacketByTimer(
        IRtpPacket*  packet,
        unsigned int sendDurationMs = 0
        ) = 0;

    /*
     * Get session send timing information
     *
     * Used to roughly judge TCP link send latency and aging
     */
    virtual void GetSendOnSendTick(
        int64_t* onSendTick1, /* = NULL */
        int64_t* onSendTick2  /* = NULL */
        ) const = 0;

    /*
     * Request session to callback an OnSend event
     *
     * Only for connection-oriented sessions
     */
    virtual void RequestOnSend() = 0;

    /*
     * Suspend session receive capability
     */
    virtual void SuspendRecv() = 0;

    /*
     * Resume session receive capability
     */
    virtual void ResumeRecv() = 0;

    /*
     * Add additional multicast receive address for session
     *
     * Only for RTP_ST_MCAST, RTP_ST_MCAST_EX type sessions
     */
    virtual bool AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * Remove additional multicast receive address for session
     *
     * Only for RTP_ST_MCAST, RTP_ST_MCAST_EX type sessions
     */
    virtual void RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * RTP link enabling (for CRtpSessionWrapper only)
     */

    virtual void EnableInput(bool enable) = 0;

    virtual void EnableOutput(bool enable) = 0;

    /*
     * RTP flow control (for CRtpSessionWrapper only)
     */

    virtual void SetOutputRedline(
        size_t redlineBytes,  /* = 0 */
        size_t redlineFrames, /* = 0 */
        size_t redlineDelayMs /* = 0 */
        ) = 0;

    virtual void GetOutputRedline(
        size_t* redlineBytes,  /* = NULL */
        size_t* redlineFrames, /* = NULL */
        size_t* redlineDelayMs /* = NULL */
        ) const = 0;

    virtual void GetFlowctrlInfo(
        float*  srcFrameRate, /* = NULL */
        float*  srcBitRate,   /* = NULL */
        float*  outFrameRate, /* = NULL */
        float*  outBitRate,   /* = NULL */
        size_t* cachedBytes,  /* = NULL */
        size_t* cachedFrames  /* = NULL */
        ) const = 0;

    virtual void ResetFlowctrlInfo() = 0;

    /*
     * RTP data statistics (for CRtpSessionWrapper only)
     */

    virtual void GetInputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const = 0;

    virtual void GetOutputStat(
        float*    frameRate, /* = NULL */
        float*    bitRate,   /* = NULL */
        float*    lossRate,  /* = NULL */
        uint64_t* lossCount  /* = NULL */
        ) const = 0;

    virtual void ResetInputStat() = 0;

    virtual void ResetOutputStat() = 0;

    virtual void SetMagic(int64_t magic) = 0;

    virtual int64_t GetMagic() const = 0;
};

/*
 * RTP session callback target
 *
 * Users need to implement this interface
 */
class IRtpSessionObserver
{
public:

    virtual ~IRtpSessionObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when handshake completes successfully
     */
    virtual void OnOkSession(IRtpSession* session) = 0;

    /*
     * Called when RTP packet arrives
     */
    virtual void OnRecvSession(
        IRtpSession* session,
        IRtpPacket*  packet
        ) = 0;

    /*
     * Called when send capability becomes idle
     *
     * Only for connection-oriented sessions
     */
    virtual void OnSendSession(
        IRtpSession* session,
        bool         packetErased /* Whether an RTP packet was erased by flow control queue */
        ) = 0;

    /*
     * Called when network error or timeout occurs
     */
    virtual void OnCloseSession(
        IRtpSession* session,
        int          errorCode,   /* System error code */
        int          sslCode,     /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        bool         tcpConnected /* Whether TCP connection was established */
        ) = 0;

    /*
     * Called when heartbeat occurs
     *
     * Only for the following session types:
     * RTP_ST_UDPCLIENT_EX, RTP_ST_UDPSERVER_EX,
     * RTP_ST_TCPCLIENT_EX, RTP_ST_TCPSERVER_EX,
     * RTP_ST_SSLCLIENT_EX, RTP_ST_SSLSERVER_EX
     *
     * Mainly used for debugging
     */
    virtual void OnHeartbeatSession(
        IRtpSession* session,
        int64_t      peerAliveTick
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Function: Initialize RTP library
 *
 * Parameters: None
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
ProRtpInit();

/*
 * Function: Get version number of this library
 *
 * Parameters:
 * major : Major version number
 * minor : Minor version number
 * patch : Patch number
 *
 * Return: None
 *
 * Note: The function signature is fixed
 */
PRO_RTP_API
void
ProRtpVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * Function: Create an RTP packet
 *
 * Parameters:
 * payloadBuffer : Media data pointer
 * payloadSize   : Media data length
 * packMode      : Packing mode
 *
 * Return: RTP packet object or NULL
 *
 * Note: Caller should continue to initialize RTP packet header fields
 *
 *       If packMode is RTP_EPM_DEFAULT or RTP_EPM_TCP2, payloadSize is at most (1024 * 63) bytes;
 *       If packMode is RTP_EPM_TCP4, payloadSize is at most (1024 * 1024 * 96) bytes
 */
PRO_RTP_API
IRtpPacket*
CreateRtpPacket(const void*       payloadBuffer,
                size_t            payloadSize,
                RTP_EXT_PACK_MODE packMode = RTP_EPM_DEFAULT);

/*
 * Function: Create an RTP packet
 *
 * Parameters:
 * payloadSize : Media data length
 * packMode    : Packing mode
 *
 * Return: RTP packet object or NULL
 *
 * Note: This version mainly reduces memory copy overhead.
 *       For example, video encoder can get media data pointer via IRtpPacket::GetPayloadBuffer(),
 *       then directly initialize media data
 *
 *      If packMode is RTP_EPM_DEFAULT or RTP_EPM_TCP2, payloadSize is at most (1024 * 63) bytes;
 *      If packMode is RTP_EPM_TCP4, payloadSize is at most (1024 * 1024 * 96) bytes
 */
PRO_RTP_API
IRtpPacket*
CreateRtpPacketSpace(size_t            payloadSize,
                     RTP_EXT_PACK_MODE packMode = RTP_EPM_DEFAULT);

/*
 * Function: Clone an RTP packet
 *
 * Parameters:
 * packet : Original RTP packet object
 *
 * Return: Cloned RTP packet object or NULL
 *
 * Note: None
 */
PRO_RTP_API
IRtpPacket*
CloneRtpPacket(const IRtpPacket* packet);

/*
 * Function: Parse standard RTP stream data
 *
 * Parameters:
 * streamBuffer : Stream pointer
 * streamSize   : Stream length
 *
 * Return: RTP packet object or NULL
 *
 * Note: Parsing process will ignore fields not supported by IRtpPacket
 */
PRO_RTP_API
IRtpPacket*
ParseRtpStreamToPacket(const void* streamBuffer,
                       uint16_t    streamSize);

/*
 * Function: Find standard RTP stream from RTP packet
 *
 * Parameters:
 * packet     : RTP packet object to search
 * streamSize : Found stream length
 *
 * Return: Found stream pointer or NULL
 *
 * Note: Upon return, (*streamSize) contains stream length
 */
PRO_RTP_API
const void*
FindRtpStreamFromPacket(const IRtpPacket* packet,
                        uint16_t*         streamSize);

/*
 * Function: Set RTP port allocation range
 *
 * Parameters:
 * minUdpPort : Minimum UDP port number. 0 ignores UDP setting
 * maxUdpPort : Maximum UDP port number. 0 ignores UDP setting
 * minTcpPort : Minimum TCP port number. 0 ignores TCP setting
 * maxTcpPort : Maximum TCP port number. 0 ignores TCP setting
 *
 * Return: None
 *
 * Note: Default port range is [3000 ~ 9999]
 */
PRO_RTP_API
void
SetRtpPortRange(unsigned short minUdpPort,  /* = 0 */
                unsigned short maxUdpPort,  /* = 0 */
                unsigned short minTcpPort,  /* = 0 */
                unsigned short maxTcpPort); /* = 0 */

/*
 * Function: Get RTP port allocation range
 *
 * Parameters:
 * minUdpPort : Returned minimum UDP port number
 * maxUdpPort : Returned maximum UDP port number
 * minTcpPort : Returned minimum TCP port number
 * maxTcpPort : Returned maximum TCP port number
 *
 * Return: None
 *
 * Note: Default port range is [3000 ~ 9999]
 */
PRO_RTP_API
void
GetRtpPortRange(unsigned short* minUdpPort,  /* = NULL */
                unsigned short* maxUdpPort,  /* = NULL */
                unsigned short* minTcpPort,  /* = NULL */
                unsigned short* maxTcpPort); /* = NULL */

/*
 * Function: Automatically allocate a UDP port number
 *
 * Parameters:
 * rfc : Whether to follow RFC rules (i.e. allocate even port numbers)
 *
 * Return: UDP port number
 *
 * Note: Returned port may not be free, try multiple allocations
 */
PRO_RTP_API
unsigned short
AllocRtpUdpPort(bool rfc);

/*
 * Function: Automatically allocate a TCP port number
 *
 * Parameters:
 * rfc : Whether to follow RFC rules (i.e. allocate even port numbers)
 *
 * Return: TCP port number
 *
 * Note: Returned port may not be free, try multiple allocations
 */
PRO_RTP_API
unsigned short
AllocRtpTcpPort(bool rfc);

/*
 * Function: Set session keepalive timeout
 *
 * Parameters:
 * keepaliveInSeconds : Keepalive timeout. Default 60 seconds
 *
 * Return: None
 *
 * Note: Used with reactor's heartbeat timer. Keepalive timeout should be > 2x heartbeat period
 */
PRO_RTP_API
void
SetRtpKeepaliveTimeout(unsigned int keepaliveInSeconds); /* = 60 */

/*
 * Function: Get session keepalive timeout
 *
 * Parameters: None
 *
 * Return: Keepalive timeout. Default 60 seconds
 *
 * Note: Used with reactor's heartbeat timer. Keepalive timeout should be > 2x heartbeat period
 */
PRO_RTP_API
unsigned int
GetRtpKeepaliveTimeout();

/*
 * Function: Set session flow control time window
 *
 * Parameters:
 * flowctrlInSeconds : Flow control time window. Default 1 second
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
SetRtpFlowctrlTimeSpan(unsigned int flowctrlInSeconds); /* = 1 */

/*
 * Function: Get session flow control time window
 *
 * Parameters: None
 *
 * Return: Flow control time window. Default 1 second
 *
 * Note: None
 */
PRO_RTP_API
unsigned int
GetRtpFlowctrlTimeSpan();

/*
 * Function: Set session statistics time window
 *
 * Parameters:
 * statInSeconds : Statistics time window. Default 5 seconds
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
SetRtpStatTimeSpan(unsigned int statInSeconds); /* = 5 */

/*
 * Function: Get session statistics time window
 *
 * Parameters: None
 *
 * Return: Statistics time window. Default 5 seconds
 *
 * Note: None
 */
PRO_RTP_API
unsigned int
GetRtpStatTimeSpan();

/*
 * Function: Set underlying UDP socket system parameters
 *
 * Parameters:
 * mmType          : Media type
 * sockBufSizeRecv : Underlying socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Underlying socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Underlying receive pool size in bytes. Default (1024 * 65)
 *
 * Return: None
 *
 * Note: When an item is 0, means don't change that item's setting
 */
PRO_RTP_API
void
SetRtpUdpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize);   /* = 0 */

/*
 * Function: Get underlying UDP socket system parameters
 *
 * Parameters:
 * mmType          : Media type
 * sockBufSizeRecv : Returned underlying socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Returned underlying socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Returned underlying receive pool size in bytes. Default (1024 * 65)
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
GetRtpUdpSocketParams(RTP_MM_TYPE mmType,
                      size_t*     sockBufSizeRecv, /* = NULL */
                      size_t*     sockBufSizeSend, /* = NULL */
                      size_t*     recvPoolSize);   /* = NULL */

/*
 * Function: Set underlying TCP socket system parameters
 *
 * Parameters:
 * mmType          : Media type
 * sockBufSizeRecv : Underlying socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Underlying socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Underlying receive pool size in bytes. Default (1024 * 65)
 *
 * Return: None
 *
 * Note: When an item is 0, means don't change that item's setting
 */
PRO_RTP_API
void
SetRtpTcpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize);   /* = 0 */

/*
 * Function: Get underlying TCP socket system parameters
 *
 * Parameters:
 * mmType          : Media type
 * sockBufSizeRecv : Returned underlying socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Returned underlying socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Returned underlying receive pool size in bytes. Default (1024 * 65)
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
GetRtpTcpSocketParams(RTP_MM_TYPE mmType,
                      size_t*     sockBufSizeRecv, /* = NULL */
                      size_t*     sockBufSizeSend, /* = NULL */
                      size_t*     recvPoolSize);   /* = NULL */

/*
 * Function: Create an RTP service
 *
 * Parameters:
 * sslConfig        : SSL configuration. Can be NULL
 * observer         : Callback target
 * reactor          : Reactor
 * mmType           : Service media type
 * serviceHubPort   : Service hub port number
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 *
 * Return: RTP service object or NULL
 *
 * Note: If sslConfig is NULL, means this service doesn't support SSL
 */
PRO_RTP_API
IRtpService*
CreateRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig, /* = NULL */
                 IRtpServiceObserver*         observer,
                 IProReactor*                 reactor,
                 RTP_MM_TYPE                  mmType,
                 unsigned short               serviceHubPort,
                 unsigned int                 timeoutInSeconds = 0);

/*
 * Function: Delete an RTP service
 *
 * Parameters:
 * service : RTP service object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpService(IRtpService* service);

/*
 * Function: Verify password hash value received by RTP service
 *
 * Parameters:
 * serviceNonce       : Server session nonce
 * servicePassword    : Server-stored session password
 * clientPasswordHash : Password hash sent by client
 *
 * Return: true on success, false on failure
 *
 * Note: Used by server to verify client access legitimacy
 */
PRO_RTP_API
bool
CheckRtpServiceData(const unsigned char serviceNonce[32],
                    const char*         servicePassword,
                    const unsigned char clientPasswordHash[32]);

/*
 * Function: Create a session wrapper
 *
 * Parameters:
 * sessionType : Session type
 * initArgs    : Session initialization parameters
 * localInfo   : Session information
 *
 * Return: Session wrapper object or NULL
 *
 * Note: Session wrapper object contains flow control strategy
 */
PRO_RTP_API
IRtpSession*
CreateRtpSessionWrapper(RTP_SESSION_TYPE        sessionType,
                        const RTP_INIT_ARGS*    initArgs,
                        const RTP_SESSION_INFO* localInfo);

/*
 * Function: Delete a session wrapper
 *
 * Parameters:
 * sessionWrapper : Session wrapper object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpSessionWrapper(IRtpSession* sessionWrapper);

/*
 * Function: Create a base flow control bucket
 *
 * Parameters: None
 *
 * Return: Flow control bucket object or NULL
 *
 * Note: Thread safety of flow control bucket is caller's responsibility
 */
PRO_RTP_API
IRtpBucket*
CreateRtpBaseBucket();

/*
 * Function: Create an audio flow control bucket
 *
 * Parameters: None
 *
 * Return: Flow control bucket object or NULL
 *
 * Note: Thread safety of flow control bucket is caller's responsibility
 */
PRO_RTP_API
IRtpBucket*
CreateRtpAudioBucket();

/*
 * Function: Create a video flow control bucket
 *
 * Parameters: None
 *
 * Return: Flow control bucket object or NULL
 *
 * Note: Thread safety of flow control bucket is caller's responsibility
 *
 *       This bucket operates per video frame, suitable for source end.
 *       If used for non-source end relaying, note it adds extra network latency
 */
PRO_RTP_API
IRtpBucket*
CreateRtpVideoBucket();

/*
 * Function: Create a reorderer
 *
 * Parameters: None
 *
 * Return: Reorderer object or NULL
 *
 * Note: Thread safety of reorderer is caller's responsibility
 */
PRO_RTP_API
IRtpReorder*
CreateRtpReorder();

/*
 * Function: Delete a reorderer
 *
 * Parameters:
 * reorder : Reorderer object
 *
 * Return: None
 *
 * Note: None
 */
PRO_RTP_API
void
DeleteRtpReorder(IRtpReorder* reorder);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____RTP_BASE_H____ */
