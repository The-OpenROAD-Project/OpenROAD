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

#include "dr/FlexDR.h"

using namespace std;
using namespace fr;

void FlexDRWorker::endGetModNets(set<frNet*, frBlockObjectComp>& modNets)
{
  for (auto& net : nets_) {
    if (net->isModified()) {
      modNets.insert(net->getFrNet());
    }
  }
  // change modified flag to true if another subnet get routed
  for (auto& net : nets_) {
    if (!net->isModified() && modNets.find(net->getFrNet()) != modNets.end()) {
      net->setModified(true);
    }
  }
}

void FlexDRWorker::endRemoveNets_pathSeg(frDesign* design,
                                         frPathSeg* pathSeg,
                                         set<pair<Point, frLayerNum>>& boundPts)
{
  auto [begin, end] = pathSeg->getPoints();
  auto routeBox = getRouteBox();
  auto net = pathSeg->getNet();
  auto regionQuery = design->getRegionQuery();
  // vertical seg
  if (begin.x() == end.x()) {
    // if cross routeBBox
    // initDR must merge on top and right boundaries
    // -------------------
    // |    routebox     |
    // |                 |---------------wire------------
    // |                 |
    // |                 |
    // -------------------
    if (isInitDR()
        && (begin.x() == routeBox.xMin() || begin.x() == routeBox.xMax())) {
      if (begin.y() < routeBox.yMin() || end.y() > routeBox.yMax()
          || pathSeg->getBeginStyle() != frcTruncateEndStyle
          || pathSeg->getEndStyle() != frcTruncateEndStyle)
        return;
    }
    bool condition2 = (begin.y() <= routeBox.yMax());  // orthogonal to wire
    if (routeBox.xMin() <= begin.x() && begin.x() <= routeBox.xMax()
        && !(begin.y() > routeBox.yMax() || end.y() < routeBox.yMin())) {
      // bottom seg to ext
      if (begin.y() < routeBox.yMin()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        Point boundPt(end.x(), min(end.y(), routeBox.yMin()));
        uPathSeg->setPoints(begin, boundPt);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (end.y() > routeBox.yMin()) {
          frSegStyle style = pathSeg->getStyle();
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }
        if (save_updates_) {
          drUpdate update(drUpdate::ADD_SHAPE);
          update.setNet(net);
          update.setPathSeg(*ps);
          design_->addUpdate(update);
        }
        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.y() >= routeBox.yMin()) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      // top seg to ext
      if (end.y() > routeBox.yMax()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        Point boundPt(begin.x(), max(begin.y(), routeBox.yMax()));
        uPathSeg->setPoints(boundPt, end);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (begin.y() < routeBox.yMax()) {
          frSegStyle style = pathSeg->getStyle();
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }
        if (save_updates_) {
          drUpdate update(drUpdate::ADD_SHAPE);
          update.setNet(net);
          update.setPathSeg(*ps);
          design_->addUpdate(update);
        }
        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary piont
        if (condition2) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_NET);
        update.setNet(net);
        update.setIndexInOwner(pathSeg->getIndexInOwner());
        design_->addUpdate(update);
      }
      regionQuery->removeDRObj(pathSeg);  // delete rq
      net->removeShape(pathSeg);          // delete segment
    }
    // horizontal seg
  } else if (begin.y() == end.y()) {
    if (isInitDR()
        && (begin.y() == routeBox.yMin() || begin.y() == routeBox.yMax())) {
      if (begin.x() < routeBox.xMin() || end.x() > routeBox.xMax()
          || pathSeg->getBeginStyle() != frcTruncateEndStyle
          || pathSeg->getEndStyle() != frcTruncateEndStyle)
        return;
    }
    // if cross routeBBox
    bool condition2 = /*isInitDR() ? (begin.x() < routeBox.xMax()):*/ (
        begin.x() <= routeBox.xMax());  // orthogonal to wire
    if (routeBox.yMin() <= begin.y() && begin.y() <= routeBox.yMax()
        && !(begin.x() > routeBox.xMax() || end.x() < routeBox.xMin())) {
      // left seg to ext
      if (begin.x() < routeBox.xMin()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        Point boundPt(min(end.x(), routeBox.xMin()), end.y());
        uPathSeg->setPoints(begin, boundPt);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (end.x() > routeBox.xMin()) {
          frSegStyle style = pathSeg->getStyle();
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }
        if (save_updates_) {
          drUpdate update(drUpdate::ADD_SHAPE);
          update.setNet(net);
          update.setPathSeg(*ps);
          design_->addUpdate(update);
        }
        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.x() >= routeBox.xMin()) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      // right seg to ext
      if (end.x() > routeBox.xMax()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        Point boundPt(max(begin.x(), routeBox.xMax()), begin.y());
        uPathSeg->setPoints(boundPt, end);
        // change boundary style to ext if orig pathSeg crosses at boundary
        if (begin.x() < routeBox.xMax()) {
          frSegStyle style = pathSeg->getStyle();
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }
        if (save_updates_) {
          drUpdate update(drUpdate::ADD_SHAPE);
          update.setNet(net);
          update.setPathSeg(*ps);
          design_->addUpdate(update);
        }
        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (condition2) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_NET);
        update.setNet(net);
        update.setIndexInOwner(pathSeg->getIndexInOwner());
        design_->addUpdate(update);
      }
      regionQuery->removeDRObj(pathSeg);  // delete rq
      net->removeShape(pathSeg);          // delete segment
    }
  }
}

