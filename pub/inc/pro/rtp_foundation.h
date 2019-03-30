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
 *        |                        ProRtp                        |
 *        |______________________________________________________|
 *        |          |                              |            |
 *        |          |             ProNet           |            |
 *        |          |______________________________|            |
 *        |                    |                                 |
 *        |                    |             ProUtil             |
 *        |      MbedTLS       |_________________________________|
 *        |                              |                       |
 *        |                              |       ProShared       |
 *        |______________________________|_______________________|
 *                     Fig.1 module hierarchy diagram
 */

/*         ______________________________________________________
 *        |                                                      |
 *        |                      msg server                      |
 *        |______________________________________________________|
 *                 |             |        |              |
 *                 | ...         |        |          ... |
 *         ________|_______      |        |      ________|_______
 *        |                |     |        |     |                |
 *        |     msg c2s    |     |        |     |     msg c2s    |
 *        |________________|     |        |     |________________|
 *            |        |         |        |         |        |
 *            |  ...   |         |  ...   |         |  ...   |
 *         ___|___  ___|___   ___|___  ___|___   ___|___  ___|___
 *        |  msg  ||  msg  | |  msg  ||  msg  | |  msg  ||  msg  |
 *        | client|| client| | client|| client| | client|| client|
 *        |_______||_______| |_______||_______| |_______||_______|
 *                 Fig.2 structure diagram of msg system
 */

#if !defined(____RTP_FOUNDATION_H____)
#define ____RTP_FOUNDATION_H____

#include "rtp_framework.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

class IRtpBucket;

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_UDPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_UDPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_TCPCLIENT
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_TCPSERVER
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_UDPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_UDPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         localIp[64];        /* = "" */
    unsigned short               localPort;          /* = 0 */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_TCPCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];       /* = "" */
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_TCPSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * sslConfigָ���Ķ�������ڻỰ������������һֱ��Ч;
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_SSLCLIENT_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    const PRO_SSL_CLIENT_CONFIG* sslConfig;
    char                         sslServiceName[64]; /* = "" */
    char                         remoteIp[64];
    unsigned short               remotePort;
    char                         password[64];       /* = "" */
    char                         localIp[64];        /* = "" */
    unsigned long                timeoutInSeconds;   /* = 0 */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_SSLSERVER_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    PRO_SSL_CTX*                 sslCtx;
    PRO_INT64                    sockId;
    bool                         unixSocket;
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_MCAST
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;          /* = 0 */
    char                         localIp[64];        /* = "" */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 *
 * ���bucketΪNULL,ϵͳ���Զ�����һ��bucket
 */
struct RTP_INIT_MCAST_EX
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
    char                         mcastIp[64];
    unsigned short               mcastPort;          /* = 0 */
    char                         localIp[64];        /* = "" */
    IRtpBucket*                  bucket;             /* = NULL */
};

/*
 * �Ự��ʼ������
 */
struct RTP_INIT_COMMON
{
    IRtpSessionObserver*         observer;
    IProReactor*                 reactor;
};

/*
 * �Ự��ʼ����������
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

/*
 * ��Ϣ�û���. 1-2-1, 1-2-2, 1-3-1, 1-3-2, ...; 2-1-1, 2-1-2, 2-2-1, 2-2-2, ...
 *
 * classId : 8bits.���ֶ����ڱ�ʶ�û����,�Ա���Ӧ�ó���������.
 *           0��Ч, 1Ӧ�÷������ڵ�, 2~255Ӧ�ÿͻ��˽ڵ�
 *
 * userId  : 40bits.���ֶ����ڱ�ʶ�û�id(��绰����),����Ϣ��������������.
 *           0��̬����,��ΧΪ[0xF000000000 ~ 0xFFFFFFFFFF];
 *           ����̬����,��ΧΪ[1 ~ 0xEFFFFFFFFF]
 *
 * instId  : 16bits.���ֶ����ڱ�ʶ�û�ʵ��id(��绰�ֻ���),����Ϣ����������
 *           �����.��Ч��ΧΪ[0 ~ 65535]
 *
 * ˵��    : classId-userId ֮ 1-1 ����,���ڱ�ʶ��Ϣ����������(root)
 */
struct RTP_MSG_USER
{
    RTP_MSG_USER()
    {
        Zero();
    }

