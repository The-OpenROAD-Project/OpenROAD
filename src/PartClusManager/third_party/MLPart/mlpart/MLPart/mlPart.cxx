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

// Created by Igor Markov, 970223

//  Reworked for standalone ML by Igor Markov,  March 30, 1998

// CHANGES

// 980330   ilm  fixed an ML bug (feeding the same problem to partitioner)
//                pointed by aec
// 980413   ilm  split into doMLdoFlat.cxx, mlAux.cxx and remainder
// 980520   ilm  a series of updates to the new partitioning infrastructure
// 980912   aec  version 4.0 - uses ClusteredHGraph
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Geoms/plGeom.h"
#include "ClusteredHGraph/clustHGraph.h"
#include "Partitioning/partitioning.h"
#include "HGraph/hgFixed.h"
#include "mlPart.h"
#include "PartEvals/univPartEval.h"
#include "HGraph/subHGraph.h"
#include "FMPart/fmPart.h"
#include <new>
#include <set>
#include <algorithm>
#include <sstream>

using std::set;
using std::min;
using std::max;
using uofm::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using uofm::vector;

bool MLPart::_go()  // returns true if solns are valid, false
                    // if they are not (ie, they were pruned)
{
        unsigned firstLevelKWay = 20 * _problem.getNumPartitions();
        if ((_params.clParams.sizeOfTop < firstLevelKWay) && (_problem.getNumPartitions() > 2)) {
                abkwarn3(0,
                         "Default 'firstLevelSize' too small for k-way, "
                         "switching to ",
                         firstLevelKWay, " \n");
                _params.clParams.sizeOfTop = firstLevelKWay;
        }
        return _doMultiLevel();
}

