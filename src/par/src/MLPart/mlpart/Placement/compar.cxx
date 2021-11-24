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

// Created: July 5, 1997 by Igor Markov  VLSICAD ABKGroup UCLA/CS
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "compar.h"

double MaxMDiff::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double dst, max = 0.0;
        for (unsigned i = 0; i < pl1.getSize(); i++) {
                dst = mDist(pl1[i], pl2[i]);
                if (dst > max) max = dst;
        }
        return max;
}

double AvgMDiff::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double sum = 0.0;
        for (unsigned i = 0; i < pl1.getSize(); i++) {
                sum += mDist(pl1[i], pl2[i]);
        }
        return sum / pl1.getSize();
}

double AvgElongation::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double sum = 0.0;
        RandomUnsigned randIdx(0, pl1.getSize());
        for (unsigned i = 0; i < 5 * pl1.getSize(); i++) {
                unsigned idxA = randIdx;
                unsigned idxB = randIdx;
                double dist1 = mDist(pl1[idxA], pl1[idxB]);
                double dist2 = mDist(pl2[idxA], pl2[idxB]);
                sum += fabs(dist2 - dist1);
        }
        return sum / (5 * pl1.getSize());
}

double MaxElongation::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double max = 0.0;
        RandomUnsigned randIdx(0, pl1.getSize());
        for (unsigned i = 0; i < 5 * pl1.getSize(); i++) {
                unsigned idxA = randIdx;
                unsigned idxB = randIdx;
                double dist1 = mDist(pl1[idxA], pl1[idxB]);
                double dist2 = mDist(pl2[idxA], pl2[idxB]);
                if (max < fabs(dist2 - dist1)) max = fabs(dist2 - dist1);
        }
        return max;
}

double AvgStretch::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double sum = 0.0;
        RandomUnsigned randIdx(0, pl1.getSize());
        for (unsigned i = 0; i < 5 * pl1.getSize(); i++) {
                unsigned idxA = randIdx;
                unsigned idxB = randIdx;
                double dist1 = mDist(pl1[idxA], pl1[idxB]);
                double dist2 = mDist(pl2[idxA], pl2[idxB]);
                if (dist1 > 1e-15 && dist2 > 1e-15) {
                        double rat = dist2 / dist1;
                        if (rat > 1)
                                sum += rat;
                        else
                                sum += 1 / rat;
                }
        }
        return sum / (5 * pl1.getSize());
}

double MaxStretch::operator()(Placement& pl1, Placement& pl2) {
        abkfatal(pl1.getSize() == pl2.getSize(), "Can\'t compare placements of different sizes");
        double max = 0.0;
        RandomUnsigned randIdx(0, pl1.getSize());
        for (unsigned i = 0; i < 5 * pl1.getSize(); i++) {
                unsigned idxA = randIdx;
                unsigned idxB = randIdx;
                double dist1 = mDist(pl1[idxA], pl1[idxB]);
                double dist2 = mDist(pl2[idxA], pl2[idxB]);
                if (dist1 > 1e-15 && dist2 > 1e-15) {
                        double rat = dist2 / dist1;
                        if (rat > 1) rat = 1 / rat;
                        if (max < rat) max = rat;
                }
        }
        return max;
}
