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

/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#ifndef __SGI_STL_COMPAT
#define __SGI_STL_COMPAT

#if defined(__GNUC__)
#define ported_from_sgi std
#else

#include <utility>
#include <algorithm>
#include <iterator>
#include <functional>

// #include <ABKCommon/SGI_STL_COMPAT/hash_map.h>

#ifdef _MSC_VER
// HP and SGI copyrights do not apply to these definitions of
// std::min and std::max
#if _MSC_VER < 1300
namespace std {
template <class T>
const T& min(const T& x, const T& y) {
        if (x < y)
                return x;
        else
                return y;
}
template <class T>
const T& max(const T& x, const T& y) {
        if (x > y)
                return x;
        else
                return y;
}
};
#endif
#endif

namespace std {

template <class _ForwardIterator, class _Tp>
void iota(_ForwardIterator _first, _ForwardIterator _last, _Tp _val) {
        while (_first != _last) *_first++ = _val++;
}

template <class _Base, class _Integer>
_Base power(_Base _x, _Integer _n) {
        if (_n == 0)
                return 1;
        else {
                while ((_n & 1) == 0) {
                        _n >>= 1;
                        _x = _x * _x;
                }

                _Base _result = _x;
                _n >>= 1;
                while (_n != 0) {
                        _x = _x * _x;
                        if ((_n & 1) != 0) _result = _result * _x;
                        _n >>= 1;
                }
                return _result;
        }
}

template <class _ForwardIter, class _OutputIter, class _Distance>
_OutputIter random_sample_n(_ForwardIter _first, _ForwardIter _last, _OutputIter _out, const _Distance _n) {
        _Distance _remaining = _last - _first;
        _Distance _m = std::min(_n, _remaining);

        while (_m > 0) {
                if (__random_number(_remaining) < _m) {
                        *_out = *_first;
                        ++_out;
                        --_m;
                }

                --_remaining;
                ++_first;
        }
        return _out;
}

template <class _ForwardIter, class _OutputIter, class _Distance, class _RandomNumberGenerator>
_OutputIter random_sample_n(_ForwardIter _first, _ForwardIter _last, _OutputIter _out, const _Distance _n, _RandomNumberGenerator& _rand) {
        _Distance _remaining = _last - _first;
        _Distance _m = std::min(_n, _remaining);

        while (_m > 0) {
                if (_rand(_remaining) < _m) {
                        *_out = *_first;
                        ++_out;
                        --_m;
                }

                --_remaining;
                ++_first;
        }
        return _out;
}

template <class _InputIter, class _RandomAccessIter, class _Distance>
_RandomAccessIter _random_sample(_InputIter _first, _InputIter _last, _RandomAccessIter _out, const _Distance _n) {
        _Distance _m = 0;
        _Distance _t = _n;
        for (; _first != _last && _m < _n; ++_m, ++_first) _out[_m] = *_first;

        while (_first != _last) {
                ++_t;
                _Distance _M = __random_number(_t);
                if (_M < _n) _out[_M] = *_first;
                ++_first;
        }

        return _out + _m;
}

template <class _InputIter, class _RandomAccessIter, class _RandomNumberGenerator, class _Distance>
_RandomAccessIter _random_sample(_InputIter _first, _InputIter _last, _RandomAccessIter _out, _RandomNumberGenerator& _rand, const _Distance _n) {
        _Distance _m = 0;
        _Distance _t = _n;
        for (; _first != _last && _m < _n; ++_m, ++_first) _out[_m] = *_first;

        while (_first != _last) {
                ++_t;
                _Distance _M = _rand(_t);
                if (_M < _n) _out[_M] = *_first;
                ++_first;
        }

        return _out + _m;
}

template <class _InputIter, class _RandomAccessIter>
inline _RandomAccessIter random_sample(_InputIter _first, _InputIter _last, _RandomAccessIter _out_first, _RandomAccessIter _out_last) {
        return _random_sample(_first, _last, _out_first, _out_last - _out_first);
}

template <class _InputIter, class _RandomAccessIter, class _RandomNumberGenerator>
inline _RandomAccessIter random_sample(_InputIter _first, _InputIter _last, _RandomAccessIter _out_first, _RandomAccessIter _out_last, _RandomNumberGenerator& _rand) {
        return _random_sample(_first, _last, _out_first, _rand, _out_last - _out_first);
}

template <class _ForwardIter>
bool is_sorted(_ForwardIter _first, _ForwardIter _last) {
        if (_first == _last) return true;

        _ForwardIter _next = _first;
        for (++_next; _next != _last; _first = _next, ++_next) {
                if (*_next < *_first) return false;
        }

        return true;
}

template <class _ForwardIter, class _StrictWeakOrdering>
bool is_sorted(_ForwardIter _first, _ForwardIter _last, _StrictWeakOrdering _comp) {
        if (_first == _last) return true;

        _ForwardIter _next = _first;
        for (++_next; _next != _last; _first = _next, ++_next) {
                if (_comp(*_next, *_first)) return false;
        }

        return true;
}

// identity_element (not part of the C++ standard).
/* #ifdef __NO_SUCH_SYMBOL__

template <class _Tp> inline _Tp identity_element(plus<_Tp>)
{ return _Tp(0); }

template <class _Tp> inline _Tp identity_element(multiplies<_Tp>)
{ return _Tp(1); }

#endif
*/

// identity is an extensions: it is not part of the standard.

template <class _Tp>
struct _Identity : public unary_function<_Tp, _Tp> {
        const _Tp& operator()(const _Tp& _x) const { return _x; }
};

template <class _Tp>
struct identity : public _Identity<_Tp> {};

template <class Pair, class U>
// JDJ (CW Pro1 doesn't like const when first_type is also const)
struct _select1st_hint : public unary_function<Pair, U> {
        const U& operator()(const Pair& x) const { return x.first; }
};

template <class Pair>
struct select1st : public unary_function<Pair, typename Pair::first_type> {
        const typename Pair::first_type& operator()(const Pair& x) const { return x.first; }
};

template <class Pair>
struct select2nd : public unary_function<Pair, typename Pair::second_type> {
        const typename Pair::second_type& operator()(const Pair& x) const { return x.second; }
};
};

#endif

#endif
