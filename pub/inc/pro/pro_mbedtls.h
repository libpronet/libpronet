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
 * ���ﶼ�ǻ���֤���AEAD�����׼�,����,����ֻ�Ƽ�ǰ��ȫ(PFS)�ļ����׼�. ���
 * ��ҪԤ����Կ(PSK)���ƻ���ӷḻ�ļ����׼�,ʹ���߿���ֱ������mbedtls��Ķ���
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
 * ����: ����һ�������SSL����
 *
 * ����: ��
 *
 * ����ֵ: SSL���ö����NULL
 *
 * ˵��: PRO_SSL_SERVER_CONFIG�̳���mbedtls_ssl_config. �����Ҫ,����ͨ��
 *       mbedtls����ݸö���
 */
PRO_NET_API
PRO_SSL_SERVER_CONFIG*
PRO_CALLTYPE
ProSslServerConfig_Create();

/*
 * ����: ɾ��һ�������SSL����
 *
 * ����:
 * config : SSL���ö���
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_Delete(PRO_SSL_SERVER_CONFIG* config);

/*
 * ����: ���ü����׼�
 *
 * ����:
 * config     : SSL���ö���
 * suites     : �����׼��б�. �ͻ��˵��׼��б�˳������
 * suiteCount : �б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: �����Ҫ,����ͨ��mbedtls�����ø��ӷḻ�ļ����׼�
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSuiteList(PRO_SSL_SERVER_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * ����: ����ALPN
 *
 * ����:
 * config    : SSL���ö���
 * alpns     : ALPN�б�. �ͻ��˵��б�˳������
 * alpnCount : �б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: ��
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAlpnList(PRO_SSL_SERVER_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * ����: �Ƿ�֧��SHA-1֤��
 *
 * ����:
 * config : SSL���ö���
 * enable : true֧��, false��֧��
 *
 * ����ֵ: ��
 *
 * ˵��: Ĭ�ϲ�֧��SHA-1֤��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_EnableSha1Cert(PRO_SSL_SERVER_CONFIG* config,
                                  bool                   enable);

/*
 * ����: ����CA֤���б�
 *
 * ����:
 * config       : SSL���ö���
 * caFiles      : CA�ļ��б�
 * caFileCount  : CA�б���
 * crlFiles     : CRL�ļ��б�
 * crlFileCount : CRL�б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: caFiles���԰�������֤��,
 *       1)���ŵ�CA֤��;
 *       2)���ŵ���ǩ���ն��û�֤��. ��ʱ,֤���CA��־λ���Բ�����
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
 * ����: ׷��һ��֤����
 *
 * ����:
 * config        : SSL���ö���
 * certFiles     : ֤���ļ��б�
 * certFileCount : �б���
 * keyFile       : ˽Կ�ļ�. ��certFiles[0]��Ӧ
 * password      : ˽Կ�ļ�����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: ��
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
 * ����: ������֤����
 *
 * ����:
 * config : SSL���ö���
 * level  : ��֤����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: Ĭ�������, server����֤client
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*
 * ����: ���һ��SNI��Ŀ
 *
 * ����:
 * config  : SSL���ö���
 * sniName : SNI������
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: ��
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AddSni(PRO_SSL_SERVER_CONFIG* config,
                          const char*            sniName);

/*
 * ����: ɾ��һ��SNI��Ŀ
 *
 * ����:
 * config  : SSL���ö���
 * sniName : SNI������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_RemoveSni(PRO_SSL_SERVER_CONFIG* config,
                             const char*            sniName);

/*
 * ����: �����ض�SNI��Ŀ��CA֤���б�
 *
 * ����:
 * config       : SSL���ö���
 * sniName      : SNI������
 * caFiles      : CA�ļ��б�
 * caFileCount  : CA�б���
 * crlFiles     : CRL�ļ��б�
 * crlFileCount : CRL�б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: caFiles���԰�������֤��,
 *       1)���ŵ�CA֤��;
 *       2)���ŵ���ǩ���ն��û�֤��. ��ʱ,֤���CA��־λ���Բ�����
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
 * ����: ׷���ض�SNI��Ŀ��һ��֤����
 *
 * ����:
 * config        : SSL���ö���
 * sniName       : SNI������
 * certFiles     : ֤���ļ��б�
 * certFileCount : �б���
 * keyFile       : ˽Կ�ļ�. ��certFiles[0]��Ӧ
 * password      : ˽Կ�ļ�����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: ��
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
 * ����: �����ض�SNI��Ŀ����֤����
 *
 * ����:
 * config : SSL���ö���
 * level  : ��֤����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: Ĭ�������, server����֤client
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSniAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                   const char*            sniName,
                                   PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * ����: ����һ���ͻ���SSL����
 *
 * ����: ��
 *
 * ����ֵ: SSL���ö����NULL
 *
 * ˵��: PRO_SSL_CLIENT_CONFIG�̳���mbedtls_ssl_config. �����Ҫ,����ͨ��
 *       mbedtls����ݸö���
 */
PRO_NET_API
PRO_SSL_CLIENT_CONFIG*
PRO_CALLTYPE
ProSslClientConfig_Create();

