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

// Created by Igor Markov, 970413 from mlPart.cxx

// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ClusteredHGraph/clustHGraph.h"
#include "Partitioning/partitioning.h"
#include "HGraph/hgFixed.h"
#include "mlPart.h"
#include <set>

using std::set;
using std::cout;
using std::endl;

bool MLPart::_doMultiLevel()  // returns true if this tree produced
                              // valid solns, false if not (currently,
                              // solns are only not valid if they were
                              // pruned)
{
        //  unsigned nTerminals = _hgraphs->getNumTerminals();
        _levelsToGo = _hgraphs->getNumLevels();

        if (_params.verb.getForMajStats() > 3) {
                cout << " Need " << _levelsToGo << " steps to cover " << _hgraphs->getNumLeafNodes() << " starting at " << _hgraphs->getHGraph(_levelsToGo - 1).getNumNodes() << endl;
        }

        const HGraphFixed& hgraphTop = _hgraphs->getHGraph(_levelsToGo - 1);

        PartitioningBuffer* mainBuf = NULL, *shadowBuf = NULL;
        const Partitioning& topLvlPart = _hgraphs->getTopLevelPart();

        _soln2Buffers->checkOutBuf(mainBuf, shadowBuf);
        unsigned solN;
        for (solN = mainBuf->beginUsedSoln(); solN != mainBuf->endUsedSoln(); solN++) {
                Partitioning& part = (*mainBuf)[solN];
                for (unsigned clN = 0; clN != hgraphTop.getNumNodes(); clN++) part[clN] = topLvlPart[clN];
        }
        _soln2Buffers->checkInBuf(mainBuf, shadowBuf);

        //  Timer callPartTimer;

        _callPartitioner(hgraphTop, _hgraphs->getFixedConst(_levelsToGo - 1));
        // delete the hgraph we just used, because it isn't needed any longer
        // its at _levelsToGo in the hierarchy (not _levelsToGo-1 because
        // _callPartitioner already decremented that for us)
        _hgraphs->destroyHGraph(_levelsToGo);

        //  callPartTimer.stop();
        //  _callPartTime += callPartTimer.getUserTime();

        // used for pruning later trees based on previous tree's results
        bool checkedMidPoint = false;
        unsigned currentMidPoint = UINT_MAX;  // the midpoint cost of the
        // best soln on the current tree.
        bool continueML = true;

        //  this loop is *usually* traversed at least once since we are in ML!
        //  It isn't traversed only when the level growth factor is rounded to
        // one
        for (unsigned level = 2; _levelsToGo > 0 && continueML; level++) {
                const HGraphFixed& hgraph = _hgraphs->getHGraph(_levelsToGo - 1);

                mainBuf = NULL;
                shadowBuf = NULL;
                _soln2Buffers->checkOutBuf(mainBuf, shadowBuf);

                unsigned soln = _soln2Buffers->beginUsedSoln(), solnEnd = _soln2Buffers->endUsedSoln();
                for (; soln != solnEnd; soln++) {
                        _hgraphs->mapPartitionings((*mainBuf)[soln], (*shadowBuf)[soln], _levelsToGo - 1);

                        if (_params.verb.getForMajStats() > 10) {
                                for (unsigned nId = 0; nId < hgraph.getNumNodes(); nId++)
                                        if ((*shadowBuf)[soln][nId].numberPartitions() != 1) {
                                                cout << " soln =" << soln << endl;
                                                cout << "ERROR:  mainBuf[" << nId << "] " << (*mainBuf)[soln][nId] << endl;
                                                cout << "        shadBuf[" << nId << "] " << (*shadowBuf)[soln][nId] << endl;
                                        }
                        }
                }

                _soln2Buffers->checkInBuf(mainBuf, shadowBuf);
                _soln2Buffers->swapBuf();

                unsigned numNodes = hgraph.getNumNodes();

                _callPartitioner(hgraph, _hgraphs->getFixedConst(_levelsToGo - 1));
                // delete the hgraph we just used, because it isn't needed any
                // longer
                // its at _levelsToGo in the hierarchy (not _levelsToGo-1
                // because
                // _callPartitioner already decremented that for us)
                _hgraphs->destroyHGraph(_levelsToGo);

                if (!checkedMidPoint && numNodes >= _midWayPoint && !_doingCycling && _levelsToGo > 0)  // don't bother if
                                                                                                        // this was the bottom
                {
                        checkedMidPoint = true;
                        currentMidPoint = _partitioner->getBestSolnCost();
                        unsigned pruningLimit = static_cast<unsigned>(ceil((1.0 + _params.pruningPercent / 100.0) * (double)_bestCostsMidWayCost));

                        if (_params.verb.getForMajStats() > 1 && _params.pruningPercent < 1000) {
                                cout << "Current mid-point is " << currentMidPoint << endl;
                                cout << "Best mid-point is " << _bestCostsMidWayCost << endl;
                                cout << "Pruning limit is " << pruningLimit << endl;
                        }

                        if (currentMidPoint > pruningLimit) {
                                cout << "PRUNED" << endl;
                                _minCallCounter = 0;
                                _majCallCounter++;
                                continueML = false;
                        }
                }
        }

        if (_partitioner->getBestSolnCost() < _bestCostSoFar) {
                _bestCostSoFar = _partitioner->getBestSolnCost();
                _bestCostsMidWayCost = currentMidPoint;
        }

        if (_params.verb.getForMajStats() > 1) {
                cout << "BestSoFar cost: " << _bestCostSoFar;
                if (_params.pruningPercent < 1000) cout << "  with midpoint cost: " << _bestCostsMidWayCost;
                cout << endl;
        }

        return continueML;  // continueML will still be true if the
                            // starts were not pruned..and thus, the solns
                            // are valid
}
