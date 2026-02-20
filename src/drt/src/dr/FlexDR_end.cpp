// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/infra/frPoint.h"
#include "db/infra/frSegStyle.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "distributed/drUpdate.h"
#include "dr/FlexDR.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "odb/geom.h"

namespace drt {

void FlexDRWorker::endGetModNets(frOrderedIdSet<frNet*>& modNets)
{
  for (auto& net : nets_) {
    if (net->isModified()) {
      auto fr_net = net->getFrNet();
      fr_net->setModified(true);
      modNets.insert(fr_net);
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
    std::set<std::pair<odb::Point, frLayerNum>>& boundPts)
{
  auto [begin, end] = pathSeg->getPoints();
  auto routeBox = getRouteBox();
  auto net = pathSeg->getNet();
  auto regionQuery = design->getRegionQuery();
  if (pathSeg->isApPathSeg()) {
    if (getRouteBox().intersects(pathSeg->getApLoc())) {
      if (save_updates_) {
        drUpdate update(drUpdate::REMOVE_FROM_NET);
        update.setNet(net);
        update.setIndexInOwner(pathSeg->getIndexInOwner());
        design_->addUpdate(update);
      }
      regionQuery->removeDRObj(pathSeg);
      net->removeShape(pathSeg);
    }
    return;
  }
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
      return;
    }
    bool condition2 = (begin.y() <= routeBox.yMax());  // orthogonal to wire
    if (routeBox.xMin() <= begin.x() && begin.x() <= routeBox.xMax()
        && !(begin.y() > routeBox.yMax() || end.y() < routeBox.yMin())) {
      // bottom seg to ext
      if (begin.y() < routeBox.yMin()) {
        auto uPathSeg = std::make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        odb::Point boundPt(end.x(), std::min(end.y(), routeBox.yMin()));
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
        std::unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.y() >= routeBox.yMin()) {
          boundPts.insert(std::make_pair(boundPt, lNum));
        }
      }
      // top seg to ext
      if (end.y() > routeBox.yMax()) {
        auto uPathSeg = std::make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        odb::Point boundPt(begin.x(), std::max(begin.y(), routeBox.yMax()));
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
        std::unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary piont
        if (condition2) {
          boundPts.insert(std::make_pair(boundPt, lNum));
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
      return;
    }
    // if cross routeBBox
    bool condition2 = /*isInitDR() ? (begin.x() < routeBox.xMax()):*/ (
        begin.x() <= routeBox.xMax());  // orthogonal to wire
    if (routeBox.yMin() <= begin.y() && begin.y() <= routeBox.yMax()
        && !(begin.x() > routeBox.xMax() || end.x() < routeBox.xMin())) {
      // left seg to ext
      if (begin.x() < routeBox.xMin()) {
        auto uPathSeg = std::make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        odb::Point boundPt(std::min(end.x(), routeBox.xMin()), end.y());
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
        std::unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (end.x() >= routeBox.xMin()) {
          boundPts.insert(std::make_pair(boundPt, lNum));
        }
      }
      // right seg to ext
      if (end.x() > routeBox.xMax()) {
        auto uPathSeg = std::make_unique<frPathSeg>(*pathSeg);
        auto ps = uPathSeg.get();
        odb::Point boundPt(std::max(begin.x(), routeBox.xMax()), begin.y());
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
        std::unique_ptr<frShape> uShape(std::move(uPathSeg));
        auto sptr = uShape.get();
        net->addShape(std::move(uShape));
        regionQuery->addDRObj(sptr);
        // add cutSegs
        auto lNum = sptr->getLayerNum();

        // only insert true boundary point
        if (condition2) {
          boundPts.insert(std::make_pair(boundPt, lNum));
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
    frOrderedIdSet<frNet*>& modNets,
    frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>&
        boundPts)
{
  std::vector<frBlockObject*> result;
  design->getRegionQuery()->queryDRObj(getExtBox(), result);
  for (auto rptr : result) {
    if (rptr->typeId() == frcPathSeg) {
      auto cptr = static_cast<frPathSeg*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_pathSeg(design, cptr, boundPts[cptr->getNet()]);
        }
      } else {
        std::cout << "Error: endRemoveNet hasNet() empty\n";
      }
    } else if (rptr->typeId() == frcVia) {
      auto cptr = static_cast<frVia*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_via(design, cptr);
        }
      } else {
        std::cout << "Error: endRemoveNet hasNet() empty\n";
      }
    } else if (rptr->typeId() == frcPatchWire) {
      auto cptr = static_cast<frPatchWire*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_patchWire(design, cptr);
        }
      } else {
        std::cout << "Error: endRemoveNet hasNet() empty\n";
      }
    } else {
      std::cout << "Error: endRemoveNets unsupported type\n";
    }
  }
}

