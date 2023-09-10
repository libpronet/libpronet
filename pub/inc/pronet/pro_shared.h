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
 * pro_shared动态库主要是为了解决"Thread Local Storage"变量和内存池的实例控制问题, 避免
 * 随机数种子实例分散存在于进程内的各个动态库的数据段中, 避免内存池实例的重复创建, 提高内存的
 * 全局使用率
 *
 * 比如, srand()/rand()关联的种子是TLS存储类型的, 当一个网络回调线程的执行路径第一次抵达
 * 某个动态库时, 如果它在该动态库中用到了rand(), 但该线程在该动态库中对应的种子实例可能尚未
 * 初始化, 也即尚未调用过srand(), 那调用者就会得到一个错误的伪随机数序列...
 *
 * 虽然可以让每个动态库记录各个线程的首次和末次进入并为之初始化随机数种子, 但这并不方便!!!
 *
 * 通常情况下, 这个问题由操作系统和编译器通过类似于DllMain()的方法来解决. 但考虑到移植通用性,
 * 我们采用另外的方法来处理, 那就是让各个模块调用pro_shared动态库提供的替代函数, 而不是调用
 * srand()/rand(). 这样, 某个线程原本在各个动态库中对应的多个种子实例就会被"合并"到同一个
 * pro_shared动态库里, 只要pro_shared动态库里的种子实例初始化了, 其他模块就可以安全使用了
 */

#if !defined(____PRO_SHARED_H____)
#define ____PRO_SHARED_H____

#include "pro_a.h"
#include "pro_z.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_SHARED_EXPORTS)
#if defined(_MSC_VER)
#define PRO_SHARED_API /* using xxx.def */
#elif defined(__MINGW32__) || defined(__CYGWIN__)
#define PRO_SHARED_API __declspec(dllexport)
#else
#define PRO_SHARED_API __attribute__((visibility("default")))
#endif
#else
#define PRO_SHARED_API
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
ProSrand();

/*
 * 功能: 获取一个伪随机数
 *
 * 参数: 无
 *
 * 返回值: [0.0, 1.0]之间的伪随机数
 *
 * 说明: 参见文件头部注释
 */
PRO_SHARED_API
double
ProRand_0_1();

/*
 * 功能: 获取一个伪随机数
 *
 * 参数: 无
 *
 * 返回值: [0, 32767]之间的伪随机数
 *
 * 说明: 参见文件头部注释
 */
PRO_SHARED_API
int
ProRand_0_32767();

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
int64_t
ProGetTickCount64();

/*
 * 功能: 休眠当前线程
 *
 * 参数:
 * milliseconds : 休眠时间. 单位(ms)
 *
 * 返回值: 无
 *
 * 说明: Windows版本使用带超时的select()实现, 精度高于Sleep()
 */
PRO_SHARED_API
void
ProSleep(unsigned int milliseconds);

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
uint64_t
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
uint64_t
ProMakeMmTimerId();

/*
 * 功能: 从SGI内存池里分配一块内存
 *
 * 参数:
 * size      : 分配的字节数
 * poolIndex : 内存池索引号. [0, 3], 一共4个内存池
 *
 * 返回值: 分配的内存地址或NULL
 *
 * 说明: 每个内存池有自己的访问锁
 */
PRO_SHARED_API
void*
ProAllocateSgiPoolBuffer(size_t       size,
                         unsigned int poolIndex); /* [0, 3] */

/*
 * 功能: 从SGI内存池里重新分配一块内存
 *
 * 参数:
 * buf       : 原来分配的内存地址
 * newSize   : 重新分配的字节数
 * poolIndex : 内存池索引号. [0, 3], 一共4个内存池
 *
 * 返回值: 重新分配的内存地址或NULL
 *
 * 说明: 每个内存池有自己的访问锁
 */
PRO_SHARED_API
void*
ProReallocateSgiPoolBuffer(void*        buf,
                           size_t       newSize,
                           unsigned int poolIndex); /* [0, 3] */

/*
 * 功能: 释放一块内存到SGI内存池里
 *
 * 参数:
 * buf       : 内存地址
 * poolIndex : 内存池索引号. [0, 3], 一共4个内存池
 *
 * 返回值: 无
 *
 * 说明: 无
 */
PRO_SHARED_API
void
ProDeallocateSgiPoolBuffer(void*        buf,
                           unsigned int poolIndex); /* [0, 3] */

/*
 * 功能: 获取SGI内存池信息
 *
 * 参数:
 * freeList    : 返回的链表序列
 * objSize     : 返回的对象尺寸序列
 * busyObjNum  : 返回的忙对象数目序列
 * totalObjNum : 返回的总对象数目序列
 * heapBytes   : 返回的堆总容量
 * poolIndex   : 内存池索引号. [0, 3], 一共4个内存池
 *
 * 返回值: 无
 *
 * 说明: 该函数用于调试或状态监控
 */
PRO_SHARED_API
void
ProGetSgiPoolInfo(void*        freeList[64],
                  size_t       objSize[64],
                  size_t       busyObjNum[64],
                  size_t       totalObjNum[64],
                  size_t*      heapBytes,  /* = NULL */
                  unsigned int poolIndex); /* [0, 3] */

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ____PRO_SHARED_H____ */
