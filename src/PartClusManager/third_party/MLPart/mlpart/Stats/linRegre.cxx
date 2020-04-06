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

// Created 970926 by Igor Markov
// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "linRegre.h"

using uofm::vector;

LinearRegression::LinearRegression(const vector<double>& x, const vector<double>& y) : _k(0.0) {
        abkfatal(x.size() == y.size(),
                 "Attempt to compute linear regression for vectors of difft "
                 "sizes \n");

        unsigned i, size = x.size();
        double sx = 0, sy = 0;

        for (i = 0; i != size; i++) {
                sx += x[i];
                sy += y[i];
        }

        double sxoss = sx / size;
        double st2 = 0;
        for (i = 0; i != size; i++) {
                double t = x[i] - sxoss;
                st2 += square(t);
                _k += t * y[i];
        }

        _k /= st2;
        _c = (sy - sx * _k) / size;
}

LinearRegression::LinearRegression(const vector<unsigned>& x, const vector<unsigned>& y) : _k(0.0) {
        abkfatal(x.size() == y.size(),
                 "Attempt to compute linear regression for vectors of difft "
                 "sizes \n");

        unsigned i, size = x.size();
        double sx = 0, sy = 0;

        for (i = 0; i != size; i++) {
                sx += x[i];
                sy += y[i];
        }

        double sxoss = sx / size;
        double st2 = 0;
        for (i = 0; i != size; i++) {
                double t = x[i] - sxoss;
                st2 += square(t);
                _k += t * y[i];
        }

        _k /= st2;
        _c = (sy - sx * _k) / size;
}
