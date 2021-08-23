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

void FlexDRWorker::endRemoveNets_pathSeg(
    frDesign* design,
    frPathSeg* pathSeg,
    set<pair<frPoint, frLayerNum>>& boundPts)
{
  frPoint begin, end;
  pathSeg->getPoints(begin, end);
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
        && (begin.x() == routeBox.left() || begin.x() == routeBox.right())) {
      if (begin.y() < routeBox.bottom() || end.y() > routeBox.top()
          || pathSeg->getBeginStyle() != frcTruncateEndStyle
          || pathSeg->getEndStyle() != frcTruncateEndStyle)
        return;
    }
    bool condition2 = (begin.y() <= routeBox.top());  // orthogonal to wire
    if (routeBox.left() <= begin.x() && begin.x() <= routeBox.right()
        && !(begin.y() > routeBox.top() || end.y() < routeBox.bottom())) {
      // bottom seg to ext
      if (begin.y() < routeBox.bottom()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        frPoint boundPt(end.x(), min(end.y(), routeBox.bottom()));
        uPathSeg->setPoints(begin, boundPt);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (end.y() > routeBox.bottom()) {
          frSegStyle style;
          pathSeg->getStyle(style);
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }

        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.y() >= routeBox.bottom()) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      // top seg to ext
      if (end.y() > routeBox.top()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        frPoint boundPt(begin.x(), max(begin.y(), routeBox.top()));
        uPathSeg->setPoints(boundPt, end);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (begin.y() < routeBox.top()) {
          frSegStyle style;
          pathSeg->getStyle(style);
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
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
      // std::cout << "  removingPathSeg " << &(*pathSeg) << " (" << begin.x()
      // << ", " << begin.y()
      //           << ") -- (" << end.x() << ", " << end.y() << ") " <<
      //           drNet->getName() <<  "\n" << std::flush;
      regionQuery->removeDRObj(pathSeg);  // delete rq
      net->removeShape(pathSeg);          // delete segment
    }
    // horizontal seg
  } else if (begin.y() == end.y()) {
    if (isInitDR()
        && (begin.y() == routeBox.bottom() || begin.y() == routeBox.top())) {
      if (begin.x() < routeBox.left() || end.x() > routeBox.right()
          || pathSeg->getBeginStyle() != frcTruncateEndStyle
          || pathSeg->getEndStyle() != frcTruncateEndStyle)
        return;
    }
    // if cross routeBBox
    bool condition2 = /*isInitDR() ? (begin.x() < routeBox.right()):*/ (
        begin.x() <= routeBox.right());  // orthogonal to wire
    if (routeBox.bottom() <= begin.y() && begin.y() <= routeBox.top()
        && !(begin.x() > routeBox.right() || end.x() < routeBox.left())) {
      // left seg to ext
      if (begin.x() < routeBox.left()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        frPoint boundPt(min(end.x(), routeBox.left()), end.y());
        uPathSeg->setPoints(begin, boundPt);
        // change boundary style to ext if orig pathSeg crosses boundary
        if (end.x() > routeBox.left()) {
          frSegStyle style;
          pathSeg->getStyle(style);
          style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
        }

        unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.x() >= routeBox.left()) {
          boundPts.insert(make_pair(boundPt, lNum));
        }
      }
      // right seg to ext
      if (end.x() > routeBox.right()) {
        auto uPathSeg = make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        frPoint boundPt(max(begin.x(), routeBox.right()), begin.y());
        uPathSeg->setPoints(boundPt, end);
        // change boundary style to ext if orig pathSeg crosses at boundary
        if (begin.x() < routeBox.right()) {
          frSegStyle style;
          pathSeg->getStyle(style);
          style
              .setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt() /*getTech()->getLayer(pathSeg->getLayerNum())->getWidth() / 2*/);
          ps->setStyle(style);
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
      regionQuery->removeDRObj(pathSeg);  // delete rq
      net->removeShape(pathSeg);          // delete segment
    }
  }
}

