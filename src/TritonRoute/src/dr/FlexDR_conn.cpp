/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <omp.h>

#include "dr/FlexDR.h"
#include "frProfileTask.h"
#include "io/io.h"

using namespace std;
using namespace fr;

string netToDebug = "";
// copied from FlexDRWorker::initNets_searchRepair_pin2epMap_helper
void FlexDR::checkConnectivity_pin2epMap_helper(
    const frNet* net,
    const frPoint& bp,
    frLayerNum lNum,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap,
    bool pathSeg)
{
  bool enableOutput = false;
  // bool enableOutput = true;
  auto regionQuery = getRegionQuery();
  frRegionQuery::Objects<frBlockObject> result;
  // result.clear();
  //  In PA we may have used NearbyGrid which puts a via outside the pin
  //  but still overlapping.  So we expand bp to a min-width square when
  //  searching for the pin shape.
  auto half_min_width = getTech()->getLayer(lNum)->getMinWidth() / 2;
  frBox query_box(bp.x() - half_min_width,
                  bp.y() - half_min_width,
                  bp.x() + half_min_width,
                  bp.y() + half_min_width);
  regionQuery->query(query_box, lNum, result);
  for (auto& [bx, rqObj] : result) {
    if (pathSeg && !bx.contains(bp))
      continue;
    if (rqObj->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(rqObj);
      if (instTerm->getNet() == net) {
          if (!pathSeg && !bx.contains(bp) && lNum != 2)
              continue;
        if (enableOutput) {
          cout << "    found " << instTerm->getName() << "\n";
        }
        pin2epMap[rqObj].insert(make_pair(bp, lNum));
      }
    } else if (rqObj->typeId() == frcTerm) {
      auto term = static_cast<frTerm*>(rqObj);
      if (term->getNet() == net) {
          if (!pathSeg && !bx.contains(bp) && lNum != 2)
              continue;
        if (enableOutput) {
          cout << "    found PIN/" << term->getName() << endl;
        }
        pin2epMap[rqObj].insert(make_pair(bp, lNum));
      }
    }
  }
}

void FlexDR::checkConnectivity_pin2epMap(
    const frNet* net,
    const vector<frConnFig*>& netDRObjs,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap)
{
  bool enableOutput = net->getName() == netToDebug;
  // bool enableOutput = true;
  if (enableOutput)
    cout << "pin2epMap\n\n";
  frPoint bp, ep;
  set<pair<frPoint, frLayerNum>>
      extEndPoints;  // to avoid delooping fake planar ep in pin
  for (auto& connFig : netDRObjs) {
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      frSegStyle style;
      obj->getStyle(style);
      auto lNum = obj->getLayerNum();
      if (enableOutput) {
        cout << *obj << " layer " << getTech()->getLayer(lNum)->getName()
             << endl;
        cout << "  query bp" << endl;
      }
        checkConnectivity_pin2epMap_helper(net, bp, lNum, pin2epMap, true);
      if (enableOutput) {
        cout << "  query ep" << endl;
      }
        checkConnectivity_pin2epMap_helper(net, ep, lNum, pin2epMap, true);
    }
  }
  for (auto& connFig : netDRObjs) {
    if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig);
      obj->getOrigin(bp);
      auto l1Num = obj->getViaDef()->getLayer1Num();
      auto l2Num = obj->getViaDef()->getLayer2Num();
      if (enableOutput) {
        cout << *obj;
      }
      if (enableOutput) {
        cout << "  query pt l1" << endl;
      }
        checkConnectivity_pin2epMap_helper(net, bp, l1Num, pin2epMap, false);
      if (enableOutput) {
        cout << "  query pt l2" << endl;
      }
        checkConnectivity_pin2epMap_helper(net, bp, l2Num, pin2epMap, false);
      //} else if (connFig->typeId() == frcPatchWire) {
      //  ;
    }
  }
}

void FlexDR::checkConnectivity_initDRObjs(const frNet* net,
                                          vector<frConnFig*>& netDRObjs)
{
  bool enableOutput = false;
  // bool enableOutput = true;
  if (enableOutput)
    cout << "initDRObjs\n\n";
  for (auto& uPtr : net->getShapes()) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == frcPathSeg) {
      netDRObjs.push_back(connFig);
      if (enableOutput) {
        auto obj = static_cast<frPathSeg*>(connFig);
        frPoint bp, ep;
        auto lNum = obj->getLayerNum();
        obj->getPoints(bp, ep);
        cout << *obj << " layer " << getTech()->getLayer(lNum)->getName()
             << endl;
      }
    } else {
      cout << "Error: checkConnectivity_initDRObjs unsupported type" << endl;
    }
  }
  for (auto& uPtr : net->getVias()) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == frcVia) {
      netDRObjs.push_back(connFig);
      if (enableOutput) {
        auto obj = static_cast<frVia*>(connFig);
        frPoint bp;
        obj->getOrigin(bp);
        cout << *obj << " layer " << obj->getViaDef()->getName() << endl;
      }
    } else {
      cout << "Error: checkConnectivity_initDRObjs unsupported type" << endl;
    }
  }
}

