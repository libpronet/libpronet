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
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
////

template<typename __T>
class CProStlVector
: public std::vector<__T, std::pro_allocator<__T, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __T>
class CProStlDeque
: public std::deque<__T, std::pro_allocator<__T, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __T, typename __C = CProStlDeque<__T> >
class CProStlQueue
: public std::queue<__T, __C>
{
    DECLARE_SGI_POOL(0)
};

/*
 * We use 'deque' instead of 'vector'.
 */
template<typename __T, typename __C = CProStlDeque<__T>, typename __Pr = std::less<typename __C::value_type> >
class CProStlPriorQueue
: public std::priority_queue<__T, __C, __Pr>
{
    DECLARE_SGI_POOL(0)
};

template<typename __T, typename __C = CProStlDeque<__T> >
class CProStlStack
: public std::stack<__T, __C>
{
    DECLARE_SGI_POOL(0)
};

template<typename __T>
class CProStlList
: public std::list<__T, std::pro_allocator<__T, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __T, typename __Pr = std::less<__K> >
class CProStlMap
: public std::map<__K, __T, __Pr, std::pro_allocator<std::pair<const __K, __T>, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __T, typename __Pr = std::less<__K> >
class CProStlMultimap
: public std::multimap<__K, __T, __Pr, std::pro_allocator<std::pair<const __K, __T>, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __Pr = std::less<__K> >
class CProStlSet
: public std::set<__K, __Pr, std::pro_allocator<__K, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __Pr = std::less<__K> >
class CProStlMultiset
: public std::multiset<__K, __Pr, std::pro_allocator<__K, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __T, typename __H = std::hash<__K>, typename __Pr = std::equal_to<__K> >
class CProStlHashMap
: public std::unordered_map<__K, __T, __H, __Pr, std::pro_allocator<std::pair<const __K, __T>, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __T, typename __H = std::hash<__K>, typename __Pr = std::equal_to<__K> >
class CProStlHashMultimap
: public std::unordered_multimap<__K, __T, __H, __Pr, std::pro_allocator<std::pair<const __K, __T>, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __H = std::hash<__K>, typename __Pr = std::equal_to<__K> >
class CProStlHashSet
: public std::unordered_set<__K, __H, __Pr, std::pro_allocator<__K, 0> >
{
    DECLARE_SGI_POOL(0)
};

template<typename __K, typename __H = std::hash<__K>, typename __Pr = std::equal_to<__K> >
class CProStlHashMultiset
: public std::unordered_multiset<__K, __H, __Pr, std::pro_allocator<__K, 0> >
{
    DECLARE_SGI_POOL(0)
};

typedef std::basic_string<char   , std::char_traits<char>   , std::pro_allocator<char   , 0> > CProStlString;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, std::pro_allocator<wchar_t, 0> > CProStlWstring;

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_STL_H____ */
