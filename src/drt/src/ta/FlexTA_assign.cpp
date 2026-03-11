// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "db/taObj/taShape.h"
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

namespace {
constexpr int kWarnViaLayerNonRoutingMinSpacingMsgId = 414;
constexpr int kWarnViaLayerNonRoutingCutSpacingMsgId = 415;
}  // namespace

frSquaredDistance FlexTAWorker::box2boxDistSquare(const odb::Rect& box1,
                                                  const odb::Rect& box2,
                                                  frCoord& dx,
                                                  frCoord& dy)
{
  dx = std::max(
      std::max(box1.xMin(), box2.xMin()) - std::min(box1.xMax(), box2.xMax()),
      0);
  dy = std::max(
      std::max(box1.yMin(), box2.yMin()) - std::min(box1.yMax(), box2.yMax()),
      0);
  return (frSquaredDistance) dx * dx + (frSquaredDistance) dy * dy;
}

// must be current TA layer
void FlexTAWorker::modMinSpacingCostPlanar(const odb::Rect& box,
                                           frLayerNum lNum,
                                           taPinFig* fig,
                                           bool is_add_cost,
                                           frOrderedIdSet<taPin*>* pin_set)
{
  // obj1 = curr obj
  frCoord width1 = box.minDXDY();
  frCoord length1 = box.maxDXDY();
  // obj2 = other obj
  auto layer = getDesign()->getTech()->getLayer(lNum);
  // layer default width
  frCoord width2 = layer->getWidth();
  frCoord halfwidth2 = width2 / 2;
  // spacing value needed
  frCoord bloatDist = layer->getMinSpacingValue(width1, width2, length1, false);
  if (fig->getNet()->getNondefaultRule()) {
    bloatDist = std::max(
        bloatDist,
        fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
  }

  frSquaredDistance bloatDistSquare = (frSquaredDistance) bloatDist * bloatDist;

  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  // now assume track in H direction
  frCoord boxLow = isH ? box.yMin() : box.xMin();
  frCoord boxHigh = isH ? box.yMax() : box.xMax();
  frCoord boxLeft = isH ? box.xMin() : box.yMin();
  frCoord boxRight = isH ? box.xMax() : box.yMax();
  odb::Rect box1(boxLeft, boxLow, boxRight, boxHigh);

  int idx_1, idx_2;
  getTrackIdx(boxLow - bloatDist - halfwidth2 + 1,
              boxHigh + bloatDist + halfwidth2 - 1,
              lNum,
              idx_1,
              idx_2);

  odb::Rect box2(-halfwidth2, -halfwidth2, halfwidth2, halfwidth2);
  frCoord dx, dy;
  auto& trackLocs = getTrackLocs(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  for (int i = idx_1; i <= idx_2; i++) {
    auto trackLoc = trackLocs[i];
    odb::dbTransform xform(odb::Point(boxLeft, trackLoc));
    xform.apply(box2);
    box2boxDistSquare(box1, box2, dx, dy);
    if (dy >= bloatDist) {
      continue;
    }
    frCoord maxX = (frCoord) (sqrt(1.0 * bloatDistSquare
                                   - 1.0 * (frSquaredDistance) dy * dy));
    if ((frSquaredDistance) maxX * maxX + (frSquaredDistance) dy * dy
        == bloatDistSquare) {
      maxX = std::max(0, maxX - 1);
    }
    frCoord blockLeft = boxLeft - maxX - halfwidth2;
    frCoord blockRight = boxRight + maxX + halfwidth2;
    odb::Rect tmpBox;
    if (isH) {
      tmpBox.init(blockLeft, trackLoc, blockRight, trackLoc);
    } else {
      tmpBox.init(trackLoc, blockLeft, trackLoc, blockRight);
    }
    auto con = layer->getMinSpacing();
    if (is_add_cost) {
      workerRegionQuery.addCost(tmpBox, lNum, fig, con);
      if (pin_set) {
        workerRegionQuery.query(tmpBox, lNum, *pin_set);
      }
    } else {
      workerRegionQuery.removeCost(tmpBox, lNum, fig, con);
      if (pin_set) {
        workerRegionQuery.query(tmpBox, lNum, *pin_set);
      }
    }
  }
}

bool FlexTAWorker::getFollowTrackLayerNum(
    frLayerNum cut_layer_num,
    frLayerNum& follow_track_layer_num) const
{
  if (cut_layer_num - 1 >= getDesign()->getTech()->getBottomLayerNum()
      && getDesign()->getTech()->getLayer(cut_layer_num - 1)->getType()
             == dbTechLayerType::ROUTING
      && getDesign()->getTech()->getLayer(cut_layer_num - 1)->getDir()
             == getDir()) {
    follow_track_layer_num = cut_layer_num - 1;
    return true;
  }
  if (cut_layer_num + 1 <= getDesign()->getTech()->getTopLayerNum()
      && getDesign()->getTech()->getLayer(cut_layer_num + 1)->getType()
             == dbTechLayerType::ROUTING
      && getDesign()->getTech()->getLayer(cut_layer_num + 1)->getDir()
             == getDir()) {
    follow_track_layer_num = cut_layer_num + 1;
    return true;
  }
  return false;
}

void FlexTAWorker::getViaTrackRange(const odb::Rect& box,
                                    const odb::Rect& via_box,
                                    frCoord bloat_dist,
                                    bool is_horizontal,
                                    frLayerNum follow_track_layer_num,
                                    int& idx_1,
                                    int& idx_2) const
{
  if (is_horizontal) {
    getTrackIdx(box.yMin() - bloat_dist - via_box.yMax() + 1,
                box.yMax() + bloat_dist - via_box.yMin() - 1,
                follow_track_layer_num,
                idx_1,
                idx_2);
  } else {
    getTrackIdx(box.xMin() - bloat_dist - via_box.xMax() + 1,
                box.xMax() + bloat_dist - via_box.xMin() - 1,
                follow_track_layer_num,
                idx_1,
                idx_2);
  }
}

frCoord FlexTAWorker::getViaParallelRunLength(const odb::Rect& box,
                                              const odb::Rect& via_box,
                                              frCoord dx,
                                              frCoord dy,
                                              bool is_horizontal,
                                              bool is_curr_ps) const
{
  if (is_horizontal) {
    if (dy > 0) {
      return is_curr_ps ? via_box.dx() : std::min(box.dx(), via_box.dx());
    }
    return is_curr_ps ? via_box.dy() : std::min(box.dy(), via_box.dy());
  }

  if (dx > 0) {
    return is_curr_ps ? via_box.dy() : std::min(box.dy(), via_box.dy());
  }
  return is_curr_ps ? via_box.dx() : std::min(box.dx(), via_box.dx());
}

bool FlexTAWorker::buildViaBlockBox(const odb::Rect& box,
                                    const odb::Rect& via_box,
                                    frCoord dx,
                                    frCoord dy,
                                    frCoord req_dist,
                                    bool is_horizontal,
                                    bool is_center_to_center,
                                    frCoord track_loc,
                                    const odb::Point& box_center,
                                    odb::Rect& block_box) const
{
  frCoord distance = is_horizontal ? dy : dx;
  if (is_center_to_center) {
    distance = is_horizontal ? std::abs(box_center.y() - track_loc)
                             : std::abs(box_center.x() - track_loc);
  }
  if (distance >= req_dist) {
    return false;
  }

  frCoord max_delta = (frCoord) std::sqrt(1.0 * req_dist * req_dist
                                          - 1.0 * distance * distance);
  if (max_delta * max_delta + distance * distance == req_dist * req_dist) {
    max_delta = std::max(0, max_delta - 1);
  }

  frCoord block_left = 0;
  frCoord block_right = 0;
  if (is_center_to_center) {
    block_left = is_horizontal ? box_center.x() - max_delta
                               : box_center.y() - max_delta;
    block_right = is_horizontal ? box_center.x() + max_delta
                                : box_center.y() + max_delta;
  } else if (is_horizontal) {
    block_left = box.xMin() - max_delta - via_box.xMax();
    block_right = box.xMax() + max_delta - via_box.xMin();
  } else {
    block_left = box.yMin() - max_delta - via_box.yMax();
    block_right = box.yMax() + max_delta - via_box.yMin();
  }

  if (is_horizontal) {
    block_box.init(block_left, track_loc, block_right, track_loc);
  } else {
    block_box.init(track_loc, block_left, track_loc, block_right);
  }
  return true;
}

void FlexTAWorker::updateViaCost(const odb::Rect& block_box,
                                 frLayerNum layer_num,
                                 taPinFig* fig,
                                 frConstraint* con,
                                 bool is_add_cost,
                                 frOrderedIdSet<taPin*>* pin_set,
                                 bool is_cut_cost)
{
  auto& worker_region_query = getWorkerRegionQuery();
  if (is_add_cost) {
    if (is_cut_cost) {
      worker_region_query.addViaCost(block_box, layer_num, fig, con);
    } else {
      worker_region_query.addCost(block_box, layer_num, fig, con);
    }
  } else if (is_cut_cost) {
    worker_region_query.removeViaCost(block_box, layer_num, fig, con);
  } else {
    worker_region_query.removeCost(block_box, layer_num, fig, con);
  }
  if (pin_set) {
    worker_region_query.query(block_box, layer_num, *pin_set);
  }
}

bool FlexTAWorker::adjustParallelOverlapBlockBox(const odb::Rect& box,
                                                 const odb::Rect& via_box,
                                                 frCoord dx,
                                                 frCoord dy,
                                                 bool is_horizontal,
                                                 frCoord track_loc,
                                                 odb::Rect& block_box) const
{
  if (is_horizontal) {
    if (dy <= 0) {
      return false;
    }
    block_box.init(std::max(box.xMin() - via_box.xMax() + 1, block_box.xMin()),
                   track_loc,
                   std::min(box.xMax() - via_box.xMin() - 1, block_box.xMax()),
                   track_loc);
  } else {
    if (dx <= 0) {
      return false;
    }
    block_box.init(track_loc,
                   std::max(box.yMin() - via_box.yMax() + 1, block_box.yMin()),
                   track_loc,
                   std::min(box.yMax() - via_box.yMin() - 1, block_box.yMax()));
  }
  return block_box.xMin() <= block_box.xMax()
         && block_box.yMin() <= block_box.yMax();
}

bool FlexTAWorker::isCutSpacingConstraintViolated(
    const frCutSpacingConstraint* con,
    const odb::Rect& box,
    const odb::Rect& transformed_via_box,
    frCoord dx,
    frCoord dy,
    bool is_horizontal,
    frCoord track_loc,
    const odb::Point& box_center,
    const odb::Rect& via_box,
    odb::Rect& block_box) const
{
  if (con->hasSameNet()) {
    return false;
  }

  if (!buildViaBlockBox(box,
                        via_box,
                        dx,
                        dy,
                        con->getCutSpacing(),
                        is_horizontal,
                        con->hasCenterToCenter(),
                        track_loc,
                        box_center,
                        block_box)) {
    return false;
  }

  if (con->isLayer()) {
    return false;
  }
  if (con->isAdjacentCuts()) {
    return true;
  }
  if (con->isParallelOverlap()) {
    return adjustParallelOverlapBlockBox(
        box, via_box, dx, dy, is_horizontal, track_loc, block_box);
  }
  if (con->isArea()) {
    return std::max(box.area(), transformed_via_box.area())
           >= con->getCutArea();
  }
  return true;
}

// given a shape on any routing layer n, block via @(n+1) if is_upper_via is
// true
void FlexTAWorker::modMinSpacingCostVia(const odb::Rect& box,
                                        frLayerNum lNum,
                                        taPinFig* fig,
                                        bool is_add_cost,
                                        bool is_upper_via,
                                        bool is_curr_ps,
                                        frOrderedIdSet<taPin*>* pin_set)
{
  const frViaDef* via_def = nullptr;
  frLayerNum cut_layer_num = 0;
  if (is_upper_via) {
    via_def
        = (lNum < getDesign()->getTech()->getTopLayerNum())
              ? getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef()
              : nullptr;
    cut_layer_num = lNum + 1;
  } else {
    via_def
        = (lNum > getDesign()->getTech()->getBottomLayerNum())
              ? getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef()
              : nullptr;
    cut_layer_num = lNum - 1;
  }
  if (via_def == nullptr) {
    return;
  }

  frLayerNum follow_track_layer_num = 0;
  if (!getFollowTrackLayerNum(cut_layer_num, follow_track_layer_num)) {
    logger_->warn(DRT,
                  kWarnViaLayerNonRoutingMinSpacingMsgId,
                  "Via layer connected to non-routing layer, skipped in "
                  "modMinSpacingCostVia.");
    return;
  }

  frVia via(via_def);
  odb::Rect via_box = is_upper_via ? via.getLayer1BBox() : via.getLayer2BBox();
  const frCoord width1 = box.minDXDY();
  const frCoord length1 = box.maxDXDY();
  const frCoord width2 = via_box.minDXDY();
  const frCoord length2 = via_box.maxDXDY();
  const bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);

  auto* layer = getTech()->getLayer(lNum);
  frCoord bloat_dist = layer->getMinSpacingValue(
      width1, width2, is_curr_ps ? length2 : std::min(length1, length2), false);
  if (fig->getNet()->getNondefaultRule()) {
    bloat_dist = std::max(
        bloat_dist,
        fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
  }

  int idx_1 = 0;
  int idx_2 = -1;
  getViaTrackRange(box,
                   via_box,
                   bloat_dist,
                   is_horizontal,
                   follow_track_layer_num,
                   idx_1,
                   idx_2);
  auto& track_locs = getTrackLocs(follow_track_layer_num);

  odb::Rect transformed_via_box;
  odb::dbTransform xform;
  for (int i = idx_1; i <= idx_2; i++) {
    const frCoord track_loc = track_locs[i];
    xform.setOffset(is_horizontal ? odb::Point(box.xMin(), track_loc)
                                  : odb::Point(track_loc, box.yMin()));
    transformed_via_box = via_box;
    xform.apply(transformed_via_box);

    frCoord dx = 0;
    frCoord dy = 0;
    box2boxDistSquare(box, transformed_via_box, dx, dy);
    const frCoord prl = getViaParallelRunLength(
        box, via_box, dx, dy, is_horizontal, is_curr_ps);

    frCoord req_dist = layer->getMinSpacingValue(width1, width2, prl, false);
    if (fig->getNet()->getNondefaultRule()) {
      req_dist = std::max(
          req_dist,
          fig->getNet()->getNondefaultRule()->getSpacing(lNum / 2 - 1));
    }

    odb::Rect block_box;
    if (!buildViaBlockBox(box,
                          via_box,
                          dx,
                          dy,
                          req_dist,
                          is_horizontal,
                          false,
                          track_loc,
                          odb::Point(),
                          block_box)) {
      continue;
    }
    updateViaCost(block_box,
                  cut_layer_num,
                  fig,
                  layer->getMinSpacing(),
                  is_add_cost,
                  pin_set,
                  true);
  }
}

void FlexTAWorker::modCutSpacingCost(const odb::Rect& box,
                                     frLayerNum lNum,
                                     taPinFig* fig,
                                     bool is_add_cost,
                                     frOrderedIdSet<taPin*>* pin_set)
{
  auto* layer = getDesign()->getTech()->getLayer(lNum);
  if (!layer->hasCutSpacing()) {
    return;
  }

  const frViaDef* via_def = layer->getDefaultViaDef();
  if (via_def == nullptr) {
    return;
  }
  frVia via(via_def);
  odb::Rect via_box = via.getCutBBox();

  frLayerNum follow_track_layer_num = 0;
  if (!getFollowTrackLayerNum(lNum, follow_track_layer_num)) {
    logger_->warn(DRT,
                  kWarnViaLayerNonRoutingCutSpacingMsgId,
                  "Via layer connected to non-routing layer, skipped in "
                  "modCutSpacingCost.");
    return;
  }

  frCoord bloat_dist = 0;
  for (const auto* con : layer->getCutSpacing()) {
    bloat_dist = std::max(bloat_dist, con->getCutSpacing());
  }

  const bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
  int idx_1 = 0;
  int idx_2 = -1;
  getViaTrackRange(box,
                   via_box,
                   bloat_dist,
                   is_horizontal,
                   follow_track_layer_num,
                   idx_1,
                   idx_2);
  auto& track_locs = getTrackLocs(follow_track_layer_num);
  const odb::Point box_center((box.xMin() + box.xMax()) / 2,
                              (box.yMin() + box.yMax()) / 2);

  odb::dbTransform xform;
  for (int i = idx_1; i <= idx_2; i++) {
    const frCoord track_loc = track_locs[i];
    xform.setOffset(is_horizontal ? odb::Point(box.xMin(), track_loc)
                                  : odb::Point(track_loc, box.yMin()));
    odb::Rect transformed_via_box = via_box;
    xform.apply(transformed_via_box);

    frCoord dx = 0;
    frCoord dy = 0;
    box2boxDistSquare(box, transformed_via_box, dx, dy);

    for (auto* con : layer->getCutSpacing()) {
      odb::Rect block_box;
      if (!isCutSpacingConstraintViolated(con,
                                          box,
                                          transformed_via_box,
                                          dx,
                                          dy,
                                          is_horizontal,
                                          track_loc,
                                          box_center,
                                          via_box,
                                          block_box)) {
        continue;
      }
      updateViaCost(block_box, lNum, fig, con, is_add_cost, pin_set, true);
    }
  }
}

void FlexTAWorker::addCost(taPinFig* fig, frOrderedIdSet<taPin*>* pin_set)
{
  modCost(fig, true, pin_set);
}

void FlexTAWorker::subCost(taPinFig* fig, frOrderedIdSet<taPin*>* pin_set)
{
  modCost(fig, false, pin_set);
}

void FlexTAWorker::modCost(taPinFig* fig,
                           bool is_add_cost,
                           frOrderedIdSet<taPin*>* pin_set)
{
  if (fig->typeId() == tacPathSeg) {
    auto obj = static_cast<taPathSeg*>(fig);
    auto layerNum = obj->getLayerNum();
    odb::Rect box = obj->getBBox();
    modMinSpacingCostPlanar(
        box, layerNum, obj, is_add_cost, pin_set);  // must be current TA layer
    modMinSpacingCostVia(box, layerNum, obj, is_add_cost, true, true, pin_set);
    modMinSpacingCostVia(box, layerNum, obj, is_add_cost, false, true, pin_set);
  } else if (fig->typeId() == tacVia) {
    auto obj = static_cast<taVia*>(fig);
    // assumes enclosure for via is always rectangle
    odb::Rect box = obj->getLayer1BBox();
    auto layerNum = obj->getViaDef()->getLayer1Num();
    // current TA layer
    if (getDir() == getDesign()->getTech()->getLayer(layerNum)->getDir()) {
      modMinSpacingCostPlanar(box, layerNum, obj, is_add_cost, pin_set);
    }
    modMinSpacingCostVia(box, layerNum, obj, is_add_cost, true, false, pin_set);
    modMinSpacingCostVia(
        box, layerNum, obj, is_add_cost, false, false, pin_set);

    // assumes enclosure for via is always rectangle
    box = obj->getLayer2BBox();
    layerNum = obj->getViaDef()->getLayer2Num();
    // current TA layer
    if (getDir() == getDesign()->getTech()->getLayer(layerNum)->getDir()) {
      modMinSpacingCostPlanar(box, layerNum, obj, is_add_cost, pin_set);
    }
    modMinSpacingCostVia(box, layerNum, obj, is_add_cost, true, false, pin_set);
    modMinSpacingCostVia(
        box, layerNum, obj, is_add_cost, false, false, pin_set);

    odb::Point pt = obj->getOrigin();
    odb::dbTransform xform(pt);
    for (auto& uFig : obj->getViaDef()->getCutFigs()) {
      auto rect = static_cast<frRect*>(uFig.get());
      box = rect->getBBox();
      xform.apply(box);
      layerNum = obj->getViaDef()->getCutLayerNum();
      modCutSpacingCost(box, layerNum, obj, is_add_cost, pin_set);
    }
  } else {
    std::cout << "Error: unsupported region query add\n";
  }
}

void FlexTAWorker::assignIroute_availTracks(taPin* iroute,
                                            frLayerNum& layer_num,
                                            int& idx_1,
                                            int& idx_2)
{
  layer_num = iroute->getGuide()->getBeginLayerNum();
  auto [gbp, gep] = iroute->getGuide()->getPoints();
  odb::Point gIdx = getDesign()->getTopBlock()->getGCellIdx(gbp);
  odb::Rect gBox = getDesign()->getTopBlock()->getGCellBox(gIdx);
  bool is_horizontal = (getDir() == dbTechLayerDir::HORIZONTAL);
  frCoord coordLow = is_horizontal ? gBox.yMin() : gBox.xMin();
  frCoord coordHigh = is_horizontal ? gBox.yMax() : gBox.xMax();
  coordHigh--;  // to avoid higher track == guide top/right
  if (getTech()->getLayer(layer_num)->isUnidirectional()) {
    const odb::Rect& dieBx = design_->getTopBlock()->getDieBox();
    const frViaDef* via = nullptr;
    odb::Rect testBox;
    if (layer_num + 1 <= getTech()->getTopLayerNum()) {
      via = getTech()->getLayer(layer_num + 1)->getDefaultViaDef();
      testBox = via->getLayer1ShapeBox();
      testBox.merge(via->getLayer2ShapeBox());
    } else {
      via = getTech()->getLayer(layer_num - 1)->getDefaultViaDef();
      testBox = via->getLayer1ShapeBox();
      testBox.merge(via->getLayer2ShapeBox());
    }
    int diffLow, diffHigh;
    if (is_horizontal) {
      diffLow = dieBx.yMin() - (coordLow - testBox.dy() / 2);
      diffHigh = coordHigh + testBox.dy() / 2 - dieBx.yMax();
    } else {
      diffLow = dieBx.xMin() - (coordLow - testBox.dx() / 2);
      diffHigh = coordHigh + testBox.dx() / 2 - dieBx.xMax();
    }
    if (diffLow > 0) {
      coordLow += diffLow;
    }
    if (diffHigh > 0) {
      coordHigh -= diffHigh;
    }
  }
  getTrackIdx(coordLow, coordHigh, layer_num, idx_1, idx_2);
  if (idx_2 < idx_1) {
    const double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    logger_->error(DRT,
                   406,
                   "No {} tracks found in ({}, {}) for layer {}",
                   is_horizontal ? "horizontal" : "vertical",
                   coordLow / dbu,
                   coordHigh / dbu,
                   getTech()->getLayer(layer_num)->getName());
  }
}

// This adds a cost based on the connected iroutes. For a vertical iroute if
// there are more connected iroutes to the right, we should force this iroute
// towards the right of the gcell (and vice versa). The cost is scaled based on
// the net number of iroutes in that direction.
frUInt4 FlexTAWorker::assignIroute_getNextIrouteDirCost(taPin* iroute,
                                                        frCoord trackLoc)
{
  auto guide = iroute->getGuide();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  auto [begin, end] = guide->getPoints();
  odb::Point idx = getDesign()->getTopBlock()->getGCellIdx(end);
  odb::Rect endBox = getDesign()->getTopBlock()->getGCellBox(idx);
  int nextIrouteDirCost = 0;
  auto nextIrouteDir = iroute->getNextIrouteDir();
  if (nextIrouteDir <= 0) {
    if (isH) {
      nextIrouteDirCost = abs(nextIrouteDir) * (trackLoc - endBox.yMin());
    } else {
      nextIrouteDirCost = abs(nextIrouteDir) * (trackLoc - endBox.xMin());
    }
  } else {
    if (isH) {
      nextIrouteDirCost = abs(nextIrouteDir) * (endBox.yMax() - trackLoc);
    } else {
      nextIrouteDirCost = abs(nextIrouteDir) * (endBox.xMax() - trackLoc);
    }
  }
  if (nextIrouteDirCost < 0) {
    double dbu = getDesign()->getTopBlock()->getDBUPerUU();
    std::cout << "Error: nextIrouteDirCost < 0" << ", trackLoc@"
              << trackLoc / dbu << " box (" << endBox.xMin() / dbu << ", "
              << endBox.yMin() / dbu << ") (" << endBox.xMax() / dbu << ", "
              << endBox.yMax() / dbu << ")\n";
    return (frUInt4) 0;
  }
  return (frUInt4) nextIrouteDirCost;
}

frUInt4 FlexTAWorker::assignIroute_getPinCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 sol = 0;
  if (iroute->hasPinCoord()) {
    sol = abs(trackLoc - iroute->getPinCoord());

    // add cost to locations that will cause forbidden via spacing to
    // boundary pin
    auto layerNum = iroute->getGuide()->getBeginLayerNum();
    auto layer = getTech()->getLayer(layerNum);

    if (layer->isUnidirectional()) {
      bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
      int zIdx = layerNum / 2 - 1;
      if (sol) {
        if (isH) {
          // if cannot use bottom or upper layer to bridge, then add cost
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, false, sol, nullptr)
               || layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, false, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += router_cfg_->TADRCCOST;
          }
        } else {
          if ((getTech()->isVia2ViaForbiddenLen(
                   zIdx, false, false, true, sol, nullptr)
               || layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER)
              && (getTech()->isVia2ViaForbiddenLen(
                      zIdx, true, true, true, sol, nullptr)
                  || layerNum + 2 > getTech()->getTopLayerNum())) {
            sol += router_cfg_->TADRCCOST;
          }
        }
      }
    }
  }
  return sol;
}

