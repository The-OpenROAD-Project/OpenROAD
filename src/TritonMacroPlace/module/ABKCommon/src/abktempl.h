/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
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


//! author=" Igor Markov and Max Moroz" 
//! CHANGES="abktempl.h  970820   ilm included templated function clear(Sequence &) by msm moved string stuff into abkcommon.h"

/*
 This file to be included into all projects in the group
 CHANGES
 970820   ilm included templated function clear(Sequence&) by msm
             moved "string stuff" into abkcommon.h
*/
#ifndef  _ABKTEMPL_H_
#define  _ABKTEMPL_H_

/*
 #ifdef __GNUC__
 #if ( __GNUC__ >= 3)
  #include <iostream>
 #else
  #include <iostream>
 #endif
#else
  #include <iostream>
#endif
*/
#include <iostream>
#include <utility>

//: Use this class as template argument
class Empty { };
//  when you mean "nothing" (or "no user informathing")

inline std::ostream& operator<<(std::ostream& out, const Empty&) 
{ return out; }
inline std::istream& operator>>(std::istream& in,        Empty&) 
{ return in;  }

const Empty empty=Empty();

//template <class T>
//inline T abs(const T& a) {
//    return  (0 <= a) ? a : -a;
//}

template <class T>
inline T square(const T& a) {
    return  a*a;
}

template <class Sequence>
void  deleteAllPointersIn(Sequence& s)
{
   for (typename Sequence::iterator it=s.begin();it!=s.end();it++)
   delete *it; 
}

template <class Iterator>
Iterator next(Iterator i)
{
      return ++i;
}

template <class T>
T clone(const T& t)
{
      return T(t);
}



#endif 