void FlexDRWorker::endRemoveNets_via(frDesign* design, frVia* via)
{
  if (isRouteVia(via)) {
    auto net = via->getNet();
    if (save_updates_) {
      drUpdate update(drUpdate::REMOVE_FROM_NET);
      update.setNet(net);
      update.setIndexInOwner(via->getIndexInOwner());
      design_->addUpdate(update);
    }
    auto regionQuery = design->getRegionQuery();
    regionQuery->removeDRObj(via);  // delete rq
    net->removeVia(via);
  }
}

void FlexDRWorker::endRemoveNets_patchWire(frDesign* design, frPatchWire* pwire)
{
  if (isRoutePatchWire(pwire)) {
    auto net = pwire->getNet();
    if (save_updates_) {
      drUpdate update(drUpdate::REMOVE_FROM_NET);
      update.setNet(net);
      update.setIndexInOwner(pwire->getIndexInOwner());
      design_->addUpdate(update);
    }
    auto regionQuery = design->getRegionQuery();
    regionQuery->removeDRObj(pwire);  // delete rq
    net->removePatchWire(pwire);
  }
}

void FlexDRWorker::endRemoveNets(
    frDesign* design,
    set<frNet*, frBlockObjectComp>& modNets,
    map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp>& boundPts)
{
  vector<frBlockObject*> result;
  design->getRegionQuery()->queryDRObj(getExtBox(), result);
  for (auto rptr : result) {
    if (rptr->typeId() == frcPathSeg) {
      auto cptr = static_cast<frPathSeg*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_pathSeg(design, cptr, boundPts[cptr->getNet()]);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcVia) {
      auto cptr = static_cast<frVia*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_via(design, cptr);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcPatchWire) {
      auto cptr = static_cast<frPatchWire*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_patchWire(design, cptr);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else {
      cout << "Error: endRemoveNets unsupported type" << endl;
    }
  }
}

void FlexDRWorker::endAddNets_pathSeg(frDesign* design, drPathSeg* pathSeg)
{
  auto net = pathSeg->getNet()->getFrNet();
  unique_ptr<frShape> uShape = make_unique<frPathSeg>(*pathSeg);
  auto rptr = uShape.get();
  net->addShape(std::move(uShape));
  design->getRegionQuery()->addDRObj(rptr);
  if (save_updates_) {
    drUpdate update(drUpdate::ADD_SHAPE);
    update.setNet(net);
    update.setPathSeg(*pathSeg);
    design_->addUpdate(update);
  }
}

void FlexDRWorker::endAddNets_via(frDesign* design, drVia* via)
{
  auto net = via->getNet()->getFrNet();
  unique_ptr<frVia> uVia = make_unique<frVia>(*via);
  auto rptr = uVia.get();
  net->addVia(std::move(uVia));
  design->getRegionQuery()->addDRObj(rptr);
  if (save_updates_) {
    drUpdate update(drUpdate::ADD_SHAPE);
    update.setNet(net);
    update.setVia(*via);
    design_->addUpdate(update);
  }
}

void FlexDRWorker::endAddNets_patchWire(frDesign* design, drPatchWire* pwire)
{
  auto net = pwire->getNet()->getFrNet();
  unique_ptr<frShape> uShape = make_unique<frPatchWire>(*pwire);
  auto rptr = uShape.get();
  net->addPatchWire(std::move(uShape));
  design->getRegionQuery()->addDRObj(rptr);
  if (save_updates_) {
    drUpdate update(drUpdate::ADD_SHAPE);
    update.setNet(net);
    update.setPatchWire(*pwire);
    design_->addUpdate(update);
  }
}

void FlexDRWorker::endAddNets_merge(frDesign* design,
                                    frNet* net,
                                    set<pair<Point, frLayerNum>>& boundPts)
{
  frRegionQuery::Objects<frBlockObject> result;
  vector<frBlockObject*> drObjs;
  vector<frPathSeg*> horzPathSegs;
  vector<frPathSeg*> vertPathSegs;
  bool hasPatchMetal = false;
  auto regionQuery = design->getRegionQuery();
  for (auto& [pt, lNum] : boundPts) {
    hasPatchMetal = false;
    bool skip = false;
    result.clear();
    regionQuery->query(Rect(pt, pt), lNum, result);
    for (auto& [bx, obj] : result) {
      auto type = obj->typeId();
      if (type == frcInstTerm) {
        auto instTerm = static_cast<frInstTerm*>(obj);
        if (instTerm->getNet() == net) {
          skip = true;
          break;
        }
      } else if (type == frcBTerm) {
        auto term = static_cast<frBTerm*>(obj);
        if (term->getNet() == net) {
          skip = true;
          break;
        }
      }
    }
    // always merge offGrid boundary points
    frCoord manuGrid = getTech()->getManufacturingGrid();
    if (pt.x() % manuGrid || pt.y() % manuGrid) {
      skip = false;
    }

    if (skip) {
      continue;
    }
    // get all drObjs
    drObjs.clear();
    horzPathSegs.clear();
    vertPathSegs.clear();
    regionQuery->queryDRObj(Rect(pt, pt), lNum, drObjs);
    for (auto& obj : drObjs) {
      if (obj->typeId() == frcPathSeg) {
        auto ps = static_cast<frPathSeg*>(obj);
        if (!(ps->getNet() == net)) {
          continue;
        }
        auto [bp, ep] = ps->getPoints();
        if (ps->intersectsCenterLine(pt)) {
          // vertical
          if (bp.x() == ep.x()) {
            vertPathSegs.push_back(ps);
            // horizontal
          } else {
            horzPathSegs.push_back(ps);
          }
        }
      } else if (obj->typeId() == frcPatchWire) {
        auto pwire = static_cast<frPatchWire*>(obj);
        if (!(pwire->getNet() == net)) {
          continue;
        }
        Point bp = pwire->getOrigin();
        if (bp == pt) {
          hasPatchMetal = true;
          break;
        }
      }
    }
    // merge horz pathseg
    if ((int) horzPathSegs.size() == 2 && vertPathSegs.empty() && !hasPatchMetal
        && horzPathSegs[0]->isTapered() == horzPathSegs[1]->isTapered()) {
      unique_ptr<frShape> uShape = make_unique<frPathSeg>(*horzPathSegs[0]);
      auto rptr = static_cast<frPathSeg*>(uShape.get());
      auto [bp1, ep1] = horzPathSegs[0]->getPoints();
      auto [bp2, ep2] = horzPathSegs[1]->getPoints();
      Point bp(min(bp1.x(), bp2.x()), bp1.y());
      Point ep(max(ep1.x(), ep2.x()), ep1.y());
      rptr->setPoints(bp, ep);

      frSegStyle style = horzPathSegs[0]->getStyle();
      frSegStyle style_1 = horzPathSegs[1]->getStyle();
      if (bp1.x() < bp2.x()) {
        style.setEndStyle(style_1.getEndStyle(), style_1.getEndExt());
      } else {
        style.setBeginStyle(style_1.getBeginStyle(), style_1.getBeginExt());
      }
      rptr->setStyle(style);
      if (save_updates_) {
        drUpdate update1(drUpdate::REMOVE_FROM_NET),
            update2(drUpdate::REMOVE_FROM_NET);
        update1.setNet(net);
        update2.setNet(net);
        update1.setIndexInOwner(horzPathSegs[0]->getIndexInOwner());
        update2.setIndexInOwner(horzPathSegs[1]->getIndexInOwner());
        design_->addUpdate(update1);
        design_->addUpdate(update2);
      }
      regionQuery->removeDRObj(horzPathSegs[0]);
      regionQuery->removeDRObj(horzPathSegs[1]);
      net->removeShape(horzPathSegs[0]);
      net->removeShape(horzPathSegs[1]);
      if (save_updates_) {
        drUpdate update3(drUpdate::ADD_SHAPE);
        update3.setNet(net);
        update3.setPathSeg(*rptr);
        design_->addUpdate(update3);
      }
      net->addShape(std::move(uShape));
      regionQuery->addDRObj(rptr);
    }
    if ((int) vertPathSegs.size() == 2 && horzPathSegs.empty() && !hasPatchMetal
        && vertPathSegs[0]->isTapered() == vertPathSegs[1]->isTapered()) {
      unique_ptr<frShape> uShape = make_unique<frPathSeg>(*vertPathSegs[0]);
      auto rptr = static_cast<frPathSeg*>(uShape.get());
      auto [bp1, ep1] = vertPathSegs[0]->getPoints();
      auto [bp2, ep2] = vertPathSegs[1]->getPoints();
      Point bp(bp1.x(), min(bp1.y(), bp2.y()));
      Point ep(ep1.x(), max(ep1.y(), ep2.y()));
      rptr->setPoints(bp, ep);

      frSegStyle style = vertPathSegs[0]->getStyle();
      frSegStyle style_1 = vertPathSegs[1]->getStyle();
      if (bp1.y() < bp2.y()) {
        style.setEndStyle(style_1.getEndStyle(), style_1.getEndExt());
      } else {
        style.setBeginStyle(style_1.getBeginStyle(), style_1.getBeginExt());
      }
      rptr->setStyle(style);
      if (save_updates_) {
        drUpdate update1(drUpdate::REMOVE_FROM_NET),
            update2(drUpdate::REMOVE_FROM_NET);
        update1.setNet(net);
        update2.setNet(net);
        update1.setIndexInOwner(vertPathSegs[0]->getIndexInOwner());
        update2.setIndexInOwner(vertPathSegs[1]->getIndexInOwner());
        design_->addUpdate(update1);
        design_->addUpdate(update2);
      }
      regionQuery->removeDRObj(vertPathSegs[0]);
      regionQuery->removeDRObj(vertPathSegs[1]);
      net->removeShape(vertPathSegs[0]);
      net->removeShape(vertPathSegs[1]);
      if (save_updates_) {
        drUpdate update3(drUpdate::ADD_SHAPE);
        update3.setNet(net);
        update3.setPathSeg(*rptr);
        design_->addUpdate(update3);
      }
      net->addShape(std::move(uShape));
      regionQuery->addDRObj(rptr);
    }
  }
}

void FlexDRWorker::endAddNets(
    frDesign* design,
    map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp>& boundPts)
{
  for (auto& net : nets_) {
    if (!net->isModified()) {
      continue;
    }
    for (auto& connFig : net->getBestRouteConnFigs()) {
      if (connFig->typeId() == drcPathSeg) {
        endAddNets_pathSeg(
            design, static_cast<drPathSeg*>(connFig.get()) /*, cutSegs[fNet]*/);
      } else if (connFig->typeId() == drcVia) {
        endAddNets_via(design, static_cast<drVia*>(connFig.get()));
      } else if (connFig->typeId() == drcPatchWire) {
        endAddNets_patchWire(design, static_cast<drPatchWire*>(connFig.get()));
      } else {
        cout << "Error: endAddNets unsupported type" << endl;
      }
    }
  }
  for (auto& [net, bPts] : boundPts) {
    endAddNets_merge(design, net, bPts);
  }
}

void FlexDRWorker::endRemoveMarkers(frDesign* design)
{
  auto regionQuery = design->getRegionQuery();
  auto topBlock = design->getTopBlock();
  vector<frMarker*> result;
  regionQuery->queryMarker(getDrcBox(), result);
  for (auto mptr : result) {
    if (save_updates_) {
      drUpdate update(drUpdate::REMOVE_FROM_BLOCK);
      update.setIndexInOwner(mptr->getIndexInOwner());
      design_->addUpdate(update);
    }
    regionQuery->removeMarker(mptr);
    topBlock->removeMarker(mptr);
  }
}

void FlexDRWorker::endAddMarkers(frDesign* design)
{
  auto regionQuery = design->getRegionQuery();
  auto topBlock = design->getTopBlock();
  // for (auto &m: getMarkers()) {
  for (auto& m : getBestMarkers()) {
    if (getDrcBox().intersects(m.getBBox())) {
      auto uptr = make_unique<frMarker>(m);
      auto ptr = uptr.get();
      regionQuery->addMarker(ptr);
      topBlock->addMarker(std::move(uptr));
      if (save_updates_) {
        drUpdate update(drUpdate::ADD_SHAPE);
        update.setMarker(*ptr);
        design_->addUpdate(update);
      }
    }
  }
}

void FlexDRWorker::cleanup()
{
  apSVia_.clear();
  planarHistoryMarkers_.clear();
  viaHistoryMarkers_.clear();
  historyMarkers_.clear();
  historyMarkers_.shrink_to_fit();
  for (auto& net : getNets()) {
    net->cleanup();
  }
  owner2nets_.clear();
  gridGraph_.cleanup();
  markers_.clear();
  markers_.shrink_to_fit();
  rq_.cleanup();
  specialAccessAPs.clear();
}

bool FlexDRWorker::end(frDesign* design)
{
  if (skipRouting_ == true) {
    return false;
  }
  // skip if current clip does not have input DRCs
  // ripupMode = 0 must have enableDRC = true in previous iteration
  if (getDRIter() && getInitNumMarkers() == 0 && !needRecheck_) {
    return false;
    // do not write back if current clip is worse than input
  }
  if (getRipupMode() != RipUpMode::ALL
      && getBestNumMarkers() > getInitNumMarkers()) {
    // cout <<"skip clip with #init/final = " <<getInitNumMarkers() <<"/"
    // <<getNumMarkers() <<endl;
    return false;
  }
  if (getDRIter() && getRipupMode() == RipUpMode::ALL
      && getBestNumMarkers() > 5 * getInitNumMarkers()) {
    return false;
  }
  save_updates_ = dist_on_;
  set<frNet*, frBlockObjectComp> modNets;
  endGetModNets(modNets);
  // get lock
  map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp> boundPts;
  endRemoveNets(design, modNets, boundPts);
  endAddNets(design, boundPts);  // if two subnets have diff isModified()
                                 // status, then should always write back
  endRemoveMarkers(design);
  endAddMarkers(design);
  return true;
  // release lock
  return true;
}
