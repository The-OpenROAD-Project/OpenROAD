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

// created on 06/27/98 by Andrew Caldwell (caldwell@cs.ucla.edu)

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "pinHemClustHG.h"
#include "Stats/stats.h"

using std::min;
using std::max;
using std::cout;
using std::endl;
using uofm::vector;

void PinHEMClusteredHGraph::populateTree() {
        // reserve the vectors.
        _randMap.reserve(getNumLeafNodes());
        _edgeWeights.insert(_edgeWeights.begin(), getNumLeafNodes(), 0.0);
        _adjNodes.reserve(200);
        _equalWtSmallNeighbors.reserve(200);

        for (unsigned t = 0; t < _terminals.size(); t++) _terminals[t]->setTag(UINT_MAX);  // marked as not clusterable

        // these may be incremented if we get stuck..
        double maxNewClRatio = _params.maxNewClRatio;
        double maxChildClRatio = _params.maxChildClRatio;

        if (maxNewClRatio == 0) maxNewClRatio = 1000;
        if (maxChildClRatio == 0) maxChildClRatio = 1000;

        // this will never change...
        double maxClArea = _totalArea * (_params.maxClArea / 100.0);
        if (maxClArea == 0)  // 0 == no limit
                maxClArea = _totalArea;

        if (_params.verb.getForMajStats()) cout << "PinHEM Clustering" << endl;

        printStatsHeader();
        printStats();
        clusterDegree1Nodes();
        // clusterDegree2Nodes();
        // printStats();

        if (_params.removeDup > 0) removeDuplicateEdges();

        unsigned level = 1;
        unsigned numTimesStuck = 0;
        bool savedLastLevel;
        bool doAnotherLevel = true;
        unsigned prevSnapshotSize = numClusterable();
        unsigned snapshotTarget = static_cast<unsigned>(ceil(prevSnapshotSize / _params.levelGrowth));
        snapshotTarget = max(snapshotTarget, _params.sizeOfTop);

        while (doAnotherLevel) {
                savedLastLevel = false;

                unsigned prevLevelSize = numClusterable();
                unsigned targetNum = static_cast<unsigned>(ceil(prevLevelSize / _params.clusterRatio));
                targetNum = max(targetNum, snapshotTarget);

                double currentAveClArea = _totalArea / prevLevelSize;
                double currentMaxNewClArea = min(maxNewClRatio * currentAveClArea, maxClArea);
                double currentMaxChildClArea = min(maxChildClRatio * currentAveClArea, maxClArea);

                heavyEdgeMatchingLevel(currentMaxChildClArea, currentMaxNewClArea, targetNum);

                unsigned newLevelSize = numClusterable();

                if (newLevelSize <= _params.sizeOfTop) doAnotherLevel = false;

                if (newLevelSize >= prevLevelSize)  // stuck
                {
                        maxNewClRatio++;
                        maxChildClRatio++;
                        if (numTimesStuck++ >= 4) doAnotherLevel = false;
                }

                if (newLevelSize <= snapshotTarget)  // save this one
                {
                        if (_params.removeDup > 0) removeDuplicateEdges();
                        printStats();
                        addHGraph();
                        prevSnapshotSize = newLevelSize;
                        snapshotTarget = static_cast<unsigned>(ceil(prevSnapshotSize / _params.levelGrowth));

                        if (snapshotTarget <= _params.sizeOfTop * (1.0 + (_params.levelGrowth - 1.0) / 2)) snapshotTarget = _params.sizeOfTop;
                        // close enough..just go ahead
                        // and cluster all the way to top.
                        savedLastLevel = true;

                        abkfatal(snapshotTarget >= _params.sizeOfTop, "snap shot is incorrect");
                }
                level++;
        };

        if (!savedLastLevel) {
                if (_params.removeDup > 0) removeDuplicateEdges();
                printStats();
                addHGraph();
        }

        addTopLevelPartitioning();
}