    RTP_MSG_USER(
        unsigned char classId,
        PRO_UINT64    userId,
        PRO_UINT16    instId
        )
    {
        this->classId = classId;
        UserId(userId);
        this->instId  = instId;
    }

    void UserId(PRO_UINT64 userId)
    {
        userId5 = (unsigned char)userId;
        userId >>= 8;
        userId4 = (unsigned char)userId;
        userId >>= 8;
        userId3 = (unsigned char)userId;
        userId >>= 8;
        userId2 = (unsigned char)userId;
        userId >>= 8;
        userId1 = (unsigned char)userId;
    }

    /*
     * userId = userId1.userId2.userId3.userId4.userId5
     */
    PRO_UINT64 UserId() const
    {
        PRO_UINT64 userId = userId1;
        userId <<= 8;
        userId |= userId2;
        userId <<= 8;
        userId |= userId3;
        userId <<= 8;
        userId |= userId4;
        userId <<= 8;
        userId |= userId5;

        return (userId);
    }

    void Zero()
    {
        classId = 0;
        userId1 = 0;
        userId2 = 0;
        userId3 = 0;
        userId4 = 0;
        userId5 = 0;
        instId  = 0;
    }

    bool IsRoot() const
    {
        return (classId == 1 && UserId() == 1);
    }

    bool operator==(const RTP_MSG_USER& user) const
    {
        return (
            classId == user.classId &&
            userId1 == user.userId1 &&
            userId2 == user.userId2 &&
            userId3 == user.userId3 &&
            userId4 == user.userId4 &&
            userId5 == user.userId5 &&
            instId  == user.instId
            );
    }

    bool operator!=(const RTP_MSG_USER& user) const
    {
        return (!(*this == user));
    }

    bool operator<(const RTP_MSG_USER& user) const
    {
        if (classId < user.classId)
        {
            return (true);
        }
        if (classId > user.classId)
        {
            return (false);
        }

        if (userId1 < user.userId1)
        {
            return (true);
        }
        if (userId1 > user.userId1)
        {
            return (false);
        }

        if (userId2 < user.userId2)
        {
            return (true);
        }
        if (userId2 > user.userId2)
        {
            return (false);
        }

        if (userId3 < user.userId3)
        {
            return (true);
        }
        if (userId3 > user.userId3)
        {
            return (false);
        }

        if (userId4 < user.userId4)
        {
            return (true);
        }
        if (userId4 > user.userId4)
        {
            return (false);
        }

        if (userId5 < user.userId5)
        {
            return (true);
        }
        if (userId5 > user.userId5)
        {
            return (false);
        }

        if (instId < user.instId)
        {
            return (true);
        }

        return (false);
    }

    unsigned char classId;
    unsigned char userId1;
    unsigned char userId2;
    unsigned char userId3;
    unsigned char userId4;
    unsigned char userId5;
    PRO_UINT16    instId;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * rtpͰ
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

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ��Ϣ�ͻ���
 */
class IRtpMsgClient
{
public:

    /*
     * ��ȡ�û���
     */
    virtual void PRO_CALLTYPE GetUser(RTP_MSG_USER* user) const = 0;

    /*
     * ������Ϣ
     *
     * ϵͳ�ڲ�����Ϣ���Ͷ���
     */
    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,         /* ��Ϣ���� */
        PRO_UINT16          size,        /* ���(1024 * 60)�ֽ� */
        PRO_UINT16          charset,     /* �û��Զ������Ϣ�ַ������� */
        const RTP_MSG_USER* dstUsers,    /* ��Ϣ������ */
        unsigned char       dstUserCount /* ���255��Ŀ�� */
        ) = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * ��Ϣ�ͻ��˻ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IRtpMsgClientObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * ��¼�ɹ�ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnOkMsg(
        IRtpMsgClient*      msgClient,
        const RTP_MSG_USER* myUser,
        const char*         myPublicIp
        ) = 0;

    /*
     * ��Ϣ����ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgClient*      msgClient,
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        ) = 0;

    /*
     * ��������ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnCloseMsg(
        IRtpMsgClient* msgClient,
        long           errorCode,   /* ϵͳ������ */
        long           sslCode,     /* ssl������.�μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool           tcpConnected /* tcp�����Ƿ��Ѿ����� */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ��Ϣ������
 */
class IRtpMsgServer
{
public:

    /*
     * ��ȡ�û���
     */
    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* baseUserCount,    /* = NULL */
        unsigned long* subUserCount      /* = NULL */
        ) const = 0;

    /*
     * �߳��û�
     */
    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * ������Ϣ
     *
     * ϵͳ�ڲ�����Ϣ���Ͷ���
     */
    virtual bool PRO_CALLTYPE SendMsg(
        const void*         buf,         /* ��Ϣ���� */
        PRO_UINT16          size,        /* ���(1024 * 60)�ֽ� */
        PRO_UINT16          charset,     /* �û��Զ������Ϣ�ַ������� */
        const RTP_MSG_USER* dstUsers,    /* ��Ϣ������ */
        unsigned char       dstUserCount /* ���255��Ŀ�� */
        ) = 0;

    /*
     * ����server->c2s��·���ͺ���.Ĭ��(1024 * 1024 * 8)�ֽ�
     */
    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes) = 0;

    /*
     * ��ȡserver->c2s��·���ͺ���.Ĭ��(1024 * 1024 * 8)�ֽ�
     */
    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * ��Ϣ�������ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IRtpMsgServerObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * �û������¼ʱ,�ú��������ص�
     *
     * �ϲ�Ӧ�ø����û���,�ҵ�ƥ����û�����,Ȼ�����CheckRtpServiceData(...)
     * ����У��
     *
     * ����ֵ��ʾ�Ƿ�������û���¼
     */
    virtual bool PRO_CALLTYPE OnCheckUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* �û������ĵ�¼�û��� */
        const char*         userPublicIp, /* �û���ip��ַ */
        const RTP_MSG_USER* c2sUser,      /* �����ĸ�c2s����.������NULL */
        const char          hash[32],     /* �û������Ŀ���hashֵ */
        PRO_UINT64          nonce,        /* �Ự�����.����CheckRtpServiceData(...)У�����hashֵ */
        PRO_UINT64*         userId,       /* �ϲ�������ɵ��û�id */
        PRO_UINT16*         instId,       /* �ϲ�������ɵ�ʵ��id */
        PRO_INT64*          appData,      /* �ϲ����õı�ʶ����.������OnOkUser(...)������� */
        bool*               isC2s         /* �ϲ����õ��Ƿ�ýڵ�Ϊc2s */
        ) = 0;

    /*
     * �û���¼�ɹ�ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnOkUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,         /* �ϲ�������ɵ��û��� */
        const char*         userPublicIp, /* �û���ip��ַ */
        const RTP_MSG_USER* c2sUser,      /* �����ĸ�c2s����.������NULL */
        PRO_INT64           appData       /* OnCheckUser(...)ʱ�ϲ����õı�ʶ���� */
        ) = 0;

    /*
     * �û���������ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnCloseUser(
        IRtpMsgServer*      msgServer,
        const RTP_MSG_USER* user,
        long                errorCode,    /* ϵͳ������ */
        long                sslCode       /* ssl������.�μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;

    /*
     * ��Ϣ����ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnRecvMsg(
        IRtpMsgServer*      msgServer,
        const void*         buf,
        PRO_UINT16          size,
        PRO_UINT16          charset,
        const RTP_MSG_USER* srcUser
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ��Ϣc2s
 *
 * c2sλ��client��server֮��,�����Է�ɢserver�ĸ���,����������server��λ��.
 * ����client����,c2s��͸����,client�޷����������ӵ���server����c2s
 */
class IRtpMsgC2s
{
public:

    /*
     * ��ȡc2s�û���
     */
    virtual void PRO_CALLTYPE GetC2sUser(RTP_MSG_USER* c2sUser) const = 0;

    /*
     * ��ȡ�û���
     */
    virtual void PRO_CALLTYPE GetUserCount(
        unsigned long* pendingUserCount, /* = NULL */
        unsigned long* userCount         /* = NULL */
        ) const = 0;

    /*
     * �߳��û�
     */
    virtual void PRO_CALLTYPE KickoutUser(const RTP_MSG_USER* user) = 0;

    /*
     * ����c2s->server��·���ͺ���.Ĭ��(1024 * 1024 * 8)�ֽ�
     */
    virtual void PRO_CALLTYPE SetOutputRedline(unsigned long redlineBytes) = 0;

