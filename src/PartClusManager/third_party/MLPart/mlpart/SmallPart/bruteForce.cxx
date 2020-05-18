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

// Created by Mike Oliver on 31 mar 1999

// The functions in this source file are to be used if the
// partitioner is not finding the optimal solution.  They
// find an optimal solution by brute force, and allow
// you to set a breakpoint in _runBB() which stops whenever
// the current stack is an initial segment of a chosen
// optimal solution.  To use them, define BRUTEFORCECHECK
// in bbPart.cxx .

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <algorithm>
#include "bbPart.h"
#include "Partitioning/partProb.h"
#include "HGraph/hgFixed.h"
#include "PartEvals/partEvals.h"
#include "PartLegality/1balanceGen.h"
#include "netStacks.h"

void BBPart::_bruteForcePart(PartitioningProblem &problem, unsigned cut) {
        abkfatal(_movables.size() <= 32,
                 "Can't use brute-force partitioning"
                 " for more than 32 movables");
        _bestSeen = cut;
        unsigned i, k;
        unsigned numSolns = 0;
        unsigned numCombs = 1 << _movables.size();
        for (i = 0; i < numCombs; i++) {
                double areaLeft[2] = {_partMax[0], _partMax[1]};
                for (k = 0; k < _movables.size(); k++) {
                        unsigned part = (i & 1 << k) >> k;
                        abkfatal(part == 0 || part == 1, "ZZ");
                        unsigned nodeIdx = _movables[k];
                        double wt = _hgraph.getWeight(nodeIdx);
                        areaLeft[part] -= wt;
                        if (areaLeft[part] <= _bestMaxViol) break;
                        _part[nodeIdx].setToPart(part);
                }
                if (k == _movables.size()) {
                        NetCutWNetVec cutEval(problem, _part);
                        unsigned cost;
                        if ((cost = cutEval.getTotalCost()) <= _bestSeen) {
                                _bestSeen = cost;
                                numSolns++;         // breakpoint here
                                if (numSolns == 1)  // save one solution
                                {
                                        _savedSoln = i;
                                }
                        }
                }
        }
}

bool BBPart::_isInitSeg() {
        for (unsigned k = 0; k < _assignmentStack.size(); k++) {
                if (_assignmentStack[k] != ((_savedSoln & 1 << k) >> k)) return false;
        }
        return true;
}
