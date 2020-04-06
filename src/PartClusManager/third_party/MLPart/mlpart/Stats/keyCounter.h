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

//! author="Aug 20, 1997 by Igor Markov"

#ifndef _KEYCOUNTER_H_
#define _KEYCOUNTER_H_

#include <ABKCommon/sgi_hash_map.h>
#include <ABKCommon/abkcommon.h>

#include <iostream>

/* ---------------------------- INTERFACES ------------------------------ */

//:Computes the frequence of all values in a given statistical sample
class KeyCounter : public hash_map<unsigned, unsigned, hash<unsigned>, equal_to<unsigned> > {
        unsigned _threshold;
        // the key must be less than \_threshold
       public:
        KeyCounter(unsigned threshold = UINT_MAX) : _threshold(threshold) {
                if (_threshold == 0) threshold = UINT_MAX;
        }
        unsigned operator[](unsigned key) const;
        // get the number of the given key
        unsigned operator+=(unsigned key) { return addKey(key); }
        // increase the number of the given key by 1
        unsigned addKey(unsigned key);
        // increase the number of the given key by 1
};

std::ostream& operator<<(std::ostream&, const KeyCounter& stats);

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "980825, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
