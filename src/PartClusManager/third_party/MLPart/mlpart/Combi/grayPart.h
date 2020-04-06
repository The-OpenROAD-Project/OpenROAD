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

//! author = " Igor Markov"

#ifndef _GRAY_PART_H_
#define _GRAY_PART_H_

#include <math.h>
#include "ABKCommon/abkassert.h"

#ifndef byte
#define byte char
#endif

//: Generates all possible partitioning results of (size) elements sorted by
// Gray code.
//  "Graycode": each of which differs from the preceding expression
//  in one place only, represent the sequence of the partitionings.
//  GrayCodeForPartitionings class requires 40 > size > 1 and 255 > numPart > 1.
class GrayCodeForPartitionings {
        byte* _begin;
        byte* _current;
        byte* _end;
        /*  unsigned \_size;  */
        static byte* _tables[];
        // the size of these two tables
        static byte _numPartForTables[];
        // is defined in the .cxx file
        void initTable(unsigned size, unsigned numPart = 2);

       public:
        GrayCodeForPartitionings(unsigned size, unsigned numPart = 2);
        void startAtEnd() { _current = _end - 1; }

        bool finished() const { return _current == _end; }
        // have we gone through all partitionings
        bool atStart() const { return _current == _begin - 1; }
        unsigned nextIncrement() { return *_current++; }
        // partition assignment to be incremented
        unsigned prevIncrement()
            // partition assignment to be incremented
        {
                return *_current--;
        }
        /*      unsigned resetEndSymmetric()  // uses curr. pointer to shrink
           table
                { unsigned shrinkBy= \_current-\_begin; \_end-=shrinkBy; return
           shrinkBy; }
        */
};

#endif
