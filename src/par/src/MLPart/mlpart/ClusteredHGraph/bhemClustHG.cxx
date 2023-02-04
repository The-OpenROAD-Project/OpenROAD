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

// created on 10/05/98 by Andrew Caldwell (caldwell@cs.ucla.edu)

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "bhemClustHG.h"

#include <algorithm>

#include "Stats/stats.h"

using std::max;
using std::min;
using std::vector;

#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif

void BestHEMClusteredHGraph::populateTree()
{
  bool doAnotherLevel = true;
  unsigned prevLevelSize = numClusterable();
  unsigned prevSnapshotSize = numClusterable();

  _edgeWeights.insert(_edgeWeights.begin(), getNumLeafNodes(), 0.0);
  _heavyEdges.insert(_heavyEdges.begin(), getNumLeafNodes(), 0.0);

  unsigned snapshotTarget
      = static_cast<unsigned>(ceil(prevSnapshotSize / _params.levelGrowth));

  for (unsigned t = 0; t < _terminals.size(); t++)
    _terminals[t]->setTag(UINT_MAX);  // marked as not clusterable

  printStatsHeader();

  unsigned numTimesStuck = 0;

  // these may be incremented if we get stuck..
  double maxNewClRatio = _params.maxNewClRatio;
  double maxChildClRatio = _params.maxChildClRatio;

  if (maxNewClRatio == 0)
    maxNewClRatio = 1000;
  if (maxChildClRatio == 0)
    maxChildClRatio = 1000;

  // this will never change...
  double maxClArea = _totalArea * (_params.maxClArea / 100.0);
  if (maxClArea == 0)  // 0 == no limit
    maxClArea = _totalArea;

  printStats();

  if (_params.removeDup > 0)
    removeDuplicateEdges();

  bool savedLastLevel;

  while (doAnotherLevel) {
    savedLastLevel = false;

    unsigned targetNum
        = static_cast<unsigned>(ceil(prevLevelSize / _params.clusterRatio));
    targetNum = max(targetNum, snapshotTarget);

    double currentAveClArea = _totalArea / prevLevelSize;
    double currentMaxNewClArea
        = min(maxNewClRatio * currentAveClArea, maxClArea);
    double currentMaxChildClArea
        = min(maxChildClRatio * currentAveClArea, maxClArea);

    heavyEdgeMatchingLevel(
        currentMaxChildClArea, currentMaxNewClArea, targetNum, numTimesStuck);
    unsigned newLevelSize = numClusterable();

    if (newLevelSize <= _params.sizeOfTop)
      doAnotherLevel = false;

    if (newLevelSize >= prevLevelSize)  // stuck
    {
      maxNewClRatio++;
      maxChildClRatio++;
      if (numTimesStuck++ >= 4)
        doAnotherLevel = false;
    }

    prevLevelSize = newLevelSize;

    if (newLevelSize <= snapshotTarget)  // save this one
    {
      if (_params.removeDup > 0)
        removeDuplicateEdges();
      printStats();
      addHGraph();
      prevSnapshotSize = newLevelSize;
      snapshotTarget
          = static_cast<unsigned>(ceil(prevSnapshotSize / _params.levelGrowth));

      if (snapshotTarget
          <= _params.sizeOfTop * (1.0 + (_params.levelGrowth - 1.0) / 2))
        snapshotTarget = _params.sizeOfTop;
      // close enoughy..just go ahead
      // and cluster all the way to top.
      savedLastLevel = true;

      abkfatal(snapshotTarget >= _params.sizeOfTop, "snap shot is incorrect");
    }
  };

  if (!savedLastLevel) {
    if (_params.removeDup > 0)
      removeDuplicateEdges();
    printStats();
    addHGraph();
  }

  addTopLevelPartitioning();
}

