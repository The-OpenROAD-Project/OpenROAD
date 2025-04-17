// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dr/FlexDR_conn.h"

#include <omp.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "dr/FlexDR.h"
#include "frProfileTask.h"
#include "io/io.h"
#include "triton_route/TritonRoute.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;
frRegionQuery* FlexDRConnectivityChecker::getRegionQuery() const
{
  return getDesign()->getRegionQuery();
}

frTechObject* FlexDRConnectivityChecker::getTech() const
{
  return getDesign()->getTech();
}

frDesign* FlexDRConnectivityChecker::getDesign() const
{
  return router_->getDesign();
}
// copied from FlexDRWorker::initNets_searchRepair_pin2epMap_helper
void FlexDRConnectivityChecker::pin2epMap_helper(
    const frNet* net,
    const Point& pt,
    const frLayerNum lNum,
    frOrderedIdMap<frBlockObject*, std::set<std::pair<Point, frLayerNum>>>&
        pin2epMap)
{
  auto regionQuery = getRegionQuery();
  frRegionQuery::Objects<frBlockObject> result;
  Rect query_box(pt.x(), pt.y(), pt.x(), pt.y());
  regionQuery->query(query_box, lNum, result);
  for (auto& [bx, rqObj] : result) {
    switch (rqObj->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj);
        if (instTerm->getNet() == net) {
          pin2epMap[instTerm].insert(std::make_pair(pt, lNum));
        }
        break;
      }
      case frcBTerm: {
        auto bterm = static_cast<frBTerm*>(rqObj);
        if (bterm->getNet() == net) {
          pin2epMap[bterm].insert(std::make_pair(pt, lNum));
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
    frOrderedIdMap<frBlockObject*, std::set<std::pair<Point, frLayerNum>>>&
        pin2epMap)
{
  // to avoid delooping fake planar ep in pin
  std::set<std::pair<Point, frLayerNum>> extEndPoints;
  for (auto& connFig : netRouteObjs) {
    if (connFig->typeId() != frcPathSeg) {
      continue;
    }
    auto obj = static_cast<frPathSeg*>(connFig);
    const auto [bp, ep] = obj->getPoints();
    const frSegStyle& style = obj->getStyle();
    auto lNum = obj->getLayerNum();
    if (style.getBeginStyle() == frEndStyle(frcTruncateEndStyle)) {
      pin2epMap_helper(net, bp, lNum, pin2epMap);
    }
    if (style.getEndStyle() == frEndStyle(frcTruncateEndStyle)) {
      pin2epMap_helper(net, ep, lNum, pin2epMap);
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
    if (obj->isBottomConnected()) {
      pin2epMap_helper(net, origin, l1Num, pin2epMap);
    }
    if (obj->isTopConnected()) {
      pin2epMap_helper(net, origin, l2Num, pin2epMap);
    }
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
      std::cout << "Error: initRoutObjs unsupported type" << std::endl;
    }
  }
  for (auto& uPtr : net->getVias()) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == frcVia) {
      netRouteObjs.push_back(connFig);
    } else {
      std::cout << "Error: initRouteObjs unsupported type" << std::endl;
    }
  }
}

