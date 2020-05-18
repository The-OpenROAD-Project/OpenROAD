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

// created by Andrew Caldwell on 02/09/98
// major portions from LIFO-FM code by S.Dutt W.Deng
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "fmNetCut2WayMM.h"
#include "PartEvals/univPartEval.h"
#include "Partitioners/multiStartPart.h"

using std::min;
using uofm::vector;

FMNetCut2WayMoveManager::FMNetCut2WayMoveManager(const PartitioningProblem& problem, const PartitionerParams& params, bool allowCorkingNodes) : MoveManagerInterface(problem), _hgraph(problem.getHGraph()), _edgeWeights(_hgraph.getNumEdges()), _partAreas(2, 0.0), _maxGain(problem.getHGraph()), _gainCo(problem.getHGraph().getNumNodes(), 2, params, (_maxGain.getAvgMaxGain() > 500.0)), _lockedNode(_hgraph.getNumNodes()), _moveLog(), _allowCorkingNodes(allowCorkingNodes), allowIllegalMoves(false) {
        _unlocked[0] = new unsigned short[_hgraph.getNumEdges()];
        _unlocked[1] = new unsigned short[_hgraph.getNumEdges()];
        _locked[0] = new unsigned short[_hgraph.getNumEdges()];
        _locked[1] = new unsigned short[_hgraph.getNumEdges()];

        const vector<vector<double> >& maxes = problem.getMaxCapacities();
        const vector<vector<double> >& mins = problem.getMinCapacities();

        _maxAreas[0] = maxes[0][0];
        _maxAreas[1] = maxes[1][0];
        _minAreas[0] = mins[0][0];
        _minAreas[1] = mins[1][0];

        // unsigned m=0;
        for (unsigned e = 0; e < _hgraph.getNumEdges(); e++) {
                _edgeWeights[e] = static_cast<unsigned>(rint(_hgraph.getEdgeByIdx(e).getWeight()));
                //    if(_edgeWeights[e]>m)
                //        m=_edgeWeights[e];
        }

        _gainCo.setMaxGain(_maxGain.getMaxGain());
}

FMNetCut2WayMoveManager::~FMNetCut2WayMoveManager() {
        delete[] _unlocked[0];
        delete[] _unlocked[1];
        delete[] _locked[0];
        delete[] _locked[1];
}

// reinitialize should reset everything, including
//-net tallies
//-total costs
//-gainCo
//-actual areas of partitions
//-moveLog

void FMNetCut2WayMoveManager::reinitialize() {
        _gainCo.reinitialize();  // removes nodes that were left(if any)
        std::fill(_lockedNode.begin(), _lockedNode.end(), false);

        const Partitioning& curPart = *_curPart;
        _partAreas[0] = _partAreas[1] = 0;

        unsigned to, from;

        vector<HGWeight> nodeWeights(_hgraph.getNumNodes());

        for (unsigned n = 0; n < _hgraph.getNumNodes(); ++n) {
                to = curPart[n].isInPart(0) ? 1 : 0;
                from = 1 - to;

                _gainCo.getElement(n, to)._gainOffset = 0;
                _gainCo.getElement(n, to)._gain = 0;

                nodeWeights[n] = _hgraph.getWeight(n);
                _partAreas[from] += nodeWeights[n];
        }

        _partSol->partArea[0][0] = _partAreas[0];
        _partSol->partArea[1][0] = _partAreas[1];
        _curCost = 0;
        double tol = min(_maxAreas[0] - _minAreas[0], _maxAreas[1] - _minAreas[1]);

        memset(_locked[0], 0, sizeof(short) * _hgraph.getNumEdges());
        memset(_locked[1], 0, sizeof(short) * _hgraph.getNumEdges());
        memset(_unlocked[0], 0, sizeof(short) * _hgraph.getNumEdges());
        memset(_unlocked[1], 0, sizeof(short) * _hgraph.getNumEdges());

        for (unsigned e = 0; e < _hgraph.getNumEdges(); ++e) {
                const itHGFNodeLocal endNode = _hgraph.getEdgeByIdx(e).nodesEnd();
                for (itHGFNodeLocal nd = _hgraph.getEdgeByIdx(e).nodesBegin(); nd != endNode; ++nd) {
                        unsigned idx = (*nd)->getIndex();
                        from = curPart[idx].isInPart(0) ? 0 : 1;

                        if (!((*_fixedConstr)[idx][to]) || nodeWeights[idx] > tol) {
                                ++_locked[from][e];
                        } else {
                                ++_unlocked[from][e];
                        }
                }
                if ((_unlocked[0][e] || _locked[0][e]) && (_unlocked[1][e] || _locked[1][e])) {
                        _curCost += _edgeWeights[e];
                }
        }
        _moveLog.clear();
}

