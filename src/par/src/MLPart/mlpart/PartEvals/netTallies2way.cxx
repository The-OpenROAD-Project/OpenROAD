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
#include "HGraph/hgFixed.h"
#include "Ctainers/umatrix.h"
#include "netTallies2way.h"

using std::ostream;
using std::endl;
using std::setw;

NetTallies2way::NetTallies2way(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs)
    : PartEvalXFace(problem, part),
      _terminalsCountAs(terminalsCountAs),
      _tallies(NULL)
      // the order of arguments to _tallies() is important
{
        unsigned numShort = 2 * problem.getHGraph().getNumEdges();
        _tallies = new unsigned short[numShort];
        reinitializeProper();
}

NetTallies2way::NetTallies2way(const PartitioningProblem& problem, unsigned terminalsCountAs)
    : PartEvalXFace(problem),
      _terminalsCountAs(terminalsCountAs),
      _tallies(NULL)
      // the order of arguments to _tallies() is important
{
        unsigned numShort = 2 * problem.getHGraph().getNumEdges();
        _tallies = new unsigned short[numShort];
}

NetTallies2way::NetTallies2way(const HGraphFixed& hg, const Partitioning& part, unsigned terminalsCountAs)
    : PartEvalXFace(hg, part),
      _terminalsCountAs(terminalsCountAs),
      _tallies(NULL)
      // the order of arguments to _tallies() is important
{
        unsigned numShort = 2 * _hg.getNumEdges();
        _tallies = new unsigned short[numShort];
        reinitializeProper();
}

void NetTallies2way::reinitializeProper() {
        PartEvalXFace::reinitialize();

        std::fill(_tallies, _tallies + 2 * _hg.getNumEdges(), 0);

        itHGFNodeGlobal n = _hg.nodesBegin();
        for (; n != _hg.nodesEnd(); n++) {
                unsigned nodeId = (*n)->getIndex();
                PartitionIds partIds = (*_part)[nodeId];

                itHGFEdgeLocal e = (*n)->edgesBegin();
                if (partIds.isInPart(0)) {
                        if (_hg.isTerminal(nodeId))
                                for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex()] += _terminalsCountAs;
                        else
                                for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex()]++;
                } else if (partIds.isInPart(1)) {
                        if (_hg.isTerminal(nodeId))
                                for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex() + 1] += _terminalsCountAs;
                        else
                                for (; e != (*n)->edgesEnd(); e++) _tallies[2 * (*e)->getIndex() + 1]++;
                } else
                        abkfatal(0, "module is not in any partition");
        }
}

ostream& NetTallies2way::prettyPrint(ostream& os) const {
        os << " Net tallies :" << endl;
        for (unsigned i = 0; i != _hg.getNumEdges(); i++) {
                os << setw(6) << i << " : ";
                os << " " << setw(4) << static_cast<unsigned>(_tallies[2 * i]) << " " << setw(4) << static_cast<unsigned>(_tallies[2 * i + 1]) << endl;
        }
        os << endl;
        return os;
}
