/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#include "ABKCommon/sgi_stl_compat.h"

#ifndef _SGI_HASHMAP_H
#define _SGI_HASHMAP_H
#ifdef __GNUC__
#if (__GNUC__ >= 3)
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <ext/hash_map>
#include "uofm_alloc.h"
#if (GCC_VERSION >= 30100)
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;
namespace __gnu_cxx {
template <>
struct hash<uofm::string> {
        size_t operator()(const uofm::string& x) const { return hash<const char*>()(x.c_str()); }
};

template <typename A, typename B>
struct hash<std::pair<A, B> > {
        size_t operator()(const std::pair<A, B>& x) const { return hash<A>()(x.first) * (1 + hash<B>()(x.second)); }
};
}
#else
using std::hash_map;
using std::hash;
#endif
#else
#include <hash_map>
#include "uofm_alloc.h"
template <>
struct hash<uofm::string> {
        size_t operator()(const uofm::string& x) const { return hash<const char*>()(x.c_str()); }
};
#endif
#else
#ifdef _MSC_VER
#include <hash_map>
#ifndef _ONEHASH
#define _ONEHASH
struct lessstr {
        bool operator()(char const *s1, char const *s2) const { return (strcmp(s1, s2) == -1); }
};
template <class T>
class hash : public stdext::hash_compare<T, std::less<T> > {};
template <>
class hash<const char *> : public stdext::hash_compare<const char *, lessstr> {};
namespace stdext {
template <typename A, typename B>
std::size_t hash_value(const std::pair<A, B> &p) {
        return hash_value(p.first) * (1 + hash_value(p.second));
}
}
#endif
template <class one, class two, class three, class four>
class hash_map : public stdext::hash_map<one, two, three> {};
#else
#include "ABKCommon/SGI_STL_COMPAT/hash_map.h"
#endif
#endif
#endif

#include <functional>
using std::equal_to;