MLPart::MLPart(PartitioningProblem& problem, const MLPartParams& params, BBPartBitBoardContainer& bbBitBoards, MaxMem* maxMem, FillableHierarchy* fill) : BaseMLPart(problem, params, bbBitBoards, maxMem, fill), _peakMemUsage(0.) {
	abkfatal(problem.getNumPartitions() == 2,
			"More than 2-way"
			"partitioning is not supported in this distribution. For more"
			"details contact Igor Markov <imarkov@eecs.umich.edu>");

	try {
		Timer tm;
		const HGraphFixed* graph = problem.getHGraphPointer();

		unsigned numNodes = graph->getNumNodes();
		unsigned numEdges = graph->getNumEdges();
		unsigned sizeOfTop = _params.clParams.sizeOfTop;
		bool doFlat = sizeOfTop == 0 || (graph->getNumNodes() - graph->getNumTerminals()) <= sizeOfTop;

		if (numEdges <= numNodes / 2 && !doFlat) {
			cerr << "\n NumEdges must be > numNodes/2 \n "
				<< "The actual values are " << numEdges << " and " << numNodes << flush;
			//    abkfatal(0," Cannot perform clustering ");
		}
		if (doFlat) {
			if (_params.verb.getForActions()) cout << endl << "Problem too small for ML, running flat" << endl;
			_soln2Buffers = NULL;
			_callPartitionerWoML(problem);
			const PartitioningSolution& bestSol = _partitioner->getResult(_bestSolnNum);
			_bestSolnPartWeights = bestSol.partArea;
			_bestSolnPartNumNodes = bestSol.partCount;
			_figureBestCost(problem);
		} else  // do ml and/or vcycle
		{
			// allocate ML's double buffer --- an implicit parameter
			// to the call _startWithPartProblem() below
			_soln2Buffers = new PartitioningDoubleBuffer(problem.getSolnBuffers());

			// setup for multiple clusterings
			unsigned solnBegin = _soln2Buffers->beginUsedSoln(), solnEnd = _soln2Buffers->endUsedSoln();
			double totalCost = 0.0;
			unsigned totalNumLegalSolns = 0;
			double curBestCost = DBL_MAX;
			unsigned curBestSolnNum = UINT_MAX;

			if (_params.verb.getForActions() > 1 || _params.verb.getForMajStats() > 1)
				cout << "\n Multilevel partitioning on " << problem.getHGraph().getNumNodes() << " nodes "
					<< " including " << problem.getHGraph().getNumTerminals() << " terminals " << endl;

			problem.propagateTerminals();

			unsigned numClTrees;

			if (_params.Vcycling == Parameters::Initial) {
				numClTrees = totalNumLegalSolns = 1;
				curBestSolnNum = problem.getBestSolnNum();
				UniversalPartEval costComputer(PartEvalType::NetCut2wayWWeights);
				curBestCost = totalCost = costComputer.computeCost(problem, problem.getBestSoln());
			} else {
				unsigned step = _params.runsPerClTree;
				double numClTreesDbl = max(double(1), ceil((solnEnd - solnBegin) / (1.0 * step)));
				numClTrees = static_cast<unsigned>(rint(numClTreesDbl));
				vector<double> bestCosts(numClTrees, DBL_MAX);
				vector<unsigned> bestSolnNums(numClTrees, UINT_MAX);
				vector<double> aveCosts(numClTrees, DBL_MAX);
				vector<unsigned> numLegalSolns(numClTrees, UINT_MAX);

				if (_params.verb.getForActions() || _params.verb.getForMajStats()) {
					cout << " Will now create " << numClTrees << " clusterTree";
					if (numClTrees != 1) cout << "s and use each";
					cout << " to find ";
					if (step == 1)
						cout << "one solution" << endl;
					else
						cout << "(up to) " << step << " solutions" << endl;
				}

				for (unsigned clTreeNum = 0, curBegin = 0; clTreeNum != numClTrees; clTreeNum++, curBegin += step) {
					unsigned curEnd = min(curBegin + step, solnEnd);
					_soln2Buffers->setBeginUsedSoln(curBegin);
					_soln2Buffers->setEndUsedSoln(curEnd);
					if (numClTrees > 1) {
						if (_params.verb.getForActions() > 1 || _params.verb.getForMajStats() > 1) {
							cout << "  --- Tree " << clTreeNum << endl;
						}
					}

					PartitioningBuffer* mBuf, *sBuf;
					_soln2Buffers->checkOutBuf(mBuf, sBuf);

					const Partitioning& treePart = (*mBuf)[curBegin];
					Timer clTreeCreationTime;
					ClusteredHGraph* hgraphs;
					hgraphs = _makeClusterTrees(treePart, clTreeNum);
					clTreeCreationTime.stop();

					_totalClusteringTime += clTreeCreationTime.getUserTime();

					MemUsage m1;
					_peakMemUsage = max(_peakMemUsage, m1.getPeakMem());
					_maxMem->update("MLPart after clustering");

					_soln2Buffers->checkInBuf(mBuf, sBuf);

					_params.solnPoolOnTopLevel = std::min(10u, std::max(3u, hgraphs->getNumLevels()));

					Timer unClusteringTime;
					_startWithPartProblem(problem, *hgraphs);
					delete hgraphs;
					MemUsage m2;
					_peakMemUsage = max(_peakMemUsage, m2.getPeakMem());
					_maxMem->update("MLPart after unclustering");

					unClusteringTime.stop();
					_totalUnClusteringTime += unClusteringTime.getUserTime();
					bestCosts[clTreeNum] = _bestCost;
					bestSolnNums[clTreeNum] = _bestSolnNum;
					aveCosts[clTreeNum] = _aveCost;
					numLegalSolns[clTreeNum] = _numLegalSolns;

					totalNumLegalSolns += _numLegalSolns;
					totalCost += _numLegalSolns * _aveCost;

					if (_bestCost < curBestCost) {
						curBestCost = _bestCost;
						curBestSolnNum = _bestSolnNum;
						const PartitioningSolution& bestSol = _partitioner->getBestResult();
						_bestSolnPartWeights = bestSol.partArea;
						_bestSolnPartNumNodes = bestSol.partCount;
					}

					delete _partitioner;
					_partitioner = NULL;

				}  // for loop
			}

			if (_params.verb.getForActions() > 1) {
				cout << "Threshold for starting VCycling is " << _params.lastVCycleStartThreshold << endl << "Best cost is " << curBestCost << endl;
			}
			if (_params.Vcycling != Parameters::NoVcycles && (curBestCost <= _params.lastVCycleStartThreshold)) {
				totalCost -= curBestCost;
				_doLastVCycle(curBestCost, curBestSolnNum);
				totalCost += curBestCost;
			} else {
				if (_params.verb.getForActions() > 1) {
					if (_params.Vcycling == Parameters::NoVcycles) {
						cout << "VCycling skipped due "
							"to NoVcycles option" << endl;
					} else {
						cout << "VCycling skipped "
							"because solution "
							"quality worse than "
							"threshold" << endl;
					}
				}
			}
			_soln2Buffers->setBeginUsedSoln(solnBegin);
			_soln2Buffers->setEndUsedSoln(solnEnd);

			_bestCost = curBestCost;
			_problem.setBestSolnNum(_bestSolnNum = curBestSolnNum);
			_numLegalSolns = totalNumLegalSolns;
			_aveCost = totalCost / totalNumLegalSolns;

			_aveClusteringTime = _totalClusteringTime / numClTrees;
			_aveUnClusteringTime = _totalUnClusteringTime / numClTrees;
			_callPartTime /= numClTrees;

			if (_params.verb.getForMajStats() > 10) {
				UniversalPartEval lastCheck;
				unsigned checkedBestCost = lastCheck.computeCost(_problem, _problem.getBestSoln());
				cout << endl << endl << "Final Checked Best Cost is " << checkedBestCost << endl;
			}

		}  // do ml and/or vcycling
		tm.stop();

		if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
			unsigned numStartsDone;
			if (_soln2Buffers == NULL)  // was running flat
			{
				const PartitioningBuffer& _solnBuffer = problem.getSolnBuffers();
				numStartsDone = _solnBuffer.endUsedSoln() - _solnBuffer.beginUsedSoln();
			} else {
				numStartsDone = _soln2Buffers->endUsedSoln() - _soln2Buffers->beginUsedSoln();
			}

			cout << " Best Cost in one ML (" << numStartsDone << " starts):" << _bestCost << "   average :    " << _aveCost << endl;
			cout << "  Part Areas: " << getPartitionArea(0) << "  (" << getPartitionNumNodes(0) << " nodes with terms) "
				<< "  /  " << getPartitionArea(1) << "  (" << getPartitionNumNodes(1) << " nodes with terms) " << endl;

			{
				unsigned pinB0 = 0, pinB1 = 0;
				const Partitioning& part = _problem.getBestSoln();
				const HGraphFixed& hg = _problem.getHGraph();
				for (unsigned n = 0; n != hg.getNumNodes(); n++) {
					unsigned deg = hg.getNodeByIdx(n).getDegree();
					if (part[n].isInPart(0))
						pinB0 += deg;
					else
						pinB1 += deg;
				}
				cout << " Pin balances :   " << pinB0 << " / " << pinB1 << endl;
			}

			cout << " One ML took           " << tm << endl;
			if (_soln2Buffers != NULL)  // non-trivial ML
			{
				if (_params.verb.getForMajStats() > 1) {

					cout << " Ave Clustering Time Per Tree "
						"  : " << _aveClusteringTime << endl;
					cout << " Ave UnClustering Time Per "
						"Tree : " << _aveUnClusteringTime << endl;
					cout << " Ave CallPartitioner Time Per "
						"Tree: " << _callPartTime << endl;
				}
			}
		}
		_userTime = tm.getUserTime();
	}
	catch (const std::bad_alloc& e) {
		cout << endl << "MLPart has run out of memory." << endl;
		throw(e);
	}
}


MLPart::MLPart(PartitioningProblem& problem, const MLPartParams& params, BBPartBitBoardContainer& bbBitBoards, MaxMem* maxMem, int* part, int nLevels, FillableHierarchy* fill) : BaseMLPart(problem, params, bbBitBoards, maxMem, fill), _peakMemUsage(0.) {
        abkfatal(problem.getNumPartitions() == 2,
                 "More than 2-way"
                 "partitioning is not supported in this distribution. For more"
                 "details contact Igor Markov <imarkov@eecs.umich.edu>");

                Timer tm;
                const HGraphFixed* graph = problem.getHGraphPointer();

                unsigned numNodes = graph->getNumNodes();
                unsigned numEdges = graph->getNumEdges();
                unsigned sizeOfTop = _params.clParams.sizeOfTop;
                bool doFlat = sizeOfTop == 0 || (graph->getNumNodes() - graph->getNumTerminals()) <= sizeOfTop;

                if (numEdges <= numNodes / 2 && !doFlat) {
                        cerr << "\n NumEdges must be > numNodes/2 \n "
                             << "The actual values are " << numEdges << " and " << numNodes << flush;
                        //    abkfatal(0," Cannot perform clustering ");
                }

		_soln2Buffers = new PartitioningDoubleBuffer(problem.getSolnBuffers());

		unsigned solnBegin = _soln2Buffers->beginUsedSoln(), solnEnd = _soln2Buffers->endUsedSoln();

		// these allow to find "cre`me de la cre`me" of all
		// solutions
		double totalCost = 0.0;
		unsigned totalNumLegalSolns = 0;
		double curBestCost = DBL_MAX;
		unsigned curBestSolnNum = UINT_MAX;

		problem.propagateTerminals();

		unsigned curBegin = 0;
		unsigned clTreeNum = 0;
		unsigned numClTrees;

		unsigned step = _params.runsPerClTree;
		double numClTreesDbl = max(double(1), ceil((solnEnd - solnBegin) / (1.0 * step)));
		numClTrees = static_cast<unsigned>(rint(numClTreesDbl));


		// narrow the buffers
		unsigned curEnd = min(curBegin + step, solnEnd);
		_soln2Buffers->setBeginUsedSoln(curBegin);
		_soln2Buffers->setEndUsedSoln(curEnd);

		PartitioningBuffer* mBuf, *sBuf;
		_soln2Buffers->checkOutBuf(mBuf, sBuf);

		const Partitioning& treePart = (*mBuf)[curBegin];

		ClusteredHGraph* hgraphs;
		// TODO: if(ml and not vcycle)
		hgraphs = _makeClusterTrees(treePart, clTreeNum);
		
		if (nLevels > hgraphs->getNumLevels()){
			cout << "Max number of levels for this benchmark is " << hgraphs->getNumLevels() << "\n";
		} else {
			for (unsigned i=0; i < nLevels; i++ ){
				uofm::vector<unsigned> mapping = hgraphs->getMapping(i);	
				for (int j=0; j < numNodes; j++){
					if (part[j] == -1){
						part[j] = mapping[j];
					}else {
						int idx = part[j];
						part[j] = mapping[idx];
					}
				}
			}
		}
		_soln2Buffers->checkInBuf(mBuf, sBuf);
		delete hgraphs;
}

