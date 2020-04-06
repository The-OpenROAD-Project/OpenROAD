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

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <string.h>
#include <numeric>

// Mateus@180515
#include <algorithm>
//--------------

#include "ABKCommon/abkcommon.h"
#include "grayPart.h"
#include "ABKCommon/sgi_stl_compat.h"

using std::swap;
using std::min;
using std::max;
using std::sort;

#if (GCC_VERSION >= 30100)
#include <ext/numeric>
using __gnu_cxx::power;
#else
using std::power;
#endif

// #ifdef __SUNPRO_CC
//  unsigned power(unsigned base, unsigned p) { return UINT_MAX; }
// #endif

// requires XXX GB RAM
const unsigned maxSize = 40;
byte* GrayCodeForPartitionings::_tables[maxSize];
byte GrayCodeForPartitionings::_numPartForTables[maxSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void GrayCodeForPartitionings::initTable(unsigned size, unsigned numPart) {
        abkfatal3(size < maxSize,
                  " Partitioning too large for Gray codes ---"
                  " can only work with ",
                  maxSize, "\n");
        double tSize = pow(static_cast<double>(numPart), static_cast<double>(size));
        abkfatal3(tSize < 20 * 1024 * 1024,
                  " Can not exceed 20Mbs for Gray codes --"
                  " requested ",
                  tSize / (1024. * 1024.), " MB \n");
        unsigned tableSize = static_cast<unsigned>(tSize);

        abkwarn3(tableSize < 1024 * 1024, "Generating GrayCodeForPartitionings taking ", tableSize / (1024. * 1024.), " MB of RAM;\n");
        if (_tables[size]) delete[] _tables[size];
        _tables[size] = new byte[tableSize];
        _numPartForTables[size] = numPart;
        byte* begin = _tables[size];
        byte* ptr = begin;

        // generate tables

        unsigned p;
        for (p = numPart - 1; p != 0; p--) *ptr++ = 0;  // initialize recursion

        for (unsigned k = 1; k != size; k++) {
                unsigned bytesToCopy = ptr - begin;
                for (p = numPart - 1; p != 0; p--) {
                        *ptr++ = k;
                        memcpy(ptr, begin, bytesToCopy);
                        ptr += bytesToCopy;
                }
        }
}

GrayCodeForPartitionings::GrayCodeForPartitionings(unsigned size, unsigned numPart) {
        abkfatal(size > 1, "GrayCodeForPartitionings class requires size > 1");
        abkfatal(numPart > 1, "GrayCodeForPartitionings class requires numPart > 1");
        abkfatal(numPart < 255, "GrayCodeForPartitionings class requires numPart <255");
        if (!_tables[size] || _numPartForTables[size] != int(numPart)) initTable(size, numPart);
        _begin = _current = _tables[size];
        unsigned tableSize = power(numPart, size);
        /* In case STL is not available
            unsigned tableSize=numPart;
            for(unsigned k=size-1; k!=0; k--) tableSize*=numPart;
        */
        _end = _current + tableSize - 1;
}
