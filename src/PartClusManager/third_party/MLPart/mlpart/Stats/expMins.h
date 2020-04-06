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

#ifndef _EXPECTED_MINS_H
#define _EXPECTED_MINS_H

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <iostream>

class ExpectedMins {
        uofm::vector<double> _data;
        uofm::vector<double> _exps;
        unsigned _oversampl;

        void _ctruct();

       public:
        ExpectedMins(const uofm::vector<unsigned>& data, unsigned oversampl = UINT_MAX);
        ExpectedMins(const uofm::vector<double>& data, unsigned oversampl = UINT_MAX);

        unsigned getSize() const { return _exps.size(); }

        double operator[](unsigned idx) const {
                abkfatal2(idx > 0 && idx < _exps.size(), " Index out of range: ", idx);
                return _exps[idx];
        }
};

std::ostream& operator<<(std::ostream&, const ExpectedMins&);

#endif
