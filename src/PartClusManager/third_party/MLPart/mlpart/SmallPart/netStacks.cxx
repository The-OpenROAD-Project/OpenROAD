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

// Created by Igor Markov on Feb 25, 1999

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "HGraph/hgFixed.h"
#include "Partitioning/partitionData.h"
#include "netStacks.h"
#include <iostream>

using std::ostream;
using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

NetStacks::NetStacks() : _populated(false) { const_cast<PartitionIds*>(&inBoth)->setToAll(2); }

NetStacks::NetStacks(const HGraphFixed& hg, const Partitioning& part, const vector<unsigned>& movables) : _populated(false), _netData(NULL), _netStacks(NULL), _netCheck(NULL), _nets(NULL), _firstNets(NULL) {
        const_cast<PartitionIds*>(&inBoth)->setToAll(2);
        populate(hg, part, movables);
}

void NetStacks::populate(const HGraphFixed& hg, const Partitioning& part, const vector<unsigned>& movables) {
        abkfatal(!_populated, "Can't populate NetStacks twice");
        _populated = true;
        _trivInit(hg, part, movables);

        for (unsigned k = 0; k != movables.size(); k++) {
                const HGFNode& node = hg.getNodeByIdx(movables[k]);
                _perMovableInit(k);

                for (itHGFEdgeLocal eIt = node.edgesBegin(); eIt != node.edgesEnd(); eIt++) {
                        const HGFEdge& edge = (**eIt);
                        _procEdge(edge, part, k);
                }
        }
        _finalize(hg, part, movables);
}

void NetStacks::_finalize(const HGraphFixed& hg, const Partitioning& part, const vector<unsigned>& movables) {
        (void)hg;
        (void)part;                                  // unused
        _firstNets[movables.size()] = _pinCounter2;  // sentinel value
}

ostream& operator<<(ostream& os, const NetStacks& ns) {
        os << "---- Vanilla NetStacks: " << endl << " Current cut         : " << ns._cuts.back() << endl << " Movables            : " << ns._numMovables << endl << " Total nodes         : " << ns._numNodes << endl << " Original edges      : " << ns._allEdges << endl << " Essential edges     : " << ns._usedEdges << endl << " Essential pins      : " << ns._usedPins << endl;

        os << "\n Nets on nodes: \n";

        unsigned k;
        for (k = 0; k != ns._numMovables; k++) {
                os << setw(4) << k << ":";
                for (const NetStacks::edgeAndWeight* pt = ns._firstNets[k]; pt != ns._firstNets[k + 1]; pt++) os << " " << setw(3) << pt->edge;
                os << endl;
        }

        os << "\n Net states: \n";
        for (k = 0; k != ns._allEdges; k++)
                if (ns._netStacks[k] != NULL) {
                        cout << setw(4) << k << ": " << setw(3) << static_cast<int>(*ns._netStacks[k]);
#ifdef ABKDEBUG
                        cout << " (offset " << setw(3) << ns._netStacks[k] - ns._netCheck[k] << ") ";
#endif
                        cout << endl;
                }
        for (k = 0; k != ns._allEdges - 1; k++)
                if (ns._netStacks[k] != NULL) {
                        //           abkfatal(0,"The code needs to be fixed");
                }

        return os;
}

NetStacks::~NetStacks() {
        if (_firstNets) delete[] _firstNets;
        if (_nets) delete[] _nets;
        if (_netStacks) delete[] _netStacks;
        if (_netData) delete[] _netData;
#ifdef ABKDEBUG
        if (_netCheck) delete[] _netCheck;
#endif
}

void NetStacks::_trivInit(const HGraphFixed& hg, const Partitioning& part, const vector<unsigned>& movables) {
        static_cast<void>(part);

        _traversed.clear();
        _mapBack.clear();
        _traversed.insert(_traversed.end(), hg.getNumEdges(), false);
        _mapBack.insert(_mapBack.end(), hg.getNumNodes(), UINT_MAX);

        _numMovables = movables.size();
        _numNodes = hg.getNumNodes();
        _allEdges = hg.getNumEdges();
        _usedEdges = 0;
        _usedPins = 0;

        _cuts.reserve(movables.size() + 2);
        _cuts.push_back(0);
        typedef unsigned* up;
        typedef unsigned** upp;
        typedef edgeAndWeight* pew;
        _netData = new unsigned[hg.getNumPins() + hg.getNumEdges() + 1];
        _netStacks = new up[hg.getNumEdges() + 1];
#ifdef ABKDEBUG
        _netCheck = new up[hg.getNumEdges() + 1];
#endif
        _nets = new edgeAndWeight[hg.getNumPins() + 1];
        _firstNets = new pew[movables.size() + 1];

        std::fill(_netStacks, _netStacks + hg.getNumEdges() + 1, static_cast<up>(NULL));

#ifdef ABKDEBUG
        std::fill(_netCheck, _netCheck + hg.getNumEdges() + 1, static_cast<up>(NULL));
#endif
        _pinCounter1 = _netData;
        _pinCounter2 = _nets;

        for (unsigned k = 0; k != movables.size(); k++) {
                _mapBack[movables[k]] = k;
        }
        _netIds = Partitioning(hg.getNumEdges());
}

void NetStacks::_procEdge(const HGFEdge& edge, const Partitioning& part, unsigned movableIdx) {
        (void)movableIdx;  // unused
        unsigned edgeIdx = edge.getIndex();
        unsigned edgeWgt = 2 * unsigned(edge.getWeight() + 0.5);
        if (!_traversed[edgeIdx]) {
                _traversed[edgeIdx] = true;
                for (itHGFNodeLocal nIt = edge.nodesBegin(); nIt != edge.nodesEnd(); nIt++) {
                        unsigned nodeIdx = (*nIt)->getIndex();
                        if (_mapBack[nodeIdx] == UINT_MAX)
                                if (part[nodeIdx] != inBoth) _netIds[edgeIdx] |= part[nodeIdx];
                }

                if (_netIds[edgeIdx] == inBoth)
                        _cuts.back() += edgeWgt;
                else {
                        _usedEdges++;
                        _netStacks[edgeIdx] = _pinCounter1;
#ifdef ABKDEBUG
                        _netCheck[edgeIdx] = _pinCounter1;
#endif
                        if (_netIds[edgeIdx] == inNeither)
                                *_pinCounter1 = 0;
                        else
                                *_pinCounter1 = (_netIds[edgeIdx].isInPart(0) ? 1 : 2);
                        _pinCounter1 += (edge.getDegree() + 1);
                        _usedPins += edge.getDegree();
                }
        }

        if (_netIds[edgeIdx] != inBoth)  // not cut
        {
                _pinCounter2->edge = edgeIdx;
                _pinCounter2->weight = edgeWgt;
                _pinCounter2++;
        }
}

void NetStacks::_perMovableInit(unsigned k) { _firstNets[k] = _pinCounter2; }
