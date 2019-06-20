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

#if !defined(____PRO_MBEDTLS_H____)
#define ____PRO_MBEDTLS_H____

#if !defined(____PRO_NET_H____)
#error You must include "pro_net.h" first.
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

/*
 * [[[[ authentication levels
 */
typedef unsigned char PRO_SSL_AUTH_LEVEL;

static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_NONE     = (PRO_SSL_AUTH_LEVEL)0;
static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_OPTIONAL = (PRO_SSL_AUTH_LEVEL)1;
static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_REQUIRED = (PRO_SSL_AUTH_LEVEL)2;
/*
 * ]]]]
 */

/*
 * [[[[ SSL/TLS suites
 *
 * 这里都是基于证书的AEAD加密套件,并且,我们只推荐前向安全(PFS)的加密套件. 如果
 * 需要预主密钥(PSK)机制或更加丰富的加密套件,使用者可以直接引用mbedtls库的定义
 *
 * please refer to "mbedtls/ssl_ciphersuites.h"
 */
typedef unsigned short PRO_SSL_SUITE_ID;

static const PRO_SSL_SUITE_ID PRO_SSL_SUITE_NONE                                = (PRO_SSL_SUITE_ID)0x0000;
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256           = (PRO_SSL_SUITE_ID)0x009E; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384           = (PRO_SSL_SUITE_ID)0x009F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256       = (PRO_SSL_SUITE_ID)0xC02B; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384       = (PRO_SSL_SUITE_ID)0xC02C; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256         = (PRO_SSL_SUITE_ID)0xC02F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384         = (PRO_SSL_SUITE_ID)0xC030; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256      = (PRO_SSL_SUITE_ID)0xC07C; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384      = (PRO_SSL_SUITE_ID)0xC07D; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256  = (PRO_SSL_SUITE_ID)0xC086; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384  = (PRO_SSL_SUITE_ID)0xC087; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256    = (PRO_SSL_SUITE_ID)0xC08A; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384    = (PRO_SSL_SUITE_ID)0xC08B; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_128_CCM                  = (PRO_SSL_SUITE_ID)0xC09E; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_256_CCM                  = (PRO_SSL_SUITE_ID)0xC09F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_128_CCM              = (PRO_SSL_SUITE_ID)0xC0AC; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_256_CCM              = (PRO_SSL_SUITE_ID)0xC0AD; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256   = (PRO_SSL_SUITE_ID)0xCCA8; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 = (PRO_SSL_SUITE_ID)0xCCA9; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256     = (PRO_SSL_SUITE_ID)0xCCAA; /* TLS 1.2 */
/*
 * ]]]]
 */

struct PRO_SSL_CLIENT_CONFIG; /* derived from mbedtls_ssl_config */
struct PRO_SSL_CTX;           /* derived from mbedtls_ssl_context */
struct PRO_SSL_SERVER_CONFIG; /* derived from mbedtls_ssl_config */

/////////////////////////////////////////////////////////////////////////////
////

/*
 * 功能: 创建一个服务端SSL配置
 *
 * 参数: 无
 *
 * 返回值: SSL配置对象或NULL
 *
 * 说明: PRO_SSL_SERVER_CONFIG继承自mbedtls_ssl_config. 如果需要,可以通过
 *       mbedtls库操纵该对象
 */
PRO_NET_API
PRO_SSL_SERVER_CONFIG*
PRO_CALLTYPE
ProSslServerConfig_Create();

/*
 * 功能: 删除一个服务端SSL配置
 *
 * 参数:
 * config : SSL配置对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_Delete(PRO_SSL_SERVER_CONFIG* config);

/*
 * 功能: 设置加密套件
 *
 * 参数:
 * config     : SSL配置对象
 * suites     : 加密套件列表. 客户端的套件列表顺序优先
 * suiteCount : 列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: 如果需要,可以通过mbedtls库设置更加丰富的加密套件
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSuiteList(PRO_SSL_SERVER_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * 功能: 设置ALPN
 *
 * 参数:
 * config    : SSL配置对象
 * alpns     : ALPN列表. 客户端的列表顺序优先
 * alpnCount : 列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: 无
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAlpnList(PRO_SSL_SERVER_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * 功能: 是否支持SHA-1证书
 *
 * 参数:
 * config : SSL配置对象
 * enable : true支持, false不支持
 *
 * 返回值: 无
 *
 * 说明: 默认不支持SHA-1证书
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_EnableSha1Cert(PRO_SSL_SERVER_CONFIG* config,
                                  bool                   enable);

/*
 * 功能: 设置CA证书列表
 *
 * 参数:
 * config       : SSL配置对象
 * caFiles      : CA文件列表
 * caFileCount  : CA列表长度
 * crlFiles     : CRL文件列表
 * crlFileCount : CRL列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: caFiles可以包含两类证书,
 *       1)可信的CA证书;
 *       2)可信的自签名终端用户证书. 此时,证书的CA标志位可以不设置
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetCaList(PRO_SSL_SERVER_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,      /* = NULL */
                             size_t                 crlFileCount); /* = 0 */