void FlexDRWorker::endRemoveNets_via(frDesign* design, frVia* via)
{
  auto gridBBox = getRouteBox();
  auto regionQuery = design->getRegionQuery();
  auto net = via->getNet();
  frPoint viaPoint;
  via->getOrigin(viaPoint);
  if (isInitDR()
      && (viaPoint.x() == gridBBox.left() || viaPoint.x() == gridBBox.right()
          || viaPoint.y() == gridBBox.bottom()
          || viaPoint.y() == gridBBox.top())) {
    return;
  }
  if (viaPoint.x() >= gridBBox.left() && viaPoint.y() >= gridBBox.bottom()
      && viaPoint.x() <= gridBBox.right() && viaPoint.y() <= gridBBox.top()) {
    regionQuery->removeDRObj(via);  // delete rq
    net->removeVia(via);
  }
}

void FlexDRWorker::endRemoveNets_patchWire(frDesign* design, frPatchWire* pwire)
{
  auto gridBBox = getRouteBox();
  auto regionQuery = design->getRegionQuery();
  auto net = pwire->getNet();
  frPoint origin;
  pwire->getOrigin(origin);
  if (isInitDR()
      && (origin.x() == gridBBox.left() || origin.x() == gridBBox.right()
          || origin.y() == gridBBox.bottom() || origin.y() == gridBBox.top())) {
    return;
  }
  if (origin.x() >= gridBBox.left() && origin.y() >= gridBBox.bottom()
      && origin.x() <= gridBBox.right() && origin.y() <= gridBBox.top()) {
    regionQuery->removeDRObj(pwire);  // delete rq
    net->removePatchWire(pwire);
  }
}

void FlexDRWorker::endRemoveNets(
    frDesign* design,
    set<frNet*, frBlockObjectComp>& modNets,
    map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>& boundPts)
{
  vector<frBlockObject*> result;
  design->getRegionQuery()->queryDRObj(getRouteBox(), result);
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
}

void FlexDRWorker::endAddNets_via(frDesign* design, drVia* via)
{
  auto net = via->getNet()->getFrNet();
  unique_ptr<frVia> uVia = make_unique<frVia>(*via);
  auto rptr = uVia.get();
  net->addVia(std::move(uVia));
  design->getRegionQuery()->addDRObj(rptr);
}

void FlexDRWorker::endAddNets_patchWire(frDesign* design, drPatchWire* pwire)
{
  auto net = pwire->getNet()->getFrNet();
  unique_ptr<frShape> uShape = make_unique<frPatchWire>(*pwire);
  auto rptr = uShape.get();
  net->addPatchWire(std::move(uShape));
  design->getRegionQuery()->addDRObj(rptr);
}

