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

#include <iostream>

#include "db/drObj/drNet.h"
#include "dr/FlexDR.h"
#include "frProfileTask.h"
#include "gc/FlexGC_impl.h"

using namespace std;
using namespace fr;

gcNet* FlexGCWorker::Impl::getNet(frBlockObject* obj)
{
  bool isFloatingVDD = false;
  bool isFloatingVSS = false;
  frBlockObject* owner = nullptr;
  if (obj->typeId() == frcTerm) {
    auto term = static_cast<frTerm*>(obj);
    if (term->hasNet()) {
      owner = term->getNet();
    } else {
      if (term->getType() == frTermEnum::frcPowerTerm) {
        isFloatingVDD = true;
      } else if (term->getType() == frTermEnum::frcGroundTerm) {
        isFloatingVSS = true;
      }
      owner = obj;
    }
  } else if (obj->typeId() == frcInstTerm) {
    auto instTerm = static_cast<frInstTerm*>(obj);
    if (instTerm->hasNet()) {
      owner = instTerm->getNet();
    } else {
      if (instTerm->getTerm()->getType() == frTermEnum::frcPowerTerm) {
        isFloatingVDD = true;
      } else if (instTerm->getTerm()->getType() == frTermEnum::frcGroundTerm) {
        isFloatingVSS = true;
      }
      owner = obj;
    }
  } else if (obj->typeId() == frcInstBlockage || obj->typeId() == frcBlockage) {
    owner = obj;
  } else if (obj->typeId() == frcPathSeg || obj->typeId() == frcVia
             || obj->typeId() == frcPatchWire) {
    auto shape = static_cast<frShape*>(obj);
    if (shape->hasNet()) {
      owner = shape->getNet();
    } else {
      logger_->error(DRT, 37, "init_design_helper shape does not have net.");
    }
  } else if (obj->typeId() == drcPathSeg || obj->typeId() == drcVia
             || obj->typeId() == drcPatchWire) {
    auto shape = static_cast<drShape*>(obj);
    if (shape->hasNet()) {
      owner = shape->getNet()->getFrNet();
    } else {
      logger_->error(DRT, 38, "init_design_helper shape does not have dr net.");
    }
  } else {
    logger_->error(DRT, 39, "init_design_helper unsupported type.");
  }

  if (isFloatingVSS) {
    return nets_[0].get();
  } else if (isFloatingVDD) {
    return nets_[1].get();
  } else {
    auto it = owner2nets_.find(owner);
    gcNet* currNet = nullptr;
    if (it == owner2nets_.end()) {
      currNet = addNet(owner);
    } else {
      currNet = it->second;
    }
    return currNet;
  }
}

void FlexGCWorker::Impl::initObj(const frBox& box,
                                 frLayerNum layerNum,
                                 frBlockObject* obj,
                                 bool isFixed)
{
  auto currNet = getNet(obj);
  if (getTech()->getLayer(layerNum)->getType() == frLayerTypeEnum::CUT) {
    currNet->addRectangle(box, layerNum, isFixed);
  } else {
    currNet->addPolygon(box, layerNum, isFixed);
  }
}

bool FlexGCWorker::Impl::initDesign_skipObj(frBlockObject* obj)
{
  if (targetObj_) {
    if (targetObj_->typeId() == frcInst) {
      if (obj->typeId() == frcInstTerm
          && static_cast<frInstTerm*>(obj)->getInst() == targetObj_) {
        return false;
      } else if (obj->typeId() == frcInstBlockage
                 && static_cast<frInstBlockage*>(obj)->getInst()
                        == targetObj_) {
        return false;
      } else {
        return true;
      }
    } else if (targetObj_->typeId() == frcTerm) {
      if (obj->typeId() == frcTerm
          && static_cast<frTerm*>(obj) == static_cast<frTerm*>(targetObj_)) {
        return false;
      } else {
        return true;
      }
    } else {
      logger_->error(
          DRT, 40, "FlexGCWorker::initDesign_skipObj type not supported.");
    }
  } else {
    return false;
  }
  return false;
}