void MLPart::_startWithPartProblem(PartitioningProblem& problem, ClusteredHGraph& hgraphs) {
        abkfatal(&problem.getPartitions() != NULL, " No partition BBoxes available ");

        const HGraphFixed* graph = problem.getHGraphPointer();

        const vector<BBox>& terminalBBoxes = problem.getPadBlocks();
        const vector<unsigned>& terminalToBlock = problem.getTerminalToBlock();

        unsigned numTerminals = graph->getNumTerminals();

        stringstream strbuf;
        strbuf << "(" << terminalToBlock.size() << " vs " << numTerminals << ")";

        abkfatal2(terminalToBlock.size() >= numTerminals, " #terminals mismatch ", strbuf.str().c_str());

        populate(hgraphs, terminalToBlock, terminalBBoxes);

        bool validSoln = _go();
        // validSoln is true if the tree generated valid solns, false if not.

        // Copy solutions from the double buffer to part. problem

        PartitioningBuffer* mainBuf = NULL, *shadowBuf = NULL;

        if (validSoln) {
                _figureBestCost(problem);  // set's _bestSolnNum to the best
                // solution for this tree

                _soln2Buffers->checkOutBuf(mainBuf, shadowBuf);

                Partitioning& bestPart = problem.getSolnBuffers()[_bestSolnNum];

                for (unsigned nId = 0; nId < problem.getHGraph().getNumNodes(); nId++) {
                        bestPart[nId] = (*mainBuf)[_bestSolnNum][nId];
                }

                if (_params.verb.getForMajStats() > 10) {
                        UnivPartEval checker;
                        unsigned checkedCost = checker.computeCost(problem, bestPart);
                        cout << " Start W. PartProblem :Net Cut is " << checkedCost << endl;
                }

                _soln2Buffers->checkInBuf(mainBuf, shadowBuf);
        } else
                cout << endl << "No legal solutions were found" << endl;
}

void MLPart::_figureBestCost(PartitioningProblem& problem) {
        _bestSolnNum = _partitioner->getBestSolnNum();
        _bestCost = _partitioner->getBestSolnCost();
        _aveCost = _partitioner->getAveSolnCost();
        _numLegalSolns = _partitioner->getNumLegalSolns();

        problem.setBestSolnNum(_bestSolnNum);
        return;
}

// by sadya+ramania for analytical clustering
MLPart::MLPart(PartitioningProblem& problem, const MLPartParams& params, PlacementWOrient* placement, BBPartBitBoardContainer& bbBitBoards, MaxMem* maxMem, FillableHierarchy* fill) : BaseMLPart(problem, params, bbBitBoards, maxMem, fill), _peakMemUsage(0.) {
        abkfatal(problem.getNumPartitions() == 2,
                 "More than 2-way"
                 "partitioning is not supported in this distribution. For more"
                 "details contact Igor Markov <imarkov@eecs.umich.edu>");

        Timer tm;
        const HGraphFixed* graph = problem.getHGraphPointer();

        unsigned numNodes = graph->getNumNodes();
        unsigned numEdges = graph->getNumEdges();
        unsigned sizeOfTop = _params.clParams.sizeOfTop;
        bool doFlat = sizeOfTop == 0 || (graph->getNumNodes() - graph->getNumTerminals()) <= sizeOfTop;

        if (numEdges <= numNodes / 2 && !doFlat) {
                cerr << "\n NumEdges must be > numNodes/2 \n "
                     << "The actual values are " << numEdges << " and " << numNodes << flush;
                //    abkfatal(0," Cannot perform clustering ");
        }

        if (doFlat) {
                if (_params.verb.getForActions()) cout << endl << "Problem too small for ML, running flat" << endl;
                _soln2Buffers = NULL;
                _callPartitionerWoML(problem);
                const PartitioningSolution& bestSol = _partitioner->getResult(_bestSolnNum);
                _bestSolnPartWeights = bestSol.partArea;
                _figureBestCost(problem);
        } else {

                if (_params.verb.getForActions() > 1 || _params.verb.getForMajStats() > 1)
                        cout << "\n Multilevel partitioning on " << problem.getHGraph().getNumNodes() << " nodes "
                             << " including " << problem.getHGraph().getNumTerminals() << " terminals ";

                // allocate ML's double buffer --- an implicit parameter
                // to the call _startWithPartProblem() below
                _soln2Buffers = new PartitioningDoubleBuffer(problem.getSolnBuffers());

                // setup for multiple clusterings
                unsigned solnBegin = _soln2Buffers->beginUsedSoln(), solnEnd = _soln2Buffers->endUsedSoln();

                unsigned step = _params.runsPerClTree;
                double numClTreesDbl = max(double(1), ceil((solnEnd - solnBegin) / (1.0 * step)));
                unsigned numClTrees = static_cast<unsigned>(rint(numClTreesDbl));

                if (_params.verb.getForActions() || _params.verb.getForMajStats()) {
                        cout << "\n Will now create " << numClTrees << " clusterTree";
                        if (numClTrees != 1) cout << "s and use each";
                        cout << " to find ";
                        if (step == 1)
                                cout << "one solution" << endl;
                        else
                                cout << "(up to) " << step << " solutions" << endl;
                }

                // these for are primarily for debug output
                vector<double> bestCosts(numClTrees, DBL_MAX);
                vector<unsigned> bestSolnNums(numClTrees, UINT_MAX);
                vector<double> aveCosts(numClTrees, DBL_MAX);
                vector<unsigned> numLegalSolns(numClTrees, UINT_MAX);
                // these allow to find "cre`me de la cre`me" of all solutions
                double totalCost = 0.0;
                unsigned totalNumLegalSolns = 0;
                double curBestCost = DBL_MAX;
                unsigned curBestSolnNum = UINT_MAX;

                problem.propagateTerminals();

                for (unsigned clTreeNum = 0, curBegin = 0; clTreeNum != numClTrees; clTreeNum++, curBegin += step) {
                        // narrow the buffers
                        unsigned curEnd = min(curBegin + step, solnEnd);
                        _soln2Buffers->setBeginUsedSoln(curBegin);
                        _soln2Buffers->setEndUsedSoln(curEnd);
                        if (numClTrees > 1) {
                                if (_params.verb.getForActions() > 1 || _params.verb.getForMajStats() > 1) {
                                        cout << "  --- Tree " << clTreeNum << endl;
                                }
                        }

                        PartitioningBuffer* mBuf, *sBuf;
                        _soln2Buffers->checkOutBuf(mBuf, sBuf);

                        const Partitioning& treePart = (*mBuf)[curBegin];
                        abkwarn(curEnd - curBegin == 1,
                                "Warning: multiple starts/tree. Dangerous with "
                                "an initial solution");

                        // setup to construct clustering
                        ClustHGraphParameters clParams = _params.clParams;
                        clParams.verb.setForActions(clParams.verb.getForActions() / 10);
                        clParams.verb.setForMajStats(clParams.verb.getForMajStats() / 10);
                        clParams.verb.setForSysRes(clParams.verb.getForSysRes() / 10);

                        if (clTreeNum % 2)  // every other tree is HEM
                        {
                                clParams.clType = ClHG_ClusteringType::HEM;
                                clParams.weightOption = 6;
                        }

                        Timer clTreeCreationTime;
                        ClusteredHGraph* hgraphs;

                        if (_hierarchy != NULL)  // use the hierarchy
                        {
                                hgraphs = new ClusteredHGraph(*_hierarchy, *graph, clParams);

                                // the clusteredHGraph ctor that takes a
                                // hierarchy does not
                                // deal with fixed constraints.  However,
                                // terminals aren't
                                // clustered, so they will have the same Id's in
                                // all
                                // partitionings.
                                int topLvlIdx = hgraphs->getNumLevels() - 1;
                                Partitioning& topLvlFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));
                                Partitioning& topLvlPart = const_cast<Partitioning&>(hgraphs->getTopLevelPart());

                                for (unsigned t = 0; t < graph->getNumTerminals(); t++) topLvlFixed[t] = topLvlPart[t] = problem.getFixedConstr()[t];

                                // propigate the fixed constraints down...
                                for (topLvlIdx--; topLvlIdx >= 0; topLvlIdx--) {
                                        Partitioning& aboveFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx + 1));
                                        Partitioning& belowFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));

                                        hgraphs->mapPartitionings(aboveFixed, belowFixed, topLvlIdx);
                                }
                        } else {
                                hgraphs = new ClusteredHGraph(*graph, clParams, problem.getFixedConstr(), treePart, placement);
                        }
                        clTreeCreationTime.stop();
                        _totalClusteringTime += clTreeCreationTime.getUserTime();

                        _soln2Buffers->checkInBuf(mBuf, sBuf);

                        _doingCycling = false;
                        Timer unClusteringTime;

                        _startWithPartProblem(problem, *hgraphs);
                        delete hgraphs;

                        unClusteringTime.stop();
                        _totalUnClusteringTime += unClusteringTime.getUserTime();

                        // collect stats
                        bestCosts[clTreeNum] = _bestCost;
                        bestSolnNums[clTreeNum] = _bestSolnNum;
                        aveCosts[clTreeNum] = _aveCost;
                        numLegalSolns[clTreeNum] = _numLegalSolns;

                        totalNumLegalSolns += _numLegalSolns;
                        totalCost += _numLegalSolns * _aveCost;

                        if (_bestCost < curBestCost) {
                                curBestCost = _bestCost;
                                curBestSolnNum = _bestSolnNum;
                                const PartitioningSolution& bestSol = _partitioner->getBestResult();
                                _bestSolnPartWeights = bestSol.partArea;
                        }

                        delete _partitioner;
                        _partitioner = NULL;

                }  // forloop

                if (_params.verb.getForActions() > 1) {
                        cout << "Threshold for starting VCycling is " << _params.lastVCycleStartThreshold << endl << "Best cost is " << curBestCost << endl;
                }
                if (_params.Vcycling != Parameters::NoVcycles && (curBestCost <= _params.lastVCycleStartThreshold)) {
                        _doLastVCycle(curBestCost, curBestSolnNum);
                } else {
                        if (_params.verb.getForActions() > 1) {
                                if (_params.Vcycling == Parameters::NoVcycles) {
                                        cout << "VCycling skipped due to "
                                                "NoVcycles option" << endl;
                                } else {
                                        cout << "VCycling skipped because "
                                                "solution quality worse than "
                                                "threshold" << endl;
                                }
                        }
                }

                // restore the initial buffer state
                _soln2Buffers->setBeginUsedSoln(solnBegin);
                _soln2Buffers->setEndUsedSoln(solnEnd);

                _bestCost = curBestCost;
                _problem.setBestSolnNum(_bestSolnNum = curBestSolnNum);
                _numLegalSolns = totalNumLegalSolns;
                _aveCost = totalCost / totalNumLegalSolns;

                _aveClusteringTime = _totalClusteringTime / numClTrees;
                _aveUnClusteringTime = _totalUnClusteringTime / numClTrees;
                _callPartTime /= numClTrees;

                if (_params.verb.getForMajStats() > 10) {
                        UniversalPartEval lastCheck;
                        unsigned checkedBestCost = lastCheck.computeCost(_problem, _problem.getBestSoln());
                        cout << endl << endl << "Final Checked Best Cost is " << checkedBestCost << endl;
                }
        }

        tm.stop();
        if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
                unsigned numStartsDone;
                if (_soln2Buffers == NULL)  // was running flat
                {
                        const PartitioningBuffer& _solnBuffer = problem.getSolnBuffers();
                        numStartsDone = _solnBuffer.endUsedSoln() - _solnBuffer.beginUsedSoln();
                } else {
                        numStartsDone = _soln2Buffers->endUsedSoln() - _soln2Buffers->beginUsedSoln();
                }

                cout << " Best Cost in one ML (" << numStartsDone << " starts):" << _bestCost << "   average :    " << _aveCost << endl;
                cout << "  Part Areas: " << getPartitionArea(0) << "  (" << getPartitionNumNodes(0) << " nodes with terms) "
                     << "  /  " << getPartitionArea(1) << "  (" << getPartitionNumNodes(1) << " nodes with terms) " << endl;

                {
                        unsigned pinB0 = 0, pinB1 = 0;
                        const Partitioning& part = _problem.getBestSoln();
                        const HGraphFixed& hg = _problem.getHGraph();
                        for (unsigned n = 0; n != hg.getNumNodes(); n++) {
                                unsigned deg = hg.getNodeByIdx(n).getDegree();
                                if (part[n].isInPart(0))
                                        pinB0 += deg;
                                else
                                        pinB1 += deg;
                        }
                        cout << " Pin balances :   " << pinB0 << " / " << pinB1 << endl;
                }

                cout << " One ML took           " << tm << endl;
                if (_soln2Buffers != NULL)  // non-trivial ML
                {
                        if (_params.verb.getForMajStats() > 1) {

                                cout << " Ave Clustering Time Per Tree   : " << _aveClusteringTime << endl;
                                cout << " Ave UnClustering Time Per Tree : " << _aveUnClusteringTime << endl;
                                cout << " Ave CallPartitioner Time Per Tree: " << _callPartTime << endl;
                        }
                }
        }
        _userTime = tm.getUserTime();
}

