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

#include <algorithm>
#include <boost/polygon/polygon.hpp>

#include "dr/FlexDR.h"
#include "frRTree.h"

using namespace std;
using namespace fr;
namespace bgi = boost::geometry::index;

using Rectangle = boost::polygon::rectangle_data<int>;

void FlexDRWorker::initNetObjs_pathSeg(
    frPathSeg* pathSeg,
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs)
{
  auto gridBBox = getRouteBox();
  auto net = pathSeg->getNet();
  nets.insert(net);
  // split seg
  frPoint begin, end;
  pathSeg->getPoints(begin, end);
  // cout <<"here" <<endl;
  //  vertical seg
  if (begin.x() == end.x()) {
    // cout <<"vert seg" <<endl;
    //  may cross routeBBox
    bool condition1 = isInitDR() ? (begin.x() < gridBBox.right())
                                 : (begin.x() <= gridBBox.right());
    if (gridBBox.left() <= begin.x() && condition1) {
      // cout << " pathSeg (" << begin.x() << ", " << begin.y() << ") - (" <<
      // end.x() << ", " << end.y() << ")\n";
      //  bottom seg to ext
      if (begin.y() < gridBBox.bottom()) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(begin,
                            frPoint(end.x(), min(end.y(), gridBBox.bottom())));
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        if (end.y() < gridBBox.bottom()) {
          netExtObjs[net].push_back(std::move(uDRObj));  // pure ext
        } else {
          // change boundary style to ext if pathSeg does not end exactly at
          // boundary
          frSegStyle style;
          pathSeg->getStyle(style);
          if (end.y() != gridBBox.bottom()) {
            style
                .setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          }
          ps->setStyle(style);

          netRouteObjs[net].push_back(std::move(uDRObj));
        }
      }
      // middle seg to route
      if (!(begin.y() >= gridBBox.top() || end.y() <= gridBBox.bottom())) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(
            frPoint(begin.x(), max(begin.y(), gridBBox.bottom())),
            frPoint(end.x(), min(end.y(), gridBBox.top())));
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        // change boundary style to ext if it does not end within at boundary
        frSegStyle style;
        pathSeg->getStyle(style);
        if (begin.y() < gridBBox.bottom()) {
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
        }
        if (end.y() > gridBBox.top()) {
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
        }
        ps->setStyle(style);

        netRouteObjs[net].push_back(std::move(uDRObj));
      }
      // top seg to ext
      if (end.y() > gridBBox.top()) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(frPoint(begin.x(), max(begin.y(), gridBBox.top())),
                            end);
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        if (begin.y() > gridBBox.top()) {
          netExtObjs[net].push_back(std::move(uDRObj));  // pure ext
        } else {
          // change boundary style to ext if pathSeg does not end exactly at
          // boundary
          frSegStyle style;
          pathSeg->getStyle(style);
          if (begin.y() != gridBBox.top()) {
            style
                .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          }
          ps->setStyle(style);

          netRouteObjs[net].push_back(std::move(uDRObj));
        }
      }
      // cannot cross routeBBox
    } else {
      auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
      unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
      netExtObjs[net].push_back(std::move(uDRObj));
    }
    // horizontal seg
  } else if (begin.y() == end.y()) {
    // cout <<"horz seg" <<endl;
    //  may cross routeBBox
    bool condition1 = isInitDR() ? (begin.y() < gridBBox.top())
                                 : (begin.y() <= gridBBox.top());
    if (gridBBox.bottom() <= begin.y() && condition1) {
      // cout << " pathSeg (" << begin.x() / 2000.0 << ", " << begin.y() /
      // 2000.0 << ") - ("
      //                      << end.x()   / 2000.0 << ", " << end.y() / 2000.0
      //                      << ")\n";
      //  left seg to ext
      if (begin.x() < gridBBox.left()) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(begin,
                            frPoint(min(end.x(), gridBBox.left()), end.y()));
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        if (end.x() < gridBBox.left()) {
          netExtObjs[net].push_back(std::move(uDRObj));  // pure ext
        } else {
          // change boundary style to ext if pathSeg does not end exactly at
          // boundary
          frSegStyle style;
          pathSeg->getStyle(style);
          if (end.x() != gridBBox.left()) {
            style
                .setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          }
          ps->setStyle(style);

          netRouteObjs[net].push_back(std::move(uDRObj));  // touching bounds
        }
      }
      // middle seg to route
      if (!(begin.x() >= gridBBox.right() || end.x() <= gridBBox.left())) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(frPoint(max(begin.x(), gridBBox.left()), begin.y()),
                            frPoint(min(end.x(), gridBBox.right()), end.y()));
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        // change boundary style to ext if it does not end within at boundary
        frSegStyle style;
        pathSeg->getStyle(style);
        if (begin.x() < gridBBox.left()) {
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
        }
        if (end.x() > gridBBox.right()) {
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
        }
        ps->setStyle(style);

        netRouteObjs[net].push_back(std::move(uDRObj));
      }
      // right seg to ext
      if (end.x() > gridBBox.right()) {
        auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        uPathSeg->setPoints(
            frPoint(max(begin.x(), gridBBox.right()), begin.y()), end);
        unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
        if (begin.x() > gridBBox.right()) {
          netExtObjs[net].push_back(std::move(uDRObj));  // pure ext
        } else {
          // change boundary style to ext if pathSeg does not end exactly at
          // boundary
          frSegStyle style;
          pathSeg->getStyle(style);
          if (begin.x() != gridBBox.right()) {
            style
                .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          }
          ps->setStyle(style);

          netRouteObjs[net].push_back(std::move(uDRObj));  // touching bounds
        }
      }
      // cannot cross routeBBox
    } else {
      auto uPathSeg = make_unique<drPathSeg>(*pathSeg);
      unique_ptr<drConnFig> uDRObj(std::move(uPathSeg));
      netExtObjs[net].push_back(std::move(uDRObj));
    }
  } else {
    cout << "wtf" << endl;
  }
}

void FlexDRWorker::initNetObjs_via(
    frVia* via,
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs)
{
  auto gridBBox = getRouteBox();
  auto net = via->getNet();
  nets.insert(net);
  frPoint viaPoint;
  via->getOrigin(viaPoint);
  bool condition1 = isInitDR() ? (viaPoint.x() < gridBBox.right()
                                  && viaPoint.y() < gridBBox.top())
                               : (viaPoint.x() <= gridBBox.right()
                                  && viaPoint.y() <= gridBBox.top());
  if (viaPoint.x() >= gridBBox.left() && viaPoint.y() >= gridBBox.bottom()
      && condition1) {
    auto uVia = make_unique<drVia>(*via);
    unique_ptr<drConnFig> uDRObj(std::move(uVia));
    netRouteObjs[net].push_back(std::move(uDRObj));
  } else {
    auto uVia = make_unique<drVia>(*via);
    unique_ptr<drConnFig> uDRObj(std::move(uVia));
    netExtObjs[net].push_back(std::move(uDRObj));
  }
}

void FlexDRWorker::initNetObjs_patchWire(
    frPatchWire* pwire,
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs)
{
  auto gridBBox = getRouteBox();
  auto net = pwire->getNet();
  nets.insert(net);
  frPoint origin;
  pwire->getOrigin(origin);
  bool condition1
      = isInitDR()
            ? (origin.x() < gridBBox.right() && origin.y() < gridBBox.top())
            : (origin.x() <= gridBBox.right() && origin.y() <= gridBBox.top());
  if (origin.x() >= gridBBox.left() && origin.y() >= gridBBox.bottom()
      && condition1) {
    auto uPWire = make_unique<drPatchWire>(*pwire);
    unique_ptr<drConnFig> uDRObj(std::move(uPWire));
    netRouteObjs[net].push_back(std::move(uDRObj));
  } else {
    auto uPWire = make_unique<drPatchWire>(*pwire);
    unique_ptr<drConnFig> uDRObj(std::move(uPWire));
    netExtObjs[net].push_back(std::move(uDRObj));
  }
}
// inits nets based on the routing shapes in the extRouteBox or based on the
// guides, if initDR(). inits netRouteObjs with routing shapes touching routeBox
// (shapes that touch only right/top borders of routeBox are not considered, IF
// initDR()). inits netExtObjs with routing shapes not touching routeBox, using
// the same criterion above
void FlexDRWorker::initNetObjs(
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs,
    map<frNet*, vector<frRect>, frBlockObjectComp>& netOrigGuides)
{
  vector<frBlockObject*> result;
  getRegionQuery()->queryDRObj(getExtBox(), result);
  int cnt1 = 0;
  int cnt2 = 0;
  for (auto rptr : result) {
    if (rptr->typeId() == frcPathSeg) {
      auto cptr = static_cast<frPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_pathSeg(cptr, nets, netRouteObjs, netExtObjs);
        cnt1++;
      } else {
        cout << "Error: initNetObjs hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcVia) {
      auto cptr = static_cast<frVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_via(cptr, nets, netRouteObjs, netExtObjs);
        cnt2++;
      } else {
        cout << "Error: initNetObjs hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcPatchWire) {
      auto cptr = static_cast<frPatchWire*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_patchWire(cptr, nets, netRouteObjs, netExtObjs);
        cnt1++;
      } else {
        cout << "Error: initNetObjs hasNet() empty" << endl;
      }
    } else {
      cout << rptr->typeId() << "\n";
      cout << "Error: initCopyDRObjs unsupported type" << endl;
    }
  }
  if (isInitDR()) {
    vector<frGuide*> guides;
    getRegionQuery()->queryGuide(getRouteBox(), guides);
    for (auto& guide : guides) {
      if (guide->hasNet()) {
        auto net = guide->getNet();
        if (nets.find(net) == nets.end()) {
          nets.insert(net);
          netRouteObjs[net].clear();
          netExtObjs[net].clear();
        }
      }
    }
  }

  if (isFollowGuide()) {
    frRegionQuery::Objects<frNet> origGuides;
    frRect rect;
    for (auto lNum = getDesign()->getTech()->getBottomLayerNum();
         lNum <= getDesign()->getTech()->getTopLayerNum();
         lNum++) {
      origGuides.clear();
      getRegionQuery()->queryOrigGuide(getRouteBox(), lNum, origGuides);
      for (auto& [box, net] : origGuides) {
        if (nets.find(net) == nets.end()) {
          continue;
        }
        // if (getExtBox().overlaps(box, false)) {
        //   continue;
        // }
        rect.setBBox(box);
        rect.setLayerNum(lNum);
        netOrigGuides[net].push_back(rect);
      }
    }
  }

}
// inits nets based on the pins
void FlexDRWorker::initNets_initDR(
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs,
    map<frNet*, vector<frRect>, frBlockObjectComp>& netOrigGuides)
{
  map<frNet*, set<frBlockObject*, frBlockObjectComp>, frBlockObjectComp>
      netTerms;
  vector<frBlockObject*> result;
  getRegionQuery()->queryGRPin(getRouteBox(), result);
  for (auto obj : result) {
    if (obj->typeId() == frcInstTerm) {
      auto net = static_cast<frInstTerm*>(obj)->getNet();
      nets.insert(net);
      netTerms[net].insert(obj);
    } else if (obj->typeId() == frcTerm) {
      auto net = static_cast<frTerm*>(obj)->getNet();
      nets.insert(net);
      netTerms[net].insert(obj);
    } else {
      cout << "Error: initNetTerms unsupported obj" << endl;
    }
  }
  vector<unique_ptr<drConnFig>> vRouteObjs;
  vector<unique_ptr<drConnFig>> vExtObjs;
  for (auto net : nets) {
    vRouteObjs.clear();
    vExtObjs.clear();
    vExtObjs = std::move(netExtObjs[net]);
    for (int i = 0; i < (int) netRouteObjs[net].size(); i++) {
      auto& obj = netRouteObjs[net][i];
      if (obj->typeId() == drcPathSeg) {
        auto ps = static_cast<drPathSeg*>(obj.get());
        frPoint bp, ep;
        ps->getPoints(bp, ep);
        auto& box = getRouteBox();
        if (box.contains(bp) && box.contains(ep)) {  // how can this be false?
          vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
        } else {
          vExtObjs.push_back(std::move(netRouteObjs[net][i]));
        }
      } else if (obj->typeId() == drcVia) {
        vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
      } else if (obj->typeId() == drcPatchWire) {
        vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
      }
    }
    vector<frBlockObject*> tmpTerms;
    tmpTerms.assign(netTerms[net].begin(), netTerms[net].end());
    initNet(net, vRouteObjs, vExtObjs, netOrigGuides[net], tmpTerms);
  }
}

// copied to FlexDR::checkConnectivity_pin2epMap_helper
void FlexDRWorker::initNets_searchRepair_pin2epMap_helper(
    frNet* net,
    const frPoint& bp,
    frLayerNum lNum,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap)
{
  auto regionQuery = getRegionQuery();
  frRegionQuery::Objects<frBlockObject> result;
  // In PA we may have used NearbyGrid which puts a via outside the pin
  // but still overlapping.  So we expand bp to a min-width square when
  // searching for the pin shape.
  auto half_min_width = getTech()->getLayer(lNum)->getMinWidth() / 2;
  frBox query_box(bp.x() - half_min_width,
                  bp.y() - half_min_width,
                  bp.x() + half_min_width,
                  bp.y() + half_min_width);
  regionQuery->query(query_box, lNum, result);
  for (auto& [bx, rqObj] : result) {
    if (rqObj->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(rqObj);
      if (instTerm->getNet() == net) {
        pin2epMap[rqObj].insert(make_pair(bp, lNum));
      } else {
      }
    } else if (rqObj->typeId() == frcTerm) {
      auto term = static_cast<frTerm*>(rqObj);
      if (term->getNet() == net) {
        pin2epMap[rqObj].insert(make_pair(bp, lNum));
      } else {
      }
    }
  }
}
// maps the currently used access points to their pins
void FlexDRWorker::initNets_searchRepair_pin2epMap(
    frNet* net,
    vector<unique_ptr<drConnFig>>& netRouteObjs,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap)
{
  frPoint bp, ep;
  // should not count extObjs in union find
  for (auto& uPtr : netRouteObjs) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      frSegStyle style;
      obj->getStyle(style);
      if (style.getBeginStyle() == frEndStyle(frcTruncateEndStyle)
          && getRouteBox().contains(bp)) {
        initNets_searchRepair_pin2epMap_helper(net, bp, lNum, pin2epMap);
      }
      if (style.getEndStyle() == frEndStyle(frcTruncateEndStyle)
          && getRouteBox().contains(ep)) {
        initNets_searchRepair_pin2epMap_helper(net, ep, lNum, pin2epMap);
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      obj->getOrigin(bp);
      auto l1Num = obj->getViaDef()->getLayer1Num();
      auto l2Num = obj->getViaDef()->getLayer2Num();
      if (getRouteBox().contains(bp)) {
        initNets_searchRepair_pin2epMap_helper(net, bp, l1Num, pin2epMap);
        initNets_searchRepair_pin2epMap_helper(net, bp, l2Num, pin2epMap);
      }
    } else if (connFig->typeId() == drcPatchWire) {
    } else {
      cout << "Error: initNets_searchRepair_pin2epMap unsupported type" << endl;
    }
  }
  // cout <<net->getName() <<" " <<pin2epMap.size() <<endl;
}
// maps begin/end points of shapes to their shapes
void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjEnd(
    frNet* net,
    vector<unique_ptr<drConnFig>>& netRouteObjs,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  frPoint bp, ep;
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto connFig = netRouteObjs[i].get();
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      nodeMap[make_pair(bp, lNum)].insert(i);
      nodeMap[make_pair(ep, lNum)].insert(i);
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      obj->getOrigin(bp);
      auto l1Num = obj->getViaDef()->getLayer1Num();
      auto l2Num = obj->getViaDef()->getLayer2Num();
      nodeMap[make_pair(bp, l1Num)].insert(i);
      nodeMap[make_pair(bp, l2Num)].insert(i);
    } else if (connFig->typeId() == drcPatchWire) {
      auto obj = static_cast<drPatchWire*>(connFig);
      obj->getOrigin(bp);
      auto lNum = obj->getLayerNum();
      nodeMap[make_pair(bp, lNum)].insert(i);
    } else {
      cout << "Error: initNets_searchRepair_nodeMap unsupported type" << endl;
    }
  }
}

void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjSplit_helper(
    const frPoint& crossPt,
    frCoord trackCoord,
    frCoord splitCoord,
    frLayerNum lNum,
    vector<map<frCoord, map<frCoord, pair<frCoord, int>>>>& mergeHelper,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  auto it1 = mergeHelper[lNum].find(trackCoord);
  if (it1 != mergeHelper[lNum].end()) {
    auto& mp = it1->second;  // map<ep, pair<bp, objIdx> >
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
// creates mapping of middle points of shapes, that are intersecting other
// shapes, to their shapes
void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjSplit(
    frNet* net,
    vector<unique_ptr<drConnFig>>& netRouteObjs,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  frPoint bp, ep;
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> horzMergeHelper(
      getTech()->getLayers().size());
  vector<map<frCoord, map<frCoord, pair<frCoord, int>>>> vertMergeHelper(
      getTech()->getLayers().size());
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto connFig = netRouteObjs[i].get();
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
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
    auto connFig = netRouteObjs[i].get();
    // ep on pathseg
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      // vert seg, find horz crossing seg
      if (bp.x() == ep.x()) {
        // find whether there is horz track at bp
        auto crossPt = bp;
        auto trackCoord = bp.y();
        auto splitCoord = bp.x();
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
        // find whether there is horz track at ep
        crossPt = ep;
        trackCoord = ep.y();
        splitCoord = ep.x();
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
        // horz seg
      } else {
        // find whether there is vert track at bp
        auto crossPt = bp;
        auto trackCoord = bp.x();
        auto splitCoord = bp.y();
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
        // find whether there is vert track at ep
        crossPt = ep;
        trackCoord = ep.x();
        splitCoord = ep.y();
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      obj->getOrigin(bp);
      auto lNum = obj->getViaDef()->getLayer1Num();
      // find whether there is horz track at bp on layer1
      auto crossPt = bp;
      auto trackCoord = bp.y();
      auto splitCoord = bp.x();
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer1
      crossPt = bp;
      trackCoord = bp.x();
      splitCoord = bp.y();
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);

      lNum = obj->getViaDef()->getLayer2Num();
      // find whether there is horz track at bp on layer2
      crossPt = bp;
      trackCoord = bp.y();
      splitCoord = bp.x();
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer2
      crossPt = bp;
      trackCoord = bp.x();
      splitCoord = bp.y();
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          crossPt, trackCoord, splitCoord, lNum, vertMergeHelper, nodeMap);
    }
  }
}
// creates entries in the node_map with pin access points and their pins
void FlexDRWorker::initNets_searchRepair_nodeMap_pin(
    frNet* net,
    vector<unique_ptr<drConnFig>>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
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
// maps points (in the wire tips, via centers, wire intersections) to their
// shapes (routeObjs)
void FlexDRWorker::initNets_searchRepair_nodeMap(
    frNet* net,
    vector<unique_ptr<drConnFig>>& netRouteObjs,
    vector<frBlockObject*>& netPins,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2epMap,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap)
{
  initNets_searchRepair_nodeMap_routeObjEnd(net, netRouteObjs, nodeMap);
  initNets_searchRepair_nodeMap_routeObjSplit(net, netRouteObjs, nodeMap);
  initNets_searchRepair_nodeMap_pin(
      net, netRouteObjs, netPins, pin2epMap, nodeMap);
}
// maps routeObjs to sub-nets
void FlexDRWorker::initNets_searchRepair_connComp(
    frNet* net,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap,
    vector<int>& compIdx)
{
  int nCnt = (int) compIdx.size();  // total node cnt

  vector<vector<int>> adjVec(nCnt, vector<int>());
  vector<bool> adjVisited(nCnt, false);
  for (auto& [pr, idxS] : nodeMap) {
    for (auto it1 = idxS.begin(); it1 != idxS.end(); it1++) {
      auto it2 = it1;
      it2++;
      auto idx1 = *it1;
      for (; it2 != idxS.end(); it2++) {
        auto idx2 = *it2;
        adjVec[idx1].push_back(idx2);
        adjVec[idx2].push_back(idx1);
      }
    }
  }

  struct wf
  {
    int nodeIdx;
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

  int currNetIdx = 0;
  auto it = find(adjVisited.begin(), adjVisited.end(), false);
  while (it != adjVisited.end()) {
    priority_queue<wf> pq;
    int srcIdx = distance(adjVisited.begin(), it);
    pq.push({srcIdx, 0});
    while (!pq.empty()) {
      auto wfront = pq.top();
      auto currIdx = wfront.nodeIdx;
      pq.pop();
      if (adjVisited[currIdx]) {
        continue;
      }
      adjVisited[currIdx] = true;
      compIdx[currIdx] = currNetIdx;
      for (auto nbrIdx : adjVec[currIdx]) {
        if (!adjVisited[nbrIdx]) {
          pq.push({nbrIdx, wfront.cost + 1});
        }
      }
    }
    it = find(adjVisited.begin(), adjVisited.end(), false);
    ++currNetIdx;
  }
  // if (currNetIdx > 1) {
  //   cout <<"@@debug " <<net->getName() <<" union find get " <<currNetIdx <<"
  //   subnets" <<endl;
  // }
}

void FlexDRWorker::initNets_searchRepair(
    set<frNet*, frBlockObjectComp>& nets,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netRouteObjs,
    map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp>& netExtObjs,
    map<frNet*, vector<frRect>, frBlockObjectComp>& netOrigGuides)
{
  for (auto net : nets) {
    // build big graph;
    // node number : routeObj, pins
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>
        pin2epMap;
    initNets_searchRepair_pin2epMap(
        net,
        netRouteObjs[net] /*, netExtObjs[net], netPins*/,
        pin2epMap /*, nodeMap*/);

    vector<frBlockObject*> netPins;
    map<pair<frPoint, frLayerNum>, set<int>> nodeMap;
    initNets_searchRepair_nodeMap(
        net, netRouteObjs[net], netPins, pin2epMap, nodeMap);

    vector<int> compIdx((int) netPins.size() + (int) netRouteObjs[net].size(),
                        0);

    initNets_searchRepair_connComp(net, nodeMap, compIdx);

    vector<vector<unique_ptr<drConnFig>>> vRouteObjs;
    vector<vector<unique_ptr<drConnFig>>> vExtObjs;
    vector<vector<frBlockObject*>> vPins;

    auto it = max_element(compIdx.begin(), compIdx.end());
    int numSubNets = (it == compIdx.end()) ? 1 : ((*it) + 1);
    // put all pure ext objs to the first subnet
    vExtObjs.resize(numSubNets);
    vExtObjs[0] = std::move(netExtObjs[net]);

    vRouteObjs.resize(numSubNets);
    vPins.resize(numSubNets);
    // does a logical assignment of routing shapes to sub-nets
    for (int i = 0; i < (int) compIdx.size(); i++) {
      int subNetIdx = compIdx[i];
      if (i < (int) netRouteObjs[net].size()) {
        auto& obj = netRouteObjs[net][i];
        if (obj->typeId() == drcPathSeg) {
          auto ps = static_cast<drPathSeg*>(obj.get());
          frPoint bp, ep;
          ps->getPoints(bp, ep);
          auto& box = getRouteBox();
          if (box.contains(bp) && box.contains(ep)) {
            vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
          } else {
            vExtObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
          }
        } else if (obj->typeId() == drcVia) {
          vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
        } else if (obj->typeId() == drcPatchWire) {
          vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
        }
      } else {
        vPins[subNetIdx].push_back(netPins[i - (int) netRouteObjs[net].size()]);
      }
    }

    for (int i = 0; i < numSubNets; i++) {
      initNet(net, vRouteObjs[i], vExtObjs[i], netOrigGuides[net], vPins[i]);
    }
  }
}

void FlexDRWorker::initNet_termGenAp_new(drPin* dPin)
{
  using namespace boost::polygon::operators;

  auto routeBox = getRouteBox();
  Rectangle routeRect(
      routeBox.left(), routeBox.bottom(), routeBox.right(), routeBox.top());
  using Point = boost::polygon::point_data<int>;
  Point routeRectCenter;
  center(routeRectCenter, routeRect);
  vector<pair<frBox, frLayerNum>> outOfRouteBoxRects;
  frCoord halfWidth;
  auto dPinTerm = dPin->getFrTerm();
  if (dPinTerm->typeId() == frcInstTerm) {
    auto instTerm = static_cast<frInstTerm*>(dPinTerm);
    auto inst = instTerm->getInst();
    frTransform xform;
    inst->getUpdatedXform(xform);

    for (auto& uPin : instTerm->getTerm()->getPins()) {
      auto pin = uPin.get();
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          frLayerNum currLayerNum = rpinRect->getLayerNum();
          if (getTech()->getLayer(currLayerNum)->getType()
              != frLayerTypeEnum::ROUTING) {
            continue;
          }
          halfWidth
              = design_->getTech()->getLayer(currLayerNum)->getMinWidth() / 2;
          frRect instPinRect(*rpinRect);
          instPinRect.move(xform);
          frBox instPinRectBBox;
          instPinRect.getBBox(instPinRectBBox);
          Rectangle pinRect(instPinRectBBox.left(),
                            instPinRectBBox.bottom(),
                            instPinRectBBox.right(),
                            instPinRectBBox.top());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            if (instPinRectBBox.distMaxXY(routeBox) <= halfWidth)
              outOfRouteBoxRects.push_back(
                  make_pair(instPinRectBBox, currLayerNum));
            continue;
          }
          // pinRect now equals intersection of pinRect and routeRect
          auto currPrefRouteDir = getTech()->getLayer(currLayerNum)->getDir();
          // get intersecting tracks if any
          if (currPrefRouteDir == frcHorzPrefRoutingDir) {
            getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            if (currLayerNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
            } else if (currLayerNum - 2
                       >= getDesign()->getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
            } else {
              getTrackLocs(
                  false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            }
          } else {
            getTrackLocs(false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            if (currLayerNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
            } else if (currLayerNum - 2
                       >= getDesign()->getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
            } else {
              getTrackLocs(
                  false, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            }
          }
          // gen new temp on-track access point if any
          if (!xLocs.empty() && !yLocs.empty()) {
            // TODO: update access pattern information as needed
            auto uap = std::make_unique<drAccessPattern>();
            auto xLoc = *(xLocs.begin());
            auto yLoc = *(yLocs.begin());
            frPoint pt(xLoc, yLoc);
            uap->setBeginLayerNum(currLayerNum);
            uap->setPoint(pt);
            // prevent unchecked planar access
            vector<bool> validAccess(6, true);
            validAccess[5] = false;
            if (currLayerNum == VIA_ACCESS_LAYERNUM) {
              validAccess[0] = false;
              validAccess[1] = false;
              validAccess[2] = false;
              validAccess[3] = false;
            }
            uap->setValidAccess(validAccess);
            uap->setOnTrack(false, true);
            uap->setOnTrack(false, false);
            uap->setPin(dPin);
            // any non-zero value works
            uap->setPinCost(7);
            // to resolve temp AP end seg minArea patch problem
            // frCoord reqArea = 0;
            auto minAreaConstraint = getDesign()
                                         ->getTech()
                                         ->getLayer(currLayerNum)
                                         ->getAreaConstraint();
            if (minAreaConstraint) {
              auto reqArea = minAreaConstraint->getMinArea();
              uap->setBeginArea(reqArea);
            }

            if (!isInitDR()
                || (xLoc != xh(routeRect) && yLoc != yh(routeRect))) {
              dPin->addAccessPattern(std::move(uap));
              break;
            }
          }
        } else {
          cout << "Error: initNet_termGenAp_new unsupported pinFig\n";
        }
      }

      // no on-track temp ap found
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          frLayerNum currLayerNum = rpinRect->getLayerNum();
          if (getTech()->getLayer(currLayerNum)->getType()
              != frLayerTypeEnum::ROUTING) {
            continue;
          }
          frRect instPinRect(*rpinRect);
          instPinRect.move(xform);
          frBox instPinRectBBox;
          instPinRect.getBBox(instPinRectBBox);
          Rectangle pinRect(instPinRectBBox.left(),
                            instPinRectBBox.bottom(),
                            instPinRectBBox.right(),
                            instPinRectBBox.top());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            continue;
          }
          // pinRect now equals intersection of pinRect and routeRect
          auto currPrefRouteDir = getTech()->getLayer(currLayerNum)->getDir();
          // get intersecting tracks if any
          if (currPrefRouteDir == frcHorzPrefRoutingDir) {
            getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            if (currLayerNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
            } else if (currLayerNum - 2
                       >= getDesign()->getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
            } else {
              getTrackLocs(
                  false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            }
          } else {
            getTrackLocs(false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            if (currLayerNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
            } else if (currLayerNum - 2
                       >= getDesign()->getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
            } else {
              getTrackLocs(
                  false, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            }
          }
          // gen new temp on-track access point if any
          frCoord xLoc, yLoc;
          // xLoc
          if (!xLocs.empty() && !isInitDR()) {
            xLoc = *(xLocs.begin());
          } else {
            xLoc = (xl(pinRect) + xh(pinRect)) / 2;
          }
          // xLoc
          if (!yLocs.empty() && !isInitDR()) {
            yLoc = *(yLocs.begin());
          } else {
            yLoc = (yl(pinRect) + yh(pinRect)) / 2;
          }
          if (!isInitDR() || (xLoc != xh(routeRect) || yLoc != yh(routeRect))) {
            // TODO: update as drAccessPattern updated
            auto uap = std::make_unique<drAccessPattern>();
            frPoint pt(xLoc, yLoc);
            uap->setBeginLayerNum(currLayerNum);
            uap->setPoint(pt);
            // prevent unchecked planar access
            vector<bool> validAccess(6, true);
            validAccess[5] = false;
            if (currLayerNum == VIA_ACCESS_LAYERNUM) {
              validAccess[0] = false;
              validAccess[1] = false;
              validAccess[2] = false;
              validAccess[3] = false;
            }
            uap->setValidAccess(validAccess);
            uap->setOnTrack(false, true);
            uap->setOnTrack(false, false);
            uap->setPin(dPin);
            // any non-zero value works
            uap->setPinCost(7);
            // to resolve temp AP end seg minArea patch problem
            // frCoord reqArea = 0;
            auto minAreaConstraint = getDesign()
                                         ->getTech()
                                         ->getLayer(currLayerNum)
                                         ->getAreaConstraint();
            if (minAreaConstraint) {
              auto reqArea = minAreaConstraint->getMinArea();
              uap->setBeginArea(reqArea);
            }

            dPin->addAccessPattern(std::move(uap));
            break;
          }
        } else {
          cout << "Error: initNet_termGenAp_new unsupported pinFig\n";
        }
      }
    }
  } else if (dPinTerm->typeId() == frcTerm) {
    auto term = static_cast<frTerm*>(dPinTerm);
    for (auto& uPin : term->getPins()) {
      auto pin = uPin.get();
      bool hasTempAp = false;
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          frLayerNum currLayerNum = rpinRect->getLayerNum();
          if (getTech()->getLayer(currLayerNum)->getType()
              != frLayerTypeEnum::ROUTING) {
            continue;
          }
          halfWidth
              = design_->getTech()->getLayer(currLayerNum)->getMinWidth() / 2;
          frRect instPinRect(*rpinRect);
          // instPinRect.move(xform);
          frBox instPinRectBBox;
          instPinRect.getBBox(instPinRectBBox);
          Rectangle pinRect(instPinRectBBox.left(),
                            instPinRectBBox.bottom(),
                            instPinRectBBox.right(),
                            instPinRectBBox.top());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            if (instPinRectBBox.distMaxXY(routeBox) <= halfWidth)
              outOfRouteBoxRects.push_back(
                  make_pair(instPinRectBBox, currLayerNum));
            continue;
          }
          // pinRect now equals intersection of pinRect and routeRect
          auto currPrefRouteDir = getTech()->getLayer(currLayerNum)->getDir();
          bool useCenterLine = true;
          auto xSpan = instPinRectBBox.right() - instPinRectBBox.left();
          auto ySpan = instPinRectBBox.top() - instPinRectBBox.bottom();
          bool isPinRectHorz = (xSpan > ySpan);

          if (!useCenterLine) {
            // get intersecting tracks if any
            if (currPrefRouteDir == frcHorzPrefRoutingDir) {
              getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
              if (currLayerNum + 2
                  <= getDesign()->getTech()->getTopLayerNum()) {
                getTrackLocs(
                    false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
              } else if (currLayerNum - 2
                         >= getDesign()->getTech()->getBottomLayerNum()) {
                getTrackLocs(
                    false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
              } else {
                getTrackLocs(
                    false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
              }
            } else {
              getTrackLocs(
                  false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
              if (currLayerNum + 2
                  <= getDesign()->getTech()->getTopLayerNum()) {
                getTrackLocs(
                    false, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
              } else if (currLayerNum - 2
                         >= getDesign()->getTech()->getBottomLayerNum()) {
                getTrackLocs(
                    false, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
              } else {
                getTrackLocs(
                    false, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
              }
            }
          } else {
            int layerWidth = getTech()
                                 ->getLayer(currLayerNum)
                                 ->getWidth();  // for ISPD off track pins
            if (isPinRectHorz) {
              bool didUseCenterline = false;
              frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();
              auto centerY = (instPinRectBBox.top() + instPinRectBBox.bottom())
                             / 2 / manuGrid * manuGrid;
              if (centerY >= yl(routeRect) && centerY < yh(routeRect)) {
                yLocs.insert(centerY);
                didUseCenterline = true;
              }
              if (!didUseCenterline) {
                getTrackLocs(
                    true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
              }
              if (didUseCenterline) {
                if (currLayerNum + 2
                    <= getDesign()->getTech()->getTopLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
                } else if (currLayerNum - 2
                           >= getDesign()->getTech()->getBottomLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
                } else {
                  getTrackLocs(
                      false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
                }
              } else {
                frCoord lowerBoundX = xl(pinRect);
                frCoord upperBoundX = xh(pinRect);
                if ((upperBoundX - lowerBoundX) >= 2 * layerWidth) {
                  lowerBoundX += layerWidth;
                  upperBoundX -= layerWidth;
                }
                if (currLayerNum + 2
                    <= getDesign()->getTech()->getTopLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum + 2, lowerBoundX, upperBoundX, xLocs);
                } else if (currLayerNum - 2
                           >= getDesign()->getTech()->getBottomLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum - 2, lowerBoundX, upperBoundX, xLocs);
                } else {
                  getTrackLocs(
                      false, currLayerNum, lowerBoundX, upperBoundX, xLocs);
                }
              }

            } else {
              bool didUseCenterline = false;
              frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();
              auto centerX = (instPinRectBBox.left() + instPinRectBBox.right())
                             / 2 / manuGrid * manuGrid;
              if (centerX >= xl(routeRect) && centerX < xh(routeRect)) {
                xLocs.insert(centerX);
                didUseCenterline = true;
              }
              if (!didUseCenterline) {
                getTrackLocs(
                    false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
              }
              if (didUseCenterline) {
                if (currLayerNum + 2
                    <= getDesign()->getTech()->getTopLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
                } else if (currLayerNum - 2
                           >= getDesign()->getTech()->getBottomLayerNum()) {
                  getTrackLocs(
                      false, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
                } else {
                  getTrackLocs(
                      false, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
                }
              } else {
                frCoord lowerBoundY = yl(pinRect);
                frCoord upperBoundY = yh(pinRect);
                if ((upperBoundY - lowerBoundY) >= 2 * layerWidth) {
                  lowerBoundY += layerWidth;
                  upperBoundY -= layerWidth;
                  if (currLayerNum + 2
                      <= getDesign()->getTech()->getTopLayerNum()) {
                    getTrackLocs(false,
                                 currLayerNum + 2,
                                 lowerBoundY,
                                 upperBoundY,
                                 yLocs);
                  } else if (currLayerNum - 2
                             >= getDesign()->getTech()->getBottomLayerNum()) {
                    getTrackLocs(false,
                                 currLayerNum - 2,
                                 lowerBoundY,
                                 upperBoundY,
                                 yLocs);
                  } else {
                    getTrackLocs(
                        false, currLayerNum, lowerBoundY, upperBoundY, yLocs);
                  }
                }
              }
            }
          }

          // gen new temp on-track access point if any
          if (!xLocs.empty() && !yLocs.empty()) {
            // TODO: update access pattern information as needed
            for (auto xLoc : xLocs) {
              for (auto yLoc : yLocs) {
                auto uap = std::make_unique<drAccessPattern>();
                // auto xLoc =*(xLocs.begin());
                // auto yLoc =*(yLocs.begin());
                frPoint pt(xLoc, yLoc);
                uap->setBeginLayerNum(currLayerNum);
                uap->setPoint(pt);
                // to resolve temp AP end seg minArea patch problem
                // frCoord reqArea = 0;
                auto minAreaConstraint = getDesign()
                                             ->getTech()
                                             ->getLayer(currLayerNum)
                                             ->getAreaConstraint();
                if (minAreaConstraint) {
                  auto reqArea = minAreaConstraint->getMinArea();
                  uap->setBeginArea(reqArea);
                }
                // io pin pref direction setting
                // Point pinCenter;
                // center(pinCenter, pinRect);
                vector<bool> validAccess(6, false);
                if (isPinRectHorz) {
                  validAccess[0] = true;
                  validAccess[2] = true;
                } else {
                  validAccess[3] = true;
                  validAccess[1] = true;
                }
                uap->setValidAccess(validAccess);
                uap->setPin(dPin);
                if (!isInitDR() || xLoc != xh(routeRect)
                    || yLoc != yh(routeRect)) {
                  if (xLoc >= xl(routeRect) && xLoc < xh(routeRect)
                      && yLoc >= yl(routeRect) && yLoc < yh(routeRect)) {
                    hasTempAp = true;
                    dPin->addAccessPattern(std::move(uap));
                    // if (term->getName() == string("pin128")) {
                    //   cout << "@@@ debug @@@: pin128 temp AP loc (" << xLoc /
                    //   2000.0 << ", " << yLoc / 2000.0 << ")\n";
                    // }
                    // break;
                  }
                }
              }
            }

            if (hasTempAp) {
              break;
            }
          }
        } else {
          cout << "Error: initNet_termGenAp_new unsupported pinFig\n";
        }
      }

      // no on-track temp ap found
      if (!hasTempAp) {
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          // horizontal tracks == yLocs
          std::set<frCoord> xLocs, yLocs;
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            frLayerNum currLayerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(currLayerNum)->getType()
                != frLayerTypeEnum::ROUTING) {
              continue;
            }
            frRect instPinRect(*rpinRect);
            // instPinRect.move(xform);
            frBox instPinRectBBox;
            instPinRect.getBBox(instPinRectBBox);
            Rectangle pinRect(instPinRectBBox.left(),
                              instPinRectBBox.bottom(),
                              instPinRectBBox.right(),
                              instPinRectBBox.top());
            if (!boost::polygon::intersect(pinRect, routeRect)) {
              continue;
            }

            frCoord xLoc, yLoc;
            auto instPinCenterX
                = (instPinRectBBox.left() + instPinRectBBox.right()) / 2;
            auto instPinCenterY
                = (instPinRectBBox.bottom() + instPinRectBBox.top()) / 2;
            auto pinCenterX = (xl(pinRect) + xh(pinRect)) / 2;
            auto pinCenterY = (yl(pinRect) + yh(pinRect)) / 2;
            if (instPinCenterX >= xl(routeRect)
                && instPinCenterX < xh(routeRect)) {
              xLoc = instPinCenterX;
            } else {
              xLoc = pinCenterX;
            }
            if (instPinCenterY >= yl(routeRect)
                && instPinCenterY < yh(routeRect)) {
              yLoc = instPinCenterY;
            } else {
              yLoc = pinCenterY;
            }

            if (!isInitDR() || xLoc != xh(routeRect) || yLoc != yh(routeRect)) {
              // TODO: update as drAccessPattern updated
              auto uap = std::make_unique<drAccessPattern>();
              frPoint pt(xLoc, yLoc);
              uap->setBeginLayerNum(currLayerNum);
              uap->setPoint(pt);
              uap->setPin(dPin);
              // to resolve temp AP end seg minArea patch problem
              // frCoord reqArea = 0;
              auto minAreaConstraint = getDesign()
                                           ->getTech()
                                           ->getLayer(currLayerNum)
                                           ->getAreaConstraint();
              if (minAreaConstraint) {
                auto reqArea = minAreaConstraint->getMinArea();
                uap->setBeginArea(reqArea);
              }

              dPin->addAccessPattern(std::move(uap));
              break;
            }
          } else {
            cout << "Error: initNet_termGenAp_new unsupported pinFig\n";
          }
        }
      }
    }
  } else {
    cout << "Error: initNet_termGenAp_new unexpected type\n";
  }
  if (dPin->getAccessPatterns().empty()) {
    frCoord x, y, halfWidth;
    for (auto& r : outOfRouteBoxRects) {
      halfWidth = design_->getTech()->getLayer(r.second)->getMinWidth() / 2;
      if (r.first.left() > routeBox.right()) {
        x = r.first.left() - halfWidth;
      } else if (r.first.right() < routeBox.left()) {
        x = r.first.right() + halfWidth;
      } else
        x = max(routeBox.left(), r.first.left());
      if (r.first.bottom() > routeBox.top()) {
        y = r.first.bottom() - halfWidth;
      } else if (r.first.top() < routeBox.bottom()) {
        y = r.first.top() + halfWidth;
      } else
        y = max(routeBox.bottom(), r.first.bottom());
      auto uap = std::make_unique<drAccessPattern>();
      frPoint pt(x, y);
      uap->setBeginLayerNum(r.second);
      uap->setPoint(pt);
      uap->setOnTrack(false, true);
      uap->setOnTrack(false, false);
      uap->setPin(dPin);
      uap->setPinCost(7);
      auto minAreaConstraint
          = getDesign()->getTech()->getLayer(r.second)->getAreaConstraint();
      if (minAreaConstraint) {
        uap->setBeginArea(minAreaConstraint->getMinArea());
      }
      dPin->addAccessPattern(std::move(uap));
    }
  }
}

// when isHorzTracks == true, it means track loc == y loc
void FlexDRWorker::getTrackLocs(bool isHorzTracks,
                                frLayerNum currLayerNum,
                                frCoord low,
                                frCoord high,
                                std::set<frCoord>& trackLocs)
{
  frPrefRoutingDirEnum currPrefRouteDir
      = getTech()->getLayer(currLayerNum)->getDir();
  for (auto& tp : design_->getTopBlock()->getTrackPatterns(currLayerNum)) {
    if ((tp->isHorizontal() && currPrefRouteDir == frcVertPrefRoutingDir)
        || (!tp->isHorizontal() && currPrefRouteDir == frcHorzPrefRoutingDir)) {
      int trackNum = (low - tp->getStartCoord()) / (int) tp->getTrackSpacing();
      if (trackNum < 0) {
        trackNum = 0;
      }
      if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord() < low) {
        ++trackNum;
      }
      for (; trackNum < (int) tp->getNumTracks()
             && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                    <= high;
           ++trackNum) {
        frCoord trackLoc
            = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
        trackLocs.insert(trackLoc);
        if (tp->isHorizontal() && !isHorzTracks) {
          trackLocs.insert(trackLoc);
        } else if (!tp->isHorizontal() && isHorzTracks) {
          trackLocs.insert(trackLoc);
        } else {
          continue;
        }
      }
    }
  }
}

void FlexDRWorker::initNet_term_new(drNet* dNet, vector<frBlockObject*>& terms)
{
  for (auto term : terms) {
    auto dPin = make_unique<drPin>();
    dPin->setFrTerm(term);
    // ap
    frTransform instXform;  // (0,0), frcR0
    frTransform shiftXform;
    frTerm* trueTerm = nullptr;
    string name;
    // bool hasInst = false;
    frInst* inst = nullptr;
    // vector<frAccessPoint*> *instTermPrefAps = nullptr;
    if (term->typeId() == frcInstTerm) {
      // hasInst = true;
      inst = static_cast<frInstTerm*>(term)->getInst();
      // inst->getTransform(instXform);
      inst->getTransform(shiftXform);
      shiftXform.set(frOrient(frcR0));
      inst->getUpdatedXform(instXform);
      // inst->getUpdatedXform(shiftXform, true); // get no orient version
      trueTerm = static_cast<frInstTerm*>(term)->getTerm();
      name = inst->getName() + string("/") + trueTerm->getName();
      // instTermPrefAps = static_cast<frInstTerm*>(term)->getAccessPoints();
    } else if (term->typeId() == frcTerm) {
      trueTerm = static_cast<frTerm*>(term);
      name = string("PIN/") + trueTerm->getName();
    }
    int pinIdx = 0;
    int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : -1;
    for (auto& pin : trueTerm->getPins()) {
      frAccessPoint* prefAp = nullptr;
      if (inst) {
        prefAp = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
      }
      if (!pin->hasPinAccess()) {
        continue;
      }
      if (pinAccessIdx == -1) {
        continue;
      }
      for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
        frPoint bp;
        ap->getPoint(bp);
        auto bNum = ap->getLayerNum();
        auto bLayer = getDesign()->getTech()->getLayer(bNum);
        bp.transform(shiftXform);

        auto dAp = make_unique<drAccessPattern>();
        dAp->setPoint(bp);
        dAp->setBeginLayerNum(bNum);
        if (ap.get() == prefAp) {
          dAp->setPinCost(0);
        } else {
          dAp->setPinCost(1);
        }
        // set min area
        if (ENABLE_BOUNDARY_MAR_FIX) {
          auto minAreaConstraint
              = getDesign()->getTech()->getLayer(bNum)->getAreaConstraint();
          if (minAreaConstraint) {
            auto reqArea = minAreaConstraint->getMinArea();
            dAp->setBeginArea(reqArea);
          }
        }
        dAp->setValidAccess(ap->getAccess());
        if (ap->hasAccess(frDirEnum::U)) {
          if (!(ap->getViaDefs().empty())) {
            dAp->setAccessViaDef(frDirEnum::U, &(ap->getViaDefs()));
          }
        }
        if (getRouteBox().contains(bp)) {
          if (isInitDR() && getRouteBox().right() == bp.x()
              && getRouteBox().top() == bp.y()) {
          } else if ((getRouteBox().right() == bp.x()
                      && bLayer->getDir() == frcVertPrefRoutingDir
                      && bLayer->getLef58RectOnlyConstraint())
                     || (getRouteBox().top() == bp.y()
                         && bLayer->getDir() == frcHorzPrefRoutingDir
                         && bLayer->getLef58RectOnlyConstraint())) {
          } else {
            dPin->addAccessPattern(std::move(dAp));
          }
        } else {
        }
      }
      pinIdx++;
    }


    if (dPin->getAccessPatterns().empty()) {
      initNet_termGenAp_new(dPin.get());
      if (dPin->getAccessPatterns().empty()) {
        cout << "\nError: pin " << name << " still does not have temp ap"
             << endl;
        exit(1);
      } else {
        // if (dNet->getFrNet()->getName() == string("iopin242")) {
        //   cout <<"@@@@@debug ";
        //   if (term->typeId() == frcInstTerm) {
        //     auto inst = static_cast<frInstTerm*>(term)->getInst();
        //     auto trueTerm = static_cast<frInstTerm*>(term)->getTerm();
        //     name = inst->getName() + string("/") + trueTerm->getName();
        //   } else if (term->typeId() == frcTerm) {
        //     trueTerm = static_cast<frTerm*>(term);
        //     name = string("PIN/") + trueTerm->getName();
        //   }
        //   for (auto &ap: dPin->getAccessPatterns()) {
        //     frPoint bp;
        //     ap->getPoint(bp);
        //       cout <<" (" <<bp.x() * 1.0 /
        //       getDesign()->getTopBlock()->getDBUPerUU() <<", "
        //                   <<bp.y() * 1.0 /
        //                   getDesign()->getTopBlock()->getDBUPerUU() <<") ";
        //   }
        //   cout <<endl;
        // }
      }
    }
    dPin->setId(pinCnt_);
    pinCnt_++;
    dNet->addPin(std::move(dPin));
  }
}

void FlexDRWorker::initNet_boundary(drNet* dNet,
                                    vector<unique_ptr<drConnFig>>& extObjs)
{
  // if (!isInitDR() && dNet->getFrNet()->getName() == string("net14488")) {
  // if (!isInitDR() && dNet->getFrNet()->getName() == string("net100629")) {
  //   cout <<"here" <<endl;
  // }
  auto gridBBox = getRouteBox();
  // location to area
  map<pair<frPoint, frLayerNum>, frCoord> extBounds;
  frSegStyle segStyle;
  frCoord currArea = 0;
  if (!isInitDR()) {
    for (auto& obj : extObjs) {
      if (obj->typeId() == drcPathSeg) {
        auto ps = static_cast<drPathSeg*>(obj.get());
        frPoint begin, end;
        ps->getPoints(begin, end);
        frLayerNum lNum = ps->getLayerNum();
        // vert pathseg
        if (begin.x() == end.x() && begin.x() >= gridBBox.left()
            && end.x() <= gridBBox.right()) {
          if (begin.y() == gridBBox.top()) {
            extBounds[make_pair(begin, lNum)] = currArea;
          }
          if (end.y() == gridBBox.bottom()) {
            extBounds[make_pair(end, lNum)] = currArea;
          }
          // horz pathseg
        } else if (begin.y() == end.y() && begin.y() >= gridBBox.bottom()
                   && end.y() <= gridBBox.top()) {
          if (begin.x() == gridBBox.right()) {
            extBounds[make_pair(begin, lNum)] = currArea;
          }
          if (end.x() == gridBBox.left()) {
            extBounds[make_pair(end, lNum)] = currArea;
          }
        }
      }
    }
    // initDR
  } else {
    auto it = boundaryPin_.find(dNet->getFrNet());
    // cout <<string(dNet->getFrNet() == nullptr ? "null" :
    // dNet->getFrNet()->getName()) <<endl;
    if (it != boundaryPin_.end()) {
      // cout <<"here" <<endl;
      // extBounds = it->second;
      transform(
          it->second.begin(),
          it->second.end(),
          inserter(extBounds, extBounds.end()),
          [](const pair<frPoint, frLayerNum>& pr) { return make_pair(pr, 0); });
    }
  }
  for (auto& [pr, area] : extBounds) {
    auto& [pt, lNum] = pr;
    auto dPin = make_unique<drPin>();
    auto dAp = make_unique<drAccessPattern>();
    dAp->setPoint(pt);
    dAp->setBeginLayerNum(lNum);
    dPin->addAccessPattern(std::move(dAp));
    // ap
    dPin->setId(pinCnt_);
    pinCnt_++;
    dNet->addPin(std::move(dPin));
  }
}

void FlexDRWorker::initNet_addNet(unique_ptr<drNet> in)
{
  owner2nets_[in->getFrNet()].push_back(in.get());
  nets_.push_back(std::move(in));
}

void FlexDRWorker::initNet(frNet* net,
                           vector<unique_ptr<drConnFig>>& routeObjs,
                           vector<unique_ptr<drConnFig>>& extObjs,
                           vector<frRect>& origGuides,
                           vector<frBlockObject*>& terms)
{
  auto dNet = make_unique<drNet>();
  dNet->setFrNet(net);
  // true pin
  initNet_term_new(dNet.get(), terms);
  // boundary pin, could overlap with any of true pins
  initNet_boundary(dNet.get(), extObjs);
  // no ext routes in initDR to avoid weird TA shapes
  for (auto& obj : extObjs) {
    dNet->addRoute(std::move(obj), true);
  }
  if (getRipupMode() != 1) {
    for (auto& obj : routeObjs) {
      dNet->addRoute(std::move(obj), false);
    }
  }
  dNet->setOrigGuides(origGuides);
  dNet->setId(nets_.size());
  initNet_addNet(std::move(dNet));
}

void FlexDRWorker::initNets_regionQuery()
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.init();
}

void FlexDRWorker::initNets_numPinsIn()
{
  vector<rq_box_value_t<drPin*>> allPins;
  frPoint pt;
  for (auto& net : nets_) {
    for (auto& pin : net->getPins()) {
      bool hasPrefAP = false;
      drAccessPattern* firstAP = nullptr;
      for (auto& ap : pin->getAccessPatterns()) {
        if (firstAP == nullptr) {
          firstAP = ap.get();
        }
        if (ap->getPinCost() == 0) {
          ap->getPoint(pt);
          allPins.push_back(
              make_pair(box_t(point_t(pt.x(), pt.y()), point_t(pt.x(), pt.y())),
                        pin.get()));
          hasPrefAP = true;
          break;
        }
      }
      if (!hasPrefAP) {
        firstAP->getPoint(pt);
        allPins.push_back(
            make_pair(box_t(point_t(pt.x(), pt.y()), point_t(pt.x(), pt.y())),
                      pin.get()));
      }
    }
  }
  bgi::rtree<rq_box_value_t<drPin*>, bgi::quadratic<16>> pinRegionQuery(
      allPins);
  for (auto& net : nets_) {
    frCoord x1 = getExtBox().right();
    frCoord x2 = getExtBox().left();
    frCoord y1 = getExtBox().top();
    frCoord y2 = getExtBox().bottom();
    for (auto& pin : net->getPins()) {
      bool hasPrefAP = false;
      drAccessPattern* firstAP = nullptr;
      for (auto& ap : pin->getAccessPatterns()) {
        if (firstAP == nullptr) {
          firstAP = ap.get();
        }
        if (ap->getPinCost() == 0) {
          ap->getPoint(pt);
          hasPrefAP = true;
          break;
        }
      }
      if (!hasPrefAP) {
        firstAP->getPoint(pt);
      }

      if (pt.x() < x1) {
        x1 = pt.x();
      }
      if (pt.x() > x2) {
        x2 = pt.x();
      }
      if (pt.y() < y1) {
        y1 = pt.y();
      }
      if (pt.y() > y2) {
        y2 = pt.y();
      }
    }
    if (x1 <= x2 && y1 <= y2) {
      frBox box = frBox(frPoint(x1, y1), frPoint(x2, y2));
      allPins.clear();
      pinRegionQuery.query(bgi::intersects(box), back_inserter(allPins));
      net->setNumPinsIn(allPins.size());
      net->setPinBox(box);
    } else {
      net->setNumPinsIn(99999);
      net->setPinBox(getExtBox());
    }
  }
}

void FlexDRWorker::initNets_boundaryArea()
{
  frPoint bp, psBp, psEp, pt2 /*, psBp2, psEp2*/;
  frLayerNum lNum;
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  vector<rq_box_value_t<drConnFig*>> results2;
  frBox queryBox;
  frBox queryBox2;
  frCoord currArea = 0;
  frSegStyle segStyle;

  for (auto& uNet : nets_) {
    auto net = uNet.get();
    for (auto& pin : net->getPins()) {
      if (pin->hasFrTerm()) {
        continue;
      }
      for (auto& ap : pin->getAccessPatterns()) {
        // initialization
        results.clear();
        currArea = 0;

        ap->getPoint(bp);
        lNum = ap->getBeginLayerNum();
        queryBox.set(bp, bp);
        workerRegionQuery.query(queryBox, lNum, results);
        for (auto& [ignored, connFig] : results) {
          if (connFig->getNet() != net) {
            continue;
          }
          if (connFig->typeId() == drcPathSeg) {
            auto obj = static_cast<drPathSeg*>(connFig);
            obj->getPoints(psBp, psEp);
            // psEp is outside
            if (bp == psBp && (!getRouteBox().contains(psEp))) {
              // calc area
              obj->getStyle(segStyle);
              currArea += (abs(psEp.x() - psBp.x()) + abs(psEp.y() - psBp.y()))
                          * segStyle.getWidth();
              results2.clear();
              queryBox2.set(psEp, psEp);
              workerRegionQuery.query(queryBox2, lNum, results2);
              for (auto& [viaBox2, connFig2] : results) {
                if (connFig2->getNet() != net) {
                  continue;
                }
                if (connFig2->typeId() == drcVia) {
                  auto obj2 = static_cast<drVia*>(connFig2);
                  obj2->getOrigin(pt2);
                  if (pt2 == psEp) {
                    currArea += viaBox2.width() * viaBox2.length() / 2;
                    break;
                  }
                } else if (connFig2->typeId() == drcPatchWire) {
                  auto obj2 = static_cast<drPatchWire*>(connFig2);
                  obj2->getOrigin(pt2);
                  if (pt2 == psEp) {
                    currArea += viaBox2.width()
                                * viaBox2.length();  // patch wire no need / 2
                    break;
                  }
                }
              }
            }
            // psBp is outside
            if ((!getRouteBox().contains(psBp)) && bp == psEp) {
              // calc area
              obj->getStyle(segStyle);
              currArea += (abs(psEp.x() - psBp.x()) + abs(psEp.y() - psBp.y()))
                          * segStyle.getWidth();
              results2.clear();
              queryBox2.set(psEp, psEp);
              workerRegionQuery.query(queryBox2, lNum, results2);
              for (auto& [viaBox2, connFig2] : results) {
                if (connFig2->getNet() != net) {
                  continue;
                }
                if (connFig2->typeId() == drcVia) {
                  auto obj2 = static_cast<drVia*>(connFig2);
                  obj2->getOrigin(pt2);
                  if (pt2 == psBp) {
                    currArea += viaBox2.width() * viaBox2.length() / 2;
                    break;
                  }
                } else if (connFig2->typeId() == drcPatchWire) {
                  auto obj2 = static_cast<drPatchWire*>(connFig2);
                  obj2->getOrigin(pt2);
                  if (pt2 == psBp) {
                    currArea += viaBox2.width() * viaBox2.length();
                    break;
                  }
                }
              }
            }
          }
        }
        ap->setBeginArea(currArea);
      }
    }
  }
}

void FlexDRWorker::initNets()
{
  set<frNet*, frBlockObjectComp> nets;
  map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp> netRouteObjs;
  map<frNet*, vector<unique_ptr<drConnFig>>, frBlockObjectComp> netExtObjs;
  map<frNet*, vector<frRect>, frBlockObjectComp> netOrigGuides;
  // get lock
  initNetObjs(nets, netRouteObjs, netExtObjs, netOrigGuides);
  // release lock
  if (isInitDR()) {
    initNets_initDR(nets, netRouteObjs, netExtObjs, netOrigGuides);
  } else {
    // find inteTerm/terms using netRouteObjs;
    initNets_searchRepair(nets, netRouteObjs, netExtObjs, netOrigGuides);
  }
  initNets_regionQuery();
  initNets_numPinsIn();
  // here because region query is needed
  if (ENABLE_BOUNDARY_MAR_FIX) {
    initNets_boundaryArea();
  }
}

void FlexDRWorker::initTrackCoords_route(
    drNet* net,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add for routes
  vector<drConnFig*> allObjs;
  for (auto& uConnFig : net->getExtConnFigs()) {
    allObjs.push_back(uConnFig.get());
  }
  for (auto& uConnFig : net->getRouteConnFigs()) {
    allObjs.push_back(uConnFig.get());
  }
  for (auto& uConnFig : allObjs) {
    if (uConnFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(uConnFig);
      frPoint bp, ep;
      obj->getPoints(bp, ep);
      auto lNum = obj->getLayerNum();
      // vertical
      if (bp.x() == ep.x()) {
        // non pref dir
        if (getTech()->getLayer(lNum)->getDir() == frcHorzPrefRoutingDir) {
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            xMap[bp.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            xMap[bp.x()][lNum - 2] = nullptr;
          } else {
            cout << "Error: initTrackCoords cannot add non-pref track" << endl;
          }
          // add bp, ep
          yMap[bp.y()][lNum] = nullptr;
          yMap[ep.y()][lNum] = nullptr;
          // pref dir
        } else {
          xMap[bp.x()][lNum] = nullptr;
          // add bp, ep
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            yMap[bp.y()][lNum + 2]
                = nullptr;  // default add track to upper layer
            yMap[ep.y()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            yMap[bp.y()][lNum - 2] = nullptr;
            yMap[ep.y()][lNum - 2] = nullptr;
          } else {
            cout << "Error: initTrackCoords cannot add non-pref track" << endl;
          }
        }
        // horizontal
      } else {
        // non pref dir
        if (getTech()->getLayer(lNum)->getDir() == frcVertPrefRoutingDir) {
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            yMap[bp.y()][lNum + 2] = nullptr;
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            yMap[bp.y()][lNum - 2] = nullptr;
          } else {
            cout << "Error: initTrackCoords cannot add non-pref track" << endl;
          }
          // add bp, ep
          xMap[bp.x()][lNum] = nullptr;
          xMap[ep.x()][lNum] = nullptr;
        } else {
          yMap[bp.y()][lNum] = nullptr;
          // add bp, ep
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            xMap[bp.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
            xMap[ep.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            xMap[bp.x()][lNum - 2] = nullptr;
            xMap[ep.x()][lNum - 2] = nullptr;
          } else {
            cout << "Error: initTrackCoords cannot add non-pref track" << endl;
          }
        }
      }
    } else if (uConnFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(uConnFig);
      frPoint pt;
      obj->getOrigin(pt);
      // add pref dir track to layer1
      auto layer1Num = obj->getViaDef()->getLayer1Num();
      if (getTech()->getLayer(layer1Num)->getDir() == frcHorzPrefRoutingDir) {
        yMap[pt.y()][layer1Num] = nullptr;
      } else {
        xMap[pt.x()][layer1Num] = nullptr;
      }
      // add pref dir track to layer2
      auto layer2Num = obj->getViaDef()->getLayer2Num();
      if (getTech()->getLayer(layer2Num)->getDir() == frcHorzPrefRoutingDir) {
        yMap[pt.y()][layer2Num] = nullptr;
      } else {
        xMap[pt.x()][layer2Num] = nullptr;
      }
    } else if (uConnFig->typeId() == drcPatchWire) {
    } else {
      cout << "Error: initTrackCoords unsupported type" << endl;
    }
  }
}

void FlexDRWorker::initTrackCoords_pin(
    drNet* net,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add for aps
  for (auto& pin : net->getPins()) {
    for (auto& ap : pin->getAccessPatterns()) {
      frPoint pt;
      ap->getPoint(pt);
      auto lNum = ap->getBeginLayerNum();
      frLayerNum lNum2 = 0;
      if (lNum + 2 <= getTech()->getTopLayerNum()) {
        lNum2 = lNum + 2;
      } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
        lNum2 = lNum - 2;
      } else {
        cout << "Error: initTrackCoords cannot add non-pref track" << endl;
      }
      // if (net->getFrNet() && net->getFrNet()->getName() == string("sa13_1_"))
      // {
      //   if (pin->getFrTerm() && pin->getFrTerm()->typeId() == frcInstTerm) {
      //     frInstTerm* instTerm = (frInstTerm*)pin->getFrTerm();
      //     // if (instTerm->getInst()->getRefBlock()->getName() ==
      //     "DFFSQ_X1N_A10P5PP84TR_C14_mod" && instTerm->getTerm()->getName()
      //     == "Q") {
      //     //   cout << "  initTrackCoords ap (" << pt.x() / 2000.0 << ", " <<
      //     pt.y() / 2000.0 << ")\n";
      //     //   cout << "    lNum = " << lNum << ", lNum2 = " << lNum2 <<
      //     endl;
      //     // }
      //   }
      // }
      if (getTech()->getLayer(lNum)->getDir() == frcHorzPrefRoutingDir) {
        yMap[pt.y()][lNum] = nullptr;
      } else {
        xMap[pt.x()][lNum] = nullptr;
      }
      if (getTech()->getLayer(lNum2)->getDir() == frcHorzPrefRoutingDir) {
        yMap[pt.y()][lNum2] = nullptr;
      } else {
        xMap[pt.x()][lNum2] = nullptr;
      }
    }
  }
}

void FlexDRWorker::initTrackCoords(
    map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add boundary points
  // lNum = -10 to indicate routeBox and extBox frCoord
  auto rbox = getRouteBox();
  auto ebox = getExtBox();
  yMap[rbox.bottom()][-10] = nullptr;
  yMap[rbox.top()][-10] = nullptr;
  yMap[ebox.bottom()][-10] = nullptr;
  yMap[ebox.top()][-10] = nullptr;
  xMap[rbox.left()][-10] = nullptr;
  xMap[rbox.right()][-10] = nullptr;
  xMap[ebox.left()][-10] = nullptr;
  xMap[ebox.right()][-10] = nullptr;
  // add all track coords
  for (auto& net : nets_) {
    initTrackCoords_route(net.get(), xMap, yMap);
    initTrackCoords_pin(net.get(), xMap, yMap);
  }
}

void FlexDRWorker::initGridGraph()
{
  // get all track coords based on existing objs and aps
  map<frCoord, map<frLayerNum, frTrackPattern*>> xMap;
  map<frCoord, map<frLayerNum, frTrackPattern*>> yMap;
  initTrackCoords(xMap, yMap);
  gridGraph_.setCost(workerDRCCost_, workerMarkerCost_);
  gridGraph_.init(
      getRouteBox(), getExtBox(), xMap, yMap, isInitDR(), isFollowGuide());
  // gridGraph.print();
}

void FlexDRWorker::initMazeIdx_connFig(drConnFig* connFig)
{
  if (connFig->typeId() == drcPathSeg) {
    auto obj = static_cast<drPathSeg*>(connFig);
    frPoint bp, ep;
    obj->getPoints(bp, ep);
    bp.set(max(bp.x(), getExtBox().left()), max(bp.y(), getExtBox().bottom()));
    ep.set(min(ep.x(), getExtBox().right()), min(ep.y(), getExtBox().top()));
    auto lNum = obj->getLayerNum();
    if (gridGraph_.hasMazeIdx(bp, lNum) && gridGraph_.hasMazeIdx(ep, lNum)) {
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bi, bp, lNum);
      gridGraph_.getMazeIdx(ei, ep, lNum);
      obj->setMazeIdx(bi, ei);
      // cout <<"has idx pathseg" <<endl;
    } else {
      cout << "Error: initMazeIdx_connFig pathseg no idx ("
           << bp.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", "
           << bp.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") ("
           << ep.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", "
           << ep.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") "
           << getTech()->getLayer(lNum)->getName() << endl;
    }
  } else if (connFig->typeId() == drcVia) {
    auto obj = static_cast<drVia*>(connFig);
    frPoint bp;
    obj->getOrigin(bp);
    auto layer1Num = obj->getViaDef()->getLayer1Num();
    auto layer2Num = obj->getViaDef()->getLayer2Num();
    if (gridGraph_.hasMazeIdx(bp, layer1Num)
        && gridGraph_.hasMazeIdx(bp, layer2Num)) {
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bi, bp, layer1Num);
      gridGraph_.getMazeIdx(ei, bp, layer2Num);
      obj->setMazeIdx(bi, ei);
      // cout <<"has idx via" <<endl;
    } else {
      cout << "Error: initMazeIdx_connFig via no idx ("
           << bp.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", "
           << bp.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") "
           << getTech()->getLayer(layer1Num + 1)->getName() << endl;
    }
  } else if (connFig->typeId() == drcPatchWire) {
  } else {
    cout << "Error: initMazeIdx_connFig unsupported type" << endl;
  }
}

void FlexDRWorker::initMazeIdx_ap(drAccessPattern* ap)
{
  frPoint bp;
  ap->getPoint(bp);
  auto lNum = ap->getBeginLayerNum();
  if (gridGraph_.hasMazeIdx(bp, lNum)) {
    FlexMazeIdx bi;
    gridGraph_.getMazeIdx(bi, bp, lNum);
    ap->setMazeIdx(bi);
    // set curr layer on track status
    if (getDesign()->getTech()->getLayer(lNum)->getDir()
        == frcHorzPrefRoutingDir) {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::W)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::E)) {
        ap->setOnTrack(false, true);
      }
    } else {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::S)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::N)) {
        ap->setOnTrack(false, false);
      }
    }
    // cout <<"has idx via" <<endl;
  } else {
    cout << "Error: initMazeIdx_ap no idx ("
         << bp.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", "
         << bp.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") "
         << getTech()->getLayer(lNum)->getName() << endl;
  }

  if (gridGraph_.hasMazeIdx(bp, lNum + 2)) {
    FlexMazeIdx bi;
    gridGraph_.getMazeIdx(bi, bp, lNum + 2);
    // set curr layer on track status
    if (getDesign()->getTech()->getLayer(lNum + 2)->getDir()
        == frcHorzPrefRoutingDir) {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::W)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::E)) {
        ap->setOnTrack(false, true);
      }
    } else {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::S)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::N)) {
        ap->setOnTrack(false, false);
      }
    }
    // cout <<"has idx via" <<endl;
  } else {
    ;
  }
}

void FlexDRWorker::initMazeIdx()
{
  for (auto& net : nets_) {
    for (auto& connFig : net->getExtConnFigs()) {
      initMazeIdx_connFig(connFig.get());
    }
    for (auto& connFig : net->getRouteConnFigs()) {
      initMazeIdx_connFig(connFig.get());
    }
    for (auto& pin : net->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        initMazeIdx_ap(ap.get());
      }
    }
  }
}

void FlexDRWorker::initMazeCost_ap_planarGrid_helper(const FlexMazeIdx& mi,
                                                     const frDirEnum& dir,
                                                     frCoord bloatLen,
                                                     bool isAddPathCost)
{
  frCoord currLen = 0;
  frMIdx x = mi.x();
  frMIdx y = mi.y();
  frMIdx z = mi.z();
  while (1) {
    if (currLen > bloatLen || !gridGraph_.hasEdge(x, y, z, dir)) {
      break;
    }
    if (isAddPathCost) {
      gridGraph_.setGridCost(x, y, z, dir);
      gridGraph_.setGridCost(x, y, z, frDirEnum::D);
      gridGraph_.setGridCost(x, y, z, frDirEnum::U);
    } else {
      gridGraph_.resetGridCost(x, y, z, dir);
      gridGraph_.resetGridCost(x, y, z, frDirEnum::D);
      gridGraph_.resetGridCost(x, y, z, frDirEnum::U);
    }
    switch (dir) {
      case frDirEnum::W:
        --x;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::W);
        break;
      case frDirEnum::E:
        ++x;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::E);
        break;
      case frDirEnum::S:
        --y;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::S);
        break;
      case frDirEnum::N:
        ++y;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::N);
        break;
      default:;
    }
  }
}

void FlexDRWorker::initMazeCost_ap_helper(drNet* net, bool isAddPathCost)
{
  FlexMazeIdx mi;
  frLayerNum lNum;
  frCoord defaultWidth;
  int planarGridBloatNumWidth = 10;
  for (auto& pin : net->getPins()) {
    bool isStdCellPin = true;
    auto term = pin->getFrTerm();
    if (term) {
      // macro cell or stdcell
      if (term->typeId() == frcInstTerm) {
        if (static_cast<frInstTerm*>(term)
                    ->getInst()
                    ->getRefBlock()
                    ->getMacroClass()
                == MacroClassEnum::BLOCK
            || isPad(static_cast<frInstTerm*>(term)
                         ->getInst()
                         ->getRefBlock()
                         ->getMacroClass())
            || static_cast<frInstTerm*>(term)
                       ->getInst()
                       ->getRefBlock()
                       ->getMacroClass()
                   == MacroClassEnum::RING) {
          isStdCellPin = false;
          // cout <<"curr dPin is macro pin" <<endl;
        } else {
          // cout <<"curr dPin is stdcell pin" <<endl;
        }
        // IO
      } else if (term->typeId() == frcTerm) {
        isStdCellPin = false;
        // cout <<"curr dPin is io pin" <<endl;
      }
    } else {
      continue;
    }

    bool hasUpperOnTrackAP = false;
    if (isStdCellPin) {
      for (auto& ap : pin->getAccessPatterns()) {
        lNum = ap->getBeginLayerNum();
        if (ap->hasValidAccess(frDirEnum::U)) {
          if (getDesign()->getTech()->getLayer(lNum + 2)->getDir()
                  == frcHorzPrefRoutingDir
              && ap->isOnTrack(true)) {
            hasUpperOnTrackAP = true;
            break;
          }
          if (getDesign()->getTech()->getLayer(lNum + 2)->getDir()
                  == frcVertPrefRoutingDir
              && ap->isOnTrack(false)) {
            hasUpperOnTrackAP = true;
            break;
          }
        }
      }
    }

    for (auto& ap : pin->getAccessPatterns()) {
      ap->getMazeIdx(mi);
      lNum = ap->getBeginLayerNum();
      defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();
      if (ap->hasValidAccess(frDirEnum::U)) {
        lNum = ap->getBeginLayerNum();
        if (lNum + 2 <= getDesign()->getTech()->getTopLayerNum()) {
          auto upperDefaultWidth
              = getDesign()->getTech()->getLayer(lNum + 2)->getWidth();
          if (getDesign()->getTech()->getLayer(lNum + 2)->getDir()
                  == frcHorzPrefRoutingDir
              && !ap->isOnTrack(true)) {
            if (!hasUpperOnTrackAP) {
              auto upperMi = FlexMazeIdx(mi.x(), mi.y(), mi.z() + 1);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::W,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::E,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
            }
          }
          if (getDesign()->getTech()->getLayer(lNum + 2)->getDir()
                  == frcVertPrefRoutingDir
              && !ap->isOnTrack(false)) {
            if (!hasUpperOnTrackAP) {
              auto upperMi = FlexMazeIdx(mi.x(), mi.y(), mi.z() + 1);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::N,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::S,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
            }
          }
        }

        if (isAddPathCost) {
          gridGraph_.resetOverrideShapeCostVia(mi.x(), mi.y(), mi.z());
        } else {
          gridGraph_.setOverrideShapeCostVia(mi.x(), mi.y(), mi.z());
        }
      }

      if (ap->hasValidAccess(frDirEnum::W)) {
        if (!isStdCellPin) {
          initMazeCost_ap_planarGrid_helper(
              mi,
              frDirEnum::W,
              planarGridBloatNumWidth * defaultWidth,
              isAddPathCost);
        }
      }
      if (ap->hasValidAccess(frDirEnum::E)) {
        if (!isStdCellPin) {
          initMazeCost_ap_planarGrid_helper(
              mi,
              frDirEnum::E,
              planarGridBloatNumWidth * defaultWidth,
              isAddPathCost);
        }
      }
      if (ap->hasValidAccess(frDirEnum::S)) {
        if (!isStdCellPin) {
          initMazeCost_ap_planarGrid_helper(
              mi,
              frDirEnum::S,
              planarGridBloatNumWidth * defaultWidth,
              isAddPathCost);
        }
      }
      if (ap->hasValidAccess(frDirEnum::N)) {
        if (!isStdCellPin) {
          initMazeCost_ap_planarGrid_helper(
              mi,
              frDirEnum::N,
              planarGridBloatNumWidth * defaultWidth,
              isAddPathCost);
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_ap()
{
  int cnt = 0;
  FlexMazeIdx mi;
  for (auto& net : nets_) {
    for (auto& pin : net->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        ap->getMazeIdx(mi);
        if (!ap->hasValidAccess(frDirEnum::E)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::E);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::E);
        }

        if (!ap->hasValidAccess(frDirEnum::W)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::W);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::W);
        }

        if (!ap->hasValidAccess(frDirEnum::N)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::N);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::N);
        }

        if (!ap->hasValidAccess(frDirEnum::S)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::S);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::S);
        }

        if (!ap->hasValidAccess(frDirEnum::U)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::U);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::U);
        }

        if (!ap->hasValidAccess(frDirEnum::D)) {
          gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::D);
        } else {
          gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), frDirEnum::D);
        }

        if (ap->hasAccessViaDef(frDirEnum::U)) {
          gridGraph_.setSVia(mi.x(), mi.y(), mi.z());
          apSVia_[mi] = ap.get();
          if (ap->getAccessViaDef()
              != getDesign()
                     ->getTech()
                     ->getLayer(ap->getBeginLayerNum() + 1)
                     ->getDefaultViaDef()) {
            cnt++;
          }
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_marker_fixMode_0(const frMarker& marker)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;
  int xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);

  results.clear();
  marker.getBBox(mBox);
  if (!getDrcBox().overlaps(mBox)) {
    return;
  }
  auto lNum = marker.getLayerNum();
  frCoord bloatDist;
  if (getDesign()->getTech()->getLayer(lNum)->getType()
      == frLayerTypeEnum::CUT) {
    bloatDist = getDesign()->getTech()->getLayer(lNum - 1)->getWidth()
                * workerMarkerBloatWidth_;
  } else {
    bloatDist = getDesign()->getTech()->getLayer(lNum)->getWidth()
                * workerMarkerBloatWidth_;
  }
  mBox.bloat(bloatDist, bloatBox);
  workerRegionQuery.query(bloatBox, lNum, results);
  for (auto& [objBox, connFig] : results) {
    // for pathseg-related marker, bloat marker by half width and add marker
    // cost planar
    if (connFig->typeId() == drcPathSeg) {
      // cout <<"@@pathseg" <<endl;
      //  update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy);

      connFig->getNet()->setRipup();
      // new
      gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);
      auto obj = static_cast<drPathSeg*>(connFig);
      FlexMazeIdx psMIdx1, psMIdx2;
      obj->getMazeIdx(psMIdx1, psMIdx2);
      bool isH = (psMIdx1.y() == psMIdx2.y());
      if (isH) {
        for (int i = max(0, max(psMIdx1.x(), mIdx1.x() - 1));
             i <= min(xDim - 1, (psMIdx2.x(), mIdx2.x() + 1));
             i++) {
          gridGraph_.addMarkerCostPlanar(i, psMIdx1.y(), psMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(i, psMIdx1.y(), psMIdx1.z()));
        }
      } else {
        for (int i = max(0, max(psMIdx1.y(), mIdx1.y() - 1));
             i <= min(yDim - 1, min(psMIdx2.y(), mIdx2.y() + 1));
             i++) {
          gridGraph_.addMarkerCostPlanar(psMIdx1.x(), i, psMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(psMIdx1.x(), i, psMIdx1.z()));
        }
      }

      // for via-related marker, add marker cost via
    } else if (connFig->typeId() == drcVia) {
      // cout <<"@@via" <<endl;
      if (getDesign()->getTech()->getLayer(lNum)->getType()
              == frLayerTypeEnum::ROUTING
          && marker.getConstraint()->typeId()
                 == frConstraintTypeEnum::frcShortConstraint) {
        continue;
      }
      // update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy);

      auto obj = static_cast<drVia*>(connFig);
      obj->getMazeIdx(mIdx1, mIdx2);
      connFig->getNet()->setRipup();
      gridGraph_.addMarkerCostVia(mIdx1.x(), mIdx1.y(), mIdx1.z());
      viaHistoryMarkers_.insert(mIdx1);
    } else if (connFig->typeId() == drcPatchWire) {
      // TODO: could add marker // for now we think the other part in the
      // violation would not be patchWire
      auto obj = static_cast<drPatchWire*>(connFig);
      // update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy);

      frPoint bp;
      obj->getOrigin(bp);
      if (getExtBox().contains(bp)) {
        gridGraph_.getMazeIdx(mIdx1, bp, lNum);
        connFig->getNet()->setRipup();
        gridGraph_.addMarkerCostPlanar(mIdx1.x(), mIdx1.y(), mIdx1.z());
        planarHistoryMarkers_.insert(
            FlexMazeIdx(mIdx1.x(), mIdx1.y(), mIdx1.z()));
      }
    } else {
      cout << "Error: unsupported dr type" << endl;
    }
  }
}

void FlexDRWorker::initMazeCost_marker_fixMode_1(const frMarker& marker,
                                                 bool keepViaNet)
{

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;
  int xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);

  marker.getBBox(mBox);
  auto lNum = marker.getLayerNum();
  //  skip if marker outside of routebox, ignore drcbox here because we would
  //  like to push wire
  if (!getRouteBox().overlaps(mBox)) {
    return;
  }
  // skip if marker not metal layer
  if (!(getDesign()->getTech()->getLayer(lNum)->getType()
        == frLayerTypeEnum::ROUTING)) {
    return;
  }
  // skip if marker is unknown type
  if (!(marker.getConstraint()->typeId()
            == frConstraintTypeEnum::frcShortConstraint
        || marker.getConstraint()->typeId()
               == frConstraintTypeEnum::frcSpacingConstraint
        || marker.getConstraint()->typeId()
               == frConstraintTypeEnum::frcSpacingEndOfLineConstraint
        || marker.getConstraint()->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint
        || marker.getConstraint()->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint)) {
    return;
  }
  // skip if not fat via
  bool hasFatVia = false;
  results.clear();
  workerRegionQuery.query(mBox, lNum, results);
  bool bloatXp = false;  // bloat to x+ dir
  bool bloatXn = false;  // bloat to x- dir
  bool bloatYp = false;  // bloat to y+ dir
  bool bloatYn = false;  // bloat to y- dir
  int numVias = 0;
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();
  frCoord viaWidth = 0;
  bool isLayerH = (getDesign()->getTech()->getLayer(lNum)->getDir()
                   == frcHorzPrefRoutingDir);
  drNet* viaNet = nullptr;
  for (auto& [objBox, connFig] : results) {
    if (connFig->typeId() == drcVia) {
      // cout <<"@@via" <<endl;
      viaNet = connFig->getNet();
      if (marker.hasDir()) {  // spacing violation
        if (marker.isH() && isLayerH) {
          if (objBox.top() == mBox.bottom()) {
            bloatYp = true;
          }
          if (objBox.bottom() == mBox.top()) {
            bloatYn = true;
          }
          if (objBox.top() - objBox.bottom() > defaultWidth) {
            hasFatVia = true;
            viaWidth = objBox.top() - objBox.bottom();
          }
        } else if ((!marker.isH()) && (!isLayerH)) {
          if (objBox.right() == mBox.left()) {
            bloatXp = true;
          }
          if (objBox.left() == mBox.right()) {
            bloatXn = true;
          }
          if (objBox.right() - objBox.left() > defaultWidth) {
            hasFatVia = true;
            viaWidth = objBox.right() - objBox.left();
          }
        }
      } else {  // short violation
        bloatXp = true;
        bloatXn = true;
        bloatYp = true;
        bloatYn = true;
        if (isLayerH) {
          if (objBox.top() - objBox.bottom() > defaultWidth) {
            hasFatVia = true;
            viaWidth = objBox.top() - objBox.bottom();
          }
        } else {
          if (objBox.right() - objBox.left() > defaultWidth) {
            hasFatVia = true;
            viaWidth = objBox.right() - objBox.left();
          }
        }
      }
      numVias++;
    }
  }
  // skip if no via or more than one via
  if (numVias != 1) {
    return;
  }
  // skip if no fat via
  if (!hasFatVia) {
    return;
  }

  // calc bloat box
  frCoord bloatDist;
  bloatDist = getDesign()->getTech()->getLayer(lNum)->getWidth() * viaWidth;
  mBox.bloat(bloatDist, bloatBox);
  bloatBox.set(bloatXn ? bloatBox.left() : mBox.left(),
               bloatYn ? bloatBox.bottom() : mBox.bottom(),
               bloatXp ? bloatBox.right() : mBox.right(),
               bloatYp ? bloatBox.top() : mBox.top());


  // get all objs
  results.clear();
  workerRegionQuery.query(bloatBox, lNum, results);
  for (auto& [objBox, connFig] : results) {
    // do not add marker cost for ps, rely on DRCCOST
    if (connFig->typeId() == drcPathSeg) {
      // cout <<"@@pathseg" <<endl;
      auto currNet = connFig->getNet();
      // update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy);
      if (viaNet != currNet) {
        currNet->setRipup();
      }
      // add marker cost via
    } else if (connFig->typeId() == drcVia) {
      if (!keepViaNet) {
        connFig->getNet()->updateMarkerDist(-1);
        auto obj = static_cast<drVia*>(connFig);
        obj->getMazeIdx(mIdx1, mIdx2);
        connFig->getNet()->setRipup();
      }
    } else if (connFig->typeId() == drcPatchWire) {
      // TODO: could add marker // for now we think the other part in the
      // violation would not be patchWire
    } else {
      cout << "Error: unsupported dr type" << endl;
    }
  }
}

void FlexDRWorker::initMazeCost_marker_route_queue_addHistoryCost(
    const frMarker& marker)
{

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;
  set<drNet*> vioNets;  // for self-violation, only add cost for one side
                        // (experiment with self cut spacing)

  marker.getBBox(mBox);
  auto lNum = marker.getLayerNum();

  workerRegionQuery.query(mBox, lNum, results);
  frPoint bp, ep;
  frCoord width;
  frSegStyle segStyle;
  FlexMazeIdx objMIdx1, objMIdx2;

  for (auto& [objBox, connFig] : results) {
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      // skip if unfixable obj
      obj->getPoints(bp, ep);
      if (!(getRouteBox().contains(bp) && getRouteBox().contains(ep))) {
        continue;
      }
      // add history cost
      // get points to mark up, markup up to "width" grid points to the left and
      // right of pathseg
      obj->getStyle(segStyle);
      width = segStyle.getWidth();
      mBox.bloat(width, bloatBox);
      gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);

      obj->getMazeIdx(objMIdx1, objMIdx2);
      bool isH = (objMIdx1.y() == objMIdx2.y());
      // temporarily bloat marker at most 4X width to find neighboring grid
      // point block at least two width grid points to ensure no matter where
      // the startpoint is
      for (int i = 0; i < 5; i++) {
        if (i == 4) {
          cout << "Warning: marker bloat 4x width but could not find two grids "
                  "to add marker cost"
               << endl;
          cout << "  marker -- src: ";
          for (auto src : marker.getSrcs()) {
            if (src) {
              switch (src->typeId()) {
                case frcNet:
                  cout << (static_cast<frNet*>(src))->getName() << " ";
                  break;
                case frcInstTerm: {
                  frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
                  cout << instTerm->getInst()->getName() << "/"
                       << instTerm->getTerm()->getName() << " ";
                  break;
                }
                case frcTerm: {
                  frTerm* term = (static_cast<frTerm*>(src));
                  cout << "PIN/" << term->getName() << " ";
                  break;
                }
                case frcInstBlockage: {
                  frInstBlockage* instBlockage
                      = (static_cast<frInstBlockage*>(src));
                  cout << instBlockage->getInst()->getName() << "/OBS"
                       << " ";
                  break;
                }
                case frcBlockage: {
                  cout << "PIN/OBS"
                       << " ";
                  break;
                }
                default:;
              }
            }
            cout << "\n";
            // get violation bbox
            frBox bbox;
            marker.getBBox(bbox);
            double dbu = design_->getTech()->getDBUPerUU();
            cout << "    bbox = ( " << bbox.left() / dbu << ", "
                 << bbox.bottom() / dbu << " ) - ( " << bbox.right() / dbu
                 << ", " << bbox.top() / dbu << " ) on Layer ";
            if (getTech()->getLayer(marker.getLayerNum())->getType()
                    == frLayerTypeEnum::CUT
                && marker.getLayerNum() - 1 >= getTech()->getBottomLayerNum()) {
              cout << getTech()->getLayer(marker.getLayerNum() - 1)->getName()
                   << "\n";
            } else {
              cout << getTech()->getLayer(marker.getLayerNum())->getName()
                   << "\n";
            }
          }
        }
        if (isH
            && max(objMIdx1.x(), mIdx1.x()) >= min(objMIdx2.x(), mIdx2.x())) {
          bloatBox.bloat(width, bloatBox);
          // cout <<"i=" <<i <<" " <<width <<", " <<bloatBox <<endl;
        } else if ((!isH)
                   && max(objMIdx1.y(), mIdx1.y())
                          >= min(objMIdx2.y(), mIdx2.y())) {
          bloatBox.bloat(width, bloatBox);
          // cout <<bloatBox <<endl;
        } else {
          break;
        }
        gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);
      }
      if (isH) {
        for (int i = max(objMIdx1.x(), mIdx1.x());
             i <= min(objMIdx2.x(), mIdx2.x());
             i++) {
          gridGraph_.addMarkerCostPlanar(i, objMIdx1.y(), objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(i, objMIdx1.y(), objMIdx1.z()));
        }
      } else {
        for (int i = max(objMIdx1.y(), mIdx1.y());
             i <= min(objMIdx2.y(), mIdx2.y());
             i++) {
          gridGraph_.addMarkerCostPlanar(objMIdx1.x(), i, objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(objMIdx1.x(), i, objMIdx1.z()));
        }
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      obj->getOrigin(bp);
      // skip if unfixable obj
      if (!getRouteBox().contains(bp)) {
        continue;
      }
      if (vioNets.find(obj->getNet()) == vioNets.end()) {
        // add history cost
        obj->getMazeIdx(objMIdx1, objMIdx2);
        if (viaHistoryMarkers_.find(objMIdx1) == viaHistoryMarkers_.end()) {
          gridGraph_.addMarkerCostVia(objMIdx1.x(), objMIdx1.y(), objMIdx1.z());
          viaHistoryMarkers_.insert(objMIdx1);

          vioNets.insert(obj->getNet());
        }
      }
    } else if (connFig->typeId() == drcPatchWire) {
      auto obj = static_cast<drPatchWire*>(connFig);
      obj->getOrigin(bp);
      // skip if unfixable obj
      if (!getRouteBox().contains(bp)) {
        continue;
      }
      // add history cost
      // gridGraph_.getMazeIdx(objMIdx1, bp, lNum);
      frBox patchBBox;
      obj->getBBox(patchBBox);
      frPoint patchLL = patchBBox.lowerLeft();
      frPoint patchUR = patchBBox.upperRight();
      FlexMazeIdx startIdx, endIdx;
      gridGraph_.getMazeIdx(startIdx, patchLL, lNum);
      gridGraph_.getMazeIdx(endIdx, patchUR, lNum);
      for (auto xIdx = startIdx.x(); xIdx <= endIdx.x(); xIdx++) {
        for (auto yIdx = startIdx.y(); yIdx <= endIdx.y(); yIdx++) {
          gridGraph_.addMarkerCostPlanar(xIdx, yIdx, startIdx.z());
          gridGraph_.addMarkerCost(
              xIdx,
              yIdx,
              startIdx.z(),
              frDirEnum::U);  // always block upper via in case stack via
          gridGraph_.addMarkerCost(
              xIdx,
              yIdx,
              startIdx.z(),
              frDirEnum::D);  // always block upper via in case stack via
          FlexMazeIdx objMIdx(xIdx, yIdx, startIdx.z());
          planarHistoryMarkers_.insert(objMIdx);
        }
      }
    }
  }
}

bool FlexDRWorker::initMazeCost_marker_fixMode_3_addHistoryCost1(
    const frMarker& marker)
{

  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;

  marker.getBBox(mBox);
  auto lNum = marker.getLayerNum();
  auto topLNum = getDesign()->getTech()->getTopLayerNum();
  frCoord width;
  frMIdx zIdx;
  if (getDesign()->getTech()->getLayer(lNum)->getType()
      == frLayerTypeEnum::CUT) {
    width = getDesign()->getTech()->getLayer(lNum - 1)->getWidth();
    zIdx = gridGraph_.getMazeZIdx(lNum - 1);
  } else {
    width = getDesign()->getTech()->getLayer(lNum)->getWidth();
    zIdx = gridGraph_.getMazeZIdx(lNum);
  }
  mBox.bloat(width * workerMarkerBloatWidth_, bloatBox);
  gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);

  frMIdx minZIdx = max(0, int(zIdx - workerMarkerBloatDepth_ / 2));
  frMIdx maxZIdx;
  if (getDesign()->getTech()->getLayer(topLNum)->getType()
      == frLayerTypeEnum::CUT) {
    maxZIdx = gridGraph_.getMazeZIdx(topLNum - 1);
  } else {
    maxZIdx = gridGraph_.getMazeZIdx(topLNum);
  }
  maxZIdx = min(int(maxZIdx), int(zIdx + workerMarkerBloatDepth_ / 2));

  for (int k = minZIdx; k <= maxZIdx; k++) {
    for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
      for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
        gridGraph_.addMarkerCostPlanar(i, j, k);
        gridGraph_.addMarkerCost(
            i, j, k, frDirEnum::U);  // always block upper via in case stack via
      }
    }
  }
  return true;
}

// return fixable
bool FlexDRWorker::initMazeCost_marker_fixMode_3_addHistoryCost(
    const frMarker& marker)
{

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;

  marker.getBBox(mBox);
  auto lNum = marker.getLayerNum();

  workerRegionQuery.query(mBox, lNum, results);
  frPoint bp, ep;
  frCoord width;
  frSegStyle segStyle;
  FlexMazeIdx objMIdx1, objMIdx2;
  bool fixable = false;
  for (auto& [objBox, connFig] : results) {
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      // skip if unfixable obj
      obj->getPoints(bp, ep);
      if (!(getRouteBox().contains(bp) && getRouteBox().contains(ep))) {
        continue;
      }
      fixable = true;
      // add history cost
      // get points to mark up, markup up to "width" grid points to the left and
      // right of pathseg
      obj->getStyle(segStyle);
      width = segStyle.getWidth();
      mBox.bloat(width, bloatBox);
      gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);

      obj->getMazeIdx(objMIdx1, objMIdx2);
      bool isH = (objMIdx1.y() == objMIdx2.y());
      // temporarily bloat marker at most 4X width to find neighboring grid
      // point block at least two width grid points to ensure no matter where
      // the startpoint is
      for (int i = 0; i < 5; i++) {
        if (i == 4) {
          cout << "Warning: marker bloat 4x width but could not find two grids "
                  "to add marker cost"
               << endl;
          cout << "  marker -- src: ";
          for (auto src : marker.getSrcs()) {
            if (src) {
              switch (src->typeId()) {
                case frcNet:
                  cout << (static_cast<frNet*>(src))->getName() << " ";
                  break;
                case frcInstTerm: {
                  frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
                  cout << instTerm->getInst()->getName() << "/"
                       << instTerm->getTerm()->getName() << " ";
                  break;
                }
                case frcTerm: {
                  frTerm* term = (static_cast<frTerm*>(src));
                  cout << "PIN/" << term->getName() << " ";
                  break;
                }
                case frcInstBlockage: {
                  frInstBlockage* instBlockage
                      = (static_cast<frInstBlockage*>(src));
                  cout << instBlockage->getInst()->getName() << "/OBS"
                       << " ";
                  break;
                }
                case frcBlockage: {
                  cout << "PIN/OBS"
                       << " ";
                  break;
                }
                default:;
              }
            }
            cout << "\n";
            // get violation bbox
            frBox bbox;
            marker.getBBox(bbox);
            double dbu = design_->getTech()->getDBUPerUU();
            cout << "    bbox = ( " << bbox.left() / dbu << ", "
                 << bbox.bottom() / dbu << " ) - ( " << bbox.right() / dbu
                 << ", " << bbox.top() / dbu << " ) on Layer ";
            if (getTech()->getLayer(marker.getLayerNum())->getType()
                    == frLayerTypeEnum::CUT
                && marker.getLayerNum() - 1 >= getTech()->getBottomLayerNum()) {
              cout << getTech()->getLayer(marker.getLayerNum() - 1)->getName()
                   << "\n";
            } else {
              cout << getTech()->getLayer(marker.getLayerNum())->getName()
                   << "\n";
            }
          }
        }
        if (isH
            && max(objMIdx1.x(), mIdx1.x()) >= min(objMIdx2.x(), mIdx2.x())) {
          bloatBox.bloat(width, bloatBox);
          // cout <<"i=" <<i <<" " <<width <<", " <<bloatBox <<endl;
        } else if ((!isH)
                   && max(objMIdx1.y(), mIdx1.y())
                          >= min(objMIdx2.y(), mIdx2.y())) {
          bloatBox.bloat(width, bloatBox);
          // cout <<bloatBox <<endl;
        } else {
          break;
        }
        gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);
      }
      if (isH) {
        for (int i = max(objMIdx1.x(), mIdx1.x());
             i <= min(objMIdx2.x(), mIdx2.x());
             i++) {
          gridGraph_.addMarkerCostPlanar(i, objMIdx1.y(), objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(i, objMIdx1.y(), objMIdx1.z()));
        }
      } else {
        for (int i = max(objMIdx1.y(), mIdx1.y());
             i <= min(objMIdx2.y(), mIdx2.y());
             i++) {
          gridGraph_.addMarkerCostPlanar(objMIdx1.x(), i, objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(objMIdx1.x(), i, objMIdx1.z()));
        }
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      obj->getOrigin(bp);
      // skip if unfixable obj
      if (!getRouteBox().contains(bp)) {
        continue;
      }
      fixable = true;
      // add history cost
      obj->getMazeIdx(objMIdx1, objMIdx2);
      gridGraph_.addMarkerCostVia(objMIdx1.x(), objMIdx1.y(), objMIdx1.z());
      viaHistoryMarkers_.insert(objMIdx1);
    } else if (connFig->typeId() == drcPatchWire) {
      auto obj = static_cast<drPatchWire*>(connFig);
      obj->getOrigin(bp);
      // skip if unfixable obj
      if (!getRouteBox().contains(bp)) {
        continue;
      }
      fixable = true;
      // add history cost
      gridGraph_.getMazeIdx(objMIdx1, bp, lNum);
      gridGraph_.addMarkerCostPlanar(objMIdx1.x(), objMIdx1.y(), objMIdx1.z());
      gridGraph_.addMarkerCost(
          objMIdx1.x(),
          objMIdx1.y(),
          objMIdx1.z(),
          frDirEnum::U);  // always block upper via in case stack via
      gridGraph_.addMarkerCost(
          objMIdx1.x(),
          objMIdx1.y(),
          objMIdx1.z(),
          frDirEnum::D);  // always block upper via in case stack via
      planarHistoryMarkers_.insert(objMIdx1);
    }
  }
  return fixable;
}

void FlexDRWorker::initMazeCost_marker_fixMode_3_ripupNets(
    const frMarker& marker)
{

  auto& workerRegionQuery = getWorkerRegionQuery();
  vector<rq_box_value_t<drConnFig*>> results;
  frBox mBox, bloatBox;
  FlexMazeIdx mIdx1, mIdx2;
  frPoint bp, ep;

  marker.getBBox(mBox);
  auto lNum = marker.getLayerNum();

  // ripup all nets within bloatbox
  frCoord bloatDist = 0;
  mBox.bloat(bloatDist, bloatBox);

  auto currLNum = lNum;
  results.clear();
  workerRegionQuery.query(bloatBox, currLNum, results);
  for (auto& [objBox, connFig] : results) {
    bool isEnter = false;
    if (getFixMode() == 3) {
      isEnter = true;
    }
    if (!isEnter) {
      continue;
    }
    // for pathseg-related marker, bloat marker by half width and add marker
    // cost planar
    if (connFig->typeId() == drcPathSeg) {
      // cout <<"@@pathseg" <<endl;
      auto obj = static_cast<drPathSeg*>(connFig);
      obj->getPoints(bp, ep);
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      auto dz = abs(bloatDist * (lNum - currLNum));
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy + dz * dz);
      connFig->getNet()->setRipup();
      // for via-related marker, add marker cost via
    } else if (connFig->typeId() == drcVia) {
      // cout <<"@@via" <<endl;
      //  update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      auto dz = abs(bloatDist * (lNum - currLNum));
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy + dz * dz);

      connFig->getNet()->setRipup();
    } else if (connFig->typeId() == drcPatchWire) {
      // update marker dist
      auto dx = max(
          max(objBox.left(), mBox.left()) - min(objBox.right(), mBox.right()),
          0);
      auto dy = max(
          max(objBox.bottom(), mBox.bottom()) - min(objBox.top(), mBox.top()),
          0);
      auto dz = abs(bloatDist * (lNum - currLNum));
      connFig->getNet()->updateMarkerDist(dx * dx + dy * dy + dz * dz);

      connFig->getNet()->setRipup();
    } else {
      cout << "Error: unsupported dr type" << endl;
    }
  }
  // }
}

// TODO: modify behavior for bloat width != 0
void FlexDRWorker::initMazeCost_marker_route_queue(const frMarker& marker)
{
  if (workerMarkerBloatWidth_) {
    initMazeCost_marker_route_queue_addHistoryCost(marker);
  } else {
    initMazeCost_marker_route_queue_addHistoryCost(marker);
  }
}

void FlexDRWorker::initMazeCost_marker_fixMode_3(const frMarker& marker)
{
  bool fixable = false;
  if (workerMarkerBloatWidth_) {
    fixable = initMazeCost_marker_fixMode_3_addHistoryCost1(marker);
  } else {
    fixable = initMazeCost_marker_fixMode_3_addHistoryCost(marker);
  }
  // skip non-fixable markers
  if (!fixable) {
    return;
  }
  initMazeCost_marker_fixMode_3_ripupNets(marker);
}

void FlexDRWorker::route_queue_resetRipup()
{
  for (auto& net : nets_) {
    net->resetNumReroutes();
  }
}

void FlexDRWorker::route_queue_markerCostDecay()
{
  for (auto it = planarHistoryMarkers_.begin();
       it != planarHistoryMarkers_.end();) {
    auto currIt = it;
    auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostPlanar(mi.x(), mi.y(), mi.z(), MARKERDECAY)) {
      planarHistoryMarkers_.erase(currIt);
    }
  }
  for (auto it = viaHistoryMarkers_.begin(); it != viaHistoryMarkers_.end();) {
    auto currIt = it;
    auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostVia(mi.x(), mi.y(), mi.z(), MARKERDECAY)) {
      viaHistoryMarkers_.erase(currIt);
    }
  }
}

void FlexDRWorker::route_queue_addMarkerCost(
    const vector<unique_ptr<frMarker>>& markers)
{
  if (getFixMode() < 9) {
    // if (true) {
    for (auto& uMarker : markers) {
      auto& marker = *(uMarker.get());
      initMazeCost_marker_fixMode_3(marker);
    }
  } else {
    for (auto& uMarker : markers) {
      auto& marker = *(uMarker.get());
      initMazeCost_marker_route_queue(marker);
    }
  }
}

// init from member markers
void FlexDRWorker::route_queue_addMarkerCost()
{
  if (getFixMode() < 9) {
    // if (true) {
    for (auto& marker : markers_) {
      initMazeCost_marker_fixMode_3(marker);
    }
  } else {
    for (auto& marker : markers_) {
      initMazeCost_marker_route_queue(marker);
    }
  }
}

void FlexDRWorker::route_queue_init_queue(queue<RouteQueueEntry>& rerouteQueue)
{
  set<frBlockObject*> uniqueVictims;
  set<frBlockObject*> uniqueAggressors;
  vector<RouteQueueEntry> checks;
  vector<RouteQueueEntry> routes;

  if (getRipupMode() == 0) {
    for (auto& marker : markers_) {
      route_queue_update_from_marker(
          &marker, uniqueVictims, uniqueAggressors, checks, routes);
    }
  } else if (getRipupMode() == 1 || getRipupMode() == 2) {
    // ripup all nets and clear objs here
    // nets are ripped up during initNets()
    vector<drNet*> ripupNets;
    for (auto& net : nets_) {
      ripupNets.push_back(net.get());
    }

    // sort nets
    mazeIterInit_sortRerouteNets(0, ripupNets);
    // if (routeBox.left() == 462000 && routeBox.bottom() == 81100) {
    //   cout << "@@@ debug nets:\n";
    //   for (auto &net: ripupNets) {
    //     cout << net->getFrNet()->getName() << "\n";
    //   }
    // }
    for (auto& net : ripupNets) {
      routes.push_back({net, 0, true});
      // reserve via because all nets are ripupped
      initMazeCost_via_helper(net, true);
      // no need to clear the net because route objs are not pushed to the net
      // (See FlexDRWorker::initNet)
    }
  } else {
    cout << "Error: unsupported ripup mode\n";
  }
  route_queue_update_queue(checks, routes, rerouteQueue);
}

void FlexDRWorker::route_queue_update_queue(
    const vector<RouteQueueEntry>& checks,
    const vector<RouteQueueEntry>& routes,
    queue<RouteQueueEntry>& rerouteQueue)
{
  for (auto& route : routes) {
    rerouteQueue.push(route);
  }
  for (auto& check : checks) {
    rerouteQueue.push(check);
  }
}

//*****************************************************************************************//
// EXPONENTIAL QUEUE SIZE IF NOT MAKE AGGRESSORS AND VICTIMS UNIQUE FOR A SET OF
// MARKERS!! // NET A --> PUSH ROUTE A --> PUSH CHECK A * 3 --> //
//        |                                      | //
//        ---------------------------------------- //
//*****************************************************************************************//
void FlexDRWorker::route_queue_update_from_marker(
    frMarker* marker,
    set<frBlockObject*>& uniqueVictims,
    set<frBlockObject*>& uniqueAggressors,
    vector<RouteQueueEntry>& checks,
    vector<RouteQueueEntry>& routes)
{

  vector<frBlockObject*> uniqueVictimOwners;     // to maintain order
  vector<frBlockObject*> uniqueAggressorOwners;  // to maintain order

  auto& markerAggressors = marker->getAggressors();
  set<frNet*> movableAggressorNets;
  set<frBlockObject*> movableAggressorOwners;

  int n_NDnets = 0, n_dNets = 0;
  if (design_->getTech()->hasNondefaultRules()) {
    for (auto& a : marker->getSrcs()) {
      if (a->typeId() == frcNet) {
        auto fNet = static_cast<frNet*>(a);
        if (getDRNets(fNet)) {
          for (auto dNet : *(getDRNets(fNet))) {
            if (!canRipup(dNet)) {
              continue;
            }
            if (dNet->hasNDR())
              n_NDnets++;
            else
              n_dNets++;
          }
        }
      }
    }
  }
  for (auto& aggressorPair : markerAggressors) {
    auto& aggressor = aggressorPair.first;
    if (aggressor && aggressor->typeId() == frcNet) {
      auto fNet = static_cast<frNet*>(aggressor);
      if (fNet->getType() == frNetEnum::frcNormalNet
          || fNet->getType() == frNetEnum::frcClockNet) {
        movableAggressorNets.insert(fNet);
        if (getDRNets(fNet)) {
          for (auto dNet : *(getDRNets(fNet))) {
            if (!canRipup(dNet)) {
              continue;
            }
            movableAggressorOwners.insert(aggressor);
          }
        }
      }
    }
  }

  // push movable aggressors for reroute and other srcs for drc checking
  bool hasRerouteNet = false;
  if (!movableAggressorNets.empty()) {
    for (auto& fNet : movableAggressorNets) {
      if (getDRNets(fNet)) {
        // int subNetIdx = -1;
        for (auto dNet : *(getDRNets(fNet))) {
          // subNetIdx++;
          if (!canRipup(dNet)) {
            continue;
          }
          // rerouteQueue.push_back(make_pair(dNet, make_pair(true,
          // dNet->getNumReroutes())));
          if (uniqueAggressors.find(fNet) == uniqueAggressors.end()) {
            uniqueAggressors.insert(fNet);
            uniqueAggressorOwners.push_back(fNet);
          }
          // cout << "push route1: " << dNet->getFrNet()->getName() <<
          // "(subNetIdx " << subNetIdx << "), NumReroutes = " <<
          // dNet->getNumReroutes() <<  "\n";
          hasRerouteNet = true;
        }
      }
    }
  }

  if (hasRerouteNet) {
    set<frBlockObject*> checkDRCOwners;
    for (auto& src : marker->getSrcs()) {
      if (movableAggressorOwners.find(src) == movableAggressorOwners.end()) {
        if (src) {
          checkDRCOwners.insert(src);
        }
      }
    }
    // push checkDRCOwners to queue for DRC
    for (auto& owner : checkDRCOwners) {
      // rerouteQueue.push_back(make_pair(owner, make_pair(false, -1)));
      if (uniqueVictims.find(owner) == uniqueVictims.end()) {
        uniqueVictims.insert(owner);
        uniqueVictimOwners.push_back(owner);
      }
    }
  } else {
    set<frBlockObject*> owners, otherOwners, routeOwners;
    auto& srcs = marker->getSrcs();
    for (auto& src : srcs) {
      if (src) {
        owners.insert(src);
      }
    }
    std::set_difference(owners.begin(),
                        owners.end(),
                        movableAggressorOwners.begin(),
                        movableAggressorOwners.end(),
                        std::inserter(otherOwners, otherOwners.end()));
    for (auto& owner : otherOwners) {
      if (owner && owner->typeId() == frcNet) {
        auto fNet = static_cast<frNet*>(owner);
        if (fNet->getType() == frNetEnum::frcNormalNet
            || fNet->getType() == frNetEnum::frcClockNet) {
          if (getDRNets(fNet)) {
            // int subNetIdx = -1;
            for (auto dNet : *(getDRNets(fNet))) {
              if (!canRipup(dNet)) {
                continue;
              }
              if (uniqueAggressors.find(fNet) == uniqueAggressors.end()) {
                uniqueAggressors.insert(fNet);
                uniqueAggressorOwners.push_back(fNet);
              }
              // cout << "push route2: " << dNet->getFrNet()->getName() <<
              // "(subNetIdx " << subNetIdx << "), NumReroutes = " <<
              // dNet->getNumReroutes() <<  "\n";
              routeOwners.insert(owner);
              hasRerouteNet = true;
            }
          }
        }
      }
    }
    if (hasRerouteNet) {
      set<frBlockObject*> checkDRCOwners;
      for (auto& src : marker->getSrcs()) {
        if (routeOwners.find(src) == routeOwners.end()) {
          if (src) {
            checkDRCOwners.insert(src);
          }
        }
      }
      // push checkDRCOwners to queue for DRC
      for (auto& owner : checkDRCOwners) {
        if (uniqueVictims.find(owner) == uniqueVictims.end()) {
          uniqueVictims.insert(owner);
          uniqueVictimOwners.push_back(owner);
        }
      }
    }
  }
  // add to victims and aggressors as appropriate
  for (auto& aggressorOwner : uniqueAggressorOwners) {
    if (aggressorOwner && aggressorOwner->typeId() == frcNet) {
      auto fNet = static_cast<frNet*>(aggressorOwner);
      if (fNet->getType() == frNetEnum::frcNormalNet
          || fNet->getType() == frNetEnum::frcClockNet) {
        if (getDRNets(fNet)) {
          for (auto dNet : *(getDRNets(fNet))) {
            if (!canRipup(dNet)) {
              continue;
            }
            if (dNet->hasNDR() && n_NDnets == 1 && n_dNets > 0) {
              if (dNet->getNdrRipupThresh() < NDR_NETS_RIPUP_THRESH) {
                dNet->incNdrRipupThresh();
                continue;
              }
              dNet->setNdrRipupThresh(0);
            }
            routes.push_back({dNet, dNet->getNumReroutes(), true});
          }
        }
      }
    }
  }

  for (auto& victimOwner : uniqueVictimOwners) {
    checks.push_back({victimOwner, -1, false});
  }
}

bool FlexDRWorker::canRipup(drNet* n)
{
  if (n->getNumReroutes() >= getMazeEndIter())
    return false;
  return true;
}

void FlexDRWorker::route_queue_update_queue(
    const vector<unique_ptr<frMarker>>& markers,
    queue<RouteQueueEntry>& rerouteQueue)
{
  set<frBlockObject*> uniqueVictims;
  set<frBlockObject*> uniqueAggressors;
  vector<RouteQueueEntry> checks;
  vector<RouteQueueEntry> routes;

  for (auto& uMarker : markers) {
    auto marker = uMarker.get();
    route_queue_update_from_marker(
        marker, uniqueVictims, uniqueAggressors, checks, routes);
  }

  route_queue_update_queue(checks, routes, rerouteQueue);
}

void FlexDRWorker::initMazeCost_marker()
{
  //  decay all existing mi
  //  old
  for (auto it = planarHistoryMarkers_.begin();
       it != planarHistoryMarkers_.end();) {
    auto currIt = it;
    auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostPlanar(mi.x(), mi.y(), mi.z(), MARKERDECAY)) {
      planarHistoryMarkers_.erase(currIt);
    }
  }
  for (auto it = viaHistoryMarkers_.begin(); it != viaHistoryMarkers_.end();) {
    auto currIt = it;
    auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostVia(mi.x(), mi.y(), mi.z(), MARKERDECAY)) {
      viaHistoryMarkers_.erase(currIt);
    }
  }
  // new
  // for (int i = 0; i < 3; i++) {
  //  frDirEnum dir = frDirEnum::UNKNOWN;
  //  switch(i) {
  //    case 0: dir = frDirEnum::E; break;
  //    case 1: dir = frDirEnum::N; break;
  //    case 2: dir = frDirEnum::U; break;
  //    default: ;
  //  }
  //  for (auto it = historyMarkers[i].begin(); it != historyMarkers[i].end();)
  //  {
  //    auto currIt = it;
  //    auto &mi = *currIt;
  //    ++it;
  //    if (gridGraph_.decayMarkerCost(mi.x(), mi.y(), mi.z(), dir,
  //    MARKERDECAY)) {
  //      historyMarkers[i].erase(currIt);
  //    }
  //  }
  //}
  // add new marker mi
  for (auto& marker : markers_) {
    switch (getFixMode()) {
      case 0:
        initMazeCost_marker_fixMode_0(marker);
        break;
      case 1:
        initMazeCost_marker_fixMode_1(marker, true);
        break;
      case 2:
        initMazeCost_marker_fixMode_1(marker, false);
        break;
      case 3:
      case 4:
      case 5:
        initMazeCost_marker_fixMode_3(marker);
        break;
      default:;
    }
  }
}

void FlexDRWorker::initMazeCost_guide_helper(drNet* net, bool isAdd)
{
  FlexMazeIdx mIdx1, mIdx2;
  frBox box, tmpBox;
  frLayerNum lNum;
  frMIdx z;
  for (auto& rect : net->getOrigGuides()) {
    rect.getBBox(tmpBox);
    // tmpBox.bloat(1000, box);
    tmpBox.bloat(0, box);
    lNum = rect.getLayerNum();
    z = gridGraph_.getMazeZIdx(lNum);
    gridGraph_.getIdxBox(mIdx1, mIdx2, box);
    if (isAdd) {
      gridGraph_.setGuide(mIdx1.x(), mIdx1.y(), mIdx2.x(), mIdx2.y(), z);
    } else {
      gridGraph_.resetGuide(mIdx1.x(), mIdx1.y(), mIdx2.x(), mIdx2.y(), z);
    }
  }
}

void FlexDRWorker::initMazeCost()
{
  // init Maze cost by snet shapes and blockages
  initMazeCost_fixedObj();
  initMazeCost_ap();
  initMazeCost_connFig();
  // init Maze Cost by planar access terms (prevent early wrongway / turn)
  initMazeCost_planarTerm();
}

// init maze cost for snet objs and blockages
void FlexDRWorker::initMazeCost_fixedObj()
{
  frRegionQuery::Objects<frBlockObject> result;
  frMIdx zIdx = 0;
  map<frNet*, set<frBlockObject*>> frNet2Terms;
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    bool isRoutingLayer = true;
    result.clear();
    if (getTech()->getLayer(layerNum)->getType() == frLayerTypeEnum::ROUTING) {
      isRoutingLayer = true;
      zIdx = gridGraph_.getMazeZIdx(layerNum);
    } else if (getTech()->getLayer(layerNum)->getType()
               == frLayerTypeEnum::CUT) {
      isRoutingLayer = false;
      if (getTech()->getBottomLayerNum() <= layerNum - 1
          && getTech()->getLayer(layerNum - 1)->getType()
                 == frLayerTypeEnum::ROUTING) {
        zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
      } else {
        continue;
      }
    } else {
      continue;
    }
    getRegionQuery()->query(getExtBox(), layerNum, result);
    // process blockage first, then unblock based on pin shape
    for (auto& [box, obj] : result) {
      if (obj->typeId() == frcBlockage) {
        if (isRoutingLayer) {
          // assume only routing layer
          modMinSpacingCostPlanar(box, zIdx, 3, true);
          modMinSpacingCostVia(box, zIdx, 3, true, false, true);
          modMinSpacingCostVia(box, zIdx, 3, false, false, true);
          modEolSpacingRulesCost(box, zIdx, 3);
          // block
          modBlockedPlanar(box, zIdx, true);
          modBlockedVia(box, zIdx, true);
        } else {
          modCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, false);
        }
      } else if (obj->typeId() == frcInstBlockage) {
        if (isRoutingLayer) {
          // assume only routing layer
          modMinSpacingCostPlanar(box, zIdx, 3, true);
          modMinSpacingCostVia(box, zIdx, 3, true, false, true);
          modMinSpacingCostVia(box, zIdx, 3, false, false, true);
          modEolSpacingRulesCost(box, zIdx, 3);
          // block
          modBlockedPlanar(box, zIdx, true);
          modBlockedVia(box, zIdx, true);
        } else {
          modCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, false);
        }
      }
    }
    for (auto& [box, obj] : result) {
      // term no bloat
      if (obj->typeId() == frcTerm) {
        frNet2Terms[static_cast<frTerm*>(obj)->getNet()].insert(obj);
      } else if (obj->typeId() == frcInstTerm) {
        frNet2Terms[static_cast<frInstTerm*>(obj)->getNet()].insert(obj);
        if (isRoutingLayer) {
          // unblock planar edge for obs over pin, ap will unblock via edge for
          // legal pin access
          modBlockedPlanar(box, zIdx, false);
          if (zIdx <= (VIA_ACCESS_LAYERNUM / 2 - 1)) {
            modMinSpacingCostPlanar(box, zIdx, 3, true);
            modEolSpacingRulesCost(box, zIdx, 3);
          }
        } else {
          modCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, false);
        }
        // snet
      } else if (obj->typeId() == frcPathSeg) {
        auto ps = static_cast<frPathSeg*>(obj);
        // assume only routing layer
        modMinSpacingCostPlanar(box, zIdx, 3);
        modMinSpacingCostVia(box, zIdx, 3, true, true);
        modMinSpacingCostVia(box, zIdx, 3, false, true);
        modEolSpacingRulesCost(box, zIdx, 3);
        // block for PDN (fixed obj)
        if (ps->getNet()->getType() == frNetEnum::frcPowerNet
            || ps->getNet()->getType() == frNetEnum::frcGroundNet) {
          modBlockedPlanar(box, zIdx, true);
          modBlockedVia(box, zIdx, true);
        }
        // snet
      } else if (obj->typeId() == frcVia) {
        if (isRoutingLayer) {
          // assume only routing layer
          modMinSpacingCostPlanar(box, zIdx, 3);
          modMinSpacingCostVia(box, zIdx, 3, true, false);
          modMinSpacingCostVia(box, zIdx, 3, false, false);
          modEolSpacingRulesCost(box, zIdx, 3);
        } else {
          auto via = static_cast<frVia*>(obj);
          modAdjCutSpacingCost_fixedObj(box, via);

          modCutSpacingCost(box, zIdx, 3);
          modInterLayerCutSpacingCost(box, zIdx, 3, true);
          modInterLayerCutSpacingCost(box, zIdx, 3, false);
        }
      }
    }
  }

  // assign terms to each subnet
  for (auto& [net, objs] : frNet2Terms) {
    // to remove once verify error will not be triggered
    if (owner2nets_.find(net) == owner2nets_.end()) {
      // cout << "Error: frNet with term(s) does not exist in owner2nets\n";
      // continue;
    } else {
      for (auto dNet : owner2nets_[net]) {
        dNet->setFrNetTerms(objs);
      }
    }
    initMazeCost_terms(objs, true);
  }
}

void FlexDRWorker::initMazeCost_terms(const set<frBlockObject*>& objs,
                                      bool isAddPathCost,
                                      bool isSkipVia)
{
  for (auto& obj : objs) {
    if (obj->typeId() == frcTerm) {
      auto term = static_cast<frTerm*>(obj);
      for (auto& uPin : term->getPins()) {
        auto pin = uPin.get();
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            frLayerNum layerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(layerNum)->getType()
                != frLayerTypeEnum::ROUTING) {
              continue;
            }
            frMIdx zIdx;
            frRect instPinRect(*rpinRect);
            frBox box;
            instPinRect.getBBox(box);

            bool isRoutingLayer = true;
            if (getTech()->getLayer(layerNum)->getType()
                == frLayerTypeEnum::ROUTING) {
              isRoutingLayer = true;
              zIdx = gridGraph_.getMazeZIdx(layerNum);
            } else if (getTech()->getLayer(layerNum)->getType()
                       == frLayerTypeEnum::CUT) {
              isRoutingLayer = false;
              if (getTech()->getBottomLayerNum() <= layerNum - 1
                  && getTech()->getLayer(layerNum - 1)->getType()
                         == frLayerTypeEnum::ROUTING) {
                zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
              } else {
                continue;
              }
            } else {
              continue;
            }

            int type = isAddPathCost ? 3 : 2;

            if (isRoutingLayer) {
              modMinSpacingCostPlanar(box, zIdx, type);
              if (!isSkipVia) {
                modMinSpacingCostVia(box, zIdx, type, true, false);
                modMinSpacingCostVia(box, zIdx, type, false, false);
              }
              modEolSpacingRulesCost(box, zIdx, type);
            } else {
              modCutSpacingCost(box, zIdx, type);
              modInterLayerCutSpacingCost(box, zIdx, type, true);
              modInterLayerCutSpacingCost(box, zIdx, type, false);
            }
          } else {
            cout << "Error: initMazeCost_terms unsupported pinFig\n";
          }
        }
      }

    } else if (obj->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(obj);
      auto inst = instTerm->getInst();
      frTransform xform;
      inst->getUpdatedXform(xform);

      for (auto& uPin : instTerm->getTerm()->getPins()) {
        auto pin = uPin.get();
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            frLayerNum layerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(layerNum)->getType()
                != frLayerTypeEnum::ROUTING) {
              continue;
            }
            frMIdx zIdx;
            frRect instPinRect(*rpinRect);
            instPinRect.move(xform);
            frBox box;
            instPinRect.getBBox(box);

            // add cost
            bool isRoutingLayer = true;
            if (getTech()->getLayer(layerNum)->getType()
                == frLayerTypeEnum::ROUTING) {
              isRoutingLayer = true;
              zIdx = gridGraph_.getMazeZIdx(layerNum);
            } else if (getTech()->getLayer(layerNum)->getType()
                       == frLayerTypeEnum::CUT) {
              isRoutingLayer = false;
              if (getTech()->getBottomLayerNum() <= layerNum - 1
                  && getTech()->getLayer(layerNum - 1)->getType()
                         == frLayerTypeEnum::ROUTING) {
                zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
              } else {
                continue;
              }
            } else {
              continue;
            }

            int type = isAddPathCost ? 3 : 2;

            if (isRoutingLayer) {
              modMinSpacingCostPlanar(box, zIdx, type);
              if (inst->getRefBlock()->getMacroClass()
                  == MacroClassEnum::BLOCK) {  // temp solution for ISPD19
                                               // benchmarks
                modCornerToCornerSpacing(box, zIdx, type);
              }
              if (!isSkipVia) {
                modMinSpacingCostVia(box, zIdx, type, true, false);
                modMinSpacingCostVia(box, zIdx, type, false, false);
              }
              modEolSpacingRulesCost(box, zIdx, type);
            } else {
              modCutSpacingCost(box, zIdx, type);
              modInterLayerCutSpacingCost(box, zIdx, type, true);
              modInterLayerCutSpacingCost(box, zIdx, type, false);
            }
            // temporary solution, only add cost around macro pins
            if (inst->getRefBlock()->getMacroClass() == MacroClassEnum::BLOCK
                || isPad(inst->getRefBlock()->getMacroClass())
                || inst->getRefBlock()->getMacroClass()
                       == MacroClassEnum::RING) {
              modMinimumcutCostVia(box, zIdx, type, true);
              modMinimumcutCostVia(box, zIdx, type, false);
            }
          } else {
            cout << "Error: initMazeCost_terms unsupported pinFig\n";
          }
        }
      }
    } else {
      cout << "Error: unexpected obj type in initMazeCost_terms\n";
    }
  }
}

void FlexDRWorker::initMazeCost_planarTerm()
{
  frRegionQuery::Objects<frBlockObject> result;
  frMIdx zIdx = 0;
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    result.clear();
    if (getTech()->getLayer(layerNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    zIdx = gridGraph_.getMazeZIdx(layerNum);
    getRegionQuery()->query(getExtBox(), layerNum, result);
    for (auto& [box, obj] : result) {
      // term no bloat
      if (obj->typeId() == frcTerm) {
        FlexMazeIdx mIdx1, mIdx2;
        gridGraph_.getIdxBox(mIdx1, mIdx2, box);
        bool isPinRectHorz
            = (box.right() - box.left()) > (box.top() - box.bottom());
        for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
          for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
            FlexMazeIdx mIdx(i, j, zIdx);
            gridGraph_.setBlocked(i, j, zIdx, frDirEnum::U);
            gridGraph_.setBlocked(i, j, zIdx, frDirEnum::D);
            if (isPinRectHorz) {
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::N);
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::S);
            } else {
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::W);
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::E);
            }
          }
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_connFig()
{
  int cnt = 0;
  for (auto& net : nets_) {
    for (auto& connFig : net->getExtConnFigs()) {
      addPathCost(connFig.get());
      cnt++;
    }
    for (auto& connFig : net->getRouteConnFigs()) {
      addPathCost(connFig.get());
      cnt++;
    }
  }
  // cout <<"init " <<cnt <<" connfig costs" <<endl;
}

void FlexDRWorker::initMazeCost_via_helper(drNet* net, bool isAddPathCost)
{
  unique_ptr<drVia> via = nullptr;
  frPoint bp;
  for (auto& pin : net->getPins()) {
    if (pin->getFrTerm() == nullptr) {
      continue;
    }
    // MACRO pin does not prefer via access
    // bool macroPinViaBlock = false;
    auto dPinTerm = pin->getFrTerm();
    if (dPinTerm->typeId() == frcInstTerm) {
      frInstTerm* instTerm = static_cast<frInstTerm*>(dPinTerm);
      frInst* inst = instTerm->getInst();
      if (inst->getRefBlock()->getMacroClass() == MacroClassEnum::BLOCK
          || isPad(inst->getRefBlock()->getMacroClass())
          || inst->getRefBlock()->getMacroClass() == MacroClassEnum::RING) {
        continue;
      }
    }

    drAccessPattern* minCostAP = nullptr;
    for (auto& ap : pin->getAccessPatterns()) {
      if (ap->hasAccessViaDef(frDirEnum::U)) {
        if (minCostAP == nullptr) {
          minCostAP = ap.get();
        }
        if (ap->getPinCost() < minCostAP->getPinCost()) {
          minCostAP = ap.get();
          if (ap->getPinCost() == 0) {
            break;
          }
        }
      }
    }

    if (!minCostAP) {
      continue;
    }

    minCostAP->getPoint(bp);
    frViaDef* viaDef = minCostAP->getAccessViaDef();
    via = make_unique<drVia>(viaDef);
    via->setOrigin(bp);
    via->addToNet(net);
    initMazeIdx_connFig(via.get());
    FlexMazeIdx bi, ei;
    via->getMazeIdx(bi, ei);
    if (isAddPathCost) {
      addPathCost(via.get());
    } else {
      subPathCost(via.get());
    }
  }
}

// TODO: replace l1Box / l2Box calculation with via get bounding box function
void FlexDRWorker::initMazeCost_minCut_helper(drNet* net, bool isAddPathCost)
{
  int modType = isAddPathCost ? 1 : 0;
  for (auto& connFig : net->getExtConnFigs()) {
    if (connFig->typeId() == drcVia) {
      auto via = static_cast<drVia*>(connFig.get());
      frBox l1Box, l2Box;
      frTransform xform;
      via->getTransform(xform);

      auto l1Num = via->getViaDef()->getLayer1Num();
      auto l1Fig = (via->getViaDef()->getLayer1Figs()[0].get());
      l1Fig->getBBox(l1Box);
      l1Box.transform(xform);
      modMinimumcutCostVia(l1Box, gridGraph_.getMazeZIdx(l1Num), modType, true);
      modMinimumcutCostVia(
          l1Box, gridGraph_.getMazeZIdx(l1Num), modType, false);

      auto l2Num = via->getViaDef()->getLayer2Num();
      auto l2Fig = (via->getViaDef()->getLayer2Figs()[0].get());
      l2Fig->getBBox(l2Box);
      l2Box.transform(xform);
      modMinimumcutCostVia(l2Box, gridGraph_.getMazeZIdx(l2Num), modType, true);
      modMinimumcutCostVia(
          l2Box, gridGraph_.getMazeZIdx(l2Num), modType, false);
    }
  }
}

void FlexDRWorker::initMazeCost_boundary_helper(drNet* net, bool isAddPathCost)
{
  // do not check same-net rules between ext and route objs to avoid pessimism
  for (auto& connFig : net->getExtConnFigs()) {
    if (isAddPathCost) {
      addPathCost(connFig.get());
    } else {
      subPathCost(connFig.get());
    }
  }
}

void FlexDRWorker::initFixedObjs()
{
  auto& extBox = getExtBox();
  box_t queryBox(point_t(extBox.left(), extBox.bottom()),
                 point_t(extBox.right(), extBox.top()));
  set<frBlockObject*> drcObjSet;
  // fixed obj
  int cnt = 0;
  for (auto layerNum = getDesign()->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    auto regionQuery = getDesign()->getRegionQuery();
    frRegionQuery::Objects<frBlockObject> queryResult;
    regionQuery->query(queryBox, layerNum, queryResult);
    for (auto& objPair : queryResult) {
      cnt++;
      if (drcObjSet.find(objPair.second) == drcObjSet.end()) {
        drcObjSet.insert(objPair.second);
        fixedObjs_.push_back(objPair.second);
      }
    }
  }
}

void FlexDRWorker::initMarkers()
{
  vector<frMarker*> result;
  getRegionQuery()->queryMarker(getDrcBox(),
                                result);  // get all markers within drc box
  for (auto mptr : result) {
    // check recheck， if true， then markers.clear(), set drWorker bit to check
    // drc at start
    if (mptr->getConstraint()->typeId()
        != frConstraintTypeEnum::frcRecheckConstraint) {
      markers_.push_back(*mptr);
    } else {
      needRecheck_ = true;
    }
  }
  setInitNumMarkers(getNumMarkers());
  if (getDRIter() <= 1) {
    if (getNumMarkers() == 0) {
      setInitNumMarkers(1);
    }
  }
}

void FlexDRWorker::init()
{
  // if initDR
  //   get all instterm/term for each net
  // else
  //   1. get all insterm/term based on begin/end of pathseg, via
  //   2. union and find
  //
  // using namespace std::chrono;
  initMarkers();
  if (isEnableDRC() && getDRIter() && getInitNumMarkers() == 0
      && !needRecheck_) {
    skipRouting_ = true;
  }
  if (skipRouting_) {
    return;
  }
  initFixedObjs();
  initNets();
  initGridGraph();
  initMazeIdx();
  initMazeCost();
}