void FlexGCWorker::Impl::initDesign(const frDesign* design)
{
  if (ignoreDB_) {
    return;
  }

  if (design->getTech() != tech_) {
    logger_->error(DRT, 253, "Design and tech mismatch");
  }

  auto& extBox = getExtBox();
  box_t queryBox(point_t(extBox.left(), extBox.bottom()),
                 point_t(extBox.right(), extBox.top()));
  auto regionQuery = design->getRegionQuery();
  frRegionQuery::Objects<frBlockObject> queryResult;
  int cnt = 0;
  // init all non-dr objs from design
  for (auto i = 0; i <= getTech()->getTopLayerNum(); i++) {
    queryResult.clear();
    regionQuery->query(queryBox, i, queryResult);
    for (auto& [box, obj] : queryResult) {
      if (initDesign_skipObj(obj)) {
        continue;
      }
      initObj(box, i, obj, true);
      cnt++;
    }
  }
  // init all dr objs from design
  if (getDRWorker()) {
    return;
  }
  cnt = 0;
  for (auto i = getTech()->getBottomLayerNum();
       i <= getTech()->getTopLayerNum();
       i++) {
    queryResult.clear();
    regionQuery->queryDRObj(queryBox, i, queryResult);
    for (auto& [box, obj] : queryResult) {
      if (initDesign_skipObj(obj)) {
        continue;
      }
      initObj(box, i, obj, false);
      cnt++;
    }
  }
}

void FlexGCWorker::Impl::addPAObj(frConnFig* obj, frBlockObject* owner)
{
  auto it = owner2nets_.find(owner);
  gcNet* currNet = nullptr;
  if (it == owner2nets_.end()) {
    currNet = addNet(owner);
  } else {
    currNet = it->second;
  }

  frBox box;
  frTransform xform;
  frLayerNum layerNum;
  if (obj->typeId() == frcPathSeg) {
    auto pathSeg = static_cast<frPathSeg*>(obj);
    pathSeg->getBBox(box);
    currNet->addPolygon(box, pathSeg->getLayerNum(), false);
  } else if (obj->typeId() == frcVia) {
    auto via = static_cast<frVia*>(obj);
    layerNum = via->getViaDef()->getLayer1Num();
    via->getTransform(xform);
    for (auto& fig : via->getViaDef()->getLayer1Figs()) {
      fig->getBBox(box);
      box.transform(xform);
      currNet->addPolygon(box, layerNum, false);
    }
    // push cut layer rect
    layerNum = via->getViaDef()->getCutLayerNum();
    for (auto& fig : via->getViaDef()->getCutFigs()) {
      fig->getBBox(box);
      box.transform(xform);
      currNet->addRectangle(box, layerNum, false);
    }
    // push layer2 rect
    layerNum = via->getViaDef()->getLayer2Num();
    for (auto& fig : via->getViaDef()->getLayer2Figs()) {
      frBox bbox;
      fig->getBBox(box);
      box.transform(xform);
      currNet->addPolygon(box, layerNum, false);
    }
  } else if (obj->typeId() == frcPatchWire) {
    auto pwire = static_cast<frPatchWire*>(obj);
    pwire->getBBox(box);
    currNet->addPolygon(box, pwire->getLayerNum(), false);
  }
}
void addNonTaperedPatches(gcNet* gNet,
                          const std::vector<unique_ptr<drConnFig>>& figs)
{
  frBox box;
  for (auto& obj : figs) {
    if (obj->typeId() == drcPatchWire) {
      auto pwire = static_cast<drPatchWire*>(obj.get());
      pwire->getBBox(box);
      int z = pwire->getLayerNum() / 2 - 1;
      for (auto& nt : gNet->getNonTaperedRects(z)) {
        if (nt.overlaps(box)) {
          gNet->addNonTaperedRect(box, z);
          break;
        }
      }
    }
  }
}
void addNonTaperedPatches(gcNet* gNet, drNet* dNet)
{
  addNonTaperedPatches(gNet, dNet->getExtConnFigs());
  addNonTaperedPatches(gNet, dNet->getRouteConnFigs());
}

gcNet* FlexGCWorker::Impl::initDRObj(drConnFig* obj, gcNet* currNet)
{
  if (currNet == nullptr) {
    currNet = getNet(obj);
  }
  frBox box;
  frTransform xform;
  frLayerNum layerNum;
  if (obj->typeId() == drcPathSeg) {
    auto pathSeg = static_cast<drPathSeg*>(obj);
    pathSeg->getBBox(box);
    // debug
    // if (pathSeg->getLayerNum() == 4 && box.left() < 470300 && box.right() >
    // 470300 && box.bottom() < 100800 && box.top() > 100800) {
    //   cout << "  @@@ debug: initDRObj catches wire on M2 (";
    //   auto owner = currNet->getOwner();
    //   if (owner == nullptr) {
    //     cout <<" FLOATING";
    //   } else {
    //     if (owner->typeId() == frcNet) {
    //       cout <<static_cast<frNet*>(owner)->getName();
    //     } else if (owner->typeId() == frcInstTerm) {
    //       cout <<static_cast<frInstTerm*>(owner)->getInst()->getName() <<"/"
    //            <<static_cast<frInstTerm*>(owner)->getTerm()->getName();
    //     } else if (owner->typeId() == frcTerm) {
    //       cout <<"PIN/" <<static_cast<frTerm*>(owner)->getName();
    //     } else if (owner->typeId() == frcInstBlockage) {
    //       cout <<static_cast<frInstBlockage*>(owner)->getInst()->getName()
    //       <<"/OBS";
    //     } else if (owner->typeId() == frcBlockage) {
    //       cout <<"PIN/OBS";
    //     } else {
    //       cout <<"UNKNOWN";
    //     }
    //   }
    //   cout <<  ") (" << box.left() << ", " << box.bottom() << ") - (" <<
    //   box.right() << ", " << box.top() << ")\n";
    // }
    currNet->addPolygon(box, pathSeg->getLayerNum());
    if (pathSeg->isTapered())
      currNet->addTaperedRect(box, pathSeg->getLayerNum() / 2 - 1);
    else if (pathSeg->hasNet() && pathSeg->getNet()->hasNDR()
             && AUTO_TAPER_NDR_NETS)
      currNet->addNonTaperedRect(box, pathSeg->getLayerNum() / 2 - 1);
  } else if (obj->typeId() == drcVia) {
    auto via = static_cast<drVia*>(obj);
    layerNum = via->getViaDef()->getLayer1Num();
    via->getTransform(xform);
    for (auto& fig : via->getViaDef()->getLayer1Figs()) {
      fig->getBBox(box);
      box.transform(xform);
      if (via->isTapered())
        currNet->addTaperedRect(box, layerNum / 2 - 1);
      else if (via->hasNet() && via->getNet()->hasNDR() && AUTO_TAPER_NDR_NETS)
        currNet->addNonTaperedRect(box, layerNum / 2 - 1);
      currNet->addPolygon(box, layerNum);
    }
    // push cut layer rect
    layerNum = via->getViaDef()->getCutLayerNum();
    for (auto& fig : via->getViaDef()->getCutFigs()) {
      fig->getBBox(box);
      box.transform(xform);
      currNet->addRectangle(box, layerNum);
    }
    // push layer2 rect
    layerNum = via->getViaDef()->getLayer2Num();
    for (auto& fig : via->getViaDef()->getLayer2Figs()) {
      fig->getBBox(box);
      box.transform(xform);
      if (via->isTapered())
        currNet->addTaperedRect(box, layerNum / 2 - 1);
      else if (via->hasNet() && via->getNet()->hasNDR() && AUTO_TAPER_NDR_NETS)
        currNet->addNonTaperedRect(box, layerNum / 2 - 1);
      currNet->addPolygon(box, layerNum);
    }
  } else if (obj->typeId() == drcPatchWire) {
    auto pwire = static_cast<drPatchWire*>(obj);
    pwire->getBBox(box);
    currNet->addPolygon(box, pwire->getLayerNum());
  }
  return currNet;
}
// creates net polygons based on the net shapes
void FlexGCWorker::Impl::initDRWorker()
{
  if (!getDRWorker()) {
    return;
  }
  int cnt = 0;
  for (auto& uDRNet : getDRWorker()->getNets()) {
    // always first generate gcnet in case owner does not have any object
    auto it = owner2nets_.find(uDRNet->getFrNet());
    if (it == owner2nets_.end()) {
      addNet(uDRNet->getFrNet());
    }
    gcNet* gNet = nullptr;
    // auto net = uDRNet->getFrNet();
    for (auto& uConnFig : uDRNet->getExtConnFigs()) {
      gNet = initDRObj(uConnFig.get());
      cnt++;
    }
    for (auto& uConnFig : uDRNet->getRouteConnFigs()) {
      gNet = initDRObj(uConnFig.get());
      cnt++;
    }
    addNonTaperedPatches(gNet, uDRNet.get());
  }
}

void FlexGCWorker::Impl::initNet_pins_polygon(gcNet* net)
{
  int numLayers = getTech()->getLayers().size();
  // init pin from polygons
  vector<gtl::polygon_90_set_data<frCoord>> layerPolys(numLayers);
  vector<gtl::polygon_90_with_holes_data<frCoord>> polys;
  for (int i = 0; i < numLayers; i++) {
    polys.clear();
    using namespace gtl::operators;
    layerPolys[i] += net->getPolygons(i, false);
    layerPolys[i] += net->getPolygons(i, true);
    layerPolys[i].get(polys);
    for (auto& poly : polys) {
      net->addPin(poly, i);
    }
  }
  // init pin from rectangles
  for (int i = 0; i < numLayers; i++) {
    for (auto& rect : net->getRectangles(i, false)) {
      net->addPin(rect, i);
    }
    for (auto& rect : net->getRectangles(i, true)) {
      net->addPin(rect, i);
    }
  }
}

void FlexGCWorker::Impl::initNet_pins_polygonEdges_getFixedPolygonEdges(
    gcNet* net,
    vector<set<pair<frPoint, frPoint>>>& fixedPolygonEdges)
{
  int numLayers = getTech()->getLayers().size();
  vector<gtl::polygon_90_with_holes_data<frCoord>> polys;
  frPoint bp, ep, firstPt;
  // get fixed polygon edges from polygons
  for (int i = 0; i < numLayers; i++) {
    polys.clear();
    net->getPolygons(i, true).get(polys);
    for (auto& poly : polys) {
      // skip the first pt
      auto outerIt = poly.begin();
      bp.set((*outerIt).x(), (*outerIt).y());
      firstPt.set((*outerIt).x(), (*outerIt).y());
      outerIt++;
      // loop from second to last pt (n-1) edges
      for (; outerIt != poly.end(); outerIt++) {
        ep.set((*outerIt).x(), (*outerIt).y());
        fixedPolygonEdges[i].insert(make_pair(bp, ep));
        bp.set(ep);
      }
      // insert last edge
      fixedPolygonEdges[i].insert(make_pair(bp, firstPt));
      for (auto holeIt = poly.begin_holes(); holeIt != poly.end_holes();
           holeIt++) {
        auto& hole_poly = *holeIt;
        // skip the first pt
        auto innerIt = hole_poly.begin();
        bp.set((*innerIt).x(), (*innerIt).y());
        firstPt.set((*innerIt).x(), (*innerIt).y());
        innerIt++;
        // loop from second to last pt (n-1) edges
        for (; innerIt != hole_poly.end(); innerIt++) {
          ep.set((*innerIt).x(), (*innerIt).y());
          fixedPolygonEdges[i].insert(make_pair(bp, ep));
          bp.set(ep);
        }
        // insert last edge
        fixedPolygonEdges[i].insert(make_pair(bp, firstPt));
      }
    }
  }
  // for rectangles input --> non-merge scenario
  for (int i = 0; i < numLayers; i++) {
    for (auto& rect : net->getRectangles(i, true)) {
      fixedPolygonEdges[i].insert(
          make_pair(frPoint(gtl::xl(rect), gtl::yl(rect)),
                    frPoint(gtl::xh(rect), gtl::yl(rect))));
      fixedPolygonEdges[i].insert(
          make_pair(frPoint(gtl::xh(rect), gtl::yl(rect)),
                    frPoint(gtl::xh(rect), gtl::yh(rect))));
      fixedPolygonEdges[i].insert(
          make_pair(frPoint(gtl::xh(rect), gtl::yh(rect)),
                    frPoint(gtl::xl(rect), gtl::yh(rect))));
      fixedPolygonEdges[i].insert(
          make_pair(frPoint(gtl::xl(rect), gtl::yh(rect)),
                    frPoint(gtl::xl(rect), gtl::yl(rect))));
    }
  }
}

void FlexGCWorker::Impl::initNet_pins_polygonEdges_helper_outer(
    gcNet* net,
    gcPin* pin,
    gcPolygon* poly,
    frLayerNum i,
    const vector<set<pair<frPoint, frPoint>>>& fixedPolygonEdges)
{
  frPoint bp, ep, firstPt;
  gtl::point_data<frCoord> bp1, ep1, firstPt1;
  vector<unique_ptr<gcSegment>> tmpEdges;
  // skip the first pt
  auto outerIt = poly->begin();
  bp.set((*outerIt).x(), (*outerIt).y());
  bp1 = *outerIt;
  firstPt.set((*outerIt).x(), (*outerIt).y());
  firstPt1 = *outerIt;
  outerIt++;
  // loop from second to last pt (n-1) edges
  for (; outerIt != poly->end(); outerIt++) {
    ep.set((*outerIt).x(), (*outerIt).y());
    ep1 = *outerIt;
    auto edge = make_unique<gcSegment>();
    edge->setLayerNum(i);
    edge->addToPin(pin);
    edge->addToNet(net);
    // edge->setPoints(bp, ep);
    edge->setSegment(bp1, ep1);
    if (fixedPolygonEdges[i].find(make_pair(bp, ep))
        != fixedPolygonEdges[i].end()) {
      // fixed edge
      edge->setFixed(true);
      // cntFixed++;
    } else {
      // route edge;
      edge->setFixed(false);
      // cntRoute++;
    }
    if (!tmpEdges.empty()) {
      edge->setPrevEdge(tmpEdges.back().get());
      tmpEdges.back()->setNextEdge(edge.get());
    }
    tmpEdges.push_back(std::move(edge));
    bp.set(ep);
    bp1 = ep1;
    // cntOuter++;
  }
  // last edge
  auto edge = make_unique<gcSegment>();
  edge->setLayerNum(i);
  edge->addToPin(pin);
  edge->addToNet(net);
  // edge->setPoints(bp, firstPt);
  edge->setSegment(bp1, firstPt1);
  if (fixedPolygonEdges[i].find(make_pair(bp, firstPt))
      != fixedPolygonEdges[i].end()) {
    // fixed edge
    edge->setFixed(true);
    // cntFixed++;
  } else {
    // route edge;
    edge->setFixed(false);
    // cntRoute++;
  }
  edge->setPrevEdge(tmpEdges.back().get());
  tmpEdges.back()->setNextEdge(edge.get());
  // set first edge
  tmpEdges.front()->setPrevEdge(edge.get());
  edge->setNextEdge(tmpEdges.front().get());

  tmpEdges.push_back(std::move(edge));
  // add to polygon edges
  pin->addPolygonEdges(tmpEdges);
}

void FlexGCWorker::Impl::initNet_pins_polygonEdges_helper_inner(
    gcNet* net,
    gcPin* pin,
    const gtl::polygon_90_data<frCoord>& hole_poly,
    frLayerNum i,
    const vector<set<pair<frPoint, frPoint>>>& fixedPolygonEdges)
{
  frPoint bp, ep, firstPt;
  gtl::point_data<frCoord> bp1, ep1, firstPt1;
  vector<unique_ptr<gcSegment>> tmpEdges;
  // skip the first pt
  auto innerIt = hole_poly.begin();
  bp.set((*innerIt).x(), (*innerIt).y());
  bp1 = *innerIt;
  firstPt.set((*innerIt).x(), (*innerIt).y());
  firstPt1 = *innerIt;
  innerIt++;
  // loop from second to last pt (n-1) edges
  for (; innerIt != hole_poly.end(); innerIt++) {
    ep.set((*innerIt).x(), (*innerIt).y());
    ep1 = *innerIt;
    auto edge = make_unique<gcSegment>();
    edge->setLayerNum(i);
    edge->addToPin(pin);
    edge->addToNet(net);
    // edge->setPoints(bp, ep);
    edge->setSegment(bp1, ep1);
    if (fixedPolygonEdges[i].find(make_pair(bp, ep))
        != fixedPolygonEdges[i].end()) {
      // fixed edge
      edge->setFixed(true);
      // cntFixed++;
    } else {
      // route edge;
      edge->setFixed(false);
      // cntRoute++;
    }
    if (!tmpEdges.empty()) {
      edge->setPrevEdge(tmpEdges.back().get());
      tmpEdges.back()->setNextEdge(edge.get());
    }
    tmpEdges.push_back(std::move(edge));
    bp.set(ep);
    bp1 = ep1;
    // cntInner++;
  }
  auto edge = make_unique<gcSegment>();
  edge->setLayerNum(i);
  edge->addToPin(pin);
  edge->addToNet(net);
  // edge->setPoints(bp, firstPt);
  edge->setSegment(bp1, firstPt1);
  // last edge
  if (fixedPolygonEdges[i].find(make_pair(bp, firstPt))
      != fixedPolygonEdges[i].end()) {
    // fixed edge
    edge->setFixed(true);
    // cntFixed++;
  } else {
    // route edge;
    edge->setFixed(false);
    // cntRoute++;
  }
  edge->setPrevEdge(tmpEdges.back().get());
  tmpEdges.back()->setNextEdge(edge.get());
  // set first edge
  tmpEdges.front()->setPrevEdge(edge.get());
  edge->setNextEdge(tmpEdges.front().get());

  tmpEdges.push_back(std::move(edge));
  // add to polygon edges
  pin->addPolygonEdges(tmpEdges);
}

void FlexGCWorker::Impl::initNet_pins_polygonEdges(gcNet* net)
{
  int numLayers = getTech()->getLayers().size();
  vector<set<pair<frPoint, frPoint>>> fixedPolygonEdges(numLayers);
  // get all fixed polygon edges
  initNet_pins_polygonEdges_getFixedPolygonEdges(net, fixedPolygonEdges);

  // loop through all merged polygons and build mark edges
  for (int i = 0; i < numLayers; i++) {
    for (auto& pin : net->getPins(i)) {
      auto poly = pin->getPolygon();
      initNet_pins_polygonEdges_helper_outer(
          net, pin.get(), poly, i, fixedPolygonEdges);
      // pending
      for (auto holeIt = poly->begin_holes(); holeIt != poly->end_holes();
           holeIt++) {
        auto& hole_poly = *holeIt;
        initNet_pins_polygonEdges_helper_inner(
            net, pin.get(), hole_poly, i, fixedPolygonEdges);
      }
    }
  }
}

void FlexGCWorker::Impl::initNet_pins_polygonCorners_helper(gcNet* net,
                                                            gcPin* pin)
{
  for (auto& edges : pin->getPolygonEdges()) {
    vector<unique_ptr<gcCorner>> tmpCorners;
    auto prevEdge = edges.back().get();
    auto layerNum = prevEdge->getLayerNum();
    gcCorner* prevCorner = nullptr;
    for (int i = 0; i < (int) edges.size(); i++) {
      auto nextEdge = edges[i].get();
      auto uCurrCorner = std::make_unique<gcCorner>();
      auto currCorner = uCurrCorner.get();
      tmpCorners.push_back(std::move(uCurrCorner));
      // set edge attributes
      prevEdge->setHighCorner(currCorner);
      nextEdge->setLowCorner(currCorner);
      // set currCorner attributes
      currCorner->setPrevEdge(prevEdge);
      currCorner->setNextEdge(nextEdge);
      currCorner->x(prevEdge->high().x());
      currCorner->y(prevEdge->high().y());
      int orient = gtl::orientation(*prevEdge, *nextEdge);
      if (orient == 1) {
        currCorner->setType(frCornerTypeEnum::CONVEX);
      } else if (orient == -1) {
        currCorner->setType(frCornerTypeEnum::CONCAVE);
      } else {
        currCorner->setType(frCornerTypeEnum::UNKNOWN);
      }

      if ((prevEdge->getDir() == frDirEnum::N
           && nextEdge->getDir() == frDirEnum::W)
          || (prevEdge->getDir() == frDirEnum::W
              && nextEdge->getDir() == frDirEnum::N)) {
        currCorner->setDir(frCornerDirEnum::NE);
      } else if ((prevEdge->getDir() == frDirEnum::W
                  && nextEdge->getDir() == frDirEnum::S)
                 || (prevEdge->getDir() == frDirEnum::S
                     && nextEdge->getDir() == frDirEnum::W)) {
        currCorner->setDir(frCornerDirEnum::NW);
      } else if ((prevEdge->getDir() == frDirEnum::S
                  && nextEdge->getDir() == frDirEnum::E)
                 || (prevEdge->getDir() == frDirEnum::E
                     && nextEdge->getDir() == frDirEnum::S)) {
        currCorner->setDir(frCornerDirEnum::SW);
      } else if ((prevEdge->getDir() == frDirEnum::E
                  && nextEdge->getDir() == frDirEnum::N)
                 || (prevEdge->getDir() == frDirEnum::N
                     && nextEdge->getDir() == frDirEnum::E)) {
        currCorner->setDir(frCornerDirEnum::SE);
      }

      // set fixed / route status
      if (currCorner->getType() == frCornerTypeEnum::CONVEX) {
        currCorner->setFixed(false);
        for (auto& rect : net->getRectangles(true)[layerNum]) {
          if (isCornerOverlap(currCorner, rect)) {
            currCorner->setFixed(true);
            break;
          }
        }
      } else if (currCorner->getType() == frCornerTypeEnum::CONCAVE) {
        currCorner->setFixed(true);
        auto cornerPt = currCorner->getNextEdge()->low();
        for (auto& rect : net->getRectangles(false)[layerNum]) {
          if (gtl::contains(rect, cornerPt, true)
              && !gtl::contains(rect, cornerPt, false)) {
            currCorner->setFixed(false);
            break;
          }
        }
      }
      // currCorner->setFixed(prevEdge->isFixed() && nextEdge->isFixed());

      if (prevCorner) {
        prevCorner->setNextCorner(currCorner);
        currCorner->setPrevCorner(prevCorner);
      }
      prevCorner = currCorner;
      prevEdge = nextEdge;
    }
    // update attributes between first and last corners
    auto currCorner = tmpCorners.front().get();
    prevCorner->setNextCorner(currCorner);
    currCorner->setPrevCorner(prevCorner);
    // add to polygon corners
    pin->addPolygonCorners(tmpCorners);
  }
}

void FlexGCWorker::Impl::initNet_pins_polygonCorners(gcNet* net)
{
  int numLayers = getTech()->getLayers().size();
  for (int i = 0; i < numLayers; i++) {
    for (auto& pin : net->getPins(i)) {
      initNet_pins_polygonCorners_helper(net, pin.get());
    }
  }
}

void FlexGCWorker::Impl::initNet_pins_maxRectangles_getFixedMaxRectangles(
    gcNet* net,
    vector<set<pair<frPoint, frPoint>>>& fixedMaxRectangles)
{
  int numLayers = getTech()->getLayers().size();
  vector<gtl::rectangle_data<frCoord>> rects;
  frPoint bp, ep;
  for (int i = 0; i < numLayers; i++) {
    rects.clear();
    gtl::get_max_rectangles(rects, net->getPolygons(i, true));
    for (auto& rect : rects) {
      fixedMaxRectangles[i].insert(
          make_pair(frPoint(gtl::xl(rect), gtl::yl(rect)),
                    frPoint(gtl::xh(rect), gtl::yh(rect))));
    }
    // for rectangles input --> non-merge scenario
    for (auto& rect : net->getRectangles(i, true)) {
      fixedMaxRectangles[i].insert(
          make_pair(frPoint(gtl::xl(rect), gtl::yl(rect)),
                    frPoint(gtl::xh(rect), gtl::yh(rect))));
    }
  }
}

void FlexGCWorker::Impl::initNet_pins_maxRectangles_helper(
    gcNet* net,
    gcPin* pin,
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum i,
    const vector<set<pair<frPoint, frPoint>>>& fixedMaxRectangles)
{
  auto rectangle = make_unique<gcRect>();
  rectangle->setRect(rect);
  rectangle->setLayerNum(i);
  rectangle->addToPin(pin);
  rectangle->addToNet(net);
  if (fixedMaxRectangles[i].find(
          make_pair(frPoint(gtl::xl(rect), gtl::yl(rect)),
                    frPoint(gtl::xh(rect), gtl::yh(rect))))
      != fixedMaxRectangles[i].end()) {
    // fixed max rectangles
    rectangle->setFixed(true);
    // cntFixed++;
  } else {
    // route max rectangles
    rectangle->setFixed(false);
    // cntRoute++;
    int k = i / 2 - 1;
    for (auto& r : net->getTaperedRects(k)) {
      if (rectangle->intersects(r)) {
        rectangle->setTapered(true);
        for (auto& nt : net->getNonTaperedRects(k)) {
          if (rectangle->intersects(nt)) {
            net->addSpecialSpcRect(
                nt, i, rectangle->getPin(), rectangle->getNet());
          }
        }
        break;
      }
    }
  }
  pin->addMaxRectangle(std::move(rectangle));
}

void FlexGCWorker::Impl::initNet_pins_maxRectangles(gcNet* net)
{
  int numLayers = getTech()->getLayers().size();
  vector<set<pair<frPoint, frPoint>>> fixedMaxRectangles(numLayers);
  // get all fixed max rectangles
  initNet_pins_maxRectangles_getFixedMaxRectangles(net, fixedMaxRectangles);

  // gen all max rectangles
  vector<gtl::rectangle_data<frCoord>> rects;
  for (int i = 0; i < numLayers; i++) {
    for (auto& pin : net->getPins(i)) {
      rects.clear();
      gtl::get_max_rectangles(rects, *(pin->getPolygon()));
      for (auto& rect : rects) {
        initNet_pins_maxRectangles_helper(
            net, pin.get(), rect, i, fixedMaxRectangles);
      }
    }
  }
}

void FlexGCWorker::Impl::initNet(gcNet* net)
{
  initNet_pins_polygon(net);
  initNet_pins_polygonEdges(net);
  initNet_pins_polygonCorners(net);
  initNet_pins_maxRectangles(net);
}

void FlexGCWorker::Impl::initNets()
{
  for (auto& uNet : getNets()) {
    auto net = uNet.get();
    initNet(net);
  }
}

void FlexGCWorker::Impl::initRegionQuery()
{
  getWorkerRegionQuery().init(getTech()->getLayers().size());
}

// init initializes all nets from frDesign if no drWorker is provided
void FlexGCWorker::Impl::init(const frDesign* design)
{
  ProfileTask profile("GC:init");
  addNet(design->getTopBlock()->getFakeVSSNet());  //[0] floating VSS
  addNet(design->getTopBlock()->getFakeVDDNet());  //[1] floating VDD
  initDesign(design);
  initDRWorker();
  initNets();
  initRegionQuery();
}

// init initializes all nets from frDesign if no drWorker is provided
void FlexGCWorker::Impl::initPA0(const frDesign* design)
{
  addNet(design->getTopBlock()->getFakeVSSNet());  //[0] floating VSS
  addNet(design->getTopBlock()->getFakeVDDNet());  //[1] floating VDD
  initDesign(design);
  initDRWorker();
}

void FlexGCWorker::Impl::initPA1()
{
  initNets();
  initRegionQuery();
}

void FlexGCWorker::Impl::updateGCWorker()
{
  if (!getDRWorker()) {
    cout << "Error: updateGCWorker expects a valid DRWorker" << endl;
    exit(1);
  }

  // get all frNets, must be sorted by id
  set<frNet*, frBlockObjectComp> fnets;
  for (auto dnet : modifiedDRNets_) {
    fnets.insert(dnet->getFrNet());
  }
  modifiedDRNets_.clear();

  // get GC patched net as well
  for (auto& patch : pwires_) {
    if (patch->hasNet() && patch->getNet()->getFrNet()) {
      fnets.insert(patch->getNet()->getFrNet());
    }
  }

  // start init from dr objs
  for (auto fnet : fnets) {
    auto net = owner2nets_[fnet];
    getWorkerRegionQuery().removeFromRegionQuery(
        net);      // delete all region queries
    net->clear();  // delete all pins and routeXXX
    // re-init gcnet from drobjs
    auto vecptr = getDRWorker()->getDRNets(fnet);
    if (vecptr) {
      for (auto dnet : *vecptr) {
        // initialize according to drnets
        for (auto& uConnFig : dnet->getExtConnFigs()) {
          initDRObj(uConnFig.get(), net);
        }
        for (auto& uConnFig : dnet->getRouteConnFigs()) {
          initDRObj(uConnFig.get(), net);
        }
        addNonTaperedPatches(net, dnet);
      }
    } else {
      logger_->error(DRT, 53, "updateGCWorker cannot find frNet in DRWorker.");
    }
  }

  // start init from GC patches
  for (auto& patch : pwires_) {
    if (patch->hasNet() && patch->getNet()->getFrNet()) {
      auto fnet = patch->getNet()->getFrNet();
      auto net = owner2nets_[fnet];
      initDRObj(patch.get(), net);
    }
  }

  // init
  for (auto fnet : fnets) {
    auto net = owner2nets_[fnet];
    // init gc net
    initNet(net);
    getWorkerRegionQuery().addToRegionQuery(net);
  }
}

void FlexGCWorker::updateDRNet(drNet* net)
{
  impl_->modifiedDRNets_.push_back(net);
}
