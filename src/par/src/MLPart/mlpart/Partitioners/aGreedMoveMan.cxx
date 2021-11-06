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

// created by Igor Markov on 08/27/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#ifndef _AGreedMM_CXX_
#define _AGreedMM_CXX_

#include "aGreedMoveMan.h"

template <class Evaluator>
AGreedMoveManager<Evaluator>::AGreedMoveManager(const PartitioningProblem& problem)
    : AcceleratedMoveManagerTemplateBase<Evaluator>(problem) {
        // need to call resetTo before the pass and then
        abkfatal(problem.getNumPartitions() == 2, " AGreed move manager requires bipartitioning");
}

/*   bool isMovable=false;
     int  baseCost=_eval.getTotalCost();

     unsigned from=(*_curPart)[moduleIdx].lowestNumPart();
     PartitionIds canGoTo=(*_fixedConstr)[moduleIdx];

     for(unsigned to=0; to!=_numParts; to++)
     {
         if (to==from)     continue;
         if (!canGoTo[to]) continue;
         _eval.moveModuleTo(moduleIdx,from,to);  // to compute "what if" cost
         int gain=baseCost - _eval.getTotalCost();
         _gainCo.addElement(moduleIdx,to,gain);
         isMovable=true;
         _eval.moveModuleTo(moduleIdx,to,from);  // move the module back
     }
     _lockedModules[moduleIdx]= !isMovable;
*/

/* PartitionIds& movingIds=(*_curPart)[_movingModuleIdx];
 {
   movingIds.removeFromPart(_moveFromPart);
   movingIds.addToPart     (_moveToPart);
 }
   _eval.moveModuleTo   (_movingModuleIdx,_moveFromPart,_moveToPart);
   _partLeg.moveModuleTo(_movingModuleIdx,_moveFromPart,_moveToPart);
*/

template <class Evaluator>
unsigned AGreedMoveManager<Evaluator>::doOnePass() {
        unsigned numModules = AGreedMoveManager<Evaluator>::_problem.getHGraph().getNumNodes();
        unsigned numMovesMade = 0;
        int curCost = AGreedMoveManager<Evaluator>::_eval.getTotalCost();
        if (!AGreedMoveManager<Evaluator>::randomized) {
                for (unsigned moduleIdx = 0; moduleIdx != numModules; moduleIdx++) {
                        unsigned from = (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].lowestNumPart(), to = 1 - from;
                        PartitionIds canGoTo = (*AGreedMoveManager<Evaluator>::_fixedConstr)[moduleIdx];

                        if (!canGoTo[to] || !AGreedMoveManager<Evaluator>::_partLeg.isLegalMove(moduleIdx, from, to)) continue;

                        AGreedMoveManager<Evaluator>::_eval.moveModuleTo(moduleIdx, from, to);  // to compute "what if" cost
                        int newCost = AGreedMoveManager<Evaluator>::_eval.getTotalCost();
                        if (newCost > curCost) {
                                AGreedMoveManager<Evaluator>::_eval.moveModuleTo(moduleIdx, to, from);  // move back
                                continue;
                        } else if (newCost == curCost) {
                                AGreedMoveManager<Evaluator>::_partLeg.moveModuleTo(moduleIdx, from, to);
                                curCost = newCost;
                                (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].setToPart(to);
                        } else {
                                numMovesMade++;
                                //        numMovesMade=1;
                                AGreedMoveManager<Evaluator>::_partLeg.moveModuleTo(moduleIdx, from, to);
                                curCost = newCost;
                                (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].setToPart(to);
                        }
                }
        } else {
                Permutation randPerm(numModules, Mapping::_Random);
                for (unsigned idx = 0; idx != numModules; idx++) {
                        unsigned moduleIdx = randPerm[idx];
                        unsigned from = (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].lowestNumPart(), to = 1 - from;
                        PartitionIds canGoTo = (*AGreedMoveManager<Evaluator>::_fixedConstr)[moduleIdx];

                        if (!canGoTo[to] || !AGreedMoveManager<Evaluator>::_partLeg.isLegalMove(moduleIdx, from, to)) continue;

                        AGreedMoveManager<Evaluator>::_eval.moveModuleTo(moduleIdx, from, to);  // to compute "what if" cost
                        int newCost = AGreedMoveManager<Evaluator>::_eval.getTotalCost();
                        if (newCost > curCost) {
                                AGreedMoveManager<Evaluator>::_eval.moveModuleTo(moduleIdx, to, from);  // move back
                                continue;
                        } else if (newCost == curCost) {
                                AGreedMoveManager<Evaluator>::_partLeg.moveModuleTo(moduleIdx, from, to);
                                curCost = newCost;
                                (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].setToPart(to);
                        } else {
                                numMovesMade++;
                                //        numMovesMade=1;
                                AGreedMoveManager<Evaluator>::_partLeg.moveModuleTo(moduleIdx, from, to);
                                curCost = newCost;
                                (*AGreedMoveManager<Evaluator>::_curPart)[moduleIdx].setToPart(to);
                        }
                }
        }
        return numMovesMade;
}

#endif
