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
ProMd5New();

void
ProMd5Delete(void* ctx);

void
ProMd5Init(void* ctx);

void
ProMd5Update(void*       ctx,
             const void* buf,
             size_t      size);

void
ProMd5Finish(void* ctx,
             char  hashValue[16]);

void
ProMd5All(const void* buf,
          size_t      size,
          char        hashValue[16]);

/*-------------------------------------------------------------------------*/

void*
ProSha1New();

void
ProSha1Delete(void* ctx);

void
ProSha1Init(void* ctx);

void
ProSha1Update(void*       ctx,
              const void* buf,
              size_t      size);

void
ProSha1Finish(void* ctx,
              char  hashValue[20]);

void
ProSha1All(const void* buf,
           size_t      size,
           char        hashValue[20]);

/*-------------------------------------------------------------------------*/

void*
ProSha256New();

void
ProSha256Delete(void* ctx);

void
ProSha256Init(void* ctx);

void
ProSha256Update(void*       ctx,
                const void* buf,
                size_t      size);

void
ProSha256Finish(void* ctx,
                char  hashValue[32]);

void
ProSha256All(const void* buf,
             size_t      size,
             char        hashValue[32]);

/*-------------------------------------------------------------------------*/

void*
ProRipemd160New();

void
ProRipemd160Delete(void* ctx);

void
ProRipemd160Init(void* ctx);

void
ProRipemd160Update(void*       ctx,
                   const void* buf,
                   size_t      size);

void
ProRipemd160Finish(void* ctx,
                   char  hashValue[20]);

void
ProRipemd160All(const void* buf,
                size_t      size,
                char        hashValue[20]);

/*-------------------------------------------------------------------------*/

void*
ProRsaNew();

void
ProRsaDelete(void* ctx);

bool
ProRsaInitPub(void*       ctx,
              const char* nString,
              const char* eString);

bool
ProRsaInitPrivNED(void*       ctx,
                  const char* nString,
                  const char* eString,
                  const char* dString);

bool
ProRsaInitPrivPQE(void*       ctx,
                  const char* pString,
                  const char* qString,
                  const char* eString);

void
ProRsaEncrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer);

void
ProRsaDecrypt(void*                ctx,
              const void*          inputBuffer,
              size_t               inputSize,
              CProStlVector<char>& outputBuffer);

bool
ProRsaKeyGen(unsigned long  keyBytes, /* 128, 256, 512 */
             CProStlString& pString,
             CProStlString& qString,
             CProStlString& nString,
             CProStlString& eString,
             CProStlString& dString);

/*-------------------------------------------------------------------------*/

void
ProBase64Encode(const void*    inputBuffer,
                size_t         inputSize,
                CProStlString& outputString);

void
ProBase64Decode(const void*          inputBuffer,
                size_t               inputSize,
                CProStlVector<char>& outputBuffer);

void
ProBase64DecodeStr(const char*          inputString,
                   CProStlVector<char>& outputBuffer);

/*-------------------------------------------------------------------------*/

/*
 * ����: ����(�Ự�����+����)��ϵ�HASH(SHA-256)ֵ
 *
 * ����:
 * nonce        : �Ự�����
 * password     : ����
 * passwordHash : ������
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
void
ProCalcPasswordHash(const char  nonce[32],
                    const char* password,
                    char        passwordHash[32]);

/*
 * ����: ��ȫ����
 *
 * ����:
 * buf  : ��������ַ
 * size : ����������
 *
 * ����ֵ: ��
 *
 * ˵��: �ú����ٶȽ���, ֻ���ڿ�����������ݵĲ���
 */
void
ProZeroMemory(char*  buf,
              size_t size);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_SSL_UTIL_H____ */
