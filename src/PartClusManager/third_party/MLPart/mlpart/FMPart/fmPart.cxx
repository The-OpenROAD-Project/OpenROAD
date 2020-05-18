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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "fmPart.h"
#include "PartEvals/netCut2way.h"
#include "PartEvals/netCut2wayWWeights.h"
#include "PartLegality/1balanceGen.h"
#include "PartLegality/bfsGen.h"
#include "dcGainCont.h"
#include "mmSwitchBox.h"
#include "PartLegality/solnGenRegistry.h"
#include <sstream>

using std::cout;
using std::endl;
using std::setw;
using std::max;
using uofm::vector;

FMPartitioner::FMPartitioner(PartitioningProblem& problem, MaxMem* maxMem, const Parameters& params, bool skipSolnGen, bool dontRunYet) : MultiStartPartitioner(problem, params), _activeMoveMgr(NULL), _moveMgrLIFO(NULL), _moveMgr2(NULL), _switchBox(NULL), _totalMovesMade(0), _totalTime(0.0), _peakMemUsage(0.0), _skipSolnGen(skipSolnGen), _numPasses(UINT_MAX), _maxMem(maxMem) {
        abkfatal(problem.getNumPartitions() == 2,
                 "More than 2-way"
                 "partitioning is not supported in this distribution. For more"
                 "details contact Igor Markov <imarkov@eecs.umich.edu>");

        if (_params.verb.getForMajStats() > 5) cout << " Using " << _params.eval << " evaluator " << endl;

        setMoveManagerAndSwitchBox();  // once for all starts

        static unsigned fmCall = 0;

        if (!dontRunYet) {
                fmCall++;

                MemUsage m;
                _peakMemUsage = max(_peakMemUsage, m.getPeakMem());

                uofm::stringstream updateMessage1;
                updateMessage1 << "FMPart call #" << fmCall << " before runMultiStart()";
                _maxMem->update(updateMessage1.str().c_str());

                if (_params.verb.getForMajStats() > 0) cout << m << " during FMPart call #" << fmCall << " before runMultiStart()" << endl;

                runMultiStart();

                MemUsage m1;
                _peakMemUsage = max(_peakMemUsage, m1.getPeakMem());

                uofm::stringstream updateMessage2;
                updateMessage2 << "FMPart call #" << fmCall << " after runMultiStart()";
                _maxMem->update(updateMessage2.str().c_str());

                if (_params.verb.getForMajStats() > 0) cout << m1 << " during FMPart call #" << fmCall << " after runMultiStart()" << endl;

                if (_params.verb.getForMajStats() > 4) {
                        cout << " Made " << _totalMovesMade << " in " << _totalTime << " seconds.  ";
                        cout << (double)_totalMovesMade / _totalTime << " moves per second" << endl;
                }
                if (_params.printHillStats) {
                        cout << " Max hill seen (h/w) : " << _maxHillHeightSeen << "/" << _maxHillWidthSeen << endl;
                        cout << " Max good hill seen (h/w) : " << _maxGoodHillHeightSeen << "/" << _maxGoodHillWidthSeen << endl;
                }

                MemUsage m2;
                _peakMemUsage = max(_peakMemUsage, m2.getPeakMem());
        }
}

void FMPartitioner::doOneFM(unsigned initSoln) {
        Timer moveTimer;

        Partitioning& curPart = const_cast<Partitioning&>(_solutions[initSoln]->part);
        if (_numPasses == UINT_MAX) _numPasses = 0;

        if (!_skipSolnGen) {
                if (_params.verb.getForMajStats() > 5) cout << "In FM. Generating Initial Solution " << endl;

                if (_problem.getNumPartitions() == 2) {
                        switch (_params.solnGen) {
                                case SolnGenType::RandomizedEngineers: {
                                        //	    SingleBalanceGenerator2way
                                        // sGen(_problem);
                                        SBGWeightRand sGen(_problem);
                                        sGen.generateSoln(curPart);
                                        break;
                                }
                                case SolnGenType::AllToOne: {
                                        AllToOneGen sGen(_problem);
                                        sGen.generateSoln(curPart);
                                        break;
                                }
                                case SolnGenType::Bfs: {
                                        SBGWeightBfs sGen(_problem);
                                        sGen.generateSoln(curPart);
                                        break;
                                }
                                default:
                                        abkfatal(0, "Unknown solution generator");
                        }
                } else {
                        SingleBalanceGenerator sGen(_problem);
                        sGen.generateSoln(curPart);
                }
        }

        unsigned relTolPasses = _params.relaxedTolerancePasses;
        vector<double> maxCellSize(1, 0);
        if (relTolPasses > 0) {
                double tol0 = _problem.getMaxCapacities()[0][0] - _problem.getMinCapacities()[0][0];
                double tol1 = _problem.getMaxCapacities()[1][0] - _problem.getMinCapacities()[1][0];
                double tol = max(tol0, tol1);

                const HGraphFixed& h = _problem.getHGraph();
                for (itHGFNodeGlobal it = h.nodesBegin(); it != h.nodesEnd(); ++it)
                        if (h.getWeight((*it)->getIndex()) > maxCellSize[0]) maxCellSize[0] = h.getWeight((*it)->getIndex());

                // if(((maxCellSize[0]/(_problem.getTotalWeight())[0]) < 0.05 )
                // && maxCellSize[0]*1.8 < tol)
                if (maxCellSize[0] * 1.8 < tol) relTolPasses = 0;
                maxCellSize[0] *= 1.8;

                if (relTolPasses > 0) {
                        // cout<<"Relax tol pass. "<<" maxCellSize
                        // "<<maxCellSize<<" "<<_problem.getTotalWeight()<<endl;
                        doOneFMRelaxedTol(initSoln, relTolPasses, maxCellSize);
                }
        }

        // if(relTolPasses > 0)
        // return;

        //    _moveMgrLIFO->resetTo(*_solutions[initSoln]);
        //     if (_moveMgrLIFO!=_moveMgr2 && _moveMgr2)
        // _moveMgr2->resetTo(*_solutions[initSoln]);
        _activeMoveMgr = _moveMgrLIFO;
        _activeMoveMgr->resetTo(*_solutions[initSoln]);

        if (_params.verb.getForMajStats() > 5) {
                cout << "Min Areas  [" << _problem.getMinCapacities()[0][0] << " ";
                cout << _problem.getMinCapacities()[1][0] << "]" << endl;
                cout << "Init Areas [" << _solutions[initSoln]->partArea[0][0] << " ";
                cout << _solutions[initSoln]->partArea[1][0] << "]" << endl;
                cout << "Max Areas  [" << _problem.getMaxCapacities()[0][0] << " ";
                cout << _problem.getMaxCapacities()[1][0] << "]" << endl;
        }

        double oldCost = _activeMoveMgr->getCost();
        //  double firstCost 	= _activeMoveMgr->getCost();
        PartitioningSolution oldPart = *_solutions[initSoln];
        _bestPassCost = _activeMoveMgr->getCost();
        unsigned useClip = _params.useClip;
        bool done = false;

        // cout<<"IN doONE FM : COST AFTER RELAXATION "<<oldCost<<endl;

        unsigned passNumber = 0;
        unsigned maxNumPasses = (_params.maxNumPasses ? _params.maxNumPasses : UINT_MAX);

        _switchBox->reinitialize();

        while (!done && passNumber < maxNumPasses) {
                if (_switchBox->quitNow()) {
                        done = true;
                        break;
                }

                if (_switchBox->swapMMs()) {
                        swapMoveManagers();
                        _activeMoveMgr->resetTo(*_solutions[initSoln]);
                }

                oldCost = _bestPassCost;

                if (_params.verb.getForActions() > 3) {
                        if (_switchBox->isSecondMMinUse()) {
                                if (useClip)
                                        cout << "Pass " << setw(3) << passNumber << " (Clip)";
                                else
                                        cout << "Pass " << setw(3) << passNumber << " (LIFO2)";
                        } else
                                cout << "Pass " << setw(3) << passNumber << " (LIFO)";
                }

                _totalMovesMade += doOneFMPass(useClip && _switchBox->isSecondMMinUse());

                if ((oldCost - _bestPassCost) <= oldCost * _params.minPassImprovement * 0.01)
                        _switchBox->passFailed();
                else
                        _switchBox->passImprovedCost();

                if (_params.verb.getForMajStats() > 3) {
                        cout << "  cost: " << setw(5) << _bestPassCost << " [";
                        for (unsigned k = 0; k < _problem.getNumPartitions(); k++) cout << setw(10) << _bestPassAreas[k] << " ";
                        cout << "]\n";
                }
                passNumber++;
        }
        _numPasses += passNumber;

        PartitioningSolution& soln = *_solutions[initSoln];

        // cout<<"IN doONE FM : COST IN END "<<_bestPassCost<<endl;

        soln.cost = static_cast<unsigned>(rint(_bestPassCost));
        soln.violation = _bestPassViolation;
        soln.imbalance = _bestPassImbalance;

        abkassert(soln.partArea.size() == _bestPassAreas.size(), "size mismatch for soln areas");

        for (unsigned k = 0; k < _problem.getNumPartitions(); k++) soln.partArea[k][0] = _bestPassAreas[k];

        moveTimer.stop();
        double uTime = moveTimer.getUserTime();
        _totalTime += uTime;
        if (_params.verb.getForMajStats() > 5) {
                cout << " Made " << _totalMovesMade << " in " << uTime << " secs.  ";
                cout << (double)_totalMovesMade / uTime << " moves per second" << endl;
        }

#ifdef ABKDEBUG
        for (unsigned nId = 0; nId < _problem.getHGraph().getNumNodes(); nId++) abkassert(soln.part[nId].numberPartitions() == 1, "finished partitioning w. node in <> 1 partition");
#endif
}

unsigned FMPartitioner::doOneFMPass(bool useClip) {
        Timer passInitTime;
        Timer passTotalTime;

        _activeMoveMgr->reinitialize();  // we still do 1 of these / pass, but,
        // at the begining of the pass the GainCo &
        // move log are already empty, so it's faster

        _activeMoveMgr->computeGains();
        if (useClip) _activeMoveMgr->setupClip();
        MemUsage m;
        _peakMemUsage = max(_peakMemUsage, m.getPeakMem());

        _bestPassCost = _activeMoveMgr->getCost();
        _bestPassImbalance = _activeMoveMgr->getImbalance();
        _bestPassViolation = _activeMoveMgr->getViolation();
        _bestPassAreas = _activeMoveMgr->getAreas();

        if (_params.verb.getForMajStats() > 5) cout << "Start of the pass cost : " << _bestPassCost << endl;

        unsigned bestMoveNum = 0;
        unsigned movesMade = 0;
        unsigned moveLimit = UINT_MAX;
        if (_params.maxNumMoves != 0 && _params.useEarlyStop) moveLimit = _params.maxNumMoves;

        double maxHillHeight = _params.maxHillHeightFactor;
        if (maxHillHeight == 0) maxHillHeight = 1000;

        unsigned numModules = _problem.getHGraph().getNumNodes();
        unsigned maxConsecNonImprovingMoves = static_cast<unsigned>(floor(0.01 * _params.maxHillWidth * numModules));
        unsigned consecNonImprovingMoves = 0;

        passInitTime.stop();
        Timer passMovesTime;

        if (!_params.printHillStats) {
                if (!_params.useEarlyStop || (maxHillHeight >= 99 && _params.maxHillWidth >= 100))
                        while (movesMade < moveLimit && _activeMoveMgr->pickMoveApplyIt()) {
                                movesMade++;
                                double curCost = _activeMoveMgr->getCost();
                                double curViolation = _activeMoveMgr->getViolation();

                                if (curViolation < _bestPassViolation)  // a more legal
                                                                        // soln..take it
                                {
                                        _bestPassCost = curCost;
                                        bestMoveNum = movesMade;
                                        _bestPassImbalance = _activeMoveMgr->getImbalance();
                                        _bestPassViolation = curViolation;
                                        _bestPassAreas = _activeMoveMgr->getAreas();
                                } else if (curViolation == _bestPassViolation) {
                                        if (curCost < _bestPassCost) {
                                                _bestPassCost = curCost;
                                                bestMoveNum = movesMade;
                                                _bestPassImbalance = _activeMoveMgr->getImbalance();
                                                _bestPassViolation = _activeMoveMgr->getViolation();
                                                _bestPassAreas = _activeMoveMgr->getAreas();
                                        } else if (curCost == _bestPassCost) {
                                                double newImbalance = _activeMoveMgr->getImbalance();
                                                if (newImbalance < _bestPassImbalance) {
                                                        bestMoveNum = movesMade;
                                                        _bestPassImbalance = newImbalance;
                                                        _bestPassViolation = _activeMoveMgr->getViolation();
                                                        _bestPassAreas = _activeMoveMgr->getAreas();
                                                }
                                        }
                                }
                        }
                else
                        while (movesMade < moveLimit && _activeMoveMgr->pickMoveApplyIt()) {
                                movesMade++;
                                double curCost = _activeMoveMgr->getCost();
                                double curViolation = _activeMoveMgr->getViolation();

                                if (curViolation < _bestPassViolation)  // a more legal
                                                                        // soln..take it
                                {
                                        _bestPassCost = curCost;
                                        bestMoveNum = movesMade;
                                        _bestPassImbalance = _activeMoveMgr->getImbalance();
                                        _bestPassViolation = curViolation;
                                        _bestPassAreas = _activeMoveMgr->getAreas();
                                        consecNonImprovingMoves = 0;
                                } else if (curViolation == _bestPassViolation) {
                                        if (curCost < _bestPassCost) {
                                                _bestPassCost = curCost;
                                                bestMoveNum = movesMade;
                                                _bestPassImbalance = _activeMoveMgr->getImbalance();
                                                _bestPassViolation = _activeMoveMgr->getViolation();
                                                _bestPassAreas = _activeMoveMgr->getAreas();
                                                consecNonImprovingMoves = 0;
                                        } else if (curCost == _bestPassCost) {
                                                double newImbalance = _activeMoveMgr->getImbalance();
                                                if (newImbalance < _bestPassImbalance) {
                                                        bestMoveNum = movesMade;
                                                        _bestPassImbalance = newImbalance;
                                                        _bestPassViolation = _activeMoveMgr->getViolation();
                                                        _bestPassAreas = _activeMoveMgr->getAreas();
                                                        consecNonImprovingMoves = 0;
                                                } else
                                                        consecNonImprovingMoves++;
                                        } else {
                                                if (consecNonImprovingMoves++ >= maxConsecNonImprovingMoves || curCost > _bestPassCost * maxHillHeight) break;
                                        }
                                }
                        }
        } else  // track hill heights
        {
                _maxHillHeightSeenThisPass = 1;
                _maxHillWidthSeenThisPass = 0;
                while (movesMade < moveLimit && _activeMoveMgr->pickMoveApplyIt()) {
                        movesMade++;
                        double curCost = _activeMoveMgr->getCost();
                        double curViolation = _activeMoveMgr->getViolation();

                        if (curViolation < _bestPassViolation)  // a more legal soln..take it
                        {
                                _bestPassCost = curCost;
                                bestMoveNum = movesMade;
                                _bestPassImbalance = _activeMoveMgr->getImbalance();
                                _bestPassViolation = curViolation;
                                _bestPassAreas = _activeMoveMgr->getAreas();
                        } else if (curViolation == _bestPassViolation) {
                                if (curCost < _bestPassCost) {
                                        _bestPassCost = curCost;
                                        bestMoveNum = movesMade;
                                        _bestPassImbalance = _activeMoveMgr->getImbalance();

                                        if (_maxHillHeightSeenThisPass > _maxGoodHillHeightSeen) _maxGoodHillHeightSeen = _maxHillHeightSeenThisPass;
                                        if (_maxHillWidthSeenThisPass > _maxGoodHillWidthSeen) _maxGoodHillWidthSeen = _maxHillWidthSeenThisPass;

                                        consecNonImprovingMoves = 0;
                                } else {
                                        if (curCost == _bestPassCost) {
                                                double newImbalance = _activeMoveMgr->getImbalance();
                                                if (newImbalance < _bestPassImbalance) {
                                                        _bestPassCost = curCost;
                                                        bestMoveNum = movesMade;
                                                        _bestPassImbalance = newImbalance;
                                                        _bestPassViolation = _activeMoveMgr->getViolation();
                                                        _bestPassAreas = _activeMoveMgr->getAreas();
                                                        consecNonImprovingMoves = 0;
                                                } else
                                                        consecNonImprovingMoves++;
                                        } else
                                                consecNonImprovingMoves++;

                                        double curHillHeight = curCost / _bestPassCost;
                                        double curHillWidth = consecNonImprovingMoves * (100.0 / numModules);
                                        if (curHillHeight > _maxHillHeightSeenThisPass) _maxHillHeightSeenThisPass = curHillHeight;
                                        if (curHillWidth > _maxHillWidthSeenThisPass) _maxHillWidthSeenThisPass = curHillWidth;
                                        if (curHillHeight > maxHillHeight || consecNonImprovingMoves > maxConsecNonImprovingMoves) break;
                                }
                        }
                }
                if (_maxHillHeightSeenThisPass > _maxHillHeightSeen) _maxHillHeightSeen = _maxHillHeightSeenThisPass;
                if (_maxHillWidthSeenThisPass > _maxHillWidthSeen) _maxHillWidthSeen = _maxHillWidthSeenThisPass;
        }

        if (_params.verb.getForMajStats() > 5) {
                cout << " best Cost was :" << _bestPassCost << " at move " << bestMoveNum << " out of " << movesMade << " total moves" << endl;
                cout << "undoing " << movesMade - bestMoveNum << endl;
        }

        passMovesTime.stop();
        Timer passUndoTime;
        if (_params.saveMoveLog)
                //   if (passNumber < maxNumPasses &&
                //       (oldCost-_bestPassCost) <=
                // oldCost*_params.minPassImprovement*0.01)
                _moveLog = _activeMoveMgr->getMoveLog();
        _activeMoveMgr->undo(movesMade - bestMoveNum);
        passUndoTime.stop();

        passTotalTime.stop();

        if (_params.verb.getForMajStats() > 4) {
                cout << "Setup: " << passInitTime.getUserTime() << " Moves: " << passMovesTime.getUserTime() << " Undo: " << passUndoTime.getUserTime() << " Total Pass Time: " << passTotalTime.getUserTime() << endl;
        }

        return movesMade;
}

