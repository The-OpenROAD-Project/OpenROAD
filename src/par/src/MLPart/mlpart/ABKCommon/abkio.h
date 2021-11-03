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

//! author="Igor Markov June 22, 1997 "
//! CHANGES="abkio.h  970827 ilm  added arrangements for the needword(const char
//*) manipulator ilm  uninlined manipulators 980213 ilm  renamed into abkio.h
// ilm  added printRange() 980320 ilm  added optional lineNo arguments to
// asserting input manipulators"

/* This file to be included into all projects in the group

 CHANGES
  970827 ilm  added arrangements for the needword(const char *) manipulator
        ilm  uninlined manipulators
  980213 ilm  renamed into abkio.h
         ilm  added printRange()
  980320 ilm  added optional lineNo arguments to asserting input manipulators
*/

#ifndef _ABKIO_H_
#define _ABKIO_H_
#include <iostream>
#include <iomanip>
#include <vector>

std::istream& eatblank(std::istream& i);
std::istream& skiptoeol(std::istream& i);

/*
std::istream& eathash (std::istream\& i);
std::istream& needeol (std::istream\& i);
std::istream& noeol   (std::istream\& i);
std::istream& isnumber(std::istream\& i);
std::istream& isword  (std::istream\& i);
*/

//: Function template for the manipulators in std::istream and std::ostream
template <class Arg>
class ManipFuncObj1 {
        std::istream& (*_func)(std::istream&, Arg);
        Arg _arg;

       public:
        ManipFuncObj1(std::istream& (*f)(std::istream&, Arg), Arg arg) : _func(f), _arg(arg) {}
        std::istream& operator()(std::istream& i) const { return (*_func)(i, _arg); }
};

template <class Arg>
std::istream& operator>>(std::istream& is, const ManipFuncObj1<Arg>& im) {
        return im(is);
}

template <class Arg1, class Arg2>
class ManipFuncObj2 {
        std::istream& (*_func)(std::istream&, Arg1, Arg2);
        Arg1 _arg1;
        Arg2 _arg2;

       public:
        ManipFuncObj2(std::istream& (*f)(std::istream&, Arg1, Arg2), Arg1 arg1, Arg2 arg2) : _func(f), _arg1(arg1), _arg2(arg2) {}
        std::istream& operator()(std::istream& i) const { return (*_func)(i, _arg1, _arg2); }
};

template <class Arg1, class Arg2>
std::istream& operator>>(std::istream& is, const ManipFuncObj2<Arg1, Arg2>& im) {
        return im(is);
}

ManipFuncObj1<int&> eathash(int& lineNo);
ManipFuncObj1<int> needeol(int lineNo = -1);
ManipFuncObj1<int> noeol(int lineNo = -1);
// for 64-bit. isnumber is a system function. replace with my_isnumber()
ManipFuncObj1<int> my_isnumber(int lineNo = -1);
ManipFuncObj1<int> isword(int lineNo = -1);

ManipFuncObj2<const char*, int> needword(const char* word, int lineNo = -1);
ManipFuncObj2<const char*, int> needcaseword(const char* word, int lineNo = -1);

template <class T, class A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T, A>& rhs) {
        unsigned size = rhs.size();
        for (unsigned i = 0; i != size; ++i) {
                if (i % 10 == 0) {
                        if (i) out << std::endl;
                        out << "     ";
                }
                out << std::setw(6) << rhs[i] << " ";
        }
        out << "\n";
        return out;
}

template <class Iter>
inline unsigned printRange(std::ostream& out, Iter first, Iter last) {
        unsigned count = 0;
        for (; first != last; first++, count++) {
                if (count % 10 == 0) {
                        if (count) out << '\n';
                        out << "     ";
                }
                out << std::setw(6) << *first << ' ';
        }
        out << '\n';
        return count;
}

#endif
