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
 * pro_shared共享库模块主要是为了解决Thread Local Storage(TLS)变量和内存池的
 * 实例控制问题,避免上述实例分散存在于进程内各个模块的数据段中
 *
 * 比如, srand()/rand()关联的种子是TLS存储类型的.当一个网络回调线程的执行路径
 * 第一次抵达某个模块时,假定它在该模块中用到了rand(),但该线程在该模块中对应的
 * 种子实例可能尚未初始化,那调用者就会得到一个不正确的伪随机数序列...
 *
 * 多个模块中记录多个线程的首次进入并不方便!!!
 *
 * 通常情况下,这个问题由操作系统和编译器通过类似于DllMain(...)的方法来解决.但
 * 考虑到移植通用性和维护可控性,我们采用另外的方法来处理,那就是让各个模块调用
 * pro_shared模块提供的替代函数,而不是调用srand()/rand().这样,某个线程原本在
 * 各个模块中对应的种子实例就会"合并"到pro_shared模块里,多线程多模块的问题就
 * 转化为多线程单模块的问题.只要pro_shared模块里的种子实例初始化了,其他模块就
 * 可以安全使用了
 *
 * 内存池放在这里维护,主要是为了全局调度资源
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
 * 功能: 获取该库的版本号
 *
 * 参数:
 * major : 主版本号
 * minor : 次版本号
 * patch : 补丁号
 *
 * 返回值: 无
 *
 * 说明: 该函数格式恒定
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSharedVersion(unsigned char* major,  /* = NULL */
                 unsigned char* minor,  /* = NULL */
                 unsigned char* patch); /* = NULL */

/*
 * 功能: 初始化当前线程的伪随机数种子
 *
 * 参数: 无
 *
 * 返回值: 无
 *
 * 说明: 参见文件头部注释
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSrand();

/*
 * 功能: 获取一个伪随机数
 *
 * 参数: 无
 *
 * 返回值: [0.0 ~ 1.0]之间的伪随机数
 *
 * 说明: 参见文件头部注释
 */
PRO_SHARED_API
double
PRO_CALLTYPE
ProRand_0_1();

/*
 * 功能: 获取当前的系统tick值
 *
 * 参数: 无
 *
 * 返回值: tick值. 单位(ms)
 *
 * 说明: Windows版本使用TLS实现
 */
PRO_SHARED_API
PRO_INT64
PRO_CALLTYPE
ProGetTickCount64_s();

/*
 * 功能: 休眠当前线程
 *
 * 参数:
 * milliseconds : 休眠时间. 单位(ms)
 *
 * 返回值: 无
 *
 * 说明: 使用套接字结合带超时的select(...)实现,精度高于Sleep(...)/usleep(...)
 *
 *       放在pro_shared共享库里而不是放在静态库里实现,主要是为了避免进程的多个
 *       模块占用多份套接字资源
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProSleep_s(PRO_UINT32 milliseconds);

/*
 * 功能: 生成一个普通定时器id
 *
 * 参数: 无
 *
 * 返回值: 定时器id
 *
 * 说明: 无
 */
PRO_SHARED_API
unsigned long
PRO_CALLTYPE
ProMakeTimerId();

/*
 * 功能: 生成一个多媒体定时器id
 *
 * 参数: 无
 *
 * 返回值: 定时器id
 *
 * 说明: 无
 */
PRO_SHARED_API
unsigned long
PRO_CALLTYPE
ProMakeMmTimerId();

/*
 * 功能: 从SGI内存池里分配一块内存
 *
 * 参数:
 * size      : 分配的字节数
 * poolIndex : 内存池索引号. [0 ~ 9],一共10组内存池
 *
 * 返回值: 分配的内存地址或NULL
 *
 * 说明: 每组内存池有自己的访问锁
 */
PRO_SHARED_API
void*
PRO_CALLTYPE
ProAllocateSgiPoolBuffer(size_t        size,
                         unsigned long poolIndex);   /* 0 ~ 9 */

/*
 * 功能: 从SGI内存池里重新分配一块内存
 *
 * 参数:
 * buf       : 原来分配的内存地址
 * newSize   : 重新分配的字节数
 * poolIndex : 内存池索引号. [0 ~ 9],一共10组内存池
 *
 * 返回值: 重新分配的内存地址或NULL
 *
 * 说明: 每组内存池有自己的访问锁
 */
PRO_SHARED_API
void*
PRO_CALLTYPE
ProReallocateSgiPoolBuffer(void*         buf,
                           size_t        newSize,
                           unsigned long poolIndex); /* 0 ~ 9 */

/*
 * 功能: 释放一块内存到SGI内存池里
 *
 * 参数:
 * buf       : 内存地址
 * poolIndex : 内存池索引号. [0 ~ 9],一共10组内存池
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_SHARED_API
void
PRO_CALLTYPE
ProDeallocateSgiPoolBuffer(void*         buf,
                           unsigned long poolIndex); /* 0 ~ 9 */

/*
 * 功能: 获取SGI内存池信息
 *
 * 参数:
 * freeList  : 用于接收信息
 * objSize   : 用于接收信息
 * heapSize  : 用于接收信息
 * poolIndex : 内存池索引号. [0 ~ 9],一共10组内存池
 *
 * 返回值: 无
 *
 * 说明: 该函数用于调试或状态监控
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
