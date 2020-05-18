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

// Created:    Igor Markov,  VLSI CAD ABKGROUP UCLA  Sept 29, 1997
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/uofm_alloc.h>
#include <Placement/plStats.h>
#include <Placement/placement.h>
#include <Stats/trivStats.h>
#include <Stats/rancor.h>

using uofm::vector;

PlCorrelation::PlCorrelation(const Placement& pl1, const Placement& pl2) {
        unsigned size = pl1.getSize();
        abkfatal(size == pl2.getSize(), "Attempting to correlate placements of different sizes");

        vector<double> pl1x(size);
        vector<double> pl2x(size);
        vector<double> pl1y(size);
        vector<double> pl2y(size);
        for (unsigned k = 0; k != size; k++) {
                pl1x[k] = pl1[k].x;
                pl1y[k] = pl1[k].y;
                pl2x[k] = pl2[k].x;
                pl2y[k] = pl2[k].y;
        }
        _x = Correlation(pl1x, pl2x);
        _y = Correlation(pl1y, pl2y);
}

PlRankCorrelation::PlRankCorrelation(const Placement& pl1, const Placement& pl2) {
        unsigned size = pl1.getSize();
        abkfatal(size == pl2.getSize(), "Attempting to correlate placements of different sizes");

        vector<double> pl1x(size);
        vector<double> pl2x(size);
        vector<double> pl1y(size);
        vector<double> pl2y(size);
        for (unsigned k = 0; k != size; k++) {
                pl1x[k] = pl1[k].x;
                pl1y[k] = pl1[k].y;
                pl2x[k] = pl2[k].x;
                pl2y[k] = pl2[k].y;
        }
        _x = RankCorrelation(pl1x, pl2x);
        _y = RankCorrelation(pl1y, pl2y);
}
