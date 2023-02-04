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

#include "grhemClustHG.h"

#include <algorithm>

#include "Stats/stats.h"

using std::max;
using std::min;
using std::vector;

void GreedyHEMClusteredHGraph::populateTree()
{
  // reserve the vectors.
  _randMap.reserve(getNumLeafNodes());
  _edgeWeights.insert(_edgeWeights.begin(), getNumLeafNodes(), 0.0);
  _adjNodes.reserve(200);
  _equalWtSmallNeighbors.reserve(200);

  for (unsigned t = 0; t < _terminals.size(); t++)
    _terminals[t]->setTag(UINT_MAX);  // marked as not clusterable

  // these may be incremented if we get stuck..
  double maxNewClRatio = _params.maxNewClRatio;
  double maxChildClRatio = _params.maxChildClRatio;

  if (maxNewClRatio == 0)
    maxNewClRatio = 10000;
  if (maxChildClRatio == 0)
    maxChildClRatio = 10000;

  // this will never change...
  double maxClArea = _totalArea * (_params.maxClArea / 100.0);
  if (maxClArea == 0)  // 0 == no limit
    maxClArea = _totalArea;

  printStatsHeader();
  printStats();
  // clusterDegree1Nodes();
  // clusterDegree2Nodes();
  // printStats();

  if (_params.removeDup > 0)
    removeDuplicateEdges();

  unsigned numTimesStuck = 0;
  bool savedLastLevel;
  bool doAnotherLevel = true;
  unsigned prevSnapshotSize = numClusterable();
  unsigned snapshotTarget
      = static_cast<unsigned>(ceil(prevSnapshotSize / _params.levelGrowth));
  snapshotTarget = max(snapshotTarget, _params.sizeOfTop);

  while (doAnotherLevel) {
    savedLastLevel = false;

    unsigned prevLevelSize = numClusterable();
    unsigned targetNum
        = static_cast<unsigned>(ceil(prevLevelSize / _params.clusterRatio));
    targetNum = max(targetNum, snapshotTarget);

    double currentAveClArea = _totalArea / prevLevelSize;
    double currentMaxNewClArea
        = min(maxNewClRatio * currentAveClArea, maxClArea);
    double currentMaxChildClArea
        = min(maxChildClRatio * currentAveClArea, maxClArea);

    heavyEdgeMatchingLevel(
        currentMaxChildClArea, currentMaxNewClArea, targetNum);

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
      // close enough..just go ahead
      // and cluster all the way to top.
      savedLastLevel = true;

      abkfatal(snapshotTarget >= _params.sizeOfTop, "snap shot is incorrect");

      // numDeg1 = numDeg2 = 0;
      // for(x = 0; x < _toCluster.size(); x++)
      //{
      //	if(_toCluster[x]->getDegree() == 1)
      //	  numDeg1++;
      //	if(_toCluster[x]->getDegree() == 2)
      // 	  numDeg2++;
      //}
      //	cout<<"Num Degree1: "<<numDeg1<<" Degree2:
      //"<<numDeg2<<endl;
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

void GreedyHEMClusteredHGraph::heavyEdgeMatchingLevel(double maxChildArea,
                                                      double maxNewArea,
                                                      unsigned targetNum)
{
  // numToCluster is the number of clusters the level should be  reduced
  // by.

  unsigned numToCluster = numClusterable() - targetNum;

  // set all tags to 0
  for (unsigned m = 0; m < _toCluster.size(); m++)
    _toCluster[m]->setTag(0);

  _matchRecs.clear();

  // compute the best matches for each cluster
  for (unsigned K = 0; K < _toCluster.size(); K++) {
    ClHG_Cluster* cl0 = _toCluster[K];

    if (cl0->getTag() != 0 || cl0->getDegree() == 0 || !cl0->isClusterable()
        || cl0->isTerminal() || cl0->getArea() > maxChildArea)
      continue;

    _adjNodes.clear();

    double c0Area = cl0->getArea();
    double cl0Wt = max(c0Area, _minArea);

    ClHG_CutNetItr n;
    for (n = cl0->cutNetsBegin(); n != cl0->cutNetsEnd(); n++) {
      ClHG_ClNet& net = *(n->net);
      if (net.getNumClusters() <= 1 || net.getNumClusters() > 10
          || net.getWeight() == 0)
        continue;

      double edgeVal = 1.0 / (double) (net.getNumClusters());
      edgeVal *= net.getWeight();
      //(1/k) edge weight model

      ClHG_NetClItr cl;
      for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
        abkassert(*cl != NULL, "null in net iterators");
        PartitionIds unionPIds = cl0->getAllowableParts();
        unionPIds &= (*cl)->getAllowableParts();

        double adjClArea = (*cl)->getArea();
        double combArea = c0Area + adjClArea;

        if ((*cl)->getTag() == 0 && (*cl) != cl0 && !unionPIds.isEmpty()
            && (*cl)->isClusterable()
            && ((combArea <= maxNewArea && adjClArea <= maxChildArea)
                || c0Area == 0 || adjClArea == 0)) {
          double newWt = edgeVal;
          double cl1Wt = max((*cl)->getArea(), _minArea / 10.0);

          switch (_params.weightOption) {
            case 1:  // divide by min
              newWt /= min(cl0Wt, cl1Wt);
              break;
            case 2:  // divide by max
              newWt /= max(cl0Wt, cl1Wt);
              break;
            case 3:  // divide by sum
              newWt /= (cl0Wt + cl1Wt);
              break;
            case 4:  // divide by product
              newWt /= (cl0Wt * cl1Wt);
              break;
            case 5:  // divide by sqrt(max)
              newWt /= sqrt(max(cl0Wt, cl1Wt));
              break;
            case 6:  // divide by sqrt(sum)
              newWt /= sqrt((cl0Wt + cl1Wt));
              break;
            case 7:  // divide by
                     // sqrt(product)
              newWt /= sqrt((cl0Wt * cl1Wt));
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

    double maxWt = DBL_MIN;
    _equalWtSmallNeighbors.clear();
    double minNeighborArea = DBL_MAX;

    vector<ClHG_Cluster*>::iterator cl;
    for (cl = _adjNodes.begin(); cl != _adjNodes.end(); cl++) {
      abkassert(*cl != NULL, "null in adjNodes");

      if (_edgeWeights[(*cl)->getIndex()]
          > maxWt) {  // a new best..clear all the ties from the
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

    for (unsigned r = 0; r < _equalWtSmallNeighbors.size(); r++)
      _matchRecs.push_back(
          HEMMatchRecord(cl0, _equalWtSmallNeighbors[r], maxWt));
  }

  RandomUnsigned rng(0, UINT_MAX);
  std::shuffle(_matchRecs.begin(), _matchRecs.end(), rng);

  std::sort(_matchRecs.begin(), _matchRecs.end());

  // IMPORTANT.  How you set the tag for this cluster will determine
  // wether or not it can be re-used as a child cluster at this level.
  // If it is given a tag of 0, then it will be 're-used'.
  // If it is given a tag of -1, it will not be re-used.
  // The tag MUST NOT be set to 1, or it will be removed from
  // toCluster at the end of the pass.

  // double maxMatchWt = 0;
  // double minMatchWt = DBL_MAX;

  unsigned matchId = 0;
  unsigned numMatches = 0;
  double minAllowedWt = _matchRecs[_matchRecs.size() / 2].matchWeight;

  // while(numToCluster > 0 && matchId < _matchRecs.size())
  while (matchId < _matchRecs.size()
         && _matchRecs[matchId].matchWeight >= minAllowedWt) {
    const HEMMatchRecord& mRec = _matchRecs[matchId];

    if (mRec.cl1->getTag() == 0 && mRec.cl2->getTag() == 0) {
      mRec.cl1->removeIndNets();
      mRec.cl2->removeIndNets();
      ClHG_Cluster* newCl;
      if (mRec.cl1->getIndex() < mRec.cl2->getIndex())
      // the lower numbered one will be the terminal, if
      // there
      // is one
      {
        mRec.cl1->mergeWith(*mRec.cl2, *this);
        mRec.cl2->setTag(1);  // to be deleted
        newCl = mRec.cl1;
      } else {
        mRec.cl2->mergeWith(*mRec.cl1, *this);
        mRec.cl1->setTag(1);  // to be deleted
        newCl = mRec.cl2;
      }

      newCl->induceNets();
      newCl->setTag(5);  // do not allow re-using this on the
      // same level it was created.
      numToCluster--;
      // maxMatchWt = max(maxMatchWt, mRec.matchWeight);
      // minMatchWt = min(maxMatchWt, mRec.matchWeight);
      numMatches++;
    }
    matchId++;
  }

  // cout<<"Of "<<_matchRecs.size()<<" possible matches,"
  //         <<" "<<numMatches<<" were made"<<endl;
  // cout<<" Max Wt of matches taken: "<<maxMatchWt<<endl;
  // cout<<" Min Wt of matches taken: "<<minMatchWt<<endl;

  // remove each cl we deleted from _toCluster
  _toCluster.erase(
      std::remove_if(
          _toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()),
      _toCluster.end());
}
