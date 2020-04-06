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

//! author="Igor Markov, April 8, 1998"

#ifndef _TERMIPROP_H_
#define _TERMIPROP_H_

#include <Geoms/plGeom.h>
#include <Partitioning/partitioning.h>

class TerminalPropagatorRough;
class TerminalPropagatorFine;

/* typedef TerminalPropagatorRough TerminalPropagator;*/
typedef TerminalPropagatorFine TerminalPropagator;

//:  Computes distances to all partitions
//   and propagates a given module to all partitions as close as
//   fuzzyFactor=1.00+0.01*partitionFuzziness times the minimum distance.
//
//   A terminal propagator, in general, is initialized
//   with a vector of partition bounding boxes and, on request,
//   propagates modules with given coordinates (or in a given BBox)
//  to one or more closest partitions. The "partition fuzziness"
//   parameter allows to propagate into more than one partition
//   even when one partition is closer than others. The parameter
//   values are given in per cent.
class TerminalPropagatorRough {
        const uofm::vector<BBox>& _partitions;
        uofm::vector<double> _mdists;
        double _fuzzyFactor;

       public:
        TerminalPropagatorRough(const uofm::vector<BBox>& partitions, double partFuzziness = 0) : _partitions(partitions), _mdists(partitions.size()), _fuzzyFactor(1.0 + 0.01 * partFuzziness) {}
        void doOneTerminal(const BBox& termiBox, PartitionIds& fixedConstr, PartitionIds& parts);
        // "ands" to bits already set in partitioninIds
};

//:  Blows up all partitions by
//   fuzzyFactor=1.00+0.01*partFuzziness from the start and then
//   propagates modules to the closest partitions. The effect of
//   fuzzyFactor is that, while partitions typically do not overlap
//   before being blown up, a module can now be contained in several
//   partitions afterwards.
//
//   TerminalPropagatorFine blows up each partition's bbox by
//   fuzzyFactor. Instead, TerminalPropagatorRough propagates
//   terminals to partitions that are closer than fuzzyFactor
//   times the minimal distance
//   "fine" has bigger size and start-up cost, since it has
//   to copy BBoxes and expand them. It runs slightly faster though.
//
//   Only one of the two classes can be used at a time, which is
//   evidenced by the typedef TerminalPropagator in the termiProp.h
//   This way, we can experiment with alternative implementations of
//   terminal propagators.
class TerminalPropagatorFine {
        const uofm::vector<BBox> _partitions;
        uofm::vector<BBox> _partitionsBloated;
        double _fuzzyFactor;

       public:
        TerminalPropagatorFine(const uofm::vector<BBox>& partitions, double partFuzziness = 0);
        void doOneTerminal(const BBox& termiBox, PartitionIds& fixedConstr, PartitionIds& parts);
        // "ands" to bits already set in partitioninIds
};

#endif