void MLPart::_doLastVCycle(double& curBestCost, unsigned curBestSolnNum) {
        _doingCycling = true;
        const HGraphFixed* graph = _problem.getHGraphPointer();
        if (_params.verb.getForActions()) cout << " Performing Last V-Cycle(s) on best solution" << endl;

        // Vcycle on the best 1 solution
        _soln2Buffers->setBeginUsedSoln(curBestSolnNum);
        _soln2Buffers->setEndUsedSoln(curBestSolnNum + 1);

        _params.pruningPercent = 1000;  // turn pruning off

        ClustHGraphParameters curParams = _params.clParams;
        curParams.clType = ClHG_ClusteringType::HEM;
        curParams.weightOption = 6;
        curParams.sizeOfTop = static_cast<unsigned>(curParams.sizeOfTop * curParams.levelGrowth);
        curParams.verb.setForActions(curParams.verb.getForActions() / 10);
        curParams.verb.setForMajStats(curParams.verb.getForMajStats() / 10);
        curParams.verb.setForSysRes(curParams.verb.getForSysRes() / 10);

        if (_params.verb.getForActions() > 1) {
                cout << "Before last VCycle(s), best cost is " << curBestCost << endl;
                cout << "Improvement threshold is " << 100. * _params.lastVCycleImproveThreshold << "%" << endl;
        }

        // by royj for multiple vcycles at the end
        double improvement;
        do {
                Partitioning& part = _problem.getSolnBuffers()[curBestSolnNum];
                Timer clTreeCreationTime;
                // this is the VCycling clustering...don't use
                // the hierarchy here.
                ClusteredHGraph hgraphs(*graph, curParams, _problem.getFixedConstr(), part);
                clTreeCreationTime.stop();
                _totalClusteringTime += clTreeCreationTime.getUserTime();

                MemUsage m4;
                _peakMemUsage = max(_peakMemUsage, m4.getPeakMem());
                _maxMem->update("MLPart VCycling after clustering");

                Timer unClusteringTime;

                _startWithPartProblem(_problem, hgraphs);

                unClusteringTime.stop();
                _totalUnClusteringTime += unClusteringTime.getUserTime();

                MemUsage m3;
                _peakMemUsage = max(_peakMemUsage, m3.getPeakMem());
                _maxMem->update("MLPart VCycling after unclustering");

                improvement = (curBestCost - _bestCost) / curBestCost;
                curBestCost = _bestCost;
                const PartitioningSolution& bestSol = _partitioner->getBestResult();
                _bestSolnPartWeights = bestSol.partArea;
                _bestSolnPartNumNodes = bestSol.partCount;

                if (_params.verb.getForActions() > 1) cout << "Best cost is " << curBestCost << " improvement was " << 100 * improvement << "%" << endl;

                delete _partitioner;
                _partitioner = NULL;

        } while (improvement > _params.lastVCycleImproveThreshold);
}

