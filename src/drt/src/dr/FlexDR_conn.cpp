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
#include "dr/FlexDR_conn.h"
#include "frProfileTask.h"
#include "io/io.h"

#include "utl/exception.h"

using namespace std;
using namespace fr;

using utl::ThreadException;

// copied from FlexDRWorker::initNets_searchRepair_pin2epMap_helper
void FlexDRConnectivityChecker::pin2epMap_helper(
    const frNet* net,
    const Point& pt,
    const frLayerNum lNum,
    map<frBlockObject*, set<pair<Point, frLayerNum>>, frBlockObjectComp>&
        pin2epMap,
    const bool isPathSeg)
{
  auto regionQuery = getRegionQuery();
  frRegionQuery::Objects<frBlockObject> result;
  //  In PA we may have used NearbyGrid which puts a via outside the pin
  //  but still overlapping.  So we expand pt to a min-width square when
  //  searching for the pin shape.
  auto half_min_width = getTech()->getLayer(lNum)->getMinWidth() / 2;
  Rect query_box(pt.x() - half_min_width,
                  pt.y() - half_min_width,
                  pt.x() + half_min_width,
                  pt.y() + half_min_width);
  regionQuery->query(query_box, lNum, result);
  for (auto& [bx, rqObj] : result) {
    if (isPathSeg && !bx.intersects(pt))
      continue;
    switch (rqObj->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj);
        if (instTerm->getNet() == net) {
          if (!isPathSeg && !bx.intersects(pt)
              && !instTerm->hasAccessPoint(pt.x(), pt.y(), lNum))
            continue;
          pin2epMap[instTerm].insert(make_pair(pt, lNum));
        }
        break;
      }
      case frcBTerm: {
        if (!isPathSeg && !bx.intersects(pt))
          continue;
        auto bterm = static_cast<frBTerm*>(rqObj);
        if (bterm->getNet() == net) {
          pin2epMap[bterm].insert(make_pair(pt, lNum));
        }
        break;
      }
      default:
        break;
    }
  }
}

void FlexDRConnectivityChecker::buildPin2epMap(
    const frNet* net,
    const NetRouteObjs& netRouteObjs,
    map<frBlockObject*, set<pair<Point, frLayerNum>>, frBlockObjectComp>&
        pin2epMap)
{
  // to avoid delooping fake planar ep in pin
  set<pair<Point, frLayerNum>> extEndPoints;
  for (auto& connFig : netRouteObjs) {
    if (connFig->typeId() != frcPathSeg) {
      continue;
    }
    auto obj = static_cast<frPathSeg*>(connFig);
    const auto [bp, ep] = obj->getPoints();
    frSegStyle style;
    obj->getStyle(style);
    auto lNum = obj->getLayerNum();
    if (style.getBeginStyle() == frEndStyle(frcTruncateEndStyle)) {
      pin2epMap_helper(net, bp, lNum, pin2epMap, true);
    }
    if (style.getEndStyle() == frEndStyle(frcTruncateEndStyle)) {
      pin2epMap_helper(net, ep, lNum, pin2epMap, true);
    }
  }

  for (auto& connFig : netRouteObjs) {
    if (connFig->typeId() != frcVia) {
      continue;
    }
    auto obj = static_cast<frVia*>(connFig);
    const Point origin = obj->getOrigin();
    auto l1Num = obj->getViaDef()->getLayer1Num();
    auto l2Num = obj->getViaDef()->getLayer2Num();
    if (obj->isBottomConnected())
      pin2epMap_helper(net, origin, l1Num, pin2epMap, false);
    if (obj->isTopConnected())
      pin2epMap_helper(net, origin, l2Num, pin2epMap, false);
  }
}

void FlexDRConnectivityChecker::initRouteObjs(const frNet* net,
                                              NetRouteObjs& netRouteObjs)
{
  for (auto& uPtr : net->getShapes()) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == frcPathSeg) {
      netRouteObjs.push_back(connFig);
    } else {
      cout << "Error: initRoutObjs unsupported type" << endl;
    }
  }
  for (auto& uPtr : net->getVias()) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == frcVia) {
      netRouteObjs.push_back(connFig);
    } else {
      cout << "Error: initRouteObjs unsupported type" << endl;
    }
  }
}

void FlexDRConnectivityChecker::nodeMap_routeObjEnd(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    const auto connFig = netRouteObjs[i];
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<const frPathSeg*>(connFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      nodeMap[{bp, lNum}].insert(i);
      nodeMap[{ep, lNum}].insert(i);
    } else if (connFig->typeId() == frcVia) {
      auto obj = static_cast<const frVia*>(connFig);
      const Point origin = obj->getOrigin();
      const auto l1Num = obj->getViaDef()->getLayer1Num();
      const auto l2Num = obj->getViaDef()->getLayer2Num();
      nodeMap[{origin, l1Num}].insert(i);
      nodeMap[{origin, l2Num}].insert(i);
    }
  }
}

void FlexDRConnectivityChecker::nodeMap_routeObjSplit_helper(
    const Point& crossPt,
    const frCoord trackCoord,
    const frCoord splitCoord,
    const frLayerNum lNum,
    const vector<map<frCoord, map<frCoord, pair<frCoord, int>>>>& mergeHelper,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  auto it1 = mergeHelper[lNum].find(trackCoord);
  if (it1 == mergeHelper[lNum].end()) {
    return;
  }
  auto& mp = it1->second;  // map<ep, pair<bp, objIdx>>
  auto it2 = mp.lower_bound(splitCoord);
  if (it2 == mp.end()) {
    return;
  }
  auto& endP = it2->first;
  auto& [beginP, objIdx] = it2->second;
  if (beginP < splitCoord && splitCoord < endP) {
    nodeMap[{crossPt, lNum}].insert(objIdx);
  }
}

void FlexDRConnectivityChecker::nodeMap_routeObjSplit(
    const frNet* net,
    const vector<frConnFig*>& netRouteObjs,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  const int numLayers = getTech()->getLayers().size();
  // lNum -> track -> ep -> (bp, segIdx)
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> horzMergeHelper(
      numLayers);
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> vertMergeHelper(
      numLayers);
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    const auto connFig = netRouteObjs[i];
    if (connFig->typeId() != frcPathSeg) {
      continue;
    }
    auto obj = static_cast<const frPathSeg*>(connFig);
    const auto [bp, ep] = obj->getPoints();
    auto lNum = obj->getLayerNum();
    if (bp.x() == ep.x()) {  // vert seg
      vertMergeHelper[lNum][bp.x()][ep.y()] = make_pair(bp.y(), i);
    } else {  // horz seg
      horzMergeHelper[lNum][bp.y()][ep.x()] = make_pair(bp.x(), i);
    }
  }
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    const auto connFig = netRouteObjs[i];
    // ep on pathseg
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<const frPathSeg*>(connFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      // vert seg, find horz crossing seg
      if (bp.x() == ep.x()) {
        // find whether there is horz track at bp
        nodeMap_routeObjSplit_helper(
            bp, bp.y(), bp.x(), lNum, horzMergeHelper, nodeMap);
        // find whether there is horz track at ep
        nodeMap_routeObjSplit_helper(
            ep, ep.y(), ep.x(), lNum, horzMergeHelper, nodeMap);
        // horz seg
      } else {
        // find whether there is vert track at bp
        nodeMap_routeObjSplit_helper(
            bp, bp.x(), bp.y(), lNum, vertMergeHelper, nodeMap);
        // find whether there is vert track at ep
        nodeMap_routeObjSplit_helper(
            ep, ep.x(), ep.y(), lNum, vertMergeHelper, nodeMap);
      }
    } else if (connFig->typeId() == frcVia) {
      auto obj = static_cast<const frVia*>(connFig);
      const Point origin = obj->getOrigin();
      const auto lNum1 = obj->getViaDef()->getLayer1Num();
      // find whether there is horz track at origin on layer1
      nodeMap_routeObjSplit_helper(
          origin, origin.y(), origin.x(), lNum1, horzMergeHelper, nodeMap);
      // find whether there is vert track at origin on layer1
      nodeMap_routeObjSplit_helper(
          origin, origin.x(), origin.y(), lNum1, vertMergeHelper, nodeMap);

      const auto lNum2 = obj->getViaDef()->getLayer2Num();
      // find whether there is horz track at origin on layer2
      nodeMap_routeObjSplit_helper(
          origin, origin.y(), origin.x(), lNum2, horzMergeHelper, nodeMap);
      // find whether there is vert track at origin on layer2
      nodeMap_routeObjSplit_helper(
          origin, origin.x(), origin.y(), lNum2, vertMergeHelper, nodeMap);
    }
  }
}

void FlexDRConnectivityChecker::nodeMap_pin(
    const vector<frConnFig*>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    const map<frBlockObject*,
              set<pair<Point, frLayerNum>>,
              frBlockObjectComp>& pin2epMap,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  int currCnt = (int) netRouteObjs.size();
  for (auto& [obj, locS] : pin2epMap) {
    netPins.push_back(obj);
    for (auto& pr : locS) {
      nodeMap[pr].insert(currCnt);
    }
    ++currCnt;
  }
}

void FlexDRConnectivityChecker::buildNodeMap(
    const frNet* net,
    const NetRouteObjs& netRouteObjs,
    vector<frBlockObject*>& netPins,
    const map<frBlockObject*,
              set<pair<Point, frLayerNum>>,
              frBlockObjectComp>& pin2epMap,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  nodeMap_routeObjEnd(net, netRouteObjs, nodeMap);
  nodeMap_routeObjSplit(net, netRouteObjs, nodeMap);
  nodeMap_pin(netRouteObjs, netPins, pin2epMap, nodeMap);
}

bool FlexDRConnectivityChecker::astar(
    const frNet* net,
    vector<char>& adjVisited,
    vector<int>& adjPrevIdx,
    const map<pair<Point, frLayerNum>, set<int>>& nodeMap,
    const NetRouteObjs& netRouteObjs,
    const int nNetRouteObjs,
    const int nNetObjs)
{
  // a star search
  // node index, node visited
  vector<vector<int>> adjVec(nNetObjs, vector<int>());
  vector<char> onPathIdx(nNetObjs, false);
  adjVisited.clear();
  adjPrevIdx.clear();
  adjVisited.resize(nNetObjs, false);
  adjPrevIdx.resize(nNetObjs, -1);
  for (auto& [pr, idxS] : nodeMap) {
    // auto &[pt, lNum] = pr;
    for (auto it1 = idxS.begin(); it1 != idxS.end(); it1++) {
      auto it2 = it1;
      it2++;
      const auto idx1 = *it1;
      for (; it2 != idxS.end(); it2++) {
        const auto idx2 = *it2;
        adjVec[idx1].push_back(idx2);
        adjVec[idx2].push_back(idx1);
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
    priority_queue<wf> pq;
    if (findNode == nNetRouteObjs) {
      // push only first pin into pq
      pq.push({nNetRouteObjs, -1, 0});
    } else {
      // push every visited node into pq
      for (int i = 0; i < nNetObjs; i++) {
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
        lastNodeIdx = wfront.nodeIdx;
        break;
      }
      adjVisited[wfront.nodeIdx] = true;
      adjPrevIdx[wfront.nodeIdx] = wfront.prevIdx;
      // visit other nodes
      for (auto nbrIdx : adjVec[wfront.nodeIdx]) {
        if (!adjVisited[nbrIdx]) {
          pq.push({nbrIdx, wfront.nodeIdx, wfront.cost + 1});
        }
      }
    }
    // trace back path
    while ((lastNodeIdx != -1) && (!onPathIdx[lastNodeIdx])) {
      onPathIdx[lastNodeIdx] = true;
      lastNodeIdx = adjPrevIdx[lastNodeIdx];
    }
    adjVisited = onPathIdx;
  }
  const int pinVisited
      = count(adjVisited.begin() + nNetRouteObjs, adjVisited.end(), true);
  // true error when allowing feedthrough
  if (pinVisited != nNetObjs - nNetRouteObjs) {
    cout << "Error: " << net->getName() << " "
         << nNetObjs - nNetRouteObjs - pinVisited
         << " pin not visited #guides = " << nNetRouteObjs << endl;
  }
  return pinVisited == nNetObjs - nNetRouteObjs;
}

void FlexDRConnectivityChecker::finish(
    frNet* net,
    NetRouteObjs& netRouteObjs,
    const vector<frBlockObject*>& netPins,
    const vector<char>& adjVisited,
    const int gCnt,
    const int nCnt,
    map<pair<Point, frLayerNum>, set<int>>& nodeMap)
{
  auto regionQuery = getRegionQuery();

  // from obj to pt
  map<int, set<pair<Point, frLayerNum>>> reverseNodeMap;
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
        Rect bbox;
        victimPathSeg->getBBox(bbox);
        addMarker(net, victimPathSeg->getLayerNum(), bbox);

        regionQuery->removeDRObj(static_cast<frShape*>(netRouteObjs[i]));
        net->removeShape(static_cast<frShape*>(netRouteObjs[i]));
      } else if (netRouteObjs[i]->typeId() == frcVia) {
        auto victimVia = static_cast<frVia*>(netRouteObjs[i]);
        // negative rule
        Rect bbox;
        victimVia->getLayer1BBox(bbox);
        addMarker(net, victimVia->getViaDef()->getLayer1Num(), bbox);

        frVia* via = static_cast<frVia*>(netRouteObjs[i]);
        regionQuery->removeDRObj(via);
        net->removeVia(via);
      } else {
        cout << "Error: finish unsupporterd type" << endl;
        exit(1);
      }
      netRouteObjs[i] = nullptr;
    } else {
      cout << "Error: finish i >= gCnt" << endl;
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
  map<pair<Point, frLayerNum>, int> psSplits;
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
        const auto [bp, ep] = ps->getPoints();
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
    auto ps1 = static_cast<frPathSeg*>(netRouteObjs[idx1]);
    const auto [bp1, ep1] = ps1->getPoints();
    bool isHorz = (bp1.y() == ep1.y());
    set<pair<Point, frLayerNum>> newPr1;
    set<pair<Point, frLayerNum>> newPr2;
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

    const auto [bp, ep] = ps->getPoints();
    auto [minPr, maxPr] = minmax_element(ptS.begin(), ptS.end());
    auto& minPt = minPr->first;
    auto& maxPt = maxPr->first;
    // shrink segment
    if (bp < minPt || maxPt < ep) {
      // negative rule
      Rect bbox;
      ps->getBBox(bbox);
      addMarker(net, ps->getLayerNum(), bbox);

      regionQuery->removeDRObj(ps);
      ps->setPoints(minPt, maxPt);
      regionQuery->addDRObj(ps);
    }
  }

  // delete redundant pwires
  set<pair<Point, frLayerNum>> validPoints;
  frLayerNum lNum;
  for (auto& connFig : net->getShapes()) {
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig.get());
      const auto [bp, ep] = obj->getPoints();
      lNum = obj->getLayerNum();
      validPoints.insert(make_pair(bp, lNum));
      validPoints.insert(make_pair(ep, lNum));
    } else {
      cout << "Error: finish unsupporterd type" << endl;
      exit(1);
    }
  }
  for (auto& connFig : net->getVias()) {
    if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig.get());
      const Point origin = obj->getOrigin();
      lNum = obj->getViaDef()->getLayer1Num();
      validPoints.insert(make_pair(origin, lNum));
      lNum = obj->getViaDef()->getLayer2Num();
      validPoints.insert(make_pair(origin, lNum));
    } else {
      cout << "Error: finish unsupporterd type" << endl;
      exit(1);
    }
  }
  for (auto it = net->getPatchWires().begin();
       it != net->getPatchWires().end();) {
    auto obj = static_cast<frPatchWire*>(it->get());
    it++;
    const Point origin = obj->getOrigin();
    lNum = obj->getLayerNum();
    if (validPoints.find(make_pair(origin, lNum)) == validPoints.end()) {
      // negative rule
      Rect bbox;
      obj->getBBox(bbox);
      addMarker(net, obj->getLayerNum(), bbox);

      regionQuery->removeDRObj(obj);
      net->removePatchWire(obj);
    }
  }
}

void FlexDRConnectivityChecker::organizePathSegsByLayerAndTrack(
    const frNet* net,
    const NetRouteObjs& netRouteObjs,
    PathSegsByLayerAndTrack& horzPathSegs,
    PathSegsByLayerAndTrack& vertPathSegs)
{
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    const auto connFig = netRouteObjs[i];
    if (connFig->typeId() != frcPathSeg) {
      continue;
    }

    auto obj = static_cast<const frPathSeg*>(connFig);
    const auto [bp, ep] = obj->getPoints();
    auto lNum = obj->getLayerNum();
    if (bp.x() == ep.x()) {
      vertPathSegs[lNum][bp.x()].push_back(i);
    } else if (bp.y() == ep.y()) {
      horzPathSegs[lNum][bp.y()].push_back(i);
    } else {
      cout << "Error: non-orthogonal wires in merge\n";
    }
  }
}

void FlexDRConnectivityChecker::findSegmentOverlaps(
    const NetRouteObjs& netRouteObjs,
    const PathSegsByLayerAndTrack& horzPathSegs,
    const PathSegsByLayerAndTrack& vertPathSegs,
    PathSegsByLayerAndTrackId& horzVictims,
    PathSegsByLayerAndTrackId& vertVictims,
    SpansByLayerAndTrackId& horzNewSegSpans,
    SpansByLayerAndTrackId& vertNewSegSpans)
{
  for (auto lNum = 0; lNum < (int) horzPathSegs.size(); lNum++) {
    auto& track2Segs = horzPathSegs[lNum];
    horzVictims[lNum].resize(track2Segs.size());
    horzNewSegSpans[lNum].resize(track2Segs.size());
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = horzVictims[lNum][i];
      auto& newSegSpans = horzNewSegSpans[lNum][i];
      merge_perform(
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
      merge_perform(
          netRouteObjs, indices, victims, newSegSpans, false /*isHorz*/);
      i++;
    }
  }
}