void FlexDR::checkConnectivity_nodeMap_routeObjEnd(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  bool enableOutput = false;
  frPoint bp, ep;
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto& connFig = netRouteObjs[i];
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      nodeMap[make_pair(bp, lNum)].insert(i);
      nodeMap[make_pair(ep, lNum)].insert(i);
      if (enableOutput) {
        cout << "node idx = " << i << ", (" << bp.x() / 2000.0 << ", "
             << bp.y() / 2000.0 << ") (" << ep.x() / 2000.0 << ", "
             << ep.y() / 2000.0 << ") " << getTech()->getLayer(lNum)->getName()
             << endl;
      }
    } else if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig);
      obj->getOrigin(bp);
      auto l1Num = obj->getViaDef()->getLayer1Num();
      auto l2Num = obj->getViaDef()->getLayer2Num();
      nodeMap[make_pair(bp, l1Num)].insert(i);
      nodeMap[make_pair(bp, l2Num)].insert(i);
      if (enableOutput) {
        cout << "node idx = " << i << ", (" << bp.x() / 2000.0 << ", "
             << bp.y() / 2000.0 << ") " << getTech()->getLayer(l1Num)->getName()
             << " --> " << getTech()->getLayer(l2Num)->getName() << endl;
      }
    } else {
      cout << "Error: checkConnectivity_nodeMap_routeObjEnd unsupported type"
           << endl;
    }
  }
}

void FlexDR::checkConnectivity_nodeMap_routeObjSplit_helper(
    const frPoint& crossPt,
    frCoord trackCoord,
    frCoord splitCoord,
    frLayerNum lNum,
    const vector<map<frCoord, map<frCoord, pair<frCoord, int>>>>& mergeHelper,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  auto it1 = mergeHelper[lNum].find(trackCoord);
  if (it1 != mergeHelper[lNum].end()) {
    auto& mp = it1->second;  // map<ep, pair<bp, objIdx>>
    auto it2 = mp.lower_bound(splitCoord);
    if (it2 != mp.end()) {
      auto& endP = it2->first;
      auto& [beginP, objIdx] = it2->second;
      if (endP > splitCoord && beginP < splitCoord) {
        nodeMap[make_pair(crossPt, lNum)].insert(objIdx);
      }
    }
  }
}

void FlexDR::checkConnectivity_nodeMap_routeObjSplit(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  frPoint bp, ep;
  // vector<map<track, map<ep, pair<bp, objIdx>>>> interval_map
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> horzMergeHelper(
      getTech()->getLayers().size());
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> vertMergeHelper(
      getTech()->getLayers().size());
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto& connFig = netRouteObjs[i];
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      // vert seg
      if (bp.x() == ep.x()) {
        vertMergeHelper[lNum][bp.x()][ep.y()] = make_pair(bp.y(), i);
        // horz seg
      } else {
        horzMergeHelper[lNum][bp.y()][ep.x()] = make_pair(bp.x(), i);
      }
    }
  }
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto& connFig = netRouteObjs[i];
    // ep on pathseg
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      // vert seg, find horz crossing seg
      if (bp.x() == ep.x()) {
        // find whether there is horz track at bp
        auto crossPt = bp;
        auto trackCoord = bp.y();
        auto splitCoord = bp.x();
        checkConnectivity_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
        // find whether there is horz track at ep
        crossPt = ep;
        trackCoord = ep.y();
        splitCoord = ep.x();
        checkConnectivity_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
        // horz seg
      } else {
        // find whether there is vert track at bp
        auto crossPt = bp;
        auto trackCoord = bp.x();
        auto splitCoord = bp.y();
        checkConnectivity_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
        // find whether there is vert track at ep
        crossPt = ep;
        trackCoord = ep.x();
        splitCoord = ep.y();
        checkConnectivity_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
      }
    } else if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig);
      obj->getOrigin(bp);
      auto lNum = obj->getViaDef()->getLayer1Num();
      // find whether there is horz track at bp on layer1
      auto crossPt = bp;
      auto trackCoord = bp.y();
      auto splitCoord = bp.x();
      checkConnectivity_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer1
      crossPt = bp;
      trackCoord = bp.x();
      splitCoord = bp.y();
      checkConnectivity_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);

      lNum = obj->getViaDef()->getLayer2Num();
      // find whether there is horz track at bp on layer2
      crossPt = bp;
      trackCoord = bp.y();
      splitCoord = bp.x();
      checkConnectivity_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer2
      crossPt = bp;
      trackCoord = bp.x();
      splitCoord = bp.y();
      checkConnectivity_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
    }
  }
}

void FlexDR::checkConnectivity_nodeMap_pin(
    const vector<frConnFig*>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    const map<frBlockObject*,
              set<pair<frPoint, frLayerNum>>,
              frBlockObjectComp>& pin2epMap,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  bool enableOutput = false;
  int currCnt = (int) netRouteObjs.size();
  for (auto& [obj, locS] : pin2epMap) {
    netPins.push_back(obj);
    for (auto& pr : locS) {
      nodeMap[pr].insert(currCnt);
      if (enableOutput) {
        cout << "pin idx = " << currCnt << ", (" << pr.first.x() << ", "
             << pr.first.y() << ") "
             << getTech()->getLayer(pr.second)->getName() << endl;
      }
    }
    ++currCnt;
  }
}

void FlexDR::checkConnectivity_nodeMap(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    const map<frBlockObject*,
              set<pair<frPoint, frLayerNum>>,
              frBlockObjectComp>& pin2epMap,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  bool enableOutput = false;
  // bool enableOutput = true;
  checkConnectivity_nodeMap_routeObjEnd(net, netRouteObjs, nodeMap);
  checkConnectivity_nodeMap_routeObjSplit(net, netRouteObjs, nodeMap);
  checkConnectivity_nodeMap_pin(netRouteObjs, netPins, pin2epMap, nodeMap);
  if (enableOutput) {
    int idx = 0;
    for (auto connFig : netRouteObjs) {
      if (connFig->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(connFig);
        frPoint bp, ep;
        auto lNum = obj->getLayerNum();
        obj->getPoints(bp, ep);
        cout << "#" << idx << *obj << getTech()->getLayer(lNum)->getName()
             << endl;
      } else if (connFig->typeId() == frcVia) {
        auto obj = static_cast<frVia*>(connFig);
        frPoint bp;
        obj->getOrigin(bp);
        cout << "#" << idx << *obj << obj->getViaDef()->getName() << endl;
      } else {
        cout << "Error: checkConnectivity_nodeMap unsupported type" << endl;
      }
      idx++;
    }
    for (auto obj : netPins) {
      if (obj->typeId() == frcInstTerm) {
        auto instTerm = static_cast<frInstTerm*>(obj);
        cout << "#" << idx << " " << instTerm->getName() << "\n";
      } else if (obj->typeId() == frcTerm) {
        auto term = static_cast<frTerm*>(obj);
        cout << "#" << idx << " PIN/" << term->getName() << endl;
      }
      idx++;
    }
  }
}

bool FlexDR::checkConnectivity_astar(
    frNet* net,
    vector<bool>& adjVisited,
    vector<int>& adjPrevIdx,
    const map<pair<frPoint, frLayerNum>, set<int>>& nodeMap,
        const vector<frConnFig*>& netDRObjs,
    const int& nNetRouteObjs,
    const int& nNetObjs)
{
  // bool enableOutput = true;
  bool enableOutput = net->getName() == netToDebug;
  // a star search
  if (enableOutput)
    cout << "checkConnectivity_astar\n\n";
  // node index, node visited
  vector<vector<int>> adjVec(nNetObjs, vector<int>());
  vector<bool> onPathIdx(nNetObjs, false);
  adjVisited.clear();
  adjPrevIdx.clear();
  adjVisited.resize(nNetObjs, false);
  adjPrevIdx.resize(nNetObjs, -1);
  for (auto& [pr, idxS] : nodeMap) {
    // auto &[pt, lNum] = pr;
    for (auto it1 = idxS.begin(); it1 != idxS.end(); it1++) {
      auto it2 = it1;
      it2++;
      auto idx1 = *it1;
      for (; it2 != idxS.end(); it2++) {
        auto idx2 = *it2;
        adjVec[idx1].push_back(idx2);
        adjVec[idx2].push_back(idx1);
        if (enableOutput)
          cout << "add edge #" << idx1 << " -- #" << idx2 << endl;
        //  one pin, one gcell
      }
    }
  }

  struct wf
  {
    int nodeIdx;
    int prevIdx;
    int cost;
    bool operator<(const wf& b) const
    {
      if (cost == b.cost) {
        return nodeIdx > b.nodeIdx;
      } else {
        return cost > b.cost;
      }
    }
  };
  for (int findNode = nNetRouteObjs; findNode < nNetObjs - 1; findNode++) {
    // adjVisited = onPathIdx;
    // cout <<"finished " <<findNode <<" nodes" <<endl;
    priority_queue<wf> pq;
    if (enableOutput) {
      // cout <<"visit";
    }
    if (findNode == nNetRouteObjs) {
      // push only first pin into pq
      pq.push({nNetRouteObjs, -1, 0});
    } else {
      // push every visited node into pq
      for (int i = 0; i < nNetObjs; i++) {
        // if (adjVisited[i]) {
        if (onPathIdx[i]) {
          // penalize feedthrough in normal mode
          if (i >= nNetRouteObjs) {
            pq.push({i, adjPrevIdx[i], 5});
          } else {
            pq.push({i, adjPrevIdx[i], 0});
          }
        }
      }
    }
    int lastNodeIdx = -1;
    while (!pq.empty()) {
      auto wfront = pq.top();
      pq.pop();
      if (!onPathIdx[wfront.nodeIdx] && adjVisited[wfront.nodeIdx]) {
        continue;
      }
      if (wfront.nodeIdx > nNetRouteObjs && wfront.nodeIdx < nNetObjs
          && adjVisited[wfront.nodeIdx] == false) {
        adjVisited[wfront.nodeIdx] = true;
        adjPrevIdx[wfront.nodeIdx] = wfront.prevIdx;
        if (enableOutput) {
          cout << "visit pin " << wfront.nodeIdx << " (" << wfront.cost << ","
               << wfront.prevIdx << ")"
               << " exit" << endl;
        }
        lastNodeIdx = wfront.nodeIdx;
        break;
      }
      adjVisited[wfront.nodeIdx] = true;
      adjPrevIdx[wfront.nodeIdx] = wfront.prevIdx;
      if (enableOutput) {
          cout << "visit ";
          if (wfront.nodeIdx < (int)netDRObjs.size())
             cout << *netDRObjs[wfront.nodeIdx];
          cout << "idx " << wfront.nodeIdx << " (" << wfront.cost << ","
             << wfront.prevIdx << ")" << endl;
      }
      // visit other nodes
      for (auto nbrIdx : adjVec[wfront.nodeIdx]) {
        if (!adjVisited[nbrIdx]) {
          pq.push({nbrIdx, wfront.nodeIdx, wfront.cost + 1});
          if (enableOutput) {
            cout << "push " << nbrIdx << endl;
          }
        }
      }
    }
    // trace back path
    if (enableOutput) {
      cout << "trace back id";
    }
    while ((lastNodeIdx != -1) && (!onPathIdx[lastNodeIdx])) {
      onPathIdx[lastNodeIdx] = true;
      if (enableOutput) {
        cout << " " << lastNodeIdx << " (" << adjPrevIdx[lastNodeIdx] << ")";
      }
      lastNodeIdx = adjPrevIdx[lastNodeIdx];
    }
    if (enableOutput) {
      cout << endl;
    }
    adjVisited = onPathIdx;
  }
  if (enableOutput) {
    cout << "stat: " << net->getName()
         << " #guide/#pin/#unused = " << nNetRouteObjs << "/"
         << nNetObjs - nNetRouteObjs << "/"
         << nNetObjs - count(adjVisited.begin(), adjVisited.end(), true)
         << endl;
  }
  int pinVisited
      = count(adjVisited.begin() + nNetRouteObjs, adjVisited.end(), true);
  // true error when allowing feedthrough
  if (pinVisited != nNetObjs - nNetRouteObjs) {
    cout << "Error: " << net->getName() << " "
         << nNetObjs - nNetRouteObjs - pinVisited
         << " pin not visited #guides = " << nNetRouteObjs << endl;
    if (enableOutput) {
      for (int i = nNetRouteObjs; i < nNetObjs; i++) {
        if (!adjVisited[i]) {
          cout << "  pin id = " << i << endl;
        }
      }
    }
  }
  if (pinVisited == nNetObjs - nNetRouteObjs) {
    return true;
  } else {
    return false;
  }
}

void FlexDR::checkConnectivity_final(
    frNet* net,
    vector<frConnFig*>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    const vector<bool>& adjVisited,
    int gCnt,
    int nCnt,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  // bool enableOutput = true;
  bool enableOutput = net->getName() == netToDebug;

  auto regionQuery = getRegionQuery();

  // from obj to pt
  map<int, set<pair<frPoint, frLayerNum>>> reverseNodeMap;
  for (auto& [pr, idxS] : nodeMap) {
    for (auto& idx : idxS) {
      reverseNodeMap[idx].insert(pr);
    }
  }

  // nodeMap delete redundant objs
  for (int i = 0; i < (int) adjVisited.size(); i++) {
    if (adjVisited[i]) {
      continue;
    }
    for (auto& pr : reverseNodeMap[i]) {
      nodeMap[pr].erase(i);
    }
    if (i < gCnt) {
      if (netRouteObjs[i]->typeId() == frcPathSeg) {
        auto victimPathSeg = static_cast<frPathSeg*>(netRouteObjs[i]);
        // negative rule
        frBox bbox;
        victimPathSeg->getBBox(bbox);
        checkConnectivity_addMarker(net, victimPathSeg->getLayerNum(), bbox);

        if (enableOutput) {
          cout << "net " << net->getName() << " deleting pathseg " << 
                  *static_cast<frPathSeg*>(netRouteObjs[i]) << endl;
        }
        regionQuery->removeDRObj(static_cast<frShape*>(netRouteObjs[i]));
        net->removeShape(static_cast<frShape*>(netRouteObjs[i]));
      } else if (netRouteObjs[i]->typeId() == frcVia) {
        auto victimVia = static_cast<frVia*>(netRouteObjs[i]);
        // negative rule
        frBox bbox;
        victimVia->getLayer1BBox(bbox);
        checkConnectivity_addMarker(
            net, victimVia->getViaDef()->getLayer1Num(), bbox);
        
        frVia* via = static_cast<frVia*>(netRouteObjs[i]);
        if (enableOutput) {
          cout << "net " << net->getName() << " deleting via " << *via << endl;
        }
        regionQuery->removeDRObj(via);
        net->removeVia(via);
        //} else if (netRouteObjs[i]->typeId() == frcPatchWire) {
        //  regionQuery->removeDRObj(static_cast<frPatchWire*>(netRouteObjs[i]));
        //  net->removePatchWire(static_cast<frPatchWire*>(netRouteObjs[i]));
        //  if (enableOutput) {
        //    cout <<"net " <<net->getName() <<" deleting pwire" <<endl;
        //  }
      } else {
        cout << "Error: checkConnectivity_final unsupporterd type" << endl;
        exit(1);
      }
      netRouteObjs[i] = nullptr;
    } else {
      cout << "Error: checkConnectivity_final i >= gCnt" << endl;
      exit(1);
    }
  }

  // rebuild reverseNodeMap
  reverseNodeMap.clear();
  for (auto& [pr, idxS] : nodeMap) {
    if (idxS.size() == 1) {
      continue;
    }
    for (auto& idx : idxS) {
      reverseNodeMap[idx].insert(pr);
    }
  }

  // must use map because split has to start from right
  map<pair<frPoint, frLayerNum>, int> psSplits;
  for (auto& [pr, idxS] : nodeMap) {
    bool hasPin = false;
    for (auto idx : idxS) {
      // skip for non-pin idx
      if (idx >= gCnt) {
        hasPin = true;
        break;
      }
    }

    if (!hasPin) {
      continue;
    }

    bool hasPinEP = false;
    int psIdx = -1;
    for (auto idx : idxS) {
      if (idx >= gCnt) {
        continue;
      }
      if (netRouteObjs[idx]->typeId() == frcPathSeg) {
        auto ps = static_cast<frPathSeg*>(netRouteObjs[idx]);
        frPoint bp, ep;
        ps->getPoints(bp, ep);
        if (bp == pr.first || ep == pr.first) {
          hasPinEP = true;
          break;
        } else {
          psIdx = idx;
        }
      } else if (netRouteObjs[idx]->typeId() == frcVia) {
        hasPinEP = true;
        break;
      }
    }

    // need to split for pin feedthrough for connectivity
    // psIdx might be -1 because empty nodeMap
    //   1. two segments of a corner deleted, corner does not exist anymore
    //   2. to a feedthrough pin, if the feedthrough is in a loop, one end does
    //   not exist anymore
    if (!hasPinEP && psIdx != -1) {
      psSplits[pr] = psIdx;
    }
  }

  vector<frPathSeg*> addedPS;
  for (auto it = psSplits.rbegin(); it != psSplits.rend(); it++) {
    auto& [pr, idx1] = *it;
    int idx2 = nCnt + addedPS.size();
    auto& [splitPt, lNum] = pr;
    frPoint bp1, ep1;
    auto ps1 = static_cast<frPathSeg*>(netRouteObjs[idx1]);
    ps1->getPoints(bp1, ep1);
    bool isHorz = (bp1.y() == ep1.y());
    set<pair<frPoint, frLayerNum>> newPr1;
    set<pair<frPoint, frLayerNum>> newPr2;
    for (auto& [prPt, prLNum] : reverseNodeMap[idx1]) {
      if (isHorz) {
        if (prPt.x() <= splitPt.x()) {
          newPr1.insert(make_pair(prPt, prLNum));
        } else {
          nodeMap[make_pair(prPt, prLNum)].erase(idx1);
        }
        if (prPt.x() >= splitPt.x()) {
          newPr2.insert(make_pair(prPt, prLNum));
          nodeMap[make_pair(prPt, prLNum)].insert(idx2);
        }
      } else {
        if (prPt.y() <= splitPt.y()) {
          newPr1.insert(make_pair(prPt, prLNum));
        } else {
          nodeMap[make_pair(prPt, prLNum)].erase(idx1);
        }
        if (prPt.y() >= splitPt.y()) {
          newPr2.insert(make_pair(prPt, prLNum));
          nodeMap[make_pair(prPt, prLNum)].insert(idx2);
        }
      }
    }

    reverseNodeMap[idx1].clear();
    reverseNodeMap[idx1] = newPr1;
    reverseNodeMap[idx2] = newPr2;

    auto uPs2 = make_unique<frPathSeg>(*ps1);
    auto ps2 = uPs2.get();
    addedPS.push_back(ps2);
    unique_ptr<frShape> uShape(std::move(uPs2));
    net->addShape(std::move(uShape));
    // manipulate ps1
    regionQuery->removeDRObj(ps1);
    frSegStyle ps1Style;
    ps1->getStyle(ps1Style);
    ps1Style.setEndStyle(frEndStyle(frcTruncateEndStyle), 0);
    ps1->setStyle(ps1Style);
    ps1->setPoints(bp1, splitPt);
    regionQuery->addDRObj(ps1);

    // manipulate ps2
    frSegStyle ps2Style;
    ps2->getStyle(ps2Style);
    ps2Style.setBeginStyle(frEndStyle(frcTruncateEndStyle), 0);
    ps2->setStyle(ps2Style);
    ps2->setPoints(splitPt, ep1);
    regionQuery->addDRObj(ps2);
  }

  for (auto& [idx, ptS] : reverseNodeMap) {
    frPathSeg* ps = nullptr;
    if (idx < gCnt) {
      if (netRouteObjs[idx]->typeId() == frcPathSeg) {
        ps = static_cast<frPathSeg*>(netRouteObjs[idx]);
      }
    } else if (idx >= nCnt) {
      ps = addedPS[idx - nCnt];
    }
    if (!ps) {
      continue;
    }

    frPoint bp, ep;
    ps->getPoints(bp, ep);
    auto [minPr, maxPr] = minmax_element(ptS.begin(), ptS.end());
    auto& minPt = minPr->first;
    auto& maxPt = maxPr->first;
    // shrink segment
    if (bp < minPt || maxPt < ep) {
      // negative rule
      frBox bbox;
      ps->getBBox(bbox);
      checkConnectivity_addMarker(net, ps->getLayerNum(), bbox);

      regionQuery->removeDRObj(ps);
      ps->setPoints(minPt, maxPt);
      regionQuery->addDRObj(ps);
      if (enableOutput) {
        cout << "net " << net->getName() << " shrinking pathseg" << endl;
      }
    }
  }

  // delete redundant pwires
  set<pair<frPoint, frLayerNum>> validPoints;
  frPoint bp, ep;
  frLayerNum lNum;
  for (auto& connFig : net->getShapes()) {
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig.get());
      obj->getPoints(bp, ep);
      lNum = obj->getLayerNum();
      validPoints.insert(make_pair(bp, lNum));
      validPoints.insert(make_pair(ep, lNum));
    } else {
      cout << "Error: checkConnectivity_final unsupporterd type" << endl;
      exit(1);
    }
  }
  for (auto& connFig : net->getVias()) {
    if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig.get());
      obj->getOrigin(bp);
      lNum = obj->getViaDef()->getLayer1Num();
      validPoints.insert(make_pair(bp, lNum));
      lNum = obj->getViaDef()->getLayer2Num();
      validPoints.insert(make_pair(bp, lNum));
    } else {
      cout << "Error: checkConnectivity_final unsupporterd type" << endl;
      exit(1);
    }
  }
  for (auto it = net->getPatchWires().begin();
       it != net->getPatchWires().end();) {
    auto obj = static_cast<frPatchWire*>(it->get());
    it++;
    obj->getOrigin(bp);
    lNum = obj->getLayerNum();
    if (validPoints.find(make_pair(bp, lNum)) == validPoints.end()) {
      // negative rule
      frBox bbox;
      obj->getBBox(bbox);
      checkConnectivity_addMarker(net, obj->getLayerNum(), bbox);

      regionQuery->removeDRObj(obj);
      net->removePatchWire(obj);
      if (enableOutput) {
        cout << "net " << net->getName() << " deleting pwire" << endl;
      }
    }
  }
}