void FMNetCut2WayMoveManager::reinitTolerances() {
        const vector<vector<double> >& maxes = _problem.getMaxCapacities();
        const vector<vector<double> >& mins = _problem.getMinCapacities();
        _maxAreas[0] = maxes[0][0];
        _maxAreas[1] = maxes[1][0];
        _minAreas[0] = mins[0][0];
        _minAreas[1] = mins[1][0];
}

void FMNetCut2WayMoveManager::computeGains() {
        unsigned to, from;
        Partitioning& curPart = *_curPart;
        itHGFEdgeLocal e;

        double tol = min(_maxAreas[0] - _minAreas[0], _maxAreas[1] - _minAreas[1]);

        for (unsigned c = 0; c < _hgraph.getNumNodes(); c++) {
                from = curPart[c].isInPart(0) ? 0 : 1;
                to = 1 - from;

                if ((!(*_fixedConstr)[c][to]) ||  // is fixed, or is too large..
                    (!_allowCorkingNodes && _hgraph.getWeight(c) >= tol)) {
                        _lockedNode[c] = true;
                        continue;
                }

                SVGainElement& gElmt = _gainCo.getElement(c, to);

                for (e = _hgraph.getNodeByIdx(c).edgesBegin(); e != _hgraph.getNodeByIdx(c).edgesEnd(); e++) {
                        unsigned eId = (*e)->getIndex();
                        if (_unlocked[from][eId] + _locked[from][eId] == 1)
                                gElmt._gain += _edgeWeights[eId];
                        else if (_unlocked[to][eId] + _locked[to][eId] == 0)
                                gElmt._gain -= _edgeWeights[eId];
                }

                _gainCo.addElement(gElmt, to, gElmt._gain);
        }

        _gainCo.resetAfterGainUpdate();
}

void FMNetCut2WayMoveManager::resetTo(PartitioningSolution& newSol) {
        _partSol = &newSol;
        _curPart = &(newSol.part);
        _moveLog.resetTo(newSol.part);

        reinitialize();
}

bool FMNetCut2WayMoveManager::pickMoveApplyIt() {
        // cout<<_gainCo<<endl;

        bool foundMove = false;
        SVGainElement* toMove;
        unsigned id, from, to;
        double nodeArea;

        while (!foundMove) {
                toMove = _gainCo.getHighestGainElement();
                if (toMove == NULL)  // none left
                        return false;

                id = _gainCo.getElementId(*toMove);
                from = (*_curPart)[id].isInPart(0) ? 0 : 1;
                to = 1 - from;
                nodeArea = _hgraph.getWeight(id);

                // the MM is allowed to select a move which makes the solution
                // illegal, only if the current solution is legal

                abkassert(from <= 1 && to <= 1, "from or to is out of range");

                if ((allowIllegalMoves && (_partAreas[from] <= _maxAreas[from]) && (_partAreas[to] <= _maxAreas[to])) || (_partAreas[from] - nodeArea > _minAreas[from] && _partAreas[to] + nodeArea < _maxAreas[to]))

                        foundMove = true;
                else
                        _gainCo.invalidateBucket(to);
        }

        if (toMove == NULL) return false;

        // cout<<"Found a Move:"<<id<<"("<<nodeArea<<") "<<*toMove;
        // cout<<"  From "<<from<<" To "<<to<<endl;

        _partAreas[from] -= nodeArea;
        _partAreas[to] += nodeArea;
        // normally, the PartLeg checker keeps track of this...
        _partSol->partArea[0][0] = _partAreas[0];
        _partSol->partArea[1][0] = _partAreas[1];
        _lockedNode[id] = true;
        _gainCo.removeElement(*toMove);
        _curCost -= toMove->getRealGain();

        _moveLog.logMove(id, (*_curPart)[id]);
        (*_curPart)[id].setToPart(to);

        // cout<<"  New Cost: "<<_curCost<<"  New Areas: "<<_partAreas<<endl;

        // sets netTallies (#unlocked,etc) & updates gains,
        updateGains(id, from, to);

        _gainCo.resetAfterGainUpdate();
        return true;
}