void PinHEMClusteredHGraph::heavyEdgeMatchingLevel(double maxChildArea, double maxNewArea, unsigned targetNum) {
        // numToCluster is the number of clusters the level should be  reduced
        // by.

        unsigned numToCluster = numClusterable() - targetNum;

        vector<ClHG_Cluster*> clPair(2);
        vector<ClHG_Cluster*>::iterator cl;

        // set all tags to 0  && randomly permute the clusters
        if (_randMap.size() > _toCluster.size())
                _randMap.erase(_randMap.begin() + _toCluster.size(), _randMap.end());
        else if (_randMap.size() < _toCluster.size())
                _randMap.insert(_randMap.end(), _toCluster.size() - _randMap.size(), 0);

        for (unsigned m = 0; m < _toCluster.size(); m++) {
                _randMap[m] = m;
                _toCluster[m]->setTag(0);
        }

        RandomUnsigned rng(0, UINT_MAX);
        std::random_shuffle(_randMap.begin(), _randMap.end(), rng);

        for (unsigned K = 0; K < _randMap.size() && numToCluster > 0; K++) {  // choose a cluster that has not yet been clustered at this
                                                                              // level
                // all unmatched clusters will have a tag of 0

                clPair[0] = _toCluster[_randMap[K]];

                if (clPair[0]->getTag() != 0 || clPair[0]->getDegree() == 0 || !clPair[0]->isClusterable() || clPair[0]->isTerminal() || clPair[0]->getArea() > maxChildArea) continue;

                _adjNodes.clear();

                double c0Area = clPair[0]->getArea();
                double cl0Wt = max(c0Area, _minArea);

                ClHG_CutNetItr n;
                for (n = clPair[0]->cutNetsBegin(); n != clPair[0]->cutNetsEnd(); n++) {

                        ClHG_ClNet& net = *(n->net);
                        if (net.getNumClusters() <= 1 || net.getNumClusters() > 20 || net.getWeight() == 0) continue;

                        double edgeVal = (net.getNumClusters()) ? 2 : 1;
                        edgeVal *= net.getWeight();

                        for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
                                abkassert(*cl != NULL, "null in net iterators");
                                PartitionIds unionPIds = clPair[0]->getAllowableParts();
                                unionPIds &= (*cl)->getAllowableParts();

                                double adjClArea = (*cl)->getArea();
                                double combArea = c0Area + adjClArea;

                                if ((*cl)->getTag() == 0 && (*cl) != clPair[0] && !unionPIds.isEmpty() && (*cl)->isClusterable() && ((combArea <= maxNewArea && adjClArea <= maxChildArea) || c0Area == 0 || adjClArea == 0)) {
                                        double newWt;
                                        double cl1Wt = max((*cl)->getArea(), _minArea / 10.0);

                                        switch (_params.weightOption) {
                                                case 0:  // no area effect
                                                        newWt = edgeVal;
                                                        break;
                                                case 1:  // divide by min
                                                        newWt = edgeVal / min(cl0Wt, cl1Wt);
                                                        break;
                                                case 2:  // divide by max
                                                        newWt = edgeVal / max(cl0Wt, cl1Wt);
                                                        break;
                                                case 3:  // divide by sum
                                                        newWt = edgeVal / (cl0Wt + cl1Wt);
                                                        break;
                                                case 4:  // divide by product
                                                        newWt = edgeVal / (cl0Wt * cl1Wt);
                                                        break;
                                                case 5:  // divide by sqrt(max)
                                                        newWt = edgeVal / sqrt(max(cl0Wt, cl1Wt));
                                                        break;
                                                case 6:  // divide by sqrt(sum)
                                                        newWt = edgeVal / sqrt((cl0Wt + cl1Wt));
                                                        break;
                                                case 7:  // divide by
                                                         // sqrt(product)
                                                        newWt = edgeVal / sqrt((cl0Wt * cl1Wt));
                                                        break;
                                        }
                                        // by sadya. prevent clustering vastly
                                        // differently
                                        // sizes. may be usefull in presence of
                                        // macros

                                        // newWt /= (1+
                                        // ((cl0Wt+cl1Wt)-max(cl0Wt,cl1Wt))/max(cl0Wt,cl1Wt));
                                        // newWt *= max(cl0Wt/cl1Wt,
                                        // cl1Wt/cl0Wt);
                                        // newWt /= max(cl0Wt/cl1Wt,
                                        // cl1Wt/cl0Wt);

                                        _edgeWeights[(*cl)->getIndex()] += newWt;
                                        _adjNodes.push_back(*cl);
                                }
                        }
                }

                clPair[1] = NULL;
                double maxWt = DBL_MIN;
                _equalWtSmallNeighbors.clear();
                double minNeighborArea = DBL_MAX;

                for (cl = _adjNodes.begin(); cl != _adjNodes.end(); cl++) {
                        abkassert(*cl != NULL, "null in adjNodes");

                        if (_edgeWeights[(*cl)->getIndex()] > maxWt) {  // a new best..clear all the ties from the
                                                                        // previous best
                                maxWt = _edgeWeights[(*cl)->getIndex()];
                                _equalWtSmallNeighbors.clear();
                                _equalWtSmallNeighbors.push_back(*cl);
                                minNeighborArea = (*cl)->getArea();
                        } else if (_edgeWeights[(*cl)->getIndex()] == maxWt) {
                                if ((*cl)->getArea() < minNeighborArea)  // new smallest
                                {
                                        _equalWtSmallNeighbors.clear();
                                        _equalWtSmallNeighbors.push_back(*cl);
                                        minNeighborArea = (*cl)->getArea();
                                } else if ((*cl)->getArea() == minNeighborArea)
                                        _equalWtSmallNeighbors.push_back(*cl);
                        }

                        _edgeWeights[(*cl)->getIndex()] = 0.0;
                }

                if (_equalWtSmallNeighbors.size() == 0) continue;  // no matches

                if (_equalWtSmallNeighbors.size() == 1)
                        clPair[1] = _equalWtSmallNeighbors[0];
                else {
                        unsigned chosenNeighbor = _rng % _equalWtSmallNeighbors.size();
                        clPair[1] = _equalWtSmallNeighbors[chosenNeighbor];
                }

                // IMPORTANT.  How you set the tag for this cluster will
                // determine
                // wether or not it can be re-used as a child cluster at this
                // level.
                // If it is given a tag of 0, then it will be 're-used'.
                // If it is given a tag of -1, it will not be re-used.
                // The tag MUST NOT be set to 1, or it will be removed from
                // toCluster at the end of the pass.

                clPair[0]->removeIndNets();
                clPair[1]->removeIndNets();
                ClHG_Cluster* newCl;
                if (clPair[0]->getIndex() < clPair[1]->getIndex())
                    // the lower numbered one will be the terminal, if there
                    // is one
                {
                        clPair[0]->mergeWith(*clPair[1], *this);
                        clPair[1]->setTag(1);  // to be deleted
                        newCl = clPair[0];
                } else {
                        clPair[1]->mergeWith(*clPair[0], *this);
                        clPair[0]->setTag(1);  // to be deleted
                        newCl = clPair[1];
                }

                newCl->induceNets();
                newCl->setTag(5);  // do not allow re-using this on the
                // same level it was created.

                numToCluster--;
        }

        // remove each cl we deleted from _toCluster
        _toCluster.erase(std::remove_if(_toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()), _toCluster.end());
}
