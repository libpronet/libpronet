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

#ifndef ____PRO_SSL_H____
#define ____PRO_SSL_H____

#if !defined(____PRO_NET_H____)
#error You must include "pro_net.h" first.
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

struct PRO_NONCE;             /* Big random number */
struct PRO_SSL_CLIENT_CONFIG; /* Derived from mbedtls_ssl_config */
struct PRO_SSL_CTX;           /* Derived from mbedtls_ssl_context */
struct PRO_SSL_SERVER_CONFIG; /* Derived from mbedtls_ssl_config */

/*
 * [[[[ Authentication levels
 */
typedef unsigned char PRO_SSL_AUTH_LEVEL;

static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_NONE     = 0;
static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_OPTIONAL = 1;
static const PRO_SSL_AUTH_LEVEL PRO_SSL_AUTHLV_REQUIRED = 2;
/*
 * ]]]]
 */

/*
 * [[[[ SSL/TLS suites
 *
 * These are all certificate-based AEAD cipher suites, and we only recommend
 * Perfect Forward Secrecy (PFS) cipher suites. If Pre-Shared Key (PSK) mechanism
 * or richer cipher suites are needed, users can directly reference mbedtls
 * library definitions
 *
 * Please refer to "mbedtls/ssl_ciphersuites.h"
 */
typedef unsigned short PRO_SSL_SUITE_ID;

static const PRO_SSL_SUITE_ID PRO_SSL_SUITE_NONE                                = 0x0000;
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256           = 0x009E; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384           = 0x009F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256       = 0xC02B; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384       = 0xC02C; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256         = 0xC02F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384         = 0xC030; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256      = 0xC07C; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384      = 0xC07D; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256  = 0xC086; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384  = 0xC087; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256    = 0xC08A; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384    = 0xC08B; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_128_CCM                  = 0xC09E; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_AES_256_CCM                  = 0xC09F; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_128_CCM              = 0xC0AC; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_AES_256_CCM              = 0xC0AD; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256   = 0xCCA8; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 = 0xCCA9; /* TLS 1.2 */
static const PRO_SSL_SUITE_ID PRO_SSL_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256     = 0xCCAA; /* TLS 1.2 */
/*
 * ]]]]
 */

/////////////////////////////////////////////////////////////////////////////
////

/*
 * Function: Create a server SSL configuration
 *
 * Parameters: None
 *
 * Return: SSL configuration object or NULL
 *
 * Note: PRO_SSL_SERVER_CONFIG inherits from mbedtls_ssl_config. If necessary,
 *       the object can be manipulated using mbedtls library functions
 */
PRO_NET_API
PRO_SSL_SERVER_CONFIG*
ProSslServerConfig_Create();

/*
 * Function: Delete a server SSL configuration
 *
 * Parameters:
 * config : SSL configuration object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProSslServerConfig_Delete(PRO_SSL_SERVER_CONFIG* config);

/*
 * Function: Set cipher suites
 *
 * Parameters:
 * config     : SSL configuration object
 * suites     : Cipher suite list (client's suite list order has priority)
 * suiteCount : List length
 *
 * Return: true on success, false on failure
 *
 * Note: If necessary, richer cipher suites can be set using mbedtls library
 */
