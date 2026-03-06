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
  const int num_layers = getDesign()->getTech()->getLayers().size();
  trackLocs_.resize(num_layers);
  std::vector<std::set<frCoord>> track_coord_sets(num_layers);
  // uPtr for tp
  auto die_box = getDesign()->getTopBlock()->getDieBox();
  auto die_center = die_box.center();
  for (int layer_num = 0; layer_num < (int) num_layers; layer_num++) {
    auto layer = getDesign()->getTech()->getLayer(layer_num);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (layer->getDir() != getDir()) {
      continue;
    }
    for (auto& tp : getDesign()->getTopBlock()->getTrackPatterns(layer_num)) {
      if ((getDir() == dbTechLayerDir::HORIZONTAL
           && tp->isHorizontal() == false)
          || (getDir() == dbTechLayerDir::VERTICAL
              && tp->isHorizontal() == true)) {
        bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
        frCoord low_coord
            = (is_horizontal ? getRouteBox().yMin() : getRouteBox().xMin());
        frCoord high_coord
            = (is_horizontal ? getRouteBox().yMax() : getRouteBox().xMax());
        int track_num
            = (low_coord - tp->getStartCoord()) / (int) tp->getTrackSpacing();
        track_num = std::max(track_num, 0);
        if (track_num * (int) tp->getTrackSpacing() + tp->getStartCoord()
            < low_coord) {
          track_num++;
        }
        for (; track_num < (int) tp->getNumTracks()
               && track_num * (int) tp->getTrackSpacing() + tp->getStartCoord()
                      < high_coord;
             track_num++) {
          frCoord trackCoord
              = track_num * tp->getTrackSpacing() + tp->getStartCoord();
          odb::Point via_pt(die_center);
          if (is_horizontal) {
            via_pt.setY(trackCoord);
          } else {
            via_pt.setX(trackCoord);
          }
          if (outOfDieVia(layer_num, via_pt, die_box)) {
            continue;
          }
          track_coord_sets[layer_num].insert(trackCoord);
        }
      }
    }
  }
  for (int i = 0; i < (int) track_coord_sets.size(); i++) {
    for (auto coord : track_coord_sets[i]) {
      trackLocs_[i].push_back(coord);
    }
  }
}

bool FlexTAWorker::initIroute_helper_pin_iterm(
    frInstTerm* iterm,
    frNet* net,
    frLayerNum layer_num,
    bool is_horizontal,
    bool has_down,
    bool has_up,
    frCoord& max_begin,
    frCoord& min_end,
    std::set<frCoord>& down_via_coord_set,
    std::set<frCoord>& up_via_coord_set,
    int& next_iroute_dir,
    frCoord& pin_coord)
{
  if (iterm->getNet() != net) {
    return false;
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
    frAccessPoint* ap = (iterm->getAccessPoints())[pinIdx];
    if (ap == nullptr) {
      pinIdx++;
      continue;
    }
    odb::Point bp = ap->getPoint();
    auto bNum = ap->getLayerNum();
    shiftXform.apply(bp);
    if (layer_num == bNum && getRouteBox().intersects(bp)) {
      pin_coord = is_horizontal ? bp.y() : bp.x();
      max_begin = is_horizontal ? bp.x() : bp.y();
      min_end = is_horizontal ? bp.x() : bp.y();
      next_iroute_dir = 0;
      if (has_down) {
        down_via_coord_set.insert(max_begin);
      }
      if (has_up) {
        up_via_coord_set.insert(max_begin);
      }
      return true;
    }
    pinIdx++;
  }
  return false;
}

bool FlexTAWorker::initIroute_helper_pin_bterm(
    frBTerm* bterm,
    frNet* net,
    frLayerNum layer_num,
    bool is_horizontal,
    bool has_down,
    bool has_up,
    frCoord& max_begin,
    frCoord& min_end,
    std::set<frCoord>& down_via_coord_set,
    std::set<frCoord>& up_via_coord_set,
    int& next_iroute_dir,
    frCoord& pin_coord)
{
  if (bterm->getNet() != net) {
    return false;
  }
  for (auto& pin : bterm->getPins()) {
    if (!pin->hasPinAccess()) {
      continue;
    }
    for (auto& ap : pin->getPinAccess(0)->getAccessPoints()) {
      odb::Point bp = ap->getPoint();
      auto bNum = ap->getLayerNum();
      if (layer_num == bNum && getRouteBox().intersects(bp)) {
        pin_coord = is_horizontal ? bp.y() : bp.x();
        max_begin = is_horizontal ? bp.x() : bp.y();
        min_end = is_horizontal ? bp.x() : bp.y();
        next_iroute_dir = 0;
        if (has_down) {
          down_via_coord_set.insert(max_begin);
        }
        if (has_up) {
          up_via_coord_set.insert(max_begin);
        }
        return true;
      }
    }
  }
  return false;
}