/*
 * 功能: 追加一条证书链
 *
 * 参数:
 * config        : SSL配置对象
 * certFiles     : 证书文件列表
 * certFileCount : 列表长度
 * keyFile       : 私钥文件. 与certFiles[0]对应
 * password      : 私钥文件口令
 *
 * 返回值: true成功, false失败
 *
 * 说明: 无
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AppendCertChain(PRO_SSL_SERVER_CONFIG* config,
                                   const char**           certFiles,
                                   size_t                 certFileCount,
                                   const char*            keyFile,
                                   const char*            password); /* = NULL */

/*
 * 功能: 设置认证级别
 *
 * 参数:
 * config : SSL配置对象
 * level  : 认证级别
 *
 * 返回值: true成功, false失败
 *
 * 说明: 默认情况下, server不认证client
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*
 * 功能: 添加一个SNI条目
 *
 * 参数:
 * config  : SSL配置对象
 * sniName : SNI服务名
 *
 * 返回值: true成功, false失败
 *
 * 说明: 无
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AddSni(PRO_SSL_SERVER_CONFIG* config,
                          const char*            sniName);

/*
 * 功能: 删除一个SNI条目
 *
 * 参数:
 * config  : SSL配置对象
 * sniName : SNI服务名
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_RemoveSni(PRO_SSL_SERVER_CONFIG* config,
                             const char*            sniName);

/*
 * 功能: 设置特定SNI条目的CA证书列表
 *
 * 参数:
 * config       : SSL配置对象
 * sniName      : SNI服务名
 * caFiles      : CA文件列表
 * caFileCount  : CA列表长度
 * crlFiles     : CRL文件列表
 * crlFileCount : CRL列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: caFiles可以包含两类证书,
 *       1)可信的CA证书;
 *       2)可信的自签名终端用户证书. 此时,证书的CA标志位可以不设置
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSniCaList(PRO_SSL_SERVER_CONFIG* config,
                                const char*            sniName,
                                const char**           caFiles,
                                size_t                 caFileCount,
                                const char**           crlFiles,      /* = NULL */
                                size_t                 crlFileCount); /* = 0 */

/*
 * 功能: 追加特定SNI条目的一条证书链
 *
 * 参数:
 * config        : SSL配置对象
 * sniName       : SNI服务名
 * certFiles     : 证书文件列表
 * certFileCount : 列表长度
 * keyFile       : 私钥文件. 与certFiles[0]对应
 * password      : 私钥文件口令
 *
 * 返回值: true成功, false失败
 *
 * 说明: 无
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AppendSniCertChain(PRO_SSL_SERVER_CONFIG* config,
                                      const char*            sniName,
                                      const char**           certFiles,
                                      size_t                 certFileCount,
                                      const char*            keyFile,
                                      const char*            password); /* = NULL */

/*
 * 功能: 设置特定SNI条目的认证级别
 *
 * 参数:
 * config : SSL配置对象
 * level  : 认证级别
 *
 * 返回值: true成功, false失败
 *
 * 说明: 默认情况下, server不认证client
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSniAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                   const char*            sniName,
                                   PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * 功能: 创建一个客户端SSL配置
 *
 * 参数: 无
 *
 * 返回值: SSL配置对象或NULL
 *
 * 说明: PRO_SSL_CLIENT_CONFIG继承自mbedtls_ssl_config. 如果需要,可以通过
 *       mbedtls库操纵该对象
 */
PRO_NET_API
PRO_SSL_CLIENT_CONFIG*
PRO_CALLTYPE
ProSslClientConfig_Create();

