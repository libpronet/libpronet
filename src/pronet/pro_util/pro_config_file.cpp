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
#include "pro_config_file.h"
#include "pro_memory_pool.h"
#include "pro_stl.h"
#include "pro_z.h"
#include <cassert>

/////////////////////////////////////////////////////////////////////////////
////

#define LINE_BUF_SIZE (1024 * 63)

/////////////////////////////////////////////////////////////////////////////
////

CProConfigFile::CProConfigFile()
{
    m_fileName = "";
}

void
CProConfigFile::Init(const char* fileName)
{
    assert(fileName != NULL);
    assert(fileName[0] != '\0');
    if (fileName == NULL || fileName[0] == '\0')
    {
        return;
    }

    assert(m_fileName.empty());
    if (!m_fileName.empty())
    {
        return;
    }

    m_fileName = fileName;
}

bool
CProConfigFile::Read(CProStlVector<PRO_CONFIG_ITEM>& configs,
                     char                            aroundCharL,       /* = '\"' */
                     char                            aroundCharR) const /* = '\"' */
{
    configs.clear();

    if (m_fileName.empty())
    {
        return (false);
    }

    FILE* const file = fopen(m_fileName.c_str(), "rb");
    if (file == NULL)
    {
        return (false);
    }

    char* const buf = (char*)ProMalloc(LINE_BUF_SIZE + 1);
    if (buf == NULL)
    {
        fclose(file);

        return (false);
    }

    buf[LINE_BUF_SIZE] = '\0';

    bool ret = true;
    bool bom = false;

    while (1)
    {
        if (fgets(buf, LINE_BUF_SIZE, file) == NULL)
        {
            if (!feof(file))
            {
                ret = false;
            }
            break;
        }

        const char* p = buf;
        const char* q = p + strlen(p) - 1;
        const char* r = NULL;

        /*
         * UTF-8 BOM
         */
        if (!bom)
        {
            bom = true;

            if (p + 2 <= q &&
                p[0] == (char)0xEF && p[1] == (char)0xBB && p[2] == (char)0xBF)
            {
                p += 3;
            }
        }

        /*
         * trim
         */
        for (; p <= q; ++p)
        {
            if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
            {
                break;
            }
        }

        /*
         * trim
         */
        for (; q >= p; --q)
        {
            if (*q != ' ' && *q != '\t' && *q != '\r' && *q != '\n')
            {
                break;
            }
        }

        /*
         * blanks
         */
        if (p > q)
        {
            continue;
        }

        /*
         * comments
         */
        if (p + 1 <= q && p[0] == '/' && p[1] == '/')
        {
            continue;
        }

        /*
         * comments
         */
        if (*p == '#' || *p == ';')
        {
            continue;
        }

        PRO_CONFIG_ITEM item;

        for (int i = 0; i < 2; ++i)
        {
            /*
             * open-quote
             */
            if (*p != aroundCharL)
            {
                ret = false;
                break;
            }

            ++p;
            if (p > q)
            {
                ret = false;
                break;
            }

            /*
             * close-quote
             */
            for (r = p; r <= q; ++r)
            {
                if (*r == aroundCharR)
                {
                    break;
                }
            }

            if (r > q)
            {
                ret = false;
                break;
            }

            if (i == 0)
            {
                item.configName.assign (p, (int)(r - p)); /* name */
            }
            else
            {
                item.configValue.assign(p, (int)(r - p)); /* value */
            }

            /*
             * trim
             */
            for (p = r + 1; p <= q; ++p)
            {
                if (*p != ' ' && *p != '\t')
                {
                    break;
                }
            }

            /*
             * blanks
             */
            if (p > q)
            {
                p = NULL;
                break;
            }

            /*
             * comments
             */
            if (p + 1 <= q && p[0] == '/' && p[1] == '/')
            {
                p = NULL;
                break;
            }

            /*
             * comments
             */
            if (*p == '#' || *p == ';')
            {
                p = NULL;
                break;
            }
        } /* end of for (...) */

        /*
         * a corrupt line
         */
        if (!ret || p != NULL)
        {
            ret = false;
            break;
        }

        configs.push_back(item);
    } /* end of while (...) */

    ProFree(buf);
    fclose(file);

    return (ret);
}

bool
CProConfigFile::Write(const CProStlVector<PRO_CONFIG_ITEM>& configs,
                      char                                  aroundCharL, /* = '\"' */
                      char                                  aroundCharR) /* = '\"' */
{
    if (m_fileName.empty())
    {
        return (false);
    }

    FILE* const file = fopen(m_fileName.c_str(), "wb");
    if (file == NULL)
    {
        return (false);
    }

    if (fprintf(file, "//#; %cconfig_name%c    %cconfig_value%c\n\n",
        aroundCharL, aroundCharR, aroundCharL, aroundCharR) < 0)
    {
        fclose(file);

        return (false);
    }

    bool ret = true;

    int       i = 0;
    const int c = (int)configs.size();

    for (; i < c; ++i)
    {
        const PRO_CONFIG_ITEM& config = configs[i];

        if (fprintf(file, "%c%s%c    %c%s%c\n",
            aroundCharL, config.configName.c_str() , aroundCharR,
            aroundCharL, config.configValue.c_str(), aroundCharR) < 0)
        {
            ret = false;
            break;
        }

        fflush(file);
    }

    fclose(file);

    return (ret);
}
