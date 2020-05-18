/**************************************************************************
***
*** Copyright (c) 2008 Regents of the University of California,
***               Andrew B. Kahng, Kwangok Jeong and Hailong Yao
***
***  Contact author(s): abk@cs.ucsd.edu, kjeong@vlsicad.ucsd.edu, hailong@cs.ucsd.edu
***  Original Affiliation:   UCSD, Computer Science and Engineering Department,
***                          La Jolla, CA 92093-0404 USA
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

/***************************
*** Created by Hailong Yao
*** hailong@cs.ucsd.edu
*** Date: Jul. 30, 2008
***************************/

#include <iostream>
#include <algorithm>
#include <cmath>
#include <unistd.h>
#include <cmath>
#include <vector>

#include "MLPart.h"

#include "ClusteredHGraph/baseClustHGraph.h"
#include "HGraph/hgFixed.h"
#include "Geoms/bbox.h"
#include "Partitioning/partProb.h"
#include "ABKCommon/abkversion.h"
#include "ABKCommon/abkseed.h"
#include "MLPart/mlPart.h"
#include "PartEvals/partEvals.h"
#include "Stats/trivStats.h"
#include "Stats/expMins.h"
#include "PartEvals/partEvals.h"

#include "ABKCommon/abkassert.h"
#include "ABKCommon/abktimer.h"
#include "ABKCommon/abkrand.h"

#include <new>

Verbosity buildVerbosity(int level) { 
        char verbString[12]; 
        sprintf(verbString, "%d_%d_%d", level, level, level); 
        Verbosity v(verbString); 
        return v; 
}


int UMpack_mlpart(int nvtxs, int nhedges, double *vwgts, int *eptr, int *eind, double *edgeWeights, int nparts, double *balanceArray, double tolerance, int *part, int startsPerRun, int totalRuns, int debugLevel, unsigned seed) {

        abkassert(nparts == 2, "nparts != 2, bipartitioning only supported operation");
        if (!SeedHandler::isInitialized()) {
                SeedHandler::turnOffLogging();
                SeedHandler::overrideExternalSeed(seed);
        }

        HGraphFixed hg(eptr, eind, edgeWeights, vwgts, nvtxs, eptr[nhedges], nhedges, 0);
        abkassert(hg.getNumNodes() == (unsigned)nvtxs, "nvtxs and hg.getNumNodes() doesn't match");

        uofm::vector<BBox> partitions = uofm::vector<BBox>(nparts, BBox(Point(0,0)));
        uofm::vector<uofm::vector<double> > capacities = uofm::vector<uofm::vector<double> >(nparts, uofm::vector<double>(1, -1));
        uofm::vector<double> tolerances = uofm::vector<double>(1);
        uofm::vector<unsigned> terminalsToBlock = uofm::vector<unsigned>(0);
        uofm::vector<BBox> padBlocks = uofm::vector<BBox>(nparts, BBox(Point(0, 0)));
        tolerances[0] = tolerance;
        PartitioningBuffer solnBuffers = PartitioningBuffer(hg.getNumNodes(), 1);
        PartitionIds nowhere;
        for (unsigned k = 0; k != hg.getNumTerminals(); k++) solnBuffers[0][k] = nowhere;
        for (unsigned k = 0; k != solnBuffers[0].size(); k++) {
                if (part[k] < 0)
                        solnBuffers[0][k].setToAll(nparts);
                else
                        solnBuffers[0][k].addToPart(part[k]);

        }
        double totalSize;
        if (!vwgts)
                totalSize = (double)nvtxs;
        else {
                totalSize = 0;
                for (int i = 0; i < nvtxs; i++) totalSize += vwgts[i];
        }
        for (int i = 0; i < nparts; i++) capacities[i][0] = totalSize * balanceArray[i];
        Partitioning fixedConstr = Partitioning(hg.getNumNodes());
        for (unsigned k = 0; k < hg.getNumNodes(); k++) {
                int predefPart = part[k];
                if (predefPart < 0)
                        fixedConstr[k].setToAll(nparts);
                else {
                        abkfatal(predefPart < nparts, "predefined to invalid partition");
                        fixedConstr[k].setToPart(predefPart);
                }
        }

        const HGraphFixed &chg = hg;
        const PartitioningBuffer &csolnBuffers = solnBuffers;
        const Partitioning &cfixedConstr = fixedConstr;
        const uofm::vector<BBox> cpartitions = partitions;
        const uofm::vector<uofm::vector<double> > ccapacities = capacities;
        const uofm::vector<double> ctolerances = tolerances;
        const uofm::vector<unsigned> cterminalToBlock = terminalsToBlock;
        const uofm::vector<BBox> cpadBlocks = padBlocks;


        PartitioningProblem problem(chg, csolnBuffers, cfixedConstr, partitions, capacities, tolerances, terminalsToBlock, padBlocks);
        problem.reserveBuffers(startsPerRun);
        UnivPartEval costCheck(PartEvalType::NetCutWNetVec);
        std::vector<unsigned> bests;
        Partitioning bestSeen(problem.getHGraph().getNumNodes());
        unsigned bestSeenCost = UINT_MAX;
        int argc = 0;
        const char *argv[10];
        BaseMLPart::Parameters mlParams(argc, argv);

        Verbosity v = buildVerbosity(debugLevel);
        mlParams.Vcycling = MLPartParams::NoVcycles;
        mlParams.clParams.verb = v;
        mlParams.verb = v;

        BBPartBitBoardContainer bbBitBoards;
        MaxMem maxMem;
        for (int r = 0; r < totalRuns; r++) {
                MLPart mlPartitioner(problem, mlParams, bbBitBoards, &maxMem, (FillableHierarchy *)NULL);
                Partitioning &curPart = (problem.getSolnBuffers()[problem.getBestSolnNum()]);
                unsigned actualCost = costCheck.computeCost(problem, curPart);

                if (actualCost < bestSeenCost) {
                        bestSeen = curPart;
                        bestSeenCost = actualCost;
                }
                bests.push_back(actualCost);

                // clear the buffer for the next run
                PartitioningBuffer &buff = problem.getSolnBuffers();
                for (unsigned s = buff.beginUsedSoln(); s != buff.endUsedSoln(); s++) {
                        Partitioning &curBuff = buff[s];
                        for (unsigned i = 0; i < curBuff.size(); i++) curBuff[i].setToAll(31);
                }
        }
        /* now assign back to the array */
        for (int i = 0; i < nvtxs; i++) part[i] = bestSeen[i].lowestNumPart();

        partitions.clear();
        capacities.clear();
        tolerances.clear();
        terminalsToBlock.clear();
        padBlocks.clear();

        return bestSeenCost;
}

int UMpack_mlpart(int nvtxs, int nhedges, double *vwgts, int *eptr, int *eind, double *edgeWeights, int nparts, double *balanceArray, double tolerance, int *part, int startsPerRun, int totalRuns, int debugLevel, unsigned seed, unsigned nLevels) {

        abkassert(nparts == 2, "nparts != 2, bipartitioning only supported operation");
        if (!SeedHandler::isInitialized()) {
                SeedHandler::turnOffLogging();
                SeedHandler::overrideExternalSeed(seed);
        }

        HGraphFixed hg(eptr, eind, edgeWeights, vwgts, nvtxs, eptr[nhedges], nhedges, 0);
        abkassert(hg.getNumNodes() == (unsigned)nvtxs, "nvtxs and hg.getNumNodes() doesn't match");

        uofm::vector<BBox> partitions = uofm::vector<BBox>(nparts, BBox(Point(0,0)));
        uofm::vector<uofm::vector<double> > capacities = uofm::vector<uofm::vector<double> >(nparts, uofm::vector<double>(1, -1));
        uofm::vector<double> tolerances = uofm::vector<double>(1);
        uofm::vector<unsigned> terminalsToBlock = uofm::vector<unsigned>(0);
        uofm::vector<BBox> padBlocks = uofm::vector<BBox>(nparts, BBox(Point(0, 0)));
        tolerances[0] = tolerance;
        PartitioningBuffer solnBuffers = PartitioningBuffer(hg.getNumNodes(), 1);
        PartitionIds nowhere;
        for (unsigned k = 0; k != hg.getNumTerminals(); k++) solnBuffers[0][k] = nowhere;
        for (unsigned k = 0; k != solnBuffers[0].size(); k++) {
                if (part[k] < 0)
                        solnBuffers[0][k].setToAll(nparts);
                else
                        solnBuffers[0][k].addToPart(part[k]);

        }
        double totalSize;
        if (!vwgts)
                totalSize = (double)nvtxs;
        else {
                totalSize = 0;
                for (int i = 0; i < nvtxs; i++) totalSize += vwgts[i];
        }
        for (int i = 0; i < nparts; i++) capacities[i][0] = totalSize * balanceArray[i];
        Partitioning fixedConstr = Partitioning(hg.getNumNodes());
        for (unsigned k = 0; k < hg.getNumNodes(); k++) {
                int predefPart = part[k];
                if (predefPart < 0)
                        fixedConstr[k].setToAll(nparts);
                else {
                        abkfatal(predefPart < nparts, "predefined to invalid partition");
                        fixedConstr[k].setToPart(predefPart);
                }
        }

        const HGraphFixed &chg = hg;
        const PartitioningBuffer &csolnBuffers = solnBuffers;
        const Partitioning &cfixedConstr = fixedConstr;
        const uofm::vector<BBox> cpartitions = partitions;
        const uofm::vector<uofm::vector<double> > ccapacities = capacities;
        const uofm::vector<double> ctolerances = tolerances;
        const uofm::vector<unsigned> cterminalToBlock = terminalsToBlock;
        const uofm::vector<BBox> cpadBlocks = padBlocks;


        PartitioningProblem problem(chg, csolnBuffers, cfixedConstr, partitions, capacities, tolerances, terminalsToBlock, padBlocks);
        problem.reserveBuffers(startsPerRun);
        UnivPartEval costCheck(PartEvalType::NetCutWNetVec);
        std::vector<unsigned> bests;
        Partitioning bestSeen(problem.getHGraph().getNumNodes());
        unsigned bestSeenCost = UINT_MAX;
        int argc = 0;
        const char *argv[10];
        BaseMLPart::Parameters mlParams(argc, argv);

        Verbosity v = buildVerbosity(debugLevel);
        mlParams.Vcycling = MLPartParams::NoVcycles;
        mlParams.clParams.verb = v;
        mlParams.verb = v;

	BBPartBitBoardContainer bbBitBoards;
	MaxMem maxMem;
	MLPart mlCluster(problem, mlParams, bbBitBoards, &maxMem, part, nLevels, (FillableHierarchy *)NULL);

        partitions.clear();
        capacities.clear();
        tolerances.clear();
        terminalsToBlock.clear();
        padBlocks.clear();

        return bestSeenCost;
}