frUInt4 FlexTAWorker::assignIroute_getDRCCost_helper(taPin* iroute,
                                                     odb::Rect& box,
                                                     frLayerNum lNum)
{
  auto layer = getDesign()->getTech()->getLayer(lNum);
  auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>> result;
  int overlap = 0;
  if (iroute->getGuide()->getNet()->getNondefaultRule()) {
    int r = iroute->getGuide()->getNet()->getNondefaultRule()->getWidth(lNum / 2
                                                                        - 1)
            / 2;
    r += iroute->getGuide()->getNet()->getNondefaultRule()->getSpacing(lNum / 2
                                                                       - 1);
    box.bloat(r, box);
  }
  workerRegionQuery.queryCost(box, lNum, result);

  auto getPartialBox = [this, lNum](odb::Rect box, bool begin) {
    auto layer = getTech()->getLayer(lNum);
    odb::Rect result;
    frCoord addHorz = 0;
    frCoord addVert = 0;
    if (layer->isHorizontal()) {
      addHorz = getDesign()->getTopBlock()->getGCellSizeHorizontal() / 2;
    } else {
      addVert = getDesign()->getTopBlock()->getGCellSizeVertical() / 2;
    }
    if (begin) {
      result.reset(box.xMin(),
                   box.yMin(),
                   std::min(box.xMax(), box.xMin() + addHorz),
                   std::min(box.yMax(), box.yMin() + addVert));
    } else {
      result.reset(std::max(box.xMin(), box.xMax() - addHorz),
                   std::max(box.yMin(), box.yMax() - addVert),
                   box.xMax(),
                   box.yMax());
    }
    return result;
  };
  std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>
      tmpResult;
  if (layer->getType() == dbTechLayerType::CUT) {
    workerRegionQuery.queryViaCost(box, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
  } else {
    odb::Rect tmpBox = getPartialBox(box, true);
    workerRegionQuery.queryViaCost(tmpBox, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
    tmpResult.clear();
    tmpBox = getPartialBox(box, false);
    workerRegionQuery.queryViaCost(tmpBox, lNum, tmpResult);
    result.insert(result.end(), tmpResult.begin(), tmpResult.end());
  }
  bool isCut = false;

  // save same net overlaps
  std::vector<odb::Rect> sameNetOverlaps;
  for (auto& [bounds, pr] : result) {
    auto& [obj, con] = pr;
    if (obj != nullptr && obj->typeId() == frcNet) {
      if (iroute->getGuide()->getNet() == obj) {
        sameNetOverlaps.push_back(bounds);
      }
    }
  }

  for (auto& [bounds, pr] : result) {
    // if the overlap bounds intersect with the pin connection, do not add drc
    // cost
    bool pinConn = false;
    for (const odb::Rect& sameNetOverlap : sameNetOverlaps) {
      if (sameNetOverlap.intersects(bounds)) {
        pinConn = true;
        break;
      }
    }
    if (pinConn) {
      continue;
    }

    auto& [obj, con] = pr;
    frCoord tmpOvlp = -std::max(box.xMin(), bounds.xMin())
                      + std::min(box.xMax(), bounds.xMax())
                      - std::max(box.yMin(), bounds.yMin())
                      + std::min(box.yMax(), bounds.yMax()) + 1;
    if (tmpOvlp <= 0) {
      logger_->error(DRT,
                     412,
                     "assignIroute_getDRCCost_helper overlap value is {}.",
                     tmpOvlp);
    }
    // unknown obj, always add cost
    if (obj == nullptr) {
      overlap += tmpOvlp;
      // only add cost for diff-net
    } else if (obj->typeId() == frcNet) {
      if (iroute->getGuide()->getNet() != obj) {
        overlap += tmpOvlp;
      }
      // two taObjs
    } else if (obj->typeId() == tacPathSeg || obj->typeId() == tacVia) {
      auto taObj = static_cast<taPinFig*>(obj);
      // can exclude same iroute objs also
      if (taObj->getPin() == iroute) {
        continue;
      }
      if (iroute->getGuide()->getNet()
          != taObj->getPin()->getGuide()->getNet()) {
        overlap += tmpOvlp;
      }
    } else {
      std::cout << "Warning: assignIroute_getDRCCost_helper unsupported type"
                << '\n';
    }
  }
  frCoord pitch = 0;
  if (getDesign()->getTech()->getLayer(lNum)->getType()
      == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum)->getPitch();
    isCut = false;
  } else if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1
             && getDesign()->getTech()->getLayer(lNum + 1)->getType()
                    == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum + 1)->getPitch();
    isCut = true;
  } else if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1
             && getDesign()->getTech()->getLayer(lNum - 1)->getType()
                    == dbTechLayerType::ROUTING) {
    pitch = getDesign()->getTech()->getLayer(lNum - 1)->getPitch();
    isCut = true;
  } else {
    std::cout << "Error: assignIroute_getDRCCost_helper unknown layer type"
              << '\n';
    exit(1);
  }
  // always penalize two pitch per cut, regardless of cnts
  if (overlap == 0) {
    return 0;
  }
  return isCut ? pitch * 2 : std::max(pitch * 2, overlap);
}

frUInt4 FlexTAWorker::assignIroute_getDRCCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 cost = 0;
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      if (isH) {
        bp = {bp.x(), trackLoc};
        ep = {ep.x(), trackLoc};
      } else {
        bp = {trackLoc, bp.y()};
        ep = {trackLoc, ep.y()};
      }
      odb::Rect bbox(bp, ep);
      frUInt4 wireCost
          = assignIroute_getDRCCost_helper(iroute, bbox, obj->getLayerNum());
      cost += wireCost;
    } else if (uPinFig->typeId() == tacVia) {
      auto obj = static_cast<taVia*>(uPinFig.get());
      auto bp = obj->getOrigin();
      if (isH) {
        bp = {bp.x(), trackLoc};
      } else {
        bp = {trackLoc, bp.y()};
      }
      odb::Rect bbox(bp, bp);
      frUInt4 viaCost = assignIroute_getDRCCost_helper(
          iroute, bbox, obj->getViaDef()->getCutLayerNum());
      cost += viaCost;
    } else {
      std::cout << "Error: assignIroute_updateIroute unsupported pinFig"
                << '\n';
      exit(1);
    }
  }
  return cost;
}

frUInt4 FlexTAWorker::assignIroute_getAlignCost(taPin* iroute, frCoord trackLoc)
{
  frUInt4 sol = 0;
  frCoord pitch = 0;
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      auto lNum = obj->getLayerNum();
      pitch = getDesign()->getTech()->getLayer(lNum)->getPitch();
      auto& workerRegionQuery = getWorkerRegionQuery();
      frOrderedIdSet<taPin*> result;
      odb::Rect box;
      if (isH) {
        box.init(bp.x(), trackLoc, ep.x(), trackLoc);
      } else {
        box.init(trackLoc, bp.y(), trackLoc, ep.y());
      }
      workerRegionQuery.query(box, lNum, result);
      for (auto& iroute2 : result) {
        if (iroute2->getGuide()->getNet() == iroute->getGuide()->getNet()) {
          sol = 1;
          break;
        }
      }
    }
    if (sol == 1) {
      break;
    }
  }
  return pitch * sol;
}

