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

// Created: Aug 20, 1997 by Igor Markov

// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "keyCounter.h"

#include <iostream>
#include <ABKCommon/sgi_hash_map.h>
#include <algorithm>
#include <utility>
#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>

using std::ostream;
using std::endl;
using std::pair;
using std::sort;
using uofm::vector;

class KeyCounter;

// ---------------------------- IMPLEMENTATIONS ------------------------------

unsigned KeyCounter::addKey(unsigned key) {
        if (_threshold < key) return 0;
        iterator pit = find(key);
        if (pit != end()) return ++((*pit).second);

        insert(KeyCounter::value_type(key, 1));
        return 1;
}

unsigned KeyCounter::operator[](unsigned key) const {
        if (_threshold < key) return 0;
        const_iterator pit = find(key);
        if (pit == end())
                return 0;
        else
                return (*pit).second;
}

inline bool operator<(const pair<unsigned, unsigned>& p1, const pair<unsigned, unsigned>& p2) { return p1.first < p2.first; }

ostream& operator<<(ostream& os, const KeyCounter& phm) {
        vector<pair<unsigned, unsigned> > keyValPairs(phm.size());
        // We're not using copy due to bug in MSVC++
        // copy(phm.begin(), phm.end(), keyValPairs.begin());
        KeyCounter::const_iterator iKC;
        vector<pair<unsigned, unsigned> >::iterator iKVP;
        for (iKC = phm.begin(), iKVP = keyValPairs.begin(); iKC != phm.end(); iKC++, iKVP++) {
                (*iKVP).first = (*iKC).first;
                (*iKVP).second = (*iKC).second;
        }

        sort(keyValPairs.begin(), keyValPairs.end());

        for (unsigned i = 0; i != keyValPairs.size(); i++) {
                if (i % 5 == 0) {
                        if (i) os << "\n";
                        os << " ";
                }
                char buf[127];
                sprintf(buf, "% 5d: %-6d", keyValPairs[i].first, keyValPairs[i].second);
                os << buf;
        }

        return os << endl;
}
