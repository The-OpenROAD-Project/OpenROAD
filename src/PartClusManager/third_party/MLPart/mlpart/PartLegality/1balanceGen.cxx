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

#include "1balanceGen.h"
#include "HGraph/hgFixed.h"
#include "Stats/stats.h"

using std::cout;
using std::cerr;
using std::endl;
using std::max;
using uofm::vector;

unsigned AllToOneGen::counter = 0;
RandomRawUnsigned SBGWeightRand::_ru;
RandomRawUnsigned SBGRandDutt::_ru;
RandomRawUnsigned SBGRandVeryIllegal::_ru;

vector<double> AllToOneGen::generateSoln(Partitioning& curPart) {
        unsigned numPart = _problem.getNumPartitions();
        const HGraphFixed& hgraph = _problem.getHGraph();
        unsigned numNodes = _problem.getHGraph().getNumNodes();
        const Partitioning& fixed = _problem.getFixedConstr();
        vector<double> balances(numPart);

        for (unsigned node = 0; node != numNodes; node++) {
                unsigned orig = counter % numPart;
                unsigned nodeGoesTo = orig;
                while (!fixed[node].isInPart(nodeGoesTo)) {
                        nodeGoesTo = (nodeGoesTo + 1) % numPart;
                        abkfatal3(nodeGoesTo != orig, " Node ", node, " can not go into any partition\n");
                }
                curPart[node].setToPart(nodeGoesTo);
                balances[nodeGoesTo] += hgraph.getWeight(node);
        }
        counter++;
        return balances;
}

// SBGWeightRand: implemented by ILM
// produces a simple-minded randomized partitioning which is balanced
// with high probability; FM, Sanchis, SA and other consumers will need
// to make improving passes -- they can combine such passes with improving
// their objectives, which is why this methods does not do additional improving
// passes (this can be added as an option if needed)
//
// Possible improvements:
//  1. some "local" random permuting needed for nodes of approx. equal sizes
//   because the locations of consequtive nodes follownig a node with
//   much bigger weight can be highly correlated
//   (permuting all nodes is not good since it can decrease the probability
//   of getting a balances partition if there very big nodes)
//  2. Can we estimate the probability of getting a balanced solution ?
//     How do we improve the probability

bool SBGWeightRand::assignNodeToPartition(Partitioning& curPart, unsigned nodeIdx, unsigned nonZeroChances, double minNonZero, vector<double>& areaToMin, vector<double>& areaToFill, vector<double>& availableArea, vector<double>& chancesOfGoingToPart) {
        minNonZero *= 1e-3;

        unsigned numPartitions = _problem.getNumPartitions();
        const HGraphFixed& hgraph = _problem.getHGraph();

        const HGFNode& node = hgraph.getNodeByIdx(nodeIdx);
        const double nodeWt = hgraph.getWeight(node.getIndex());

        if (nonZeroChances == 1) {
                for (unsigned i = 0; i < numPartitions; i++)
                        if (chancesOfGoingToPart[i] > 0.0) {
                                curPart[nodeIdx].setToPart(i);
                                areaToMin[i] -= nodeWt;
                                areaToFill[i] -= nodeWt;
                                availableArea[i] -= nodeWt;
                                return true;
                        }
                return false;
                //			abkfatal(0," Internal error: control should
                // not reach here");
                //			return false;
        } else {
                for (unsigned i = 0; i < numPartitions; i++) chancesOfGoingToPart[i] /= minNonZero;
                unsigned p = BiasedRandomSelection(chancesOfGoingToPart, _ru);
                curPart[nodeIdx].setToPart(p);
                areaToMin[p] -= nodeWt;
                areaToFill[p] -= nodeWt;
                availableArea[p] -= nodeWt;
                return true;
        }
        return false;
}

vector<double> SBGWeightRand::generateSoln(Partitioning& curPart) {
        unsigned numPartitions = _problem.getNumPartitions();
        const HGraphFixed& hgraph = _problem.getHGraph();

        abkfatal(hgraph.getNumNodes() >= numPartitions, "not enough nodes / partition");

        vector<double> areaToMin(numPartitions), areaToFill(numPartitions), targetAreas(numPartitions), availableArea(numPartitions), chancesOfGoingToPart(numPartitions);

        const vector<vector<double> >& maxCaps = _problem.getMaxCapacities();
        const vector<vector<double> >& minCaps = _problem.getMinCapacities();
        const vector<vector<double> >& caps = _problem.getCapacities();

        unsigned k;
        for (k = 0; k < numPartitions; k++) {
                availableArea[k] = maxCaps[k][0];
                areaToMin[k] = minCaps[k][0];
                areaToFill[k] = caps[k][0];
                targetAreas[k] = caps[k][0];
        }

        for (unsigned i = 0; i < _problem.getHGraph().getNumNodes(); ++i) {
                unsigned orig = curPart[i].getUnsigned();
                abkfatal(orig != 0, "This means that the original partition is nowhere");
                unsigned fixed = _problem.getFixedConstr()[i].getUnsigned();
                unsigned final = orig & fixed;
                abkfatal(final, "no legal partitions for node");
                curPart[i].loadBitsFrom(final);
        }
        unsigned m, numNodes = hgraph.getNumNodes();

        // first make a linear pass over all nodes to assign fixed nodes to
        // their proper
        // partition and remove their weight from the capacity of each partition
        // --DAP+royj
        vector<bool> fixedAlready(numNodes, false);
        for (unsigned nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
                unsigned thisNodeNonZeroChances = 0;
                vector<double> chancesOfGoingToPart(numPartitions, 0.0);
                for (unsigned p = 0; p != numPartitions; p++)
                        if (curPart[nodeIdx].isInPart(p)) {
                                ++thisNodeNonZeroChances;
                                chancesOfGoingToPart[p] = 1.0;
                        }
                if (thisNodeNonZeroChances == 1) {
                        double minNonZero = 1.0;
                        bool assignResult = assignNodeToPartition(curPart, nodeIdx, thisNodeNonZeroChances, minNonZero, areaToMin, areaToFill, availableArea, chancesOfGoingToPart);
                        abkfatal(assignResult,
                                 "There is an internal code error, "
                                 "impossibility happened :P");
                        fixedAlready[nodeIdx] = true;
                }
        }

        const Permutation& sortAsc_1 = hgraph.getNodesSortedByWeightsWShuffle();
        double totalWgt = 0;
        for (m = numNodes - 1; m != static_cast<unsigned>(-1); m--) {
                unsigned nodeIdx = sortAsc_1[m];
                const HGFNode& node = hgraph.getNodeByIdx(nodeIdx);
                double nodeWt = hgraph.getWeight(node.getIndex());
                totalWgt += nodeWt;
                abkfatal(nodeWt >= 0, "Negative nodeweight!");
                double minNonZero = DBL_MAX;
                unsigned nonZeroChances = 0;  // to how many partitions a node can go

                // This is a fixed node, and we do not need to decide where it
                // goes
                // as it has already been accounted for above --DAP+royj
                if (fixedAlready[nodeIdx]) continue;

                /* first handle weight 0 nodes -- these are easy */
                if (nodeWt == 0.0) {
                        //				unsigned p;
                        /* identify the partitions where we could put the node
                         */
                        for (unsigned p = 0; p != numPartitions; p++)
                                if (curPart[nodeIdx].isInPart(p)) nonZeroChances++;
                        if (nonZeroChances == 0) {
                                cerr << " Node " << nodeIdx << " is constrainted to " << curPart[nodeIdx];
                                abkfatal(0, " Node can't go to any partition ");
                        }
                        k = _ru % nonZeroChances; /* pick the k'th available
                                                     partition */
                        nonZeroChances = 0;
                        bool assigned = false;
                        for (unsigned p = 0; p < numPartitions; p++) {
                                if (curPart[nodeIdx].isInPart(p)) {
                                        if (nonZeroChances == k) {
                                                curPart[nodeIdx].setToPart(p);
                                                areaToMin[p] -= nodeWt;
                                                areaToFill[p] -= nodeWt;
                                                availableArea[p] -= nodeWt;
                                                assigned = true;
                                                continue;
                                        }
                                        nonZeroChances++;
                                }
                        }
                        if (assigned == false)
                                abkfatal(0,
                                         " Internal error: control should not "
                                         "reach here");
                } else {/* for nodes with wt > 0 */
                        // compute chancesOfGoingToPart[] assuming some
                        // partitions
                        // are still underfilled

                        for (k = 0; k < numPartitions; k++) {
                                if (curPart[nodeIdx].isInPart(k) && nodeWt <= (areaToMin[k] + 1e-4)) {
                                        nonZeroChances++;
                                        chancesOfGoingToPart[k] = areaToMin[k];
                                        if (chancesOfGoingToPart[k] < minNonZero) minNonZero = chancesOfGoingToPart[k];
                                } else
                                        chancesOfGoingToPart[k] = 0.0;
                        }

                        // If chances of going to some partition are nonZero ...
                        if (nonZeroChances) {
                                assignNodeToPartition(curPart, nodeIdx, nonZeroChances, minNonZero, areaToMin, areaToFill, availableArea, chancesOfGoingToPart);
                                continue;
                        }
                        // otherwise (e.g. all partitions are filled at least to
                        // minimum size),
                        // recompute chancesOfGoingToPart[]
                        // assuming some partitions are below their targets
                        for (k = 0; k < numPartitions; k++) {
                                if (curPart[nodeIdx].isInPart(k) && nodeWt <= (areaToFill[k] + 1e-4)) {
                                        nonZeroChances++;
                                        chancesOfGoingToPart[k] = areaToFill[k];
                                        if (chancesOfGoingToPart[k] < minNonZero) minNonZero = chancesOfGoingToPart[k];
                                } else
                                        chancesOfGoingToPart[k] = 0;
                        }
                        // If chances of going to some partition are nonZero ...
                        if (nonZeroChances) {
                                bool assignResult = assignNodeToPartition(curPart, nodeIdx, nonZeroChances, minNonZero, areaToMin, areaToFill, availableArea, chancesOfGoingToPart);
                                abkfatal(assignResult, "failed to assign to partition");
                                continue;
                        }

                        // otherwise (e.g. all partitions are filled at least to
                        // target size),
                        // recompute chancesOfGoingToPart[]
                        // assuming some partitions are below their max

                        for (k = 0; k < numPartitions; k++) {
                                if (curPart[nodeIdx].isInPart(k) && nodeWt <= (availableArea[k] + 1e-4)) {

                                        nonZeroChances++;
                                        chancesOfGoingToPart[k] = availableArea[k];
                                        if (chancesOfGoingToPart[k] < minNonZero) minNonZero = chancesOfGoingToPart[k];
                                } else
                                        chancesOfGoingToPart[k] = 0;
                        }

                        // If chances of going to some partition are nonZero ...
                        if (nonZeroChances) {
                                bool assignResult = assignNodeToPartition(curPart, nodeIdx, nonZeroChances, minNonZero, areaToMin, areaToFill, availableArea, chancesOfGoingToPart);
                                abkfatal(assignResult, "failed to assign to partition");
                                continue;
                        }
                        // otherwise (e.g. all partitions are filled at least to
                        // max size),
                        // recompute chancesOfGoingToPart[] to minimizing
                        // overfill

                        for (k = 0; k < numPartitions; k++) {
                                if (curPart[nodeIdx].isInPart(k)) {
                                        nonZeroChances++;
                                        chancesOfGoingToPart[k] = 1.0 / fabs(nodeWt - availableArea[k]);
                                        if (chancesOfGoingToPart[k] < minNonZero) minNonZero = chancesOfGoingToPart[k];
                                } else
                                        chancesOfGoingToPart[k] = 0;
                        }

                        // if chance of going to all partitions are 0 i
                        // (e.g. the node was not allowed anywhere), issue fatal
                        // error messg.
                        abkfatal3(nonZeroChances, " Node ", nodeIdx, " can't go into any partition ");
                        if (nonZeroChances) {
                                bool assignResult = assignNodeToPartition(curPart, nodeIdx, nonZeroChances, minNonZero, areaToMin, areaToFill, availableArea, chancesOfGoingToPart);
                                abkfatal(assignResult, "failed to assign to partition");

                                continue;
                        }
                }
        }
        double minArea = 0;
        double maxArea = 0;
        for (unsigned i = 0; i < numPartitions; i++) {
                minArea += minCaps[i][0];
                maxArea += maxCaps[i][0];
        }

        if (!greaterOrEqualDouble(totalWgt, minArea) || !lessOrEqualDouble(totalWgt, maxArea)) {
                cout << "total weight: " << totalWgt << endl;
                cout << "maxArea: " << maxArea << endl;
                cout << "minArea: " << minArea << endl;
                for (unsigned i = 0; i < numPartitions; i++) {
                        cout << "mincap for p" << i << ": " << minCaps[i][0] << endl;
                        cout << "maxcap for p" << i << ": " << maxCaps[i][0] << endl;
                }

                abkfatal(greaterOrEqualDouble(totalWgt, minArea),
                         "legal solution is trivially impossible, total area "
                         "is below minimum");
                abkfatal(lessOrEqualDouble(totalWgt, maxArea),
                         "legal solution is trivially impossible, total area "
                         "is above maximum");
        }

        vector<double> partSizes(numPartitions);
        for (k = 0; k < numPartitions; k++) partSizes[k] = targetAreas[k] - areaToFill[k];

        return partSizes;
}

