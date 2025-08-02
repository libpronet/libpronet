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
 *                   Fig.2 Structure diagram of Reactor
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
 *                Fig.3 Structure diagram of ServiceHub
 */

/*
 * 1)  client ----->                 connect()                  -----> server
 * 2)  client <-----                  accept()                  <----- server
 * 3a) client <-----                 PRO_NONCE                  <----- server
 * 3b) client ----->    serviceId + serviceOpt + (r) + (r+1)    -----> server
 * 4]  client <<====              [SSL handshake]               ====>> server
 *          Fig.4 acceptor_ex/connector_ex handshake protocol flow chart
 */

#ifndef ____PRO_NET_H____
#define ____PRO_NET_H____

#include "pro_a.h"
#include "pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_NET_EXPORTS)
#if defined(_MSC_VER)
#define PRO_NET_API /* using xxx.def */
#elif defined(__MINGW32__) || defined(__CYGWIN__)
#define PRO_NET_API __declspec(dllexport)
#else
#define PRO_NET_API __attribute__((visibility("default")))
#endif
#else
#define PRO_NET_API
#endif

#include "pro_ssl.h"

class  IProAcceptor;      /* Acceptor */
class  IProConnector;     /* Connector */
class  IProServiceHost;   /* Service host */
class  IProServiceHub;    /* Service hub */
class  IProSslHandshaker; /* SSL handshaker */
class  IProTcpHandshaker; /* TCP handshaker */
struct pbsd_sockaddr_in;  /* Socket address */

/*
 * [[[[ Transport types
 */
typedef unsigned char PRO_TRANS_TYPE;

static const PRO_TRANS_TYPE PRO_TRANS_TCP   =  1;
static const PRO_TRANS_TYPE PRO_TRANS_UDP   =  2;
static const PRO_TRANS_TYPE PRO_TRANS_MCAST =  3;
static const PRO_TRANS_TYPE PRO_TRANS_SSL   = 11;
/*
 * ]]]]
 */

/*
 * Session nonce
 */
struct PRO_NONCE
{
    unsigned char nonce[32];
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Timer callback target
 *
 * Users need to implement this interface
 *
 * Please refer to "pro_util/pro_timer_factory.h"
 */
#if !defined(____IProOnTimer____)
#define ____IProOnTimer____
class IProOnTimer
{
public:

    virtual ~IProOnTimer() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    virtual void OnTimer(
        void*    factory,
        uint64_t timerId,
        int64_t  tick,
        int64_t  userData
        ) = 0;
};
#endif /* ____IProOnTimer____ */

/*
 * Reactor
 *
 * Only timer-related methods are exposed here, to facilitate using internal timers
 * without creating additional CProTimerFactory objects
 */
class IProReactor
{
public:

    virtual ~IProReactor() {}

    /*
     * Create a normal timer
     *
     * Return timer ID. 0 is invalid
     */
    virtual uint64_t SetupTimer(
        IProOnTimer* onTimer,
        uint64_t     firstDelay, /* First trigger delay (ms) */
        uint64_t     period,     /* Timer period (ms). If 0, triggers only once */
        int64_t      userData = 0
        ) = 0;

    /*
     * Create a normal timer for link heartbeat (heartbeat timer)
     *
     * Heartbeat time points are determined by internal algorithm
     * with uniform distribution. When heartbeat event occurs, upper layer
     * can send heartbeat packets in OnTimer() callback
     *
     * Return timer ID. 0 is invalid
     */
    virtual uint64_t SetupHeartbeatTimer(
        IProOnTimer* onTimer,
        int64_t      userData = 0
        ) = 0;

    /*
     * Update heartbeat period for all heartbeat timers
     *
     * Default heartbeat period is 20 seconds
     */
    virtual bool UpdateHeartbeatTimers(unsigned int htbtIntervalInSeconds) = 0;

    /*
     * Cancel a normal timer
     */
    virtual void CancelTimer(uint64_t timerId) = 0;

    /*
     * Create a multimedia timer (high-precision timer)
     *
     * Return timer ID. 0 is invalid
     */
    virtual uint64_t SetupMmTimer(
        IProOnTimer* onTimer,
        uint64_t     firstDelay, /* First trigger delay (ms) */
        uint64_t     period,     /* Timer period (ms). If 0, triggers only once */
        int64_t      userData = 0
        ) = 0;

