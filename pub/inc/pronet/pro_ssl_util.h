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

#if !defined(____PRO_SSL_UTIL_H____)
#define ____PRO_SSL_UTIL_H____

#include "pro_a.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

void*
PRO_CALLTYPE
ProMd5New();

void
PRO_CALLTYPE
ProMd5Delete(void* ctx);

void
PRO_CALLTYPE
ProMd5Init(void* ctx);

void
PRO_CALLTYPE
ProMd5Update(void*       ctx,
             const void* buf,
             size_t      size);

void
PRO_CALLTYPE
ProMd5Finish(void* ctx,
             char  hashValue[16]);

void
PRO_CALLTYPE
ProMd5All(const void* buf,
          size_t      size,
          char        hashValue[16]);

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProSha1New();

void
PRO_CALLTYPE
ProSha1Delete(void* ctx);

void
PRO_CALLTYPE
ProSha1Init(void* ctx);

void
PRO_CALLTYPE
ProSha1Update(void*       ctx,
              const void* buf,
              size_t      size);

void
PRO_CALLTYPE
ProSha1Finish(void* ctx,
              char  hashValue[20]);

void
PRO_CALLTYPE
ProSha1All(const void* buf,
           size_t      size,
           char        hashValue[20]);

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProSha256New();

void
PRO_CALLTYPE
ProSha256Delete(void* ctx);

void
PRO_CALLTYPE
ProSha256Init(void* ctx);

void
PRO_CALLTYPE
ProSha256Update(void*       ctx,
                const void* buf,
                size_t      size);

void
PRO_CALLTYPE
ProSha256Finish(void* ctx,
                char  hashValue[32]);

void
PRO_CALLTYPE
ProSha256All(const void* buf,
             size_t      size,
             char        hashValue[32]);

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProRipemd160New();

void
PRO_CALLTYPE
ProRipemd160Delete(void* ctx);

void
PRO_CALLTYPE
ProRipemd160Init(void* ctx);

void
PRO_CALLTYPE
ProRipemd160Update(void*       ctx,
                   const void* buf,
                   size_t      size);

void
PRO_CALLTYPE
ProRipemd160Finish(void* ctx,
                   char  hashValue[20]);

void
PRO_CALLTYPE
ProRipemd160All(const void* buf,
                size_t      size,
                char        hashValue[20]);

/*-------------------------------------------------------------------------*/

void*
PRO_CALLTYPE
ProRsaNew();

void
PRO_CALLTYPE
ProRsaDelete(void* ctx);

bool
PRO_CALLTYPE
ProRsaInitPub(void*       ctx,
              const char* nString,
              const char* eString);

bool
PRO_CALLTYPE
ProRsaInitPrivNED(void*       ctx,
                  const char* nString,
                  const char* eString,
                  const char* dString);

bool
PRO_CALLTYPE
ProRsaInitPrivPQE(void*       ctx,
                  const char* pString,
                  const char* qString,
                  const char* eString);

void
PRO_CALLTYPE
ProRsaEncrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer);

void
PRO_CALLTYPE
ProRsaDecrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer);

bool
PRO_CALLTYPE
ProRsaKeyGen(unsigned long  keyBytes, /* 128, 256, 512 */
             CProStlString& pString,
             CProStlString& qString,
             CProStlString& nString,
             CProStlString& eString,
             CProStlString& dString);

/*-------------------------------------------------------------------------*/

/*
 * 功能: 计算(随机数+口令)组合的HASH(SHA-256)值
 *
 * 参数:
 * nonce        : 随机数
 * password     : 口令
 * passwordHash : 输出结果
 *
 * 返回值: 无
 *
 * 说明: 无
 */
void
PRO_CALLTYPE
ProCalcPasswordHash(PRO_UINT64  nonce,
                    const char* password,
                    char        passwordHash[32]);

/*
 * 功能: 安全清零
 *
 * 参数:
 * buf  : 缓冲区地址
 * size : 缓冲区长度
 *
 * 返回值: 无
 *
 * 说明: 该函数速度较慢, 只用于口令等敏感数据的擦除
 */
void
PRO_CALLTYPE
ProZeroMemory(char*  buf,
              size_t size);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_SSL_UTIL_H____ */
