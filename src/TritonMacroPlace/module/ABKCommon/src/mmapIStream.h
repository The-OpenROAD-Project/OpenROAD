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


#ifndef MMAPISTREAM_H
#define MMAPISTREAM_H

#include <istream>
#include "mmapStreamBuf.h"

/* 
 * Works like an ifstream, e.g.
 * try { 
 * 	MMapIStream auxFile("file.aux");
 *	auxFile >> str;
 * } catch (...) {
 * 	// this class throws...
 * }
 * 
 * mmap() calls are automatically wrapped by the mmapStreamBuf class
 *
 * (check out sstream, fstream, streambuf and istream header files for more info)
 *
 * -- aaronnn
 */

class MMapIStream : public std::basic_istream<char>
{
private:
  MMapStreamBuf    _M_mmapbuf;

public:
  explicit
  MMapIStream(const char* __s, ios_base::openmode __mode = ios_base::in)
  : __istream_type(NULL), _M_mmapbuf(__s)
  {
    this->init(&_M_mmapbuf);
  }
   /*
    * The destructor does nothing.
    *  The mmap is closed by the streambuf object, not the formatting stream.
    */
  ~MMapIStream()
  { }
};

#endif
