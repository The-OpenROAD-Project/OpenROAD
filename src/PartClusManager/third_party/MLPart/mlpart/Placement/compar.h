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

//! author="Igor Markov July 1, 1997"

#ifndef _COMPARAT_H_
#define _COMPARAT_H_

#include "placement.h"

class PlComparator;

/*------------------------  INTERFACES START HERE  ------------------- */

//: Defines generic interface for comparing two placements to each other.
//  Two placements to be comparated must be same size
class PlComparator {
       public:
        virtual double operator()(Placement& pl1, Placement& pl2) = 0;
        virtual ~PlComparator() {}
};

//: Calculates the maximum distance among the distances of each
//	pair of points from pl1 and pl2
class MaxMDiff : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

//: Calculates the average distance for two placement among the
//  distances of each pair of points from pl1 and pl2
class AvgMDiff : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

//: Computes the average value of dist2/dist1
class AvgStretch : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

//: Computes the max value of dist2/dist1
class MaxStretch : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

//:A placement comparator that computes the average
// distance between respective points.
class AvgElongation : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

//: A placement comparator that computes the maximum
// distance between respective points.
class MaxElongation : public PlComparator {
       public:
        double operator()(Placement& pl1, Placement& pl2);
};

#endif
