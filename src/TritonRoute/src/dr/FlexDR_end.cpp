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
    frPathSeg* pathSeg,
    set<pair<frPoint, frLayerNum>>& boundPts)
{
  bool enableOutput = false;
  frPoint begin, end;
  pathSeg->getPoints(begin, end);
  auto routeBox = getRouteBox();
  auto net = pathSeg->getNet();
  auto regionQuery = getRegionQuery();
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
    bool condition1
        = isInitDR() ? (begin.x() < routeBox.right())
                     : (begin.x() <= routeBox.right());  // parallel to wire
    bool condition2 = (begin.y() <= routeBox.top());     // orthogonal to wire
    if (routeBox.left() <= begin.x() && condition1
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
        if (enableOutput) {
          cout << "trim pathseg to ext bottom" << endl;
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
        if (enableOutput) {
          cout << "trim pathseg to ext top" << endl;
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
    // if cross routeBBox
    bool condition1 = isInitDR()
                          ? (begin.y() < routeBox.top())
                          : (begin.y() <= routeBox.top());  // parallel to wire
    bool condition2 = /*isInitDR() ? (begin.x() < routeBox.right()):*/ (
        begin.x() <= routeBox.right());  // orthogonal to wire
    if (routeBox.bottom() <= begin.y() && condition1
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
        if (enableOutput) {
          cout << "trim pathseg to ext left" << endl;
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
        if (enableOutput) {
          cout << "trim pathseg to ext right" << endl;
        }
      }
      regionQuery->removeDRObj(pathSeg);  // delete rq
      net->removeShape(pathSeg);          // delete segment
    }
  }
}

void FlexDRWorker::endRemoveNets_via(frVia* via)
{
  // bool enableOutput = true;
  bool enableOutput = false;
  auto gridBBox = getRouteBox();
  auto regionQuery = getRegionQuery();
  auto net = via->getNet();
  frPoint viaPoint;
  via->getOrigin(viaPoint);
  bool condition1 = isInitDR() ? (viaPoint.x() < gridBBox.right()
                                  && viaPoint.y() < gridBBox.top())
                               : (viaPoint.x() <= gridBBox.right()
                                  && viaPoint.y() <= gridBBox.top());
  if (viaPoint.x() >= gridBBox.left() && viaPoint.y() >= gridBBox.bottom()
      && condition1) {
    regionQuery->removeDRObj(via);  // delete rq
    net->removeVia(via);
    if (enableOutput) {
      cout << "delete via in route" << endl;
    }
  }
}

void FlexDRWorker::endRemoveNets_patchWire(frPatchWire* pwire)
{
  // bool enableOutput = true;
  bool enableOutput = false;
  auto gridBBox = getRouteBox();
  auto regionQuery = getRegionQuery();
  auto net = pwire->getNet();
  frPoint origin;
  pwire->getOrigin(origin);
  bool condition1
      = isInitDR()
            ? (origin.x() < gridBBox.right() && origin.y() < gridBBox.top())
            : (origin.x() <= gridBBox.right() && origin.y() <= gridBBox.top());
  if (origin.x() >= gridBBox.left() && origin.y() >= gridBBox.bottom()
      && condition1) {
    regionQuery->removeDRObj(pwire);  // delete rq
    net->removePatchWire(pwire);
    if (enableOutput) {
      cout << "delete pwire in route" << endl;
    }
  }
}

void FlexDRWorker::endRemoveNets(
    set<frNet*, frBlockObjectComp>& modNets,
    map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>& boundPts)
{
  vector<frBlockObject*> result;
  getRegionQuery()->queryDRObj(getRouteBox(), result);
  for (auto rptr : result) {
    if (rptr->typeId() == frcPathSeg) {
      auto cptr = static_cast<frPathSeg*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_pathSeg(cptr, boundPts[cptr->getNet()]);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcVia) {
      auto cptr = static_cast<frVia*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_via(cptr);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else if (rptr->typeId() == frcPatchWire) {
      auto cptr = static_cast<frPatchWire*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_patchWire(cptr);
        }
      } else {
        cout << "Error: endRemoveNet hasNet() empty" << endl;
      }
    } else {
      cout << "Error: endRemoveNets unsupported type" << endl;
    }
  }
}

void FlexDRWorker::endAddNets_pathSeg(drPathSeg* pathSeg)
{
  auto net = pathSeg->getNet()->getFrNet();
  unique_ptr<frShape> uShape = make_unique<frPathSeg>(*pathSeg);
  auto rptr = uShape.get();
  net->addShape(std::move(uShape));
  getRegionQuery()->addDRObj(rptr);
}

void FlexDRWorker::endAddNets_via(drVia* via)
{
  auto net = via->getNet()->getFrNet();
  unique_ptr<frVia> uVia = make_unique<frVia>(*via);
  auto rptr = uVia.get();
  net->addVia(std::move(uVia));
  getRegionQuery()->addDRObj(rptr);
}

void FlexDRWorker::endAddNets_patchWire(drPatchWire* pwire)
{
  auto net = pwire->getNet()->getFrNet();
  unique_ptr<frShape> uShape = make_unique<frPatchWire>(*pwire);
  auto rptr = uShape.get();
  net->addPatchWire(std::move(uShape));
  getRegionQuery()->addDRObj(rptr);
}

void FlexDRWorker::endAddNets_merge(frNet* net,
                                    set<pair<frPoint, frLayerNum>>& boundPts)
{
  frRegionQuery::Objects<frBlockObject> result;
  vector<frBlockObject*> drObjs;
  vector<frPathSeg*> horzPathSegs;
  vector<frPathSeg*> vertPathSegs;
  bool hasPatchMetal = false;
  auto regionQuery = getRegionQuery();
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
    frCoord manuGrid = getDesign()->getTech()->getManufacturingGrid();
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
        if (bp == pt || ep == pt) {
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
    map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>& boundPts)
{
  // bool enableOutput = true;
  for (auto& net : nets_) {
    if (!net->isModified()) {
      continue;
    }
    // if (enableOutput) {
    //   if (net->getFrNet()->getName() == string("net30")) {
    //     cout <<"write back net@@@" <<endl;
    //   }
    // }
    // double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    // if (getDRIter() == 2 &&
    //     getRouteBox().left()    == 63     * dbu &&
    //     getRouteBox().right()   == 84     * dbu &&
    //     getRouteBox().bottom()  == 139.65 * dbu &&
    //     getRouteBox().top()     == 159.6  * dbu) {
    //   cout <<"write back net " <<net->getFrNet()->getName() <<endl;
    // }
    for (auto& connFig : net->getBestRouteConnFigs()) {
      if (connFig->typeId() == drcPathSeg) {
        endAddNets_pathSeg(
            static_cast<drPathSeg*>(connFig.get()) /*, cutSegs[fNet]*/);
      } else if (connFig->typeId() == drcVia) {
        endAddNets_via(static_cast<drVia*>(connFig.get()));
      } else if (connFig->typeId() == drcPatchWire) {
        endAddNets_patchWire(static_cast<drPatchWire*>(connFig.get()));
      } else {
        cout << "Error: endAddNets unsupported type" << endl;
      }
    }
  }
  for (auto& [net, bPts] : boundPts) {
    endAddNets_merge(net, bPts);
  }
}

void FlexDRWorker::endRemoveMarkers()
{
  auto regionQuery = getRegionQuery();
  auto topBlock = getDesign()->getTopBlock();
  vector<frMarker*> result;
  regionQuery->queryMarker(getDrcBox(), result);
  for (auto mptr : result) {
    regionQuery->removeMarker(mptr);
    topBlock->removeMarker(mptr);
  }
}

void FlexDRWorker::endAddMarkers()
{
  auto regionQuery = getRegionQuery();
  auto topBlock = getDesign()->getTopBlock();
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
  owner2pins_.clear();
  gridGraph_.cleanup();
  markers_.clear();
  markers_.shrink_to_fit();
  rq_.cleanup();
}

void FlexDRWorker::end()
{
  if (skipRouting_ == true) {
    return;
  }
  // skip if current clip does not have input DRCs
  // ripupMode = 0 must have enableDRC = true in previous iteration
  if (isEnableDRC() && getDRIter() && getInitNumMarkers() == 0
      && !needRecheck_) {
    return;
    // do not write back if current clip is worse than input
  } else if (isEnableDRC() && getRipupMode() == 0
             && getBestNumMarkers() > getInitNumMarkers()) {
    // cout <<"skip clip with #init/final = " <<getInitNumMarkers() <<"/"
    // <<getNumMarkers() <<endl;
    return;
  } else if (isEnableDRC() && getDRIter() && getRipupMode() == 1
             && getBestNumMarkers() > 5 * getInitNumMarkers()) {
    return;
  }

  set<frNet*, frBlockObjectComp> modNets;
  endGetModNets(modNets);
  // get lock
  map<frNet*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp> boundPts;
  endRemoveNets(modNets, boundPts);
  endAddNets(boundPts);  // if two subnets have diff isModified() status, then
                         // should always write back
  if (isEnableDRC()) {
    endRemoveMarkers();
    endAddMarkers();
  }
  // release lock
}

int FlexDRWorker::getNumQuickMarkers()
{
  int totNum = 0;
  for (auto& net : nets_) {
    totNum += net->getNumMarkers();
  }
  return totNum;
}
