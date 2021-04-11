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

#include "pro_a.h"
#include "pro_ssl_util.h"
#include "pro_memory_pool.h"
#include "pro_shared.h"
#include "pro_stl.h"
#include "pro_z.h"

#include "mbedtls/md.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/rsa.h"

#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

static
int
Rand_i(void*          act,
       unsigned char* buf,
       size_t         size)
{
    if (buf != NULL && size > 0)
    {
        for (int i = 0; i < (int)size; ++i)
        {
            buf[i] = (unsigned char)(ProRand_0_1() * 255);
        }
    }

    return (0);
}

static
void*
PRO_CALLTYPE
MdNew_i(mbedtls_md_type_t type)
{
    const mbedtls_md_info_t* const info = mbedtls_md_info_from_type(type);
    if (info == NULL)
    {
        return (NULL);
    }

    mbedtls_md_context_t* const ctx =
        (mbedtls_md_context_t*)ProCalloc(1, sizeof(mbedtls_md_context_t));
    if (ctx == NULL)
    {
        return (NULL);
    }

    mbedtls_md_init(ctx);

    if (mbedtls_md_setup(ctx, info, 0) != 0)
    {
        mbedtls_md_free(ctx);
        ProFree(ctx);

        return (NULL);
    }

    return (ctx);
}

static
void
PRO_CALLTYPE
MdDelete_i(void* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    mbedtls_md_free((mbedtls_md_context_t*)ctx);
    ProFree(ctx);
}

static
void
PRO_CALLTYPE
MdInit_i(void* ctx)
{
    assert(ctx != NULL);
    if (ctx == NULL)
    {
        return;
    }

    mbedtls_md_starts((mbedtls_md_context_t*)ctx);
}

static
void
PRO_CALLTYPE
MdUpdate_i(void*       ctx,
           const void* buf,
           size_t      size)
{
    assert(ctx != NULL);
    if (ctx == NULL || buf == NULL || size == 0)
    {
        return;
    }

    mbedtls_md_update(
        (mbedtls_md_context_t*)ctx, (unsigned char*)buf, size);
}

static
void
PRO_CALLTYPE
MdFinish_i(void* ctx,
           void* hashValue)
{
    assert(ctx != NULL);
    assert(hashValue != NULL);
    if (ctx == NULL || hashValue == NULL)
    {
        return;
    }

    mbedtls_md_finish(
        (mbedtls_md_context_t*)ctx, (unsigned char*)hashValue);
}

/////////////////////////////////////////////////////////////////////////////
////

void*
PRO_CALLTYPE
ProMd5New()
{
    return (MdNew_i(MBEDTLS_MD_MD5));
}

void
PRO_CALLTYPE
ProMd5Delete(void* ctx)
{
    MdDelete_i(ctx);
}

void
PRO_CALLTYPE
ProMd5Init(void* ctx)
{
    MdInit_i(ctx);
}

void
PRO_CALLTYPE
ProMd5Update(void*       ctx,
             const void* buf,
             size_t      size)
{
    MdUpdate_i(ctx, buf, size);
}

void
PRO_CALLTYPE
ProMd5Finish(void* ctx,
             char  hashValue[16])
{
    memset(hashValue, 0, 16);

    MdFinish_i(ctx, hashValue);
}

void
PRO_CALLTYPE
ProMd5All(const void* buf,
          size_t      size,
          char        hashValue[16])
{
    memset(hashValue, 0, 16);

    void* const ctx = ProMd5New();
    if (ctx == NULL)
    {
        return;
    }

    ProMd5Init(ctx);
    if (buf != NULL && size > 0)
    {
        ProMd5Update(ctx, buf, size);
    }
    ProMd5Finish(ctx, hashValue);

    ProMd5Delete(ctx);
}

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProSha1New()
{
    return (MdNew_i(MBEDTLS_MD_SHA1));
}

void
PRO_CALLTYPE
ProSha1Delete(void* ctx)
{
    MdDelete_i(ctx);
}

void
PRO_CALLTYPE
ProSha1Init(void* ctx)
{
    MdInit_i(ctx);
}

void
PRO_CALLTYPE
ProSha1Update(void*       ctx,
              const void* buf,
              size_t      size)
{
    MdUpdate_i(ctx, buf, size);
}

