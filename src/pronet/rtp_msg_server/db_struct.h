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

#ifndef DB_STRUCT_H
#define DB_STRUCT_H

#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

/*
 * [[[[ column types
 */
typedef unsigned char DB_COLUMN_TYPE;

static const DB_COLUMN_TYPE DB_CT_I64 = 1;
static const DB_COLUMN_TYPE DB_CT_DBL = 2;
static const DB_COLUMN_TYPE DB_CT_TXT = 3;
/*
 * ]]]]
 */

struct DB_CELL_UNIT
{
    DB_CELL_UNIT()
    {
        i64 = 0;
        dbl = 0;
        txt = "";
    }

    int64_t       i64;
    double        dbl;
    CProStlString txt;

    DECLARE_SGI_POOL(0)
};

struct DB_ROW_UNIT
{
    CProStlVector<DB_CELL_UNIT> cells;

    DECLARE_SGI_POOL(0)
};

struct DB_ROW_SET
{
    CProStlVector<DB_COLUMN_TYPE> types_in;
    CProStlDeque<DB_ROW_UNIT>     rows_out;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* DB_STRUCT_H */