frUInt4 FlexTAWorker::assignIroute_getCost(taPin* iroute,
                                           frCoord trackLoc,
                                           frUInt4& outDrcCost)
{
  frCoord irouteLayerPitch
      = getTech()->getLayer(iroute->getGuide()->getBeginLayerNum())->getPitch();
  outDrcCost = assignIroute_getDRCCost(iroute, trackLoc);
  int drcCost = (isInitTA()) ? (0.05 * outDrcCost)
                             : (router_cfg_->TADRCCOST * outDrcCost);
  int nextIrouteDirCost = assignIroute_getNextIrouteDirCost(iroute, trackLoc);
  // int pinCost    = TAPINCOST * assignIroute_getPinCost(iroute, trackLoc);
  int tmpPinCost = assignIroute_getPinCost(iroute, trackLoc);
  int pinCost = (tmpPinCost == 0)
                    ? 0
                    : router_cfg_->TAPINCOST * irouteLayerPitch + tmpPinCost;
  int tmpAlignCost = assignIroute_getAlignCost(iroute, trackLoc);
  int alignCost
      = (tmpAlignCost == 0)
            ? 0
            : router_cfg_->TAALIGNCOST * irouteLayerPitch + tmpAlignCost;
  return std::max(drcCost + nextIrouteDirCost + pinCost - alignCost, 0);
}

void FlexTAWorker::assignIroute_bestTrack_helper(taPin* iroute,
                                                 frLayerNum layer_num,
                                                 int track_idx,
                                                 frUInt4& best_cost,
                                                 frCoord& best_track_loc,
                                                 int& best_track_idx,
                                                 frUInt4& drc_cost)
{
  auto trackLoc = getTrackLocs(layer_num)[track_idx];
  auto currCost = assignIroute_getCost(iroute, trackLoc, drc_cost);
  if (isInitTA()) {
    if (currCost < best_cost) {
      best_cost = currCost;
      best_track_loc = trackLoc;
      best_track_idx = track_idx;
    }
  } else {
    if (drc_cost < best_cost) {
      best_cost = drc_cost;
      best_track_loc = trackLoc;
      best_track_idx = track_idx;
    }
  }
}

int FlexTAWorker::assignIroute_clampStartTrackIdx(frLayerNum layer_num,
                                                  frCoord pin_coord,
                                                  int idx_1,
                                                  int idx_2) const
{
  const auto& track_locs = getTrackLocs(layer_num);
  int start_track_idx = static_cast<int>(
      std::ranges::lower_bound(track_locs, pin_coord) - track_locs.begin());
  start_track_idx = std::min(start_track_idx, idx_2);
  start_track_idx = std::max(start_track_idx, idx_1);
  return start_track_idx;
}

