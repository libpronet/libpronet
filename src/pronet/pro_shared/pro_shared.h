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
 * The purpose of creating this dynamic library is to consolidate the random
 * seed instances and memory pool instances used by the entire process
 * into a common module, simplifying the initialization of random seeds
 * and improving memory utilization of the memory pools.
 *
 * In the C/C++ runtime library, the seed associated with srand()/rand()
 * is TLS (Thread Local Storage) based. When a network callback thread
 * enters a dynamic library, it may call rand(). If the seed instance
 * in that dynamic library for the thread has not been initialized (i.e.,
 * srand() has not been called), rand() will get an incorrect random sequence...
 *
 * Although it is possible to record the first entry of each thread
 * in the dynamic library and initialize the random seed instance,
 * this approach is not convenient!!!
 *
 * Normally, this issue is resolved by OS and compiler through mechanisms
 * similar to DllMain(). For better portability, we adopt an alternative approach:
 * let each module call the replacement functions provided by the pro_shared
 * instead of calling srand()/rand() directly. This way, the multiple seed
 * instances originally associated with a thread in various dynamic libraries
 * are "merged" into a single dynamic library. We only need to focus on seed
 * initialization within the pro_shared dynamic library.
 */

#ifndef ____PRO_SHARED_H____
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
 * Function: Get the version number of this library
 *
 * Parameters:
 * major : Major version number
 * minor : Minor version number
 * patch : Patch number
 *
 * Return: None
 *
 * Note: The function signature is fixed
 */
PRO_SHARED_API
void
ProSharedVersion(unsigned char* major,  /* = NULL */
                 unsigned char* minor,  /* = NULL */
                 unsigned char* patch); /* = NULL */

/*
 * Function: Initialize the pseudo-random seed for the current thread
 *
 * Parameters: None
 *
 * Return: None
 *
 * Note: Refer to the header comments of this file
 */
PRO_SHARED_API
void
ProSrand();

/*
 * Function: Get a pseudo-random number
 *
 * Parameters: None
 *
 * Return: Pseudo-random number in range [0.0, 1.0]
 *
 * Note: Refer to the header comments of this file
 */
PRO_SHARED_API
double
ProRand_0_1();

/*
 * Function: Get a pseudo-random number
 *
 * Parameters: None
 *
 * Return: Pseudo-random number in range [0, 32767]
 *
 * Note: Refer to the header comments of this file
 */
PRO_SHARED_API
int
ProRand_0_32767();

/*
 * Function: Get the tick count since system boot
 *
 * Parameters: None
 *
 * Return: Tick count in milliseconds
 *
 * Note: Monotonic increment is guaranteed between multiple threads
 */
PRO_SHARED_API
int64_t
ProGetTickCount64();

/*
 * Function: Get the tick count since NTP epoch
 *
 * Parameters: None
 *
 * Return: Tick count in milliseconds
 *
 * Note: The NTP epoch is 1900-01-01 00:00:00 UTC
 */
PRO_SHARED_API
int64_t
ProGetNtpTickCount64();

/*
 * Function: Suspend the current thread
 *
 * Parameters:
 * milliseconds : Sleep duration in milliseconds
 *
 * Return: None
 *
 * Note: Windows implementation uses select() with timeout for higher precision
 *       than Sleep()
 */
PRO_SHARED_API
void
ProSleep(unsigned int milliseconds);

/*
 * Function: Generate a normal timer ID
 *
 * Parameters: None
 *
 * Return: Timer ID
 *
 * Note: The returned value is an odd number
 */
PRO_SHARED_API
uint64_t
ProMakeTimerId();

/*
 * Function: Generate a multimedia timer ID
 *
 * Parameters: None
 *
 * Return: Timer ID
 *
 * Note: The returned value is an even number
 */
PRO_SHARED_API
uint64_t
ProMakeMmTimerId();

/*
 * Function: Allocate memory from the SGI memory pool
 *
 * Parameters:
 * size      : Number of bytes to allocate
 * poolIndex : Memory pool index [0, 3], total 4 memory pools
 *
 * Return: Allocated memory address or NULL
 *
 * Note: Each memory pool has its own access lock
 */
PRO_SHARED_API
void*
ProAllocateSgiPoolBuffer(size_t       size,
                         unsigned int poolIndex); /* [0, 3] */

/*
 * Function: Reallocate memory from the SGI memory pool
 *
 * Parameters:
 * buf       : Previously allocated memory address
 * newSize   : New size in bytes
 * poolIndex : Memory pool index [0, 3], total 4 memory pools
 *
 * Return: Reallocated memory address or NULL
 *
 * Note: Each memory pool has its own access lock
 */
PRO_SHARED_API
void*
ProReallocateSgiPoolBuffer(void*        buf,
                           size_t       newSize,
                           unsigned int poolIndex); /* [0, 3] */

/*
 * Function: Deallocate memory back to the SGI memory pool
 *
 * Parameters:
 * buf       : Memory address
 * poolIndex : Memory pool index [0, 3], total 4 memory pools
 *
 * Return: None
 *
 * Note: The memory allocated from one memory pool can be deallocated
 *       to another, though rarely used this way
 */
PRO_SHARED_API
void
ProDeallocateSgiPoolBuffer(void*        buf,
                           unsigned int poolIndex); /* [0, 3] */

/*
 * Function: Get SGI memory pool information
 *
 * Parameters:
 * freeList    : Returned chunks list
 * objSize     : Returned object size list
 * busyObjNum  : Returned count of busy objects list
 * totalObjNum : Returned total object count list
 * heapBytes   : Returned total heap capacity
 * poolIndex   : Memory pool index [0, 3], total 4 memory pools
 *
 * Return: None
 *
 * Note: This function is used for debugging or status monitoring
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