// use prefAp, otherwise return false
bool FlexTAWorker::initIroute_helper_pin(frGuide* guide,
                                         frCoord& max_begin,
                                         frCoord& min_end,
                                         std::set<frCoord>& down_via_coord_set,
                                         std::set<frCoord>& up_via_coord_set,
                                         int& next_iroute_dir,
                                         frCoord& pin_coord)
{
  auto [bp, ep] = guide->getPoints();
  if (bp != ep) {
    return false;
  }

  auto net = guide->getNet();
  auto layer_num = guide->getBeginLayerNum();
  bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
  bool has_down = false;
  bool has_up = false;

  std::vector<frGuide*> nbr_guides;
  auto rq = getRegionQuery();
  odb::Rect box;
  box = odb::Rect(bp, bp);
  nbr_guides.clear();
  if (layer_num - 2 >= router_cfg_->BOTTOM_ROUTING_LAYER) {
    rq->queryGuide(box, layer_num - 2, nbr_guides);
    for (auto& nbrGuide : nbr_guides) {
      if (nbrGuide->getNet() == net) {
        has_down = true;
        break;
      }
    }
  }
  nbr_guides.clear();
  if (layer_num + 2 < (int) design_->getTech()->getLayers().size()) {
    rq->queryGuide(box, layer_num + 2, nbr_guides);
    for (auto& nbrGuide : nbr_guides) {
      if (nbrGuide->getNet() == net) {
        has_up = true;
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
        if (initIroute_helper_pin_iterm(iterm,
                                        net,
                                        layer_num,
                                        is_horizontal,
                                        has_down,
                                        has_up,
                                        max_begin,
                                        min_end,
                                        down_via_coord_set,
                                        up_via_coord_set,
                                        next_iroute_dir,
                                        pin_coord)) {
          return true;
        }
        break;
      }
      case frcBTerm: {
        auto bterm = static_cast<frBTerm*>(term);
        if (initIroute_helper_pin_bterm(bterm,
                                        net,
                                        layer_num,
                                        is_horizontal,
                                        has_down,
                                        has_up,
                                        max_begin,
                                        min_end,
                                        down_via_coord_set,
                                        up_via_coord_set,
                                        next_iroute_dir,
                                        pin_coord)) {
          return true;
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
                                     frCoord& max_begin,
                                     frCoord& min_end,
                                     std::set<frCoord>& down_via_coord_set,
                                     std::set<frCoord>& up_via_coord_set,
                                     int& next_iroute_dir,
                                     frCoord& pin_coord)
{
  if (!initIroute_helper_pin(guide,
                             max_begin,
                             min_end,
                             down_via_coord_set,
                             up_via_coord_set,
                             next_iroute_dir,
                             pin_coord)) {
    initIroute_helper_generic(guide,
                              max_begin,
                              min_end,
                              down_via_coord_set,
                              up_via_coord_set,
                              next_iroute_dir,
                              pin_coord);
  }
}

void FlexTAWorker::initIroute_helper_generic_helper(frGuide* guide,
                                                    frCoord& pin_coord)
{
  auto [bp, ep] = guide->getPoints();
  auto net = guide->getNet();
  bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);

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
                // use first ap of frMPin's pin access to set pin_coord of
                // iroute
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
            pin_coord = is_horizontal ? bp.y() : bp.x();
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
              pin_coord = is_horizontal ? bp.y() : bp.x();
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

void FlexTAWorker::initIroute_helper_generic_endpoint(
    frGuide* guide,
    const odb::Point& cp,
    bool is_begin,
    frLayerNum layer_num,
    bool is_horizontal,
    frCoord& min_begin,
    frCoord& max_end,
    bool& has_min_begin,
    bool& has_max_end,
    std::set<frCoord>& down_via_coord_set,
    std::set<frCoord>& up_via_coord_set,
    int& next_iroute_dir)
{
  auto net = guide->getNet();
  std::vector<frGuide*> nbr_guides;
  odb::Rect box(cp, cp);
  auto rq = getRegionQuery();

  if (layer_num - 2 >= router_cfg_->BOTTOM_ROUTING_LAYER) {
    rq->queryGuide(box, layer_num - 2, nbr_guides);
  }
  if (layer_num + 2 < (int) design_->getTech()->getLayers().size()) {
    rq->queryGuide(box, layer_num + 2, nbr_guides);
  }

  for (auto* nbr_guide : nbr_guides) {
    if (nbr_guide->getNet() != net) {
      continue;
    }

    auto [nbr_begin, nbr_end] = nbr_guide->getPoints();
    if (!nbr_guide->hasRoutes()) {
      const auto ps_layer_num = nbr_guide->getBeginLayerNum();
      const frCoord coord = is_horizontal ? nbr_begin.x() : nbr_begin.y();
      if (ps_layer_num == layer_num - 2) {
        down_via_coord_set.insert(coord);
      } else {
        up_via_coord_set.insert(coord);
      }
    } else {
      for (auto& conn_fig : nbr_guide->getRoutes()) {
        if (conn_fig->typeId() != frcPathSeg) {
          continue;
        }

        auto* path_seg = static_cast<frPathSeg*>(conn_fig.get());
        auto [nbr_seg_begin, ignored] = path_seg->getPoints();
        const auto ps_layer_num = path_seg->getLayerNum();
        const frCoord coord
            = is_horizontal ? nbr_seg_begin.x() : nbr_seg_begin.y();

        if (is_begin) {
          min_begin = std::min(min_begin, coord);
          has_min_begin = true;
        } else {
          max_end = std::max(max_end, coord);
          has_max_end = true;
        }

        if (ps_layer_num == layer_num - 2) {
          down_via_coord_set.insert(coord);
        } else {
          up_via_coord_set.insert(coord);
        }
      }
    }

    if (cp == nbr_end) {
      next_iroute_dir -= 1;
    }
    if (cp == nbr_begin) {
      next_iroute_dir += 1;
    }
  }
}

void FlexTAWorker::initIroute_helper_generic(
    frGuide* guide,
    frCoord& min_begin,
    frCoord& max_end,
    std::set<frCoord>& down_via_coord_set,
    std::set<frCoord>& up_via_coord_set,
    int& next_iroute_dir,
    frCoord& pin_coord)
{
  auto layer_num = guide->getBeginLayerNum();
  bool has_min_begin = false;
  bool has_max_end = false;
  min_begin = std::numeric_limits<frCoord>::max();
  max_end = std::numeric_limits<frCoord>::min();
  next_iroute_dir = 0;
  // pin_coord       = std::numeric_limits<frCoord>::max();
  bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
  down_via_coord_set.clear();
  up_via_coord_set.clear();

  auto [bp, ep] = guide->getPoints();
  initIroute_helper_generic_endpoint(guide,
                                     bp,
                                     true,
                                     layer_num,
                                     is_horizontal,
                                     min_begin,
                                     max_end,
                                     has_min_begin,
                                     has_max_end,
                                     down_via_coord_set,
                                     up_via_coord_set,
                                     next_iroute_dir);
  initIroute_helper_generic_endpoint(guide,
                                     ep,
                                     false,
                                     layer_num,
                                     is_horizontal,
                                     min_begin,
                                     max_end,
                                     has_min_begin,
                                     has_max_end,
                                     down_via_coord_set,
                                     up_via_coord_set,
                                     next_iroute_dir);

  if (!has_min_begin) {
    min_begin = (is_horizontal ? bp.x() : bp.y());
  }
  if (!has_max_end) {
    max_end = (is_horizontal ? ep.x() : ep.y());
  }
  if (min_begin > max_end) {
    std::swap(min_begin, max_end);
  }
  if (min_begin == max_end) {
    max_end += 1;
  }

  // pin_coord purely depends on ap regardless of track
  initIroute_helper_generic_helper(guide, pin_coord);
}

void FlexTAWorker::initIroute(frGuide* guide)
{
  auto iroute = std::make_unique<taPin>();
  iroute->setGuide(guide);
  odb::Rect guide_box = guide->getBBox();
  auto layer_num = guide->getBeginLayerNum();
  bool is_ext = !(getRouteBox().contains(guide_box));
  if (is_ext) {
    // extIroute empty, skip
    if (guide->getRoutes().empty()) {
      return;
    }
  }

  frCoord max_begin, min_end;
  std::set<frCoord> down_via_coord_set, up_via_coord_set;
  int next_iroute_dir = 0;
  frCoord pin_coord = std::numeric_limits<frCoord>::max();
  initIroute_helper(guide,
                    max_begin,
                    min_end,
                    down_via_coord_set,
                    up_via_coord_set,
                    next_iroute_dir,
                    pin_coord);

  frCoord track_loc = 0;
  bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
  // set trackIdx
  if (!isInitTA()) {
    for (auto& connFig : guide->getRoutes()) {
      if (connFig->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(connFig.get());
        auto [segBegin, ignored] = obj->getPoints();
        track_loc = (is_horizontal ? segBegin.y() : segBegin.x());
      }
    }
  } else {
    track_loc = 0;
  }

  std::unique_ptr<taPinFig> ps = std::make_unique<taPathSeg>();
  ps->setNet(guide->getNet());
  auto r_ptr = static_cast<taPathSeg*>(ps.get());
  if (is_horizontal) {
    r_ptr->setPoints(odb::Point(max_begin, track_loc),
                     odb::Point(min_end, track_loc));
  } else {
    r_ptr->setPoints(odb::Point(track_loc, max_begin),
                     odb::Point(track_loc, min_end));
  }
  r_ptr->setLayerNum(layer_num);
  if (guide->getNet() && guide->getNet()->getNondefaultRule()) {
    frNonDefaultRule* ndr = guide->getNet()->getNondefaultRule();
    auto style
        = getDesign()->getTech()->getLayer(layer_num)->getDefaultSegStyle();
    style.setWidth(
        std::max((int) style.getWidth(), ndr->getWidth(layer_num / 2 - 1)));
    r_ptr->setStyle(style);
  } else {
    r_ptr->setStyle(
        getDesign()->getTech()->getLayer(layer_num)->getDefaultSegStyle());
  }
  // owner set when add to taPin
  iroute->addPinFig(std::move(ps));
  const frViaDef* viaDef;
  for (auto coord : up_via_coord_set) {
    if (guide->getNet()->getNondefaultRule()
        && guide->getNet()->getNondefaultRule()->getPrefVia(layer_num / 2
                                                            - 1)) {
      viaDef
          = guide->getNet()->getNondefaultRule()->getPrefVia(layer_num / 2 - 1);
    } else {
      viaDef
          = getDesign()->getTech()->getLayer(layer_num + 1)->getDefaultViaDef();
    }
    std::unique_ptr<taPinFig> via = std::make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto r_via_ptr = static_cast<taVia*>(via.get());
    r_via_ptr->setOrigin(is_horizontal ? odb::Point(coord, track_loc)
                                       : odb::Point(track_loc, coord));
    iroute->addPinFig(std::move(via));
  }
  for (auto coord : down_via_coord_set) {
    if (guide->getNet()->getNondefaultRule()
        && guide->getNet()->getNondefaultRule()->getPrefVia((layer_num - 2) / 2
                                                            - 1)) {
      viaDef = guide->getNet()->getNondefaultRule()->getPrefVia(
          (layer_num - 2) / 2 - 1);
    } else {
      viaDef
          = getDesign()->getTech()->getLayer(layer_num - 1)->getDefaultViaDef();
    }
    std::unique_ptr<taPinFig> via = std::make_unique<taVia>(viaDef);
    via->setNet(guide->getNet());
    auto r_via_ptr = static_cast<taVia*>(via.get());
    r_via_ptr->setOrigin(is_horizontal ? odb::Point(coord, track_loc)
                                       : odb::Point(track_loc, coord));
    iroute->addPinFig(std::move(via));
  }
  iroute->setNextIrouteDir(next_iroute_dir);
  if (pin_coord < std::numeric_limits<frCoord>::max()) {
    iroute->setPinCoord(pin_coord);
  }
  addIroute(std::move(iroute), is_ext);
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
                                        frCoord bloat_dist,
                                        frLayerNum layer_num,
                                        frNet* net_ptr,
                                        bool isViaCost)
{
  odb::Rect bloatBox;
  box.bloat(bloat_dist, bloatBox);
  auto con = getDesign()->getTech()->getLayer(layer_num)->getShortConstraint();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  int idx1, idx2;
  // frCoord x1, x2;
  if (isH) {
    getTrackIdx(bloatBox.yMin(), bloatBox.yMax(), layer_num, idx1, idx2);
  } else {
    getTrackIdx(bloatBox.xMin(), bloatBox.xMax(), layer_num, idx1, idx2);
  }
  auto& trackLocs = getTrackLocs(layer_num);
  auto& workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx1; i <= idx2; i++) {
    // new
    // auto &track = tracks[i];
    // track.addToCost(net_ptr, x1, x2, 0);
    // track.addToCost(net_ptr, x1, x2, 1);
    // track.addToCost(net_ptr, x1, x2, 2);
    // old
    auto trackLoc = trackLocs[i];
    odb::Rect tmpBox;
    if (isH) {
      tmpBox.init(bloatBox.xMin(), trackLoc, bloatBox.xMax(), trackLoc);
    } else {
      tmpBox.init(trackLoc, bloatBox.yMin(), trackLoc, bloatBox.yMax());
    }
    if (isViaCost) {
      workerRegionQuery.addViaCost(tmpBox, layer_num, net_ptr, con);
    } else {
      workerRegionQuery.addCost(tmpBox, layer_num, net_ptr, con);
    }
  }
}

void FlexTAWorker::initFixedObjs_processTerm(frBlockObject* obj,
                                             frLayerNum layer_num,
                                             const odb::Rect& bounds,
                                             odb::Rect& box,
                                             frCoord width)
{
  frCoord bloat_dist = router_cfg_->TASHAPEBLOATWIDTH * width;
  frNet* net_ptr = nullptr;
  if (obj->typeId() == frcBTerm) {
    net_ptr = static_cast<frBTerm*>(obj)->getNet();
  } else {
    net_ptr = static_cast<frInstTerm*>(obj)->getNet();
  }
  initFixedObjs_helper(box, bloat_dist, layer_num, net_ptr);
}

void FlexTAWorker::initFixedObjs_processVia(frBlockObject* obj,
                                            frLayerNum layer_num,
                                            const odb::Rect& bounds,
                                            odb::Rect& box,
                                            frCoord width,
                                            frNet* net_ptr)
{
  frCoord bloat_dist = 0;
  // down-via
  if (layer_num - 2 >= getDesign()->getTech()->getBottomLayerNum()
      && getTech()->getLayer(layer_num - 2)->getType()
             == dbTechLayerType::ROUTING) {
    auto cut_layer = getTech()->getLayer(layer_num - 1);
    auto via = std::make_unique<frVia>(cut_layer->getDefaultViaDef());
    odb::Rect via_box = via->getLayer2BBox();
    frCoord via_width = via_box.minDXDY();
    // only add for fat via
    if (via_width > width) {
      bloat_dist = initFixedObjs_calcOBSBloatDistVia(
          cut_layer->getDefaultViaDef(), layer_num, bounds, false);
      initFixedObjs_helper(box, bloat_dist, layer_num, net_ptr, true);
    }
  }
  // up-via
  if (layer_num + 2 < (int) design_->getTech()->getLayers().size()
      && getTech()->getLayer(layer_num + 2)->getType()
             == dbTechLayerType::ROUTING) {
    auto cut_layer = getTech()->getLayer(layer_num + 1);
    auto via = std::make_unique<frVia>(cut_layer->getDefaultViaDef());
    odb::Rect via_box = via->getLayer1BBox();
    frCoord via_width = via_box.minDXDY();
    // only add for fat via
    if (via_width > width) {
      bloat_dist = initFixedObjs_calcOBSBloatDistVia(
          cut_layer->getDefaultViaDef(), layer_num, bounds, false);
      initFixedObjs_helper(box, bloat_dist, layer_num, net_ptr, true);
    }
  }
}

void FlexTAWorker::initFixedObjs_processRouting(frBlockObject* obj,
                                                frLayerNum layer_num,
                                                const odb::Rect& bounds,
                                                odb::Rect& box,
                                                frCoord width)
{
  frCoord bloat_dist = initFixedObjs_calcBloatDist(obj, layer_num, bounds);
  frNet* net_ptr = nullptr;
  if (obj->typeId() == frcPathSeg) {
    net_ptr = static_cast<frPathSeg*>(obj)->getNet();
  } else {
    net_ptr = static_cast<frVia*>(obj)->getNet();
  }
  initFixedObjs_helper(box, bloat_dist, layer_num, net_ptr);
  if (getTech()->getLayer(layer_num)->getType() == dbTechLayerType::ROUTING) {
    initFixedObjs_processVia(obj, layer_num, bounds, box, width, net_ptr);
  }
}

void FlexTAWorker::initFixedObjs_applyBorderViaCosts(
    frLayerNum layer_num,
    frCoord width,
    bool upper,
    const frRegionQuery::Objects<frBlockObject>& result)
{
  for (auto& [bounds, obj] : result) {
    odb::Rect box;
    bounds.bloat(-1, box);
    if (obj->typeId() != frcInstBlockage) {
      continue;
    }

    auto* inst_blkg = static_cast<frInstBlockage*>(obj);
    auto* inst = inst_blkg->getInst();
    const odb::dbMasterType master_type = inst->getMaster()->getMasterType();
    if (!master_type.isBlock() && !master_type.isPad()
        && master_type != odb::dbMasterType::RING) {
      continue;
    }
    if (bounds.minDXDY() <= 2 * width) {
      continue;
    }

    auto* cut_layer
        = getTech()->getLayer(upper ? layer_num + 1 : layer_num - 1);
    const auto bloat_dist = initFixedObjs_calcOBSBloatDistVia(
        cut_layer->getDefaultViaDef(), layer_num, bounds);
    odb::Rect bloat_box;
    box.bloat(bloat_dist, bloat_box);

    odb::Rect border_box(
        bloat_box.xMin(), bloat_box.yMin(), box.xMin(), bloat_box.yMax());
    initFixedObjs_helper(border_box, 0, layer_num, nullptr, true);
    border_box.init(
        bloat_box.xMin(), box.yMax(), bloat_box.xMax(), bloat_box.yMax());
    initFixedObjs_helper(border_box, 0, layer_num, nullptr, true);
    border_box.init(
        box.xMax(), bloat_box.yMin(), bloat_box.xMax(), bloat_box.yMax());
    initFixedObjs_helper(border_box, 0, layer_num, nullptr, true);
    border_box.init(
        bloat_box.xMin(), bloat_box.yMin(), bloat_box.xMax(), box.yMin());
    initFixedObjs_helper(border_box, 0, layer_num, nullptr, true);
  }
}

void FlexTAWorker::initFixedObjs()
{
  frRegionQuery::Objects<frBlockObject> result;
  odb::Rect box;
  frCoord width = 0;
  frCoord bloat_dist = 0;
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
        initFixedObjs_processTerm(obj, layerNum, bounds, box, width);
        // snet
      } else if (type == frcPathSeg || type == frcVia) {
        initFixedObjs_processRouting(obj, layerNum, bounds, box, width);
      } else if (type == frcBlockage || type == frcInstBlockage) {
        bloat_dist = initFixedObjs_calcBloatDist(obj, layerNum, bounds);
        initFixedObjs_helper(box, bloat_dist, layerNum, nullptr);
      } else {
        std::cout << "Warning: unsupported type in initFixedObjs\n";
      }
    }
    result.clear();
    if (layerNum - 2 >= getDesign()->getTech()->getBottomLayerNum()
        && getTech()->getLayer(layerNum - 2)->getType()
               == dbTechLayerType::ROUTING) {
      getRegionQuery()->query(getExtBox(), layerNum - 2, result);
    }
    initFixedObjs_applyBorderViaCosts(layerNum, width, false, result);
    result.clear();
    if (layerNum + 2 < getDesign()->getTech()->getLayers().size()
        && getTech()->getLayer(layerNum + 2)->getType()
               == dbTechLayerType::ROUTING) {
      getRegionQuery()->query(getExtBox(), layerNum + 2, result);
    }
    initFixedObjs_applyBorderViaCosts(layerNum, width, true, result);
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

  frCoord bloat_dist
      = layer->getMinSpacingValue(obsWidth, viaWidth, viaWidth, false);
  if (bloat_dist < 0) {
    logger_->error(
        DRT, 140, "Layer {} has negative min spacing value.", layer->getName());
  }
  auto& eol = layer->getDrEolSpacingConstraint();
  if (viaBox.minDXDY() < eol.eolWidth) {
    bloat_dist = std::max(bloat_dist, eol.eolSpace);
  }
  // at least via enclosure should not short with obs (OBS has issue with
  // wrongway and PG has issue with prefDir)
  // TODO: generalize the following
  if (isOBS) {
    bloat_dist += viaLength / 2;
  } else {
    bloat_dist += viaWidth / 2;
  }
  return bloat_dist;
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
  frCoord bloat_dist = width;
  if (layer->hasMinSpacing()) {
    bloat_dist = layer->getMinSpacingValue(objWidth, width, prl, false);
    if (bloat_dist < 0) {
      logger_->error(DRT,
                     144,
                     "Layer {} has negative min spacing value.",
                     layer->getName());
    }
  }
  // assuming the wire width is width
  bloat_dist += width / 2;
  return bloat_dist;
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
