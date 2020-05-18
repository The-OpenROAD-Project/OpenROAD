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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abklimits.h>
#include <ABKCommon/uofm_alloc.h>
#include <Geoms/plGeom.h>
#include <Partitioning/partitioning.h>
#include <Partitioning/termiProp.h>
#include <algorithm>

using std::min;
using uofm::vector;

void TerminalPropagatorRough::doOneTerminal(const BBox& termiBox, PartitionIds& fixedConstr, PartitionIds& parts) {
        double minDist = DBL_MAX;
        for (unsigned i = 0; i < _partitions.size(); ++i) {
                if (fixedConstr.isEmpty() || fixedConstr.isInPart(i)) {
                        _mdists[i] = termiBox.mdistTo(_partitions[i]);
                } else {
                        _mdists[i] = DBL_MAX;
                }
                minDist = min(minDist, _mdists[i]);
        }

        minDist *= _fuzzyFactor;

        for (unsigned i = 0; i < _mdists.size(); ++i) {
                if ((fixedConstr.isEmpty() || fixedConstr.isInPart(i)) && lessOrEqualDouble(_mdists[i], minDist)) {
                        parts.addToPart(i);
                }
        }

        abkassert(parts.numberPartitions() >= 1, "all terminals must propagate");
}

TerminalPropagatorFine::TerminalPropagatorFine(const vector<BBox>& partitions, double partFuzziness) : _partitions(partitions), _partitionsBloated(partitions), _fuzzyFactor(1.0 + 0.01 * partFuzziness) {
        for (vector<BBox>::iterator bIt = _partitionsBloated.begin(); bIt != _partitionsBloated.end(); ++bIt) bIt->ShrinkTo(_fuzzyFactor);
}

void TerminalPropagatorFine::doOneTerminal(const BBox& termiBox, PartitionIds& fixedConstr, PartitionIds& parts) {
        double minDist = std::numeric_limits<double>::max();
        for (unsigned i = 0; i < _partitions.size(); ++i) {
                if (fixedConstr.isEmpty() || fixedConstr.isInPart(i)) {
                        minDist = min(minDist, termiBox.mdistTo(_partitions[i]));
                }
        }

        for (unsigned i = 0; i < _partitionsBloated.size(); ++i) {
                if ((fixedConstr.isEmpty() || fixedConstr.isInPart(i)) && lessOrEqualDouble(termiBox.mdistTo(_partitionsBloated[i]), minDist)) {
                        parts.addToPart(i);
                }
        }

        abkassert(parts.numberPartitions() >= 1, "all terminals must propagate");
}
