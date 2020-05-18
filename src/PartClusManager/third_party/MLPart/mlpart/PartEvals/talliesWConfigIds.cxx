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

#include "talliesWConfigIds.h"
#include <iostream>

using std::ostream;
using std::endl;
using std::setw;

TalliesWConfigIds::TalliesWConfigIds(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs) : TalliesWCosts(problem, part, terminalsCountAs), _fixedConfigIds(problem.getHGraph().getNumEdges()), _movblConfigIds(problem.getHGraph().getNumEdges()) { reinitializeProper(); }

TalliesWConfigIds::TalliesWConfigIds(const PartitioningProblem& problem, unsigned terminalsCountAs) : TalliesWCosts(problem, terminalsCountAs), _fixedConfigIds(problem.getHGraph().getNumEdges()), _movblConfigIds(problem.getHGraph().getNumEdges()) {}

TalliesWConfigIds::TalliesWConfigIds(const HGraphFixed& hg, const Partitioning& part, unsigned nParts, unsigned terminalsCountAs) : TalliesWCosts(hg, part, nParts, terminalsCountAs), _fixedConfigIds(hg.getNumEdges()), _movblConfigIds(hg.getNumEdges()) { reinitializeProper(); }

void TalliesWConfigIds::reinitializeProper() {
        PartitionIds emptyIds;

        std::fill(_fixedConfigIds.begin(), _fixedConfigIds.end(), emptyIds);
        std::fill(_movblConfigIds.begin(), _movblConfigIds.end(), emptyIds);

        for (itHGFNodeGlobal n = _hg.nodesBegin(); n != _hg.nodesEnd(); n++) {
                unsigned moduleIdx = (*n)->getIndex();
                if (_hg.isTerminal(moduleIdx)) {
                        for (itHGFEdgeLocal e = (*n)->edgesBegin(); e != (*n)->edgesEnd(); e++) _fixedConfigIds[(*e)->getIndex()] |= (*_part)[moduleIdx];
                } else {
                        for (itHGFEdgeLocal e = (*n)->edgesBegin(); e != (*n)->edgesEnd(); e++) _movblConfigIds[(*e)->getIndex()] |= (*_part)[moduleIdx];
                }
        }
        if (_terminalsCountAs != 0)
                for (int netIdx = _netCosts.size() - 1; netIdx >= 0; netIdx--) _movblConfigIds[netIdx] |= _fixedConfigIds[netIdx];
}

ostream& TalliesWConfigIds::prettyPrint(ostream& os) const {
        const UDenseMatrix& mat = _tallies;
        os << "  NetId    Cost       Tally (movables + [fixed ConfigId]) " << endl;
        for (unsigned i = 0; i != mat.getNumRows(); i++) {
                os << setw(6) << i << " : " << setw(6) << getNetCost(i) << " :  ";
                for (unsigned j = 0; j != mat.getNumCols(); j++) {
                        os << " " << setw(4) << mat(i, j);
                }
                os << "  +  " << _fixedConfigIds[i];
        }
        os << "   Total cost : " << getTotalCost() << endl << endl;

        return os;
}