/*
void MLPart::_FMLegalizeOnly(PartitioningProblem& problem)
{
    PartitionerParams par(_params);

    if (_partitioner) { delete _partitioner; _partitioner = NULL ; }
    par.verb.setForActions (par.verb.getForActions ()/10);
    par.verb.setForSysRes  (par.verb.getForSysRes  ()/10);
    par.verb.setForMajStats(par.verb.getForMajStats()/10);

    par.useEarlyStop=false;
    par.maxHillWidth=100.0;
    par.maxHillHeightFactor=0.0;
    par.maxNumPasses=0;
    par.minPassImprovement=0.0;
    par.legalizeOnly=true;

    if (par.moveMan==MoveManagerType::FMwCutLineRef)
        par.moveMan=MoveManagerType::FM;

    switch (_params.flatPartitioner)
    {
    case PartitionerType :: FM :
        _partitioner = new FMPartitioner(problem,par);
        break;
    default:  abkfatal(0, " Unknown partitioner ");
    }

    _bestSolnNum=problem.getBestSolnNum();

}
*/

ClusteredHGraph* MLPart::_makeClusterTrees(const Partitioning& treePart, unsigned clTreeNum) {
        _doingCycling = false;
        const HGraphFixed* graph = _problem.getHGraphPointer();
        // setup to construct clustering
        ClustHGraphParameters clParams = _params.clParams;
        clParams.verb.setForActions(clParams.verb.getForActions() / 10);
        clParams.verb.setForMajStats(clParams.verb.getForMajStats() / 10);
        clParams.verb.setForSysRes(clParams.verb.getForSysRes() / 10);

        if (clTreeNum % 2)  // every other tree is HEM
        {
                clParams.clType = ClHG_ClusteringType::HEM;
                clParams.weightOption = 6;
        }

        ClusteredHGraph* hgraphs;
        if (_hierarchy != NULL)  // use the hierarchy
        {
                cout << "Using an awesome hierarchy" << endl;
                hgraphs = new ClusteredHGraph(*_hierarchy, *graph, clParams);

                // the clusteredHGraph ctor that takes a hierarchy does not
                // deal with fixed constraints.  However, terminals aren't
                // clustered, so they will have the same Id's in all
                // partitionings.
                int topLvlIdx = hgraphs->getNumLevels() - 1;
                Partitioning& topLvlFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));
                Partitioning& topLvlPart = const_cast<Partitioning&>(hgraphs->getTopLevelPart());

                for (unsigned t = 0; t < graph->getNumTerminals(); t++) topLvlFixed[t] = topLvlPart[t] = _problem.getFixedConstr()[t];

                // propigate the fixed constraints down...
                for (topLvlIdx--; topLvlIdx >= 0; topLvlIdx--) {
                        Partitioning& aboveFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx + 1));
                        Partitioning& belowFixed = const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));

                        hgraphs->mapPartitionings(aboveFixed, belowFixed, topLvlIdx);
                }
        } else {
                hgraphs = new ClusteredHGraph(*graph, clParams, _problem.getFixedConstr(), treePart);
        }
        return hgraphs;
}

