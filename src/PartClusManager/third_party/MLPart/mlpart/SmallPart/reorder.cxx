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

// Created by Mike Oliver on 28 april 1999

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "bbPart.h"

using std::cout;
using std::endl;
using std::min;
using std::max;
using uofm::vector;

#define WEIGHT_TERM unsigned(5 * (wt - _minNodeSize) / (_maxNodeSize - _minNodeSize)) * bbpart_nodeSizeWeight;

class CompareMovablesByReverseDegree {
        HGraphFixed const &_hg;
        vector<unsigned> const &_movables;

       public:
        CompareMovablesByReverseDegree(HGraphFixed const &hg, vector<unsigned> const &mov) : _hg(hg), _movables(mov) {}
        bool operator()(unsigned i, unsigned j) { return _hg.getNodeByIdx(_movables[i]).getDegree() > _hg.getNodeByIdx(_movables[j]).getDegree(); }
};
class ComparatorForOddSort {
        HGraphFixed const &_hg;
        vector<unsigned> const &_movables;
        vector<unsigned> const &_degs;

       public:
        ComparatorForOddSort(HGraphFixed const &hg, vector<unsigned> const &mov, vector<unsigned> const &degs) : _hg(hg), _movables(mov), _degs(degs) {}
        bool operator()(unsigned i, unsigned j) const;
};

bool ComparatorForOddSort::operator()(unsigned i, unsigned j) const {
        HGFNode const &node1 = _hg.getNodeByIdx(_movables[i]);
        HGFNode const &node2 = _hg.getNodeByIdx(_movables[j]);
        if (_degs[i] > _degs[j])
                return true;
        else {
                double a1 = _hg.getWeight(node1.getIndex());
                double a2 = _hg.getWeight(node2.getIndex());
                if (a1 < a2)
                        return true;
                else if (a1 > a2)
                        return false;
                else
                        return (node1.getIndex() < node2.getIndex());
        }
        abkfatal(0, "internal error");
        return false;
}

void BBPart::_reorderMovables() {
        Timer tm;
        ReorderMovables(_movables, _hgraph, _params.verb);
        _setWeights();  // has to be redone because we've changed the order
        // of the vertices.  LOOK!  can we eliminate some
        // redundancy here?
        tm.stop();
        if (_params.verb.getForMajStats() || _params.verb.getForSysRes()) cout << "Reordering took " << tm;
}

unsigned BBPart::ReorderMovables::_totalEdgeWeight(HGFNode const &node) {
        double runVal = 0.0;
        itHGFEdgeLocal iE = node.edgesBegin();
        for (; iE != node.edgesEnd(); iE++) runVal += (*iE)->getWeight();
        return unsigned(runVal + 0.5);
}

void BBPart::ReorderMovables::_numberHairyNodes() {
        CompareMovablesByReverseDegree c(_noTriGraph, _movables);
        vector<unsigned> temp(_movables.size());

        for (unsigned z = 0; z < temp.size(); ++z) {
                temp[z] = z;
        }

        vector<unsigned>::iterator iNTH = temp.begin() + unsigned(temp.size() / 10);
        std::nth_element(temp.begin(), iNTH, temp.end(), c);
        vector<unsigned>::iterator iT = temp.begin();
        unsigned percentileDegree = max(_noTriGraph.getNodeByIdx(_movables[*iNTH]).getDegree(), unsigned(2));
        for (; iT <= iNTH; iT++) {
                unsigned idx = *iT;
                unsigned degree;
                if (!_isNumbered[idx] && (degree = _noTriGraph.getNodeByIdx(_movables[idx]).getDegree()) > percentileDegree) {
                        _isNumbered[idx] = true;
                        _priorities[idx] = UINT_MAX;
                        _numberedNodes.push_back(idx);
                        // _prior.erase(idx); //uncomment if prioritizer
                        // populated
                }
        }
}