bool FlexTAWorker::assignIroute_scanAscending(taPin* iroute,
                                              frLayerNum layer_num,
                                              int start_idx,
                                              int end_idx,
                                              frUInt4& best_cost,
                                              frCoord& best_track_loc,
                                              int& best_track_idx,
                                              frUInt4& drc_cost)
{
  if (start_idx > end_idx) {
    return false;
  }
  for (int i = start_idx; i <= end_idx; i++) {
    assignIroute_bestTrack_helper(iroute,
                                  layer_num,
                                  i,
                                  best_cost,
                                  best_track_loc,
                                  best_track_idx,
                                  drc_cost);
    if (!drc_cost) {
      return true;
    }
  }
  return false;
}

bool FlexTAWorker::assignIroute_scanDescending(taPin* iroute,
                                               frLayerNum layer_num,
                                               int start_idx,
                                               int end_idx,
                                               frUInt4& best_cost,
                                               frCoord& best_track_loc,
                                               int& best_track_idx,
                                               frUInt4& drc_cost)
{
  if (start_idx < end_idx) {
    return false;
  }
  for (int i = start_idx; i >= end_idx; i--) {
    assignIroute_bestTrack_helper(iroute,
                                  layer_num,
                                  i,
                                  best_cost,
                                  best_track_loc,
                                  best_track_idx,
                                  drc_cost);
    if (!drc_cost) {
      return true;
    }
  }
  return false;
}