void FlexDR::checkConnectivity_merge1(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    vector<map<frCoord, vector<int>>>& horzPathSegs,
    vector<map<frCoord, vector<int>>>& vertPathSegs)
{
  // bool enableOutput = false;
  frPoint bp, ep;
  if (netRouteObjs.empty()) {
    return;
  }

  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto& connFig = netRouteObjs[i];
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      // vert
      if (bp.x() == ep.x()) {
        vertPathSegs[lNum][bp.x()].push_back(i);
      } else if (bp.y() == ep.y()) {
        horzPathSegs[lNum][bp.y()].push_back(i);
      } else {
        cout << "Error: non-orthogonal wires in checkConnectivity_merge\n";
      }
      // vert
    } else if (connFig->typeId() == frcVia) {
      ;
    } else if (connFig->typeId() == frcPatchWire) {
      ;
    } else {
      cout << "Warning: unsupporterd obj type in checkConnectivity_merge"
           << endl;
    }
  }
}

void FlexDR::checkConnectivity_merge2(
    frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    const vector<map<frCoord, vector<int>>>& horzPathSegs,
    const vector<map<frCoord, vector<int>>>& vertPathSegs,
    vector<vector<vector<int>>>& horzVictims,
    vector<vector<vector<int>>>& vertVictims,
    vector<vector<vector<pair<frCoord, frCoord>>>>& horzNewSegSpans,
    vector<vector<vector<pair<frCoord, frCoord>>>>& vertNewSegSpans)
{
  for (auto lNum = 0; lNum < (int) horzPathSegs.size(); lNum++) {
    auto& track2Segs = horzPathSegs[lNum];
    horzVictims[lNum].resize(track2Segs.size());
    horzNewSegSpans[lNum].resize(track2Segs.size());
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = horzVictims[lNum][i];
      auto& newSegSpans = horzNewSegSpans[lNum][i];
      checkConnectivity_merge_perform(
          netRouteObjs, indices, victims, newSegSpans, true /*isHorz*/);
      i++;
    }
  }

  for (auto lNum = 0; lNum < (int) vertPathSegs.size(); lNum++) {
    auto& track2Segs = vertPathSegs[lNum];
    vertVictims[lNum].resize(track2Segs.size());
    vertNewSegSpans[lNum].resize(track2Segs.size());
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = vertVictims[lNum][i];
      auto& newSegSpans = vertNewSegSpans[lNum][i];
      checkConnectivity_merge_perform(
          netRouteObjs, indices, victims, newSegSpans, false /*isHorz*/);
      i++;
    }
  }
}