void FlexDRWorker::endAddNets_merge(frDesign* design,
                                    frNet* net,
                                    set<pair<frPoint, frLayerNum>>& boundPts)
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
    regionQuery->query(frBox(pt, pt), lNum, result);
    for (auto& [bx, obj] : result) {
      if (obj->typeId() == frcInstTerm) {
        auto instTerm = static_cast<frInstTerm*>(obj);
        if (instTerm->getNet() == net) {
          skip = true;
          break;
        }
      } else if (obj->typeId() == frcTerm) {
        auto term = static_cast<frTerm*>(obj);
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
    regionQuery->queryDRObj(frBox(pt, pt), lNum, drObjs);
    for (auto& obj : drObjs) {
      if (obj->typeId() == frcPathSeg) {
        auto ps = static_cast<frPathSeg*>(obj);
        if (!(ps->getNet() == net)) {
          continue;
        }
        frPoint bp, ep;
        ps->getPoints(bp, ep);
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
        frPoint bp;
        pwire->getOrigin(bp);
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
      frPoint bp1, ep1, bp2, ep2;
      horzPathSegs[0]->getPoints(bp1, ep1);
      horzPathSegs[1]->getPoints(bp2, ep2);
      frPoint bp(min(bp1.x(), bp2.x()), bp1.y());
      frPoint ep(max(ep1.x(), ep2.x()), ep1.y());
      rptr->setPoints(bp, ep);

      frSegStyle style, style_1;
      horzPathSegs[0]->getStyle(style);
      horzPathSegs[1]->getStyle(style_1);
      if (bp1.x() < bp2.x()) {
        style.setEndStyle(style_1.getEndStyle(), style_1.getEndExt());
      } else {
        style.setBeginStyle(style_1.getBeginStyle(), style_1.getBeginExt());
      }
      rptr->setStyle(style);

      regionQuery->removeDRObj(horzPathSegs[0]);
      regionQuery->removeDRObj(horzPathSegs[1]);
      net->removeShape(horzPathSegs[0]);
      net->removeShape(horzPathSegs[1]);

      net->addShape(std::move(uShape));
      regionQuery->addDRObj(rptr);
    }
    if ((int) vertPathSegs.size() == 2 && horzPathSegs.empty() && !hasPatchMetal
        && vertPathSegs[0]->isTapered() == vertPathSegs[1]->isTapered()) {
      unique_ptr<frShape> uShape = make_unique<frPathSeg>(*vertPathSegs[0]);
      auto rptr = static_cast<frPathSeg*>(uShape.get());
      frPoint bp1, ep1, bp2, ep2;
      vertPathSegs[0]->getPoints(bp1, ep1);
      vertPathSegs[1]->getPoints(bp2, ep2);
      frPoint bp(bp1.x(), min(bp1.y(), bp2.y()));
      frPoint ep(ep1.x(), max(ep1.y(), ep2.y()));
      rptr->setPoints(bp, ep);

      frSegStyle style, style_1;
      vertPathSegs[0]->getStyle(style);
      vertPathSegs[1]->getStyle(style_1);
      if (bp1.y() < bp2.y()) {
        style.setEndStyle(style_1.getEndStyle(), style_1.getEndExt());
      } else {
        style.setBeginStyle(style_1.getBeginStyle(), style_1.getBeginExt());
      }
      rptr->setStyle(style);

      regionQuery->removeDRObj(vertPathSegs[0]);
      regionQuery->removeDRObj(vertPathSegs[1]);
      net->removeShape(vertPathSegs[0]);
      net->removeShape(vertPathSegs[1]);

      net->addShape(std::move(uShape));
      regionQuery->addDRObj(rptr);
    }
  }
}

void FlexDRWorker::endAddNets(
    frDesign* design,
    map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>& boundPts)
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
    regionQuery->removeMarker(mptr);
    topBlock->removeMarker(mptr);
  }
}

void FlexDRWorker::endAddMarkers(frDesign* design)
{
  auto regionQuery = design->getRegionQuery();
  auto topBlock = design->getTopBlock();
  frBox mBox;
  // for (auto &m: getMarkers()) {
  for (auto& m : getBestMarkers()) {
    m.getBBox(mBox);
    if (getDrcBox().overlaps(mBox)) {
      auto uptr = make_unique<frMarker>(m);
      auto ptr = uptr.get();
      regionQuery->addMarker(ptr);
      topBlock->addMarker(std::move(uptr));
    }
  }
}

void FlexDRWorker::cleanup()
{
  apSVia_.clear();
  fixedObjs_.clear();
  fixedObjs_.shrink_to_fit();
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
}

void FlexDRWorker::end(frDesign* design)
{
  if (skipRouting_ == true) {
    return;
  }
  // skip if current clip does not have input DRCs
  // ripupMode = 0 must have enableDRC = true in previous iteration
  if (getDRIter() && getInitNumMarkers() == 0 && !needRecheck_) {
    return;
    // do not write back if current clip is worse than input
  } else if (getRipupMode() == 0 && getBestNumMarkers() > getInitNumMarkers()) {
    // cout <<"skip clip with #init/final = " <<getInitNumMarkers() <<"/"
    // <<getNumMarkers() <<endl;
    return;
  } else if (getDRIter() && getRipupMode() == 1
             && getBestNumMarkers() > 5 * getInitNumMarkers()) {
    return;
  }

  set<frNet*, frBlockObjectComp> modNets;
  endGetModNets(modNets);
  // get lock
  map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp> boundPts;
  endRemoveNets(design, modNets, boundPts);
  endAddNets(design, boundPts);  // if two subnets have diff isModified()
                                 // status, then should always write back
  endRemoveMarkers(design);
  endAddMarkers(design);
  // release lock
}
