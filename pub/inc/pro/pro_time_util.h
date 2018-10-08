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

#if !defined(____PRO_TIME_UTIL_H____)
#define ____PRO_TIME_UTIL_H____

#include "pro_a.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"

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

    bool operator==(const PRO_LOCAL_TIME& lt) const
    {
        return (
            year        == lt.year   &&
            month       == lt.month  &&
            day         == lt.day    &&
            hour        == lt.hour   &&
            minute      == lt.minute &&
            second      == lt.second &&
            millisecond == lt.millisecond
            );
    }

    bool operator!=(const PRO_LOCAL_TIME& lt) const
    {
        return (!(*this == lt));
    }

    bool operator<(const PRO_LOCAL_TIME& lt) const
    {
        if (year < lt.year)
        {
            return (true);
        }
        if (year > lt.year)
        {
            return (false);
        }

        if (month < lt.month)
        {
            return (true);
        }
        if (month > lt.month)
        {
            return (false);
        }

        if (day < lt.day)
        {
            return (true);
        }
        if (day > lt.day)
        {
            return (false);
        }

        if (hour < lt.hour)
        {
            return (true);
        }
        if (hour > lt.hour)
        {
            return (false);
        }

        if (minute < lt.minute)
        {
            return (true);
        }
        if (minute > lt.minute)
        {
            return (false);
        }

        if (second < lt.second)
        {
            return (true);
        }
        if (second > lt.second)
        {
            return (false);
        }

        if (millisecond < lt.millisecond)
        {
            return (true);
        }

        return (false);
    }

    bool operator<=(const PRO_LOCAL_TIME& lt) const
    {
        return (*this < lt || *this == lt);
    }

    bool operator>(const PRO_LOCAL_TIME& lt) const
    {
        return (!(*this < lt || *this == lt));
    }

    bool operator>=(const PRO_LOCAL_TIME& lt) const
    {
        return (!(*this < lt));
    }

    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short millisecond;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

PRO_INT64
PRO_CALLTYPE
ProGetTickCount64();

void
PRO_CALLTYPE
ProSleep(PRO_UINT32 milliseconds);

void
PRO_CALLTYPE
ProGetLocalTime(PRO_LOCAL_TIME& localTime,
                long            deltaSeconds = 0);

const char*
PRO_CALLTYPE
ProGetLocalTimeString(CProStlString& timeString,
                      long           deltaSeconds = 0);

const char*
PRO_CALLTYPE
ProLocalTime2String(const PRO_LOCAL_TIME& localTime,
                    char                  timeString[64]);

void
PRO_CALLTYPE
ProString2LocalTime(const char*     timeString,
                    PRO_LOCAL_TIME& localTime);

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_TIME_UTIL_H____ */