void FlexDR::checkConnectivity_merge3(
    frNet* net,
    vector<frConnFig*>& netRouteObjs,
    const vector<map<frCoord, vector<int>>>& horzPathSegs,
    const vector<map<frCoord, vector<int>>>& vertPathSegs,
    const vector<vector<vector<int>>>& horzVictims,
    const vector<vector<vector<int>>>& vertVictims,
    const vector<vector<vector<pair<frCoord, frCoord>>>>& horzNewSegSpans,
    const vector<vector<vector<pair<frCoord, frCoord>>>>& vertNewSegSpans)
{
  for (auto lNum = 0; lNum < (int) horzPathSegs.size(); lNum++) {
    auto& track2Segs = horzPathSegs[lNum];
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = horzVictims[lNum][i];
      auto& newSegSpans = horzNewSegSpans[lNum][i];
      checkConnectivity_merge_commit(net,
                                     netRouteObjs,
                                     victims,
                                     lNum,
                                     trackCoord,
                                     newSegSpans,
                                     true /*isHorz*/);
      i++;
    }
  }

  for (auto lNum = 0; lNum < (int) vertPathSegs.size(); lNum++) {
    auto& track2Segs = vertPathSegs[lNum];
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = vertVictims[lNum][i];
      auto& newSegSpans = vertNewSegSpans[lNum][i];
      checkConnectivity_merge_commit(net,
                                     netRouteObjs,
                                     victims,
                                     lNum,
                                     trackCoord,
                                     newSegSpans,
                                     false /*isHorz*/);
      i++;
    }
  }
}

void FlexDR::checkConnectivity_merge_perform(
    const vector<frConnFig*>& netRouteObjs,
    const vector<int>& indices,
    vector<int>& victims,
    vector<pair<frCoord, frCoord>>& newSegSpans,
    bool isHorz)
{
  vector<pair<pair<frCoord, frCoord>, int>> segSpans;
  frPoint bp, ep;
  for (auto& idx : indices) {
    auto obj = static_cast<frPathSeg*>(netRouteObjs[idx]);
    obj->getPoints(bp, ep);
    if (isHorz) {
      segSpans.push_back(make_pair(make_pair(bp.x(), ep.x()), idx));
      if (bp.x() >= ep.x()) {
        cout << "Error: bp >= ep\n";
      }
    } else {
      segSpans.push_back(make_pair(make_pair(bp.y(), ep.y()), idx));
      if (bp.y() >= ep.y()) {
        cout << "Error: bp >= ep\n";
      }
    }
  }
  sort(segSpans.begin(), segSpans.end());

  // get victim segments and merged segments
  checkConnectivity_merge_perform_helper(segSpans, victims, newSegSpans);
}

void FlexDR::checkConnectivity_merge_perform_helper(
    const vector<pair<pair<frCoord, frCoord>, int>>& segSpans,
    vector<int>& victims,
    vector<pair<frCoord, frCoord>>& newSegSpans)
{
  // bool enableOutput = true;
  map<int, int> victimIdx2SegSpanIdx;
  int ovlpCnt = 0;
  frCoord currStart = INT_MAX, currEnd = INT_MIN;
  vector<int> localVictims;
  for (auto& segSpan : segSpans) {
    if (segSpan.first.first >= currEnd) {
      ovlpCnt++;
      if (ovlpCnt >= 2) {
        // commit prev merged segs
        newSegSpans.push_back(make_pair(currStart, currEnd));
        // commit victims in merged segs
        victims.insert(victims.end(), localVictims.begin(), localVictims.end());
        // if (enableOutput) {
        //   cout << "intervals ";
        //   for (auto &victimIdx: localVictims) {
        //     auto &intv = segSpans[victimIdx2SegSpanIdx[victimIdx]].first;
        //     cout << "[" << intv.first << "," << intv.second << "] ";
        //   }
        //   cout << "merged to [" << currStart << "," << currEnd << "]\n";
        // }
      }
      // cleanup
      localVictims.clear();
      ovlpCnt = 0;
      // update local variables
      currStart = segSpan.first.first;
      currEnd = segSpan.first.second;
      localVictims.push_back(segSpan.second);
    } else {
      ovlpCnt++;
      // update local variables
      currEnd = max(currEnd, segSpan.first.second);
      localVictims.push_back(segSpan.second);
    }
  }
  if (ovlpCnt >= 1) {
    newSegSpans.push_back(make_pair(currStart, currEnd));
    victims.insert(victims.end(), localVictims.begin(), localVictims.end());
  }
}

void FlexDR::checkConnectivity_merge_commit(
    frNet* net,
    vector<frConnFig*>& netRouteObjs,
    const vector<int>& victims,
    frLayerNum lNum,
    frCoord trackCoord,
    const vector<pair<frCoord, frCoord>>& newSegSpans,
    bool isHorz)
{
  if (victims.empty()) {
    return;
  }
  auto regionQuery = getRegionQuery();
  // add segments from overlapped segments
  int cnt = 0;
  frPathSeg *last, *curr;
  for (auto& newSegSpan : newSegSpans) {
    auto victimPathSeg = static_cast<frPathSeg*>(netRouteObjs[victims[cnt]]);
    regionQuery->removeDRObj(static_cast<frShape*>(victimPathSeg));

    frPoint bp, ep;
    if (isHorz) {
      bp.set(newSegSpan.first, trackCoord);
      ep.set(newSegSpan.second, trackCoord);
    } else {
      bp.set(trackCoord, newSegSpan.first);
      ep.set(trackCoord, newSegSpan.second);
    }
    victimPathSeg->setPoints(bp, ep);
    cnt++;
    last = nullptr;
    for (; cnt < (int) victims.size(); cnt++) {
      curr = static_cast<frPathSeg*>(netRouteObjs[victims[cnt]]);
      if (curr->high() <= newSegSpan.second) {
        last = curr;
        regionQuery->removeDRObj(
            static_cast<frShape*>(netRouteObjs[victims[cnt]]));
        net->removeShape(static_cast<frShape*>(netRouteObjs[victims[cnt]]));
        netRouteObjs[victims[cnt]] = nullptr;
      } else
        break;
    }
    if (last) {
      victimPathSeg->setEndStyle(last->getEndStyle(), last->getEndExt());
    }
    regionQuery->addDRObj(victimPathSeg);
  }
}

void FlexDR::checkConnectivity_addMarker(frNet* net,
                                         frLayerNum lNum,
                                         const frBox& bbox)
{
  auto regionQuery = getRegionQuery();
  auto marker = make_unique<frMarker>();
  marker->setBBox(bbox);
  marker->setLayerNum(lNum);
  marker->setConstraint(
      getDesign()->getTech()->getLayer(lNum)->getRecheckConstraint());
  marker->addSrc(net);
  marker->addVictim(net, make_tuple(lNum, bbox, false));
  marker->addAggressor(net, make_tuple(lNum, bbox, false));
  regionQuery->addMarker(marker.get());
  getDesign()->getTopBlock()->addMarker(std::move(marker));
}

// feedthrough and loop check
void FlexDR::checkConnectivity(int iter)
{
  ProfileTask profile("DR:checkConnectivity");
  bool isWrong = false;

  int batchSize = 131072;
  vector<vector<frNet*>> batches(1);
  for (auto& uPtr : getDesign()->getTopBlock()->getNets()) {
    auto net = uPtr.get();
    if (!net->isModified()) {
      continue;
    } else {
      net->setModified(false);
      if ((int) batches.back().size() < batchSize) {
        batches.back().push_back(net);
      } else {
        batches.push_back(vector<frNet*>());
        batches.back().push_back(net);
      }
    }
  }
  if (batches.size() && batches[0].empty()) {
    batches.clear();
  }

  // omp_set_num_threads(MAX_THREADS);
  omp_set_num_threads(1);
  for (auto& batch : batches) {
    // only for init batch vectors, prefix t = temporary
    vector<map<frCoord, vector<int>>> tHorzPathSegs(
        getTech()->getLayers().size());
    vector<map<frCoord, vector<int>>> tVertPathSegs(
        getTech()->getLayers().size());
    vector<vector<vector<int>>> tHorzVictims(getTech()->getLayers().size());
    vector<vector<vector<int>>> tVertVictims(getTech()->getLayers().size());
    vector<vector<vector<pair<frCoord, frCoord>>>> tHorzNewSegSpans(
        getTech()->getLayers().size());
    vector<vector<vector<pair<frCoord, frCoord>>>> tVertNewSegSpans(
        getTech()->getLayers().size());

    // prefix a = all batch
    // net->figs
    vector<vector<frConnFig*>> aNetDRObjs(batchSize);
    // net->layer->track->indices of DRObj
    vector<vector<map<frCoord, vector<int>>>> aHorzPathSegs(batchSize,
                                                            tHorzPathSegs);
    vector<vector<map<frCoord, vector<int>>>> aVertPathSegs(batchSize,
                                                            tVertPathSegs);
    // net->lnum->trackIdx->objIdxs
    vector<vector<vector<vector<int>>>> aHorzVictims(batchSize, tHorzVictims);
    vector<vector<vector<vector<int>>>> aVertVictims(batchSize, tVertVictims);
    // net->lnum->trackIdx->seg_start_end_pairs
    vector<vector<vector<vector<pair<frCoord, frCoord>>>>> aHorzNewSegSpans(
        batchSize, tHorzNewSegSpans);
    vector<vector<vector<vector<pair<frCoord, frCoord>>>>> aVertNewSegSpans(
        batchSize, tVertNewSegSpans);
    // net->term/instTerm->pt_layer
    vector<
        map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>>
        aPin2epMap(batchSize);
    vector<vector<frBlockObject*>> aNetPins(batchSize);
    vector<map<pair<frPoint, frLayerNum>, set<int>>> aNodeMap(batchSize);
    vector<vector<bool>> aAdjVisited(batchSize);
    vector<vector<int>> aAdjPrevIdx(batchSize);
    vector<bool> status(batchSize, false);

// parallel
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int) batch.size(); i++) {
      auto& net = batch[i];
      auto& initNetDRObjs = aNetDRObjs[i];
      auto& horzPathSegs = aHorzPathSegs[i];
      auto& vertPathSegs = aVertPathSegs[i];
      auto& horzVictims = aHorzVictims[i];
      auto& vertVictims = aVertVictims[i];
      auto& horzNewSegSpans = aHorzNewSegSpans[i];
      auto& vertNewSegSpans = aVertNewSegSpans[i];

      checkConnectivity_initDRObjs(net, initNetDRObjs);
      checkConnectivity_merge1(net, initNetDRObjs, horzPathSegs, vertPathSegs);
      checkConnectivity_merge2(net,
                               initNetDRObjs,
                               horzPathSegs,
                               vertPathSegs,
                               horzVictims,
                               vertVictims,
                               horzNewSegSpans,
                               vertNewSegSpans);
    }

    // sequential
    for (int i = 0; i < (int) batch.size(); i++) {
      auto& net = batch[i];
      auto& initNetDRObjs = aNetDRObjs[i];
      auto& horzPathSegs = aHorzPathSegs[i];
      auto& vertPathSegs = aVertPathSegs[i];
      auto& horzVictims = aHorzVictims[i];
      auto& vertVictims = aVertVictims[i];
      auto& horzNewSegSpans = aHorzNewSegSpans[i];
      auto& vertNewSegSpans = aVertNewSegSpans[i];
      checkConnectivity_merge3(net,
                               initNetDRObjs,
                               horzPathSegs,
                               vertPathSegs,
                               horzVictims,
                               vertVictims,
                               horzNewSegSpans,
                               vertNewSegSpans);
    }

// parallel
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int) batch.size(); i++) {
      auto& net = batch[i];
      auto& netDRObjs = aNetDRObjs[i];
      auto& pin2epMap = aPin2epMap[i];
      auto& netPins = aNetPins[i];
      auto& nodeMap = aNodeMap[i];
      auto& adjVisited = aAdjVisited[i];
      auto& adjPrevIdx = aAdjPrevIdx[i];
      netDRObjs.clear();
      checkConnectivity_initDRObjs(net, netDRObjs);
      checkConnectivity_pin2epMap(net, netDRObjs, pin2epMap);
      checkConnectivity_nodeMap(net, netDRObjs, netPins, pin2epMap, nodeMap);

      int nNetRouteObjs = (int) netDRObjs.size();
      int nNetObjs = (int) netDRObjs.size() + (int) netPins.size();
      status[i] = checkConnectivity_astar(
          net, adjVisited, adjPrevIdx, nodeMap, netDRObjs, nNetRouteObjs, nNetObjs);
    }

    // sequential
    for (int i = 0; i < (int) batch.size(); i++) {
      auto& net = batch[i];
      auto& netDRObjs = aNetDRObjs[i];
      auto& netPins = aNetPins[i];
      auto& nodeMap = aNodeMap[i];
      auto& adjVisited = aAdjVisited[i];

      int gCnt = (int) netDRObjs.size();
      int nCnt = (int) netDRObjs.size() + (int) netPins.size();

      if (!status[i]) {
        cout << "Error: checkConnectivity break, net " << net->getName() << endl
             << "Objs not visited:\n";
        for (int idx = 0; idx < (int)adjVisited.size(); idx++) {
          if (!adjVisited[idx]) {
            if (idx < (int)netDRObjs.size())
              cout << *(netDRObjs[idx]) << "\n";
            else
              cout << *(netPins[idx - netDRObjs.size()]) << "\n";
          }
        }
        isWrong = true;
      } else {
        // get lock
        // delete / shrink netRouteObjs,
        checkConnectivity_final(
            net, netDRObjs, netPins, adjVisited, gCnt, nCnt, nodeMap);
        // release lock
      }
    }
  }

  if (isWrong) {
    auto writer = io::Writer(getDesign(), logger_);
    writer.updateDb(db_);
    if (graphics_.get()) {
      graphics_->debugWholeDesign();
    }
    logger_->error(utl::DRT, 206, "checkConnectivity error");
  }
}
