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

#ifndef ____PRO_TIME_UTIL_H____
#define ____PRO_TIME_UTIL_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

struct PRO_LOCAL_TIME
{
    PRO_LOCAL_TIME()
    {
        Zero();
    }

    void Zero()
    {
        year        = 0;
        month       = 0;
        day         = 0;
        hour        = 0;
        minute      = 0;
        second      = 0;
        millisecond = 0;
    }

    bool operator==(const PRO_LOCAL_TIME& other) const
    {
        return year        == other.year   &&
               month       == other.month  &&
               day         == other.day    &&
               hour        == other.hour   &&
               minute      == other.minute &&
               second      == other.second &&
               millisecond == other.millisecond;
    }

    bool operator!=(const PRO_LOCAL_TIME& other) const
    {
        return !(*this == other);
    }

    bool operator<(const PRO_LOCAL_TIME& other) const
    {
        if (year < other.year)
        {
            return true;
        }
        if (year > other.year)
        {
            return false;
        }

        if (month < other.month)
        {
            return true;
        }
        if (month > other.month)
        {
            return false;
        }

        if (day < other.day)
        {
            return true;
        }
        if (day > other.day)
        {
            return false;
        }

        if (hour < other.hour)
        {
            return true;
        }
        if (hour > other.hour)
        {
            return false;
        }

        if (minute < other.minute)
        {
            return true;
        }
        if (minute > other.minute)
        {
            return false;
        }

        if (second < other.second)
        {
            return true;
        }
        if (second > other.second)
        {
            return false;
        }

        return millisecond < other.millisecond;
    }

    bool operator<=(const PRO_LOCAL_TIME& other) const
    {
        return *this < other || *this == other;
    }

    bool operator>(const PRO_LOCAL_TIME& other) const
    {
        return !(*this < other || *this == other);
    }

    bool operator>=(const PRO_LOCAL_TIME& other) const
    {
        return !(*this < other);
    }

    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short millisecond;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
extern "C" {
#endif

extern
int64_t
ProGetTickCount64();

extern
void
ProSleep(unsigned int milliseconds);

#if defined(__cplusplus)
}
#endif

void
ProGetLocalTime(PRO_LOCAL_TIME& localTime,
                int             deltaMilliseconds = 0);

const char*
ProGetLocalTimeString(CProStlString& timeString,
                      int            deltaMilliseconds = 0);

const char*
ProLocalTime2String(const PRO_LOCAL_TIME& localTime,
                    char                  timeString[64]);

void
ProString2LocalTime(const char*     timeString,
                    PRO_LOCAL_TIME& localTime);

void
ProGetLocalTimeval(struct timeval& localTimeval,
                   int             deltaMilliseconds = 0);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_TIME_UTIL_H____ */