void FlexDRConnectivityChecker::nodeMap_routeObjEnd(
    const frNet* net,
    const std::vector<frConnFig*>& netRouteObjs,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
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
    const std::vector<
        std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
        mergeHelper,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  auto it1 = mergeHelper[lNum].find(trackCoord);
  if (it1 == mergeHelper[lNum].end()) {
    return;
  }
  auto& mp = it1->second;  // std::map<ep, std::pair<bp, objIdx>>
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
    const std::vector<frConnFig*>& netRouteObjs,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  const int numLayers = getTech()->getLayers().size();
  // lNum -> track -> ep -> (bp, segIdx)
  std::vector<std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>
      horzMergeHelper(numLayers);
  std::vector<std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>
      vertMergeHelper(numLayers);
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    const auto connFig = netRouteObjs[i];
    if (connFig->typeId() != frcPathSeg) {
      continue;
    }
    auto obj = static_cast<const frPathSeg*>(connFig);
    const auto [bp, ep] = obj->getPoints();
    auto lNum = obj->getLayerNum();
    if (bp.x() == ep.x()) {  // vert seg
      vertMergeHelper[lNum][bp.x()][ep.y()] = std::make_pair(bp.y(), i);
    } else {  // horz seg
      horzMergeHelper[lNum][bp.y()][ep.x()] = std::make_pair(bp.x(), i);
    }
  }
  for (const auto connFig : netRouteObjs) {
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
    const std::vector<frConnFig*>& netRouteObjs,
    std::vector<frBlockObject*>& netPins,
    const frOrderedIdMap<frBlockObject*,
                         std::set<std::pair<Point, frLayerNum>>>& pin2epMap,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
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
    std::vector<frBlockObject*>& netPins,
    const frOrderedIdMap<frBlockObject*,
                         std::set<std::pair<Point, frLayerNum>>>& pin2epMap,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  nodeMap_routeObjEnd(net, netRouteObjs, nodeMap);
  nodeMap_routeObjSplit(net, netRouteObjs, nodeMap);
  nodeMap_pin(netRouteObjs, netPins, pin2epMap, nodeMap);
}

bool FlexDRConnectivityChecker::astar(
    const frNet* net,
    std::vector<char>& adjVisited,
    std::vector<int>& adjPrevIdx,
    const std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
    const NetRouteObjs& netRouteObjs,
    const int nNetRouteObjs,
    const int nNetObjs)
{
  // a star search
  // node index, node visited
  std::vector<std::vector<int>> adjVec(nNetObjs, std::vector<int>());
  std::vector<char> onPathIdx(nNetObjs, false);
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
        if ((idx1 >= nNetRouteObjs) ^ (idx2 >= nNetRouteObjs)) {
          // one of them is a pin
          // check that the route object connects to the pin
          const auto route_obj_idx = (idx1 >= nNetRouteObjs) ? idx2 : idx1;
          if (netRouteObjs[route_obj_idx]->typeId() == frcPathSeg) {
            auto ps = static_cast<frPathSeg*>(netRouteObjs[route_obj_idx]);
            bool valid_connection = false;
            if (ps->getBeginPoint() == pr.first
                && ps->getBeginStyle() == frEndStyle(frcTruncateEndStyle)) {
              valid_connection = true;
            }
            if (ps->getEndPoint() == pr.first
                && ps->getEndStyle() == frEndStyle(frcTruncateEndStyle)) {
              valid_connection = true;
            }
            if (!valid_connection) {
              continue;
            }
          } else if (netRouteObjs[route_obj_idx]->typeId() == frcVia) {
            auto via = static_cast<frVia*>(netRouteObjs[route_obj_idx]);
            if (!via->isBottomConnected() && !via->isTopConnected()) {
              continue;
            }
          } else {
            continue;
          }
        }
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
      }
      return cost > b.cost;
    }
  };
  for (int findNode = nNetRouteObjs; findNode < nNetObjs - 1; findNode++) {
    std::priority_queue<wf> pq;
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
    std::cout << "Error: " << net->getName() << " "
              << nNetObjs - nNetRouteObjs - pinVisited
              << " pin not visited #guides = " << nNetRouteObjs << std::endl;
  }
  return pinVisited == nNetObjs - nNetRouteObjs;
}

void FlexDRConnectivityChecker::finish(
    frNet* net,
    NetRouteObjs& netRouteObjs,
    const std::vector<frBlockObject*>& netPins,
    const std::vector<char>& adjVisited,
    const int gCnt,
    const int nCnt,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  auto regionQuery = getRegionQuery();

  // from obj to pt
  std::map<int, std::set<std::pair<Point, frLayerNum>>> reverseNodeMap;
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
        addMarker(net, victimPathSeg->getLayerNum(), victimPathSeg->getBBox());
        if (save_updates_) {
          drUpdate update(drUpdate::REMOVE_FROM_NET);
          update.setNet(victimPathSeg->getNet());
          update.setIndexInOwner(victimPathSeg->getIndexInOwner());
          getDesign()->addUpdate(update);
        }
        regionQuery->removeDRObj(static_cast<frShape*>(netRouteObjs[i]));
        net->removeShape(static_cast<frShape*>(netRouteObjs[i]));
      } else if (netRouteObjs[i]->typeId() == frcVia) {
        auto victimVia = static_cast<frVia*>(netRouteObjs[i]);
        // negative rule
        Rect bbox = victimVia->getLayer1BBox();
        addMarker(net, victimVia->getViaDef()->getLayer1Num(), bbox);

        frVia* via = static_cast<frVia*>(netRouteObjs[i]);
        if (save_updates_) {
          drUpdate update(drUpdate::REMOVE_FROM_NET);
          update.setNet(via->getNet());
          update.setIndexInOwner(via->getIndexInOwner());
          getDesign()->addUpdate(update);
        }
        regionQuery->removeDRObj(via);
        net->removeVia(via);
      } else {
        std::cout << "Error: finish unsupported type" << std::endl;
        exit(1);
      }
      netRouteObjs[i] = nullptr;
    } else {
      std::cout << "Error: finish i >= gCnt" << std::endl;
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
  std::map<std::pair<Point, frLayerNum>, int> psSplits;
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
        }
        psIdx = idx;
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

  std::vector<frPathSeg*> addedPS;
  for (auto it = psSplits.rbegin(); it != psSplits.rend(); it++) {
    auto& [pr, idx1] = *it;
    int idx2 = nCnt + addedPS.size();
    auto& [splitPt, lNum] = pr;
    auto ps1 = static_cast<frPathSeg*>(netRouteObjs[idx1]);
    const auto [bp1, ep1] = ps1->getPoints();
    bool isHorz = (bp1.y() == ep1.y());
    std::set<std::pair<Point, frLayerNum>> newPr1;
    std::set<std::pair<Point, frLayerNum>> newPr2;
    for (auto& [prPt, prLNum] : reverseNodeMap[idx1]) {
      if (isHorz) {
        if (prPt.x() <= splitPt.x()) {
          newPr1.insert(std::make_pair(prPt, prLNum));
        } else {
          nodeMap[std::make_pair(prPt, prLNum)].erase(idx1);
        }
        if (prPt.x() >= splitPt.x()) {
          newPr2.insert(std::make_pair(prPt, prLNum));
          nodeMap[std::make_pair(prPt, prLNum)].insert(idx2);
        }
      } else {
        if (prPt.y() <= splitPt.y()) {
          newPr1.insert(std::make_pair(prPt, prLNum));
        } else {
          nodeMap[std::make_pair(prPt, prLNum)].erase(idx1);
        }
        if (prPt.y() >= splitPt.y()) {
          newPr2.insert(std::make_pair(prPt, prLNum));
          nodeMap[std::make_pair(prPt, prLNum)].insert(idx2);
        }
      }
    }

    reverseNodeMap[idx1].clear();
    reverseNodeMap[idx1] = std::move(newPr1);
    reverseNodeMap[idx2] = std::move(newPr2);

    auto uPs2 = std::make_unique<frPathSeg>(*ps1);
    auto ps2 = uPs2.get();
    addedPS.push_back(ps2);
    std::unique_ptr<frShape> uShape(std::move(uPs2));
    if (save_updates_) {
      drUpdate update(drUpdate::ADD_SHAPE_NET_ONLY);
      update.setNet(ps2->getNet());
      update.setPathSeg(*ps2);
      getDesign()->addUpdate(update);
    }
    net->addShape(std::move(uShape));
    // manipulate ps1
    if (save_updates_) {
      drUpdate update(drUpdate::REMOVE_FROM_RQ);
      update.setNet(ps1->getNet());
      update.setIndexInOwner(ps1->getIndexInOwner());
      getDesign()->addUpdate(update);
    }
    regionQuery->removeDRObj(ps1);
    frSegStyle ps1Style = ps1->getStyle();
    ps1Style.setEndStyle(frEndStyle(frcTruncateEndStyle), 0);
    ps1->setStyle(ps1Style);
    ps1->setPoints(bp1, splitPt);
    if (save_updates_) {
      drUpdate update(drUpdate::UPDATE_SHAPE);
      update.setNet(ps1->getNet());
      update.setIndexInOwner(ps1->getIndexInOwner());
      update.setPathSeg(*ps1);
      getDesign()->addUpdate(update);
    }
    regionQuery->addDRObj(ps1);

    // manipulate ps2
    frSegStyle ps2Style = ps2->getStyle();
    ps2Style.setBeginStyle(frEndStyle(frcTruncateEndStyle), 0);
    ps2->setStyle(ps2Style);
    ps2->setPoints(splitPt, ep1);
    if (save_updates_) {
      drUpdate update(drUpdate::UPDATE_SHAPE);
      update.setNet(ps2->getNet());
      update.setIndexInOwner(ps2->getIndexInOwner());
      update.setPathSeg(*ps2);
      getDesign()->addUpdate(update);
    }
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
    auto [minPr, maxPr] = std::minmax_element(ptS.begin(), ptS.end());
    auto& minPt = minPr->first;
    auto& maxPt = maxPr->first;
    // shrink segment
    if (bp < minPt || maxPt < ep) {
      // negative rule
      addMarker(net, ps->getLayerNum(), ps->getBBox());
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_RQ);
        update.setNet(ps->getNet());
        update.setIndexInOwner(ps->getIndexInOwner());
        getDesign()->addUpdate(update);
      }
      regionQuery->removeDRObj(ps);
      ps->setPoints(minPt, maxPt);
      if (save_updates_) {
        drUpdate update(drUpdate::UPDATE_SHAPE);
        update.setNet(ps->getNet());
        update.setIndexInOwner(ps->getIndexInOwner());
        update.setPathSeg(*ps);
        getDesign()->addUpdate(update);
      }
      regionQuery->addDRObj(ps);
    }
  }

  // delete redundant pwires
  std::set<std::pair<Point, frLayerNum>> validPoints;
  frLayerNum lNum;
  for (auto& connFig : net->getShapes()) {
    if (connFig->typeId() == frcPathSeg) {
      auto obj = static_cast<frPathSeg*>(connFig.get());
      const auto [bp, ep] = obj->getPoints();
      lNum = obj->getLayerNum();
      validPoints.insert(std::make_pair(bp, lNum));
      validPoints.insert(std::make_pair(ep, lNum));
    } else {
      std::cout << "Error: finish unsupported type" << std::endl;
      exit(1);
    }
  }
  for (auto& connFig : net->getVias()) {
    if (connFig->typeId() == frcVia) {
      auto obj = static_cast<frVia*>(connFig.get());
      const Point origin = obj->getOrigin();
      lNum = obj->getViaDef()->getLayer1Num();
      validPoints.insert(std::make_pair(origin, lNum));
      lNum = obj->getViaDef()->getLayer2Num();
      validPoints.insert(std::make_pair(origin, lNum));
    } else {
      std::cout << "Error: finish unsupported type" << std::endl;
      exit(1);
    }
  }
  for (auto it = net->getPatchWires().begin();
       it != net->getPatchWires().end();) {
    auto obj = static_cast<frPatchWire*>(it->get());
    it++;
    const Point origin = obj->getOrigin();
    lNum = obj->getLayerNum();
    if (validPoints.find(std::make_pair(origin, lNum)) == validPoints.end()) {
      // negative rule
      addMarker(net, obj->getLayerNum(), obj->getBBox());
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_NET);
        update.setNet(obj->getNet());
        update.setIndexInOwner(obj->getIndexInOwner());
        getDesign()->addUpdate(update);
      }
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
      std::cout << "Error: non-orthogonal wires in merge\n";
    }
  }
}

void FlexDRConnectivityChecker::findSegmentOverlaps(
    NetRouteObjs& netRouteObjs,
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
      handleOverlaps_perform(
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
      handleOverlaps_perform(
          netRouteObjs, indices, victims, newSegSpans, false /*isHorz*/);
      i++;
    }
  }
}

void FlexDRConnectivityChecker::handleSegmentOverlaps(
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

void FlexDRConnectivityChecker::handleOverlaps_perform(
    NetRouteObjs& netRouteObjs,
    const std::vector<int>& indices,
    std::vector<int>& victims,
    std::vector<Span>& newSegSpans,
    const bool isHorz)
{
  std::vector<std::pair<Span, int>> segSpans;
  for (auto& idx : indices) {
    auto obj = static_cast<frPathSeg*>(netRouteObjs[idx]);
    const auto [bp, ep] = obj->getPoints();
    if (isHorz) {
      segSpans.push_back({{bp.x(), ep.x()}, idx});
      if (bp.x() >= ep.x()) {
        std::cout << "Error1: bp.x() >= ep.x()" << bp << " " << ep << "\n";
      }
    } else {
      segSpans.push_back({{bp.y(), ep.y()}, idx});
      if (bp.y() >= ep.y()) {
        std::cout << "Error2: bp.y() >= ep.y()" << bp << " " << ep << "\n";
      }
    }
  }
  std::sort(segSpans.begin(), segSpans.end());

  splitPathSegs(netRouteObjs, segSpans);
  // get victim segments and merged segments
  merge_perform_helper(netRouteObjs, segSpans, victims, newSegSpans);
}

bool isRedundant(std::vector<int>& splitPoints, int v)
{
  return std::find(splitPoints.begin(), splitPoints.end(), v)
         != splitPoints.end();
}
void FlexDRConnectivityChecker::splitPathSegs(
    NetRouteObjs& netRouteObjs,
    std::vector<std::pair<Span, int>>& segSpans)
{
  frPathSeg* highestPs = nullptr;  // overlapping ps with the highest endPoint
  int first = 0;
  std::vector<int> splitPoints;
  if (segSpans.empty()) {
    return;
  }
  for (int i = 0; i < segSpans.size(); i++) {
    auto& curr = segSpans[i];
    frPathSeg* currPs = static_cast<frPathSeg*>(netRouteObjs[curr.second]);
    if (!highestPs || curr.first.lo >= highestPs->high()) {
      if (!splitPoints.empty() && highestPs != nullptr) {
        splitPathSegs_commit(
            splitPoints, highestPs, first, i, segSpans, netRouteObjs);
      }
      first = i;
      highestPs = currPs;
    } else {
      // this section tries to gather all split points (truncatStyle points)
      // involved in the overlapping of pathsegs
      auto& prev = segSpans[i - 1];
      frPathSeg* prevPs = static_cast<frPathSeg*>(netRouteObjs[prev.second]);
      if (curr.first.lo != segSpans[first].first.lo
          && currPs->isBeginTruncated()
          && !isRedundant(splitPoints, curr.first.lo)) {
        splitPoints.push_back(curr.first.lo);
      }
      if (currPs->isEndTruncated()
          && !isRedundant(splitPoints, curr.first.hi)) {
        splitPoints.push_back(curr.first.hi);
      }
      if (i - 1 == first) {
        if (prevPs->isEndTruncated()
            && !isRedundant(splitPoints, prevPs->high())) {
          splitPoints.push_back(prevPs->high());
        }
      }
      if (highestPs->high() < curr.first.hi) {
        highestPs = currPs;
      }
    }
  }
  if (!splitPoints.empty()) {
    int i = segSpans.size();
    splitPathSegs_commit(
        splitPoints, highestPs, first, i, segSpans, netRouteObjs);
  }
}
void FlexDRConnectivityChecker::splitPathSegs_commit(
    std::vector<int>& splitPoints,
    frPathSeg* highestPs,
    int first,
    int& i,
    std::vector<std::pair<Span, int>>& segSpans,
    NetRouteObjs& netRouteObjs)
{
  sort(splitPoints.begin(), splitPoints.end());
  if (splitPoints[splitPoints.size() - 1]
      == highestPs->high()) {  // we don't need this split point, only those
                               // inside the overlapping interval
    splitPoints.erase(std::prev(splitPoints.end()));
  }
  if (!splitPoints.empty()) {
    frEndStyle highestPsEndStyle = highestPs->getEndStyle();
    int highestHi = highestPs->high();
    std::vector<int> splitSpanIdxs;
    for (int k = first; k < i;
         k++) {  // detect split ps's. We want to get all path segs that have
                 // some split point in the middle
      auto& spn = segSpans[k].first;
      for (auto p : splitPoints) {
        if (spn.lo < p && spn.hi > p) {
          splitSpanIdxs.push_back(k);
          break;
        }
      }
    }
    int currIdxSplitSpanIdxs = 0;
    int s;
    // change existing split ps's and spans to match split points
    // TODO: This part is not handled in distributed detailed routing.
    for (s = 0;
         s <= splitPoints.size() && currIdxSplitSpanIdxs < splitSpanIdxs.size();
         s++, currIdxSplitSpanIdxs++) {
      int segSpnIdx = splitSpanIdxs[currIdxSplitSpanIdxs];
      frPathSeg* ps
          = static_cast<frPathSeg*>(netRouteObjs[segSpans[segSpnIdx].second]);
#pragma omp critical
      {
        getRegionQuery()->removeDRObj(ps);
        // set low
        if (s != 0) {
          segSpans[segSpnIdx].first.lo = splitPoints[s - 1];
          ps->setLow(splitPoints[s - 1]);
          ps->setBeginStyle(frcTruncateEndStyle);
        }
        // set high
        if (s == splitPoints.size()) {
          ps->setHigh(highestHi);
          ps->setEndStyle(highestPsEndStyle);
          ps->setBeginStyle(frcTruncateEndStyle);
          segSpans[segSpnIdx].first.hi = highestHi;
          // if we have 2 or more splitting pathsegs than split points. This
          // will create redundant pathsegs that will be merged later
          if (currIdxSplitSpanIdxs < splitSpanIdxs.size() - 1) {
            s--;
          }
        } else {
          segSpans[segSpnIdx].first.hi = splitPoints[s];
          ps->setHigh(splitPoints[s]);
          ps->setEndStyle(frcTruncateEndStyle);
        }
        getRegionQuery()->addDRObj(ps);
      }
    }
    // add remaining splits. This will create new split pathsegs
    for (; s <= splitPoints.size(); s++) {
      int lo = splitPoints[s - 1];
      int hi;
      frEndStyle hiStyle;
      if (s == splitPoints.size()) {
        hiStyle = highestPsEndStyle;
        hi = highestHi;
      } else {
        hi = splitPoints[s];
        hiStyle = frcTruncateEndStyle;
      }
      auto newSpan = std::pair<Span, int>({lo, hi}, netRouteObjs.size());
      segSpans.insert(segSpans.begin() + i, newSpan);  // add last segment piece
      i++;
      std::unique_ptr<frPathSeg> newPs = std::make_unique<frPathSeg>();
      Point begin, end;
      if (highestPs->isVertical()) {
        begin = {highestPs->getBeginPoint().x(), lo};
        end = {highestPs->getBeginPoint().x(), hi};
      } else {
        begin = {lo, highestPs->getBeginPoint().y()};
        end = {hi, highestPs->getBeginPoint().y()};
      }
      newPs->setPoints(begin, end);
      newPs->setBeginStyle(frEndStyle(frcTruncateEndStyle));
      newPs->setEndStyle(hiStyle);
      newPs->setLayerNum(highestPs->getLayerNum());
      frPathSeg* ptr = newPs.get();
      netRouteObjs.push_back(ptr);
#pragma omp critical
      {
        if (save_updates_) {
          drUpdate update(drUpdate::ADD_SHAPE);
          update.setNet(highestPs->getNet());
          update.setPathSeg(*newPs);
          getDesign()->addUpdate(update);
        }
      }
      highestPs->getNet()->addShape(std::move(newPs));
#pragma omp critical
      getRegionQuery()->addDRObj(ptr);
    }
    sort(segSpans.begin() + first, segSpans.begin() + i);
  }
  splitPoints.clear();
}

void FlexDRConnectivityChecker::merge_perform_helper(
    NetRouteObjs& netRouteObjs,
    const std::vector<std::pair<Span, int>>& segSpans,
    std::vector<int>& victims,
    std::vector<Span>& newSegSpans)
{
  bool hasOverlap = false;
  frCoord currStart = INT_MAX, currEnd = INT_MIN;
  std::vector<int> localVictims;
  frPathSeg* currEndPs = nullptr;
  for (auto& segSpan : segSpans) {
    frPathSeg* ps = static_cast<frPathSeg*>(netRouteObjs[segSpan.second]);
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
      currEndPs = ps;
      localVictims.push_back(segSpan.second);
    } else {
      if (ps->isBeginTruncated()
          && ((currEnd < ps->high() && currEndPs->isEndTruncated())
              || (currEnd > ps->high() && ps->isEndTruncated()))) {
        logger_->warn(
            DRT, 6001, "Path segs were not split: {} and {}", *ps, *currEndPs);
      }
      hasOverlap = true;
      // update local variables
      localVictims.push_back(segSpan.second);
      currEnd = std::max(currEnd, segSpan.first.hi);
      if (currEnd == currEndPs->high()) {
        if (currEnd == ps->high() && !currEndPs->isEndTruncated()) {
          currEndPs = ps;
        }
      } else {
        currEndPs = ps;
      }
    }
  }
  if (hasOverlap) {
    newSegSpans.push_back({currStart, currEnd});
    victims.insert(victims.end(), localVictims.begin(), localVictims.end());
  }
}

void FlexDRConnectivityChecker::merge_commit(
    frNet* net,
    std::vector<frConnFig*>& netRouteObjs,
    const std::vector<int>& victims,
    const frCoord trackCoord,
    const std::vector<Span>& newSegSpans,
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
    if (save_updates_) {
      drUpdate update(drUpdate::REMOVE_FROM_RQ);
      update.setNet(victimPathSeg->getNet());
      update.setIndexInOwner(victimPathSeg->getIndexInOwner());
      getDesign()->addUpdate(update);
    }
    regionQuery->removeDRObj(static_cast<frShape*>(victimPathSeg));

    Point bp, ep;
    frCoord high = victimPathSeg->high();
    if (isHorz) {
      bp = {newSegSpan.lo, trackCoord};
      ep = {newSegSpan.hi, trackCoord};
    } else {
      bp = {trackCoord, newSegSpan.lo};
      ep = {trackCoord, newSegSpan.hi};
    }
    victimPathSeg->setPoints(bp, ep);
    cnt++;
    frEndStyle end_style = victimPathSeg->getEndStyle();
    frUInt4 end_ext = victimPathSeg->getEndExt();
    for (; cnt < (int) victims.size(); cnt++) {
      frPathSeg* curr = static_cast<frPathSeg*>(netRouteObjs[victims[cnt]]);
      if (curr->high() > newSegSpan.hi) {
        break;
      }
      if (curr->high() >= high) {
        end_style = curr->getEndStyle();
        end_ext = curr->getEndExt();
        high = curr->high();
      }
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_NET);
        update.setNet(curr->getNet());
        update.setIndexInOwner(curr->getIndexInOwner());
        getDesign()->addUpdate(update);
      }
      regionQuery->removeDRObj(curr);  // deallocates curr
      net->removeShape(curr);
      netRouteObjs[victims[cnt]] = nullptr;
    }
    victimPathSeg->setEndStyle(end_style, end_ext);
    regionQuery->addDRObj(victimPathSeg);
    if (save_updates_) {
      drUpdate update(drUpdate::UPDATE_SHAPE);
      update.setNet(victimPathSeg->getNet());
      update.setIndexInOwner(victimPathSeg->getIndexInOwner());
      update.setPathSeg(*victimPathSeg);
      getDesign()->addUpdate(update);
    }
  }
}
void FlexDRConnectivityChecker::addMarker(frNet* net,
                                          frLayerNum lNum,
                                          const Rect& bbox)
{
  auto regionQuery = getRegionQuery();
  auto marker = std::make_unique<frMarker>();
  marker->setBBox(bbox);
  marker->setLayerNum(lNum);
  marker->setConstraint(getTech()->getLayer(lNum)->getRecheckConstraint());
  marker->addSrc(net);
  marker->addVictim(net, std::make_tuple(lNum, bbox, false));
  marker->addAggressor(net, std::make_tuple(lNum, bbox, false));
  if (save_updates_) {
    drUpdate update(drUpdate::ADD_SHAPE);
    update.setMarker(*marker);
    getDesign()->addUpdate(update);
  }
  regionQuery->addMarker(marker.get());
  getDesign()->getTopBlock()->addMarker(std::move(marker));
}

// feedthrough and loop check
void FlexDRConnectivityChecker::check(int iter)
{
  ProfileTask profile("checkConnectivity");
  bool isWrong = false;

  int batchSize = 1 << 17;  // 128k
  std::vector<std::vector<frNet*>> batches(1);
  batches.reserve(32);
  batches.back().reserve(batchSize);
  for (auto& uPtr : getDesign()->getTopBlock()->getNets()) {
    auto net = uPtr.get();
    if (!net->isModified()) {
      continue;
    }
    net->setModified(false);
    if ((int) batches.back().size() < batchSize) {
      batches.back().push_back(net);
    } else {
      batches.emplace_back(std::vector<frNet*>());
      batches.back().reserve(batchSize);
      batches.back().push_back(net);
    }
  }
  if (batches[0].empty()) {
    batches.clear();
  }

  const int numLayers = getTech()->getLayers().size();
  omp_set_num_threads(router_cfg_->MAX_THREADS);
  for (auto& batch : batches) {
    ProfileTask profile("batch");
    // prefix a = all batch
    // net->figs
    std::vector<NetRouteObjs> aNetRouteObjs(batchSize);
    // net->layer->track->indices of RouteObj
    std::vector<PathSegsByLayerAndTrack> aHorzPathSegs(
        batchSize, PathSegsByLayerAndTrack(numLayers));
    std::vector<PathSegsByLayerAndTrack> aVertPathSegs(
        batchSize, PathSegsByLayerAndTrack(numLayers));
    // net->lnum->trackIdx->objIdxs
    std::vector<PathSegsByLayerAndTrackId> aHorzVictims(
        batchSize, PathSegsByLayerAndTrackId(numLayers));
    std::vector<PathSegsByLayerAndTrackId> aVertVictims(
        batchSize, PathSegsByLayerAndTrackId(numLayers));
    // net->lnum->trackIdx->seg_start_end_pairs
    std::vector<SpansByLayerAndTrackId> aHorzNewSegSpans(
        batchSize, SpansByLayerAndTrackId(numLayers));
    std::vector<SpansByLayerAndTrackId> aVertNewSegSpans(
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
      handleSegmentOverlaps(net,
                            initNetRouteObjs,
                            horzPathSegs,
                            vertPathSegs,
                            horzVictims,
                            vertVictims,
                            horzNewSegSpans,
                            vertNewSegSpans);
    }
    // net->term/instTerm->pt_layer
    std::vector<
        frOrderedIdMap<frBlockObject*, std::set<std::pair<Point, frLayerNum>>>>
        aPin2epMap(batchSize);
    std::vector<std::vector<frBlockObject*>> aNetPins(batchSize);
    std::vector<std::map<std::pair<Point, frLayerNum>, std::set<int>>> aNodeMap(
        batchSize);
    std::vector<std::vector<char>> aAdjVisited(batchSize);
    std::vector<std::vector<int>> aAdjPrevIdx(batchSize);
    std::vector<char> status(batchSize, false);

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
        std::cout << "Error: checkConnectivity break, net " << net->getName()
                  << std::endl
                  << "Objs not visited:\n";
        for (int idx = 0; idx < (int) adjVisited.size(); idx++) {
          if (!adjVisited[idx]) {
            if (idx < (int) netRouteObjs.size()) {
              std::cout << *(netRouteObjs[idx]) << "\n";
            } else if (idx - netRouteObjs.size() < netRouteObjs.size()) {
              std::cout << *(netPins[idx - netRouteObjs.size()]) << "\n";
            }
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
    auto writer = io::Writer(getDesign(), logger_);
    writer.updateDb(router_->getDb(), router_cfg_);
    logger_->error(utl::DRT, 206, "checkConnectivity error.");
  }
}

FlexDRConnectivityChecker::FlexDRConnectivityChecker(
    drt::TritonRoute* router,
    Logger* logger,
    RouterConfiguration* router_cfg,
    AbstractDRGraphics* graphics,
    bool save_updates)
    : router_(router),
      logger_(logger),
      router_cfg_(router_cfg),
      graphics_(graphics),
      save_updates_(save_updates)
{
}

}  // namespace drt