void FlexDRWorker::endAddNets_pathSeg(frDesign* design, drPathSeg* pathSeg)
{
  auto net = pathSeg->getNet()->getFrNet();
  std::unique_ptr<frShape> uShape = std::make_unique<frPathSeg>(*pathSeg);
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
  std::unique_ptr<frVia> uVia = std::make_unique<frVia>(*via);
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
  std::unique_ptr<frShape> uShape = std::make_unique<frPatchWire>(*pwire);
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

void FlexDRWorker::endAddNets_merge(
    frDesign* design,
    frNet* net,
    std::set<std::pair<odb::Point, frLayerNum>>& boundPts)
{
  frRegionQuery::Objects<frBlockObject> result;
  std::vector<frBlockObject*> drObjs;
  std::vector<frPathSeg*> horzPathSegs;
  std::vector<frPathSeg*> vertPathSegs;
  bool hasPatchMetal = false;
  auto regionQuery = design->getRegionQuery();
  for (auto& [pt, lNum] : boundPts) {
    hasPatchMetal = false;
    bool skip = false;
    result.clear();
    regionQuery->query(odb::Rect(pt, pt), lNum, result);
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
    regionQuery->queryDRObj(odb::Rect(pt, pt), lNum, drObjs);
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
        odb::Point bp = pwire->getOrigin();
        if (bp == pt) {
          hasPatchMetal = true;
          break;
        }
      }
    }
    // merge horz pathseg
    if ((int) horzPathSegs.size() == 2 && vertPathSegs.empty() && !hasPatchMetal
        && horzPathSegs[0]->isTapered() == horzPathSegs[1]->isTapered()) {
      std::unique_ptr<frShape> uShape
          = std::make_unique<frPathSeg>(*horzPathSegs[0]);
      auto rptr = static_cast<frPathSeg*>(uShape.get());
      auto [bp1, ep1] = horzPathSegs[0]->getPoints();
      auto [bp2, ep2] = horzPathSegs[1]->getPoints();
      odb::Point bp(std::min(bp1.x(), bp2.x()), bp1.y());
      odb::Point ep(std::max(ep1.x(), ep2.x()), ep1.y());
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
      std::unique_ptr<frShape> uShape
          = std::make_unique<frPathSeg>(*vertPathSegs[0]);
      auto rptr = static_cast<frPathSeg*>(uShape.get());
      auto [bp1, ep1] = vertPathSegs[0]->getPoints();
      auto [bp2, ep2] = vertPathSegs[1]->getPoints();
      odb::Point bp(bp1.x(), std::min(bp1.y(), bp2.y()));
      odb::Point ep(ep1.x(), std::max(ep1.y(), ep2.y()));
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

bool FlexDRWorker::endAddNets_updateExtFigs_pathSeg(drNet* net,
                                                    const Point3D& update_pt,
                                                    frPathSeg* path_seg)
{
  frNet* fr_net = net->getFrNet();
  if (path_seg->getNet() != fr_net) {
    return false;
  }
  const auto [bp, ep] = path_seg->getPoints();
  if (bp != update_pt && ep != update_pt) {
    return false;
  }
  // remove from rq before updating bbox
  getDesign()->getRegionQuery()->removeDRObj(path_seg);
  if (save_updates_) {
    drUpdate update(
        drUpdate::REMOVE_FROM_RQ, fr_net, path_seg->getIndexInOwner());
    getDesign()->addUpdate(update);
  }
  // update path_seg style
  frSegStyle updated_style;
  net->getExtFigUpdate(update_pt, updated_style);
  path_seg->setStyle(updated_style);
  // add to rq after updating bbox
  getDesign()->getRegionQuery()->addDRObj(path_seg);
  if (save_updates_) {
    drUpdate update(
        drUpdate::UPDATE_SHAPE, fr_net, path_seg->getIndexInOwner());
    // UPDATE_SHAPE with pathSeg adds to rq
    update.setPathSeg(*path_seg);
    getDesign()->addUpdate(update);
  }
  return true;
}

bool FlexDRWorker::endAddNets_updateExtFigs_via(drNet* net,
                                                const Point3D& update_pt,
                                                frVia* via)
{
  frNet* fr_net = net->getFrNet();
  if (via->getNet() != fr_net || via->getOrigin() != update_pt) {
    return false;
  }
  // update via connections
  bool is_bottom_connected, is_top_connected;
  net->getExtFigUpdate(update_pt, is_bottom_connected, is_top_connected);
  via->setBottomConnected(is_bottom_connected);
  via->setTopConnected(is_top_connected);
  if (save_updates_) {
    drUpdate update(drUpdate::UPDATE_SHAPE, fr_net, via->getIndexInOwner());
    update.setVia(*via);
    getDesign()->addUpdate(update);
  }
  return true;
}

void FlexDRWorker::endAddNets_updateExtFigs(drNet* net)
{
  const std::vector<Point3D> locs = net->getExtFigsUpdatesLocs();
  auto region_query = design_->getRegionQuery();
  for (const auto& pt : locs) {
    frRegionQuery::Objects<frBlockObject> result;
    region_query->queryDRObj({pt, pt}, pt.z(), result);
    const bool is_via = net->isExtFigUpdateVia(pt);
    for (const auto& [_, obj] : result) {
      if (is_via && obj->typeId() == frcVia) {
        auto via = static_cast<frVia*>(obj);
        if (endAddNets_updateExtFigs_via(net, pt, via)) {
          break;
        }
      } else if (!is_via && obj->typeId() == frcPathSeg) {
        auto path_seg = static_cast<frPathSeg*>(obj);
        if (endAddNets_updateExtFigs_pathSeg(net, pt, path_seg)) {
          break;
        }
      }
    }
  }
}
void FlexDRWorker::endAddNets(
    frDesign* design,
    frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>&
        boundPts)
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
        std::cout << "Error: endAddNets unsupported type\n";
      }
    }
    if (net->hasExtFigUpdates()) {
      endAddNets_updateExtFigs(net.get());
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
  std::vector<frMarker*> result;
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
      auto uptr = std::make_unique<frMarker>(m);
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
  specialAccessAPs_.clear();
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
  if ((getRipupMode() == RipUpMode::DRC || getRipupMode() == RipUpMode::NEARDRC
       || getRipupMode() == RipUpMode::VIASWAP)
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
  frOrderedIdSet<frNet*> modNets;
  endGetModNets(modNets);
  // get lock
  frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>> boundPts;
  endRemoveNets(design, modNets, boundPts);
  endAddNets(design, boundPts);  // if two subnets have diff isModified()
                                 // status, then should always write back
  endRemoveMarkers(design);
  endAddMarkers(design);
  // release lock
  return true;
}

}  // namespace drt