void BestHEMClusteredHGraph::heavyEdgeMatchingLevel(double maxChildArea,
                                                    double maxNewArea,
                                                    unsigned targetNum,
                                                    bool useBHEM)
{
  vector<ClHG_Cluster*> clPair(2);

  // set all tags to 0
  vector<ClHG_Cluster*>::iterator cl;
  for (cl = _toCluster.begin(); cl != _toCluster.end(); cl++)
    (*cl)->setTag(0);

  // numToCluster is the number of clusters the level should be  reduced
  // by.
  unsigned numToCluster = numClusterable() - targetNum;

  // randomly permute the clusters
  vector<unsigned> randMap(_toCluster.size());
  for (unsigned m = 0; m < randMap.size(); m++)
    randMap[m] = m;

  RandomUnsigned rng(0, UINT_MAX);

  unsigned maxPasses;
  if (useBHEM)
    maxPasses = 2;
  else
    maxPasses = 1;

  for (; maxPasses > 0; maxPasses--) {
    std::shuffle(randMap.begin(), randMap.end(), rng);

    for (unsigned K = 0; K < randMap.size() && numToCluster > 0;
         K++) {  // choose a cluster that has not yet been clustered
                 // at this level
      // all unmatched clusters will have a tag of 0

      clPair[0] = _toCluster[randMap[K]];

      if (clPair[0]->getTag() != 0 || clPair[0]->getDegree() == 0
          || !clPair[0]->isClusterable() || clPair[0]->getArea() > maxChildArea)
        continue;

      _adjNodes.clear();

      double cl0Wt = max(clPair[0]->getArea(), _minArea);
      abkassert(cl0Wt > 0, "zero weight in BestHEM clustering");

      ClHG_CutNetItr n;
      for (n = clPair[0]->cutNetsBegin(); n != clPair[0]->cutNetsEnd(); n++) {
        ClHG_ClNet& net = *(n->net);
        if (net.getNumClusters() <= 1 || net.getNumClusters() > 10
            || net.getWeight() == 0)
          continue;

        double edgeVal = 1.0 / (double) (net.getNumClusters() + 1);
        edgeVal *= net.getWeight();
        // 1 over k+1 edge weight model

        for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
          PartitionIds unionPIds = clPair[0]->getAllowableParts();
          unionPIds &= (*cl)->getAllowableParts();

          if ((*cl)->getTag() == 0 && (*cl) != clPair[0]
              && (*cl)->getArea() <= maxChildArea
              && (*cl)->getArea() + clPair[0]->getArea() <= maxNewArea
              && !unionPIds.isEmpty() && (*cl)->isClusterable()
              && !(clPair[0]->isTerminal() && (*cl)->isTerminal())) {
            double newWt;
            double cl1Wt = max((*cl)->getArea(), _minArea / 10.0);

            if (maxPasses <= 1)  // not doing BestHEM
              newWt = edgeVal / sqrt((cl0Wt + cl1Wt));
            else
              newWt = edgeVal;

            _edgeWeights[(*cl)->getIndex()] += newWt;
            _adjNodes.push_back(*cl);
          }
        }
      }

      clPair[1] = NULL;
      double maxWt = DBL_MIN;
      vector<ClHG_Cluster*> equalWtNeighbors;
      equalWtNeighbors.reserve(_adjNodes.size());

      // of the adjacent nodes, find the collection of all
      // adjacent nodes connected by the heaviest edge
      // weights,
      // for which that edge weight is also that nodes
      // heaviest edge

      for (cl = _adjNodes.begin(); cl != _adjNodes.end(); cl++) {
        if (_edgeWeights[(*cl)->getIndex()] > maxWt) {
          maxWt = _edgeWeights[(*cl)->getIndex()];
          equalWtNeighbors.clear();
          if (_heavyEdges[(*cl)->getIndex()] == maxWt || maxPasses <= 1)
            equalWtNeighbors.push_back(*cl);
        } else if (_edgeWeights[(*cl)->getIndex()] == maxWt) {
          if (_heavyEdges[(*cl)->getIndex()] == maxWt || maxPasses <= 1)
            equalWtNeighbors.push_back(*cl);
        }
        _edgeWeights[(*cl)->getIndex()] = 0.0;
      }

      if (equalWtNeighbors.size() == 0) {
        _heavyEdges[clPair[0]->getIndex()] = maxWt;
        continue;
      }

      if (equalWtNeighbors.size() == 1)
        clPair[1] = equalWtNeighbors[0];
      else {
        unsigned chosenNeighbor = _rng % equalWtNeighbors.size();
        clPair[1] = equalWtNeighbors[chosenNeighbor];
      }

      // IMPORTANT.  How you set the tag for this cluster will
      // determine
      // wether or not it can be re-used as a child cluster at
      // this level.
      // If it is given a tag of 0, then it will be 're-used'.
      // If it is given a tag of -1, it will not be re-used.
      // The tag MUST NOT be set to 1, or it will be removed
      // from
      // toCluster at the end of the pass.

      clPair[0]->removeIndNets();
      clPair[1]->removeIndNets();
      ClHG_Cluster* newCl;
      if (clPair[0]->getIndex() < clPair[1]->getIndex())
      // the lower numbered one will be the terminal, if
      // there
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
      _heavyEdges[newCl->getIndex()] = 0;

      numToCluster--;
    }
  }
  // remove each cl we deleted from _toCluster
  _toCluster.erase(
      std::remove_if(
          _toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()),
      _toCluster.end());
}