void FMNetCut2WayMoveManager::updateGains(unsigned id, unsigned from, unsigned to) {
        itHGFEdgeLocal edg;
        for (edg = _hgraph.getNodeByIdx(id).edgesBegin(); edg != _hgraph.getNodeByIdx(id).edgesEnd(); edg++) {
                HGFEdge& curEdge = **edg;
                unsigned edgId = curEdge.getIndex();
                unsigned eWt = _edgeWeights[edgId];

                if (curEdge.getDegree() == 2) {
                        itHGFNodeLocal node = curEdge.nodesBegin();
                        unsigned nodeIdx;
                        if ((*node)->getIndex() == id)
                                nodeIdx = (*++node)->getIndex();
                        else
                                nodeIdx = (*node)->getIndex();
                        if (!_lockedNode[nodeIdx]) {
                                unsigned curPart = (*_curPart)[nodeIdx].isInPart(0) ? 0 : 1;
                                if (curPart == from)
                                        _gainCo.updateGain(nodeIdx, 1 - curPart, 2 * eWt);
                                else
                                        _gainCo.updateGain(nodeIdx, from, -2 * eWt);
                        }
                } else {
                        if (_locked[to][edgId] == 0) {
                                if (_unlocked[to][edgId] == 0)      // net was prev uncut.
                                        fixAllGains(curEdge, eWt);  // inc. gain
                                                                    // for all
                                                                    // unlocked
                                                                    // cells
                                else if (_unlocked[to][edgId] == 1)
                                        fixOneGain(curEdge, -1 * eWt, to);  // dec. gain for the 1
                                                                            // unlocked
                                                                            // cell in the 'to' partition
                        }

                        _unlocked[from][edgId]--;
                        _locked[to][edgId]++;

                        if (_locked[from][edgId] == 0)  // nothing locked on the 'from' side
                        {
                                if (_unlocked[from][edgId] == 0)         // nothing at all on the 'from' side
                                        fixAllGains(curEdge, -1 * eWt);  // dec. gain for
                                                                         // everything
                                                                         // movable

                                else if (_unlocked[from][edgId] == 1)    // inc. gain for the one
                                        fixOneGain(curEdge, eWt, from);  // movable thing on
                                                                         // the from side
                        }
                }
        }
}

void FMNetCut2WayMoveManager::fixAllGains(HGFEdge& edge, int delta) {
        for (itHGFNodeLocal node = edge.nodesBegin(); node != edge.nodesEnd(); ++node) {
                unsigned nId = (*node)->getIndex();
                if (!_lockedNode[nId]) {
                        if ((*_curPart)[nId].isInPart(0))
                                _gainCo.updateGain(nId, 1, delta);
                        else
                                _gainCo.updateGain(nId, 0, delta);
                }
        }
}

void FMNetCut2WayMoveManager::fixOneGain(HGFEdge& edge, int delta, unsigned side) {
        for (itHGFNodeLocal node = edge.nodesBegin(); node != edge.nodesEnd(); ++node) {
                unsigned nId = (*node)->getIndex();
                if (!_lockedNode[nId] && (*_curPart)[nId].isInPart(side)) {
                        _gainCo.updateGain(nId, 1 - side, delta);
                        break;
                }
        }
}
