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

#if !defined(RTP_MSG_COMMAND_H)
#define RTP_MSG_COMMAND_H

/////////////////////////////////////////////////////////////////////////////
////

/*
 * server : 1-1
 * c2s    : 1-2, 1-3, ...
 * client : 2-1, 2-2, ...; 3-1, 3-2, ...; ...
 */

/*-------------------------------------------------------------------------*/

/*
 * c2s ---> server
 *
 * "msg_name"                "***client_login"
 * "client_index"            "1"
 * "client_id"               "2-1" or "2-1-0" or "2-1-1" or "2-0" or "2-0-0"
 * "client_public_ip"        "a.b.c.d"
 * "client_hash_string"      "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"
 * "client_nonce"            "1234"
 */

/*
 * c2s <--- server
 *
 * "msg_name"                "***client_login_ok"
 * "client_index"            "1"
 * "client_id"               "2-1-0"
 */

/*
 * c2s <--- server
 *
 * "msg_name"                "***client_login_error"
 * "client_index"            "1"
 */

/*-------------------------------------------------------------------------*/

/*
 * c2s ---> server
 *
 * "msg_name"                "***client_logout"
 * "client_id"               "2-1-0"
 */

/*-------------------------------------------------------------------------*/

/*
 * server ---> c2s
 *
 * "msg_name"                "***client_kickout"
 * "client_id"               "2-1-0"
 */

/////////////////////////////////////////////////////////////////////////////
////

static const char* const TAG_msg_name                    = "msg_name"             ;
static const char* const TAG_client_index                = "client_index"         ;
static const char* const TAG_client_id                   = "client_id"            ;
static const char* const TAG_client_public_ip            = "client_public_ip"     ;
static const char* const TAG_client_hash_string          = "client_hash_string"   ;
static const char* const TAG_client_nonce                = "client_nonce"         ;

static const char* const MSG_client_login                = "***client_login"      ;
static const char* const MSG_client_login_ok             = "***client_login_ok"   ;
static const char* const MSG_client_login_error          = "***client_login_error";
static const char* const MSG_client_logout               = "***client_logout"     ;
static const char* const MSG_client_kickout              = "***client_kickout"    ;

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_MSG_COMMAND_H */