PRO_NET_API
bool
ProSslServerConfig_SetSuiteList(PRO_SSL_SERVER_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * Function: Set ALPN
 *
 * Parameters:
 * config    : SSL configuration object
 * alpns     : ALPN list (client's list order has priority)
 * alpnCount : List length
 *
 * Return: true on success, false on failure
 *
 * Note: None
 */
PRO_NET_API
bool
ProSslServerConfig_SetAlpnList(PRO_SSL_SERVER_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * Function: Enable or disable SHA-1 certificate support
 *
 * Parameters:
 * config : SSL configuration object
 * enable : true to enable, false to disable
 *
 * Return: None
 *
 * Note: SHA-1 certificates are not supported by default
 */
PRO_NET_API
void
ProSslServerConfig_EnableSha1Cert(PRO_SSL_SERVER_CONFIG* config,
                                  bool                   enable);

/*
 * Function: Set CA certificate list
 *
 * Parameters:
 * config       : SSL configuration object
 * caFiles      : CA file list
 * caFileCount  : CA list length
 * crlFiles     : CRL file list
 * crlFileCount : CRL list length
 *
 * Return: true on success, false on failure
 *
 * Note: caFiles can contain two types of certificates:
 *       1) Trusted CA certificates
 *       2) Trusted self-signed end-entity certificates (CA flag may be unset)
 */
PRO_NET_API
bool
ProSslServerConfig_SetCaList(PRO_SSL_SERVER_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,      /* = NULL */
                             size_t                 crlFileCount); /* = 0 */

/*
 * Function: Append a certificate chain
 *
 * Parameters:
 * config        : SSL configuration object
 * certFiles     : Certificate file list
 * certFileCount : List length
 * keyFile       : Private key file (corresponds to certFiles[0])
 * password      : Private key password
 *
 * Return: true on success, false on failure
 *
 * Note: None
 */
PRO_NET_API
bool
ProSslServerConfig_AppendCertChain(PRO_SSL_SERVER_CONFIG* config,
                                   const char**           certFiles,
                                   size_t                 certFileCount,
                                   const char*            keyFile,
                                   const char*            password); /* = NULL */

/*
 * Function: Set authentication level
 *
 * Parameters:
 * config : SSL configuration object
 * level  : Authentication level
 *
 * Return: true on success, false on failure
 *
 * Note: By default, server does not authenticate client
 */
PRO_NET_API
bool
ProSslServerConfig_SetAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*
 * Function: Add an SNI entry
 *
 * Parameters:
 * config  : SSL configuration object
 * sniName : SNI service name
 *
 * Return: true on success, false on failure
 *
 * Note: None
 */
PRO_NET_API
bool
ProSslServerConfig_AddSni(PRO_SSL_SERVER_CONFIG* config,
                          const char*            sniName);

/*
 * Function: Remove an SNI entry
 *
 * Parameters:
 * config  : SSL configuration object
 * sniName : SNI service name
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProSslServerConfig_RemoveSni(PRO_SSL_SERVER_CONFIG* config,
                             const char*            sniName);

/*
 * Function: Set CA certificate list for a specific SNI entry
 *
 * Parameters:
 * config       : SSL configuration object
 * sniName      : SNI service name
 * caFiles      : CA file list
 * caFileCount  : CA list length
 * crlFiles     : CRL file list
 * crlFileCount : CRL list length
 *
 * Return: true on success, false on failure
 *
 * Note: caFiles can contain two types of certificates:
 *       1) Trusted CA certificates
 *       2) Trusted self-signed end-entity certificates (CA flag may be unset)
 */
PRO_NET_API
bool
ProSslServerConfig_SetSniCaList(PRO_SSL_SERVER_CONFIG* config,
                                const char*            sniName,
                                const char**           caFiles,
                                size_t                 caFileCount,
                                const char**           crlFiles,      /* = NULL */
                                size_t                 crlFileCount); /* = 0 */

/*
 * Function: Append a certificate chain for a specific SNI entry
 *
 * Parameters:
 * config        : SSL configuration object
 * sniName       : SNI service name
 * certFiles     : Certificate file list
 * certFileCount : List length
 * keyFile       : Private key file (corresponds to certFiles[0])
 * password      : Private key password
 *
 * Return: true on success, false on failure
 *
 * Note: None
 */
PRO_NET_API
bool
ProSslServerConfig_AppendSniCertChain(PRO_SSL_SERVER_CONFIG* config,
                                      const char*            sniName,
                                      const char**           certFiles,
                                      size_t                 certFileCount,
                                      const char*            keyFile,
                                      const char*            password); /* = NULL */

/*
 * Function: Set authentication level for a specific SNI entry
 *
 * Parameters:
 * config : SSL configuration object
 * level  : Authentication level
 *
 * Return: true on success, false on failure
 *
 * Note: By default, server does not authenticate client
 */
PRO_NET_API
bool
ProSslServerConfig_SetSniAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                   const char*            sniName,
                                   PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * Function: Create a client SSL configuration
 *
 * Parameters: None
 *
 * Return: SSL configuration object or NULL
 *
 * Note: PRO_SSL_CLIENT_CONFIG inherits from mbedtls_ssl_config. If necessary,
 *       the object can be manipulated using mbedtls library functions
 */
PRO_NET_API
PRO_SSL_CLIENT_CONFIG*
ProSslClientConfig_Create();

/*
 * Function: Delete a client SSL configuration
 *
 * Parameters:
 * config : SSL configuration object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProSslClientConfig_Delete(PRO_SSL_CLIENT_CONFIG* config);

/*
 * Function: Set cipher suites
 *
 * Parameters:
 * config     : SSL configuration object
 * suites     : Cipher suite list
 * suiteCount : List length
 *
 * Return: true on success, false on failure
 *
 * Note: If necessary, richer cipher suites can be set using mbedtls library
 */
PRO_NET_API
bool
ProSslClientConfig_SetSuiteList(PRO_SSL_CLIENT_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * Function: Set ALPN
 *
 * Parameters:
 * config    : SSL configuration object
 * alpns     : ALPN list
 * alpnCount : List length
 *
 * Return: true on success, false on failure
 *
 * Note: None
 */
PRO_NET_API
bool
ProSslClientConfig_SetAlpnList(PRO_SSL_CLIENT_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * Function: Enable or disable SHA-1 certificate support
 *
 * Parameters:
 * config : SSL configuration object
 * enable : true to enable, false to disable
 *
 * Return: None
 *
 * Note: SHA-1 certificates are not supported by default
 */
PRO_NET_API
void
ProSslClientConfig_EnableSha1Cert(PRO_SSL_CLIENT_CONFIG* config,
                                  bool                   enable);

/*
 * Function: Set CA certificate list
 *
 * Parameters:
 * config       : SSL configuration object
 * caFiles      : CA file list
 * caFileCount  : CA list length
 * crlFiles     : CRL file list
 * crlFileCount : CRL list length
 *
 * Return: true on success, false on failure
 *
 * Note: caFiles can contain two types of certificates:
 *       1) Trusted CA certificates
 *       2) Trusted self-signed end-entity certificates (CA flag may be unset)
 */
PRO_NET_API
bool
ProSslClientConfig_SetCaList(PRO_SSL_CLIENT_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,      /* = NULL */
                             size_t                 crlFileCount); /* = 0 */

/*
 * Function: Set certificate chain
 *
 * Parameters:
 * config        : SSL configuration object
 * certFiles     : Certificate file list
 * certFileCount : List length
 * keyFile       : Private key file (corresponds to certFiles[0])
 * password      : Private key password
 *
 * Return: true on success, false on failure
 *
 * Note: This function can only be successfully called once
 */
PRO_NET_API
bool
ProSslClientConfig_SetCertChain(PRO_SSL_CLIENT_CONFIG* config,
                                const char**           certFiles,
                                size_t                 certFileCount,
                                const char*            keyFile,
                                const char*            password); /* = NULL */

/*
 * Function: Set authentication level
 *
 * Parameters:
 * config : SSL configuration object
 * level  : Authentication level
 *
 * Return: true on success, false on failure
 *
 * Note: By default, the client requires server certificate verification
 */
PRO_NET_API
bool
ProSslClientConfig_SetAuthLevel(PRO_SSL_CLIENT_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * Function: Create a server SSL context
 *
 * Parameters:
 * config : SSL configuration object
 * sockId : Socket ID
 * nonce  : Perturbation random number. NULL means no perturbation
 *
 * Return: SSL context object or NULL
 *
 * Note: PRO_SSL_CTX inherits from mbedtls_ssl_context. If necessary,
 *       the object can be manipulated using mbedtls library functions
 *
 *       The nonce is used to add perturbation to initial handshake traffic,
 *       mainly to prevent plaintext certificates from being intercepted/filtered.
 *       Both client and server must use the same nonce, typically obtained
 *       from OnAccept() or OnConnectOk()
 */
PRO_NET_API
PRO_SSL_CTX*
ProSslCtx_CreateS(const PRO_SSL_SERVER_CONFIG* config,
                  int64_t                      sockId,
                  const PRO_NONCE*             nonce); /* = NULL */

/*
 * Function: Create a client SSL context
 *
 * Parameters:
 * config         : SSL configuration object
 * serverHostName : Server hostname.
 *                  If valid, participates in server certificate verification
 * sockId         : Socket ID
 * nonce          : Perturbation random number. NULL means no perturbation
 *
 * Return: SSL context object or NULL
 *
 * Note: PRO_SSL_CTX inherits from mbedtls_ssl_context. If necessary,
 *       the object can be manipulated using mbedtls library functions
 *
 *       The nonce is used to add perturbation to initial handshake traffic,
 *       mainly to prevent plaintext certificates from being intercepted/filtered.
 *       Both client and server must use the same nonce, typically obtained
 *       from OnAccept() or OnConnectOk()
 */
PRO_NET_API
PRO_SSL_CTX*
ProSslCtx_CreateC(const PRO_SSL_CLIENT_CONFIG* config,
                  const char*                  serverHostName, /* = NULL */
                  int64_t                      sockId,
                  const PRO_NONCE*             nonce);         /* = NULL */

/*
 * Function: Delete an SSL context
 *
 * Parameters:
 * ctx : SSL context object
 *
 * Return: None
 *
 * Note: None
 */
PRO_NET_API
void
ProSslCtx_Delete(PRO_SSL_CTX* ctx);

/*
 * Function: Get negotiated cipher suite between client and server
 *
 * Parameters:
 * ctx       : SSL context object
 * suiteName : Returned cipher suite name
 *
 * Return: Cipher suite ID
 *
 * Note: Only meaningful after SSL/TLS handshake completes
 */
PRO_NET_API
PRO_SSL_SUITE_ID
ProSslCtx_GetSuite(PRO_SSL_CTX* ctx,
                   char         suiteName[64]);

/*
 * Function: Get negotiated ALPN protocol name between client and server
 *
 * Parameters:
 * ctx : SSL context object
 *
 * Return: ALPN protocol name (can be NULL)
 *
 * Note: Only meaningful after SSL/TLS handshake completes
 */
PRO_NET_API
const char*
ProSslCtx_GetAlpn(PRO_SSL_CTX* ctx);

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_SSL_H____ */