ClusteredHGraph* MLPart::_makeVCycleTrees(const Partitioning& part, unsigned clTreeNum) {
        _doingCycling = false;
        if (_params.verb.getForActions() > 1) cout << "Making a VCycle tree" << endl;
        const HGraphFixed* graph = _problem.getHGraphPointer();
        // _params.pruningPercent     = 1000; //turn pruning off

        //  _soln2Buffers->setBeginUsedSoln(clTreeNum);
        //  _soln2Buffers->setEndUsedSoln(clTreeNum+1);

        ClustHGraphParameters curParams = _params.clParams;
        curParams.clType = ClHG_ClusteringType::HEM;
        curParams.weightOption = 6;
        /*  curParams.sizeOfTop =
              static_cast<unsigned>(curParams.sizeOfTop *
           curParams.levelGrowth);*/
        curParams.verb.setForActions(curParams.verb.getForActions() / 10);
        curParams.verb.setForMajStats(curParams.verb.getForMajStats() / 10);
        curParams.verb.setForSysRes(curParams.verb.getForSysRes() / 10);
        // this is the VCycling clustering...don't use
        // the hierarchy here.
        ClusteredHGraph* hgraphs = new ClusteredHGraph(*graph, curParams, _problem.getFixedConstr(), part);
        /*
            int           topLvlIdx = hgraphs->getNumLevels()-1;
            Partitioning& topLvlFixed   =
                    const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));
            Partitioning& topLvlPart    =
                    const_cast<Partitioning&>(hgraphs->getTopLevelPart());

            for(unsigned t = 0; t < graph->getNumTerminals(); t++)
                 topLvlFixed[t] = topLvlPart[t]  = _problem.getFixedConstr()[t];

            //propigate the fixed constraints down...
            for(topLvlIdx-- ; topLvlIdx >= 0; topLvlIdx--)
            {
                 Partitioning& aboveFixed   =
                     const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx+1));
                 Partitioning& belowFixed   =
                     const_cast<Partitioning&>(hgraphs->getFixedConst(topLvlIdx));
                 hgraphs->mapPartitionings(aboveFixed, belowFixed, topLvlIdx);
            }*/

        return hgraphs;
}
