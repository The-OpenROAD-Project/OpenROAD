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


#include "abkmessagebuf.h"

using std::priority_queue;
using std::pair;
using std::make_pair;
using std::ostream;
using std::flush;
using std::string;


ABKMessageBuf::ABKMessageBuf(void): _buf(NULL)
{
  _buf = new priority_queue< pair<unsigned,string> >();
}

void ABKMessageBuf::addMessage(unsigned priority, const string &msg)
{
  if(_buf != NULL) _buf->push(make_pair(priority,msg));
}

void ABKMessageBuf::print(ostream& out)
{
  if(_buf != NULL)
  {
    while(_buf->size() > 0)
    {
      out << _buf->top().second << flush;
      _buf->pop();
    }
    delete _buf;
    _buf = NULL;
  }
}

ABKMessageBuf &getABKMessageBuf(void)
{
  static ABKMessageBuf theBuf;
  return theBuf;
}

ostream& operator<<(ostream& out, ABKMessageBuf& buf)
{
  buf.print(out);
  return out;
}
