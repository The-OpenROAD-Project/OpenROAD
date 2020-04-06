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
#include "HGraph/hgFixed.h"
#include "Ctainers/umatrix.h"
#include "netTallies.h"

using std::ostream;
using std::endl;
using std::setw;

NetTallies::NetTallies(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs)
    : PartEvalXFace(problem, part),
      _terminalsCountAs(terminalsCountAs),
      _nParts(problem.getNumPartitions()),
      _tallies(problem.getHGraph().getNumEdges(), problem.getNumPartitions())
      // the order of arguments to _tallies() is important
{
        reinitializeProper();
}

NetTallies::NetTallies(const PartitioningProblem& problem, unsigned terminalsCountAs)
    : PartEvalXFace(problem),
      _terminalsCountAs(terminalsCountAs),
      _nParts(problem.getNumPartitions()),
      _tallies(problem.getHGraph().getNumEdges(), problem.getNumPartitions())
      // the order of arguments to _tallies() is important
{}

NetTallies::NetTallies(const HGraphFixed& hg, const Partitioning& part, unsigned nParts, unsigned terminalsCountAs)
    : PartEvalXFace(hg, part),
      _terminalsCountAs(terminalsCountAs),
      _nParts(nParts),
      _tallies(hg.getNumEdges(), nParts)
      // the order of arguments to _tallies() is important
{
        reinitializeProper();
}

void NetTallies::reinitializeProper() {
        PartEvalXFace::reinitialize();

        _tallies.setToZero();

        itHGFNodeGlobal n = _hg.nodesBegin();
        for (; n != _hg.nodesEnd(); n++) {
                unsigned nodeId = (*n)->getIndex();
                PartitionIds partIds = (*_part)[nodeId];
                unsigned partitionNum = _nParts;
                for (unsigned k = 0; k != _nParts; k++)
                        if (partIds.isInPart(k)) {
                                partitionNum = k;
                                break;
                        }
                abkfatal(partitionNum < _nParts, "module is not in any partition");

                itHGFEdgeLocal e = (*n)->edgesBegin();
                for (; e != (*n)->edgesEnd(); e++) {
                        if (_hg.isTerminal(nodeId))
                                _tallies((*e)->getIndex(), partitionNum) += _terminalsCountAs;
                        else
                                _tallies((*e)->getIndex(), partitionNum)++;
                }
        }
}

ostream& NetTallies::prettyPrint(ostream& os) const {
        const UDenseMatrix& mat = _tallies;
        os << " Net tallies :" << endl;
        for (unsigned i = 0; i != mat.getNumRows(); i++) {
                os << setw(6) << i << " : ";
                for (unsigned j = 0; j != mat.getNumCols(); j++) {
                        os << " " << setw(4) << mat(i, j);
                }
                os << endl;
        }
        os << endl;
        return os;
}
