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

// created by Igor Markov on 05/17/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Partitioning/partitioning.h"
#include "talliesWCosts.h"

using std::ostream;
using std::endl;
using std::setw;

TalliesWCosts::TalliesWCosts(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs) : NetTallies(problem, part, terminalsCountAs), _totalCost(UINT_MAX), _netCosts(problem.getHGraph().getNumEdges()) { reinitializeProper(); }

TalliesWCosts::TalliesWCosts(const PartitioningProblem& problem, unsigned terminalsCountAs) : NetTallies(problem, terminalsCountAs), _totalCost(UINT_MAX), _netCosts(problem.getHGraph().getNumEdges()) {}

TalliesWCosts::TalliesWCosts(const HGraphFixed& hg, const Partitioning& part, unsigned nParts, unsigned terminalsCountAs) : NetTallies(hg, part, nParts, terminalsCountAs), _totalCost(UINT_MAX), _netCosts(hg.getNumEdges()) { reinitializeProper(); }

void TalliesWCosts::updateAllCosts()
    // derived classes need to be calli it in the end of reinitializeProper()
{
        for (itHGFEdgeGlobal e = _hg.edgesBegin(); e != _hg.edgesEnd(); e++) {
                unsigned netIdx = (*e)->getIndex();
                _netCosts[netIdx] = computeCostOfOneNet(netIdx);
        }
        _totalCost = 0;
        for (int x = _netCosts.size() - 1; x >= 0; --x) {
                _totalCost += _netCosts[x];
        }
}

ostream& TalliesWCosts::prettyPrint(ostream& os) const {
        const UDenseMatrix& mat = _tallies;
        os << "  NetId    Cost       Tally (terminals count as " << terminalsCountAs() << " module(s)) " << endl;
        for (unsigned i = 0; i != mat.getNumRows(); i++) {
                os << setw(6) << i << " : " << setw(6) << getNetCost(i) << " :  ";
                for (unsigned j = 0; j != mat.getNumCols(); j++) {
                        os << " " << setw(4) << mat(i, j);
                }
                os << endl;
        }
        os << "   Total cost : " << getTotalCost() << endl << endl;

        return os;
}