void FlexDRConnectivityChecker::mergeSegmentOverlaps(
    frNet* net,
    NetRouteObjs& netRouteObjs,
    const PathSegsByLayerAndTrack& horzPathSegs,
    const PathSegsByLayerAndTrack& vertPathSegs,
    const PathSegsByLayerAndTrackId& horzVictims,
    const PathSegsByLayerAndTrackId& vertVictims,
    const SpansByLayerAndTrackId& horzNewSegSpans,
    const SpansByLayerAndTrackId& vertNewSegSpans)
{
  for (auto lNum = 0; lNum < (int) horzPathSegs.size(); lNum++) {
    auto& track2Segs = horzPathSegs[lNum];
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = horzVictims[lNum][i];
      auto& newSegSpans = horzNewSegSpans[lNum][i];
      merge_commit(
          net, netRouteObjs, victims, trackCoord, newSegSpans, true /*isHorz*/);
      i++;
    }
  }

  for (auto lNum = 0; lNum < (int) vertPathSegs.size(); lNum++) {
    auto& track2Segs = vertPathSegs[lNum];
    int i = 0;
    for (auto& [trackCoord, indices] : track2Segs) {
      auto& victims = vertVictims[lNum][i];
      auto& newSegSpans = vertNewSegSpans[lNum][i];
      merge_commit(net,
                   netRouteObjs,
                   victims,
                   trackCoord,
                   newSegSpans,
                   false /*isHorz*/);
      i++;
    }
  }
}

void FlexDRConnectivityChecker::merge_perform(const NetRouteObjs& netRouteObjs,
                                              const vector<int>& indices,
                                              vector<int>& victims,
                                              vector<Span>& newSegSpans,
                                              const bool isHorz)
{
  vector<pair<Span, int>> segSpans;
  for (auto& idx : indices) {
    auto obj = static_cast<frPathSeg*>(netRouteObjs[idx]);
    const auto [bp, ep] = obj->getPoints();
    if (isHorz) {
      segSpans.push_back({{bp.x(), ep.x()}, idx});
      if (bp.x() >= ep.x()) {
        cout << "Error: bp >= ep\n";
      }
    } else {
      segSpans.push_back({{bp.y(), ep.y()}, idx});
      if (bp.y() >= ep.y()) {
        cout << "Error: bp >= ep\n";
      }
    }
  }
  sort(segSpans.begin(), segSpans.end());

  // get victim segments and merged segments
  merge_perform_helper(segSpans, victims, newSegSpans);
}

void FlexDRConnectivityChecker::merge_perform_helper(
    const vector<pair<Span, int>>& segSpans,
    vector<int>& victims,
    vector<Span>& newSegSpans)
{
  bool hasOverlap = false;
  frCoord currStart = INT_MAX, currEnd = INT_MIN;
  vector<int> localVictims;
  for (auto& segSpan : segSpans) {
    if (segSpan.first.lo >= currEnd) {
      if (hasOverlap) {
        // commit prev merged segs
        newSegSpans.push_back({currStart, currEnd});
        // commit victims in merged segs
        victims.insert(victims.end(), localVictims.begin(), localVictims.end());
      }
      // cleanup
      localVictims.clear();
      hasOverlap = false;
      // update local variables
      currStart = segSpan.first.lo;
      currEnd = segSpan.first.hi;
      localVictims.push_back(segSpan.second);
    } else {
      hasOverlap = true;
      // update local variables
      currEnd = max(currEnd, segSpan.first.hi);
      localVictims.push_back(segSpan.second);
    }
  }
  if (hasOverlap) {
    newSegSpans.push_back({currStart, currEnd});
    victims.insert(victims.end(), localVictims.begin(), localVictims.end());
  }
}

void FlexDRConnectivityChecker::merge_commit(frNet* net,
                                             vector<frConnFig*>& netRouteObjs,
                                             const vector<int>& victims,
                                             const frCoord trackCoord,
                                             const vector<Span>& newSegSpans,
                                             const bool isHorz)
{
  if (victims.empty()) {
    return;
  }
  auto regionQuery = getRegionQuery();
  // add segments from overlapped segments
  int cnt = 0;
  for (auto& newSegSpan : newSegSpans) {
    auto victimPathSeg = static_cast<frPathSeg*>(netRouteObjs[victims[cnt]]);
    regionQuery->removeDRObj(static_cast<frShape*>(victimPathSeg));

    Point bp, ep;
    if (isHorz) {
      bp.set(newSegSpan.lo, trackCoord);
      ep.set(newSegSpan.hi, trackCoord);
    } else {
      bp.set(trackCoord, newSegSpan.lo);
      ep.set(trackCoord, newSegSpan.hi);
    }
    victimPathSeg->setPoints(bp, ep);
    cnt++;
    frEndStyle end_style = victimPathSeg->getEndStyle();
    frUInt4 end_ext = victimPathSeg->getEndExt();
    for (; cnt < (int) victims.size(); cnt++) {
      frPathSeg* curr = static_cast<frPathSeg*>(netRouteObjs[victims[cnt]]);
      if (curr->high() <= newSegSpan.hi) {
        end_style = curr->getEndStyle();
        end_ext = curr->getEndExt();
        regionQuery->removeDRObj(curr);  // deallocates curr
        net->removeShape(curr);
        netRouteObjs[victims[cnt]] = nullptr;
      } else {
        break;
      }
    }
    victimPathSeg->setEndStyle(end_style, end_ext);
    regionQuery->addDRObj(victimPathSeg);
  }
}

void FlexDRConnectivityChecker::addMarker(frNet* net,
                                          frLayerNum lNum,
                                          const Rect& bbox)
{
  auto regionQuery = getRegionQuery();
  auto marker = make_unique<frMarker>();
  marker->setBBox(bbox);
  marker->setLayerNum(lNum);
  marker->setConstraint(getTech()->getLayer(lNum)->getRecheckConstraint());
  marker->addSrc(net);
  marker->addVictim(net, make_tuple(lNum, bbox, false));
  marker->addAggressor(net, make_tuple(lNum, bbox, false));
  regionQuery->addMarker(marker.get());
  design_->getTopBlock()->addMarker(std::move(marker));
}

// feedthrough and loop check
void FlexDRConnectivityChecker::check(int iter)
{
  ProfileTask profile("checkConnectivity");
  bool isWrong = false;

  int batchSize = 1 << 17;  // 128k
  vector<vector<frNet*>> batches(1);
  batches.reserve(32);
  batches.back().reserve(batchSize);
  for (auto& uPtr : design_->getTopBlock()->getNets()) {
    auto net = uPtr.get();
    if (!net->isModified()) {
      continue;
    }
    net->setModified(false);
    if ((int) batches.back().size() < batchSize) {
      batches.back().push_back(net);
    } else {
      batches.push_back(vector<frNet*>());
      batches.back().reserve(batchSize);
      batches.back().push_back(net);
    }
  }
  if (batches[0].empty()) {
    batches.clear();
  }

  const int numLayers = getTech()->getLayers().size();
  omp_set_num_threads(MAX_THREADS);
  for (auto& batch : batches) {
    ProfileTask profile("batch");
    // prefix a = all batch
    // net->figs
    vector<NetRouteObjs> aNetRouteObjs(batchSize);
    // net->layer->track->indices of RouteObj
    vector<PathSegsByLayerAndTrack> aHorzPathSegs(
        batchSize, PathSegsByLayerAndTrack(numLayers));
    vector<PathSegsByLayerAndTrack> aVertPathSegs(
        batchSize, PathSegsByLayerAndTrack(numLayers));
    // net->lnum->trackIdx->objIdxs
    vector<PathSegsByLayerAndTrackId> aHorzVictims(
        batchSize, PathSegsByLayerAndTrackId(numLayers));
    vector<PathSegsByLayerAndTrackId> aVertVictims(
        batchSize, PathSegsByLayerAndTrackId(numLayers));
    // net->lnum->trackIdx->seg_start_end_pairs
    vector<SpansByLayerAndTrackId> aHorzNewSegSpans(
        batchSize, SpansByLayerAndTrackId(numLayers));
    vector<SpansByLayerAndTrackId> aVertNewSegSpans(
        batchSize, SpansByLayerAndTrackId(numLayers));

    ProfileTask init_parallel("init-parallel");
// parallel
    ThreadException exception;
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int) batch.size(); i++) {
      try {
        const auto net = batch[i];
        auto& initNetRouteObjs = aNetRouteObjs[i];
        auto& horzPathSegs = aHorzPathSegs[i];
        auto& vertPathSegs = aVertPathSegs[i];
        auto& horzVictims = aHorzVictims[i];
        auto& vertVictims = aVertVictims[i];
        auto& horzNewSegSpans = aHorzNewSegSpans[i];
        auto& vertNewSegSpans = aVertNewSegSpans[i];
  
        initRouteObjs(net, initNetRouteObjs);
        organizePathSegsByLayerAndTrack(
            net, initNetRouteObjs, horzPathSegs, vertPathSegs);
        findSegmentOverlaps(initNetRouteObjs,
                            horzPathSegs,
                            vertPathSegs,
                            horzVictims,
                            vertVictims,
                            horzNewSegSpans,
                            vertNewSegSpans);
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
    init_parallel.done();
    ProfileTask merge_serial("merge-serial");

    // sequential - writes to the db
    for (int i = 0; i < (int) batch.size(); i++) {
      auto net = batch[i];
      auto& initNetRouteObjs = aNetRouteObjs[i];
      const auto& horzPathSegs = aHorzPathSegs[i];
      const auto& vertPathSegs = aVertPathSegs[i];
      const auto& horzVictims = aHorzVictims[i];
      const auto& vertVictims = aVertVictims[i];
      const auto& horzNewSegSpans = aHorzNewSegSpans[i];
      const auto& vertNewSegSpans = aVertNewSegSpans[i];
      mergeSegmentOverlaps(net,
                           initNetRouteObjs,
                           horzPathSegs,
                           vertPathSegs,
                           horzVictims,
                           vertVictims,
                           horzNewSegSpans,
                           vertNewSegSpans);
    }
    // net->term/instTerm->pt_layer
    vector<
        map<frBlockObject*, set<pair<Point, frLayerNum>>, frBlockObjectComp>>
        aPin2epMap(batchSize);
    vector<vector<frBlockObject*>> aNetPins(batchSize);
    vector<map<pair<Point, frLayerNum>, set<int>>> aNodeMap(batchSize);
    vector<vector<char>> aAdjVisited(batchSize);
    vector<vector<int>> aAdjPrevIdx(batchSize);
    vector<char> status(batchSize, false);

    merge_serial.done();
    ProfileTask astar_parallel("astar-parallel");

// parallel
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int) batch.size(); i++) {
      try {
        const auto net = batch[i];
        auto& netRouteObjs = aNetRouteObjs[i];
        auto& pin2epMap = aPin2epMap[i];
        auto& netPins = aNetPins[i];
        auto& nodeMap = aNodeMap[i];
        auto& adjVisited = aAdjVisited[i];
        auto& adjPrevIdx = aAdjPrevIdx[i];
        netRouteObjs.clear();
        initRouteObjs(net, netRouteObjs);
        buildPin2epMap(net, netRouteObjs, pin2epMap);
        buildNodeMap(net, netRouteObjs, netPins, pin2epMap, nodeMap);
  
        const int nNetRouteObjs = (int) netRouteObjs.size();
        const int nNetObjs = (int) netRouteObjs.size() + (int) netPins.size();
        status[i] = astar(net,
                          adjVisited,
                          adjPrevIdx,
                          nodeMap,
                          netRouteObjs,
                          nNetRouteObjs,
                          nNetObjs);
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
    astar_parallel.done();
    ProfileTask finish_serial("finish-serial");

    // sequential
    for (int i = 0; i < (int) batch.size(); i++) {
      auto net = batch[i];
      auto& netRouteObjs = aNetRouteObjs[i];
      const auto& netPins = aNetPins[i];
      auto& nodeMap = aNodeMap[i];
      const auto& adjVisited = aAdjVisited[i];

      const int gCnt = (int) netRouteObjs.size();
      const int nCnt = (int) netRouteObjs.size() + (int) netPins.size();

      if (!status[i]) {
        cout << "Error: checkConnectivity break, net " << net->getName() << endl
             << "Objs not visited:\n";
        for (int idx = 0; idx < (int) adjVisited.size(); idx++) {
          if (!adjVisited[idx]) {
            if (idx < (int) netRouteObjs.size())
              cout << *(netRouteObjs[idx]) << "\n";
            else
              cout << *(netPins[idx - netRouteObjs.size()]) << "\n";
          }
        }
        isWrong = true;
      } else {
        // get lock
        // delete / shrink netRouteObjs,
        finish(net, netRouteObjs, netPins, adjVisited, gCnt, nCnt, nodeMap);
        // release lock
      }
    }
  }

  if (isWrong) {
    if (graphics_) {
      graphics_->debugWholeDesign();
    }
    auto writer = io::Writer(design_, logger_);
    writer.updateDb(db_);
    logger_->error(utl::DRT, 206, "checkConnectivity error.");
  }
}

FlexDRConnectivityChecker::FlexDRConnectivityChecker(frDesign* design,
                                                     Logger* logger,
                                                     odb::dbDatabase* db,
                                                     FlexDRGraphics* graphics)
    : design_(design), logger_(logger), db_(db), graphics_(graphics)
{
}