vector<double> SBGRandDutt::generateSoln(Partitioning& curPart) {
        unsigned numPartitions = _problem.getNumPartitions();
        const HGraphFixed& hgraph = _problem.getHGraph();

        abkfatal(numPartitions == 2, " This ini soln generator is only good for 2 parts");

        abkfatal(hgraph.getNumNodes() >= numPartitions, "not enough nodes / partition");

        //  cout << " Ini solution :" << curPart << endl;

        const vector<vector<double> >& maxCaps = _problem.getMaxCapacities();
        //  const vector<vector<double> >& targets = _problem.getCapacities();
        vector<double> availableArea(numPartitions);

        unsigned k;
        for (k = 0; k < numPartitions; k++) availableArea[k] = maxCaps[k][0];
        // for(unsigned k = 0; k < numPartitions; k++)
        // availableArea[k]=targets[k][0];

        unsigned m, numMovables = hgraph.getNumNodes();

        const Permutation& sortAsc_1 = hgraph.getNodesSortedByWeights();
        double minCellSize = hgraph.getWeight(0);

        PartitionIds cantMove;

        for (m = numMovables - 1; m != static_cast<unsigned>(-1); m--) {
                unsigned nodeIdx = sortAsc_1[m];
                // unsigned nodeIdx=m;

                abkfatal3(curPart[nodeIdx] != cantMove, "Node ", nodeIdx, " cannot be assigned to a partition\n");

                double nodeWt = hgraph.getWeight(nodeIdx);

                if (curPart[nodeIdx].isInPart(0) && curPart[nodeIdx].isInPart(1)) {
                        unsigned side = (_ru % 2 ? 1 : 0);
                        if (availableArea[side] > minCellSize + nodeWt) {
                                curPart[nodeIdx].setToPart(side);
                                availableArea[side] -= nodeWt;
                        } else {
                                curPart[nodeIdx].setToPart(1 - side);
                                availableArea[1 - side] -= nodeWt;
                        }
                } else {
                        unsigned side = (curPart[nodeIdx].isInPart(0) ? 0 : 1);
                        availableArea[side] -= nodeWt;
                }
        }

        vector<double> partSizes(numPartitions);
        for (k = 0; k < numPartitions; k++) partSizes[k] = maxCaps[k][0] - availableArea[k];
        // partSizes[k]=targets[k][0]-availableArea[k];

        return partSizes;
}

vector<double> SBGRandVeryIllegal::generateSoln(Partitioning& curPart) {
        unsigned numPartitions = _problem.getNumPartitions();
        const HGraphFixed& hgraph = _problem.getHGraph();

        abkfatal(numPartitions == 2, " This ini soln generator is only good for 2 parts");

        abkfatal(hgraph.getNumNodes() >= numPartitions, "not enough nodes / partition");

        //  cout << " Ini solution :" << curPart << endl;

        //  const vector<vector<double> >& maxCaps =
        // _problem.getMaxCapacities();
        //  const vector<vector<double> >& targets = _problem.getCapacities();
        vector<double> availableArea(numPartitions);

        vector<double> partSizes(numPartitions);

        unsigned m, numMovables = hgraph.getNumNodes();

        const Permutation& sortAsc_1 = hgraph.getNodesSortedByDegrees();
        //  double minCellSize=hgraph.getNodeByIdx(0).getWeight();

        PartitionIds cantMove;

        unsigned side = (_ru % 2 ? 1 : 0);
        unsigned startRand = max(numMovables / 3, unsigned(5));
        for (m = numMovables - 1; m != startRand; m--) {
                unsigned nodeIdx = sortAsc_1[m];
                abkfatal3(curPart[nodeIdx] != cantMove, "Node ", nodeIdx, " cannot be assigned to a partition\n");
                double nodeWt = hgraph.getWeight(nodeIdx);
                if (curPart[nodeIdx].isInPart(0) && curPart[nodeIdx].isInPart(1)) {
                        curPart[nodeIdx].setToPart(side);
                        partSizes[side] += nodeWt;
                } else {
                        unsigned side1 = (curPart[nodeIdx].isInPart(0) ? 0 : 1);
                        partSizes[side1] += nodeWt;
                }
        }

        for (; m != static_cast<unsigned>(-1); m--) {
                unsigned nodeIdx = sortAsc_1[m];
                // unsigned nodeIdx=m;

                abkfatal3(curPart[nodeIdx] != cantMove, "Node ", nodeIdx, " cannot be assigned to a partition\n");

                double nodeWt = hgraph.getWeight(nodeIdx);

                if (curPart[nodeIdx].isInPart(0) && curPart[nodeIdx].isInPart(1)) {
                        side = (_ru % 2 ? 1 : 0);
                        curPart[nodeIdx].setToPart(side);
                        partSizes[side] += nodeWt;
                } else {
                        unsigned side1 = (curPart[nodeIdx].isInPart(0) ? 0 : 1);
                        partSizes[side1] += nodeWt;
                }
        }
        return partSizes;
}
