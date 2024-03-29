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
#include "pro_time_util.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/////////////////////////////////////////////////////////////////////////////
////

void
ProGetLocalTime(PRO_LOCAL_TIME& localTime,
                int             deltaMilliseconds) /* = 0 */
{
    localTime.Zero();

#if defined(_WIN32)

    SYSTEMTIME st;
    ::GetLocalTime(&st);

    if (deltaMilliseconds != 0)
    {
        FILETIME ft;
        if (!::SystemTimeToFileTime(&st, &ft))
        {
            return;
        }

        int64_t ft64 = ft.dwHighDateTime;
        ft64 <<= 32;
        ft64 |=  ft.dwLowDateTime;
        ft64 +=  (int64_t)deltaMilliseconds * 10000;

        ft.dwLowDateTime  = (uint32_t)ft64;
        ft.dwHighDateTime = (uint32_t)(ft64 >> 32);

        if (!::FileTimeToSystemTime(&ft, &st))
        {
            return;
        }
    }

    localTime.year        = st.wYear;
    localTime.month       = st.wMonth;
    localTime.day         = st.wDay;
    localTime.hour        = st.wHour;
    localTime.minute      = st.wMinute;
    localTime.second      = st.wSecond;
    localTime.millisecond = st.wMilliseconds;

#else  /* _WIN32 */

    /*
     * [1970 ~ 2038]
     */
    struct timeval now = { 0 };
    if (gettimeofday(&now, NULL) != 0)
    {
        return;
    }

    int64_t tt = (int64_t)now.tv_sec * 1000000 + now.tv_usec + (int64_t)deltaMilliseconds * 1000;

    time_t seconds  = (time_t)(tt / 1000000);
    time_t useconds = (time_t)(tt % 1000000);
    if (seconds <= 0) /* overflow */
    {
        return;
    }

    const struct tm* tm = localtime(&seconds);
    if (tm == NULL)
    {
        return;
    }

    localTime.year        = (unsigned short)(tm->tm_year + 1900);
    localTime.month       = (unsigned short)(tm->tm_mon + 1);
    localTime.day         = (unsigned short)tm->tm_mday;
    localTime.hour        = (unsigned short)tm->tm_hour;
    localTime.minute      = (unsigned short)tm->tm_min;
    localTime.second      = (unsigned short)tm->tm_sec;
    localTime.millisecond = (unsigned short)(useconds / 1000);

#endif /* _WIN32 */
}

const char*
ProGetLocalTimeString(CProStlString& timeString,
                      int            deltaMilliseconds) /* = 0 */
{
    PRO_LOCAL_TIME localTime;
    char           timeString2[64] = "";

    ProGetLocalTime(localTime, deltaMilliseconds);
    timeString = ProLocalTime2String(localTime, timeString2);

    return timeString.c_str();
}

const char*
ProLocalTime2String(const PRO_LOCAL_TIME& localTime,
                    char                  timeString[64])
{
    sprintf(
        timeString,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        (int)localTime.year,
        (int)localTime.month,
        (int)localTime.day,
        (int)localTime.hour,
        (int)localTime.minute,
        (int)localTime.second,
        (int)localTime.millisecond
        );

    return timeString;
}

void
ProString2LocalTime(const char*     timeString,
                    PRO_LOCAL_TIME& localTime)
{
    localTime.Zero();

    if (timeString == NULL || timeString[0] == '\0')
    {
        return;
    }

    CProStlString timeString2 = timeString;

    CProStlString yearString        = "0";
    CProStlString monthString       = "0";
    CProStlString dayString         = "0";
    CProStlString hourString        = "0";
    CProStlString minuteString      = "0";
    CProStlString secondString      = "0";
    CProStlString millisecondString = "0";

    if (strlen(timeString) == strlen("0000-00-00"))
    {
        assert(timeString[4] == '-');
        assert(timeString[7] == '-');
        if (timeString[4] != '-' || timeString[7] != '-')
        {
            return;
        }

        timeString2[4] = '0';
        timeString2[7] = '0';

        yearString.assign (timeString + 0, (int)4);
        monthString.assign(timeString + 5, (int)2);
        dayString.assign  (timeString + 8, (int)2);
    }
    else if (strlen(timeString) == strlen("0000-00-00 00:00:00"))
    {
        assert(timeString[4]  == '-');
        assert(timeString[7]  == '-');
        assert(timeString[10] == ' ');
        assert(timeString[13] == ':');
        assert(timeString[16] == ':');
        if (timeString[4]  != '-' ||
            timeString[7]  != '-' ||
            timeString[10] != ' ' ||
            timeString[13] != ':' ||
            timeString[16] != ':')
        {
            return;
        }

        timeString2[4]  = '0';
        timeString2[7]  = '0';
        timeString2[10] = '0';
        timeString2[13] = '0';
        timeString2[16] = '0';

        yearString.assign  (timeString + 0 , (int)4);
        monthString.assign (timeString + 5 , (int)2);
        dayString.assign   (timeString + 8 , (int)2);
        hourString.assign  (timeString + 11, (int)2);
        minuteString.assign(timeString + 14, (int)2);
        secondString.assign(timeString + 17, (int)2);
    }
    else if (strlen(timeString) == strlen("0000-00-00 00:00:00.000"))
    {
        assert(timeString[4]  == '-');
        assert(timeString[7]  == '-');
        assert(timeString[10] == ' ');
        assert(timeString[13] == ':');
        assert(timeString[16] == ':');
        assert(timeString[19] == '.');
        if (timeString[4]  != '-' ||
            timeString[7]  != '-' ||
            timeString[10] != ' ' ||
            timeString[13] != ':' ||
            timeString[16] != ':' ||
            timeString[19] != '.')
        {
            return;
        }

        timeString2[4]  = '0';
        timeString2[7]  = '0';
        timeString2[10] = '0';
        timeString2[13] = '0';
        timeString2[16] = '0';
        timeString2[19] = '0';

        yearString.assign       (timeString + 0 , (int)4);
        monthString.assign      (timeString + 5 , (int)2);
        dayString.assign        (timeString + 8 , (int)2);
        hourString.assign       (timeString + 11, (int)2);
        minuteString.assign     (timeString + 14, (int)2);
        secondString.assign     (timeString + 17, (int)2);
        millisecondString.assign(timeString + 20, (int)3);
    }
    else
    {
        return;
    }

    if (timeString2.find_first_not_of("0123456789") != CProStlString::npos)
    {
        return;
    }

    int year        = 0;
    int month       = 0;
    int day         = 0;
    int hour        = 0;
    int minute      = 0;
    int second      = 0;
    int millisecond = 0;

    sscanf(yearString.c_str()       , "%d", &year);
    sscanf(monthString.c_str()      , "%d", &month);
    sscanf(dayString.c_str()        , "%d", &day);
    sscanf(hourString.c_str()       , "%d", &hour);
    sscanf(minuteString.c_str()     , "%d", &minute);
    sscanf(secondString.c_str()     , "%d", &second);
    sscanf(millisecondString.c_str(), "%d", &millisecond);

    localTime.year        = (unsigned short)year;
    localTime.month       = (unsigned short)month;
    localTime.day         = (unsigned short)day;
    localTime.hour        = (unsigned short)hour;
    localTime.minute      = (unsigned short)minute;
    localTime.second      = (unsigned short)second;
    localTime.millisecond = (unsigned short)millisecond;
}
