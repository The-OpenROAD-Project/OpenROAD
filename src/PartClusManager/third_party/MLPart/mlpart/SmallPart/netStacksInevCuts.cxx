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

// Created by Mike Oliver on 20 mar 1999
// (earlier work by Igor Markov)

//////////////////////////////////////////////////////////////////////
//
// netStacksInevCuts.cxx: implementation of the NetStacksInevCuts class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "netStacksInevCuts.h"

using std::ostream;
using std::cout;
using std::endl;
using std::setw;
using std::min;
using uofm::vector;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NetStacksInevCuts::NetStacksInevCuts() : NetStacks() {}

NetStacksInevCuts::NetStacksInevCuts(const HGraphFixed &hg, const Partitioning &part, const vector<unsigned> &movables) : NetStacks(hg, part, movables) {}

NetStacksInevCuts::~NetStacksInevCuts() {
        delete[] _nodeTallies[0];
        delete[] _nodeTallies[1];
        delete[] _nodes;
        delete[] _firstNodes;
}

void NetStacksInevCuts::_perMovableInit(unsigned k) {
        NetStacks::_perMovableInit(k);
        _firstNodes[k] = _nodeCounter;
        _nodeTallies[0][k] = _nodeTallies[1][k] = 0;
}

void NetStacksInevCuts::_trivInit(const HGraphFixed &hg, const Partitioning &part, const vector<unsigned> &movables) {
        NetStacks::_trivInit(hg, part, movables);
        typedef nodeAndWeight *pnw;
        _firstNodes = new pnw[movables.size() + 1];
        _nodeTallies[0] = new unsigned[movables.size() + 1];
        _nodeTallies[1] = new unsigned[movables.size() + 1];
        _nodes = new nodeAndWeight[3 * hg.getNumEdges() + 1];
        _nodeCounter = _nodes;

        std::fill(_nodeTallies[0], _nodeTallies[0] + movables.size() + 1, unsigned(0));
        std::fill(_nodeTallies[1], _nodeTallies[1] + movables.size() + 1, unsigned(0));
}

void NetStacksInevCuts::_procEdge(const HGFEdge &edge, const Partitioning &part, unsigned movableIdx) {
        if (edge.getDegree() == 2) {
                bool swapped = false;
                unsigned nodeIdx1 = (*edge.nodesBegin())->getIndex();
                unsigned nodeIdx2 = (*(edge.nodesBegin() + 1))->getIndex();
                unsigned edgeWgt = 2 * unsigned(edge.getWeight() + 0.5);
                if (_mapBack[nodeIdx2] == movableIdx) {
                        std::swap(nodeIdx1, nodeIdx2);
                        swapped = true;
                }
                abkfatal(_mapBack[nodeIdx1] == movableIdx,
                         "Inconsistency between nodes, edges in"
                         "NetStacksInevCuts::_procEdge");
                //       cout << "k=" << k << " nodeIdx1 " << nodeIdx1
                //            << "  nodeIdx1 " << nodeIdx2 << endl;
                unsigned otherMovIdx = _mapBack[nodeIdx2];
                if (otherMovIdx == UINT_MAX)  // nodeIdx2 is fixed
                {
                        abkfatal(part[nodeIdx2] != inBoth && part[nodeIdx2] != inNeither, "Supposed movable has no partition");
                        unsigned &tal = _nodeTallies[(part[nodeIdx2].isInPart(0) ? 0 : 1)][movableIdx];
                        tal += edgeWgt;
                } else if (movableIdx <= otherMovIdx) {
                        //*(_nodeCounter++)=nodeIdx2;
                        nodeAndWeight &nw = *(_nodeCounter++);
                        nw.node = otherMovIdx;
                        nw.weight = edgeWgt;
                        _usedEdges++;
                }
        } else if (edge.getDegree() == 3) {
                unsigned nodeIdx1 = (*edge.nodesBegin())->getIndex();
                unsigned nodeIdx2 = (*(edge.nodesBegin() + 1))->getIndex();
                unsigned nodeIdx3 = (*(edge.nodesBegin() + 2))->getIndex();
                unsigned edgeWgt = unsigned(edge.getWeight() + 0.5);  // half the weight
                                                                      // it would get if
                                                                      // it weren't a
                                                                      // triangle
                if (_mapBack[nodeIdx2] == movableIdx)
                        std::swap(nodeIdx1, nodeIdx2);
                else if (_mapBack[nodeIdx3] == movableIdx)
                        std::swap(nodeIdx1, nodeIdx3);
                abkfatal(_mapBack[nodeIdx1] == movableIdx,
                         "Inconsistency between nodes, edges in"
                         "NetStacksInevCuts::_procEdge");
                //       cout << "k=" << k << " nodeIdx1 " << nodeIdx1
                //            << "  nodeIdx1 " << nodeIdx2 << endl;
                unsigned movIdx2 = _mapBack[nodeIdx2];
                unsigned movIdx3 = _mapBack[nodeIdx3];

                if (movIdx2 == UINT_MAX)  // nodeIdx2 is fixed
                {
                        abkfatal(part[nodeIdx2] != inBoth && part[nodeIdx2] != inNeither, "Supposed non-movable doesn't have partition");

                        if (movIdx3 == UINT_MAX) {
                                abkfatal(part[nodeIdx3] != inBoth && part[nodeIdx3] != inNeither,
                                         "Supposed non-movable doesn't have "
                                         "partition");
                                if (part[nodeIdx2] != part[nodeIdx3]) {
                                        _cuts.back() += 2 * edgeWgt;
                                        return;  // don't want this edge
                                                 // considered at all
                                }
                        }
                        unsigned &tal = _nodeTallies[(part[nodeIdx2].isInPart(0) ? 0 : 1)][movableIdx];
                        tal += edgeWgt;
                } else if (movableIdx <= movIdx2) {
                        nodeAndWeight &nw = *(_nodeCounter++);
                        nw.node = movIdx2;
                        nw.weight = edgeWgt;
                        _usedEdges++;
                }

                if (movIdx3 == UINT_MAX)  // nodeIdx3 is fixed
                {
                        abkfatal(part[nodeIdx3] != inBoth && part[nodeIdx3] != inNeither, "Supposed non-movable doesn't have partition")
                        unsigned &tal = _nodeTallies[(part[nodeIdx3].isInPart(0) ? 0 : 1)][movableIdx];
                        tal += edgeWgt;
                } else if (movableIdx <= movIdx3) {
                        nodeAndWeight &nw = *(_nodeCounter++);
                        nw.node = movIdx3;
                        nw.weight = edgeWgt;
                        _usedEdges++;
                }

        } else
                NetStacks::_procEdge(edge, part, movableIdx);
}

