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

#include "pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_z.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/threading.h"
#include "mbedtls/x509_crt.h"

#include <cassert>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define MAGIC_BYTES (1024 * 16)

/////////////////////////////////////////////////////////////////////////////
////

static
void
pro_entropy_free(mbedtls_entropy_context* entropy)
{
    if (entropy != NULL)
    {
        mbedtls_entropy_free(entropy);
    }
}

static
void
pro_ctr_drbg_free(mbedtls_ctr_drbg_context* rng)
{
    if (rng != NULL)
    {
        mbedtls_ctr_drbg_free(rng);
    }
}

static
void
pro_x509_crt_free(mbedtls_x509_crt* crt)
{
    if (crt != NULL)
    {
        mbedtls_x509_crt_free(crt);
    }
}

static
void
pro_x509_crl_free(mbedtls_x509_crl* crl)
{
    if (crl != NULL)
    {
        mbedtls_x509_crl_free(crl);
    }
}

static
void
pro_pk_free(mbedtls_pk_context* pk)
{
    if (pk != NULL)
    {
        mbedtls_pk_free(pk);
    }
}

static
void
pro_ssl_config_free(mbedtls_ssl_config* conf)
{
    if (conf != NULL)
    {
        mbedtls_ssl_config_free(conf);
    }
}

static
void
pro_ssl_free(mbedtls_ssl_context* ssl)
{
    if (ssl != NULL)
    {
        mbedtls_ssl_free(ssl);
    }
}

/*-------------------------------------------------------------------------*/

struct PRO_SSL_AUTH_ITEM
{
    PRO_SSL_AUTH_ITEM()
    {
        ca    = NULL;
        crl   = NULL;
        level = PRO_SSL_AUTHLV_NONE;
    }

    bool Init()
    {
        return (true);
    }

    void Fini()
    {
        int i = 0;
        int c = (int)keys.size();

        for (; i < c; ++i)
        {
            mbedtls_pk_context* const key = keys[i];
            pro_pk_free(key);
            ProFree(key);
        }

        i = 0;
        c = (int)certs.size();

        for (; i < c; ++i)
        {
            mbedtls_x509_crt* const cert = certs[i];
            pro_x509_crt_free(cert);
            ProFree(cert);
        }

        keys.clear();
        certs.clear();
        pro_x509_crl_free(crl);
        pro_x509_crt_free(ca);
        ProFree(crl);
        ProFree(ca);
        crl = NULL;
        ca  = NULL;
    }

    mbedtls_x509_crt*                  ca;
    mbedtls_x509_crl*                  crl;
    CProStlVector<mbedtls_x509_crt*>   certs;
    CProStlVector<mbedtls_pk_context*> keys;
    PRO_SSL_AUTH_LEVEL                 level;
};

struct PRO_SSL_SUITE_LIST
{
    PRO_SSL_SUITE_LIST()
    {
        suites = NULL;
    }

    bool Init()
    {
        suites = new CProStlVector<int>;

        return (true);
    }

    void Fini()
    {
        delete suites;
        suites = NULL;
    }

    CProStlVector<int>* suites;
};

struct PRO_SSL_ALPN_LIST
{
    PRO_SSL_ALPN_LIST()
    {
        alpns = NULL;
    }

    bool Init()
    {
        alpns = new CProStlVector<char*>;

        return (true);
    }

    void Fini()
    {
        if (alpns == NULL)
        {
            return;
        }

        int       i = 0;
        const int c = (int)alpns->size();

        for (; i < c; ++i)
        {
            ProFree((*alpns)[i]);
        }

        delete alpns;
        alpns = NULL;
    }

    CProStlVector<char*>* alpns;
};

struct PRO_SSL_SERVER_CONFIG : public mbedtls_ssl_config
{
    bool Init()
    {
        if (!auth.Init())
        {
            return (false);
        }

        if (!suites.Init())
        {
            auth.Fini();

            return (false);
        }

        if (!alpns.Init())
        {
            suites.Fini();
            auth.Fini();

            return (false);
        }

        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&rng);
        mbedtls_ssl_config_init(this);

        sha1Profile = mbedtls_x509_crt_profile_default;
        sha1Profile.allowed_mds |= MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1);

        return (true);
    }

    void Fini()
    {
        pro_ssl_config_free(this);

        CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator       itr = sni2Auth.begin();
        CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator const end = sni2Auth.end();

        for (; itr != end; ++itr)
        {
            itr->second.Fini();
        }

        alpns.Fini();
        suites.Fini();
        sni2Auth.clear();
        auth.Fini();
        pro_ctr_drbg_free(&rng);
        pro_entropy_free(&entropy);
    }

    mbedtls_entropy_context                      entropy;
    mbedtls_ctr_drbg_context                     rng;
    PRO_SSL_AUTH_ITEM                            auth;
    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM> sni2Auth;
    PRO_SSL_SUITE_LIST                           suites;
    PRO_SSL_ALPN_LIST                            alpns;
    mbedtls_x509_crt_profile                     sha1Profile;

    DECLARE_SGI_POOL(0);
};

