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

#include "hemClustHG.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Stats/freqDistr.h"
#include "Stats/stats.h"

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::ostream;
using std::stringstream;
using std::vector;

void HEMClusteredHGraph::populateTree()
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
    maxNewClRatio = 1000;
  if (maxChildClRatio == 0)
    maxChildClRatio = 1000;

  // this will never change...
  double maxClArea = _totalArea * (_params.maxClArea / 100.0);
  if (maxClArea == 0)  // 0 == no limit
    maxClArea = _totalArea;

  if (_params.verb.getForMajStats())
    cout << "HEM Clustering" << endl;

  printStatsHeader();
  printStats();
  clusterDegree1Nodes();
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

  if (prevSnapshotSize <= _params.sizeOfTop)
    abkwarn(1,
            "Clustering a hypergraph that is smaller than "
            "sizeOfTop");  // we are already smaller than we want to
                           // go

  while (doAnotherLevel) {
    if (_params.verb.getForMajStats() > 10)
      cout << "Starting another HEM level" << endl;

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

    // debugging check
    // if(prevLevelSize < targetNum)
    //{
    //   cout<<"Fatal error: prevLevelSize < targetNum"<<endl;
    //   cout<<"prevLevelSize: "<<prevLevelSize<<endl;
    //   cout<<"targetNum: "<<targetNum<<endl;
    //   cout<<"snapshotTarget: "<<snapshotTarget<<endl;
    //   cout<<"clusterRatio: "<<_params.clusterRatio<<endl;
    //   cout<<"prevSnapshotSize: "<<prevSnapshotSize<<endl;
    //   cout<<"_params.levelGrowth: "<< _params.levelGrowth<< endl;
    //   cout<<"_params.sizeOfTop: "<< _params.sizeOfTop<< endl;
    //   abkfatal(0, "Debug me");
    //}

    // cout<<"Calling heavyEdgeMatchingLevel with
    // currentMaxChildClArea: "<<currentMaxChildClArea<<
    //                                         "
    // currentMaxNewClArea: "<<currentMaxNewClArea<<
    //                                         " targetNum:
    // "<<targetNum<<
    //                                         " prevLevelSize:
    // "<<prevLevelSize<<endl;
    heavyEdgeMatchingLevel(
        currentMaxChildClArea, currentMaxNewClArea, targetNum);

    unsigned newLevelSize = numClusterable();

    if (_params.verb.getForMajStats() > 10)
      cout << "NewLevelSize: " << newLevelSize << endl
           << "SizeOfTop:    " << _params.sizeOfTop << endl;

    if (newLevelSize <= _params.sizeOfTop)
      doAnotherLevel = false;

    if (newLevelSize >= prevLevelSize)  // stuck
    {
      maxNewClRatio++;
      maxChildClRatio++;
      if (numTimesStuck++ >= 4)
        doAnotherLevel = false;
    }

    // cout << "DPDEBUG: newLevelSize: "<<newLevelSize<< "
    // snapshotTarget: "<<snapshotTarget<<" "<< endl;

    if (newLevelSize <= snapshotTarget)  // save this one
    {
      if (_params.removeDup > 0)
        removeDuplicateEdges();
      printStats();
      addHGraph();
      prevSnapshotSize = newLevelSize;
      snapshotTarget = int(ceil(prevSnapshotSize / _params.levelGrowth));

      if (snapshotTarget
          <= _params.sizeOfTop * (1.0 + (_params.levelGrowth - 1.0) / 2))
        snapshotTarget = _params.sizeOfTop;
      // close enough..just go ahead
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
  //_clusteredHGraphs.back()->saveAsNodesNetsWts("TopLevelHGraph");
}

void HEMClusteredHGraph::heavyEdgeMatchingLevel(double maxChildArea,
                                                double maxNewArea,
                                                unsigned targetNum)
{
  // numToCluster is the number of clusters the level should be  reduced
  // by.
  // cout<<"DPDEBUG: Calling heavy edge matching level numClusterable:
  // "<<numClusterable()<<endl;
  // cout<<"DPDEBUG: Total area of all clusters: "<<_totalArea<<endl;

  int numToCluster = numClusterable() - targetNum;
  // cout<<"numClusterable: "<<numClusterable()<<" targetNum
  // "<<targetNum<<endl;
  // if(numToCluster == 0) return;
  // if(numToCluster < 0)
  //{
  //    abkwarn(numToCluster < 0, "Clustering target is greater than
  // current number of nodes ");
  //    return;
  //}
  // int origNumToCluster = numToCluster;

  if (_params.verb.getForMajStats() > 20)
    cout << "numToCluster: " << numToCluster << endl;

  vector<ClHG_Cluster*> clPair(2);
  vector<ClHG_Cluster*> degreeZeroClusters;
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

  std::shuffle(_randMap.begin(), _randMap.end(), _rng);

  unsigned skipct = 0, notskipped = 0;
  for (unsigned K = 0; K < _randMap.size() && numToCluster > 0;
       K++) {  // choose a cluster that has not yet been clustered at this
               // level
    // all unmatched clusters will have a tag of 0

    // verify(numToCluster, origNumToCluster);

    clPair[0] = _toCluster[_randMap[K]];

    if (_params.verb.getForMajStats() > 20)
      cout << "Choose node " << clPair[0]->getIndex() << " for matching."
           << "  It has degree " << clPair[0]->getDegree() << endl;

    PartitionIds cl0unionPIds = clPair[0]->getAllowableParts();
    if (clPair[0]->getDegree() == 0 && cl0unionPIds.numberPartitions() > 1) {
      // if(!fixed)
      degreeZeroClusters.push_back(clPair[0]);
      continue;
    }

    if (isUnclusterable(clPair[0], maxChildArea)) {
      // cout<<"Ruling out cluster possibility of this
      // node:"<<endl;
      // if(clPair[0]->getTag() != 0)
      //    cout<<"clPair0tag (should be 0):
      // "<<clPair[0]->getTag()<<endl;

      // if(clPair[0]->getDegree() == 0)
      //    cout<<"clPair0 degree (should not be 0):
      // "<<clPair[0]->getDegree()<<endl;

      // if(!clPair[0]->isClusterable())
      //    cout<<"clPair0 clusterable (should be
      // clusterable): "<<clPair[0]->isClusterable()<<endl;

      // if( clPair[0]->isTerminal())
      //    cout<<"clPair0 is terminal? (should be not
      // terminal): "<<clPair[0]->isTerminal()<<endl;

      // if( clPair[0]->getArea() > maxChildArea)
      //    cout<<"clPair0 area (should be less than "<<
      // maxChildArea <<"): "<<clPair[0]->getArea()<<endl;

      ++skipct;
      continue;
    } else
      ++notskipped;

    if (_params.verb.getForMajStats() > 20)
      cout << " Node passed first test for clusterability" << endl;

    _adjNodes.clear();

    // double c0Area = clPair[0]->getArea();
    // double cl0Wt = max(c0Area, _minArea);

    // preprocess a node to check if all of its connections have
    // high-degree
    //(>30) If it does, we will do a special computation that
    // chooses a
    // random edge, and a random node on that edge to cluster to,
    // since all
    // connections are very weak anyway.
    ClHG_CutNetItr n;
    bool allLargeNets = true;
    for (n = clPair[0]->cutNetsBegin(); n != clPair[0]->cutNetsEnd(); n++) {
      allLargeNets = allLargeNets && ((n->net)->getDegree() > 30);
      if (!allLargeNets)
        break;
    }
    if (allLargeNets) {
      abkassert(clPair[0], "clPair[0] net is NULL");
      const unsigned max_tries = 100;
      for (unsigned i = 0; i < max_tries; ++i) {
        unsigned netIdx
            = _rng % (clPair[0]->cutNetsEnd() - clPair[0]->cutNetsBegin());
        // cout<<"DPDEBUG: netIdx: "<<netIdx<<endl;
        ClHG_CutNetItr rn;
        rn = clPair[0]->cutNetsBegin() + netIdx;
        ClHG_ClNet* randNet = rn->net;
        abkassert(randNet, "Random net is NULL");

        unsigned nodeIdx
            = _rng % (randNet->clustersEnd() - randNet->clustersBegin());
        // cout<<"DPDEBUG: nodeIdx: "<<nodeIdx<<endl;
        cl = randNet->clustersBegin() + nodeIdx;
        clPair[1] = *cl;
        // cout<<"DPDEBUG: clPair[0]: "<<
        // (clPair[0])->getIndex() <<" cl:
        // "<<(*cl)->getIndex()<<endl;
        if (clPair[0] != clPair[1] && !isUnclusterable(clPair[0], maxChildArea)
            && !isUnclusterable(clPair[1], maxChildArea)
            && !differentPartitions(clPair[0], clPair[1])) {
          // for(ClHG_CutNetItr rNet =
          // clPair[0]->cutNetsBegin(); rNet !=
          // clPair[0]->cutNetsEnd(); ++rNet)
          //{
          //   ClHG_ClNet& net = *(rNet->net);
          //   cout<<"Nodes on net:
          // "<<net.getIndex()<<endl;
          //   for(vector<ClHG_Cluster*>::iterator
          // cli = net.clustersBegin();cli !=
          // net.clustersEnd(); cli++)
          //   {
          //      cout<<"
          // "<<(*cli)->getIndex()<<endl;
          //   }
          //   cout<<endl;
          //}
          mergeNodes(clPair[0], clPair[1]);
          --numToCluster;
          break;
        }
      }
    } else  // some nets have small degree, normal computation
    {
      for (n = clPair[0]->cutNetsBegin(); n != clPair[0]->cutNetsEnd(); ++n) {
        ClHG_ClNet& net = *(n->net);
        if (net.getNumClusters() <= 1 || net.getNumClusters() > 30
            || net.getWeight() == 0) {
          //                    if(net.getWeight()
          // == 0)
          //                        cout<<"Ignoring
          // a net because its weight is zero
          // (DPDEBUG2243)!!!"<<endl;
          continue;
        }

        for (cl = net.clustersBegin(); cl != net.clustersEnd(); ++cl) {
          abkassert(*cl != NULL, "null in net iterators");
          if (isCompatiblePair(clPair[0], (*cl), maxChildArea, maxNewArea)) {
            double newWt = getConnectionWeight(net, clPair[0], (*cl));

            // by sadya. prevent clustering
            // vastly differently
            // sizes. may be useful in
            // presence of macros

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
    }

    clPair[1] = NULL;

    if (_params.verb.getForMajStats() > 20)
      cout << "  There are " << _adjNodes.size() << " adjacent nodes" << endl;

    computeEqualWtSmallNeighbors();

    if (_equalWtSmallNeighbors.size() == 0)
      continue;  // no matches
    else if (_equalWtSmallNeighbors.size() == 1)
      clPair[1] = _equalWtSmallNeighbors[0];
    else {
      unsigned chosenNeighbor = _rng % _equalWtSmallNeighbors.size();
      clPair[1] = _equalWtSmallNeighbors[chosenNeighbor];
    }

    mergeNodes(clPair[0], clPair[1]);

    numToCluster--;
  }

  // cluster the degree zero nodes together until they are as close to
  // average size
  // without going over as possible
  // unsigned numClusters = _toCluster.size();
  // double avgClustSize = _totalArea / numClusters;
  // double totalZeroDegreeClusterArea = 0.;
  // unsigned from=0;
  // double currArea = 0;
  // unsigned zeroCt = 0;
  // cout<<"Found "<<degreeZeroClusters.size()<<" degree 0 clusters
  // "<<endl;
  // for(unsigned i = 0; i < degreeZeroClusters.size(); ++i)
  //{
  //    if(currArea + degreeZeroClusters[i]->getArea() < avgClustSize)
  //        currArea += degreeZeroClusters[i]->getArea();
  //    else
  //    {
  //        for(unsigned j = from+1; j < i; ++j)
  //        {
  //            mergeNodes(degreeZeroClusters[from],
  // degreeZeroClusters[j]);
  //        }
  //        ++zeroCt;
  //        from = i;
  //        currArea = degreeZeroClusters[i]->getArea();
  //    }
  //}
  // if(from < degreeZeroClusters.size() - 1)
  //{
  //    for(unsigned j = from+1; j < degreeZeroClusters.size(); ++j)
  //    {
  //        mergeNodes(degreeZeroClusters[from], degreeZeroClusters[j]);
  //    }
  //    ++zeroCt;
  //}
  // cout<<"Merged them into "<<zeroCt<<" clusters "<<endl;

  // cout<<"Skipped "<<skipct<<" nodes, and didnt skip "<<notskipped<<"
  // didnt manage to cluster: "<<numToCluster<<" From an orignal goal of
  // "<<origNumToCluster<<endl;
  // abkfatal( numToCluster >= 0, "Negative numbers bad here");
  // abkfatal( origNumToCluster >= 0, "Negative numbers bad here");
  // verify(numToCluster, origNumToCluster, maxChildArea);

  // if we didnt find as many nodes merge as we wanted, then in a 2nd
  // pass,
  // allow small nodes to merge with large nodes, and relax the size
  // constraint
  if (numToCluster > 0) {
    // cout<<"DPDEBUG 2nd being called!"<<endl;
    _adjNodes.clear();

    // verify(numToCluster, origNumToCluster, maxChildArea);
    for (unsigned K = 0; K < _randMap.size() && numToCluster > 0;
         K++) {  // choose a cluster that has not yet been clustered
                 // at this level
      // all unmatched clusters will have a tag of 0

      clPair[0] = _toCluster[_randMap[K]];

      if (isUnclusterable(clPair[0], maxChildArea))
        continue;

      ClHG_CutNetItr n;
      for (n = clPair[0]->cutNetsBegin(); n != clPair[0]->cutNetsEnd(); n++) {
        ClHG_ClNet& net = *(n->net);
        if (net.getNumClusters() <= 1 || net.getNumClusters() > 100
            || net.getWeight() == 0)
          continue;

        for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
          abkassert(*cl != NULL, "null in net iterators");
          if (isCompatiblePairRelaxed(clPair[0], *cl, maxNewArea)) {
            double newWt = getConnectionWeight(net, clPair[0], (*cl));

            _edgeWeights[(*cl)->getIndex()] += newWt;
            _adjNodes.push_back(*cl);
          }
        }
      }

      computeEqualWtSmallNeighbors();

      if (_equalWtSmallNeighbors.size() == 0)
        continue;  // no matches
      else if (_equalWtSmallNeighbors.size() == 1)
        clPair[1] = _equalWtSmallNeighbors[0];
      else {
        unsigned chosenNeighbor = _rng % _equalWtSmallNeighbors.size();
        clPair[1] = _equalWtSmallNeighbors[chosenNeighbor];
      }

      mergeNodes(clPair[0], clPair[1]);

      numToCluster--;
    }
  }

  // if we still didnt find as many nodes merge as we wanted, then in a
  // 3rd
  // pass, allow nodes to merge with 2-hop nodes
  // if(numToCluster != 0)
  //{
  //    //cout<<"DPDEBUG 3rd being called!"<<endl;
  //    _adjNodes.clear();

  //    //verify(numToCluster, origNumToCluster, maxChildArea);
  //    for(unsigned K = 0; K < _randMap.size() && numToCluster > 0; K++)
  //    {   //choose a cluster that has not yet been clustered at this
  // level
  //        //all unmatched clusters will have a tag of 0

  //        clPair[0] = _toCluster[_randMap[K]];

  //        if(isUnclusterable(clPair[0], maxChildArea)) continue;

  //
  //        ClHG_CutNetItr n;
  //        for(n= clPair[0]->cutNetsBegin();n !=clPair[0]->cutNetsEnd();
  // n++)
  //        {

  //            ClHG_ClNet& net = *(n->net);
  //            if(net.getNumClusters() <= 1 || net.getNumClusters() > 100
  // || net.getWeight() == 0)
  //                continue;

  //            for(cl = net.clustersBegin();cl != net.clustersEnd();
  // cl++)
  //            {
  //                abkassert(*cl != NULL, "null in net iterators");
  //                if(isLarge(*cl, maxChildArea) &&
  // !differentPartitions(clPair[0], (*cl)))
  //                {
  //                    double newWt = getConnectionWeight(net, clPair[0],
  // (*cl));

  //                    _edgeWeights[(*cl)->getIndex()] += newWt;
  //                    _adjNodes.push_back(*cl);
  //                }
  //            }
  //        }

  //        double maxWt = DBL_MIN;
  //        _equalWtSmallNeighbors.clear();
  //        for(cl = _adjNodes.begin(); cl != _adjNodes.end(); cl++)
  //        {
  //            abkassert(*cl != NULL, "null in adjNodes");

  //            if(_edgeWeights[(*cl)->getIndex()] > maxWt)
  //            {	//a new best..clear all the ties from the previous best
  //                maxWt  = _edgeWeights[(*cl)->getIndex()];
  //                _equalWtSmallNeighbors.clear();
  //                _equalWtSmallNeighbors.push_back(*cl);
  //            }
  //            else if(_edgeWeights[(*cl)->getIndex()] == maxWt)
  //            {
  //                _equalWtSmallNeighbors.push_back(*cl);
  //            }

  //            _edgeWeights[(*cl)->getIndex()] = 0.0;
  //        }

  //        ClHG_Cluster* oneHop;
  //        if(_equalWtSmallNeighbors.size() == 0)
  //            continue;	//no matches
  //        else if(_equalWtSmallNeighbors.size() == 1)
  //            oneHop = _equalWtSmallNeighbors[0];
  //        else
  //        {
  //            unsigned chosenNeighbor = _rng %
  // _equalWtSmallNeighbors.size();
  //            oneHop = _equalWtSmallNeighbors[chosenNeighbor];
  //        }
  //
  //        //cout<<"DPDEBUG 3rd chose a 1 hop neighbor!"<<endl;

  //
  //        clPair[1] = findBestNeighborExcept(oneHop, clPair[0],
  // maxChildArea);
  //        if(clPair[1] == 0)
  //            continue; // no matches

  //        //cout<<"DPDEBUG 3rd Pass doing something useful!"<<endl;
  //        //exit(1);
  //        mergeNodes(clPair[0], clPair[1]);

  //        numToCluster--;

  //    }

  //  }

  // cout<<"After pass 3 didnt manage to cluster: "<<numToCluster<<" From
  // an orignal goal of "<<origNumToCluster<<endl;

  // remove each cl we deleted from _toCluster
  _toCluster.erase(
      std::remove_if(
          _toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()),
      _toCluster.end());
}

// IMPORTANT.  How you set the tag for this cluster will determine
// wether or not it can be re-used as a child cluster at this level.
// If it is given a tag of 0, then it will be 're-used'.
// If it is given a tag of -1, it will not be re-used.
// The tag MUST NOT be set to 1, or it will be removed from
// toCluster at the end of the pass.
void HEMClusteredHGraph::mergeNodes(ClHG_Cluster* a, ClHG_Cluster* b)
{
  abkfatal(a != b, "Cannot merge a node with itself");
  a->removeIndNets();
  b->removeIndNets();
  ClHG_Cluster* newCl;
  if (a->getIndex() < b->getIndex())
  // the lower numbered one will be the terminal, if there
  // is one
  {
    a->mergeWith(*b, *this);
    b->setTag(1);  // to be deleted
    newCl = a;
  } else {
    b->mergeWith(*a, *this);
    a->setTag(1);  // to be deleted
    newCl = b;
  }

  newCl->induceNets();
  newCl->setTag(5);  // do not allow re-using this on the
                     // same level it was created.
}

inline bool HEMClusteredHGraph::isUnclusterable(ClHG_Cluster* cl,
                                                double maxChildArea)
{
  if (cl->getTag() != 0 || cl->getDegree() == 0 || !cl->isClusterable()
      || cl->isTerminal() || cl->getArea() > maxChildArea)
    return true;
  else
    return false;
}

inline bool HEMClusteredHGraph::isCompatiblePair(ClHG_Cluster* cl0,
                                                 ClHG_Cluster* cl1,
                                                 double maxChildArea,
                                                 double maxNewArea)
{
  if (cl1 != cl0 && cl1->isClusterable() && cl1->getTag() == 0) {
    PartitionIds unionPIds = cl0->getAllowableParts();
    unionPIds &= cl1->getAllowableParts();

    if (unionPIds.isEmpty()) {
      return false;
    } else {
      double c0Area = cl0->getArea();
      double adjClArea = cl1->getArea();
      double combArea = c0Area + adjClArea;

      return (c0Area == 0. || adjClArea == 0.
              || (combArea <= maxNewArea && adjClArea <= maxChildArea));
    }
  } else {
    return false;
  }
}

inline bool HEMClusteredHGraph::isCompatiblePairRelaxed(ClHG_Cluster* cl0,
                                                        ClHG_Cluster* cl1,
                                                        double maxNewArea)
{
  if (cl1 != cl0 && cl1->isClusterable() && cl1->getTag() == 0) {
    PartitionIds unionPIds = cl0->getAllowableParts();
    unionPIds &= cl1->getAllowableParts();

    if (unionPIds.isEmpty()) {
      return false;
    } else {
      double c0Area = cl0->getArea();
      double adjClArea = cl1->getArea();
      double combArea = c0Area + adjClArea;

      return (c0Area == 0. || adjClArea == 0. || combArea <= maxNewArea);
    }
  } else {
    return false;
  }
}

double HEMClusteredHGraph::getConnectionWeight(const ClHG_ClNet& net,
                                               const ClHG_Cluster* cl0,
                                               const ClHG_Cluster* cl1)
{
  double edgeVal = net.getWeight() / static_cast<double>(net.getNumClusters());

  //(1/k) edge weight model
  double c0Area = cl0->getArea();
  double cl0Wt = max(c0Area, _minArea);
  double cl1Wt = max(cl1->getArea(), 0.1 * _minArea);

  double rval = 0;
  switch (_params.weightOption) {
    case 0:  // no area effect
      rval = edgeVal;
      break;
    case 1:  // divide by min
      rval = edgeVal / min(cl0Wt, cl1Wt);
      break;
    case 2:  // divide by max
      rval = edgeVal / max(cl0Wt, cl1Wt);
      break;
    case 3:  // divide by sum
      rval = edgeVal / (cl0Wt + cl1Wt);
      break;
    case 4:  // divide by product
      rval = edgeVal / (cl0Wt * cl1Wt);
      break;
    case 5:  // divide by sqrt(max)
      rval = edgeVal / sqrt(max(cl0Wt, cl1Wt));
      break;
    case 6:  // divide by sqrt(sum)
      rval = edgeVal / sqrt((cl0Wt + cl1Wt));
      break;
    case 7:  // divide by sqrt(product)
      rval = edgeVal / sqrt((cl0Wt * cl1Wt));
      break;
  }
  return rval;
}

void HEMClusteredHGraph::printAllReasonsForUnclusterable(ClHG_Cluster* cl0,
                                                         double maxChildArea,
                                                         double maxNewArea)
{
  stringstream ss;
  ClHG_CutNetItr n;
  for (n = cl0->cutNetsBegin(); n != cl0->cutNetsEnd(); n++) {
    ClHG_ClNet& net = *(n->net);
    if (net.getNumClusters() <= 1 || net.getNumClusters() > 30
        || net.getWeight() == 0) {
      // if(net.getWeight() == 0)
      //    cout<<"Ignoring a net because its weight is zero
      // (DPDEBUG2243)!!!"<<endl;
      continue;
    }

    vector<ClHG_Cluster*>::iterator cl;
    for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
      abkassert(*cl != NULL, "null in net iterators");

      if (isCompatiblePair(cl0, (*cl), maxChildArea, maxNewArea)) {
        ss << "Able to cluster clPair[0]: " << cl0->getIndex()
           << " to node cl: " << (*cl)->getIndex() << endl;
      } else {
        ss << "Cannot cluster clPair[0]: " << cl0->getIndex()
           << " to node cl: " << (*cl)->getIndex() << " because: " << endl;
        PartitionIds unionPIds = cl0->getAllowableParts();
        unionPIds &= (*cl)->getAllowableParts();

        double adjClArea = (*cl)->getArea();
        double c0Area = cl0->getArea();
        double combArea = c0Area + adjClArea;
        if ((*cl)->getTag() != 0)
          ss << " cl cannot cluster to anything "
                "due to non-zero tag: "
             << (*cl)->getTag() << endl;
        if ((*cl) == cl0)
          ss << " cl and clPair[0] are the same "
                "node"
             << endl;
        if (unionPIds.isEmpty())
          ss << " cl and clPair[0] are in "
                "different partitions"
             << endl;
        if (!(*cl)->isClusterable())
          ss << " cl !isClusterable()" << endl;
        if (!(combArea <= maxNewArea))
          ss << " the resulting node is too "
                "large :"
             << combArea << " the limit is: " << maxNewArea << endl;
        if (!(adjClArea <= maxChildArea))
          ss << " cl is too large: " << adjClArea
             << " maximum is: " << maxChildArea << endl;
      }
    }
  }
  cout << ss.str() << endl;
}

void HEMClusteredHGraph::verify(unsigned numToCluster,
                                unsigned origNumToCluster,
                                double maxChildArea)
{
  unsigned num1 = 0, num5 = 0;
  for (unsigned m = 0; m < _toCluster.size(); m++) {
    unsigned tag = _toCluster[m]->getTag();
    if (tag == 1)
      ++num1;
    else if (tag == 5)
      ++num5;
  }
  cout << "Num 1: " << num1 << endl;
  cout << "Num 5: " << num5 << endl;
  cout << "NumToClusteR: " << numToCluster << endl;
  cout << "origNumToClusteR: " << origNumToCluster << endl;
  //    if(num1 != num5)
  //      exit(1);

  vector<double> clusterAreas;
  for (unsigned K = 0; K < _randMap.size(); K++) {
    clusterAreas.push_back(_toCluster[_randMap[K]]->getArea());
  }
  CumulativeFrequencyDistribution stats(clusterAreas, 20);
  cout << "Equal subranges: " << endl;
  stats.printEqualSubranges(cout);
  cout << "Equipotent subranges: " << endl;
  stats.printEquipotentSubranges(cout);
  TrivialStatsWithStdDev statsb(clusterAreas);
  cout << "Trivial stats: " << endl;
  cout << statsb << endl;

  unsigned clusterableCt = 0;
  unsigned numNoClusterableNbor = 0;
  vector<double> avgMinNbor;
  for (unsigned K = 0; K < _randMap.size(); K++) {
    if (!isUnclusterable(_toCluster[_randMap[K]], maxChildArea)) {
      ++clusterableCt;
      double minN = min0tagNeighbor(_toCluster[_randMap[K]]);
      if (minN == -1.0)
        ++numNoClusterableNbor;
      else
        avgMinNbor.push_back(minN);
    }
  }
  cout << "Number of nodes with no clusterable neighbor: "
       << numNoClusterableNbor << " clusterableCt: " << clusterableCt
       << " which is " << (double(numNoClusterableNbor) / clusterableCt * 100)
       << "%" << endl;
  if (avgMinNbor.size() > 0) {
    TrivialStatsWithStdDev statsc(avgMinNbor);
    cout << "Stats of size of smallest clusterable neighbor: " << endl;
    cout << statsc << endl;
  } else
    cout << "No nodes have clusterable neighbors." << endl;
  cout << "Max child " << maxChildArea << endl;
}

double HEMClusteredHGraph::min0tagNeighbor(ClHG_Cluster* cl0)
{
  double leastNeighbor = std::numeric_limits<double>::max();
  ClHG_CutNetItr n;
  for (n = cl0->cutNetsBegin(); n != cl0->cutNetsEnd(); n++) {
    ClHG_ClNet& net = *(n->net);
    // if(net.getNumClusters() <= 1 || net.getNumClusters() > 30 ||
    //        net.getWeight() == 0)
    //{
    //    if(net.getWeight() == 0)
    //        cout<<"Ignoring a net because its weight is zero
    // (DPDEBUG2243)!!!"<<endl;
    //    continue;
    //}

    vector<ClHG_Cluster*>::iterator cl;
    for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
      abkassert(*cl != NULL, "null in net iterators");

      PartitionIds unionPIds = cl0->getAllowableParts();
      unionPIds &= (*cl)->getAllowableParts();
      if ((*cl)->getTag() == 0 && !unionPIds.isEmpty() && (*cl)->isClusterable()
          && (*cl) != cl0) {
        if ((*cl)->getArea() < leastNeighbor)
          leastNeighbor = (*cl)->getArea();
      }
    }
  }

  if (leastNeighbor == std::numeric_limits<double>::max())
    return -1.0;
  else
    return leastNeighbor;
}

void HEMClusteredHGraph::computeEqualWtSmallNeighbors(void)
{
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
}

bool HEMClusteredHGraph::isLarge(ClHG_Cluster* cl, double maxChildArea)
{
  return (cl->getArea() > maxChildArea);
}

ClHG_Cluster* HEMClusteredHGraph::findBestNeighborExcept(ClHG_Cluster* cl0,
                                                         ClHG_Cluster* except,
                                                         double maxChildArea)
{
  _adjNodes.clear();

  ClHG_CutNetItr n;
  for (n = cl0->cutNetsBegin(); n != cl0->cutNetsEnd(); n++) {
    ClHG_ClNet& net = *(n->net);
    if (net.getNumClusters() <= 1 || net.getNumClusters() > 100
        || net.getWeight() == 0)
      continue;

    for (vector<ClHG_Cluster*>::iterator cl = net.clustersBegin();
         cl != net.clustersEnd();
         cl++) {
      abkassert(*cl != NULL, "null in net iterators");
      if (!isUnclusterable((*cl), maxChildArea) && (*cl) != except
          && !differentPartitions(*cl, cl0)) {
        double newWt = getConnectionWeight(net, cl0, (*cl));

        _edgeWeights[(*cl)->getIndex()] += newWt;
        _adjNodes.push_back(*cl);
      }
    }
  }

  computeEqualWtSmallNeighbors();

  if (_equalWtSmallNeighbors.size() == 0)
    return 0;  // no matches
  else if (_equalWtSmallNeighbors.size() == 1)
    return _equalWtSmallNeighbors[0];
  else {
    unsigned chosenNeighbor = _rng % _equalWtSmallNeighbors.size();
    return _equalWtSmallNeighbors[chosenNeighbor];
  }
}

bool HEMClusteredHGraph::differentPartitions(ClHG_Cluster* a, ClHG_Cluster* b)
{
  PartitionIds unionPIds = a->getAllowableParts();
  unionPIds &= b->getAllowableParts();
  return unionPIds.isEmpty();
}
