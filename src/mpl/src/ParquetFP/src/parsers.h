/**************************************************************************
***    
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
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


#ifndef PARSERS_H
#define PARSERS_H

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace parse_utils
{
   struct ltstr
   {
      inline bool operator ()(const char* s1, const char* s2) const
         {   return strcmp(s1, s2) < 0; }
   };
   
   struct Point
   {
      float x;
      float y;
   };
   
   struct IntPoint
   {
      int x;
      int y;
   };
   
   // global parsing functions
   inline void eatblank(std::ifstream& i);   
   inline void skiptoeol(std::ifstream& i);   
   inline void eathash(std::ifstream& i);   
   inline bool needCaseChar(std::ifstream& i, char character);
}
// namespace parse_utils
   
// =========================
//      IMPLEMENTATIONS
// =========================
inline void parse_utils::eatblank(std::ifstream& i)
{
   while (i.peek()==' ' || i.peek()=='\t')
      i.get();
}
// --------------------------------------------------------
inline void parse_utils::skiptoeol(std::ifstream& i)
{
   while (!i.eof() && i.peek()!='\n' && i.peek()!='\r')
      i.get();
   i.get();
}
// --------------------------------------------------------
inline bool parse_utils::needCaseChar(std::ifstream& i,
                                      char character)
{
   while(!i.eof() && i.peek() != character)
      i.get();
   if(i.eof())
      return false;
   else
      return true;
}
// --------------------------------------------------------
inline void parse_utils::eathash(std::ifstream& i)
{
  skiptoeol(i);
}
// --------------------------------------------------------
#endif