struct PRO_SSL_CLIENT_CONFIG : public mbedtls_ssl_config
{
    bool Init()
    {
        if (!auth.Init())
        {
            return (false);
        }

        if (!suites.Init())
        {
            auth.Fini();

            return (false);
        }

        if (!alpns.Init())
        {
            suites.Fini();
            auth.Fini();

            return (false);
        }

        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&rng);
        mbedtls_ssl_config_init(this);

        sha1Profile = mbedtls_x509_crt_profile_default;
        sha1Profile.allowed_mds |= MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1);

        return (true);
    }

    void Fini()
    {
        pro_ssl_config_free(this);
        alpns.Fini();
        suites.Fini();
        auth.Fini();
        pro_ctr_drbg_free(&rng);
        pro_entropy_free(&entropy);
    }

    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context rng;
    PRO_SSL_AUTH_ITEM        auth;
    PRO_SSL_SUITE_LIST       suites;
    PRO_SSL_ALPN_LIST        alpns;
    mbedtls_x509_crt_profile sha1Profile;

    DECLARE_SGI_POOL(0);
};

struct PRO_SSL_CTX : public mbedtls_ssl_context
{
    PRO_SSL_CTX(PRO_INT64 theSockId, PRO_UINT64 theNonce)
        : sockId(theSockId), nonce(pbsd_hton64(theNonce))
    {
        sentBytes = 0;
        recvBytes = 0;
    }

