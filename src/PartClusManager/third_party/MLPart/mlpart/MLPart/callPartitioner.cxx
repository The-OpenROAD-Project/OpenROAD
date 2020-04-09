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

//  Created by Igor Markov April 14, 1998 from baseMLPart.cxx

// CHANGES

// 980520 ilm a series of updates to the new partitioning infrastructure
// 980912 aec version4.0  uses ClusteredHGraph
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "baseMLPart.h"
#include "ClusteredHGraph/clustHGraph.h"
#include "Partitioning/partitioning.h"
#include "Partitioners/partitioners.h"
#include "FMPart/fmPart.h"
#include "FMPart/fmPartPlus.h"
#include "SmallPart/bbPart.h"
#include "Partitioners/aGreedPart.h"
#include "HGraph/subHGraph.h"
#include <new>

using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::max;
using uofm::vector;

void BaseMLPart::_callPartitionerWoML(PartitioningProblem& problem) {
        problem.propagateTerminals(_params.partFuzziness);

        PartitionerParams par(_params);

        if (_partitioner) {
                delete _partitioner;
                _partitioner = NULL;
        }
        par.verb.setForActions(par.verb.getForActions() / 10);
        par.verb.setForSysRes(par.verb.getForSysRes() / 10);
        par.verb.setForMajStats(par.verb.getForMajStats() / 10);

        par.useEarlyStop = false;
        par.maxHillWidth = 100.0;
        par.maxHillHeightFactor = 0.0;
        par.maxNumPasses = 0;
        par.minPassImprovement = 0.0;

        if (par.moveMan == MoveManagerType::FMwCutLineRef) par.moveMan = MoveManagerType::FM;

        const HGraphFixed& hgraph = problem.getHGraph();
        unsigned numWeights = hgraph.getNumWeights();

        vector<double> maxNodeWeights(numWeights, 0);
        vector<double> totNodeWeights(numWeights, 0);

        unsigned i = _problem.getNumPartitions();
        itHGFNodeGlobal v;
        for (v = hgraph.nodesBegin(); v != hgraph.nodesEnd(); v++)
                for (i = 0; i != numWeights; i++) {
                        double w = hgraph.getWeight((*v)->getIndex(), i);
                        if (w > maxNodeWeights[i]) maxNodeWeights[i] = w;
                        totNodeWeights[i] += w;
                }

        if (_params.verb.getForMajStats() > 3) cout << " Original tolerances (max - target)/target : ";

        switch (_params.flatPartitioner) {
                case PartitionerType::FM:
                        if (_params.useFMPartPlus)
                                _partitioner = new FMPartitionerPlus(problem, _maxMem, par);
                        else
                                _partitioner = new FMPartitioner(problem, _maxMem, par);
                        break;
                default:
                        abkfatal(0, " Unknown partitioner ");
        }

        _bestSolnNum = problem.getBestSolnNum();

        bool wannaSave = false;
        if (_params.savePartProb == Parameters::AtAllLastLevels) wannaSave = true;
        if (wannaSave) {
                char probName[53];
                static unsigned counter = 0;
                sprintf(probName, "flatFromML-%d", counter);
                problem.saveAsNodesNets(probName);
        }
        _levelsToGo--;
}

void BaseMLPart::_callPartitioner(const HGraphFixed& hgraph, const Partitioning& fixed) {
        Timer callPartTimer;

        //**setup the partitioner parameters**//

        PartitionerParams par(_params);

        if (_params.verb.getForActions() > 6) cout << "Partitioning with " << par.eval << " Objective" << endl;

        par.verb.setForActions(par.verb.getForActions() / 10);
        par.verb.setForSysRes(par.verb.getForSysRes() / 10);
        par.verb.setForMajStats(par.verb.getForMajStats() / 10);

        bool tooEarlyForCutLineRefinement = (_minCallCounter <= _levelsToGo);
        bool topLevel = (_minCallCounter == 0);
        bool secondLevel = (_minCallCounter == 1);
        bool bottomLevel = (_levelsToGo <= 1);
        // bool   bottomLevel= (hgraph.getNumNodes() ==
        // _hgraphs->getNumLeafNodes());
        bool skipSolnGen = !topLevel;

        if (par.moveMan == MoveManagerType::FMwCutLineRef && tooEarlyForCutLineRefinement) par.moveMan = MoveManagerType::FM;

        PartitioningBuffer* mainBuf = NULL, *shadowBuf = NULL, *auxBuf = NULL;
        _soln2Buffers->checkOutBuf(mainBuf, shadowBuf);

        unsigned numSol = (mainBuf->endUsedSoln() - mainBuf->beginUsedSoln());
        bool useBranchAndBound = topLevel && _params.useBBonTop;

        par.relaxedTolerancePasses = 0;
        if (_doingCycling) {
                par.doFirstLIFOPass = false;
                auxBuf = mainBuf;
                par.useClip = false;
                par.maxHillHeightFactor = 2;
                par.useEarlyStop = true;
        } else if (topLevel) {
                if (useBranchAndBound)
                        auxBuf = mainBuf;
                else {
                        // this is the most effective partitioner we have...I
                        // think!
                        par.useClip = true;
                        par.maxHillHeightFactor = 100;  // no limit
                        par.maxHillWidth = 100;         // no limit
                        par.useEarlyStop = false;
                        par.maxNumPasses = 0;  // no limit
                        par.doFirstLIFOPass = false;

                        auxBuf = new PartitioningBuffer(hgraph.getNumNodes(), numSol * _params.solnPoolOnTopLevel);

                        // by sadya. use VILE generator with randomized FM at
                        // top level
                        // for partitioning with large modules and small
                        // tolerance
                        // par.solnGen = SolnGenType::AllToOne;
                        // par.randomized = true;
                        par.allowIllegalMoves = true;
                        // par.wiggleTerms = true;

                        if (_params.seedTopLvlSoln || _params.Vcycling == Parameters::Initial) {
                                unsigned s;
                                for (s = auxBuf->beginUsedSoln(); s != auxBuf->endUsedSoln(); s++) {
                                        Partitioning& part = (*auxBuf)[s];
                                        Partitioning& srcpart = (*mainBuf)[mainBuf->beginUsedSoln()];
                                        for (unsigned clN = 0; clN != hgraph.getNumNodes(); clN++) part[clN] = srcpart[clN];
                                }
                                skipSolnGen = true;
                        }
                        if (_params.verb.getForActions()) cout << " Selecting " << numSol << " initial solution(s), from " << _params.solnPoolOnTopLevel << " random starts each" << endl;
                }
        } else if (secondLevel) {
                par.useClip = true;
                par.maxHillHeightFactor = 100;  // no limit
                par.maxHillWidth = 100;         // no limit
                par.useEarlyStop = false;
                par.maxNumPasses = 0;  // no limit
                par.doFirstLIFOPass = false;
                par.wiggleTerms = false;

                auxBuf = mainBuf;
        } else if (bottomLevel) {
                // by royj
                // par.maxNumPasses       = _params.maxNumPassesAtBottom;
                par.maxNumPasses = _params.maxPassesAfterTopLevels;
                par.maxHillHeightFactor = 100;  // no limit
                par.useClip = false;
                auxBuf = mainBuf;
                par.wiggleTerms = false;
        } else {
                // by royj
                par.maxNumPasses = _params.maxPassesAfterTopLevels;
                par.wiggleTerms = false;
                auxBuf = mainBuf;
        }

        if (_params.verb.getForMajStats() > 6) cout << "Flat Partitioner Parameters are: " << endl << par << endl;

        vector<double> dummyTols(hgraph.getNumWeights(), 0.5);
        PartitioningProblem newProblem(hgraph, *auxBuf, fixed, _problem.getPartitions(), _problem.getCapacities(), dummyTols, *_terminalBlocksPtr, *_termBlockBBoxesPtr);

        if (!_doingCycling && ((_params.useTwoPartCalls == Parameters::ALL) || (_params.useTwoPartCalls == Parameters::ALLBUTLAST && !bottomLevel) || (_params.useTwoPartCalls == Parameters::TOPONLY && topLevel))) {
                _computeNewMinAndMaxCap(hgraph, const_cast<vector<vector<double> >&>(newProblem.getMinCapacities()), const_cast<vector<vector<double> >&>(newProblem.getMaxCapacities()));

                _runPartitioner(newProblem, par, skipSolnGen);

                if (_params.verb.getForMajStats() > 1) {
                        cout << " Costs :      ";
                        if (newProblem.getCosts().size() > 7) cout << "\n";
                        cout << newProblem.getCosts();
                }

                par.useClip = false;
                par.wiggleTerms = false;

                // if(!skipSolnGen)
                skipSolnGen = true;
        }

        _resetMinAndMaxCap(newProblem);
        _runPartitioner(newProblem, par, skipSolnGen);

        _bestSolnNum = newProblem.getBestSolnNum();
        // some caution is necessary here, as the Partitioning results will be
        // indexed by HGraphFixed Node Index..not Cluster::_index.

        if (!useBranchAndBound && _params.verb.getForMajStats() > 5) {
                cout << endl;
                cout << "Num Passes Made:   " << _partitioner->getAveNumPassesMade() << endl;
                // double moveAttempts =
                // _partitioner->getAveNumMovesAttempted();
                // double movesMade    = _partitioner->getAveNumMovesMade();
        }

        bool wannaSave = false;

        switch (_params.savePartProb) {
                case Parameters::NeverSave:
                        break;
                case Parameters::AtFirstLevelOfFirst:
                        wannaSave = (_majCallCounter == 0 && topLevel);
                        break;
                case Parameters::AtAllLevelsOfFirst:
                        wannaSave = (_majCallCounter == 0);
                        break;
                case Parameters::AtLastLevelOfFirst:
                        wannaSave = (bottomLevel && _majCallCounter == 0);
                        break;
                case Parameters::AtAllLastLevels:
                        wannaSave = (bottomLevel);
                        break;
                case Parameters::AtAllLevels:
                        wannaSave = true;
                        break;
                default:
                        abkfatal(0, " Unknown PartProblem save schedule ");
        }

        if (wannaSave) {
                char probName[53];
                sprintf(probName, "savedByML%d-%d", _majCallCounter, _minCallCounter);
                newProblem.saveAsNodesNets(probName);
        }

        if (_params.verb.getForMajStats() > 1) {
                cout << " Costs :      ";
                if (newProblem.getCosts().size() > 7) cout << "\n";
                cout << newProblem.getCosts();
        }
        if (_params.verb.getForMajStats() > 2) {
                cout << " Imbalances : ";
                if (newProblem.getImbalances().size() > 7) cout << "\n";
                cout << newProblem.getImbalances();

                cout << " Violations : ";
                if (newProblem.getViolations().size() > 7) cout << "\n";
                cout << newProblem.getViolations();
        }

        if (bottomLevel) {
                _minCallCounter = 0;
                _majCallCounter++;
        } else
                _minCallCounter++;

        if (topLevel && !_doingCycling && !useBranchAndBound) {
                const vector<double>& costs = newProblem.getCosts();
                Permutation sortAscCosts(costs);
                Permutation sortAscCosts_1;
                sortAscCosts.getInverse(sortAscCosts_1);
                {
                        unsigned solN = sortAscCosts_1[0];
                        unsigned actualNum = mainBuf->beginUsedSoln();

                        std::copy((*auxBuf)[solN].begin(), (*auxBuf)[solN].begin() + auxBuf->getNumModulesUsed(), (*mainBuf)[actualNum].begin());
                }
                const vector<double>& imbals = newProblem.getImbalances();
                unsigned actualNum = 1 + mainBuf->beginUsedSoln();
                for (unsigned m = 1; actualNum < mainBuf->endUsedSoln() && m < sortAscCosts_1.getSize(); m++) {
                        unsigned solN, prevSolN;
                        bool foundDifferent = false;
                        do {
                                solN = sortAscCosts_1[m];
                                prevSolN = sortAscCosts_1[m - 1];
                                if (imbals[solN] == imbals[prevSolN] && costs[solN] == costs[prevSolN]) {
                                        if (++m >= sortAscCosts_1.getSize()) break;
                                } else
                                        foundDifferent = true;
                        } while (!foundDifferent);

                        if (foundDifferent) {
                                std::copy((*auxBuf)[solN].begin(), (*auxBuf)[solN].begin() + auxBuf->getNumModulesUsed(), (*mainBuf)[actualNum++].begin());
                        }
                }
                if (actualNum != mainBuf->endUsedSoln()) {
                        cerr << "Could not generate enough initial solutions:" << "  was looking for " << mainBuf->endUsedSoln() - mainBuf->beginUsedSoln();
                        cerr << "  and found only  " << actualNum - mainBuf->beginUsedSoln() << endl;
                        mainBuf->setEndUsedSoln(actualNum);
                        shadowBuf->setEndUsedSoln(actualNum);
                }
                delete auxBuf;
        }

        _soln2Buffers->checkInBuf(mainBuf, shadowBuf);

        if (_params.verb.getForMajStats() > 2 || _params.verb.getForActions() > 3 || _params.verb.getForSysRes() > 3) cout << endl;

        _levelsToGo--;

        callPartTimer.stop();
        _callPartTime += callPartTimer.getUserTime();
}

void BaseMLPart::_computeNewMinAndMaxCap(const HGraphFixed& hgraph, vector<vector<double> >& newMinCaps, vector<vector<double> >& newMaxCaps) {
        // find the total node weights for each weight
        unsigned numWeights = hgraph.getNumWeights();
        unsigned numParts = _problem.getNumPartitions();

        const vector<vector<double> >& caps = _problem.getCapacities();
        const vector<vector<double> >& minCaps = _problem.getMinCapacities();
        const vector<vector<double> >& maxCaps = _problem.getMaxCapacities();

        vector<double> totNodeWeights(numWeights, 0);

        for (itHGFNodeGlobal v = hgraph.nodesBegin(); v != hgraph.nodesEnd(); v++)
                for (unsigned i = 0; i != numWeights; i++) totNodeWeights[i] += hgraph.getWeight((*v)->getIndex(), i);

        for (unsigned w = 0; w < numWeights; w++) {
                const Permutation& nodesByWeightPerm = hgraph.getNodesSortedByWeights();
                unsigned numLargeNodesToUse = 5;
                double newTol = 0;
                double wtMultiple = _params.toleranceMultiple;
                unsigned numNodes = hgraph.getNumNodes();

                for (unsigned i = 1; i <= numLargeNodesToUse; i++) {
                        unsigned largeNodeId = nodesByWeightPerm[numNodes - i];
                        const HGFNode& largeNode = hgraph.getNodeByIdx(largeNodeId);
                        newTol += hgraph.getWeight(largeNode.getIndex(), w) * wtMultiple;
                        wtMultiple *= _params.toleranceAlpha;
                }

                newTol /= totNodeWeights[w];

                if (newTol > 0.9) newTol = 0.9;

                for (unsigned p = 0; p < numParts; p++) {
                        newMaxCaps[p][w] = max(maxCaps[p][w], caps[p][w] * (1 + newTol));
                        newMinCaps[p][w] = min(minCaps[p][w], caps[p][w] * (1 - newTol));
                }
        }
}