void BBPart::ReorderMovables::_numberOddNodes() {
        BitBoard nodeVis(_noTriGraph.getNumNodes());
        unsigned oldSize = _numberedNodes.size();
        vector<unsigned> twoPinEdgeDegrees(_movables.size(), 0);

        for (unsigned k = 0; k < _movables.size(); k++) {
                nodeVis.clear();
                HGFNode const &node = _noTriGraph.getNodeByIdx(_movables[k]);
                unsigned twoPinEdgeCount = 0;
                unsigned total2PinEdgeWt = 0;
                itHGFEdgeLocal iE = node.edgesBegin();
                for (; iE != node.edgesEnd(); iE++) {
                        HGFEdge const &edge = *(*iE);
                        if (edge.getDegree() == 2) {
                                itHGFNodeLocal iN = edge.nodesBegin();
                                if ((*iN)->getIndex() == node.getIndex()) iN++;
                                HGFNode const &nbrNode = *(*iN);
                                unsigned nbrIdx = nbrNode.getIndex();
                                if (!nodeVis.isBitSet(nbrIdx)) {
                                        nodeVis.setBit(nbrIdx);
                                        total2PinEdgeWt += unsigned(edge.getWeight() + 0.5);
                                        ++twoPinEdgeCount;
                                }
                        }
                }
                if ((twoPinEdgeCount == 1 || total2PinEdgeWt % 2 == 1) && !_isNumbered[k]) {
                        _numberedNodes.push_back(k);
                        _isNumbered[k] = true;
                        _priorities[k] = UINT_MAX;  // comment out if prioritizer has
                                                    // already been initialized

                        //_prior.erase(k); //uncomment if prioritizer has been
                        // initialized
                }
                twoPinEdgeDegrees[k] = total2PinEdgeWt;
        }
        ComparatorForOddSort c(_noTriGraph, _movables, twoPinEdgeDegrees);
        if (_verb.getForMajStats() || _verb.getForSysRes()) cout << "before sorting odd nodes, _numberedNodes= " << _numberedNodes;
        std::sort(_numberedNodes.begin() + oldSize, _numberedNodes.end(), c);
}

void BBPart::ReorderMovables::_twoHops() {
        unsigned topIdx;

        while (_prior.getMaxPriority() >= bbpart_twoHopGoalWeight && ((topIdx = _prior.top()) != UINT_MAX)) {
                _updateTwoHopPriors(_movables[topIdx]);
                _numberedNodes.push_back(topIdx);
                _prior.dequeue(topIdx);
                _isNumbered[topIdx] = true;  // important that this happen *after*
                // call to _updateTwoHopPriors()
                if (_verb.getForMajStats() >= 40) cout << "_prior=" << _prior;
        }
}

void BBPart::ReorderMovables::_dom2PinEdges() {

        itHGFEdgeGlobal iEG = _noTriGraph.edgesBegin();
        for (; iEG != _noTriGraph.edgesEnd(); iEG++) {
                HGFEdge const &edge = *(*iEG);
                if (edge.getDegree() != 2) continue;
                itHGFNodeLocal iN = edge.nodesBegin();
                HGFNode const &n1 = *(*iN), &n2 = *(*(iN + 1));
                unsigned i1 = n1.getIndex(), i2 = n2.getIndex();
                unsigned m1 = _mapBack[i1], m2 = _mapBack[i2];

                if (m1 != UINT_MAX) {
                        if (_isNumbered[m1]) continue;  // edge already
                                                        // dominated
                }
                if (m2 != UINT_MAX) {
                        if (_isNumbered[m2]) continue;  // edge already
                                                        // dominated
                }
                if (m1 == UINT_MAX && m2 == UINT_MAX) continue;  // neither node movable

                unsigned w1 = _totalEdgeWeight(n1), w2 = _totalEdgeWeight(n2);

                unsigned i;
                if (m1 == UINT_MAX)
                        i = m2;
                else if (m2 == UINT_MAX)
                        i = m1;
                else
                        i = (w1 > w2) ? m1 : m2;
                _updateTwoHopPriors(_movables[i]);
                _numberedNodes.push_back(i);
                _prior.dequeue(i);
                _isNumbered[i] = true;  // important that this happen *after* call
                // to _updateTwoHopPriors();
                if (_verb.getForMajStats() >= 40) cout << "_prior= (before _twoHops()) " << _prior;
                _twoHops();
                if (_verb.getForMajStats() >= 40) cout << "_prior= (after _twoHops() " << _prior;
        }
}

void BBPart::ReorderMovables::_updateTwoHopPriors(unsigned markedOrigIdx) {
        _changedPriority.clear();
        unsigned markedIdx = _mapBack[markedOrigIdx];
#ifdef ABKDEBUG
        if (markedIdx != UINT_MAX) {
                abkfatal(!_twoHopsComputed[markedOrigIdx],
                         "double computation of "
                         "two-hop priorities");
        }
#endif
        HGFNode const &markedNode = _noTriGraph.getNodeByIdx(markedOrigIdx);
        itHGFEdgeLocal iE = markedNode.edgesBegin();
        for (; iE != markedNode.edgesEnd(); iE++) {
                HGFEdge const &firstEdge = *(*iE);
                if (firstEdge.getDegree() != 2) continue;
                itHGFNodeLocal iNbetw = firstEdge.nodesBegin();
                if ((*iNbetw)->getIndex() == markedNode.getIndex()) ++iNbetw;  // want other node
                HGFNode const &betwNode = *(*iNbetw);
                unsigned betwIdx = _mapBack[betwNode.getIndex()];
                if (betwIdx == UINT_MAX) continue;
                if (_isNumbered[betwIdx]) continue;  // only length-2 paths via unnumbered nodes

                if (markedIdx != UINT_MAX && !_isNumbered[markedIdx]) {  // call only when *newly*
                                                                         // marking a movable node
                        _reduceNbrPriors(betwIdx, markedNode, firstEdge.getWeight());
                }

                itHGFEdgeLocal i2E = betwNode.edgesBegin();

                for (; i2E != betwNode.edgesEnd(); i2E++) {
                        HGFEdge const &secondEdge = *(*i2E);
                        if (secondEdge.getDegree() != 2) continue;
                        itHGFNodeLocal iNend = secondEdge.nodesBegin();
                        if ((*iNend)->getIndex() == betwNode.getIndex()) ++iNend;  // want other node
                        HGFNode const &endNode = *(*iNend);
                        unsigned endIdx = _mapBack[endNode.getIndex()];
                        if (endIdx != UINT_MAX && !_isNumbered[endIdx] && endIdx != markedIdx) {
                                _priorities[endIdx] += unsigned(min(double(firstEdge.getWeight()), double(secondEdge.getWeight())) * bbpart_twoHopGoalWeight + 0.5);
                                _changedPriority.setBit(endIdx);
                        }
                }

                _twoHopsComputed[markedOrigIdx] = true;
        }
        for (unsigned k = 0; k < _movables.size(); k++) {
                if (_changedPriority.isBitSet(k)) {
                        _prior.changePriorHead(k, _priorities[k]);
                        if (_verb.getForMajStats() >= 40) cout << "_prior= (after requeue)" << _prior;
                }
        }
}

// nbrIdx is index in _movables
// _movables[nbrIdx] is a neighbor of markedOrigIdx
// via a 2-pin edge, and its current priority may include
// two-hop paths via markedOrigIdx.  When markedOrigIdx
// is marked, this component must be removed from the
// priority of the neighbor node.
void BBPart::ReorderMovables::_reduceNbrPriors(unsigned nbrIdx, HGFNode const &markedNode, double firstEdgeWeight) {
        abkassert(_mapBack[markedNode.getIndex()] != UINT_MAX, "called _reduceNbrPriors with non-movable marked node");
        unsigned nbrOrigIdx = _movables[nbrIdx];
        unsigned reductionCount = 0;
        itHGFEdgeLocal iE = markedNode.edgesBegin();
        for (; iE != markedNode.edgesEnd(); iE++) {
                HGFEdge const &edge = *(*iE);
                if (edge.getDegree() != 2) continue;
                itHGFNodeLocal iN = edge.nodesBegin();
                if ((*iN)->getIndex() == markedNode.getIndex()) iN++;  // want other node of 2-pin edge
                HGFNode const &sisterNode = *(*iN);
                unsigned sisterOrigIdx = sisterNode.getIndex();
                if (sisterOrigIdx == nbrOrigIdx) continue;
                unsigned sisterIdx = _mapBack[sisterOrigIdx];
                if (_twoHopsComputed[sisterOrigIdx]) {
                        abkassert(sisterIdx == UINT_MAX || _isNumbered[sisterIdx], "two-hop priors computed, but not numbered???");
                        reductionCount += unsigned(min(firstEdgeWeight, double(edge.getWeight())) * bbpart_twoHopGoalWeight + 0.5);
                }
        }

#ifdef ABKDEBUG
        if (reductionCount > _priorities[nbrIdx]) {
                cout << "Inconsistency in _reduceNbrPriors " << endl;
                cout << "nbrIdx = " << nbrIdx << " orig idx= " << _movables[nbrIdx] << " priority= " << _priorities[nbrIdx] << " reductionCount= " << reductionCount << endl << " marked idx in _movables " << _mapBack[markedNode.getIndex()] << " marked orig idx= " << markedNode.getIndex() << endl;
                abkfatal(0, "Inconsistency in _reduceNbrPriors ")
        }
#endif
        _priorities[nbrIdx] -= reductionCount;
        _changedPriority.setBit(nbrIdx);
}

void BBPart::ReorderMovables::_tailOrdering() {
        vector<unsigned> edgePriorities(_noTriGraph.getNumEdges(), UINT_MAX);
        vector<unsigned> numMovables(_noTriGraph.getNumEdges());

        // Initialize priorities for hyperedges
        itHGFEdgeGlobal iE = _noTriGraph.edgesBegin();
        for (; iE != _noTriGraph.edgesEnd(); iE++) {
                HGFEdge const &edge = *(*iE);
                unsigned numberedNodeCount = 0, movNodeCount = 0;
                itHGFNodeLocal iN = edge.nodesBegin();
                for (; iN != edge.nodesEnd(); iN++) {
                        HGFNode const &node = *(*iN);
                        unsigned nodeIdx = node.getIndex();
                        unsigned movIdx = _mapBack[nodeIdx];
                        if (movIdx != UINT_MAX) {
                                movNodeCount++;
                                if (_isNumbered[movIdx]) numberedNodeCount++;
                        }
                }

                unsigned &p = edgePriorities[edge.getIndex()];
                if (numberedNodeCount == movNodeCount)
                        p = UINT_MAX;  // omit from prioritizer
                else
                        p = numberedNodeCount;  // note higher numbers are used
                                                // as
                //*lower* priorities, in the sense
                // that we'll be reading _edgePrior from
                // the *bottom*
                numMovables[edge.getIndex()] = movNodeCount;
        }

        _edgePrior.populate(edgePriorities);

        _prioritizeNodesForTailOrdering();

        if (_verb.getForMajStats() >= 40) cout << "_edgePrior= " << _edgePrior << endl;

        while (_prior.top() != UINT_MAX) {
                unsigned toNbr = _prior.top();
                _numberedNodes.push_back(toNbr);
                _isNumbered[toNbr] = true;
                _prior.dequeue(toNbr);
                if (_verb.getForMajStats() >= 40) cout << "_prior=" << _prior;
                HGFNode const &node = _noTriGraph.getNodeByIdx(_movables[toNbr]);

                // now reprioritize edges on this node
                unsigned oldMinP = _edgePrior.getMinPriority();
                itHGFEdgeLocal iEre = node.edgesBegin();
                for (; iEre != node.edgesEnd(); iEre++) {
                        HGFEdge const &edgeRe = *(*iEre);
                        unsigned reIndex = edgeRe.getIndex();
                        if (_edgeDequeued[reIndex]) continue;  // can't reprioritize; it's not there
                        unsigned &p = edgePriorities[reIndex];
                        p++;  // this means we'll choose it *later*, not earlier
                        if (p == numMovables[reIndex]) {
                                p = UINT_MAX;
                                _edgePrior.dequeue(reIndex);
                                _edgeDequeued[reIndex] = true;
                                if (_verb.getForMajStats() >= 40) cout << "_edgePrior= (after erasure)" << _edgePrior;
                        } else {
                                if (_verb.getForMajStats() >= 40) cout << "_edgePrior= (before requeue) " << _edgePrior;
                                _edgePrior.changePriorHead(reIndex, p);
                                if (_verb.getForMajStats() >= 40) cout << "_edgePrior= (after requeue)" << _edgePrior;
                        }

                        // now reprioritize nodes on this edge
                        itHGFNodeLocal iNre = edgeRe.nodesBegin();
                        for (; iNre != edgeRe.nodesEnd(); iNre++) {
                                unsigned nodeOrigIdx = (*iNre)->getIndex();
                                unsigned nodeIdx = _mapBack[nodeOrigIdx];
                                if (nodeIdx != UINT_MAX && _prior.isEnqueued(nodeIdx)) {
                                        abkfatal(_priorities[nodeIdx] == _prior.getPriority(nodeIdx), "priority mismatch");
                                        abkfatal(_priorities[nodeIdx] >= bbpart_tailOrderingWeight,
                                                 "In tail ordering, need to "
                                                 "reduce priority by "
                                                 "more than priority");
                                        _priorities[nodeIdx] -= bbpart_tailOrderingWeight;
                                        _prior.changePriorHead(nodeIdx, _priorities[nodeIdx]);
                                }
                        }
                }
                if (_edgePrior.getMinPriority() != oldMinP) _prioritizeNodesForTailOrdering();
        }
}

BBPart::ReorderMovables::ReorderMovables(vector<unsigned> &movables, HGraphFixed const &hgraph, Verbosity verb) : _verb(verb), _movables(movables), _maxNodeSize(0.0), _minNodeSize(DBL_MAX), _noTriGraph(hgraph), _isNumbered(movables.size(), false), _changedPriority(movables.size()), _twoHopsComputed(_noTriGraph.getNumNodes(), false), _mapBack(hgraph.getNumNodes(), UINT_MAX), _priorities(movables.size(), 0), _edgeDequeued(_noTriGraph.getNumEdges(), false) {
        unsigned k;
        for (k = 0; k < _movables.size(); k++) {
                HGFNode const &node = _noTriGraph.getNodeByIdx(_movables[k]);
                double wt = _noTriGraph.getWeight(node.getIndex());
                if (wt > _maxNodeSize) _maxNodeSize = wt;
                if (wt < _minNodeSize) _minNodeSize = wt;
        }
        _numberedNodes.reserve(_movables.size());
        _numberOddNodes();
        if (verb.getForMajStats()) cout << double(_numberedNodes.size()) / _movables.size() << " after g1" << endl;
        if (verb.getForMajStats() >= 10) {
                cout << "Numbered nodes after g1: ";
                for (k = 0; k < _numberedNodes.size(); k++) cout << _numberedNodes[k] << " ";
                cout << endl;
        }
        _numberHairyNodes();
        if (verb.getForMajStats()) cout << double(_numberedNodes.size()) / _movables.size() << " after g2" << endl;
        if (verb.getForMajStats() >= 10) {
                cout << "Numbered nodes after g2: ";
                for (k = 0; k < _numberedNodes.size(); k++) cout << _numberedNodes[k] << " ";
                cout << endl;
        }

        for (k = 0; k < _movables.size(); k++) {
                HGFNode const &node = _noTriGraph.getNodeByIdx(_movables[k]);
                if (_isNumbered[k])
                        _priorities[k] = UINT_MAX;
                else {
                        double wt = _noTriGraph.getWeight(node.getIndex());
                        _priorities[k] = node.getDegree() * bbpart_degreeWeight + WEIGHT_TERM;
                }
        }

        _prior.populate(_priorities);

        for (k = 0; k < _movables.size(); k++) {
                _mapBack[_movables[k]] = k;
        }

        for (k = 0; k < _noTriGraph.getNumNodes(); k++) {
                if (_mapBack[k] == UINT_MAX) _updateTwoHopPriors(k);  // two-hops from terminals
        }

        for (unsigned n = 0; n < _numberedNodes.size(); n++) {
                abkassert(_isNumbered[_numberedNodes[n]], "Node numbered, but _isNumbered thinks not");
                _updateTwoHopPriors(_movables[_numberedNodes[n]]);
        }

        _twoHops();
        if (verb.getForMajStats()) cout << double(_numberedNodes.size()) / _movables.size() << " after g3" << endl;
        if (verb.getForMajStats() >= 10) {
                cout << "Numbered nodes after g3: ";
                for (k = 0; k < _numberedNodes.size(); k++) cout << _numberedNodes[k] << " ";
                cout << endl;
        }

        _dom2PinEdges();
        if (verb.getForMajStats()) cout << double(_numberedNodes.size()) / _movables.size() << " after g4" << endl;
        if (verb.getForMajStats() >= 10) {
                cout << "Numbered nodes after g4: ";
                for (k = 0; k < _numberedNodes.size(); k++) cout << _numberedNodes[k] << " ";
                cout << endl;
        }
        _tailOrdering();
        if (verb.getForMajStats()) cout << double(_numberedNodes.size()) / _movables.size() << " after tail ordering" << endl;

        _numberLooseNodes();  // nodes with no edges

        if (_verb.getForMajStats()) {
                cout << "Listing of _movables" << endl;
                for (k = 0; k < _movables.size(); k++) cout << _movables[k] << " ";
                cout << endl;
                cout << "Vertex ordering at end of ReorderMovables "
                        "(in terms of index in _movables):" << endl;
                for (k = 0; k < _numberedNodes.size(); k++) cout << _numberedNodes[k] << " ";
                cout << endl;
        }
        abkfatal(_numberedNodes.size() == _movables.size(),
                 "Vertex reordering changed "
                 "number of vertices");

        vector<unsigned> temp(_movables);
        _movables.clear();
        _movables.reserve(temp.size());

        for (k = 0; k < _numberedNodes.size(); k++) _movables.push_back(temp[_numberedNodes[k]]);

        if (_verb.getForMajStats()) {
                cout << "Listing of _movables after reordering" << endl;
                for (k = 0; k < _movables.size(); k++) cout << _movables[k] << " ";
                cout << endl;
        }
}

void BBPart::ReorderMovables::_prioritizeNodesForTailOrdering() {
        unsigned edgeIdx = _edgePrior.headBottom();
        if (edgeIdx == UINT_MAX) {
                _prior.dequeueAll();
                return;
        }
        abkassert(!_edgeDequeued[edgeIdx], "Edge reappeared after erasure");
        for (unsigned i = 0; i < _movables.size(); i++) {
                HGFNode const &node = _noTriGraph.getNodeByIdx(_movables[i]);
                double wt = _noTriGraph.getWeight(node.getIndex());
                _priorities[i] = node.getDegree() * bbpart_degreeWeight + WEIGHT_TERM;
        }
        _changedPriority.clear();
        while (edgeIdx != UINT_MAX) {
                HGFEdge const &edge = _noTriGraph.getEdgeByIdx(edgeIdx);
                itHGFNodeLocal iN = edge.nodesBegin();
                for (; iN != edge.nodesEnd(); iN++) {
                        unsigned nodeIdx = (*iN)->getIndex();
                        unsigned movIdx = _mapBack[nodeIdx];

                        if (movIdx != UINT_MAX && !_isNumbered[movIdx]) {
                                _priorities[movIdx] += bbpart_tailOrderingWeight;
                                _changedPriority.setBit(movIdx);
                        }
                }

                edgeIdx = _edgePrior.next();
        }
        _prior.dequeueAll();
        for (unsigned k = 0; k < _movables.size(); k++) {
                if (_changedPriority.isBitSet(k)) {
                        _prior.enqueueHead(k, _priorities[k]);
                }
        }
        if (_verb.getForMajStats() >= 40) cout << "In _prioritizeNodesForTailOrdering, _prior=" << _prior;
}

void BBPart::ReorderMovables::_numberLooseNodes() {
        vector<unsigned> dummyDegs(_movables.size(), 0);
        if (_movables.size() == _numberedNodes.size()) return;
        abkfatal(_numberedNodes.size() < _movables.size(), "Impossibility in _numberLooseNodes");
        vector<unsigned> temp;
        temp.reserve(_movables.size() - _numberedNodes.size());

        unsigned k;
        for (k = 0; k < _movables.size(); k++) {
                if (!_isNumbered[k]) temp.push_back(k);
        }
        ComparatorForOddSort c(_noTriGraph, _movables, dummyDegs);
        std::sort(temp.begin(), temp.end(), c);
        for (k = 0; k < temp.size(); k++) {
                _numberedNodes.push_back(temp[k]);
                _isNumbered[temp[k]] = true;
        }
}