void NetStacksInevCuts::_finalize(const HGraphFixed &hg, const Partitioning &part, const vector<unsigned> &movables) {
        unsigned &initCut = _cuts.back();
        unsigned *n0 = _nodeTallies[0];
        unsigned *n1 = _nodeTallies[1];

        for (unsigned k = 0; k < movables.size(); k++) {
                initCut += min(n0[k], n1[k]);  // inevitable cut
        }

        _firstNodes[movables.size()] = _nodeCounter;

        NetStacks::_finalize(hg, part, movables);
        // printState(movables);
}

class CompareByNode {
       public:
        bool operator()(NetStacksInevCuts::nodeAndWeight const &n1, NetStacksInevCuts::nodeAndWeight const &n2) { return n1.node < n2.node; }
};

void NetStacksInevCuts::consolidate() {
        nodeAndWeight *newNodes = new nodeAndWeight[_usedEdges + 1];
        nodeAndWeight *ctr = newNodes;
        // CompareByNode c;
        for (unsigned k = 0; k < _numMovables; k++) {
                nodeAndWeight *start = _firstNodes[k], *end = _firstNodes[k + 1];
                _firstNodes[k] = ctr;
                std::sort(start, end, CompareByNode());
                unsigned curNode = UINT_MAX;
                for (nodeAndWeight *pt = start; pt != end; pt++) {
                        if (pt->node == curNode) {
                                (ctr - 1)->weight += pt->weight;
                                _usedEdges--;
                        } else {
                                ctr->weight = pt->weight;
                                ctr->node = curNode = pt->node;
                                ctr++;
                        }
                }
        }
        _firstNodes[_numMovables] = ctr;
        delete[] _nodes;
        _nodes = newNodes;
}

void NetStacksInevCuts::printState(const vector<unsigned> &movables) const {
        nodeAndWeight *pt;
        cout << "Cut is " << getCut() << endl;
        for (unsigned k = 0; k < _numMovables; k++) {
                cout << "\tMovable node " << k << " original index " << movables[k] << endl;
                cout << "\t\t_nodeTallies " << _nodeTallies[0][k] << " " << _nodeTallies[1][k] << endl;
                cout << "\t\tnodes/weights ";
                for (pt = _firstNodes[k]; pt != _firstNodes[k + 1]; pt++) cout << pt->node << " " << pt->weight << " ";

                cout << endl << endl;
        }
}

ostream &operator<<(ostream &os, const NetStacksInevCuts &ns) {
        os << "---- NetStacksInevCuts: " << endl << " Current cut         : " << ns._cuts.back() << endl << " Movables            : " << ns._numMovables << endl << " Total nodes         : " << ns._numNodes << endl << " Original edges      : " << ns._allEdges << endl << " Essential edges     : " << ns._usedEdges << endl << " Essential pins      : " << ns._usedPins << endl;

        os << "\n Nets on nodes: \n";

        unsigned k;
        for (k = 0; k != ns._numMovables; k++) {
                os << setw(4) << k << ":";
                for (const NetStacks::edgeAndWeight *pt = ns._firstNets[k]; pt != ns._firstNets[k + 1]; pt++) os << "( " << setw(3) << pt->edge << " " << pt->weight << " )";
                os << endl;
                for (const NetStacksInevCuts::nodeAndWeight *ptn = ns._firstNodes[k]; ptn != ns._firstNodes[k + 1]; ptn++) os << "( " << setw(3) << ptn->node << " " << ptn->weight << " )";
                os << endl;
        }

        os << "\n Net states: \n";
        for (k = 0; k != ns._allEdges; k++)
                if (ns._netStacks[k] != NULL) {
                        os << setw(4) << k << ": " << setw(3) << static_cast<int>(*ns._netStacks[k]);
#ifdef ABKDEBUG
                        os << " (offset " << setw(3) << ns._netStacks[k] - ns._netCheck[k] << ") ";
#endif
                        os << endl;
                }
        for (k = 0; k != ns._allEdges - 1; k++)
                if (ns._netStacks[k] != NULL) {
                        //           abkfatal(0,"The code needs to be fixed");
                }

        for (k = 0; k < ns._numMovables; k++) {
                os << "\tMovable node " << k << endl;
                os << "\t\t_nodeTallies " << ns._nodeTallies[0][k] << " " << ns._nodeTallies[1][k] << endl;
                os << "\t\tnodes/weights ";
                for (const NetStacksInevCuts::nodeAndWeight *pt = ns._firstNodes[k]; pt != ns._firstNodes[k + 1]; pt++) os << pt->node << " " << pt->weight << " ";

                os << endl << endl;
        }
        os << "Cut History: " << endl;
        os << "\t";
        for (k = 0; k < ns._numMovables; k++) {
                os << ns._cuts[k] << " ";
        }
        os << endl;
        return os;
}
