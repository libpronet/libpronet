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

/*
 * pro_shared�����ģ����Ҫ��Ϊ�˽��"Thread Local Storage"(TLS)�������ڴ��
 * ��ʵ����������, ��������ʵ����ɢ�����ڽ����ڸ���ģ������ݶ���
 *
 * ����, srand()/rand()������������TLS�洢���͵�. ��һ������ص��̵߳�ִ��·
 * ����һ�εִ�ĳ��ģ��ʱ, �ٶ����ڸ�ģ�����õ���rand(), �����߳��ڸ�ģ����
 * ��Ӧ������ʵ��������δ��ʼ��, �ǵ����߾ͻ�õ�һ������ȷ��α���������...
 *
 * ���ģ���м�¼����̵߳��״ν��벢������!!!
 *
 * ͨ�������, ��������ɲ���ϵͳ�ͱ�����ͨ��������DllMain(...)�ķ��������.
 * �����ǵ���ֲͨ���Ժ�ά���ɿ���, ���ǲ�������ķ���������, �Ǿ����ø���ģ
 * �����pro_sharedģ���ṩ���������, �����ǵ���srand()/rand(). ����, ĳ��
 * �߳�ԭ���ڸ���ģ���ж�Ӧ������ʵ���ͻ�"�ϲ�"��pro_sharedģ����, ���̶߳�
 * ģ��������ת��Ϊ���̵߳�ģ�������. ֻҪpro_sharedģ���������ʵ����ʼ
 * ����, ����ģ��Ϳ��԰�ȫʹ����
 *
 * �ڴ�ط�������ά��, ��Ҫ��Ϊ��ȫ�ֵ�����Դ
 */

#if !defined(____PRO_SHARED_H____)
#define ____PRO_SHARED_H____

#include "pro_a.h"
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_SHARED_EXPORTS)
#if defined(_MSC_VER)
#define PRO_SHARED_API /* using xxx.def */
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
 * ����: ��ȡһ��α�����
 *
 * ����: ��
 *
 * ����ֵ: [0 ~ 32767]֮���α�����
 *
 * ˵��: �μ��ļ�ͷ��ע��
 */
PRO_SHARED_API
int
PRO_CALLTYPE
ProRand_0_32767();

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
 * ˵��: ʹ���׽��ֽ�ϴ���ʱ��select(...)ʵ��, ���ȸ���Sleep(...)/usleep(...)
 *
 *       ����pro_shared�����������Ƿ��ھ�̬����ʵ��, ��Ҫ��Ϊ�˱�����̵�
 *       ���ģ��ռ�ö���׽�����Դ
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
PRO_UINT64
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
PRO_UINT64
PRO_CALLTYPE
ProMakeMmTimerId();

/*
 * ����: ��SGI�ڴ�������һ���ڴ�
 *
 * ����:
 * size      : ������ֽ���
 * poolIndex : �ڴ��������. [0 ~ 9], һ��10���ڴ��
 *
 * ����ֵ: ������ڴ��ַ��NULL
 *
 * ˵��: ÿ���ڴ�����Լ��ķ�����
 */
PRO_SHARED_API
void*
PRO_CALLTYPE
ProAllocateSgiPoolBuffer(size_t        size,
                         unsigned long poolIndex); /* 0 ~ 9 */

/*
 * ����: ��SGI�ڴ�������·���һ���ڴ�
 *
 * ����:
 * buf       : ԭ��������ڴ��ַ
 * newSize   : ���·�����ֽ���
 * poolIndex : �ڴ��������. [0 ~ 9], һ��10���ڴ��
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
 * poolIndex : �ڴ��������. [0 ~ 9], һ��10���ڴ��
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
 * freeList    : ���ص���������
 * objSize     : ���صĶ���ߴ�����
 * busyObjNum  : ���ص�æ������Ŀ����
 * totalObjNum : ���ص��ܶ�����Ŀ����
 * heapBytes   : ���صĶ�������
 * poolIndex   : �ڴ��������. [0 ~ 9], һ��10���ڴ��
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
                  size_t        busyObjNum[64],
                  size_t        totalObjNum[64],
                  size_t*       heapBytes,  /* = NULL */
                  unsigned long poolIndex); /* 0 ~ 9 */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_SHARED_H____ */