bool FlexTAWorker::assignIroute_scanAlternating(taPin* iroute,
                                                frLayerNum layer_num,
                                                int start_idx,
                                                int idx_1,
                                                int idx_2,
                                                frUInt4& best_cost,
                                                frCoord& best_track_loc,
                                                int& best_track_idx,
                                                frUInt4& drc_cost)
{
  const int track_count = idx_2 - idx_1 + 1;
  for (int offset = 0; offset < track_count; offset++) {
    int curr_track_idx = start_idx + offset;
    if (curr_track_idx >= idx_1 && curr_track_idx <= idx_2) {
      assignIroute_bestTrack_helper(iroute,
                                    layer_num,
                                    curr_track_idx,
                                    best_cost,
                                    best_track_loc,
                                    best_track_idx,
                                    drc_cost);
      if (!drc_cost) {
        return true;
      }
    }

    curr_track_idx = start_idx - offset - 1;
    if (curr_track_idx >= idx_1 && curr_track_idx <= idx_2) {
      assignIroute_bestTrack_helper(iroute,
                                    layer_num,
                                    curr_track_idx,
                                    best_cost,
                                    best_track_loc,
                                    best_track_idx,
                                    drc_cost);
      if (!drc_cost) {
        return true;
      }
    }
  }
  return false;
}

int FlexTAWorker::assignIroute_bestTrack(taPin* iroute,
                                         frLayerNum layer_num,
                                         int idx_1,
                                         int idx_2)
{
  const double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  frCoord best_track_loc = 0;
  int best_track_idx = -1;
  frUInt4 best_cost = std::numeric_limits<frUInt4>::max();
  frUInt4 drc_cost = 0;
  const int next_iroute_dir = iroute->getNextIrouteDir();

  if (iroute->hasPinCoord()) {
    const int start_track_idx = assignIroute_clampStartTrackIdx(
        layer_num, iroute->getPinCoord(), idx_1, idx_2);
    if (next_iroute_dir > 0) {
      if (!assignIroute_scanAscending(iroute,
                                      layer_num,
                                      start_track_idx,
                                      idx_2,
                                      best_cost,
                                      best_track_loc,
                                      best_track_idx,
                                      drc_cost)) {
        assignIroute_scanDescending(iroute,
                                    layer_num,
                                    start_track_idx - 1,
                                    idx_1,
                                    best_cost,
                                    best_track_loc,
                                    best_track_idx,
                                    drc_cost);
      }
    } else if (next_iroute_dir == 0) {
      assignIroute_scanAlternating(iroute,
                                   layer_num,
                                   start_track_idx,
                                   idx_1,
                                   idx_2,
                                   best_cost,
                                   best_track_loc,
                                   best_track_idx,
                                   drc_cost);
    } else {
      if (!assignIroute_scanDescending(iroute,
                                       layer_num,
                                       start_track_idx,
                                       idx_1,
                                       best_cost,
                                       best_track_loc,
                                       best_track_idx,
                                       drc_cost)) {
        assignIroute_scanAscending(iroute,
                                   layer_num,
                                   start_track_idx + 1,
                                   idx_2,
                                   best_cost,
                                   best_track_loc,
                                   best_track_idx,
                                   drc_cost);
      }
    }
  } else if (next_iroute_dir > 0) {
    assignIroute_scanDescending(iroute,
                                layer_num,
                                idx_2,
                                idx_1,
                                best_cost,
                                best_track_loc,
                                best_track_idx,
                                drc_cost);
  } else if (next_iroute_dir == 0) {
    const int middle_track_idx = (idx_1 + idx_2) / 2;
    if (!assignIroute_scanAscending(iroute,
                                    layer_num,
                                    middle_track_idx,
                                    idx_2,
                                    best_cost,
                                    best_track_loc,
                                    best_track_idx,
                                    drc_cost)) {
      assignIroute_scanDescending(iroute,
                                  layer_num,
                                  middle_track_idx - 1,
                                  idx_1,
                                  best_cost,
                                  best_track_loc,
                                  best_track_idx,
                                  drc_cost);
    }
  } else {
    assignIroute_scanAscending(iroute,
                               layer_num,
                               idx_1,
                               idx_2,
                               best_cost,
                               best_track_loc,
                               best_track_idx,
                               drc_cost);
  }

  if (best_track_idx == -1) {
    auto* guide = iroute->getGuide();
    const odb::Rect box = guide->getBBox();
    constexpr int kAssignBestTrackNoTrackMsgId = 413;
    logger_->error(
        DRT,
        kAssignBestTrackNoTrackMsgId,
        "assignIroute_bestTrack could not select a track for net {} in box "
        "({}, {}) ({}, {}) on layer {} with track indices ({}, {}).",
        guide->getNet()->getName(),
        box.xMin() / dbu,
        box.yMin() / dbu,
        box.xMax() / dbu,
        box.yMax() / dbu,
        getDesign()->getTech()->getLayer(layer_num)->getName(),
        idx_1,
        idx_2);
  }
  totCost_ += drc_cost;
  iroute->setCost(drc_cost);
  return best_track_loc;
}

