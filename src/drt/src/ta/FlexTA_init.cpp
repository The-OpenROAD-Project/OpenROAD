// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGuide.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frShape.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "db/taObj/taVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "ta/FlexTA.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

bool FlexTAWorker::outOfDieVia(frLayerNum layer_num,
                               const odb::Point& pt,
                               const odb::Rect& die_box) const
{
  if (router_cfg_->USENONPREFTRACKS
      && !getTech()->getLayer(layer_num)->isUnidirectional()) {
    return false;
  }
  odb::Rect test_box_up;
  if (layer_num + 1 <= getTech()->getTopLayerNum()) {
    const frViaDef* via
        = getTech()->getLayer(layer_num + 1)->getDefaultViaDef();
    if (via) {
      test_box_up = via->getLayer1ShapeBox();
      test_box_up.merge(via->getLayer2ShapeBox());
      test_box_up.moveDelta(pt.x(), pt.y());
    } else {
      // artificial value to indicate no via in test below
      die_box.bloat(1, test_box_up);
    }
  }
  odb::Rect test_box_down;
  if (layer_num - 1 >= getTech()->getBottomLayerNum()) {
    const frViaDef* via
        = getTech()->getLayer(layer_num - 1)->getDefaultViaDef();
    if (via) {
      test_box_down = via->getLayer1ShapeBox();
      test_box_down.merge(via->getLayer2ShapeBox());
      test_box_down.moveDelta(pt.x(), pt.y());
    } else {
      // artificial value to indicate no via in test below
      die_box.bloat(1, test_box_down);
    }
  }
  return !die_box.contains(test_box_up) && !die_box.contains(test_box_down);
}
void FlexTAWorker::initTracks()
{
  trackLocs_.clear();
  const int numLayers = getDesign()->getTech()->getLayers().size();
  trackLocs_.resize(numLayers);
  std::vector<std::set<frCoord>> trackCoordSets(numLayers);
  // uPtr for tp
  auto die_box = getDesign()->getTopBlock()->getDieBox();
  auto die_center = die_box.center();
  for (int lNum = 0; lNum < (int) numLayers; lNum++) {
    auto layer = getDesign()->getTech()->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (layer->getDir() != getDir()) {
      continue;
    }
    for (auto& tp : getDesign()->getTopBlock()->getTrackPatterns(lNum)) {
      if ((getDir() == dbTechLayerDir::HORIZONTAL
           && tp->isHorizontal() == false)
          || (getDir() == dbTechLayerDir::VERTICAL
              && tp->isHorizontal() == true)) {
        bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
        frCoord lowCoord = (isH ? getRouteBox().yMin() : getRouteBox().xMin());
        frCoord highCoord = (isH ? getRouteBox().yMax() : getRouteBox().xMax());
        int trackNum
            = (lowCoord - tp->getStartCoord()) / (int) tp->getTrackSpacing();
        trackNum = std::max(trackNum, 0);
        if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
            < lowCoord) {
          trackNum++;
        }
        for (; trackNum < (int) tp->getNumTracks()
               && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                      < highCoord;
             trackNum++) {
          frCoord trackCoord
              = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
          odb::Point via_pt(die_center);
          if (isH) {
            via_pt.setY(trackCoord);
          } else {
            via_pt.setX(trackCoord);
          }
          if (outOfDieVia(lNum, via_pt, die_box)) {
            continue;
          }
          trackCoordSets[lNum].insert(trackCoord);
        }
      }
    }
  }
  for (int i = 0; i < (int) trackCoordSets.size(); i++) {
    for (auto coord : trackCoordSets[i]) {
      trackLocs_[i].push_back(coord);
    }
  }
}