void
PRO_CALLTYPE
ProSha1Finish(void* ctx,
              char  hashValue[20])
{
    memset(hashValue, 0, 20);

    MdFinish_i(ctx, hashValue);
}

void
PRO_CALLTYPE
ProSha1All(const void* buf,
           size_t      size,
           char        hashValue[20])
{
    memset(hashValue, 0, 20);

    void* const ctx = ProSha1New();
    if (ctx == NULL)
    {
        return;
    }

    ProSha1Init(ctx);
    if (buf != NULL && size > 0)
    {
        ProSha1Update(ctx, buf, size);
    }
    ProSha1Finish(ctx, hashValue);

    ProSha1Delete(ctx);
}

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProSha256New()
{
    return (MdNew_i(MBEDTLS_MD_SHA256));
}

void
PRO_CALLTYPE
ProSha256Delete(void* ctx)
{
    MdDelete_i(ctx);
}

void
PRO_CALLTYPE
ProSha256Init(void* ctx)
{
    MdInit_i(ctx);
}

void
PRO_CALLTYPE
ProSha256Update(void*       ctx,
                const void* buf,
                size_t      size)
{
    MdUpdate_i(ctx, buf, size);
}

void
PRO_CALLTYPE
ProSha256Finish(void* ctx,
                char  hashValue[32])
{
    memset(hashValue, 0, 32);

    MdFinish_i(ctx, hashValue);
}

void
PRO_CALLTYPE
ProSha256All(const void* buf,
             size_t      size,
             char        hashValue[32])
{
    memset(hashValue, 0, 32);

    void* const ctx = ProSha256New();
    if (ctx == NULL)
    {
        return;
    }

    ProSha256Init(ctx);
    if (buf != NULL && size > 0)
    {
        ProSha256Update(ctx, buf, size);
    }
    ProSha256Finish(ctx, hashValue);

    ProSha256Delete(ctx);
}

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProRipemd160New()
{
    return (MdNew_i(MBEDTLS_MD_RIPEMD160));
}

void
PRO_CALLTYPE
ProRipemd160Delete(void* ctx)
{
    MdDelete_i(ctx);
}

void
PRO_CALLTYPE
ProRipemd160Init(void* ctx)
{
    MdInit_i(ctx);
}

void
PRO_CALLTYPE
ProRipemd160Update(void*       ctx,
                   const void* buf,
                   size_t      size)
{
    MdUpdate_i(ctx, buf, size);
}

void
PRO_CALLTYPE
ProRipemd160Finish(void* ctx,
                   char  hashValue[20])
{
    memset(hashValue, 0, 20);

    MdFinish_i(ctx, hashValue);
}

void
PRO_CALLTYPE
ProRipemd160All(const void* buf,
                size_t      size,
                char        hashValue[20])
{
    memset(hashValue, 0, 20);

    void* const ctx = ProRipemd160New();
    if (ctx == NULL)
    {
        return;
    }

    ProRipemd160Init(ctx);
    if (buf != NULL && size > 0)
    {
        ProRipemd160Update(ctx, buf, size);
    }
    ProRipemd160Finish(ctx, hashValue);

    ProRipemd160Delete(ctx);
}

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProRsaNew()
{
    mbedtls_rsa_context* const ctx =
        (mbedtls_rsa_context*)ProCalloc(1, sizeof(mbedtls_rsa_context));
    if (ctx == NULL)
    {
        return (NULL);
    }

    mbedtls_rsa_init(ctx, MBEDTLS_RSA_PKCS_V15, 0);

    return (ctx);
}

void
PRO_CALLTYPE
ProRsaDelete(void* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    mbedtls_rsa_free((mbedtls_rsa_context*)ctx);
    ProFree(ctx);
}

bool
PRO_CALLTYPE
ProRsaInitPub(void*       ctx,
              const char* nString,
              const char* eString)
{
    assert(ctx != NULL);
    assert(nString != NULL);
    assert(eString != NULL);
    if (ctx == NULL || nString == NULL || eString == NULL)
    {
        return (false);
    }

    mbedtls_rsa_context* const ctx2 = (mbedtls_rsa_context*)ctx;

    if (mbedtls_mpi_read_string(&ctx2->N, 16, nString) != 0 ||
        mbedtls_mpi_read_string(&ctx2->E, 16, eString) != 0)
    {
        return (false);
    }

    ctx2->len = mbedtls_mpi_size(&ctx2->N);

    if (mbedtls_rsa_check_pubkey(ctx2) != 0 || ctx2->len <= 11)
    {
        ctx2->len = 0;

        return (false);
    }

    return (true);
}

bool
PRO_CALLTYPE
ProRsaInitPrivNED(void*       ctx,
                  const char* nString,
                  const char* eString,
                  const char* dString)
{
    assert(ctx != NULL);
    assert(nString != NULL);
    assert(eString != NULL);
    assert(dString != NULL);
    if (ctx == NULL || nString == NULL || eString == NULL || dString == NULL)
    {
        return (false);
    }

    mbedtls_rsa_context* const ctx2 = (mbedtls_rsa_context*)ctx;

    if (mbedtls_mpi_read_string(&ctx2->N, 16, nString) != 0 ||
        mbedtls_mpi_read_string(&ctx2->E, 16, eString) != 0 ||
        mbedtls_mpi_read_string(&ctx2->D, 16, dString) != 0)
    {
        return (false);
    }

    ctx2->len = mbedtls_mpi_size(&ctx2->N);

    if (mbedtls_rsa_complete(ctx2)      != 0 ||
        mbedtls_rsa_check_privkey(ctx2) != 0 ||
        ctx2->len <= 11)
    {
        ctx2->len = 0;

        return (false);
    }

    return (true);
}

bool
PRO_CALLTYPE
ProRsaInitPrivPQE(void*       ctx,
                  const char* pString,
                  const char* qString,
                  const char* eString)
{
    assert(ctx != NULL);
    assert(pString != NULL);
    assert(qString != NULL);
    assert(eString != NULL);
    if (ctx == NULL || pString == NULL || qString == NULL || eString == NULL)
    {
        return (false);
    }

    mbedtls_rsa_context* const ctx2 = (mbedtls_rsa_context*)ctx;

    if (mbedtls_mpi_read_string(&ctx2->P, 16, pString) != 0 ||
        mbedtls_mpi_read_string(&ctx2->Q, 16, qString) != 0 ||
        mbedtls_mpi_read_string(&ctx2->E, 16, eString) != 0)
    {
        return (false);
    }

    if (mbedtls_rsa_complete(ctx2)      != 0 ||
        mbedtls_rsa_check_privkey(ctx2) != 0 ||
        ctx2->len <= 11)
    {
        ctx2->len = 0;

        return (false);
    }

    return (true);
}

void
PRO_CALLTYPE
ProRsaEncrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer)
{
    outputBuffer.clear();

    assert(ctx != NULL);
    if (ctx == NULL || inputBuffer == NULL || inputSize == 0)
    {
        return;
    }

    mbedtls_rsa_context* const ctx2 = (mbedtls_rsa_context*)ctx;
    if (ctx2->len == 0)
    {
        return;
    }

    const size_t blockSize  = ctx2->len - 11;
    const size_t blockCount = (inputSize + blockSize - 1) / blockSize;

    outputBuffer.resize(ctx2->len * blockCount);

    const unsigned char* p = (unsigned char*)inputBuffer;
    unsigned char*       q = (unsigned char*)&outputBuffer[0];

    for (int i = 0; i < (int)blockCount; ++i)
    {
        size_t ilen = 0;
        if (i != (int)blockCount - 1)
        {
            ilen = blockSize;
        }
        else
        {
            ilen = inputSize - blockSize * (blockCount - 1);
        }

        if (mbedtls_rsa_pkcs1_encrypt(
            ctx2, &Rand_i, NULL, MBEDTLS_RSA_PUBLIC, ilen, p, q) != 0)
        {
            outputBuffer.clear();
            break;
        }

        p += ilen;
        q += ctx2->len;
    }
}

void
PRO_CALLTYPE
ProRsaDecrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer)
{
    outputBuffer.clear();

    assert(ctx != NULL);
    if (ctx == NULL || inputBuffer == NULL || inputSize == 0)
    {
        return;
    }

    mbedtls_rsa_context* const ctx2 = (mbedtls_rsa_context*)ctx;
    if (ctx2->len == 0 || inputSize % ctx2->len != 0)
    {
        return;
    }

    const size_t blockSize  = ctx2->len;
    const size_t blockCount = inputSize / blockSize;

    outputBuffer.resize((ctx2->len - 11) * blockCount);

    const unsigned char* p = (unsigned char*)inputBuffer;
    unsigned char*       q = (unsigned char*)&outputBuffer[0];

    for (int i = 0; i < (int)blockCount; ++i)
    {
        size_t olen = 0;
        if (mbedtls_rsa_pkcs1_decrypt(ctx2, &Rand_i, NULL,
            MBEDTLS_RSA_PRIVATE, &olen, p, q, ctx2->len - 11) != 0 ||
            olen == 0 || olen > ctx2->len - 11)
        {
            outputBuffer.clear();
            break;
        }

        if (i != (int)blockCount - 1)
        {
            assert(olen == ctx2->len - 11);
            if (olen != ctx2->len - 11)
            {
                outputBuffer.clear();
                break;
            }
        }
        else
        {
            outputBuffer.resize((ctx2->len - 11) * (blockCount - 1) + olen);
            break;
        }

        p += blockSize;
        q += olen;
    }
}

bool
PRO_CALLTYPE
ProRsaKeyGen(unsigned long  keyBytes, /* 128, 256, 512 */
             CProStlString& pString,
             CProStlString& qString,
             CProStlString& nString,
             CProStlString& eString,
             CProStlString& dString)
{
    assert(keyBytes == 128 || keyBytes == 256 || keyBytes == 512);
    if (keyBytes != 128 && keyBytes != 256 && keyBytes != 512)
    {
        return (false);
    }

    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx, MBEDTLS_RSA_PKCS_V15, 0);

    if (mbedtls_rsa_gen_key(&ctx, &Rand_i, NULL, keyBytes * 8, 65537) != 0)
    {
        return (false);
    }

    size_t theSize         = 0;
    char   theString[2048] = "";
    theString[sizeof(theString) - 1] = '\0';

    /*
     * P
     */
    {
        if (mbedtls_mpi_write_string(
            &ctx.P, 16, theString, sizeof(theString) - 1, &theSize) != 0)
        {
            return (false);
        }

        pString = theString;
    }

    /*
     * Q
     */
    {
        if (mbedtls_mpi_write_string(
            &ctx.Q, 16, theString, sizeof(theString) - 1, &theSize) != 0)
        {
            return (false);
        }

        qString = theString;
    }

    /*
     * N
     */
    {
        if (mbedtls_mpi_write_string(
            &ctx.N, 16, theString, sizeof(theString) - 1, &theSize) != 0)
        {
            return (false);
        }

        nString = theString;
    }

    /*
     * E
     */
    {
        if (mbedtls_mpi_write_string(
            &ctx.E, 16, theString, sizeof(theString) - 1, &theSize) != 0)
        {
            return (false);
        }

        eString = theString;
    }

    /*
     * D
     */
    {
        if (mbedtls_mpi_write_string(
            &ctx.D, 16, theString, sizeof(theString) - 1, &theSize) != 0)
        {
            return (false);
        }

        dString = theString;
    }

    return (true);
}

/*-------------------------------------------------------------------------*/

void
PRO_CALLTYPE
ProCalcPasswordHash(const char  nonce[32],
                    const char* password,
                    char        passwordHash[32])
{
    char nonceString[64 + 1] = "";
    nonceString[64] = '\0';

    {
        const char* const p = nonce;

        for (int i = 0; i < 32; ++i)
        {
            snprintf_pro(
                nonceString + i * 2,
                2 + 1,
                "%02x",
                (unsigned int)(unsigned char)p[i] /* unsigned */
                );
        }
    }

    CProStlString passwordString = nonceString;
    passwordString += password != NULL ? password : "";

    ProSha256All(
        passwordString.c_str(), passwordString.length(), passwordHash);

    ProZeroMemory(&passwordString[0], passwordString.length());
}

void
PRO_CALLTYPE
ProZeroMemory(char*  buf,
              size_t size)
{
    if (buf == NULL || size == 0)
    {
        return;
    }

    mbedtls_platform_zeroize(buf, size);
}
