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

//! author="Max Moroz Sept 1998"
// 020811  ilm    ported to g++ 3.0

#ifndef _listO1size_h
#define _listO1size_h

#include <iostream>
#include <list>
//#include <slist>	//slist is an SGI extension

//: derived from list and slist with O(1) size operation
template <class T>
class listO1size : public std::list<T> {
       public:
        typedef typename std::list<T>::iterator iterator;

       protected:
        unsigned mSize;

       public:
        unsigned size() const { return mSize; }
        listO1size() : std::list<T>(), mSize(0) {}
        void push_front(const T& x) {
                ++mSize;
                std::list<T>::push_front(x);
        }
        void push_back(const T& x) {
                ++mSize;
                std::list<T>::push_back(x);
        }
        void pop_front() {
                --mSize;
                std::list<T>::pop_front();
        }
        void pop_back() {
                --mSize;
                std::list<T>::pop_back();
        }
        iterator insert(iterator position, const T& x) {
                ++mSize;
                return std::list<T>::insert(position, x);
        }
        void erase(iterator position) {
                --mSize;
                std::list<T>::erase(position);
        }
        void clear() {
                mSize = 0;
                std::list<T>::clear();
        }
};

//:
// class slistO1size : public slist<T>
template <class T>
class slistO1size : public std::list<T> {
       public:
        typedef typename std::list<T>::iterator iterator;

       protected:
        unsigned mSize;

       public:
        unsigned size() const { return mSize; }
        slistO1size() : std::list<T>(), mSize(0) {}
        void push_front(const T& x) {
                ++mSize;
                std::list<T>::push_front(x);
        }
        void pop_front() {
                --mSize;
                std::list<T>::pop_front();
        }
        iterator insert(iterator position, const T& x) {
                ++mSize;
                return std::list<T>::insert(position, x);
        }
        void erase(iterator position) {
                --mSize;
                std::list<T>::erase(position);
        }
        void clear() {
                mSize = 0;
                std::list<T>::clear();
        }
};

#endif