    /*
     * Cancel a multimedia timer (high-precision timer)
     */
    virtual void CancelMmTimer(uint64_t timerId) = 0;

    /*
     * Get status information string
     */
    virtual void GetTraceInfo(
        char*  buf,
        size_t size
        ) const = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Acceptor callback target
 *
 * Users need to implement this interface
 */
class IProAcceptorObserver
{
public:

    virtual ~IProAcceptorObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a raw connection arrives
     *
     * This callback is for acceptors created by ProCreateAcceptor().
     * User is responsible for sockId resource management
     */
    virtual void OnAccept(
        IProAcceptor*  acceptor,
        int64_t        sockId,     /* Socket ID */
        bool           unixSocket, /* Whether it's a Unix socket */
        const char*    localIp,    /* Local IP address. != NULL */
        const char*    remoteIp,   /* Remote IP address. != NULL */
        unsigned short remotePort  /* Remote port number. > 0 */
        ) = 0;

    /*
     * Called when an extended protocol connection arrives
     *
     * This callback is for acceptors created by ProCreateAcceptorEx().
     * User is responsible for sockId resource management
     */
    virtual void OnAccept(
        IProAcceptor*    acceptor,
        int64_t          sockId,     /* Socket ID */
        bool             unixSocket, /* Whether it's a Unix socket */
        const char*      localIp,    /* Local IP address. != NULL */
        const char*      remoteIp,   /* Remote IP address. != NULL */
        unsigned short   remotePort, /* Remote port number. > 0 */
        unsigned char    serviceId,  /* Requested service ID */
        unsigned char    serviceOpt, /* Requested service option */
        const PRO_NONCE* nonce       /* Session nonce */
        ) = 0;
};

/*
 * Service host callback target
 *
 * Users need to implement this interface
 */
class IProServiceHostObserver
{
public:

    virtual ~IProServiceHostObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a raw connection arrives
     *
     * This callback is for service hosts created by ProCreateServiceHost().
     * User is responsible for sockId resource management
     */
    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        int64_t          sockId,     /* Socket ID */
        bool             unixSocket, /* Whether it's a Unix socket */
        const char*      localIp,    /* Local IP address. != NULL */
        const char*      remoteIp,   /* Remote IP address. != NULL */
        unsigned short   remotePort  /* Remote port number. > 0 */
        ) = 0;

    /*
     * Called when an extended protocol connection arrives
     *
     * This callback is for service hosts created by ProCreateServiceHostEx().
     * User is responsible for sockId resource management
     */
    virtual void OnServiceAccept(
        IProServiceHost* serviceHost,
        int64_t          sockId,     /* Socket ID */
        bool             unixSocket, /* Whether it's a Unix socket */
        const char*      localIp,    /* Local IP address. != NULL */
        const char*      remoteIp,   /* Remote IP address. != NULL */
        unsigned short   remotePort, /* Remote port number. > 0 */
        unsigned char    serviceId,  /* Requested service ID */
        unsigned char    serviceOpt, /* Requested service option */
        const PRO_NONCE* nonce       /* Session nonce. != NULL */
        ) = 0;
};

/*
 * Service hub callback target
 *
 * Users need to implement this interface
 */
class IProServiceHubObserver
{
public:

    virtual ~IProServiceHubObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a service host comes online
     */
    virtual void OnServiceHostConnected(
        IProServiceHub* serviceHub,
        unsigned short  servicePort,  /* Service port number. > 0 */
        unsigned char   serviceId,    /* Service ID. > 0 */
        uint64_t        hostProcessId /* Service host process ID */
        ) = 0;