/*
 * ����: ɾ��һ���ͻ���SSL����
 *
 * ����:
 * config : SSL���ö���
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_Delete(PRO_SSL_CLIENT_CONFIG* config);

/*
 * ����: ���ü����׼�
 *
 * ����:
 * config     : SSL���ö���
 * suites     : �����׼��б�
 * suiteCount : �б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: �����Ҫ,����ͨ��mbedtls�����ø��ӷḻ�ļ����׼�
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetSuiteList(PRO_SSL_CLIENT_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount);

/*
 * ����: ����ALPN
 *
 * ����:
 * config    : SSL���ö���
 * alpns     : ALPN�б�
 * alpnCount : �б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: ��
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAlpnList(PRO_SSL_CLIENT_CONFIG* config,
                               const char**           alpns,      /* = NULL */
                               size_t                 alpnCount); /* = 0 */

/*
 * ����: �Ƿ�֧��SHA-1֤��
 *
 * ����:
 * config : SSL���ö���
 * enable : true֧��, false��֧��
 *
 * ����ֵ: ��
 *
 * ˵��: Ĭ�ϲ�֧��SHA-1֤��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_EnableSha1Cert(PRO_SSL_CLIENT_CONFIG* config,
                                  bool                   enable);

/*
 * ����: ����CA֤���б�
 *
 * ����:
 * config       : SSL���ö���
 * caFiles      : CA�ļ��б�
 * caFileCount  : CA�б���
 * crlFiles     : CRL�ļ��б�
 * crlFileCount : CRL�б���
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: caFiles���԰�������֤��,
 *       1)���ŵ�CA֤��;
 *       2)���ŵ���ǩ���ն��û�֤��. ��ʱ,֤���CA��־λ���Բ�����
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
 * ����: ����֤����
 *
 * ����:
 * config        : SSL���ö���
 * certFiles     : ֤���ļ��б�
 * certFileCount : �б���
 * keyFile       : ˽Կ�ļ�. ��certFiles[0]��Ӧ
 * password      : ˽Կ�ļ�����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: �ú���ֻ�ܳɹ�����һ��
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
 * ����: ������֤����
 *
 * ����:
 * config : SSL���ö���
 * level  : ��֤����
 *
 * ����ֵ: true�ɹ�, falseʧ��
 *
 * ˵��: Ĭ�������, clientҪ����֤server
 */
PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAuthLevel(PRO_SSL_CLIENT_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level);

/*-------------------------------------------------------------------------*/

/*
 * ����: ����һ�������SSL������
 *
 * ����:
 * config : SSL���ö���
 * sockId : �׽���id
 * nonce  : �Ŷ������. 0��ʾ���Ŷ�
 *
 * ����ֵ: SSL�����Ķ����NULL
 *
 * ˵��: PRO_SSL_CTX�̳���mbedtls_ssl_context. �����Ҫ,����ͨ��mbedtls��
 *       ���ݸö���
 *
 *       nonce����Ϊ����������������Ŷ�,��Ҫ���ڷ�ֹ���ֳ��ڵ�����֤�鱻
 *       ��������. c/s���˱���һ��,һ����Դ��OnAccept(...)��OnConnectOk(...)
 */
PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Creates(const PRO_SSL_SERVER_CONFIG* config,
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce); /* = 0 */

/*
 * ����: ����һ���ͻ���SSL������
 *
 * ����:
 * config         : SSL���ö���
 * serverHostName : server������. �����Ч,�������֤server֤��
 * sockId         : �׽���id
 * nonce          : �Ŷ������. 0��ʾ���Ŷ�
 *
 * ����ֵ: SSL�����Ķ����NULL
 *
 * ˵��: PRO_SSL_CTX�̳���mbedtls_ssl_context. �����Ҫ,����ͨ��mbedtls��
 *       ���ݸö���
 *
 *       nonce����Ϊ����������������Ŷ�,��Ҫ���ڷ�ֹ���ֳ��ڵ�����֤�鱻
 *       ��������. c/s���˱���һ��,һ����Դ��OnAccept(...)��OnConnectOk(...)
 */
PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Createc(const PRO_SSL_CLIENT_CONFIG* config,
                  const char*                  serverHostName, /* = NULL */
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce);         /* = 0 */

/*
 * ����: ɾ��һ��SSL������
 *
 * ����:
 * ctx : SSL�����Ķ���
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_NET_API
void
PRO_CALLTYPE
ProSslCtx_Delete(PRO_SSL_CTX* ctx);

/*
 * ����: ��ȡc/sЭ�̵ĻỰ�����׼�
 *
 * ����:
 * ctx       : SSL�����Ķ���
 * suiteName : ���صļ����׼���
 *
 * ����ֵ: �����׼�id
 *
 * ˵��: SSL/TLS������ɺ��������
 */
PRO_NET_API
PRO_SSL_SUITE_ID
PRO_CALLTYPE
ProSslCtx_GetSuite(PRO_SSL_CTX* ctx,
                   char         suiteName[64]);

/*
 * ����: ��ȡc/sЭ�̵�ALPNЭ����
 *
 * ����:
 * ctx : SSL�����Ķ���
 *
 * ����ֵ: ALPNЭ����. ������NULL
 *
 * ˵��: SSL/TLS������ɺ��������
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