/*
 * 功能: 删除一个客户端SSL配置
 *
 * 参数:
 * config : SSL配置对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_Delete(PRO_SSL_CLIENT_CONFIG* config);

/*
 * 功能: 设置加密套件
 *
 * 参数:
 * config     : SSL配置对象
 * suites     : 加密套件列表
 * suiteCount : 列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: 如果需要,可以通过mbedtls库设置更加丰富的加密套件
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetSuiteList(PRO_SSL_CLIENT_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * 功能: 设置ALPN
 *
 * 参数:
 * config    : SSL配置对象
 * alpns     : ALPN列表
 * alpnCount : 列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: 无
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAlpnList(PRO_SSL_CLIENT_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * 功能: 是否支持SHA-1证书
 *
 * 参数:
 * config : SSL配置对象
 * enable : true支持, false不支持
 *
 * 返回值: 无
 *
 * 说明: 默认不支持SHA-1证书
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_EnableSha1Cert(PRO_SSL_CLIENT_CONFIG* config,
                                  bool                   enable);

/*
 * 功能: 设置CA证书列表
 *
 * 参数:
 * config       : SSL配置对象
 * caFiles      : CA文件列表
 * caFileCount  : CA列表长度
 * crlFiles     : CRL文件列表
 * crlFileCount : CRL列表长度
 *
 * 返回值: true成功, false失败
 *
 * 说明: caFiles可以包含两类证书,
 *       1)可信的CA证书;
 *       2)可信的自签名终端用户证书. 此时,证书的CA标志位可以不设置
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetCaList(PRO_SSL_CLIENT_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,      /* = NULL */
                             size_t                 crlFileCount); /* = 0 */

/*
 * 功能: 设置证书链
 *
 * 参数:
 * config        : SSL配置对象
 * certFiles     : 证书文件列表
 * certFileCount : 列表长度
 * keyFile       : 私钥文件. 与certFiles[0]对应
 * password      : 私钥文件口令
 *
 * 返回值: true成功, false失败
 *
 * 说明: 该函数只能成功调用一次
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetCertChain(PRO_SSL_CLIENT_CONFIG* config,
                                const char**           certFiles,
                                size_t                 certFileCount,
                                const char*            keyFile,
                                const char*            password); /* = NULL */

/*
 * 功能: 设置认证级别
 *
 * 参数:
 * config : SSL配置对象
 * level  : 认证级别
 *
 * 返回值: true成功, false失败
 *
 * 说明: 默认情况下, client要求认证server
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAuthLevel(PRO_SSL_CLIENT_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * 功能: 创建一个服务端SSL上下文
 *
 * 参数:
 * config : SSL配置对象
 * sockId : 套接字id
 * nonce  : 扰动随机数. 0表示无扰动
 *
 * 返回值: SSL上下文对象或NULL
 *
 * 说明: PRO_SSL_CTX继承自mbedtls_ssl_context. 如果需要,可以通过mbedtls库
 *       操纵该对象
 *
 *       nonce用于为初期握手流量添加扰动,主要用于防止握手初期的明文证书被
 *       过滤拦截. c/s两端必须一致,一般来源于OnAccept(...)或OnConnectOk(...)
 */
PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Creates(const PRO_SSL_SERVER_CONFIG* config,
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce); /* = 0 */

/*
 * 功能: 创建一个客户端SSL上下文
 *
 * 参数:
 * config         : SSL配置对象
 * serverHostName : server主机名. 如果有效,则参与认证server证书
 * sockId         : 套接字id
 * nonce          : 扰动随机数. 0表示无扰动
 *
 * 返回值: SSL上下文对象或NULL
 *
 * 说明: PRO_SSL_CTX继承自mbedtls_ssl_context. 如果需要,可以通过mbedtls库
 *       操纵该对象
 *
 *       nonce用于为初期握手流量添加扰动,主要用于防止握手初期的明文证书被
 *       过滤拦截. c/s两端必须一致,一般来源于OnAccept(...)或OnConnectOk(...)
 */
PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Createc(const PRO_SSL_CLIENT_CONFIG* config,
                  const char*                  serverHostName, /* = NULL */
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce);         /* = 0 */

/*
 * 功能: 删除一个SSL上下文
 *
 * 参数:
 * ctx : SSL上下文对象
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslCtx_Delete(PRO_SSL_CTX* ctx);

/*
 * 功能: 获取c/s协商的会话加密套件
 *
 * 参数:
 * ctx       : SSL上下文对象
 * suiteName : 返回的加密套件名
 *
 * 返回值: 加密套件id
 *
 * 说明: SSL/TLS握手完成后才有意义
 */
PRO_NET_API
PRO_SSL_SUITE_ID
PRO_CALLTYPE
ProSslCtx_GetSuite(PRO_SSL_CTX* ctx,
                   char         suiteName[64]);

/*
 * 功能: 获取c/s协商的ALPN协议名
 *
 * 参数:
 * ctx : SSL上下文对象
 *
 * 返回值: ALPN协议名. 可以是NULL
 *
 * 说明: SSL/TLS握手完成后才有意义
 */
PRO_NET_API
const char*
PRO_CALLTYPE
ProSslCtx_GetAlpn(PRO_SSL_CTX* ctx);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____PRO_MBEDTLS_H____ */
