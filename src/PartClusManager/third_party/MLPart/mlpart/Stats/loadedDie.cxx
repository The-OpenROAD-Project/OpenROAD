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

// Created: April 10, 1998 by Igor Markov

// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "loadedDie.h"
#include <algorithm>

#ifdef _MSC_VER
#ifndef rint
#define rint(a) int((a) + 0.5)
#endif
#endif

using uofm::vector;

// ---------------------------- IMPLEMENTATIONS ------------------------------

BiasedRandomSelection::BiasedRandomSelection(const vector<unsigned>& chances, RandomRawUnsigned& randu) : _randu(randu), _cumulative(chances) {
        abkassert(chances.size() > 1, " No chances ! ");
        vector<unsigned>::iterator it1 = _cumulative.begin(), it2 = _cumulative.begin() + 1;
        for (; it2 != _cumulative.end(); it1++, it2++) (*it2) += (*it1);
}

BiasedRandomSelection::BiasedRandomSelection(const vector<double>& chances, RandomRawUnsigned& randu) : _randu(randu), _cumulative(chances.size()) {
        abkassert(chances.size() > 1, " No chances ! ");
        vector<double>::const_iterator it0 = chances.begin() + 1;
        vector<unsigned>::iterator it1 = _cumulative.begin(), it2 = _cumulative.begin() + 1;
        (*it1) = static_cast<unsigned>(rint(chances.front()));
        for (; it2 != _cumulative.end(); it0++, it1++, it2++) (*it2) = static_cast<unsigned>(rint((*it1) + (*it0)));
}

BiasedRandomSelection::operator unsigned() const {
        // if (_cumulative.back()<=0) abk_call_debugger();
        abkfatal(_cumulative.size(), " Empty sum. chances ");
        abkfatal2(_cumulative.back() > 0, " Total chances ", _cumulative.back());
        unsigned drop = _randu % (_cumulative.back());
        return std::upper_bound(_cumulative.begin(), _cumulative.end() - 1, drop) - _cumulative.begin();
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "980410, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif
