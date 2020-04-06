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

//! author="Mike Oliver 03/14/99 "

#if !defined(RISKYSTACK_H__INCLUDED_)
#define RISKYSTACK_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000

#include <vector>
#include <ABKCommon/abkassert.h>

//: this container is intended to be used as a faster stack,
// not a general sequence like a vector.
// It's like a vector but
//        i) doesn't do bounds checking.  That's the whole
//        point, because bounds checking takes time, and this
//        is intended for tight loops.
//
//        ii) doesn't have the full interface of vector.  this
//        is intentional, as those interfaces could invalidate
//        pointers.  basically, you're allowed to push_back() and
//        pop_back() and look at/modify back() (the top element of the
//        stack), and you can also look at/modify other elements
//        using operator[], but you can't get any iterators
//        to the underlying vector, so you can't sort it or
//        anything like that
template <class T>
class RiskyStack {
        T *_meat;
        T *_end;
        unsigned _maxSize;

       public:
        RiskyStack() : _meat(new T[2]), _end(_meat), _maxSize(2) {
#ifdef ABKDEBUG
                _meat[0] = T();
                _meat[1] = T();
#endif
        }
        RiskyStack(unsigned maxSize) : _meat(new T[maxSize]), _end(_meat), _maxSize(maxSize) {
#ifdef ABKDEBUG
                for (unsigned i = 0; i < maxSize; i++) _meat[i] = T();
#endif
        }

        RiskyStack(RiskyStack const &rhs) {
                _meat = new T[rhs.size()];
                memcpy(_meat, rhs._meat, rhs.size() * sizeof(T));
                _end = _meat + rhs.size();
                _maxSize = rhs._maxSize;
        }

        ~RiskyStack() { delete[] _meat; }

        inline void push_back(const T &val) {
                *(_end++) = val;
                abkassert(size() <= _maxSize, "overrun");
        }
        inline void pop_back() { _end--; }
        inline size_t size() const { return _end - _meat; }
        inline bool empty() const { return _end == _meat; }
        inline T operator[](unsigned idx) const { return _meat[idx]; }
        inline T &operator[](unsigned idx) { return _meat[idx]; }
        inline T back() const { return *(_end - 1); }
        inline T &back() { return *(_end - 1); }

        // This should only be used when the value has been assigned
        inline T backPlusOne() const { return *_end; }
        inline T &backPlusOne() { return *_end; }

        inline void reserve(unsigned n);
        inline RiskyStack &operator=(RiskyStack const &rhs) {
                delete[] _meat;
                _meat = new T[rhs.size()];
                memcpy(_meat, rhs._meat, rhs.size() * sizeof(T));
                _end = _meat + rhs.size();
                _maxSize = rhs._maxSize;
                return *this;
        }
};

template <class T>
inline void RiskyStack<T>::reserve(unsigned n) {
        unsigned s = _maxSize;
        unsigned oldStackSize = size();
        if (n > s) {
                T *old = _meat;
                _meat = new T[n];
                _maxSize = n;
                memcpy(_meat, old, s * sizeof(T));
                delete[] old;
                _end = _meat + oldStackSize;

#ifdef ABKDEBUG
                for (unsigned i = s; i < _maxSize; i++) _meat[i] = T();
#endif
        }
}

#endif  // !defined(RISKYSTACK_H__INCLUDED_)
