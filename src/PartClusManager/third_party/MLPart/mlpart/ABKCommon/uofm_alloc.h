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

#ifndef _UOFMALLOC_
#define _UOFMALLOC_

#include <cstdlib>
#include <new>

// Mateus@180515
#include <cstddef>
//#include <iterator>
//--------------

template <typename T>
class malloc_allocation;

template <>
class malloc_allocation<void> {
       public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;
        template <typename U>
        struct rebind {
                typedef malloc_allocation<U> other;
        };
};

template <typename T>
class malloc_allocation {
       public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T value_type;
        template <typename U>
        struct rebind {
                typedef malloc_allocation<U> other;
        };

        malloc_allocation() throw() {}
        malloc_allocation(const malloc_allocation<value_type>&) throw() {}
        template <typename U>
        malloc_allocation(const malloc_allocation<U>&) throw() {}
        ~malloc_allocation() throw() {}

        pointer address(reference x) const { return (&x); }

        const_pointer address(const_reference x) const { return (&x); }

        pointer allocate(size_type n, malloc_allocation<void>::const_pointer hint = NULL) {
                if (n > max_size()) throw std::bad_alloc();

                pointer ret = static_cast<pointer>(malloc(n * sizeof(value_type)));

                if (n > 0 && ret == NULL) throw std::bad_alloc();

                return ret;
        }

        void deallocate(pointer p, size_type n) { free(p); }

        size_type max_size() const { return (static_cast<size_type>(-1) / sizeof(value_type)); }

        void construct(pointer p, const_reference val) { ::new (p) T(val); }

        void destroy(pointer p) { p->~T(); }
};

template <typename T1, typename T2>
bool operator==(const malloc_allocation<T1>&, const malloc_allocation<T2>&) {
        return true;
}

template <typename T1, typename T2>
bool operator!=(const malloc_allocation<T1>&, const malloc_allocation<T2>&) {
        return false;
}

#include <vector>
#include <string>
#include <sstream>

namespace uofm {
template <typename T>
class vector : public std::vector<T, malloc_allocation<T> > {
       public:
        typedef std::vector<T, malloc_allocation<T> > stlvector;
        explicit vector() : stlvector() {}
        explicit vector(size_t n, const T& val = T()) : stlvector(n, val) {}
        template <class In>
        vector(In first, In last)
            : stlvector(first, last) {}
};

typedef std::basic_string<char, std::char_traits<char>, malloc_allocation<char> > string;

typedef std::basic_stringstream<char, std::char_traits<char>, malloc_allocation<char> > stringstream;

typedef std::basic_ostringstream<char, std::char_traits<char>, malloc_allocation<char> > ostringstream;

typedef std::basic_istringstream<char, std::char_traits<char>, malloc_allocation<char> > istringstream;
}

#endif
