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

/*
 * pro_shared�����ģ����Ҫ��Ϊ�˽��Thread Local Storage(TLS)�������ڴ�ص�
 * ʵ����������,��������ʵ����ɢ�����ڽ����ڸ���ģ������ݶ���
 *
 * ����, srand()/rand()������������TLS�洢���͵�.��һ������ص��̵߳�ִ��·��
 * ��һ�εִ�ĳ��ģ��ʱ,�ٶ����ڸ�ģ�����õ���rand(),�����߳��ڸ�ģ���ж�Ӧ��
 * ����ʵ��������δ��ʼ��,�ǵ����߾ͻ�õ�һ������ȷ��α���������...
 *
 * ���ģ���м�¼����̵߳��״ν��벢������!!!
 *
 * ͨ�������,��������ɲ���ϵͳ�ͱ�����ͨ��������DllMain(...)�ķ��������.��
 * ���ǵ���ֲͨ���Ժ�ά���ɿ���,���ǲ�������ķ���������,�Ǿ����ø���ģ�����
 * pro_sharedģ���ṩ���������,�����ǵ���srand()/rand().����,ĳ���߳�ԭ����
 * ����ģ���ж�Ӧ������ʵ���ͻ�"�ϲ�"��pro_sharedģ����,���̶߳�ģ��������
 * ת��Ϊ���̵߳�ģ�������.ֻҪpro_sharedģ���������ʵ����ʼ����,����ģ���
 * ���԰�ȫʹ����
 *
 * �ڴ�ط�������ά��,��Ҫ��Ϊ��ȫ�ֵ�����Դ
 */

#if !defined(____PRO_SHARED_H____)
#define ____PRO_SHARED_H____

#include <cstddef>

#if !defined(____PRO_A_H____)
#define ____PRO_A_H____
#define PRO_INT16_VCC    short
#define PRO_INT32_VCC    int
#if defined(_MSC_VER)
#define PRO_INT64_VCC    __int64
#define PRO_PRT64D_VCC   "%I64d"
#else
#define PRO_INT64_VCC    long long
#define PRO_PRT64D_VCC   "%lld"
#endif
#define PRO_UINT16_VCC   unsigned short
#define PRO_UINT32_VCC   unsigned int
#if defined(_MSC_VER)
#define PRO_UINT64_VCC   unsigned __int64
#define PRO_PRT64U_VCC   "%I64u"
#else
#define PRO_UINT64_VCC   unsigned long long
#define PRO_PRT64U_VCC   "%llu"
#endif
#define PRO_CALLTYPE_VCC __stdcall
#define PRO_EXPORT_VCC   __declspec(dllexport)
#define PRO_IMPORT_VCC
#define PRO_INT16_GCC    short
#define PRO_INT32_GCC    int
#define PRO_INT64_GCC    long long
#define PRO_PRT64D_GCC   "%lld"
#define PRO_UINT16_GCC   unsigned short
#define PRO_UINT32_GCC   unsigned int
#define PRO_UINT64_GCC   unsigned long long
#define PRO_PRT64U_GCC   "%llu"
#define PRO_CALLTYPE_GCC
#define PRO_EXPORT_GCC   __attribute__((visibility("default")))
#define PRO_IMPORT_GCC
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_VCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_VCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_VCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_VCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_VCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_VCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_VCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_VCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_VCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_VCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_VCC
#endif
#else  /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#if !defined(PRO_INT16)
#define PRO_INT16    PRO_INT16_GCC
#endif
#if !defined(PRO_INT32)
#define PRO_INT32    PRO_INT32_GCC
#endif
#if !defined(PRO_INT64)
#define PRO_INT64    PRO_INT64_GCC
#endif
#if !defined(PRO_PRT64D)
#define PRO_PRT64D   PRO_PRT64D_GCC
#endif
#if !defined(PRO_UINT16)
#define PRO_UINT16   PRO_UINT16_GCC
#endif
#if !defined(PRO_UINT32)
#define PRO_UINT32   PRO_UINT32_GCC
#endif
#if !defined(PRO_UINT64)
#define PRO_UINT64   PRO_UINT64_GCC
#endif
#if !defined(PRO_PRT64U)
#define PRO_PRT64U   PRO_PRT64U_GCC
#endif
#if !defined(PRO_CALLTYPE)
#define PRO_CALLTYPE PRO_CALLTYPE_GCC
#endif
#if !defined(PRO_EXPORT)
#define PRO_EXPORT   PRO_EXPORT_GCC
#endif
#if !defined(PRO_IMPORT)
#define PRO_IMPORT   PRO_IMPORT_GCC
#endif
#endif /* _MSC_VER, __MINGW32__, __CYGWIN__ */
#endif /* ____PRO_A_H____ */

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_SHARED_LIB)
#define PRO_SHARED_API
#elif defined(PRO_SHARED_EXPORTS)
#if defined(_MSC_VER)
#define PRO_SHARED_API /* .def */
#else
#define PRO_SHARED_API PRO_EXPORT
#endif
#else
#define PRO_SHARED_API PRO_IMPORT
#endif

/////////////////////////////////////////////////////////////////////////////
////

/*
 * ����: ��ȡ�ÿ�İ汾��
 *
 * ����:
 * major : ���汾��
 * minor : �ΰ汾��
 * patch : ������
 *
 * ����ֵ: ��
 *
 * ˵��: �ú�����ʽ�㶨
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSharedVersion(unsigned char* major,  /* = NULL */
                 unsigned char* minor,  /* = NULL */
                 unsigned char* patch); /* = NULL */

/*
 * ����: ��ʼ����ǰ�̵߳�α���������
 *
 * ����: ��
 *
 * ����ֵ: ��
 *
 * ˵��: �μ��ļ�ͷ��ע��
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSrand();

/*
 * ����: ��ȡһ��α�����
 *
 * ����: ��
 *
 * ����ֵ: [0.0 ~ 1.0]֮���α�����
 *
 * ˵��: �μ��ļ�ͷ��ע��
 */
PRO_SHARED_API
double
PRO_CALLTYPE
ProRand_0_1();

/*
 * ����: ��ȡ��ǰ��ϵͳtickֵ
 *
 * ����: ��
 *
 * ����ֵ: tickֵ. ��λ(ms)
 *
 * ˵��: Windows�汾ʹ��TLSʵ��
 */
PRO_SHARED_API
PRO_INT64
PRO_CALLTYPE
ProGetTickCount64_s();

/*
 * ����: ���ߵ�ǰ�߳�
 *
 * ����:
 * milliseconds : ����ʱ��. ��λ(ms)
 *
 * ����ֵ: ��
 *
 * ˵��: ʹ���׽��ֽ�ϴ���ʱ��select(...)ʵ��,���ȸ���Sleep(...)/usleep(...)
 *
 *       ����pro_shared�����������Ƿ��ھ�̬����ʵ��,��Ҫ��Ϊ�˱�����̵Ķ��
 *       ģ��ռ�ö���׽�����Դ
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSleep_s(PRO_UINT32 milliseconds);

/*
 * ����: ����һ����ͨ��ʱ��id
 *
 * ����: ��
 *
 * ����ֵ: ��ʱ��id
 *
 * ˵��: ��
 */
PRO_SHARED_API
unsigned long
PRO_CALLTYPE
ProMakeTimerId();

/*
 * ����: ����һ����ý�嶨ʱ��id
 *
 * ����: ��
 *
 * ����ֵ: ��ʱ��id
 *
 * ˵��: ��
 */
PRO_SHARED_API
unsigned long
PRO_CALLTYPE
ProMakeMmTimerId();

/*
 * ����: ��SGI�ڴ�������һ���ڴ�
 *
 * ����:
 * size      : ������ֽ���
 * poolIndex : �ڴ��������. [0 ~ 9],һ��10���ڴ��
 *
 * ����ֵ: ������ڴ��ַ��NULL
 *
 * ˵��: ÿ���ڴ�����Լ��ķ�����
 */
PRO_SHARED_API
void*
PRO_CALLTYPE
ProAllocateSgiPoolBuffer(size_t        size,
                         unsigned long poolIndex);   /* 0 ~ 9 */

/*
 * ����: ��SGI�ڴ�������·���һ���ڴ�
 *
 * ����:
 * buf       : ԭ��������ڴ��ַ
 * newSize   : ���·�����ֽ���
 * poolIndex : �ڴ��������. [0 ~ 9],һ��10���ڴ��
 *
 * ����ֵ: ���·�����ڴ��ַ��NULL
 *
 * ˵��: ÿ���ڴ�����Լ��ķ�����
 */
PRO_SHARED_API
void*
PRO_CALLTYPE
ProReallocateSgiPoolBuffer(void*         buf,
                           size_t        newSize,
                           unsigned long poolIndex); /* 0 ~ 9 */

/*
 * ����: �ͷ�һ���ڴ浽SGI�ڴ����
 *
 * ����:
 * buf       : �ڴ��ַ
 * poolIndex : �ڴ��������. [0 ~ 9],һ��10���ڴ��
 *
 * ����ֵ: ��
 *
 * ˵��: ��
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProDeallocateSgiPoolBuffer(void*         buf,
                           unsigned long poolIndex); /* 0 ~ 9 */

/*
 * ����: ��ȡSGI�ڴ����Ϣ
 *
 * ����:
 * freeList  : ���ڽ�����Ϣ
 * objSize   : ���ڽ�����Ϣ
 * heapSize  : ���ڽ�����Ϣ
 * poolIndex : �ڴ��������. [0 ~ 9],һ��10���ڴ��
 *
 * ����ֵ: ��
 *
 * ˵��: �ú������ڵ��Ի�״̬���
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProGetSgiPoolInfo(void*         freeList[64],
                  size_t        objSize[64],
                  size_t*       heapSize,   /* = NULL */
                  unsigned long poolIndex); /* 0 ~ 9 */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* ____PRO_SHARED_H____ */