    /*
     * ��ȡc2s->server��·���ͺ���.Ĭ��(1024 * 1024 * 8)�ֽ�
     */
    virtual unsigned long PRO_CALLTYPE GetOutputRedline() const = 0;

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;
};

/*
 * ��Ϣc2s�ص�Ŀ��
 *
 * ʹ������Ҫʵ�ָýӿ�
 */
class IRtpMsgC2sObserver
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef() = 0;

    virtual unsigned long PRO_CALLTYPE Release() = 0;

    /*
     * c2s��¼�ɹ�ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnOkC2s(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* c2sUser,
        const char*         c2sPublicIp
        ) = 0;

    /*
     * c2s��������ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnCloseC2s(
        IRtpMsgC2s* msgC2s,
        long        errorCode,         /* ϵͳ������ */
        long        sslCode,           /* ssl������.�μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        bool        tcpConnected       /* tcp�����Ƿ��Ѿ����� */
        ) = 0;

    /*
     * �û���¼�ɹ�ʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnOkUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        const char*         userPublicIp
        ) = 0;

    /*
     * �û���������ʱʱ,�ú��������ص�
     */
    virtual void PRO_CALLTYPE OnCloseUser(
        IRtpMsgC2s*         msgC2s,
        const RTP_MSG_USER* user,
        long                errorCode, /* ϵͳ������ */
        long                sslCode    /* ssl������.�μ�"mbedtls/error.h, ssl.h, x509.h, ..." */
        ) = 0;
};

/////////////////////////////////////////////////////////////////////////////
////

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
 * ����: ����һ����Ϣ�ͻ���
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * mmType           : ��Ϣý������. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl����. NULL��ʾ���Ĵ���
 * sslServiceName   : ssl������.�����Ч,�������֤�����֤��
 * remoteIp         : ��������ip��ַ������
 * remotePort       : �������Ķ˿ں�
 * user             : �û���
 * password         : �û�����
 * localIp          : Ҫ�󶨵ı���ip��ַ.���ΪNULL,ϵͳ��ʹ��0.0.0.0
 * timeoutInSeconds : ���ֳ�ʱ.Ĭ��20��
 *
 * ����ֵ: ��Ϣ�ͻ��˶����NULL
 *
 * ˵��: sslConfigָ���Ķ����������Ϣ�ͻ��˵�����������һֱ��Ч
 */
PRO_RTP_API
IRtpMsgClient*
PRO_CALLTYPE
CreateRtpMsgClient(IRtpMsgClientObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_CLIENT_CONFIG* sslConfig,         /* = NULL */
                   const char*                  sslServiceName,    /* = NULL */
                   const char*                  remoteIp,
                   unsigned short               remotePort,
                   const RTP_MSG_USER*          user,
                   const char*                  password,          /* = NULL */
                   const char*                  localIp,           /* = NULL */
                   unsigned long                timeoutInSeconds); /* = 0 */

/*
 * ����: ɾ��һ����Ϣ�ͻ���
 *
 * ����:
 * msgClient : ��Ϣ�ͻ��˶���
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpMsgClient(IRtpMsgClient* msgClient);

/*
 * ����: ����һ����Ϣ������
 *
 * ����:
 * observer         : �ص�Ŀ��
 * reactor          : ��Ӧ��
 * mmType           : ��Ϣý������. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * sslConfig        : ssl����. NULL��ʾ���Ĵ���
 * sslForced        : �Ƿ�ǿ��ʹ��ssl����. sslConfigΪNULLʱ�ò�������
 * serviceHubPort   : ����hub�����Ķ˿ں�
 * timeoutInSeconds : ���ֳ�ʱ.Ĭ��20��
 *
 * ����ֵ: ��Ϣ�����������NULL
 *
 * ˵��: sslConfigָ���Ķ����������Ϣ������������������һֱ��Ч
 */
PRO_RTP_API
IRtpMsgServer*
PRO_CALLTYPE
CreateRtpMsgServer(IRtpMsgServerObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_SERVER_CONFIG* sslConfig,         /* = NULL */
                   bool                         sslForced,         /* = false */
                   unsigned short               serviceHubPort,
                   unsigned long                timeoutInSeconds); /* = 0 */

