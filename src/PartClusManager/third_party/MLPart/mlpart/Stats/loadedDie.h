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

//! author="April 10, 1998 by Igor Markov"
//! CHANGES=" loadedDie.h 9804010 ilm "

#ifndef _LOADEDDIE_H_
#define _LOADEDDIE_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <iostream>

class BiasedRandomSelection;
typedef BiasedRandomSelection LoadedDie;

/* ---------------------------- INTERFACES ------------------------------ */

//: Allows to randomly choose one of several options (numbered 0..K)
//  with prescribed probabilities. The analogy is with throwing a
//  die when all outcomes are not equally probable (loaded die).
class BiasedRandomSelection {
        RandomRawUnsigned& _randu;
        uofm::vector<unsigned> _cumulative;

       public:
        BiasedRandomSelection(const uofm::vector<double>& chances, RandomRawUnsigned& randu);
        BiasedRandomSelection(const uofm::vector<unsigned>& chances, RandomRawUnsigned& randu);
        operator unsigned() const;
};

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "980410, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