void FlexTAWorker::assignIroute_updateIroute(taPin* iroute,
                                             frCoord bestTrackLoc,
                                             frOrderedIdSet<taPin*>* pinS)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);

  // update coord
  for (auto& uPinFig : iroute->getFigs()) {
    if (uPinFig->typeId() == tacPathSeg) {
      auto obj = static_cast<taPathSeg*>(uPinFig.get());
      auto [bp, ep] = obj->getPoints();
      if (isH) {
        bp = {bp.x(), bestTrackLoc};
        ep = {ep.x(), bestTrackLoc};
      } else {
        bp = {bestTrackLoc, bp.y()};
        ep = {bestTrackLoc, ep.y()};
      }
      obj->setPoints(bp, ep);
    } else if (uPinFig->typeId() == tacVia) {
      auto obj = static_cast<taVia*>(uPinFig.get());
      auto bp = obj->getOrigin();
      if (isH) {
        bp = {bp.x(), bestTrackLoc};
      } else {
        bp = {bestTrackLoc, bp.y()};
      }
      obj->setOrigin(bp);
    } else {
      std::cout << "Error: assignIroute_updateIroute unsupported pinFig"
                << '\n';
      exit(1);
    }
  }
  // addCost
  for (auto& uPinFig : iroute->getFigs()) {
    addCost(uPinFig.get(), isInitTA() ? nullptr : pinS);
    workerRegionQuery.add(uPinFig.get());
  }
  iroute->addNumAssigned();
}

void FlexTAWorker::assignIroute_init(taPin* iroute,
                                     frOrderedIdSet<taPin*>* pinS)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  // subCost
  if (!isInitTA()) {
    for (auto& uPinFig : iroute->getFigs()) {
      workerRegionQuery.remove(uPinFig.get());
      subCost(uPinFig.get(), pinS);
    }
    totCost_ -= iroute->getCost();
  }
}

void FlexTAWorker::assignIroute_updateOthers(frOrderedIdSet<taPin*>& pinS)
{
  bool isH = (getDir() == dbTechLayerDir::HORIZONTAL);
  if (isInitTA()) {
    return;
  }
  for (auto& iroute : pinS) {
    if (iroute->getGuide()->getNet()->isClock() && !hardIroutesMode_) {
      continue;
    }
    removeFromReassignIroutes(iroute);
    // recalculate cost
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
      std::cout
          << "Error: FlexTAWorker::assignIroute_updateOthers does not find "
             "trackLoc"
          << '\n';
      exit(1);
    }
    totCost_ -= iroute->getCost();
    assignIroute_getCost(iroute, trackLoc, drcCost);
    iroute->setCost(drcCost);
    totCost_ += iroute->getCost();
    if (drcCost && iroute->getNumAssigned() < maxRetry_) {
      addToReassignIroutes(iroute);
    }
  }
}

void FlexTAWorker::assignIroute(taPin* iroute)
{
  frOrderedIdSet<taPin*> pinS;
  assignIroute_init(iroute, &pinS);
  frLayerNum layer_num;
  int idx_1, idx_2;
  assignIroute_availTracks(iroute, layer_num, idx_1, idx_2);
  auto best_track_loc = assignIroute_bestTrack(iroute, layer_num, idx_1, idx_2);

  assignIroute_updateIroute(iroute, best_track_loc, &pinS);
  assignIroute_updateOthers(pinS);
}

void FlexTAWorker::assign()
{
  int maxBufferSize = 20;
  std::vector<taPin*> buffers(maxBufferSize, nullptr);
  int currBufferIdx = 0;
  auto iroute = popFromReassignIroutes();
  while (iroute != nullptr) {
    auto it = std::ranges::find(buffers, iroute);
    // in the buffer, skip
    if (it != buffers.end() || iroute->getNumAssigned() >= maxRetry_) {
      ;
      // not in the buffer, re-assign
    } else {
      assignIroute(iroute);
      // re add last buffer item to reassigniroutes if drccost > 0
      // if (buffers[currBufferIdx]) {
      //  if (buffers[currBufferIdx]->getDrcCost()) {
      //    addToReassignIroutes(buffers[currBufferIdx]);
      //  }
      //}
      buffers[currBufferIdx] = iroute;
      currBufferIdx = (currBufferIdx + 1) % maxBufferSize;
      numAssigned_++;
    }
    iroute = popFromReassignIroutes();
  }
}

}  // namespace drt