/*
 * ����: ɾ��һ����Ϣ������
 *
 * ����:
 * msgServer : ��Ϣ����������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpMsgServer(IRtpMsgServer* msgServer);

/*
 * ����: ����һ����Ϣc2s
 *
 * ����:
 * observer               : �ص�Ŀ��
 * reactor                : ��Ӧ��
 * mmType                 : ��Ϣý������. [RTP_MMT_MSG_MIN ~ RTP_MMT_MSG_MAX]
 * uplinkSslConfig        : ������ssl����. NULL��ʾc2s<->server֮�����Ĵ���
 * uplinkSslServiceName   : ������ssl������.�����Ч,�������֤�����֤��
 * uplinkIp               : ��������ip��ַ������
 * uplinkPort             : �������Ķ˿ں�
 * uplinkUser             : c2s���û���
 * uplinkPassword         : c2s���û�����
 * uplinkLocalIp          : ����ʱҪ�󶨵ı���ip��ַ.���ΪNULL,ϵͳ��ʹ��0.0.0.0
 * uplinkTimeoutInSeconds : ���������ֳ�ʱ.Ĭ��20��
 * localSslConfig         : ���˵�ssl����. NULL��ʾ���Ĵ���
 * localSslForced         : �����Ƿ�ǿ��ʹ��ssl����. localSslConfigΪNULLʱ�ò�������
 * localServiceHubPort    : ���˵ķ���hub�����Ķ˿ں�
 * localTimeoutInSeconds  : ���˵����ֳ�ʱ.Ĭ��20��
 *
 * ����ֵ: ��Ϣc2s�����NULL
 *
 * ˵��: localSslConfigָ���Ķ����������Ϣc2s������������һֱ��Ч
 */
PRO_RTP_API
IRtpMsgC2s*
PRO_CALLTYPE
CreateRtpMsgC2s(IRtpMsgC2sObserver*          observer,
                IProReactor*                 reactor,
                RTP_MM_TYPE                  mmType,
                const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,        /* = NULL */
                const char*                  uplinkSslServiceName,   /* = NULL */
                const char*                  uplinkIp,
                unsigned short               uplinkPort,
                const RTP_MSG_USER*          uplinkUser,
                const char*                  uplinkPassword,
                const char*                  uplinkLocalIp,          /* = NULL */
                unsigned long                uplinkTimeoutInSeconds, /* = 0 */
                const PRO_SSL_SERVER_CONFIG* localSslConfig,         /* = NULL */
                bool                         localSslForced,         /* = false */
                unsigned short               localServiceHubPort,
                unsigned long                localTimeoutInSeconds); /* = 0 */

/*
 * ����: ɾ��һ����Ϣc2s
 *
 * ����:
 * msgC2s : ��Ϣc2s����
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpMsgC2s(IRtpMsgC2s* msgC2s);

/*
 * ����: ����һ���ڲ�ʵ�ֵĻ������͵�rtpͰ
 *
 * ����: ��
 *
 * ����ֵ: rtpͰ�����NULL
 *
 * ˵��: rtpͰ���̰߳�ȫ���ɵ����߱�֤
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpBaseBucket();

/*
 * ����: ����һ���ڲ�ʵ�ֵ���Ƶ���͵�rtpͰ
 *
 * ����: ��
 *
 * ����ֵ: rtpͰ�����NULL
 *
 * ˵��: rtpͰ���̰߳�ȫ���ɵ����߱�֤
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpAudioBucket();

/*
 * ����: ����һ���ڲ�ʵ�ֵ���Ƶ���͵�rtpͰ
 *
 * ����: ��
 *
 * ����ֵ: rtpͰ�����NULL
 *
 * ˵��: rtpͰ���̰߳�ȫ���ɵ����߱�֤
 */
PRO_RTP_API
IRtpBucket*
PRO_CALLTYPE
CreateRtpVideoBucket();

/*
 * ����: ��RTP_MSG_USER�ṹת��Ϊ"cid-uid-iid"��ʽ�Ĵ�
 *
 * ����:
 * user     : ��������
 * idString : ������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
RtpMsgUser2String(const RTP_MSG_USER* user,
                  char                idString[64]);

/*
 * ����: ��"cid-uid"��"cid-uid-iid"��ʽ�Ĵ�ת��ΪRTP_MSG_USER�ṹ
 *
 * ����:
 * idString : ��������
 * user     : ������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_RTP_API
void
PRO_CALLTYPE
RtpMsgString2User(const char*   idString,
                  RTP_MSG_USER* user);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____RTP_FOUNDATION_H____ */