    /*
     * Called when a service host goes offline
     */
    virtual void OnServiceHostDisconnected(
        IProServiceHub* serviceHub,
        unsigned short  servicePort,   /* Service port number. > 0 */
        unsigned char   serviceId,     /* Service ID. > 0 */
        uint64_t        hostProcessId, /* Service host process ID */
        bool            timeout        /* Whether it's due to timeout */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Connector callback target
 *
 * Users need to implement this interface
 */
class IProConnectorObserver
{
public:

    virtual ~IProConnectorObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when a raw connection succeeds
     *
     * This callback is for connectors created by ProCreateConnector().
     * User is responsible for sockId resource management
     */
    virtual void OnConnectOk(
        IProConnector* connector,
        int64_t        sockId,     /* Socket ID */
        bool           unixSocket, /* Whether it's a Unix socket */
        const char*    remoteIp,   /* Remote IP address. != NULL */
        unsigned short remotePort  /* Remote port number. > 0 */
        ) = 0;

    /*
     * Called when a raw connection fails or times out
     *
     * This callback is for connectors created by ProCreateConnector()
     */
    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* Remote IP address. != NULL */
        unsigned short remotePort, /* Remote port number. > 0 */
        bool           timeout     /* Whether it's a connection timeout */
        ) = 0;

    /*
     * Called when an extended protocol connection succeeds
     *
     * This callback is for connectors created by ProCreateConnectorEx().
     * User is responsible for sockId resource management
     */
    virtual void OnConnectOk(
        IProConnector*   connector,
        int64_t          sockId,     /* Socket ID */
        bool             unixSocket, /* Whether it's a Unix socket */
        const char*      remoteIp,   /* Remote IP address. != NULL */
        unsigned short   remotePort, /* Remote port number. > 0 */
        unsigned char    serviceId,  /* Requested service ID */
        unsigned char    serviceOpt, /* Requested service option */
        const PRO_NONCE* nonce       /* Session nonce */
        ) = 0;

    /*
     * Called when an extended protocol connection fails or times out
     *
     * This callback is for connectors created by ProCreateConnectorEx()
     */
    virtual void OnConnectError(
        IProConnector* connector,
        const char*    remoteIp,   /* Remote IP address. != NULL */
        unsigned short remotePort, /* Remote port number. > 0 */
        unsigned char  serviceId,  /* Requested service ID */
        unsigned char  serviceOpt, /* Requested service option */
        bool           timeout     /* Whether it's a connection timeout */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * TCP handshaker callback target
 *
 * Users need to implement this interface
 */
class IProTcpHandshakerObserver
{
public:

    virtual ~IProTcpHandshakerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when handshake succeeds
     *
     * User is responsible for sockId resource management. After handshake completes,
     * upper layer should wrap sockId into IProTransport or release resources
     */
    virtual void OnHandshakeOk(
        IProTcpHandshaker* handshaker,
        int64_t            sockId,     /* Socket ID */
        bool               unixSocket, /* Whether it's a Unix socket */
        const void*        buf,        /* Handshake data. Can be NULL */
        size_t             size        /* Data length. Can be 0 */
        ) = 0;

    /*
     * Called when handshake fails or times out
     */
    virtual void OnHandshakeError(
        IProTcpHandshaker* handshaker,
        int                errorCode /* System error code. See "pro_util/pro_bsd_wrapper.h" */
        ) = 0;
};

/*
 * SSL handshaker callback target
 *
 * Users need to implement this interface
 */
class IProSslHandshakerObserver
{
public:

    virtual ~IProSslHandshakerObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when handshake succeeds
     *
     * User is responsible for (ctx, sockId) resource management. After handshake
     * completes, upper layer should wrap (ctx, sockId) into IProTransport
     * or release resources
     *
     * Call ProSslCtx_GetSuite() to get the negotiated cipher suite
     */
    virtual void OnHandshakeOk(
        IProSslHandshaker* handshaker,
        PRO_SSL_CTX*       ctx,        /* SSL context */
        int64_t            sockId,     /* Socket ID */
        bool               unixSocket, /* Whether it's a Unix socket */
        const void*        buf,        /* Handshake data. Can be NULL */
        size_t             size        /* Data length. Can be 0 */
        ) = 0;

    /*
     * Called when handshake fails or times out
     */
    virtual void OnHandshakeError(
        IProSslHandshaker* handshaker,
        int                errorCode, /* System error code. See "pro_util/pro_bsd_wrapper.h" */
        int                sslCode    /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Receive pool
 *
 * Must be used within the thread context of IProTransportObserver::OnRecv(),
 * otherwise unsafe
 *
 * For TCP/SSL transports: use circular receive pool.
 * Since TCP is stream-oriented, the number of received packets may not match
 * sent packets. Therefore, in OnRecv(), data should be consumed
 * as much as possible. If no new data arrives, the reactor will not report
 * remaining data again
 *
 * For UDP/MCAST transports: use linear receive pool.
 * To prevent EMSGSIZE error due to insufficient pool space, all data should
 * be consumed in OnRecv()
 *
 * When the reactor reports data readable in a socket, if the receive pool
 * is already <<FULL>>, the socket will be closed!!! This indicates
 * a logic problem between sender and receiver!!!
 */
class IProRecvPool
{
public:

    virtual ~IProRecvPool() {}

    /*
     * Query the amount of data in the receive pool
     */
    virtual size_t PeekDataSize() const = 0;

    /*
     * Read data from the receive pool
     */
    virtual void PeekData(
        void*  buf,
        size_t size
        ) const = 0;

    /*
     * Flush already read data
     *
     * Free space to accommodate new data
     */
    virtual void Flush(size_t size) = 0;

    /*
     * Query remaining free space in the receive pool
     */
    virtual size_t GetFreeSize() const = 0;
};

/*
 * Transport
 */
class IProTransport
{
public:

    virtual ~IProTransport() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Get transport type
     */
    virtual PRO_TRANS_TYPE GetType() const = 0;

    /*
     * Get cipher suite
     *
     * Only for PRO_TRANS_SSL type transports
     */
    virtual PRO_SSL_SUITE_ID GetSslSuite(char suiteName[64]) const = 0;

    /*
     * Get underlying socket ID
     *
     * Avoid directly operating the underlying socket unless necessary
     */
    virtual int64_t GetSockId() const = 0;

    /*
     * Get local IP address of the socket
     */
    virtual const char* GetLocalIp(char localIp[64]) const = 0;

    /*
     * Get local port number of the socket
     */
    virtual unsigned short GetLocalPort() const = 0;

    /*
     * Get remote IP address of the socket
     *
     * For TCP: return connected peer's IP address
     * For UDP: return default remote IP address
     */
    virtual const char* GetRemoteIp(char remoteIp[64]) const = 0;

    /*
     * Get remote port number of the socket
     *
     * For TCP: return connected peer's port number
     * For UDP: return default remote port number
     */
    virtual unsigned short GetRemotePort() const = 0;

    /*
     * Get receive pool
     *
     * See comments for IProRecvPool
     */
    virtual IProRecvPool* GetRecvPool() = 0;

    /*
     * Send data
     *
     * For TCP: Data is placed in send pool, remoteAddr parameter is ignored.
     *          actionId is a value assigned by upper layer to identify
     *          this send action, and will be returned in OnSend() callback
     * For UDP: Data is sent directly. If remoteAddr is invalid, the default
     *          remote address is used
     *
     * Return false if send is busy. Upper layer should buffer data
     * and wait for OnSend() callback
     */
    virtual bool SendData(
        const void*             buf,
        size_t                  size,
        uint64_t                actionId   = 0,   /* for TCP */
        const pbsd_sockaddr_in* remoteAddr = NULL /* for UDP */
        ) = 0;

    /*
     * Request an OnSend callback (for CProTcpTransport & CProSslTransport)
     *
     * OnSend() fires only when send capability is restored
     */
    virtual void RequestOnSend() = 0;

    /*
     * Suspend receive capability
     */
    virtual void SuspendRecv() = 0;

    /*
     * Resume receive capability
     */
    virtual void ResumeRecv() = 0;

    /*
     * Add additional multicast address (for CProMcastTransport only)
     */
    virtual bool AddMcastReceiver(const char* mcastIp) = 0;

    /*
     * Remove additional multicast address (for CProMcastTransport only)
     */
    virtual void RemoveMcastReceiver(const char* mcastIp) = 0;

    /*
     * Start heartbeat timer
     *
     * OnHeartbeat() will be called when heartbeat event occurs
     */
    virtual void StartHeartbeat() = 0;

    /*
     * Stop heartbeat timer
     */
    virtual void StopHeartbeat() = 0;

    /*
     * Treat UDP connection reset as error (for CProUdpTransport only)
     *
     * Default false. Once set, cannot be reversed
     */
    virtual void UdpConnResetAsError(const pbsd_sockaddr_in* remoteAddr = NULL) = 0;
};

/*
 * Transport callback target
 *
 * Users need to implement this interface
 */
class IProTransportObserver
{
public:

    virtual ~IProTransportObserver() {}

    virtual unsigned long AddRef() = 0;

    virtual unsigned long Release() = 0;

    /*
     * Called when data arrives in the socket receive buffer
     *
     * For TCP/SSL transports: use circular receive pool.
     * Since TCP is stream-oriented, the number of received packets may not match
     * sent packets. Therefore, in OnRecv(), data should be consumed
     * as much as possible. If no new data arrives, the reactor will not report
     * remaining data again
     *
     * For UDP/MCAST transports: use linear receive pool.
     * To prevent EMSGSIZE error due to insufficient pool space, all data should
     * be consumed in OnRecv()
     *
     * When the reactor reports data readable in a socket, if the receive pool
     * is already <<FULL>>, the socket will be closed!!! This indicates
     * a logic problem between sender and receiver!!!
     */
    virtual void OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        ) = 0;

    /*
     * Called when data is successfully placed in socket send buffer,
     * or after RequestOnSend() is called
     *
     * If triggered by RequestOnSend(), actionId is 0
     */
    virtual void OnSend(
        IProTransport* trans,
        uint64_t       actionId
        ) = 0;

    /*
     * Called when socket error occurs
     */
    virtual void OnClose(
        IProTransport* trans,
        int            errorCode, /* System error code. See "pro_util/pro_bsd_wrapper.h" */
        int            sslCode    /* SSL error code. See "mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * Called when heartbeat event occurs
     */
    virtual void OnHeartbeat(IProTransport* trans) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Function: Initialize network library
 *
 * Parameters: None
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProNetInit();

/*
 * Function: Get the version number of this library
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
PRO_NET_API
void
ProNetVersion(unsigned char* major,  /* = NULL */
              unsigned char* minor,  /* = NULL */
              unsigned char* patch); /* = NULL */

/*
 * Function: Create a reactor
 *
 * Parameters:
 * ioThreadCount : Number of threads for handling I/O events
 *
 * Return: Reactor object or NULL
 *
 * Note: None
 */
PRO_NET_API
IProReactor*
ProCreateReactor(unsigned int ioThreadCount);

/*
 * Function: Delete a reactor
 *
 * Parameters:
 * reactor : Reactor object
 *
 * Return: None
 *
 * Note: Reactor is an active object containing callback thread pool.
 *       This function blocks until all callback threads finish execution.
 *       Upper layer must ensure deletion is initiated by a thread
 *       outside the callback thread pool and outside any locks
 *       to prevent deadlock.
 */
PRO_NET_API
void
ProDeleteReactor(IProReactor* reactor);

/*
 * Function: Create a raw acceptor
 *
 * Parameters:
 * observer  : Callback target
 * reactor   : Reactor
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort : Local port number to bind. If 0, system assigns randomly
 *
 * Return: Acceptor object or NULL
 *
 * Note: Use ProGetAcceptorPort() to get actual port number
 */
PRO_NET_API
IProAcceptor*
ProCreateAcceptor(IProAcceptorObserver* observer,
                  IProReactor*          reactor,
                  const char*           localIp   = NULL,
                  unsigned short        localPort = 0);

/*
 * Function: Create an extended protocol acceptor
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * localIp          : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort        : Local port number to bind. If 0, system assigns randomly
 * timeoutInSeconds : Handshake timeout. Default 10 seconds
 *
 * Return: Acceptor object or NULL
 *
 * Note: Use ProGetAcceptorPort() to get actual port number
 *
 *       During extended protocol handshake:
 *       - serviceId is used by server to identify client early in handshake
 *       - Server sends nonce to client
 *       - Client sends (serviceId, serviceOpt) to server
 *       - Server dispatches connection to corresponding handler/service process
 */
PRO_NET_API
IProAcceptor*
ProCreateAcceptorEx(IProAcceptorObserver* observer,
                    IProReactor*          reactor,
                    const char*           localIp          = NULL,
                    unsigned short        localPort        = 0,
                    unsigned int          timeoutInSeconds = 0);

/*
 * Function: Get acceptor's listening port number
 *
 * Parameters:
 * acceptor : Acceptor object
 *
 * Return: Port number
 *
 * Note: Mainly used to get randomly assigned port number
 */
PRO_NET_API
unsigned short
ProGetAcceptorPort(IProAcceptor* acceptor);

/*
 * Function: Delete an acceptor
 *
 * Parameters:
 * acceptor : Acceptor object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteAcceptor(IProAcceptor* acceptor);

/*
 * Function: Create a raw connector
 *
 * Parameters:
 * enableUnixSocket : Whether to allow Unix sockets (Linux 127.0.0.1 only)
 * observer         : Callback target
 * reactor          : Reactor
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localBindIp      : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Connection timeout. Default 20 seconds
 *
 * Return: Connector object or NULL
 *
 * Note: None
 */
PRO_NET_API
IProConnector*
ProCreateConnector(bool                   enableUnixSocket,
                   IProConnectorObserver* observer,
                   IProReactor*           reactor,
                   const char*            remoteIp,
                   unsigned short         remotePort,
                   const char*            localBindIp      = NULL,
                   unsigned int           timeoutInSeconds = 0);

/*
 * Function: Create an extended protocol connector
 *
 * Parameters:
 * enableUnixSocket : Whether to allow Unix sockets (Linux 127.0.0.1 only)
 * serviceId        : Service ID
 * serviceOpt       : Service option
 * observer         : Callback target
 * reactor          : Reactor
 * remoteIp         : Remote IP address or domain name
 * remotePort       : Remote port number
 * localBindIp      : Local IP address to bind. If NULL, system uses 0.0.0.0
 * timeoutInSeconds : Connection timeout. Default 20 seconds
 *
 * Return: Connector object or NULL
 *
 * Note: During extended protocol handshake:
 *       - serviceId is used by server to identify client early in handshake
 *       - Server sends nonce to client
 *       - Client sends (serviceId, serviceOpt) to server
 *       - Server dispatches connection to corresponding handler/service process
 */
PRO_NET_API
IProConnector*
ProCreateConnectorEx(bool                   enableUnixSocket,
                     unsigned char          serviceId,
                     unsigned char          serviceOpt,
                     IProConnectorObserver* observer,
                     IProReactor*           reactor,
                     const char*            remoteIp,
                     unsigned short         remotePort,
                     const char*            localBindIp      = NULL,
                     unsigned int           timeoutInSeconds = 0);

/*
 * Function: Delete a connector
 *
 * Parameters:
 * connector : Connector object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteConnector(IProConnector* connector);

/*
 * Function: Create a TCP handshaker
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * sockId           : Socket ID (from OnAccept() or OnConnectOk())
 * unixSocket       : Whether it's a Unix socket
 * sendData         : Data to send
 * sendDataSize     : Length of data to send
 * recvDataSize     : Length of data to receive
 * recvFirst        : Receive first
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 *
 * Return: Handshaker object or NULL
 *
 * Note: If recvFirst is true, handshake data is received before sending
 */
PRO_NET_API
IProTcpHandshaker*
ProCreateTcpHandshaker(IProTcpHandshakerObserver* observer,
                       IProReactor*               reactor,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned int               timeoutInSeconds = 0);

/*
 * Function: Delete a TCP handshaker
 *
 * Parameters:
 * handshaker : Handshaker object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteTcpHandshaker(IProTcpHandshaker* handshaker);

/*
 * Function: Create an SSL handshaker
 *
 * Parameters:
 * observer         : Callback target
 * reactor          : Reactor
 * ctx              : SSL context (created by ProSslCtx_Creates() or ProSslCtx_Createc())
 * sockId           : Socket ID (from OnAccept() or OnConnectOk())
 * unixSocket       : Whether it's a Unix socket
 * sendData         : Data to send
 * sendDataSize     : Length of data to send
 * recvDataSize     : Length of data to receive
 * recvFirst        : Receive first
 * timeoutInSeconds : Handshake timeout. Default 20 seconds
 *
 * Return: Handshaker object or NULL
 *
 * Note: If ctx hasn't completed SSL/TLS handshake, handshaker first performs
 *       SSL/TLS handshake, then performs higher-level handshake for user data.
 *       If recvFirst is true, higher-level handshake data is received before sending
 */
PRO_NET_API
IProSslHandshaker*
ProCreateSslHandshaker(IProSslHandshakerObserver* observer,
                       IProReactor*               reactor,
                       PRO_SSL_CTX*               ctx,
                       int64_t                    sockId,
                       bool                       unixSocket,
                       const void*                sendData         = NULL,
                       size_t                     sendDataSize     = 0,
                       size_t                     recvDataSize     = 0,
                       bool                       recvFirst        = false,
                       unsigned int               timeoutInSeconds = 0);

/*
 * Function: Delete an SSL handshaker
 *
 * Parameters:
 * handshaker : Handshaker object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteSslHandshaker(IProSslHandshaker* handshaker);

/*
 * Function: Create a TCP transport
 *
 * Parameters:
 * observer        : Callback target
 * reactor         : Reactor
 * sockId          : Socket ID (from OnAccept() or OnConnectOk())
 * unixSocket      : Whether it's a Unix socket
 * sockBufSizeRecv : Socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Receive pool size in bytes. Default (1024 * 65)
 * suspendRecv     : Whether to suspend receive capability
 *
 * Return: Transport object or NULL
 *
 * Note: suspendRecv is used for scenarios requiring precise control
 */
PRO_NET_API
IProTransport*
ProCreateTcpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0,
                      bool                   suspendRecv     = false);

/*
 * Function: Create a UDP transport
 *
 * Parameters:
 * observer        : Callback target
 * reactor         : Reactor
 * bindToLocal     : Whether to bind socket to local address
 * localIp         : Local IP address to bind. If "", system uses 0.0.0.0
 * localPort       : Local port number to bind. If 0, system assigns randomly
 * sockBufSizeRecv : Socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Receive pool size in bytes. Default (1024 * 65)
 * remoteIp        : Default remote IP address or domain name
 * remotePort      : Default remote port number
 *
 * Return: Transport object or NULL
 *
 * Note: Use IProTransport::GetLocalPort() to get actual port number
 */
PRO_NET_API
IProTransport*
ProCreateUdpTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      bool                   bindToLocal       = false,
                      const char*            localIp           = NULL,
                      unsigned short         localPort         = 0,
                      size_t                 sockBufSizeRecv   = 0,
                      size_t                 sockBufSizeSend   = 0,
                      size_t                 recvPoolSize      = 0,
                      const char*            defaultRemoteIp   = NULL,
                      unsigned short         defaultRemotePort = 0);

/*
 * Function: Create a multicast transport
 *
 * Parameters:
 * observer        : Callback target
 * reactor         : Reactor
 * mcastIp         : Multicast IP address to bind
 * mcastPort       : Multicast port number to bind. If 0, system assigns randomly
 * localBindIp     : Local IP address to bind. If NULL, system uses 0.0.0.0
 * sockBufSizeRecv : Socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Receive pool size in bytes. Default (1024 * 65)
 *
 * Return: Transport object or NULL
 *
 * Note: Valid multicast addresses: [224.0.0.0 ~ 239.255.255.255],
 *       Recommended: [224.0.1.0 ~ 238.255.255.255],
 *       RFC-1112(IGMPv1), RFC-2236(IGMPv2), RFC-3376(IGMPv3)
 *
 *       Use IProTransport::GetLocalPort() to get actual port number
 */
PRO_NET_API
IProTransport*
ProCreateMcastTransport(IProTransportObserver* observer,
                        IProReactor*           reactor,
                        const char*            mcastIp,
                        unsigned short         mcastPort       = 0,
                        const char*            localBindIp     = NULL,
                        size_t                 sockBufSizeRecv = 0,
                        size_t                 sockBufSizeSend = 0,
                        size_t                 recvPoolSize    = 0);

/*
 * Function: Create an SSL transport
 *
 * Parameters:
 * observer        : Callback target
 * reactor         : Reactor
 * ctx             : SSL context (from IProSslHandshaker handshake result)
 * sockId          : Socket ID (from OnAccept() or OnConnectOk())
 * unixSocket      : Whether it's a Unix socket
 * sockBufSizeRecv : Socket system receive buffer size in bytes. Default auto
 * sockBufSizeSend : Socket system send buffer size in bytes. Default auto
 * recvPoolSize    : Receive pool size in bytes. Default (1024 * 65)
 * suspendRecv     : Whether to suspend receive capability
 *
 * Return: Transport object or NULL
 *
 * Note: This is the last safe moment to access ctx!!!
 *       Afterwards, ctx will be managed by IProTransport
 *
 *       suspendRecv is used for scenarios requiring precise control
 */
PRO_NET_API
IProTransport*
ProCreateSslTransport(IProTransportObserver* observer,
                      IProReactor*           reactor,
                      PRO_SSL_CTX*           ctx,
                      int64_t                sockId,
                      bool                   unixSocket,
                      size_t                 sockBufSizeRecv = 0,
                      size_t                 sockBufSizeSend = 0,
                      size_t                 recvPoolSize    = 0,
                      bool                   suspendRecv     = false);

/*
 * Function: Delete a transport
 *
 * Parameters:
 * trans : Transport object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteTransport(IProTransport* trans);

/*
 * Function: Create a raw service hub
 *
 * Parameters:
 * observer          : Callback target
 * reactor           : Reactor
 * servicePort       : Service port number working in raw mode
 * enableLoadBalance : Whether to enable load balancing for this port.
 *                     If enabled, supports multiple service hosts
 *
 * Return: Service hub object or NULL
 *
 * Note: Service hub can dispatch connection requests to one or more service hosts,
 *       allowing cross-process boundaries (WinCE cannot cross processes).
 *       Service hub and service host extend acceptor functionality to different locations
 *
 *       For raw service hub, loopback address (127.0.0.1) is reserved for IPC channel
 *       and cannot serve external requests; extended protocol service hub has no such
 *       restriction
 */
PRO_NET_API
IProServiceHub*
ProCreateServiceHub(IProServiceHubObserver* observer,
                    IProReactor*            reactor,
                    unsigned short          servicePort,
                    bool                    enableLoadBalance = false);

/*
 * Function: Create an extended protocol service hub
 *
 * Parameters:
 * observer          : Callback target
 * reactor           : Reactor
 * servicePort       : Service port number working in extended protocol mode
 * enableLoadBalance : Whether to enable load balancing for this port.
 *                     If enabled, supports multiple service hosts
 * timeoutInSeconds  : Handshake timeout. Default 10 seconds
 *
 * Return: Service hub object or NULL
 *
 * Note: Service hub can dispatch connection requests to one or more service hosts,
 *       allowing cross-process boundaries (WinCE cannot cross processes).
 *       Service hub and service host extend acceptor functionality to different locations
 *
 *       For raw service hub, loopback address (127.0.0.1) is reserved for IPC channel
 *       and cannot serve external requests; extended protocol service hub has no such
 *       restriction
 */
PRO_NET_API
IProServiceHub*
ProCreateServiceHubEx(IProServiceHubObserver* observer,
                      IProReactor*            reactor,
                      unsigned short          servicePort,
                      bool                    enableLoadBalance = false,
                      unsigned int            timeoutInSeconds  = 0);

/*
 * Function: Delete a service hub
 *
 * Parameters:
 * hub : Service hub object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteServiceHub(IProServiceHub* hub);

/*
 * Function: Create a raw service host
 *
 * Parameters:
 * observer    : Callback target
 * reactor     : Reactor
 * servicePort : Service port number
 *
 * Return: Service host object or NULL
 *
 * Note: Service host can be seen as a service acceptor
 */
PRO_NET_API
IProServiceHost*
ProCreateServiceHost(IProServiceHostObserver* observer,
                     IProReactor*             reactor,
                     unsigned short           servicePort);

/*
 * Function: Create an extended protocol service host
 *
 * Parameters:
 * observer    : Callback target
 * reactor     : Reactor
 * servicePort : Service port number
 * serviceId   : Service ID. 0 is invalid
 *
 * Return: Service host object or NULL
 *
 * Note: Service host can be seen as a service acceptor
 */
PRO_NET_API
IProServiceHost*
ProCreateServiceHostEx(IProServiceHostObserver* observer,
                       IProReactor*             reactor,
                       unsigned short           servicePort,
                       unsigned char            serviceId);

/*
 * Function: Delete a service host
 *
 * Parameters:
 * host : Service host object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProDeleteServiceHost(IProServiceHost* host);

/*
 * Function: Open a TCP socket
 *
 * Parameters:
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort : Local port number to bind
 *
 * Return: Socket ID or -1
 *
 * Note: Mainly used to reserve RTCP ports
 */
PRO_NET_API
int64_t
ProOpenTcpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * Function: Open a UDP socket
 *
 * Parameters:
 * localIp   : Local IP address to bind. If NULL, system uses 0.0.0.0
 * localPort : Local port number to bind
 *
 * Return: Socket ID or -1
 *
 * Note: Mainly used to reserve RTCP ports
 */
PRO_NET_API
int64_t
ProOpenUdpSockId(const char*    localIp, /* = NULL */
                 unsigned short localPort);

/*
 * Function: Close a socket
 *
 * Parameters:
 * sockId : Socket ID
 * linger : Whether to linger close
 *
 * Return: None
 *
 * Note: OnAccept() or OnConnectOk() generates sockets. When generated socket is not
 *       wrapped into IProTransport, use this function to release socket resources
 */
PRO_NET_API
void
ProCloseSockId(int64_t sockId,
               bool    linger = false);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_NET_H____ */