// use prefAp, otherwise return false
bool FlexTAWorker::initIroute_helper_pin(frGuide* guide,
                                         frCoord& maxBegin,
                                         frCoord& minEnd,
                                         std::set<frCoord>& downViaCoordSet,
                                         std::set<frCoord>& upViaCoordSet,
                                         int& nextIrouteDir,
                                         frCoord& pinCoord)
{
  auto [bp, ep] = guide->getPoints();
  if (bp != ep) {
    return false;
  }

  auto net = guide->getNet();
  auto layerNum = guide->getBeginLayerNum();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  bool hasDown = false;
  bool hasUp = false;

  std::vector<frGuide*> nbrGuides;
  auto rq = getRegionQuery();
  odb::Rect box;
  box = odb::Rect(bp, bp);
  nbrGuides.clear();
  if (layerNum - 2 >= router_cfg_->BOTTOM_ROUTING_LAYER) {
    rq->queryGuide(box, layerNum - 2, nbrGuides);
    for (auto& nbrGuide : nbrGuides) {
      if (nbrGuide->getNet() == net) {
        hasDown = true;
        break;
      }
    }
  }
  nbrGuides.clear();
  if (layerNum + 2 < (int) design_->getTech()->getLayers().size()) {
    rq->queryGuide(box, layerNum + 2, nbrGuides);
    for (auto& nbrGuide : nbrGuides) {
      if (nbrGuide->getNet() == net) {
        hasUp = true;
        break;
      }
    }
  }

  std::vector<frBlockObject*> result;
  box = odb::Rect(bp, bp);
  rq->queryGRPin(box, result);

  for (auto& term : result) {
    switch (term->typeId()) {
      case frcInstTerm: {
        auto iterm = static_cast<frInstTerm*>(term);
        if (iterm->getNet() != net) {
          continue;
        }
        frInst* inst = iterm->getInst();
        odb::dbTransform shiftXform = inst->getNoRotationTransform();
        frMTerm* mterm = iterm->getTerm();
        int pinIdx = 0;
        for (auto& pin : mterm->getPins()) {
          if (!pin->hasPinAccess()) {
            pinIdx++;
            continue;
          }
          frAccessPoint* ap
              = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
          if (ap == nullptr) {
            pinIdx++;
            continue;
          }
          odb::Point bp = ap->getPoint();
          auto bNum = ap->getLayerNum();
          shiftXform.apply(bp);
          if (layerNum == bNum && getRouteBox().intersects(bp)) {
            pinCoord = isH ? bp.y() : bp.x();
            maxBegin = isH ? bp.x() : bp.y();
            minEnd = isH ? bp.x() : bp.y();
            nextIrouteDir = 0;
            if (hasDown) {
              downViaCoordSet.insert(maxBegin);
            }
            if (hasUp) {
              upViaCoordSet.insert(maxBegin);
            }
            return true;
          }
          pinIdx++;
        }
        break;
      }
      case frcBTerm: {
        auto bterm = static_cast<frBTerm*>(term);
        if (bterm->getNet() != net) {
          continue;
        }
        for (auto& pin : bterm->getPins()) {
          if (!pin->hasPinAccess()) {
            continue;
          }
          for (auto& ap : pin->getPinAccess(0)->getAccessPoints()) {
            odb::Point bp = ap->getPoint();
            auto bNum = ap->getLayerNum();
            if (layerNum == bNum && getRouteBox().intersects(bp)) {
              pinCoord = isH ? bp.y() : bp.x();
              maxBegin = isH ? bp.x() : bp.y();
              minEnd = isH ? bp.x() : bp.y();
              nextIrouteDir = 0;
              if (hasDown) {
                downViaCoordSet.insert(maxBegin);
              }
              if (hasUp) {
                upViaCoordSet.insert(maxBegin);
              }
              return true;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }

  return false;
}

void FlexTAWorker::initIroute_helper(frGuide* guide,
                                     frCoord& maxBegin,
                                     frCoord& minEnd,
                                     std::set<frCoord>& downViaCoordSet,
                                     std::set<frCoord>& upViaCoordSet,
                                     int& nextIrouteDir,
                                     frCoord& pinCoord)
{
  if (!initIroute_helper_pin(guide,
                             maxBegin,
                             minEnd,
                             downViaCoordSet,
                             upViaCoordSet,
                             nextIrouteDir,
                             pinCoord)) {
    initIroute_helper_generic(guide,
                              maxBegin,
                              minEnd,
                              downViaCoordSet,
                              upViaCoordSet,
                              nextIrouteDir,
                              pinCoord);
  }
}

void FlexTAWorker::initIroute_helper_generic_helper(frGuide* guide,
                                                    frCoord& pinCoord)
{
  auto [bp, ep] = guide->getPoints();
  auto net = guide->getNet();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  auto rq = getRegionQuery();
  std::vector<frBlockObject*> result;

  odb::Rect box;
  box = odb::Rect(bp, bp);
  rq->queryGRPin(box, result);
  if (!(ep == bp)) {
    box = odb::Rect(ep, ep);
    rq->queryGRPin(box, result);
  }
  for (auto& term : result) {
    switch (term->typeId()) {
      case frcInstTerm: {
        auto iterm = static_cast<frInstTerm*>(term);
        if (iterm->getNet() != net) {
          continue;
        }
        frInst* inst = iterm->getInst();
        odb::dbTransform shiftXform = inst->getNoRotationTransform();
        frMTerm* mterm = iterm->getTerm();
        int pinIdx = 0;
        for (auto& pin : mterm->getPins()) {
          if (!pin->hasPinAccess()) {
            pinIdx++;
            continue;
          }
          frAccessPoint* ap
              = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
          if (ap == nullptr) {
            // if ap is nullptr, get first PA from frMPin
            frPinAccess* pa = pin->getPinAccess(0);
            if (pa != nullptr) {
              if (pa->getNumAccessPoints() > 0) {
                // use first ap of frMPin's pin access to set pinCoord of iroute
                ap = pa->getAccessPoint(0);
              } else {
                pinIdx++;
                continue;
              }
            } else {
              pinIdx++;
              continue;
            }
          }
          odb::Point bp = ap->getPoint();
          shiftXform.apply(bp);
          if (getRouteBox().intersects(bp)) {
            pinCoord = isH ? bp.y() : bp.x();
            return;
          }
          pinIdx++;
        }
        break;
      }
      case frcBTerm: {
        auto bterm = static_cast<frBTerm*>(term);
        if (bterm->getNet() != net) {
          continue;
        }
        for (auto& pin : bterm->getPins()) {
          if (!pin->hasPinAccess()) {
            continue;
          }
          for (auto& ap : pin->getPinAccess(0)->getAccessPoints()) {
            odb::Point bp = ap->getPoint();
            if (getRouteBox().intersects(bp)) {
              pinCoord = isH ? bp.y() : bp.x();
              return;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }
}

void FlexTAWorker::initIroute_helper_generic(frGuide* guide,
                                             frCoord& minBegin,
                                             frCoord& maxEnd,
                                             std::set<frCoord>& downViaCoordSet,
                                             std::set<frCoord>& upViaCoordSet,
                                             int& nextIrouteDir,
                                             frCoord& pinCoord)
{
  auto net = guide->getNet();
  auto layerNum = guide->getBeginLayerNum();
  bool hasMinBegin = false;
  bool hasMaxEnd = false;
  minBegin = std::numeric_limits<frCoord>::max();
  maxEnd = std::numeric_limits<frCoord>::min();
  nextIrouteDir = 0;
  // pinCoord       = std::numeric_limits<frCoord>::max();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  downViaCoordSet.clear();
  upViaCoordSet.clear();

  auto [bp, ep] = guide->getPoints();
  odb::Point cp;
  // layerNum in FlexTAWorker
  std::vector<frGuide*> nbrGuides;
  auto rq = getRegionQuery();
  odb::Rect box;
  for (int i = 0; i < 2; i++) {
    nbrGuides.clear();
    // check left
    if (i == 0) {
      box = odb::Rect(bp, bp);
      cp = bp;
      // check right
    } else {
      box = odb::Rect(ep, ep);
      cp = ep;
    }
    if (layerNum - 2 >= router_cfg_->BOTTOM_ROUTING_LAYER) {
      rq->queryGuide(box, layerNum - 2, nbrGuides);
    }
    if (layerNum + 2 < (int) design_->getTech()->getLayers().size()) {
      rq->queryGuide(box, layerNum + 2, nbrGuides);
    }
    for (auto& nbrGuide : nbrGuides) {
      if (nbrGuide->getNet() == net) {
        auto [nbrBp, nbrEp] = nbrGuide->getPoints();
        if (!nbrGuide->hasRoutes()) {
          // via location assumed in center
          auto psLNum = nbrGuide->getBeginLayerNum();
          if (psLNum == layerNum - 2) {
            downViaCoordSet.insert((isH ? nbrBp.x() : nbrBp.y()));
          } else {
            upViaCoordSet.insert((isH ? nbrBp.x() : nbrBp.y()));
          }
        } else {
          for (auto& connFig : nbrGuide->getRoutes()) {
            if (connFig->typeId() == frcPathSeg) {
              auto obj = static_cast<frPathSeg*>(connFig.get());
              auto [nbrSegBegin, ignored] = obj->getPoints();
              auto psLNum = obj->getLayerNum();
              if (i == 0) {
                minBegin = std::min(minBegin,
                                    (isH ? nbrSegBegin.x() : nbrSegBegin.y()));
                hasMinBegin = true;
              } else {
                maxEnd = std::max(maxEnd,
                                  (isH ? nbrSegBegin.x() : nbrSegBegin.y()));
                hasMaxEnd = true;
              }
              if (psLNum == layerNum - 2) {
                downViaCoordSet.insert(
                    (isH ? nbrSegBegin.x() : nbrSegBegin.y()));
              } else {
                upViaCoordSet.insert((isH ? nbrSegBegin.x() : nbrSegBegin.y()));
              }
            }
          }
        }
        if (cp == nbrEp) {
          nextIrouteDir -= 1;
        }
        if (cp == nbrBp) {
          nextIrouteDir += 1;
        }
      }
    }
  }

  if (!hasMinBegin) {
    minBegin = (isH ? bp.x() : bp.y());
  }
  if (!hasMaxEnd) {
    maxEnd = (isH ? ep.x() : ep.y());
  }
  if (minBegin > maxEnd) {
    std::swap(minBegin, maxEnd);
  }
  if (minBegin == maxEnd) {
    maxEnd += 1;
  }

  // pinCoord purely depends on ap regardless of track
  initIroute_helper_generic_helper(guide, pinCoord);
}

void FlexTAWorker::initIroute(frGuide* guide)
{
  auto iroute = std::make_unique<taPin>();
  iroute->setGuide(guide);
  odb::Rect guideBox = guide->getBBox();
  auto layerNum = guide->getBeginLayerNum();
  bool isExt = !(getRouteBox().contains(guideBox));
  if (isExt) {
    // extIroute empty, skip
    if (guide->getRoutes().empty()) {
      return;
    }
  }

  frCoord maxBegin, minEnd;
  std::set<frCoord> downViaCoordSet, upViaCoordSet;
  int nextIrouteDir = 0;
  frCoord pinCoord = std::numeric_limits<frCoord>::max();
  initIroute_helper(guide,
                    maxBegin,
                    minEnd,
                    downViaCoordSet,
                    upViaCoordSet,
                    nextIrouteDir,
                    pinCoord);

  frCoord trackLoc = 0;
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  // set trackIdx
  if (!isInitTA()) {
    for (auto& connFig : guide->getRoutes()) {
      if (connFig->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(connFig.get());
        auto [segBegin, ignored] = obj->getPoints();
        trackLoc = (isH ? segBegin.y() : segBegin.x());
      }
    }
  } else {
    trackLoc = 0;
  }

  std::unique_ptr<taPinFig> ps = std::make_unique<taPathSeg>();
  ps->setNet(guide->getNet());
  auto rptr = static_cast<taPathSeg*>(ps.get());
  if (isH) {
    rptr->setPoints(odb::Point(maxBegin, trackLoc),
                    odb::Point(minEnd, trackLoc));
  } else {
    rptr->setPoints(odb::Point(trackLoc, maxBegin),
                    odb::Point(trackLoc, minEnd));
  }
  rptr->setLayerNum(layerNum);
  if (guide->getNet() && guide->getNet()->getNondefaultRule()) {
    frNonDefaultRule* ndr = guide->getNet()->getNondefaultRule();
    auto style
        = getDesign()->getTech()->getLayer(layerNum)->getDefaultSegStyle();
    style.setWidth(
        std::max((int) style.getWidth(), ndr->getWidth(layerNum / 2 - 1)));
    rptr->setStyle(style);
  } else {
    rptr->setStyle(
        getDesign()->getTech()->getLayer(layerNum)->getDefaultSegStyle());
  }
  // owner set when add to taPin
  iroute->addPinFig(std::move(ps));
  const frViaDef* viaDef;
  for (auto coord : upViaCoordSet) {
    if (guide->getNet()->getNondefaultRule()
        && guide->getNet()->getNondefaultRule()->getPrefVia(layerNum / 2 - 1)) {
      viaDef
          = guide->getNet()->getNondefaultRule()->getPrefVia(layerNum / 2 - 1);
    } else {
      viaDef
          = getDesign()->getTech()->getLayer(layerNum + 1)->getDefaultViaDef();
    }
    std::unique_ptr<taPinFig> via = std::make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto rViaPtr = static_cast<taVia*>(via.get());
    rViaPtr->setOrigin(isH ? odb::Point(coord, trackLoc)
                           : odb::Point(trackLoc, coord));
    iroute->addPinFig(std::move(via));
  }
  for (auto coord : downViaCoordSet) {
    if (guide->getNet()->getNondefaultRule()
        && guide->getNet()->getNondefaultRule()->getPrefVia((layerNum - 2) / 2
                                                            - 1)) {
      viaDef = guide->getNet()->getNondefaultRule()->getPrefVia(
          (layerNum - 2) / 2 - 1);
    } else {
      viaDef
          = getDesign()->getTech()->getLayer(layerNum - 1)->getDefaultViaDef();
    }
    std::unique_ptr<taPinFig> via = std::make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto rViaPtr = static_cast<taVia*>(via.get());
    rViaPtr->setOrigin(isH ? odb::Point(coord, trackLoc)
                           : odb::Point(trackLoc, coord));
    iroute->addPinFig(std::move(via));
  }
  iroute->setNextIrouteDir(nextIrouteDir);
  if (pinCoord < std::numeric_limits<frCoord>::max()) {
    iroute->setPinCoord(pinCoord);
  }
  addIroute(std::move(iroute), isExt);
}

void FlexTAWorker::initIroutes()
{
  frRegionQuery::Objects<frGuide> result;
  auto regionQuery = getRegionQuery();
  for (int lNum = 0; lNum < (int) getDesign()->getTech()->getLayers().size();
       lNum++) {
    auto layer = getDesign()->getTech()->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (layer->getDir() != getDir()) {
      continue;
    }
    result.clear();
    regionQuery->queryGuide(getExtBox(), lNum, result);
    for (auto& [boostb, guide] : result) {
      initIroute(guide);
    }
  }
}

void FlexTAWorker::initCosts()
{
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  frCoord bc, ec;
  // init cost
  if (isInitTA()) {
    for (auto& iroute : iroutes_) {
      auto pitch = getDesign()
                       ->getTech()
                       ->getLayer(iroute->getGuide()->getBeginLayerNum())
                       ->getPitch();
      for (auto& uPinFig : iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          auto obj = static_cast<taPathSeg*>(uPinFig.get());
          auto [bp, ep] = obj->getPoints();
          bc = isH ? bp.x() : bp.y();
          ec = isH ? ep.x() : ep.y();
          iroute->setCost(ec - bc + iroute->hasPinCoord() * pitch * 1000);
        }
      }
    }
  } else {
    auto& workerRegionQuery = getWorkerRegionQuery();
    // update worker rq
    for (auto& iroute : iroutes_) {
      for (auto& uPinFig : iroute->getFigs()) {
        workerRegionQuery.add(uPinFig.get());
        addCost(uPinFig.get());
      }
    }
    for (auto& iroute : extIroutes_) {
      for (auto& uPinFig : iroute->getFigs()) {
        workerRegionQuery.add(uPinFig.get());
        addCost(uPinFig.get());
      }
    }
    // update iroute cost
    for (auto& iroute : iroutes_) {
      frUInt4 drcCost = 0;
      frCoord trackLoc = std::numeric_limits<frCoord>::max();
      for (auto& uPinFig : iroute->getFigs()) {
        if (uPinFig->typeId() == tacPathSeg) {
          auto [bp, ep] = static_cast<taPathSeg*>(uPinFig.get())->getPoints();
          if (isH) {
            trackLoc = bp.y();
          } else {
            trackLoc = bp.x();
          }
          break;
        }
      }
      if (trackLoc == std::numeric_limits<frCoord>::max()) {
        std::cout << "Error: FlexTAWorker::initCosts does not find trackLoc"
                  << '\n';
        exit(1);
      }
      assignIroute_getCost(iroute.get(), trackLoc, drcCost);
      iroute->setCost(drcCost);
      totCost_ += drcCost;
    }
  }
}

void FlexTAWorker::sortIroutes()
{
  // init cost
  if (isInitTA()) {
    for (auto& iroute : iroutes_) {
      if (hardIroutesMode_ == iroute->getGuide()->getNet()->isClock()) {
        addToReassignIroutes(iroute.get());
      }
    }
  } else {
    for (auto& iroute : iroutes_) {
      if (iroute->getCost()) {
        if (hardIroutesMode_ == iroute->getGuide()->getNet()->isClock()) {
          addToReassignIroutes(iroute.get());
        }
      }
    }
  }
}

void FlexTAWorker::initFixedObjs_helper(const odb::Rect& box,
                                        frCoord bloatDist,
                                        frLayerNum lNum,
                                        frNet* net,
                                        bool isViaCost)
{
  odb::Rect bloatBox;
  box.bloat(bloatDist, bloatBox);
  auto con = getDesign()->getTech()->getLayer(lNum)->getShortConstraint();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  int idx1, idx2;
  // frCoord x1, x2;
  if (isH) {
    getTrackIdx(bloatBox.yMin(), bloatBox.yMax(), lNum, idx1, idx2);
  } else {
    getTrackIdx(bloatBox.xMin(), bloatBox.xMax(), lNum, idx1, idx2);
  }
  auto& trackLocs = getTrackLocs(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx1; i <= idx2; i++) {
    // new
    // auto &track = tracks[i];
    // track.addToCost(net, x1, x2, 0);
    // track.addToCost(net, x1, x2, 1);
    // track.addToCost(net, x1, x2, 2);
    // old
    auto trackLoc = trackLocs[i];
    odb::Rect tmpBox;
    if (isH) {
      tmpBox.init(bloatBox.xMin(), trackLoc, bloatBox.xMax(), trackLoc);
    } else {
      tmpBox.init(trackLoc, bloatBox.yMin(), trackLoc, bloatBox.yMax());
    }
    if (isViaCost) {
      workerRegionQuery.addViaCost(tmpBox, lNum, net, con);
    } else {
      workerRegionQuery.addCost(tmpBox, lNum, net, con);
    }
  }
}

void FlexTAWorker::initFixedObjs()
{
  frRegionQuery::Objects<frBlockObject> result;
  odb::Rect box;
  frCoord width = 0;
  frCoord bloatDist = 0;
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    result.clear();
    frLayer* layer = getTech()->getLayer(layerNum);
    if (layer->getType() != dbTechLayerType::ROUTING
        || layer->getDir() != getDir()) {
      continue;
    }
    width = layer->getWidth();
    getRegionQuery()->query(getExtBox(), layerNum, result);
    for (auto& [bounds, obj] : result) {
      bounds.bloat(-1, box);
      auto type = obj->typeId();
      // instterm term
      if (type == frcInstTerm || type == frcBTerm) {
        bloatDist = router_cfg_->TASHAPEBLOATWIDTH * width;
        frNet* netPtr = nullptr;
        if (type == frcBTerm) {
          netPtr = static_cast<frBTerm*>(obj)->getNet();
        } else {
          netPtr = static_cast<frInstTerm*>(obj)->getNet();
        }
        initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
        // snet
      } else if (type == frcPathSeg || type == frcVia) {
        bloatDist = initFixedObjs_calcBloatDist(obj, layerNum, bounds);
        frNet* netPtr = nullptr;
        if (type == frcPathSeg) {
          netPtr = static_cast<frPathSeg*>(obj)->getNet();
        } else {
          netPtr = static_cast<frVia*>(obj)->getNet();
        }
        initFixedObjs_helper(box, bloatDist, layerNum, netPtr);
        if (getTech()->getLayer(layerNum)->getType()
            == dbTechLayerType::ROUTING) {
          // down-via
          if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum()
              && getTech()->getLayer(layerNum - 2)->getType()
                     == dbTechLayerType::ROUTING) {
            auto cutLayer = getTech()->getLayer(layerNum - 1);
            auto via = std::make_unique<frVia>(cutLayer->getDefaultViaDef());
            odb::Rect viaBox = via->getLayer2BBox();
            frCoord viaWidth = viaBox.minDXDY();
            // only add for fat via
            if (viaWidth > width) {
              bloatDist = initFixedObjs_calcOBSBloatDistVia(
                  cutLayer->getDefaultViaDef(), layerNum, bounds, false);
              initFixedObjs_helper(box, bloatDist, layerNum, netPtr, true);
            }
          }
          // up-via
          if (layerNum + 2 < (int) design_->getTech()->getLayers().size()
              && getTech()->getLayer(layerNum + 2)->getType()
                     == dbTechLayerType::ROUTING) {
            auto cutLayer = getTech()->getLayer(layerNum + 1);
            auto via = std::make_unique<frVia>(cutLayer->getDefaultViaDef());
            odb::Rect viaBox = via->getLayer1BBox();
            frCoord viaWidth = viaBox.minDXDY();
            // only add for fat via
            if (viaWidth > width) {
              bloatDist = initFixedObjs_calcOBSBloatDistVia(
                  cutLayer->getDefaultViaDef(), layerNum, bounds, false);
              initFixedObjs_helper(box, bloatDist, layerNum, netPtr, true);
            }
          }
        }
      } else if (type == frcBlockage || type == frcInstBlockage) {
        bloatDist = initFixedObjs_calcBloatDist(obj, layerNum, bounds);
        initFixedObjs_helper(box, bloatDist, layerNum, nullptr);
      } else {
        std::cout << "Warning: unsupported type in initFixedObjs\n";
      }
    }
    auto costResults = [this, layerNum, width](
                           bool upper,
                           const frRegionQuery::Objects<frBlockObject>&
                               result) {
      odb::Rect box;
      for (auto& [bounds, obj] : result) {
        bounds.bloat(-1, box);
        auto type = obj->typeId();
        switch (type) {
          case frcInstBlockage: {
            auto instBlkg = (static_cast<frInstBlockage*>(obj));
            auto inst = instBlkg->getInst();
            odb::dbMasterType masterType = inst->getMaster()->getMasterType();
            if (!masterType.isBlock() && !masterType.isPad()
                && masterType != odb::dbMasterType::RING) {
              continue;
            }
            if (bounds.minDXDY() <= 2 * width) {
              continue;
            }
            auto cutLayer
                = getTech()->getLayer(upper ? layerNum + 1 : layerNum - 1);
            auto bloatDist = initFixedObjs_calcOBSBloatDistVia(
                cutLayer->getDefaultViaDef(), layerNum, bounds);
            odb::Rect bloatBox;
            box.bloat(bloatDist, bloatBox);

            odb::Rect borderBox(
                bloatBox.xMin(), bloatBox.yMin(), box.xMin(), bloatBox.yMax());
            initFixedObjs_helper(borderBox, 0, layerNum, nullptr, true);
            borderBox.init(
                bloatBox.xMin(), box.yMax(), bloatBox.xMax(), bloatBox.yMax());
            initFixedObjs_helper(borderBox, 0, layerNum, nullptr, true);
            borderBox.init(
                box.xMax(), bloatBox.yMin(), bloatBox.xMax(), bloatBox.yMax());
            initFixedObjs_helper(borderBox, 0, layerNum, nullptr, true);
            borderBox.init(
                bloatBox.xMin(), bloatBox.yMin(), bloatBox.xMax(), box.yMin());
            initFixedObjs_helper(borderBox, 0, layerNum, nullptr, true);
            break;
          }
          default:
            break;
        }
      }
    };

    result.clear();
    if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum()
        && getTech()->getLayer(layerNum - 2)->getType()
               == dbTechLayerType::ROUTING) {
      getRegionQuery()->query(getExtBox(), layerNum - 2, result);
    }
    costResults(false, result);
    result.clear();
    if (layerNum + 2 < getDesign()->getTech()->getLayers().size()
        && getTech()->getLayer(layerNum + 2)->getType()
               == dbTechLayerType::ROUTING) {
      getRegionQuery()->query(getExtBox(), layerNum + 2, result);
    }
    costResults(true, result);
  }
}

frCoord FlexTAWorker::initFixedObjs_calcOBSBloatDistVia(const frViaDef* viaDef,
                                                        const frLayerNum lNum,
                                                        const odb::Rect& box,
                                                        bool isOBS)
{
  auto layer = getTech()->getLayer(lNum);
  odb::Rect viaBox;
  auto via = std::make_unique<frVia>(viaDef);
  if (viaDef->getLayer1Num() == lNum) {
    viaBox = via->getLayer1BBox();
  } else {
    viaBox = via->getLayer2BBox();
  }
  frCoord viaWidth = viaBox.minDXDY();
  frCoord viaLength = viaBox.maxDXDY();

  frCoord obsWidth = box.minDXDY();
  if (router_cfg_->USEMINSPACING_OBS && isOBS) {
    obsWidth = layer->getWidth();
  }

  frCoord bloatDist
      = layer->getMinSpacingValue(obsWidth, viaWidth, viaWidth, false);
  if (bloatDist < 0) {
    logger_->error(
        DRT, 140, "Layer {} has negative min spacing value.", layer->getName());
  }
  auto& eol = layer->getDrEolSpacingConstraint();
  if (viaBox.minDXDY() < eol.eolWidth) {
    bloatDist = std::max(bloatDist, eol.eolSpace);
  }
  // at least via enclosure should not short with obs (OBS has issue with
  // wrongway and PG has issue with prefDir)
  // TODO: generalize the following
  if (isOBS) {
    bloatDist += viaLength / 2;
  } else {
    bloatDist += viaWidth / 2;
  }
  return bloatDist;
}

frCoord FlexTAWorker::initFixedObjs_calcBloatDist(frBlockObject* obj,
                                                  const frLayerNum lNum,
                                                  const odb::Rect& box)
{
  auto layer = getTech()->getLayer(lNum);
  frCoord width = layer->getWidth();
  frCoord objWidth = box.minDXDY();
  frCoord prl
      = (layer->getDir() == dbTechLayerDir::HORIZONTAL) ? box.dx() : box.dy();
  if (obj->typeId() == frcBlockage || obj->typeId() == frcInstBlockage) {
    if (router_cfg_->USEMINSPACING_OBS) {
      objWidth = width;
    }
  }

  // use width if minSpc does not exist
  frCoord bloatDist = width;
  if (layer->hasMinSpacing()) {
    bloatDist = layer->getMinSpacingValue(objWidth, width, prl, false);
    if (bloatDist < 0) {
      logger_->error(DRT,
                     144,
                     "Layer {} has negative min spacing value.",
                     layer->getName());
    }
  }
  // assuming the wire width is width
  bloatDist += width / 2;
  return bloatDist;
}

void FlexTAWorker::init()
{
  rq_.init();
  initTracks();
  initFixedObjs();
  initIroutes();
  initCosts();
}

}  // namespace drt
