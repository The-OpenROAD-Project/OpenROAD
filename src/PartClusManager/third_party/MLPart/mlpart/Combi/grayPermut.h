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

//! author="Max Moroz"

#ifndef _GRAY_PERMUT_H_
#define _GRAY_PERMUT_H_

#include "permut.h"

typedef unsigned char byte;

//: Uses recursion to generate the permutation of [size] elements
//  satisfy the rule of Gray Code.
class GrayTranspositionForPermutations {
        byte* _current;
        byte* _end;
        unsigned _size;
        static byte* _tables[];
        void initTable(unsigned size);

       public:
        GrayTranspositionForPermutations(unsigned size);

        bool finished() const;
        // have we gone through all permutations?

        unsigned next();
        // the next transposition is (q, q+1)
        // where q is whatever the following method returns
        // note: permutation elements are indexed as 0,1,...,size-1
        // note: q will never exceed size-2)

        Permutation getPermutation() const;
        // obtain the permutation corresponding to the current state of the
        // object
};

/**** IMPLEMENTATIONS ***/

inline GrayTranspositionForPermutations::GrayTranspositionForPermutations(unsigned size) {
        abkfatal(size > 1, "GrayTranspositionForPermutations class requires size > 1");
        if (!_tables[size]) initTable(size);
        _size = size;  // only for getPermutation()!!!
        _current = _tables[size];
        _end = _current + abkFactorial(size) - 1;
}

inline bool GrayTranspositionForPermutations::finished() const { return _current == _end; }

inline unsigned GrayTranspositionForPermutations::next() { return *_current++; }

#endif