    const PRO_INT64  sockId;
    const PRO_UINT64 nonce; /* network byte order */
    PRO_INT64        sentBytes;
    PRO_INT64        recvBytes;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

static
void
ProThreadingMutexInit_i(mbedtls_threading_mutex_t* mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    mutex->mutex = new CProThreadMutex;
}

static
void
ProThreadingMutexFree_i(mbedtls_threading_mutex_t* mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    delete (CProThreadMutex*)mutex->mutex;
    mutex->mutex = NULL;
}

static
int
ProThreadingMutexLock_i(mbedtls_threading_mutex_t* mutex)
{
    if (mutex == NULL || mutex->mutex == NULL)
    {
        return (MBEDTLS_ERR_THREADING_BAD_INPUT_DATA);
    }

    ((CProThreadMutex*)mutex->mutex)->Lock();

    return (0);
}

static
int
ProThreadingMutexUnlock_i(mbedtls_threading_mutex_t* mutex)
{
    if (mutex == NULL || mutex->mutex == NULL)
    {
        return (MBEDTLS_ERR_THREADING_BAD_INPUT_DATA);
    }

    ((CProThreadMutex*)mutex->mutex)->Unlock();

    return (0);
}

static
int
ProEntropys_i(void*          ctx,
              unsigned char* buf,
              size_t         size)
{
    PRO_SSL_SERVER_CONFIG* const config = (PRO_SSL_SERVER_CONFIG*)ctx;
    if (config == NULL)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    return (mbedtls_entropy_func(&config->entropy, buf, size));
}

static
int
ProEntropyc_i(void*          ctx,
              unsigned char* buf,
              size_t         size)
{
    PRO_SSL_CLIENT_CONFIG* const config = (PRO_SSL_CLIENT_CONFIG*)ctx;
    if (config == NULL)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    return (mbedtls_entropy_func(&config->entropy, buf, size));
}

static
int
ProRngs_i(void*          ctx,
          unsigned char* buf,
          size_t         size)
{
    PRO_SSL_SERVER_CONFIG* const config = (PRO_SSL_SERVER_CONFIG*)ctx;
    if (config == NULL)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    return (mbedtls_ctr_drbg_random(&config->rng, buf, size));
}

static
int
ProRngc_i(void*          ctx,
          unsigned char* buf,
          size_t         size)
{
    PRO_SSL_CLIENT_CONFIG* const config = (PRO_SSL_CLIENT_CONFIG*)ctx;
    if (config == NULL)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    return (mbedtls_ctr_drbg_random(&config->rng, buf, size));
}

static
int
ProSni_i(void*                ctx,
         mbedtls_ssl_context* ssl,
         const unsigned char* name,
         size_t               nameLen)
{
    PRO_SSL_SERVER_CONFIG* const config = (PRO_SSL_SERVER_CONFIG*)ctx;
    if (config == NULL)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (ssl == NULL || name == NULL || name[0] == '\0' || nameLen == 0)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    const CProStlString sniName((char*)name, nameLen);

    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::const_iterator const itr =
        config->sni2Auth.find(sniName);
    if (itr == config->sni2Auth.end())
    {
        return (-1);
    }

    const PRO_SSL_AUTH_ITEM& ai = itr->second;
    if (ai.certs.size() == 0 || ai.keys.size() == 0)
    {
        return (-1);
    }

    mbedtls_ssl_set_hs_authmode(ssl, ai.level);
    if (ai.ca != NULL)
    {
        mbedtls_ssl_set_hs_ca_chain(ssl, ai.ca, ai.crl);
    }

    int ret = -1;

    int       i = 0;
    const int c = (int)ai.certs.size();

    for (; i < c; ++i)
    {
        ret = mbedtls_ssl_set_hs_own_cert(ssl, ai.certs[i], ai.keys[i]);
        if (ret != 0)
        {
            break;
        }
    }

    return (ret);
}

static
int
ProSend_i(void*                ctx,
          const unsigned char* buf,
          size_t               size)
{
    PRO_SSL_CTX* const ctx2 = (PRO_SSL_CTX*)ctx;
    if (ctx2 == NULL || ctx2->sockId == -1)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (buf == NULL && size > 0)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    int sentSize  = 0;
    int errorCode = 0;

    if (size > 0 && ctx2->nonce != 0 && ctx2->sentBytes < MAGIC_BYTES)
    {
        if (size > MAGIC_BYTES)
        {
            size = MAGIC_BYTES;
        }

        unsigned char* const buf2 = (unsigned char*)ProMalloc(size);
        if (buf2 == NULL)
        {
            return (MBEDTLS_ERR_NET_SEND_FAILED);
        }

        memcpy(buf2, buf, size);

        unsigned char*       const k0    = (unsigned char*)&ctx2->nonce;
        const unsigned char* const kk    = k0 + sizeof(PRO_UINT64);
        unsigned char*             k     = k0 + ctx2->sentBytes % sizeof(PRO_UINT64);
        unsigned char*             p     = buf2;
        const int                  msize = (int)(MAGIC_BYTES - ctx2->sentBytes);

        for (int i = 0; i < (int)size && i < msize; ++i)
        {
            *p ^= *k;
            ++p;
            ++k;
            if (k == kk)
            {
                k = k0;
            }
        }

        sentSize  = pbsd_send(ctx2->sockId, buf2, (int)size, 0);
        errorCode = pbsd_errno((void*)&pbsd_send);

        ProFree(buf2);
    }
    else
    {
        sentSize  = pbsd_send(ctx2->sockId, buf, (int)size, 0);
        errorCode = pbsd_errno((void*)&pbsd_send);
    }

    assert(sentSize <= (int)size);
    if (sentSize > (int)size)
    {
        return (MBEDTLS_ERR_NET_SEND_FAILED);
    }

    if (sentSize > 0)
    {
        ctx2->sentBytes += sentSize;

        return (sentSize);
    }

    if (sentSize == 0)
    {
        if (size == 0) /* !!! */
        {
            return (0);
        }
        else
        {
            return (MBEDTLS_ERR_SSL_WANT_WRITE);
        }
    }

    if (errorCode == PBSD_EWOULDBLOCK)
    {
        return (MBEDTLS_ERR_SSL_WANT_WRITE);
    }
    else
    {
        return (MBEDTLS_ERR_NET_SEND_FAILED);
    }
}

static
int
ProRecv_i(void*          ctx,
          unsigned char* buf,
          size_t         size)
{
    PRO_SSL_CTX* const ctx2 = (PRO_SSL_CTX*)ctx;
    if (ctx2 == NULL || ctx2->sockId == -1)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    if (buf == NULL || size == 0)
    {
        return (MBEDTLS_ERR_SSL_BAD_INPUT_DATA);
    }

    const int recvSize  = pbsd_recv(ctx2->sockId, buf, (int)size, 0);
    const int errorCode = pbsd_errno((void*)&pbsd_recv);

    assert(recvSize <= (int)size);
    if (recvSize > (int)size)
    {
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }

    if (recvSize > 0 && ctx2->nonce != 0 && ctx2->recvBytes < MAGIC_BYTES)
    {
        unsigned char*       const k0    = (unsigned char*)&ctx2->nonce;
        const unsigned char* const kk    = k0 + sizeof(PRO_UINT64);
        unsigned char*             k     = k0 + ctx2->recvBytes % sizeof(PRO_UINT64);
        unsigned char*             p     = buf;
        const int                  msize = (int)(MAGIC_BYTES - ctx2->recvBytes);

        for (int i = 0; i < recvSize && i < msize; ++i)
        {
            *p ^= *k;
            ++p;
            ++k;
            if (k == kk)
            {
                k = k0;
            }
        }
    }

    if (recvSize >= 0)
    {
        ctx2->recvBytes += recvSize;

        return (recvSize);
    }

    if (errorCode == PBSD_EWOULDBLOCK)
    {
        return (MBEDTLS_ERR_SSL_WANT_READ);
    }
    else
    {
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }
}

/////////////////////////////////////////////////////////////////////////////
////

void
PRO_CALLTYPE
ProSslInit()
{
    static bool s_flag = false;
    if (s_flag)
    {
        return;
    }
    s_flag = true;

    mbedtls_threading_set_alt(
        &ProThreadingMutexInit_i,
        &ProThreadingMutexFree_i,
        &ProThreadingMutexLock_i,
        &ProThreadingMutexUnlock_i
        );
}

PRO_NET_API
PRO_SSL_SERVER_CONFIG*
PRO_CALLTYPE
ProSslServerConfig_Create()
{
    ProSslInit();

    PRO_SSL_SERVER_CONFIG* const config = new PRO_SSL_SERVER_CONFIG;
    const char*            const pers   = "s";

    if (!config->Init())
    {
        delete config;

        return (NULL);
    }

    if (mbedtls_ctr_drbg_seed(&config->rng, &ProEntropys_i, config,
        (unsigned char*)pers, strlen(pers)) != 0)
    {
        goto EXIT;
    }

    mbedtls_ssl_conf_rng(config, &ProRngs_i, config);

    if (mbedtls_ssl_config_defaults(config, MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
    {
        goto EXIT;
    }

    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_CCM);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_256_CCM);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_CCM);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_128_CCM);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(0);

    mbedtls_ssl_conf_ciphersuites(config, &(*config->suites.suites)[0]);

    return (config);

