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

// created by Igor Markov on 06/02/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#ifndef _FMMM_NC2w_CXX_
#define _FMMM_NC2w_CXX_

#include "fmMoveManagerNC2w.h"

using std::cerr;
using std::cout;
using std::endl;

void FMMoveManagerNC2w::computeGains() {
        unsigned moduleIdx;

        if (!randomized) {
                for (moduleIdx = 0; moduleIdx != _numTerminals + _numMovable; moduleIdx++) {
                        if (_useWts)
                                computeGainsOfModuleWW(moduleIdx);
                        else
                                computeGainsOfModule(moduleIdx);
                }
        } else {
                Permutation randPerm(_numTerminals + _numMovable, Mapping::_Random);
                for (moduleIdx = 0; moduleIdx != _numTerminals + _numMovable; moduleIdx++)
                        if (_useWts)
                                computeGainsOfModuleWW(randPerm[moduleIdx]);
                        else
                                computeGainsOfModule(randPerm[moduleIdx]);
        }
        if (wiggleNow) {
                for (moduleIdx = 0; moduleIdx != _numTerminals + _numMovable; moduleIdx++) {
                        if (_lockedModules[moduleIdx]) {
                                // touch gains of all neighbors
                                const HGFNode& n = _problem->getHGraph().getNodeByIdx(moduleIdx);
                                for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                                        const HGFEdge& net = (**e);
                                        for (itHGFNodeLocal gn = net.nodesBegin(); gn != net.nodesEnd(); gn++) {
                                                unsigned gainingModuleIdx = (*gn)->getIndex();
                                                if (_lockedModules[gainingModuleIdx]) continue;
                                                if (gainingModuleIdx == moduleIdx) continue;
                                                unsigned to = 1 - (*_curPart)[gainingModuleIdx].lowestNumPart();
                                                _gainCo.updateGain(gainingModuleIdx, to, 0);
                                        }
                                }
                        }
                }
                wiggleNow = false;
        }
        _gainCo.resetAfterGainUpdate();
        if (_problem->getParameters().verbosity.getForActions() > 2) abkassert(_gainCo.isConsistent(), "GainCo inconsistent in computeGains");
}

void FMMoveManagerNC2w::uncutNets() {
        cerr << " Running net uncutting " << endl;
        reinitialize();
        computeGains();
        // For all nets: uncut net if possible
        // cout << " Total cost before net uncutting " << _eval.getTotalCost()
        // << endl;
        double maxLegalMoveAreaTo[2] = {_partLeg.getMaxLegalMoveArea(1, 0), _partLeg.getMaxLegalMoveArea(0, 1)};
        // cout << " Is legal before uncutting " << _partLeg.isPartLegal() <<
        // endl;
        // cout<<"MaxLegalMoveAreas:"<<maxLegalMoveAreaTo[0]<<"
        // "<<maxLegalMoveAreaTo[1];
        // cout<< endl;
        const HGraphFixed& hgraph = _problem->getHGraph();
        for (itHGFEdgeGlobal ee = hgraph.edgesBegin(); ee != hgraph.edgesEnd(); ee++) {
                const HGFEdge& e = (**ee);
                if (e.getDegree() < 5) continue;
                if (e.getDegree() > 30) continue;
                unsigned netIdx = e.getIndex();
                if (getNetCost(netIdx) == 0) continue;
                bool tryMovingTo[2] = {_tallies[2 * netIdx + 1] > 2, _tallies[2 * netIdx] > 2};
                if (!tryMovingTo[0] && !tryMovingTo[1]) continue;
                int gainSum[2] = {0, 0};
                maxLegalMoveAreaTo[0] = _partLeg.getMaxLegalMoveArea(1, 0);
                maxLegalMoveAreaTo[1] = _partLeg.getMaxLegalMoveArea(0, 1);
                double moveAreaSoFarTo[2] = {0, 0};

                itHGFNodeLocal node;
                for (node = e.nodesBegin(); node != e.nodesEnd(); node++) {
                        _movingModuleIdx = (*node)->getIndex();
                        PartitionIds& curr = (*_curPart)[_movingModuleIdx];
                        _moveToPart = (curr.isInPart(0) ? 1 : 0);
                        if (!tryMovingTo[_moveToPart]) continue;

                        _moveFromPart = 1 - _moveToPart;
                        PartitionIds canGoTo = (*_fixedConstr)[_movingModuleIdx];
                        moveAreaSoFarTo[_moveToPart] += hgraph.getWeight((*node)->getIndex());
                        if (!canGoTo[_moveToPart] || moveAreaSoFarTo[_moveToPart] >= maxLegalMoveAreaTo[_moveToPart]) {
                                tryMovingTo[_moveToPart] = false;
                                if (tryMovingTo[_moveFromPart])
                                        continue;
                                else
                                        break;
                        }
                        gainSum[_moveToPart] += _gainCo.getElement(_movingModuleIdx, _moveToPart).getRealGain();
                }
                if (!tryMovingTo[0] && !tryMovingTo[1]) continue;
                unsigned moveNowTo = UINT_MAX;
                if (!tryMovingTo[0]) {
                        if (gainSum[1] < 0) continue;
                        moveNowTo = 1;
                } else if (!tryMovingTo[1]) {
                        if (gainSum[0] < 0) continue;
                        moveNowTo = 0;
                } else {
                        if (gainSum[0] > gainSum[1]) {
                                if (gainSum[0] < 0) continue;
                                moveNowTo = 0;
                        } else {
                                if (gainSum[1] < 0) continue;
                                if (gainSum[1] > gainSum[0])
                                        moveNowTo = 1;
                                else
                                        moveNowTo = (_tallies[2 * netIdx] > _tallies[2 * netIdx + 1] ? 1 : 0);
                        }
                }

                // now UNCUT the net by moving all nodes into _moveToPart
                cout << "Uncutting net " << netIdx << " of degree " << e.getDegree() << " into " << moveNowTo << " with sum of gains " << gainSum[moveNowTo];
                unsigned willMove = _tallies[2 * netIdx + (1 - moveNowTo)];
                cout << "; will move " << willMove << " nodes " << endl;
                /*
                     cout << "Tallies: " << _eval.getTallies()[2*netIdx] << " "
                   <<
                                          _eval.getTallies()[2*netIdx+1];
                     cout << " tryMovingTo: " << tryMovingTo[0]<<" "<<
                   tryMovingTo[1] << endl;
                */
                //   cout << " Max legal move areas : " <<
                //           maxLegalMoveAreaTo[0]  << " " <<
                // maxLegalMoveAreaTo[1] ;
                //   cout << " this move " << moveAreaSoFarTo[0]<<"
                // "<<moveAreaSoFarTo[1]<<endl;
                unsigned nodesMoved = 0;
                for (node = e.nodesBegin(); node != e.nodesEnd(); node++) {
                        _movingModuleIdx = (*node)->getIndex();
                        PartitionIds& curr = (*_curPart)[_movingModuleIdx];
                        _moveToPart = (curr.isInPart(0) ? 1 : 0);
                        if (_moveToPart != moveNowTo) continue;
                        if (nodesMoved++ == willMove) break;
                        _moveFromPart = 1 - _moveToPart;
                        updateGains();
                        (*_curPart)[_movingModuleIdx].setToPart(_moveToPart);
                        moveModuleTo(_movingModuleIdx, _moveFromPart, _moveToPart);
                        _partLeg.moveModuleTo(_movingModuleIdx, _moveFromPart, _moveToPart);
                }
                //  abkfatal(_partLeg.isPartLegal(), " Illegal partition ");
                //   cout << " actually moved " << nodesMoved << endl;
        }
        // cout << " Total cost after net uncutting " << _eval.getTotalCost() <<
        // endl;
        // cout << " Is legal after uncutting " << _partLeg.isPartLegal() <<
        // endl;
        // cout<<"MaxLegalMoveAreas:"<<maxLegalMoveAreaTo[0]<<"
        // "<<maxLegalMoveAreaTo[1];
        // cout << "\n-----------" << endl;
        _gainCo.resetAfterGainUpdate();
}

#define FMMMclass FMMoveManagerNC2w

#define updateGainOfModule(moduleIdx, from, to, deltaGain) _gainCo.updateGain(moduleIdx, to, deltaGain)

// FMMMclass needs to be #defined in the #including class

#include "PartEvals/univPartEval.h"

bool FMMMclass::pickMoveApplyIt() {
        // cout << _gainCo << endl;
        if (!pickMove()) return false;  // side effect !
        _gainCo.setPartitionBias(_moveToPart);
        // now apply the move

        // unsigned nodeDeg =
        //	_problem.getHGraph().getNodeByIdx(_movingModuleIdx).getDegree();

        updateGains();

        _gainCo.resetAfterGainUpdate();

        PartitionIds& movingIds = (*_curPart)[_movingModuleIdx];

        _moveLog.logMove(_movingModuleIdx, movingIds);

        movingIds.setToPart(_moveToPart);

        //  for 2-way can be done during gain update
        // _eval.moveModuleTo   (_movingModuleIdx,_moveFromPart,_moveToPart);
        _partLeg.moveModuleTo(_movingModuleIdx, _moveFromPart, _moveToPart);
        // cout<<"num is "<<_movingModuleIdx<<endl;
        if (_problem->getParameters().verbosity.getForActions() > 2) abkassert(_gainCo.isConsistent(), "GainCo inconsistent in pickMoveApplyIt");

        return true;
}

bool FMMMclass::pickMove()
    /* returns false, if no moves are available */
    /* side-effect: stores the chosen move as 3 unsigneds */
    /* calls isMoveLegal() and can change gain container, thus not const */
{
        // const HGraphFixed& hg = _problem.getHGraph();

        int invalidatedNodesInBucket = 0;
        SVGainElement* moveToMake;
        while ((moveToMake = _gainCo.getHighestGainElement()) != static_cast<SVGainElement*>(NULL)) {
                // cout<<"Part Areas are "<<_partLeg<<endl;

                _movingModuleIdx = _gainCo.getElementId(*moveToMake);
                _moveToPart = _gainCo.getElementPartition(*moveToMake);
                _moveFromPart = (*_curPart)[_movingModuleIdx].lowestNumPart();

                // cout<<" Picked move "<<_movingModuleIdx<<" "
                //<<_moveFromPart<<" -> "<<_moveToPart
                //<<"  of size
                //"<<hg.getNodeByIdx(_movingModuleIdx).getWeight()<<endl;

                if (isMoveLegal() || (_partLeg.getViolation() == 0 && allowIllegalMoves))
                        return true;
                else {
                        if (++invalidatedNodesInBucket < unCorking) {
                                if (_gainCo.invalidateElement(_moveToPart)) invalidatedNodesInBucket = 0;
                        } else {
                                invalidatedNodesInBucket = 0;
                                _gainCo.invalidateBucket(_moveToPart);
                        }
                }
        }
        return false;  // nothing legal to move
}

// FMMMclass needs to be #defined in the #including file

void FMMMclass::updateAffectedModules()
    // wrt to _movingModuleIdx, _moveFromPart and _moveToPart
{
        unsigned newLockedIds = 1 << _moveToPart;
        const HGFNode& n = _problem->getHGraph().getNodeByIdx(_movingModuleIdx);
        unsigned netId;

        const unsigned short* evalTallies = _tallies;

        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++, _useWts ? moveModuleOnNetWW(netId, _moveFromPart, _moveToPart) : moveModuleOnNet(netId, _moveFromPart, _moveToPart)) {
                const HGFEdge& net = (**e);
                netId = net.getIndex();

                if (net.getDegree() == 2) {
                        itHGFNodeLocal node = net.nodesBegin();
                        unsigned nodeIdx;
                        if ((*node)->getIndex() == _movingModuleIdx)
                                nodeIdx = (*++node)->getIndex();
                        else
                                nodeIdx = (*node)->getIndex();
                        if (!_lockedModules[nodeIdx]) {
                                unsigned curPart = (*_curPart)[nodeIdx].isInPart(0) ? 0 : 1;
                                if (curPart == _moveFromPart) {
                                        updateGainOfModule(nodeIdx, UINT_MAX, 1 - curPart, 2);
                                } else {
                                        updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -2);
                                }
                        }
                        continue;
                }

                if (_lockedNets[netId]) continue;
                if (_lockedNetConfigIds[netId] & (~newLockedIds))
                        _lockedNets[netId] = true;
                else
                        _lockedNetConfigIds[netId] |= newLockedIds;

                if (netCostNotAffectedByMove(netId, _moveFromPart, _moveToPart)) continue;

                //   {
                const unsigned short* _tallies = evalTallies + 2 * netId;
                itHGFNodeLocal nodeIter = net.nodesBegin();  // 3+ nodes on the net

                if (_tallies[_moveToPart] == 0) {
                        for (; nodeIter != net.nodesEnd(); nodeIter++) {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;
                                updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, 1);
                        }
                        continue;
                }

                if (_tallies[_moveFromPart] == 1) {
                        for (; nodeIter != net.nodesEnd(); nodeIter++) {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;
                                updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1);
                        }
                        continue;
                }

                if (net.getDegree() == 3)
                        for (; nodeIter != net.nodesEnd(); nodeIter++)  //
                        {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;

                                if ((*_curPart)[nodeIdx].isInPart(_moveFromPart)) {
                                        if (_tallies[_moveFromPart] == 2) updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, 1);
                                } else  // net was uncut
                                {
                                        if (_tallies[_moveToPart] == 1) updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1);
                                }
                        }
                else
                        for (; nodeIter != net.nodesEnd(); nodeIter++)  // 4+ nodes
                        {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;

                                if ((*_curPart)[nodeIdx].isInPart(_moveFromPart)) {
                                        if (_tallies[_moveFromPart] == 2) {
                                                updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, 1);
                                                break;
                                        }
                                } else if (_tallies[_moveToPart] == 1) {
                                        updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1);
                                        break;
                                }
                        }
                continue;
                //   }
        }
}

void FMMMclass::updateAffectedModulesWW()
    // wrt to _movingModuleIdx, _moveFromPart and _moveToPart
{
        unsigned newLockedIds = 1 << _moveToPart;
        const HGFNode& n = _problem->getHGraph().getNodeByIdx(_movingModuleIdx);
        unsigned netId;

        const unsigned short* evalTallies = _tallies;

        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++, _useWts ? moveModuleOnNetWW(netId, _moveFromPart, _moveToPart) : moveModuleOnNet(netId, _moveFromPart, _moveToPart)) {
                const HGFEdge& net = (**e);
                netId = net.getIndex();

                if (net.getDegree() == 2) {
                        itHGFNodeLocal node = net.nodesBegin();
                        unsigned nodeIdx;
                        if ((*node)->getIndex() == _movingModuleIdx)
                                nodeIdx = (*++node)->getIndex();
                        else
                                nodeIdx = (*node)->getIndex();
                        if (!_lockedModules[nodeIdx]) {
                                unsigned curPart = (*_curPart)[nodeIdx].isInPart(0) ? 0 : 1;
                                if (curPart == _moveFromPart) {
                                        updateGainOfModule(nodeIdx, UINT_MAX, 1 - curPart, 2 * _edgeWeights[netId]);
                                } else {
                                        updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -2 * _edgeWeights[netId]);
                                }
                        }
                        continue;
                }

                if (_lockedNets[netId]) continue;
                if (_lockedNetConfigIds[netId] & (~newLockedIds))
                        _lockedNets[netId] = true;
                else
                        _lockedNetConfigIds[netId] |= newLockedIds;

                if (netCostNotAffectedByMove(netId, _moveFromPart, _moveToPart)) continue;

                //   {
                const unsigned short* _tallies = evalTallies + 2 * netId;
                itHGFNodeLocal nodeIter = net.nodesBegin();  // 3+ nodes on the net

                if (_tallies[_moveToPart] == 0) {
                        for (; nodeIter != net.nodesEnd(); nodeIter++) {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;
                                updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, _edgeWeights[netId]);
                        }
                        continue;
                }

                if (_tallies[_moveFromPart] == 1) {
                        for (; nodeIter != net.nodesEnd(); nodeIter++) {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;
                                updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1 * _edgeWeights[netId]);
                        }
                        continue;
                }

                if (net.getDegree() == 3)
                        for (; nodeIter != net.nodesEnd(); nodeIter++)  //
                        {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;

                                if ((*_curPart)[nodeIdx].isInPart(_moveFromPart)) {
                                        if (_tallies[_moveFromPart] == 2) updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, _edgeWeights[netId]);
                                } else  // net was uncut
                                {
                                        if (_tallies[_moveToPart] == 1) updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1 * _edgeWeights[netId]);
                                }
                        }
                else
                        for (; nodeIter != net.nodesEnd(); nodeIter++)  // 4+ nodes
                        {
                                unsigned nodeIdx = (*nodeIter)->getIndex();
                                if (_lockedModules[nodeIdx]) continue;

                                if ((*_curPart)[nodeIdx].isInPart(_moveFromPart)) {
                                        if (_tallies[_moveFromPart] == 2) {
                                                updateGainOfModule(nodeIdx, UINT_MAX, _moveToPart, _edgeWeights[netId]);
                                                break;
                                        }
                                } else if (_tallies[_moveToPart] == 1) {
                                        updateGainOfModule(nodeIdx, UINT_MAX, _moveFromPart, -1 * _edgeWeights[netId]);
                                        break;
                                }
                        }
                continue;
                //   }
        }
}

#undef FMMMclass
#undef updateGainOfModule

#endif