void BaseMLPart::_runPartitioner(PartitioningProblem& newProblem, const PartitionerParams& par, bool skipSolnGen) {
        if (_partitioner) {
                delete _partitioner;
                _partitioner = NULL;
        }

        bool topLevel = (_minCallCounter == 0);
        bool useBranchAndBound = topLevel && _params.useBBonTop;

        if (useBranchAndBound) {  // if there's more than one slot in sdoln
                                  // buffer, BBPart will abkfatal
                BBPart::Parameters bbParams;
                bbParams.verb = par.verb;
                BBPart bbPart(newProblem, _bbBitBoards, bbParams);
        } else
                switch (_params.flatPartitioner) {
                        case PartitionerType::FM:
                                if (_params.useFMPartPlus)
                                        _partitioner = new FMPartitionerPlus(newProblem, _maxMem, par, skipSolnGen);
                                else
                                        _partitioner = new FMPartitioner(newProblem, _maxMem, par, skipSolnGen);
                                break;
                        case PartitionerType::AGreed: {
                                _partitioner = new AGreedPartitioner(newProblem, par, skipSolnGen);
                                break;
                        }
                        default:
                                abkfatal(0, " Unknown partitioner ");
                }
}

void BaseMLPart::_resetMinAndMaxCap(PartitioningProblem& newProblem) {
        const vector<vector<double> >& minCaps = _problem.getMinCapacities();
        const vector<vector<double> >& maxCaps = _problem.getMaxCapacities();

        vector<vector<double> >& newMin = const_cast<vector<vector<double> >&>(newProblem.getMinCapacities());

        vector<vector<double> >& newMax = const_cast<vector<vector<double> >&>(newProblem.getMaxCapacities());

        for (unsigned v = 0; v < newMin.size(); v++) {
                std::copy(minCaps[v].begin(), minCaps[v].end(), newMin[v].begin());
                std::copy(maxCaps[v].begin(), maxCaps[v].end(), newMax[v].begin());
        }
}