EXIT:

    config->Fini();
    delete config;

    return (NULL);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_Delete(PRO_SSL_SERVER_CONFIG* config)
{
    if (config == NULL)
    {
        return;
    }

    config->Fini();
    delete config;
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSuiteList(PRO_SSL_SERVER_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount)
{
    assert(config != NULL);
    assert(suites != NULL);
    assert(suiteCount > 0);
    if (config == NULL || suites == NULL || suiteCount == 0)
    {
        return (false);
    }

    PRO_SSL_SUITE_LIST suites2;
    if (!suites2.Init())
    {
        return (false);
    }

    for (int i = 0; i < (int)suiteCount; ++i)
    {
        suites2.suites->push_back(suites[i]);
    }

    suites2.suites->push_back(0);

    mbedtls_ssl_conf_ciphersuites(config, &(*suites2.suites)[0]);

    config->suites.Fini();
    config->suites = suites2;

    return (true);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAlpnList(PRO_SSL_SERVER_CONFIG* config,
                               const char**           alpns,     /* = NULL */
                               size_t                 alpnCount) /* = 0 */
{
    assert(config != NULL);
    if (config == NULL)
    {
        return (false);
    }

    if (alpns == NULL || alpnCount == 0)
    {
        config->alpn_list = NULL;
        config->alpns.Fini();

        return (true);
    }

    for (int i = 0; i < (int)alpnCount; ++i)
    {
        if (alpns[i] == NULL || alpns[i][0] == '\0')
        {
            return (false);
        }
    }

    PRO_SSL_ALPN_LIST alpns2;
    if (!alpns2.Init())
    {
        return (false);
    }

    for (int j = 0; j < (int)alpnCount; ++j)
    {
        char* const p = (char*)ProCalloc(1, strlen(alpns[j]) + 1);
        if (p == NULL)
        {
            break;
        }

        strcpy(p, alpns[j]);
        alpns2.alpns->push_back(p);
    }

    alpns2.alpns->push_back(NULL);

    if (alpns2.alpns->size() != alpnCount + 1)
    {
        alpns2.Fini();

        return (false);
    }

    if (mbedtls_ssl_conf_alpn_protocols(config, (const char**)&(*alpns2.alpns)[0]) != 0)
    {
        alpns2.Fini();

        return (false);
    }

    config->alpns.Fini();
    config->alpns = alpns2;

    return (true);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_EnableSha1Cert(PRO_SSL_SERVER_CONFIG* config,
                                  bool                   enable)
{
    assert(config != NULL);
    if (config == NULL)
    {
        return;
    }

    if (enable)
    {
        mbedtls_ssl_conf_cert_profile(config, &config->sha1Profile);
    }
    else
    {
        mbedtls_ssl_conf_cert_profile(config, &mbedtls_x509_crt_profile_default);
    }
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetCaList(PRO_SSL_SERVER_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,     /* = NULL */
                             size_t                 crlFileCount) /* = 0 */
{
    assert(config != NULL);
    assert(caFiles != NULL);
    assert(caFileCount > 0);
    if (config == NULL || caFiles == NULL || caFileCount == 0)
    {
        return (false);
    }

    mbedtls_x509_crt* const ca  = (mbedtls_x509_crt*)ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_x509_crl*       crl = (mbedtls_x509_crl*)ProCalloc(1, sizeof(mbedtls_x509_crl));
    if (ca == NULL || crl == NULL)
    {
        ProFree(crl);
        ProFree(ca);

        return(false);
    }

    mbedtls_x509_crt_init(ca);
    mbedtls_x509_crl_init(crl);

    for (int i = 0; i < (int)caFileCount; ++i)
    {
        if (caFiles[i] == NULL || caFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(ca, caFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (crlFiles != NULL && crlFileCount > 0)
    {
        for (int j = 0; j < (int)crlFileCount; ++j)
        {
            if (crlFiles[j] == NULL || crlFiles[j][0] == '\0')
            {
                goto EXIT;
            }

            if (mbedtls_x509_crl_parse_file(crl, crlFiles[j]) != 0)
            {
                goto EXIT;
            }
        }
    }
    else
    {
        pro_x509_crl_free(crl);
        crl = NULL;
    }

    mbedtls_ssl_conf_ca_chain(config, ca, crl);

    pro_x509_crl_free(config->auth.crl);
    pro_x509_crt_free(config->auth.ca);
    ProFree(config->auth.crl);
    ProFree(config->auth.ca);

    config->auth.ca  = ca;
    config->auth.crl = crl;

    return (true);

EXIT:

    pro_x509_crl_free(crl);
    pro_x509_crt_free(ca);
    ProFree(crl);
    ProFree(ca);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AppendCertChain(PRO_SSL_SERVER_CONFIG* config,
                                   const char**           certFiles,
                                   size_t                 certFileCount,
                                   const char*            keyFile,
                                   const char*            password) /* = NULL */
{
    assert(config != NULL);
    assert(certFiles != NULL);
    assert(certFileCount > 0);
    assert(keyFile != NULL);
    assert(keyFile[0] != '\0');
    if (config == NULL || certFiles == NULL || certFileCount == 0 ||
        keyFile == NULL || keyFile[0] == '\0')
    {
        return (false);
    }

    mbedtls_x509_crt*   const cert = (mbedtls_x509_crt*)  ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_pk_context* const key  = (mbedtls_pk_context*)ProCalloc(1, sizeof(mbedtls_pk_context));
    if (cert == NULL || key == NULL)
    {
        ProFree(key);
        ProFree(cert);

        return(false);
    }

    mbedtls_x509_crt_init(cert);
    mbedtls_pk_init(key);

    for (int i = 0; i < (int)certFileCount; ++i)
    {
        if (certFiles[i] == NULL || certFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(cert, certFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (mbedtls_pk_parse_keyfile(key, keyFile, password) != 0)
    {
        goto EXIT;
    }

    if (mbedtls_ssl_conf_own_cert(config, cert, key) != 0)
    {
        goto EXIT;
    }

    config->auth.certs.push_back(cert);
    config->auth.keys.push_back(key);

    return (true);

EXIT:

    pro_pk_free(key);
    pro_x509_crt_free(cert);
    ProFree(key);
    ProFree(cert);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level)
{
    assert(config != NULL);
    assert(
        level == PRO_SSL_AUTHLV_NONE     ||
        level == PRO_SSL_AUTHLV_OPTIONAL ||
        level == PRO_SSL_AUTHLV_REQUIRED
        );
    if (config == NULL
        ||
        level != PRO_SSL_AUTHLV_NONE     &&
        level != PRO_SSL_AUTHLV_OPTIONAL &&
        level != PRO_SSL_AUTHLV_REQUIRED)
    {
        return (false);
    }

    mbedtls_ssl_conf_authmode(config, level);

    return (true);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AddSni(PRO_SSL_SERVER_CONFIG* config,
                          const char*            sniName)
{
    assert(config != NULL);
    assert(sniName != NULL);
    assert(sniName[0] != '\0');
    if (config == NULL || sniName == NULL || sniName[0] == '\0')
    {
        return (false);
    }

    if (config->sni2Auth.size() == 0) /* first time */
    {
        mbedtls_ssl_conf_sni(config, &ProSni_i, config);
    }

    if (config->sni2Auth.find(sniName) == config->sni2Auth.end())
    {
        config->sni2Auth[sniName] = PRO_SSL_AUTH_ITEM();
    }

    return (true);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslServerConfig_RemoveSni(PRO_SSL_SERVER_CONFIG* config,
                             const char*            sniName)
{
    assert(config != NULL);
    if (config == NULL || sniName == NULL || sniName[0] == '\0')
    {
        return;
    }

    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator const itr =
        config->sni2Auth.find(sniName);
    if (itr == config->sni2Auth.end())
    {
        return;
    }

    itr->second.Fini();
    config->sni2Auth.erase(itr);

    if (config->sni2Auth.size() == 0)
    {
        mbedtls_ssl_conf_sni(config, NULL, NULL);
    }
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSniCaList(PRO_SSL_SERVER_CONFIG* config,
                                const char*            sniName,
                                const char**           caFiles,
                                size_t                 caFileCount,
                                const char**           crlFiles,     /* = NULL */
                                size_t                 crlFileCount) /* = 0 */
{
    assert(config != NULL);
    assert(sniName != NULL);
    assert(sniName[0] != '\0');
    assert(caFiles != NULL);
    assert(caFileCount > 0);
    if (config == NULL || sniName == NULL || sniName[0] == '\0' ||
        caFiles == NULL || caFileCount == 0)
    {
        return (false);
    }

    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator const itr =
        config->sni2Auth.find(sniName);
    if (itr == config->sni2Auth.end())
    {
        return (false);
    }

    PRO_SSL_AUTH_ITEM& ai = itr->second;

    mbedtls_x509_crt* const ca  = (mbedtls_x509_crt*)ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_x509_crl*       crl = (mbedtls_x509_crl*)ProCalloc(1, sizeof(mbedtls_x509_crl));
    if (ca == NULL || crl == NULL)
    {
        ProFree(crl);
        ProFree(ca);

        return(false);
    }

    mbedtls_x509_crt_init(ca);
    mbedtls_x509_crl_init(crl);

    for (int i = 0; i < (int)caFileCount; ++i)
    {
        if (caFiles[i] == NULL || caFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(ca, caFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (crlFiles != NULL && crlFileCount > 0)
    {
        for (int j = 0; j < (int)crlFileCount; ++j)
        {
            if (crlFiles[j] == NULL || crlFiles[j][0] == '\0')
            {
                goto EXIT;
            }

            if (mbedtls_x509_crl_parse_file(crl, crlFiles[j]) != 0)
            {
                goto EXIT;
            }
        }
    }
    else
    {
        pro_x509_crl_free(crl);
        crl = NULL;
    }

    pro_x509_crl_free(ai.crl);
    pro_x509_crt_free(ai.ca);
    ProFree(ai.crl);
    ProFree(ai.ca);

    ai.ca  = ca;
    ai.crl = crl;

    return (true);

EXIT:

    pro_x509_crl_free(crl);
    pro_x509_crt_free(ca);
    ProFree(crl);
    ProFree(ca);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_AppendSniCertChain(PRO_SSL_SERVER_CONFIG* config,
                                      const char*            sniName,
                                      const char**           certFiles,
                                      size_t                 certFileCount,
                                      const char*            keyFile,
                                      const char*            password) /* = NULL */
{
    assert(config != NULL);
    assert(sniName != NULL);
    assert(sniName[0] != '\0');
    assert(certFiles != NULL);
    assert(certFileCount > 0);
    assert(keyFile != NULL);
    assert(keyFile[0] != '\0');
    if (config == NULL || sniName == NULL || sniName[0] == '\0' ||
        certFiles == NULL || certFileCount == 0 || keyFile == NULL || keyFile[0] == '\0')
    {
        return (false);
    }

    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator const itr =
        config->sni2Auth.find(sniName);
    if (itr == config->sni2Auth.end())
    {
        return (false);
    }

    PRO_SSL_AUTH_ITEM& ai = itr->second;

    mbedtls_x509_crt*   const cert = (mbedtls_x509_crt*)  ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_pk_context* const key  = (mbedtls_pk_context*)ProCalloc(1, sizeof(mbedtls_pk_context));
    if (cert == NULL || key == NULL)
    {
        ProFree(key);
        ProFree(cert);

        return(false);
    }

    mbedtls_x509_crt_init(cert);
    mbedtls_pk_init(key);

    for (int i = 0; i < (int)certFileCount; ++i)
    {
        if (certFiles[i] == NULL || certFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(cert, certFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (mbedtls_pk_parse_keyfile(key, keyFile, password) != 0)
    {
        goto EXIT;
    }

    ai.certs.push_back(cert);
    ai.keys.push_back(key);

    return (true);

EXIT:

    pro_pk_free(key);
    pro_x509_crt_free(cert);
    ProFree(key);
    ProFree(cert);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslServerConfig_SetSniAuthLevel(PRO_SSL_SERVER_CONFIG* config,
                                   const char*            sniName,
                                   PRO_SSL_AUTH_LEVEL     level)
{
    assert(config != NULL);
    assert(sniName != NULL);
    assert(sniName[0] != '\0');
    assert(
        level == PRO_SSL_AUTHLV_NONE     ||
        level == PRO_SSL_AUTHLV_OPTIONAL ||
        level == PRO_SSL_AUTHLV_REQUIRED
        );
    if (config == NULL || sniName == NULL || sniName[0] == '\0'
        ||
        level != PRO_SSL_AUTHLV_NONE     &&
        level != PRO_SSL_AUTHLV_OPTIONAL &&
        level != PRO_SSL_AUTHLV_REQUIRED)
    {
        return (false);
    }

    CProStlMap<CProStlString, PRO_SSL_AUTH_ITEM>::iterator const itr =
        config->sni2Auth.find(sniName);
    if (itr == config->sni2Auth.end())
    {
        return (false);
    }

    itr->second.level = level;

    return (true);
}

/*-------------------------------------------------------------------------*/

PRO_NET_API
PRO_SSL_CLIENT_CONFIG*
PRO_CALLTYPE
ProSslClientConfig_Create()
{
    ProSslInit();

    PRO_SSL_CLIENT_CONFIG* const config = new PRO_SSL_CLIENT_CONFIG;
    const char*            const pers   = "c";

    if (!config->Init())
    {
        delete config;

        return (NULL);
    }

    if (mbedtls_ctr_drbg_seed(&config->rng, &ProEntropyc_i, config,
        (unsigned char*)pers, strlen(pers)) != 0)
    {
        goto EXIT;
    }

    mbedtls_ssl_conf_rng(config, &ProRngc_i, config);

    if (mbedtls_ssl_config_defaults(config, MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
    {
        goto EXIT;
    }

    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_256_CCM);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_256_CCM);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_AES_128_CCM);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_AES_128_CCM);
    config->suites.suites->push_back(PRO_SSL_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(PRO_SSL_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    config->suites.suites->push_back(0);

    mbedtls_ssl_conf_ciphersuites(config, &(*config->suites.suites)[0]);

    return (config);

EXIT:

    config->Fini();
    delete config;

    return (NULL);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_Delete(PRO_SSL_CLIENT_CONFIG* config)
{
    if (config == NULL)
    {
        return;
    }

    config->Fini();
    delete config;
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetSuiteList(PRO_SSL_CLIENT_CONFIG*  config,
                                const PRO_SSL_SUITE_ID* suites,
                                size_t                  suiteCount)
{
    assert(config != NULL);
    assert(suites != NULL);
    assert(suiteCount > 0);
    if (config == NULL || suites == NULL || suiteCount == 0)
    {
        return (false);
    }

    PRO_SSL_SUITE_LIST suites2;
    if (!suites2.Init())
    {
        return (false);
    }

    for (int i = 0; i < (int)suiteCount; ++i)
    {
        suites2.suites->push_back(suites[i]);
    }

    suites2.suites->push_back(0);

    mbedtls_ssl_conf_ciphersuites(config, &(*suites2.suites)[0]);

    config->suites.Fini();
    config->suites = suites2;

    return (true);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAlpnList(PRO_SSL_CLIENT_CONFIG* config,
                               const char**           alpns,     /* = NULL */
                               size_t                 alpnCount) /* = 0 */
{
    assert(config != NULL);
    if (config == NULL)
    {
        return (false);
    }

    if (alpns == NULL || alpnCount == 0)
    {
        config->alpn_list = NULL;
        config->alpns.Fini();

        return (true);
    }

    for (int i = 0; i < (int)alpnCount; ++i)
    {
        if (alpns[i] == NULL || alpns[i][0] == '\0')
        {
            return (false);
        }
    }

    PRO_SSL_ALPN_LIST alpns2;
    if (!alpns2.Init())
    {
        return (false);
    }

    for (int j = 0; j < (int)alpnCount; ++j)
    {
        char* const p = (char*)ProCalloc(1, strlen(alpns[j]) + 1);
        if (p == NULL)
        {
            break;
        }

        strcpy(p, alpns[j]);
        alpns2.alpns->push_back(p);
    }

    alpns2.alpns->push_back(NULL);

    if (alpns2.alpns->size() != alpnCount + 1)
    {
        alpns2.Fini();

        return (false);
    }

    if (mbedtls_ssl_conf_alpn_protocols(config, (const char**)&(*alpns2.alpns)[0]) != 0)
    {
        alpns2.Fini();

        return (false);
    }

    config->alpns.Fini();
    config->alpns = alpns2;

    return (true);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslClientConfig_EnableSha1Cert(PRO_SSL_CLIENT_CONFIG* config,
                                  bool                   enable)
{
    assert(config != NULL);
    if (config == NULL)
    {
        return;
    }

    if (enable)
    {
        mbedtls_ssl_conf_cert_profile(config, &config->sha1Profile);
    }
    else
    {
        mbedtls_ssl_conf_cert_profile(config, &mbedtls_x509_crt_profile_default);
    }
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetCaList(PRO_SSL_CLIENT_CONFIG* config,
                             const char**           caFiles,
                             size_t                 caFileCount,
                             const char**           crlFiles,     /* = NULL */
                             size_t                 crlFileCount) /* = 0 */
{
    assert(config != NULL);
    assert(caFiles != NULL);
    assert(caFileCount > 0);
    if (config == NULL || caFiles == NULL || caFileCount == 0)
    {
        return (false);
    }

    mbedtls_x509_crt* const ca  = (mbedtls_x509_crt*)ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_x509_crl*       crl = (mbedtls_x509_crl*)ProCalloc(1, sizeof(mbedtls_x509_crl));
    if (ca == NULL || crl == NULL)
    {
        ProFree(crl);
        ProFree(ca);

        return(false);
    }

    mbedtls_x509_crt_init(ca);
    mbedtls_x509_crl_init(crl);

    for (int i = 0; i < (int)caFileCount; ++i)
    {
        if (caFiles[i] == NULL || caFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(ca, caFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (crlFiles != NULL && crlFileCount > 0)
    {
        for (int j = 0; j < (int)crlFileCount; ++j)
        {
            if (crlFiles[j] == NULL || crlFiles[j][0] == '\0')
            {
                goto EXIT;
            }

            if (mbedtls_x509_crl_parse_file(crl, crlFiles[j]) != 0)
            {
                goto EXIT;
            }
        }
    }
    else
    {
        pro_x509_crl_free(crl);
        crl = NULL;
    }

    mbedtls_ssl_conf_ca_chain(config, ca, crl);

    pro_x509_crl_free(config->auth.crl);
    pro_x509_crt_free(config->auth.ca);
    ProFree(config->auth.crl);
    ProFree(config->auth.ca);

    config->auth.ca  = ca;
    config->auth.crl = crl;

    return (true);

EXIT:

    pro_x509_crl_free(crl);
    pro_x509_crt_free(ca);
    ProFree(crl);
    ProFree(ca);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetCertChain(PRO_SSL_CLIENT_CONFIG* config,
                                const char**           certFiles,
                                size_t                 certFileCount,
                                const char*            keyFile,
                                const char*            password) /* = NULL */
{
    assert(config != NULL);
    assert(certFiles != NULL);
    assert(certFileCount > 0);
    assert(keyFile != NULL);
    assert(keyFile[0] != '\0');
    if (config == NULL || certFiles == NULL || certFileCount == 0 ||
        keyFile == NULL || keyFile[0] == '\0')
    {
        return (false);
    }

    assert(config->auth.certs.size() == 0);
    if (config->auth.certs.size() != 0)
    {
        return (false);
    }

    mbedtls_x509_crt*   const cert = (mbedtls_x509_crt*)  ProCalloc(1, sizeof(mbedtls_x509_crt));
    mbedtls_pk_context* const key  = (mbedtls_pk_context*)ProCalloc(1, sizeof(mbedtls_pk_context));
    if (cert == NULL || key == NULL)
    {
        ProFree(key);
        ProFree(cert);

        return(false);
    }

    mbedtls_x509_crt_init(cert);
    mbedtls_pk_init(key);

    for (int i = 0; i < (int)certFileCount; ++i)
    {
        if (certFiles[i] == NULL || certFiles[i][0] == '\0')
        {
            goto EXIT;
        }

        if (mbedtls_x509_crt_parse_file(cert, certFiles[i]) != 0)
        {
            goto EXIT;
        }
    }

    if (mbedtls_pk_parse_keyfile(key, keyFile, password) != 0)
    {
        goto EXIT;
    }

    if (mbedtls_ssl_conf_own_cert(config, cert, key) != 0)
    {
        goto EXIT;
    }

    config->auth.certs.push_back(cert);
    config->auth.keys.push_back(key);

    return (true);

EXIT:

    pro_pk_free(key);
    pro_x509_crt_free(cert);
    ProFree(key);
    ProFree(cert);

    return (false);
}

PRO_NET_API
bool
PRO_CALLTYPE
ProSslClientConfig_SetAuthLevel(PRO_SSL_CLIENT_CONFIG* config,
                                PRO_SSL_AUTH_LEVEL     level)
{
    assert(config != NULL);
    assert(
        level == PRO_SSL_AUTHLV_NONE     ||
        level == PRO_SSL_AUTHLV_OPTIONAL ||
        level == PRO_SSL_AUTHLV_REQUIRED
        );
    if (config == NULL
        ||
        level != PRO_SSL_AUTHLV_NONE     &&
        level != PRO_SSL_AUTHLV_OPTIONAL &&
        level != PRO_SSL_AUTHLV_REQUIRED)
    {
        return (false);
    }

    mbedtls_ssl_conf_authmode(config, level);

    return (true);
}

/*-------------------------------------------------------------------------*/

PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Creates(const PRO_SSL_SERVER_CONFIG* config,
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce) /* = 0 */
{
    assert(config != NULL);
    assert(sockId != -1);
    if (config == NULL || sockId == -1)
    {
        return (NULL);
    }

    PRO_SSL_CTX* const ctx = new PRO_SSL_CTX(sockId, nonce);
    mbedtls_ssl_init(ctx);

    if (mbedtls_ssl_setup(ctx, config) != 0)
    {
        pro_ssl_free(ctx);
        delete ctx;

        return (NULL);
    }

    mbedtls_ssl_set_bio(ctx, ctx, &ProSend_i, &ProRecv_i, NULL);

    return (ctx);
}

PRO_NET_API
PRO_SSL_CTX*
PRO_CALLTYPE
ProSslCtx_Createc(const PRO_SSL_CLIENT_CONFIG* config,
                  const char*                  serverHostName, /* = NULL */
                  PRO_INT64                    sockId,
                  PRO_UINT64                   nonce)          /* = 0 */
{
    assert(config != NULL);
    assert(sockId != -1);
    if (config == NULL || sockId == -1)
    {
        return (NULL);
    }

    if (serverHostName != NULL && serverHostName[0] == '\0')
    {
        serverHostName = NULL;
    }

    PRO_SSL_CTX* const ctx = new PRO_SSL_CTX(sockId, nonce);
    mbedtls_ssl_init(ctx);

    if (mbedtls_ssl_setup(ctx, config) != 0)
    {
        pro_ssl_free(ctx);
        delete ctx;

        return (NULL);
    }

    if (mbedtls_ssl_set_hostname(ctx, serverHostName) != 0)
    {
        pro_ssl_free(ctx);
        delete ctx;

        return (NULL);
    }

    mbedtls_ssl_set_bio(ctx, ctx, &ProSend_i, &ProRecv_i, NULL);

    return (ctx);
}

PRO_NET_API
void
PRO_CALLTYPE
ProSslCtx_Delete(PRO_SSL_CTX* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    pro_ssl_free(ctx);
    delete ctx;
}

PRO_NET_API
PRO_SSL_SUITE_ID
PRO_CALLTYPE
ProSslCtx_GetSuite(PRO_SSL_CTX* ctx,
                   char         suiteName[64])
{
    strcpy(suiteName, "NONE");

    assert(ctx != NULL);
    if (ctx == NULL)
    {
        return (PRO_SSL_SUITE_NONE);
    }

    const char* const name = mbedtls_ssl_get_ciphersuite(ctx);
    if (name == NULL || name[0] == '\0')
    {
        return (PRO_SSL_SUITE_NONE);
    }

    strncpy_pro(suiteName, 64, name);

    return ((PRO_SSL_SUITE_ID)mbedtls_ssl_get_ciphersuite_id(name));
}

PRO_NET_API
const char*
PRO_CALLTYPE
ProSslCtx_GetAlpn(PRO_SSL_CTX* ctx)
{
    assert(ctx != NULL);
    if (ctx == NULL)
    {
        return (NULL);
    }

    return (mbedtls_ssl_get_alpn_protocol(ctx));
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif
