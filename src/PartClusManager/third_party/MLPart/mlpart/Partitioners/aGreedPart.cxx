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

#include "PartLegality/1balanceGen.h"
#include "PartEvals/partEvals.h"
#include "aGreedMoveMan.h"
#include "aGreedPart.h"

using std::cout;
using std::endl;
using std::max;
using std::min;

AGreedPartitioner::AGreedPartitioner(PartitioningProblem& problem, const PartitionerParams& params, bool skipSolnGen) : MultiStartPartitioner(problem, params), _skipSolnGen(skipSolnGen), _numMoves(0), _totalMovesMade(0), _totalTime(0.0) {
        setMoveManager();
        runMultiStart();

        if (_params.verb.getForMajStats() > 4) {
                cout << " Made " << _totalMovesMade << " in " << _totalTime << " seconds.  ";
                cout << (double)_totalMovesMade / _totalTime << " moves per second" << endl;
        }
}

void AGreedPartitioner::doOneAGreed(unsigned initSoln) {
        Timer moveTimer;
        PartitioningSolution& soln = *_solutions[initSoln];
        Partitioning& curPart = const_cast<Partitioning&>(soln.part);
        if (!_skipSolnGen) {
                SingleBalanceGenerator sGen(_problem);
                sGen.generateSoln(curPart);
        }

        _moveMgr->resetTo(soln);

        if (_params.verb.getForMajStats() > 5) {
                cout << "Min Areas [" << _problem.getMinCapacities()[0][0] << " ";
                cout << _problem.getMinCapacities()[1][0] << "]" << endl;
                cout << "Init Areas [" << soln.partArea[0][0] << " ";
                cout << soln.partArea[1][0] << "]" << endl;
                cout << "Max Areas [" << _problem.getMaxCapacities()[0][0] << " ";
                cout << _problem.getMaxCapacities()[1][0] << "]" << endl;
        }

        if (_params.verb.getForMajStats() > 3) cout << "Initial Solution Cost " << _moveMgr->getCost() << endl;

        unsigned passNumber = 0;
        unsigned maxNumPasses = (_params.maxNumPasses ? _params.maxNumPasses : UINT_MAX);
        do {
                _numMoves = _moveMgr->doOnePass();
                _totalMovesMade += _numMoves;
                passNumber++;
        } while (_numMoves != 0 && passNumber < maxNumPasses);

        _totalMovesMade = passNumber * _problem.getHGraph().getNumNodes();

        soln.cost = _moveMgr->getCost();
        soln.violation = _moveMgr->getViolation();
        soln.imbalance = _moveMgr->getImbalance();

        unsigned k;
        for (k = 0; k < _problem.getNumPartitions(); k++) {
                soln.partArea[k][0] = _moveMgr->getAreas()[k];
                soln.partPins[k] = 0;
        }

        itHGFNodeGlobal n;
        for (n = _problem.getHGraph().nodesBegin(); n != _problem.getHGraph().nodesEnd(); n++) {
                unsigned nodesPart = soln.part[(*n)->getIndex()].lowestNumPart();
                soln.partPins[nodesPart] += (*n)->getDegree();
        }

        unsigned minPins = UINT_MAX;
        unsigned maxPins = 0;
        for (k = 0; k < soln.partPins.size(); k++) {
                maxPins = max(maxPins, soln.partPins[k]);
                minPins = min(minPins, soln.partPins[k]);
        }
        soln.pinImbalance = maxPins - minPins;

        moveTimer.stop();
        double uTime = moveTimer.getUserTime();
        _totalTime += uTime;
}

void AGreedPartitioner::setMoveManager() {
        PartEvalType eval = _params.eval;
        unsigned numPart = _problem.getNumPartitions();
        abkfatal2(numPart >= 2, " Too few partitions : ", numPart);
        if (numPart == 2) {
                switch (eval) {
                        case PartEvalType::NetCutWNetVec:
                        case PartEvalType::NetCutWConfigIds:
                                eval = PartEvalType::NetCut2way;
                                break;
                        case PartEvalType::StrayNodes:
                                eval = PartEvalType::StrayNodes2way;
                                break;
                        default:
                                abkfatal(1,
                                         "This piece of code should never be "
                                         "reached.");
                }
        } else
                abkfatal3(eval != PartEvalType::StrayNodes2way && eval != PartEvalType::NetCut2way, numPart, " partitions: too many partitions for ", eval);

        // abkfatal(_params.moveMan==MoveManagerType::AGreed,
        //         "AGreedPartitioner requested with wrong move manager ");

        // cout << " Using " << eval << " evaluator " << endl;
        switch (eval) {
                /*
                      case PartEvalType::NetCutWBits :
                        _moveMgr = new AGreedMoveManager<NetCutWBits>
                   (_problem);
                        break;
                */
                case PartEvalType::NetCutWConfigIds:
                        _moveMgr = new AGreedMoveManager<NetCutWConfigIds>(_problem);
                        break;

                case PartEvalType::NetCutWNetVec:
                        _moveMgr = new AGreedMoveManager<NetCutWNetVec>(_problem);
                        break;

                case PartEvalType::StrayNodes:
                        _moveMgr = new AGreedMoveManager<StrayNodes>(_problem);
                        break;

                case PartEvalType::BBox1Dim:
                        _moveMgr = new AGreedMoveManager<BBox1Dim>(_problem);
                        break;
                /*
                      case PartEvalType::BBox1DimWCheng :
                        _moveMgr = new AGreedMoveManager<BBox1DimWCheng>
                   (_problem);
                        break;

                      case PartEvalType::BBox2Dim :
                        _moveMgr = new AGreedMoveManager<BBox2Dim> (_problem);
                        break;

                      case PartEvalType::BBox2DimWCheng :
                        _moveMgr = new AGreedMoveManager<BBox2DimWCheng>
                   (_problem);
                        break;

                      case PartEvalType::BBox2DimWRSMT :
                        _moveMgr = new AGreedMoveManager<BBox2DimWRSMT>
                   (_problem);
                        break;

                      case PartEvalType::HBBox :
                        _moveMgr = new AGreedMoveManager<HBBox> (_problem);
                        break;

                      case PartEvalType::HBBoxWCheng :
                        _moveMgr = new AGreedMoveManager<HBBoxWCheng>
                   (_problem);
                        break;

                      case PartEvalType::HBBoxWRSMT :
                        _moveMgr = new AGreedMoveManager<HBBoxWRSMT> (_problem);
                        break;

                      case PartEvalType::HBBox0 :
                        _moveMgr = new AGreedMoveManager<HBBox0> (_problem);
                        break;

                      case PartEvalType::HBBox0wCheng :
                        _moveMgr = new AGreedMoveManager<HBBox0wCheng>
                   (_problem);
                        break;

                      case PartEvalType::HBBox0wRSMT :
                        _moveMgr = new AGreedMoveManager<HBBox0wRSMT>
                   (_problem);
                        break;
                */
                case PartEvalType::NetCut2way:
                        _moveMgr = new AGreedMoveManager<NetCut2way>(_problem);
                        break;

                case PartEvalType::StrayNodes2way:
                        _moveMgr = new AGreedMoveManager<StrayNodes2way>(_problem);
                        break;

                default:
                        abkfatal(0,
                                 " Evaluator requested unknown to PartEvalType "
                                 "class\n");
        };
        _moveMgr->randomized = _params.randomized;
}
