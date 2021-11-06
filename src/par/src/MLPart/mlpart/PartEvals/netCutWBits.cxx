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

// created by Andrew Caldwell and Igor Markov on 05/17/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "netCutWBits.h"

using std::ostream;
using std::cout;
using std::endl;
using std::setw;

NetCutWBits::NetCutWBits(const PartitioningProblem& problem, const Partitioning& part) : PartEvalXFace(problem, part), _totalCost(UINT_MAX), _netCosts(problem.getHGraph().getNumEdges(), UINT_MAX) { reinitializeProper(); }

NetCutWBits::NetCutWBits(const PartitioningProblem& problem) : PartEvalXFace(problem), _totalCost(UINT_MAX), _netCosts(problem.getHGraph().getNumEdges(), UINT_MAX) {}

NetCutWBits::NetCutWBits(const HGraphFixed& hg, const Partitioning& part) : PartEvalXFace(hg, part), _totalCost(UINT_MAX), _netCosts(hg.getNumEdges(), UINT_MAX) { reinitializeProper(); }

void NetCutWBits::reinitializeProper() {
        _totalCost = 0;

        itHGFEdgeGlobal e = _hg.edgesBegin();
        unsigned netId = 0;
        for (; e != _hg.edgesEnd(); e++, netId++) {
                unsigned tmp = computeNetCost(**e);
                _netCosts[netId] = tmp;
                _totalCost += tmp;
        }
}

ostream& operator<<(ostream& os, const NetCutWBits& eval) {
        os << " Total Cost is " << eval.getTotalCost() << endl << "  Net Costs are " << endl;
        for (unsigned k = 0; k != eval._netCosts.size(); k++) cout << setw(6) << k << " : " << setw(6) << eval.getNetCost(k) << endl;
        return os;
}
