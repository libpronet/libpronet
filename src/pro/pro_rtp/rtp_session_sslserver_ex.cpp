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

#include "rtp_session_sslserver_ex.h"
#include "rtp_session_tcpserver_ex.h"
#include "../pro_util/pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

CRtpSessionSslserverEx*
CRtpSessionSslserverEx::CreateInstance(const RTP_SESSION_INFO* localInfo,
                                       PRO_SSL_CTX*            sslCtx)
{
    assert(localInfo != NULL);
    assert(localInfo->mmType != 0);
    assert(sslCtx != NULL);
    if (localInfo == NULL || localInfo->mmType == 0 || sslCtx == NULL)
    {
        return (NULL);
    }

    CRtpSessionSslserverEx* const session = new CRtpSessionSslserverEx(*localInfo, sslCtx);

    return (session);
}

CRtpSessionSslserverEx::CRtpSessionSslserverEx(const RTP_SESSION_INFO& localInfo,
                                               PRO_SSL_CTX*            sslCtx)
                                               :
CRtpSessionTcpserverEx(localInfo, sslCtx)
{
    m_info.sessionType = RTP_ST_SSLSERVER_EX;
}
