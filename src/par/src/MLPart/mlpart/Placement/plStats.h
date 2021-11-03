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

//! author=" Igor Markov, Sept 29, 1997"

#ifndef _PLSTATS_H_
#define _PLSTATS_H_

class Placement;
class PlCorrelation;
class PlRankCorrelation;

typedef PlCorrelation PlCorr;
typedef PlRankCorrelation PlRankCorr;

/*------------------------  INTERFACES START HERE  -------------------*/

//: Calculates the correlation between pl1 and pl2
//  that is, the correlation between array of x coord and y coord.
class PlCorrelation {
        double _x, _y;

       public:
        PlCorrelation(const Placement& pl1, const Placement& pl2);
        operator double() const { return 0.5 * (_x + _y); }
        double getX() const { return _x; }
        double getY() const { return _y; }
};

//: Computes two rank correlations: (i) between X coordinates of two
//  placements, and (ii) between their Y coordinates. The average and
//  the raw values are available.
class PlRankCorrelation {
        double _x, _y;

       public:
        PlRankCorrelation(const Placement& pl1, const Placement& pl2);
        operator double() const { return 0.5 * (_x + _y); }
        double getX() const { return _x; }
        double getY() const { return _y; }
};

#endif
