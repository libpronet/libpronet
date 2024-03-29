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

#include "rtp_msg.h"
#include "rtp_base.h"
#include "rtp_msg_c2s.h"
#include "rtp_msg_client.h"
#include "rtp_msg_server.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

PRO_RTP_API
IRtpMsgClient*
CreateRtpMsgClient(IRtpMsgClientObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_CLIENT_CONFIG* sslConfig,        /* = NULL */
                   const char*                  sslSni,           /* = NULL */
                   const char*                  remoteIp,
                   unsigned short               remotePort,
                   const RTP_MSG_USER*          user,
                   const char*                  password,         /* = NULL */
                   const char*                  localIp,          /* = NULL */
                   unsigned int                 timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpMsgClient* msgClient = CRtpMsgClient::CreateInstance(false, mmType, sslConfig, sslSni);
    if (msgClient == NULL)
    {
        return NULL;
    }

    if (!msgClient->Init(observer, reactor, remoteIp, remotePort, user, password,
        localIp, timeoutInSeconds))
    {
        msgClient->Release();

        return NULL;
    }

    return msgClient;
}

PRO_RTP_API
void
DeleteRtpMsgClient(IRtpMsgClient* msgClient)
{
    if (msgClient == NULL)
    {
        return;
    }

    CRtpMsgClient* p = (CRtpMsgClient*)msgClient;
    p->Fini();
    p->Release();
}

PRO_RTP_API
IRtpMsgServer*
CreateRtpMsgServer(IRtpMsgServerObserver*       observer,
                   IProReactor*                 reactor,
                   RTP_MM_TYPE                  mmType,
                   const PRO_SSL_SERVER_CONFIG* sslConfig,        /* = NULL */
                   bool                         sslForced,        /* = false */
                   unsigned short               serviceHubPort,
                   unsigned int                 timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpMsgServer* msgServer = CRtpMsgServer::CreateInstance(mmType, sslConfig, sslForced);
    if (msgServer == NULL)
    {
        return NULL;
    }

    if (!msgServer->Init(observer, reactor, serviceHubPort, timeoutInSeconds))
    {
        msgServer->Release();

        return NULL;
    }

    return msgServer;
}

PRO_RTP_API
void
DeleteRtpMsgServer(IRtpMsgServer* msgServer)
{
    if (msgServer == NULL)
    {
        return;
    }

    CRtpMsgServer* p = (CRtpMsgServer*)msgServer;
    p->Fini();
    p->Release();
}

PRO_RTP_API
IRtpMsgC2s*
CreateRtpMsgC2s(IRtpMsgC2sObserver*          observer,
                IProReactor*                 reactor,
                RTP_MM_TYPE                  mmType,
                const PRO_SSL_CLIENT_CONFIG* uplinkSslConfig,        /* = NULL */
                const char*                  uplinkSslSni,           /* = NULL */
                const char*                  uplinkIp,
                unsigned short               uplinkPort,
                const RTP_MSG_USER*          uplinkUser,
                const char*                  uplinkPassword,
                const char*                  uplinkLocalIp,          /* = NULL */
                unsigned int                 uplinkTimeoutInSeconds, /* = 0 */
                const PRO_SSL_SERVER_CONFIG* localSslConfig,         /* = NULL */
                bool                         localSslForced,         /* = false */
                unsigned short               localServiceHubPort,
                unsigned int                 localTimeoutInSeconds)  /* = 0 */
{
    ProRtpInit();

    CRtpMsgC2s* msgC2s = CRtpMsgC2s::CreateInstance(
        mmType, uplinkSslConfig, uplinkSslSni, localSslConfig, localSslForced);
    if (msgC2s == NULL)
    {
        return NULL;
    }

    if (!msgC2s->Init(observer, reactor, uplinkIp, uplinkPort, uplinkUser, uplinkPassword,
        uplinkLocalIp, uplinkTimeoutInSeconds, localServiceHubPort, localTimeoutInSeconds))
    {
        msgC2s->Release();

        return NULL;
    }

    return msgC2s;
}

PRO_RTP_API
void
DeleteRtpMsgC2s(IRtpMsgC2s* msgC2s)
{
    if (msgC2s == NULL)
    {
        return;
    }

    CRtpMsgC2s* p = (CRtpMsgC2s*)msgC2s;
    p->Fini();
    p->Release();
}

PRO_RTP_API
void
RtpMsgUser2String(const RTP_MSG_USER* user,
                  char                idString[64])
{
    strcpy(idString, "0-0-0");

    if (user == NULL)
    {
        return;
    }

    sprintf(
        idString,
        "%u-%llu-%u",
        (unsigned int)user->classId,
        (unsigned long long)user->UserId(),
        (unsigned int)user->instId
        );
}

PRO_RTP_API
void
RtpMsgString2User(const char*   idString,
                  RTP_MSG_USER* user)
{
    if (user != NULL)
    {
        user->Zero();
    }

    if (idString == NULL || idString[0] == '\0' || user == NULL)
    {
        return;
    }

    CProStlString cidString;
    CProStlString uidString;
    CProStlString iidString;

    for (int i = 0; idString[i] != '\0'; ++i)
    {
        if (idString[i] == '-')
        {
            for (int j = i + 1; idString[j] != '\0'; ++j)
            {
                if (idString[j] == '-')
                {
                    cidString.assign(idString, i);
                    uidString.assign(idString + (i + 1), j - (i + 1));
                    iidString.assign(idString + j + 1);

                    goto EXIT;
                }
            }

            cidString.assign(idString, i);
            uidString.assign(idString + i + 1);
            iidString = "0";

            goto EXIT;
        }
    }

EXIT:

    if (cidString.empty() || uidString.empty() || iidString.empty())
    {
        return;
    }

    if (cidString.find_first_not_of("0123456789") != CProStlString::npos ||
        uidString.find_first_not_of("0123456789") != CProStlString::npos ||
        iidString.find_first_not_of("0123456789") != CProStlString::npos)
    {
        return;
    }

    unsigned int cid = 0;
    uint64_t     uid = 0;
    unsigned int iid = 0;

    sscanf(cidString.c_str(), "%u"  , &cid);
    sscanf(uidString.c_str(), "%llu", (unsigned long long*)&uid);
    sscanf(iidString.c_str(), "%u"  , &iid);

    unsigned char maxCid = 0xFF;
    uint64_t      maxUid = 0xFFFFFFFFFFULL;
    uint16_t      maxIid = 0xFFFF;

    if (cid <= maxCid && uid <= maxUid && iid <= maxIid)
    {
        user->classId = (unsigned char)cid;
        user->UserId(uid);
        user->instId  = (uint16_t)iid;
    }
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif
