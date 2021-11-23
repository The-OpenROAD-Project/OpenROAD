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

// created by Igor Markov on 06/07/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Partitioning/partitioning.h"
#include "talliesWCostsWWeights2way.h"

using std::ostream;
using std::endl;
using std::setw;

void TalliesWCostsWWeights2way::setNetWeights() {
        _maxNetWeight = 0;
        itHGFEdgeGlobal edgeIt = _hg.edgesBegin();

        if (edgeIt != _hg.edgesEnd()) {
                // abkfatal3(double((*edgeIt)->getWeight())>=1.0
                //	 && double((*edgeIt)->getWeight())< UINT_MAX/10.0,
                //		 " Net 0 has weight ", (*edgeIt)->getWeight(),
                //		 ". Must be <1 or >UINT_MAX/10 \n");
        }

        for (; edgeIt != _hg.edgesEnd(); edgeIt++) {
                double weight = (*edgeIt)->getWeight();
                unsigned edgeId = (*edgeIt)->getIndex();
                //     abkfatal3(weight>=1.0 && weight < UINT_MAX/10,
                //                    " Net ",edgeId, " has weight <1 or
                // >UINT_MAX/10 \n");
                unsigned truncWeight = static_cast<unsigned>(rint(weight));
                _netWeights[edgeId] = truncWeight;
                if (truncWeight > _maxNetWeight) _maxNetWeight = truncWeight;
        }
}

TalliesWCostsWWeights2way::TalliesWCostsWWeights2way(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs) : TalliesWCosts2way(problem, part, terminalsCountAs), _netWeights(problem.getHGraph().getNumEdges()), _maxNetWeight(UINT_MAX) {
        setNetWeights();
        reinitializeProper();
}

TalliesWCostsWWeights2way::TalliesWCostsWWeights2way(const PartitioningProblem& problem, unsigned terminalsCountAs) : TalliesWCosts2way(problem, terminalsCountAs), _netWeights(problem.getHGraph().getNumEdges()), _maxNetWeight(UINT_MAX) { setNetWeights(); }

TalliesWCostsWWeights2way::TalliesWCostsWWeights2way(const HGraphFixed& hg, const Partitioning& part, unsigned terminalsCountAs) : TalliesWCosts2way(hg, part, terminalsCountAs), _netWeights(hg.getNumEdges()), _maxNetWeight(UINT_MAX) {
        setNetWeights();
        reinitializeProper();
}

ostream& TalliesWCostsWWeights2way::prettyPrint(ostream& os) const {
        os << "  NetId    Cost   Weight      Tally (terminals count as " << terminalsCountAs() << " module(s)) " << endl;
        for (unsigned i = 0; i != _hg.getNumEdges(); i++) {
                os << setw(6) << i << " : " << setw(6) << getNetCost(i) << " :  ";
                os << setw(3) << getNetWeight(i) << " :  ";
                os << " " << setw(4) << static_cast<unsigned>(_tallies[2 * i]) << " " << setw(4) << static_cast<unsigned>(_tallies[2 * i + 1]) << endl;
        }
        os << "   Total cost : " << getTotalCost() << endl << endl;

        return os;
}
