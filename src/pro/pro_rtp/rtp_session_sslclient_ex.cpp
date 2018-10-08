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

#include "rtp_session_sslclient_ex.h"
#include "rtp_session_tcpclient_ex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionSslclientEx*
CRtpSessionSslclientEx::CreateInstance(const RTP_SESSION_INFO*      localInfo,
                                       const PRO_SSL_CLIENT_CONFIG* sslConfig,
                                       const char*                  sslServiceName) /* = NULL */
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    assert(sslConfig != NULL);
    if (localInfo == NULL || localInfo->mmType == 0 || sslConfig == NULL)
    {
        return (NULL);
    }

    CRtpSessionSslclientEx* const session =
        new CRtpSessionSslclientEx(*localInfo, sslConfig, sslServiceName);

    return (session);
}

CRtpSessionSslclientEx::CRtpSessionSslclientEx(const RTP_SESSION_INFO&      localInfo,
                                               const PRO_SSL_CLIENT_CONFIG* sslConfig,
                                               const char*                  sslServiceName) /* = NULL */
                                               :
CRtpSessionTcpclientEx(localInfo, sslConfig, sslServiceName)
{
    m_info.sessionType = RTP_ST_SSLCLIENT_EX;
}