void FMPartitioner::doOneFMRelaxedTol(unsigned initSoln, unsigned relTolPasses, vector<double>& maxCellSize) {
        Timer moveTimer;

        //  Partitioning& curPart = const_cast<Partitioning&>
        //                              (_solutions[initSoln]->part);

        //    _moveMgrLIFO->resetTo(*_solutions[initSoln]);
        //    if (_moveMgrLIFO!=_moveMgr2 && _moveMgr2)
        // _moveMgr2->resetTo(*_solutions[initSoln]);
        _activeMoveMgr = _moveMgrLIFO;
        _activeMoveMgr->resetTo(*_solutions[initSoln]);

        if (_params.verb.getForMajStats() > 5) {
                cout << "Min Areas  [" << _problem.getMinCapacities()[0][0] << " ";
                cout << _problem.getMinCapacities()[1][0] << "]" << endl;
                cout << "Init Areas [" << _solutions[initSoln]->partArea[0][0] << " ";
                cout << _solutions[initSoln]->partArea[1][0] << "]" << endl;
                cout << "Max Areas  [" << _problem.getMaxCapacities()[0][0] << " ";
                cout << _problem.getMaxCapacities()[1][0] << "]" << endl;
        }

        double oldCost = _activeMoveMgr->getCost();
        double firstCost = _activeMoveMgr->getCost();
        PartitioningSolution oldPart = *_solutions[initSoln];
        _bestPassCost = _activeMoveMgr->getCost();
        unsigned useClip = _params.useClip;
        bool done = false;

        if (_numPasses == UINT_MAX) _numPasses = 0;
        unsigned passNumber = 0;

        _switchBox->reinitialize();

        unsigned maxNumPasses = relTolPasses;

        while (!done && passNumber < maxNumPasses) {
                if ((_switchBox->quitNow() /*&& (passNumber > relTolPasses)*/)) {
                        done = true;
                        if (relTolPasses > 0) {
                                if (firstCost < _bestPassCost) {
                                        // restore to previous soln
                                        _activeMoveMgr->resetTo(oldPart);
                                }
                        }
                        break;
                }

                if (_switchBox->swapMMs()) {
                        swapMoveManagers();
                        _activeMoveMgr->resetTo(*_solutions[initSoln]);
                }

                if (relTolPasses > 0 && passNumber == 0) {
                        // cout<<"Initial % tol
                        // "<<100*(_problem.getMaxCapacities()[0][0]-_problem.getMinCapacities()[0][0])/((_problem.getTotalWeight())[0])<<"
                        // :
                        // "<<100*(_problem.getMaxCapacities()[1][0]-_problem.getMinCapacities()[1][0])/((_problem.getTotalWeight())[0])<<endl;

                        const vector<double>& currPartAreas = _activeMoveMgr->getAreas();
                        _problem.temporarilyIncreaseTolerance(maxCellSize, currPartAreas);
                        //_activeMoveMgr->reinitTolerances();
                        cout << "supposed to reinit tolerances" << endl;
                        if (_moveMgrLIFO) _moveMgrLIFO->reinitTolerances();
                        if (_moveMgr2) _moveMgr2->reinitTolerances();
                        // cout<<"New % tol
                        // "<<100*(_problem.getMaxCapacities()[0][0]-_problem.getMinCapacities()[0][0])/(_problem.getTotalWeight()[0])<<"
                        // :
                        // "<<100*(_problem.getMaxCapacities()[1][0]-_problem.getMinCapacities()[1][0])/(_problem.getTotalWeight()[0])<<endl;
                }

                oldCost = _bestPassCost;

                if (_params.verb.getForActions() > 3) {
                        if (_switchBox->isSecondMMinUse()) {
                                if (useClip)
                                        cout << "Pass " << setw(3) << passNumber << " (Clip)";
                                else
                                        cout << "Pass " << setw(3) << passNumber << " (LIFO2)";
                        } else
                                cout << "Pass " << setw(3) << passNumber << " (LIFO)";
                }

                _totalMovesMade += doOneFMPass(useClip && _switchBox->isSecondMMinUse());

                if ((oldCost - _bestPassCost) <= oldCost * _params.minPassImprovement * 0.01)
                        _switchBox->passFailed();
                else
                        _switchBox->passImprovedCost();

                if (_params.verb.getForMajStats() > 3) {
                        cout << "  cost: " << setw(5) << _bestPassCost << " [";
                        for (unsigned k = 0; k < _problem.getNumPartitions(); k++) cout << setw(10) << _bestPassAreas[k] << " ";
                        cout << "]\n";
                }
                passNumber++;

                if (relTolPasses > 0 && passNumber == relTolPasses) {
                        _problem.revertTolerance();
                        //_activeMoveMgr->reinitTolerances();
                        cout << "supposed to reinit tolerances" << endl;
                        if (_moveMgrLIFO) _moveMgrLIFO->reinitTolerances();
                        if (_moveMgr2) _moveMgr2->reinitTolerances();
                }

                if (passNumber >= maxNumPasses && relTolPasses > 0) {
                        if (firstCost < _bestPassCost) {
                                // restore to previous soln
                                _activeMoveMgr->resetTo(oldPart);
                        }
                }
        }
        _numPasses += passNumber;
        // cout<<"Debug : reverting tolerance"<<endl;
        if (relTolPasses > 0) {
                _problem.revertTolerance();
                //_activeMoveMgr->reinitTolerances();
                cout << "supposed to reinit tolerances" << endl;
                if (_moveMgrLIFO) _moveMgrLIFO->reinitTolerances();
                if (_moveMgr2) _moveMgr2->reinitTolerances();
        }

        PartitioningSolution& soln = *_solutions[initSoln];

        soln.cost = static_cast<unsigned>(rint(_bestPassCost));
        soln.violation = _bestPassViolation;
        soln.imbalance = _bestPassImbalance;

        abkassert(soln.partArea.size() == _bestPassAreas.size(), "size mismatch for soln areas");

        for (unsigned k = 0; k < _problem.getNumPartitions(); k++) soln.partArea[k][0] = _bestPassAreas[k];

        moveTimer.stop();
        double uTime = moveTimer.getUserTime();
        //_totalTime += uTime;
        if (_params.verb.getForMajStats() > 5) {
                cout << " Made " << _totalMovesMade << " in " << uTime << " secs.  ";
                cout << (double)_totalMovesMade / uTime << " moves per second" << endl;
        }
}
