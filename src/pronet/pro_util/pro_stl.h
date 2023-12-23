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

#ifndef ____PRO_STL_H____
#define ____PRO_STL_H____

#include "pro_a.h"
#include "pro_memory_pool.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include <algorithm>
#include <array>
#include <bitset>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
////

template<class ____Ty, unsigned int ____poolIndex = 0>
class CProStlVector
: public std::vector<____Ty, std::pro_allocator<____Ty, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____Ty, unsigned int ____poolIndex = 0>
class CProStlDeque
: public std::deque<____Ty, std::pro_allocator<____Ty, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____Ty, unsigned int ____poolIndex = 0>
class CProStlList
: public std::list<____Ty, std::pro_allocator<____Ty, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, class ____Ty, unsigned int ____poolIndex = 0, class ____Pr = std::less<____K> >
class CProStlMap
: public std::map<____K, ____Ty, ____Pr, std::pro_allocator<std::pair<const ____K, ____Ty>, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, class ____Ty, unsigned int ____poolIndex = 0, class ____Pr = std::less<____K> >
class CProStlMultimap
: public std::multimap<____K, ____Ty, ____Pr, std::pro_allocator<std::pair<const ____K, ____Ty>, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, unsigned int ____poolIndex = 0, class ____Pr = std::less<____K> >
class CProStlSet
: public std::set<____K, ____Pr, std::pro_allocator<____K, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, unsigned int ____poolIndex = 0, class ____Pr = std::less<____K> >
class CProStlMultiset
: public std::multiset<____K, ____Pr, std::pro_allocator<____K, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, class ____Ty, unsigned int ____poolIndex = 0, class ____H = std::hash<____K>, class ____Pr = std::equal_to<____K> >
class CProStlHashMap
: public std::unordered_map<____K, ____Ty, ____H, ____Pr, std::pro_allocator<std::pair<const ____K, ____Ty>, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, class ____Ty, unsigned int ____poolIndex = 0, class ____H = std::hash<____K>, class ____Pr = std::equal_to<____K> >
class CProStlHashMultimap
: public std::unordered_multimap<____K, ____Ty, ____H, ____Pr, std::pro_allocator<std::pair<const ____K, ____Ty>, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, unsigned int ____poolIndex = 0, class ____H = std::hash<____K>, class ____Pr = std::equal_to<____K> >
class CProStlHashSet
: public std::unordered_set<____K, ____H, ____Pr, std::pro_allocator<____K, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

template<class ____K, unsigned int ____poolIndex = 0, class ____H = std::hash<____K>, class ____Pr = std::equal_to<____K> >
class CProStlHashMultiset
: public std::unordered_multiset<____K, ____H, ____Pr, std::pro_allocator<____K, ____poolIndex> >
{
    DECLARE_SGI_POOL(0)
};

/*
 * By default, CProStlString and CProStlWstring use the pool #0.
 */
typedef std::basic_string<char   , std::char_traits<char>   , std::pro_allocator<char   , 0> > CProStlString;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, std::pro_allocator<wchar_t, 0> > CProStlWstring;

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_STL_H____ */
