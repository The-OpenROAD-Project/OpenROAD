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

#include <omp.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "serialization.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

template <typename T>
std::vector<gtl::polygon_90_set_data<frCoord>>
FlexPA::mergePinShapes(T* pin, frInstTerm* inst_term, const bool is_shrink)
{
  frInst* inst = nullptr;
  if (inst_term) {
    inst = inst_term->getInst();
  }

  dbTransform xform;
  if (inst) {
    xform = inst->getUpdatedXform();
  }

  frTechObject* tech = getDesign()->getTech();
  std::size_t num_layers = tech->getLayers().size();

  std::vector<frCoord> layer_widths;
  if (is_shrink) {
    layer_widths.resize(num_layers, 0);
    for (int i = 0; i < int(layer_widths.size()); i++) {
      layer_widths[i] = tech->getLayer(i)->getWidth();
    }
  }

  std::vector<gtl::polygon_90_set_data<frCoord>> pin_shapes(num_layers);

  for (auto& shape : pin->getFigs()) {
    if (shape->typeId() == frcRect) {
      auto obj = static_cast<frRect*>(shape.get());
      auto layer_num = obj->getLayerNum();
      auto layer = tech->getLayer(layer_num);
      dbTechLayerDir dir = layer->getDir();
      if (layer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      Rect box = obj->getBBox();
      xform.apply(box);
      gtl::rectangle_data<frCoord> rect(
          box.xMin(), box.yMin(), box.xMax(), box.yMax());
      if (is_shrink) {
        if (dir == dbTechLayerDir::HORIZONTAL) {
          gtl::shrink(rect, gtl::VERTICAL, layer_widths[layer_num] / 2);
        } else if (dir == dbTechLayerDir::VERTICAL) {
          gtl::shrink(rect, gtl::HORIZONTAL, layer_widths[layer_num] / 2);
        }
      }
      using boost::polygon::operators::operator+=;
      pin_shapes[layer_num] += rect;
    } else if (shape->typeId() == frcPolygon) {
      auto obj = static_cast<frPolygon*>(shape.get());
      auto layer_num = obj->getLayerNum();
      std::vector<gtl::point_data<frCoord>> points;
      // must be copied pts
      for (Point pt : obj->getPoints()) {
        xform.apply(pt);
        points.emplace_back(pt.x(), pt.y());
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      using boost::polygon::operators::operator+=;
      pin_shapes[layer_num] += poly;
    } else {
      logger_->error(DRT, 67, "FlexPA mergePinShapes unsupported shape.");
    }
  }
  return pin_shapes;
}

/**
 *
 * @details This follows the Tao of PAO paper cost structure.
 * On track and half track are the preffered access points,
 * this function is responsible for generating them.
 * It iterates over every track coord in the range [low, high]
 * and inserts one of its coordinates on the coords map.
 * if use_nearby_grid is true it changes the access point cost to it.
 *
 * TODO:
 * This function doesn't seem to be getting the best access point.
 * it iterates through every track contained between low and high
 * and takes the first one (closest to low) not the best one (lowest cost).
 * note that std::map.insert() will not override and entry.
 * it should prioritize OnGrid access points
 */
void FlexPA::genAPOnTrack(
    std::map<frCoord, frAccessPointEnum>& coords,
    const std::map<frCoord, frAccessPointEnum>& track_coords,
    const frCoord low,
    const frCoord high,
    const bool use_nearby_grid)
{
  for (auto it = track_coords.lower_bound(low); it != track_coords.end();
       it++) {
    auto& [coord, cost] = *it;
    if (coord > high) {
      break;
    }
    if (use_nearby_grid) {
      coords.insert({coord, frAccessPointEnum::NearbyGrid});
    } else {
      coords.insert(*it);
    }
  }
}

// will not generate center for wider edge
/**
 * @details This follows the Tao of PAO paper cost structure.
 * First it iterates through the range [low, high] to check if there are
 * at least 3 possible OnTrack access points as those take priority.
 * If false it created and access points in the middle point between [low, high]
 */

void FlexPA::genAPCentered(std::map<frCoord, frAccessPointEnum>& coords,
                           const frLayerNum layer_num,
                           const frCoord low,
                           const frCoord high)
{
  // if touching two tracks, then no center??
  int candidates_on_grid = 0;
  for (auto it = coords.lower_bound(low); it != coords.end(); it++) {
    auto& [coordinate, cost] = *it;
    if (coordinate > high) {
      break;
    }
    if (cost == frAccessPointEnum::OnGrid) {
      candidates_on_grid++;
    }
  }
  if (candidates_on_grid >= 3) {
    return;
  }

  // If there are less than 3 coords OnGrid will create a Centered Access Point
  frCoord manu_grid = getDesign()->getTech()->getManufacturingGrid();
  frCoord coord = (low + high) / 2 / manu_grid * manu_grid;

  if (coords.find(coord) == coords.end()) {
    coords.insert(std::make_pair(coord, frAccessPointEnum::Center));
  } else {
    coords[coord] = std::min(coords[coord], frAccessPointEnum::Center);
  }
}

/**
 * @details This follows the Tao of PAO paper cost structure.
 * Enclosed Boundary APs satisfy via-in-pin requirement.
 * This is the worst access point adressed in the paper
 */

void FlexPA::genAPEnclosedBoundary(std::map<frCoord, frAccessPointEnum>& coords,
                                   const gtl::rectangle_data<frCoord>& rect,
                                   const frLayerNum layer_num,
                                   const bool is_curr_layer_horz)
{
  const auto rect_width = gtl::delta(rect, gtl::HORIZONTAL);
  const auto rect_height = gtl::delta(rect, gtl::VERTICAL);
  const int max_num_via_trial = 2;
  if (layer_num + 1 > getDesign()->getTech()->getTopLayerNum()) {
    return;
  }
  // hardcode first two single vias
  std::vector<frViaDef*> via_defs;
  int cnt = 0;
  for (auto& [tup, via] : layer_num_to_via_defs_[layer_num + 1][1]) {
    via_defs.push_back(via);
    cnt++;
    if (cnt >= max_num_via_trial) {
      break;
    }
  }
  for (auto& via_def : via_defs) {
    frVia via(via_def);
    const Rect box = via.getLayer1BBox();
    const auto via_width = box.dx();
    const auto via_height = box.dy();
    if (via_width > rect_width || via_height > rect_height) {
      continue;
    }
    const int coord_top = is_curr_layer_horz ? gtl::yh(rect) - box.yMax()
                                             : gtl::xh(rect) - box.xMax();
    const int coord_low = is_curr_layer_horz ? gtl::yl(rect) - box.yMin()
                                             : gtl::xl(rect) - box.xMin();
    for (const int coord : {coord_top, coord_low}) {
      if (coords.find(coord) == coords.end()) {
        coords.insert(std::make_pair(coord, frAccessPointEnum::EncOpt));
      } else {
        coords[coord] = std::min(coords[coord], frAccessPointEnum::EncOpt);
      }
    }
  }
}

// Responsible for checking if an AP is valid and configuring it
void FlexPA::gen_createAccessPoint(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& maxrect,
    const frCoord x,
    const frCoord y,
    const frLayerNum layer_num,
    const bool allow_planar,
    const bool allow_via,
    const frAccessPointEnum low_cost,
    const frAccessPointEnum high_cost)
{
  gtl::point_data<frCoord> pt(x, y);
  if (!gtl::contains(maxrect, pt) && low_cost != frAccessPointEnum::NearbyGrid
      && high_cost != frAccessPointEnum::NearbyGrid) {
    return;
  }
  Point fpt(x, y);
  if (apset.find(std::make_pair(fpt, layer_num)) != apset.end()) {
    return;
  }
  auto ap = std::make_unique<frAccessPoint>(fpt, layer_num);

  ap->setMultipleAccesses(frDirEnumPlanar, allow_planar);

  if (allow_planar) {
    const auto lower_layer = getDesign()->getTech()->getLayer(layer_num);
    // rectonly forbid wrongway planar access
    // rightway on grid only forbid off track rightway planar access
    // horz layer
    if (lower_layer->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (lower_layer->isUnidirectional()) {
        ap->setMultipleAccesses(frDirEnumVert, false);
      }
      if (lower_layer->getLef58RightWayOnGridOnlyConstraint()
          && low_cost != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
    }
    // vert layer
    if (lower_layer->getDir() == dbTechLayerDir::VERTICAL) {
      if (lower_layer->isUnidirectional()) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
      if (lower_layer->getLef58RightWayOnGridOnlyConstraint()
          && low_cost != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumVert, false);
      }
    }
  }
  ap->setAccess(frDirEnum::D, false);
  ap->setAccess(frDirEnum::U, allow_via);

  ap->setAllowVia(allow_via);
  ap->setType((frAccessPointEnum) low_cost, true);
  ap->setType((frAccessPointEnum) high_cost, false);
  if ((low_cost == frAccessPointEnum::NearbyGrid
       || high_cost == frAccessPointEnum::NearbyGrid)) {
    Point end;
    const int half_width
        = design_->getTech()->getLayer(ap->getLayerNum())->getMinWidth() / 2;
    if (fpt.x() < gtl::xl(maxrect) + half_width) {
      end.setX(gtl::xl(maxrect) + half_width);
    } else if (fpt.x() > gtl::xh(maxrect) - half_width) {
      end.setX(gtl::xh(maxrect) - half_width);
    } else {
      end.setX(fpt.x());
    }
    if (fpt.y() < gtl::yl(maxrect) + half_width) {
      end.setY(gtl::yl(maxrect) + half_width);
    } else if (fpt.y() > gtl::yh(maxrect) - half_width) {
      end.setY(gtl::yh(maxrect) - half_width);
    } else {
      end.setY(fpt.y());
    }

    Point e = fpt;
    if (fpt.x() != end.x()) {
      e.setX(end.x());
    } else if (fpt.y() != end.y()) {
      e.setY(end.y());
    }
    if (!(e == fpt)) {
      frPathSeg ps;
      ps.setPoints_safe(fpt, e);
      if (ps.getBeginPoint() == end) {
        ps.setBeginStyle(frEndStyle(frcTruncateEndStyle));
      } else if (ps.getEndPoint() == end) {
        ps.setEndStyle(frEndStyle(frcTruncateEndStyle));
      }
      ap->addPathSeg(ps);
      if (!(e == end)) {
        fpt = e;
        ps.setPoints_safe(fpt, end);
        if (ps.getBeginPoint() == end) {
          ps.setBeginStyle(frEndStyle(frcTruncateEndStyle));
        } else {
          ps.setEndStyle(frEndStyle(frcTruncateEndStyle));
        }
        ap->addPathSeg(ps);
      }
    }
  }
  aps.push_back(std::move(ap));
  apset.insert(std::make_pair(fpt, layer_num));
}

void FlexPA::gen_initializeAccessPoints(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& rect,
    const frLayerNum layer_num,
    const bool allow_planar,
    const bool allow_via,
    const bool is_layer1_horz,
    const std::map<frCoord, frAccessPointEnum>& x_coords,
    const std::map<frCoord, frAccessPointEnum>& y_coords,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  // build points;
  for (auto& [x_coord, cost_x] : x_coords) {
    for (auto& [y_coord, cost_y] : y_coords) {
      // lower full/half/center
      auto& low_cost = is_layer1_horz ? cost_y : cost_x;
      auto& high_cost = (!is_layer1_horz) ? cost_y : cost_x;
      if (low_cost == lower_type && high_cost == upper_type) {
        gen_createAccessPoint(aps,
                              apset,
                              rect,
                              x_coord,
                              y_coord,
                              layer_num,
                              allow_planar,
                              allow_via,
                              low_cost,
                              high_cost);
      }
    }
  }
}

bool FlexPA::enclosesOnTrackPlanarAccess(
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layer_num)
{
  frCoord low, high;
  frLayer* layer = getDesign()->getTech()->getLayer(layer_num);
  if (layer->isHorizontal()) {
    low = gtl::yl(rect);
    high = gtl::yh(rect);
  } else if (layer->isVertical()) {
    low = gtl::xl(rect);
    high = gtl::xh(rect);
  } else {
    logger_->error(
        DRT,
        1003,
        "enclosesPlanarAccess: layer is neither vertical or horizontal");
  }
  const auto& tracks = track_coords_[layer_num];
  const auto low_track = tracks.lower_bound(low);
  if (low_track == tracks.end()) {
    logger_->error(DRT, 1004, "enclosesPlanarAccess: low track not found");
  }
  if (low_track->first > high) {
    return false;
  }
  auto high_track = tracks.lower_bound(high);
  if (high_track != tracks.end()) {
    if (high_track->first > high) {
      high_track--;
    }
  } else {
    logger_->error(DRT, 1005, "enclosesPlanarAccess: high track not found");
  }
  if (high_track->first - low_track->first > (int) layer->getPitch()) {
    return true;
  }
  if (low_track->first - (int) layer->getWidth() / 2 < low) {
    return false;
  }
  if (high_track->first + (int) layer->getWidth() / 2 > high) {
    return false;
  }
  return true;
}

/**
 * @details Generates all necessary access points from a rectangle shape
 * In this case a rectangle is one of the pin shapes of the pin
 */
void FlexPA::genAPsFromRect(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                            std::set<std::pair<Point, frLayerNum>>& apset,
                            const gtl::rectangle_data<frCoord>& rect,
                            const frLayerNum layer_num,
                            const bool allow_planar,
                            const bool allow_via,
                            frAccessPointEnum lower_type,
                            const frAccessPointEnum upper_type,
                            const bool is_macro_cell_pin)
{
  auto layer = getDesign()->getTech()->getLayer(layer_num);
  const auto min_width_layer1 = layer->getMinWidth();
  if (std::min(gtl::delta(rect, gtl::HORIZONTAL),
               gtl::delta(rect, gtl::VERTICAL))
      < min_width_layer1) {
    return;
  }
  frLayerNum second_layer_num = 0;
  if (layer_num + 2 <= getDesign()->getTech()->getTopLayerNum()) {
    second_layer_num = layer_num + 2;
  } else if (layer_num - 2 >= getDesign()->getTech()->getBottomLayerNum()) {
    second_layer_num = layer_num - 2;
  } else {
    logger_->error(DRT, 68, "genAPsFromRect cannot find second_layer_num.");
  }
  const auto min_width_layer2
      = getDesign()->getTech()->getLayer(second_layer_num)->getMinWidth();
  auto& layer1_track_coords = track_coords_[layer_num];
  auto& layer2_track_coords = track_coords_[second_layer_num];
  const bool is_layer1_horz = (layer->getDir() == dbTechLayerDir::HORIZONTAL);

  std::map<frCoord, frAccessPointEnum> x_coords;
  std::map<frCoord, frAccessPointEnum> y_coords;
  int hwidth = layer->getWidth() / 2;
  bool use_center_line = false;
  if (is_macro_cell_pin) {
    auto rect_dir = gtl::guess_orientation(rect);
    if ((rect_dir == gtl::HORIZONTAL && is_layer1_horz)
        || (rect_dir == gtl::VERTICAL && !is_layer1_horz)) {
      auto layer_width = layer->getWidth();
      if ((rect_dir == gtl::HORIZONTAL
           && gtl::delta(rect, gtl::VERTICAL) < 2 * layer_width)
          || (rect_dir == gtl::VERTICAL
              && gtl::delta(rect, gtl::HORIZONTAL) < 2 * layer_width)) {
        use_center_line = true;
      }
    }
  }

  // gen all full/half grid coords
  const int offset = is_macro_cell_pin ? hwidth : 0;
  const int layer1_rect_min = is_layer1_horz ? gtl::yl(rect) : gtl::xl(rect);
  const int layer1_rect_max = is_layer1_horz ? gtl::yh(rect) : gtl::xh(rect);
  const int layer2_rect_min = is_layer1_horz ? gtl::xl(rect) : gtl::yl(rect);
  const int layer2_rect_max = is_layer1_horz ? gtl::xh(rect) : gtl::yh(rect);
  auto& layer1_coords = is_layer1_horz ? y_coords : x_coords;
  auto& layer2_coords = is_layer1_horz ? x_coords : y_coords;

  if (!is_macro_cell_pin || !use_center_line) {
    genAPOnTrack(
        layer1_coords, layer1_track_coords, layer1_rect_min, layer1_rect_max);
    genAPOnTrack(layer2_coords,
                 layer2_track_coords,
                 layer2_rect_min + offset,
                 layer2_rect_max - offset);
    if (lower_type >= frAccessPointEnum::Center) {
      genAPCentered(layer1_coords, layer_num, layer1_rect_min, layer1_rect_max);
    }
    if (lower_type >= frAccessPointEnum::EncOpt) {
      genAPEnclosedBoundary(layer1_coords, rect, layer_num, is_layer1_horz);
    }
    if (upper_type >= frAccessPointEnum::Center) {
      genAPCentered(layer2_coords,
                    layer_num,
                    layer2_rect_min + offset,
                    layer2_rect_max - offset);
    }
    if (upper_type >= frAccessPointEnum::EncOpt) {
      genAPEnclosedBoundary(layer2_coords, rect, layer_num, !is_layer1_horz);
    }
    if (lower_type >= frAccessPointEnum::NearbyGrid) {
      genAPOnTrack(layer1_coords,
                   layer1_track_coords,
                   layer1_rect_max,
                   layer1_rect_max + min_width_layer1,
                   true);
      genAPOnTrack(layer1_coords,
                   layer1_track_coords,
                   layer1_rect_min - min_width_layer1,
                   layer1_rect_min,
                   true);
    }
    if (upper_type >= frAccessPointEnum::NearbyGrid) {
      genAPOnTrack(layer2_coords,
                   layer2_track_coords,
                   layer2_rect_max,
                   layer2_rect_max + min_width_layer2,
                   true);
      genAPOnTrack(layer2_coords,
                   layer2_track_coords,
                   layer2_rect_min - min_width_layer2,
                   layer2_rect_min,
                   true);
    }
  } else {
    genAPOnTrack(
        layer2_coords, layer2_track_coords, layer2_rect_min, layer2_rect_max);
    if (upper_type >= frAccessPointEnum::Center) {
      genAPCentered(layer2_coords, layer_num, layer2_rect_min, layer2_rect_max);
    }
    if (upper_type >= frAccessPointEnum::EncOpt) {
      genAPEnclosedBoundary(layer2_coords, rect, layer_num, !is_layer1_horz);
    }
    if (upper_type >= frAccessPointEnum::NearbyGrid) {
      genAPOnTrack(layer2_coords,
                   layer2_track_coords,
                   layer2_rect_max,
                   layer2_rect_max + min_width_layer2,
                   true);
      genAPOnTrack(layer2_coords,
                   layer2_track_coords,
                   layer2_rect_min - min_width_layer2,
                   layer2_rect_min,
                   true);
    }
    genAPCentered(layer1_coords, layer_num, layer1_rect_min, layer1_rect_max);
    for (auto& [layer1_coord, cost] : layer1_coords) {
      layer1_coords[layer1_coord] = frAccessPointEnum::OnGrid;
    }
  }

  if (is_macro_cell_pin && use_center_line && is_layer1_horz) {
    lower_type = frAccessPointEnum::OnGrid;
  }

  gen_initializeAccessPoints(aps,
                             apset,
                             rect,
                             layer_num,
                             allow_planar,
                             allow_via,
                             is_layer1_horz,
                             x_coords,
                             y_coords,
                             lower_type,
                             upper_type);
}

void FlexPA::genAPsFromLayerShapes(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    frInstTerm* inst_term,
    const gtl::polygon_90_set_data<frCoord>& layer_shapes,
    const frLayerNum layer_num,
    bool allow_via,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  if (getDesign()->getTech()->getLayer(layer_num)->getType()
      != dbTechLayerType::ROUTING) {
    return;
  }
  bool allow_planar = true;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    dbMasterType masterType
        = inst_term->getInst()->getMaster()->getMasterType();
    if (masterType == dbMasterType::CORE
        || masterType == dbMasterType::CORE_TIEHIGH
        || masterType == dbMasterType::CORE_TIELOW
        || masterType == dbMasterType::CORE_ANTENNACELL) {
      if ((layer_num >= VIAINPIN_BOTTOMLAYERNUM
           && layer_num <= VIAINPIN_TOPLAYERNUM)
          || layer_num <= VIA_ACCESS_LAYERNUM) {
        allow_planar = false;
      }
    } else if (masterType.isBlock() || masterType.isPad()
               || masterType == dbMasterType::RING) {
      is_macro_cell_pin = true;
    }
  } else {
    // IO term is treated as the MacroCellPin as the top block
    is_macro_cell_pin = true;
    allow_planar = true;
    allow_via = false;
  }
  // lower layer is current layer
  // rightway on grid only forbid off track up via access on upper layer
  const auto upper_layer
      = (layer_num + 2 <= getDesign()->getTech()->getTopLayerNum())
            ? getDesign()->getTech()->getLayer(layer_num + 2)
            : nullptr;
  if (!is_macro_cell_pin && upper_layer
      && upper_layer->getLef58RightWayOnGridOnlyConstraint()
      && upper_type != frAccessPointEnum::OnGrid) {
    return;
  }
  std::vector<gtl::rectangle_data<frCoord>> maxrects;
  gtl::get_max_rectangles(maxrects, layer_shapes);
  for (auto& bbox_rect : maxrects) {
    genAPsFromRect(aps,
                   apset,
                   bbox_rect,
                   layer_num,
                   allow_planar,
                   allow_via,
                   lower_type,
                   upper_type,
                   is_macro_cell_pin);
  }
}

// filter off-grid coordinate
// lower on-grid 0, upper on-grid 0 = 0
// lower 1/2     1, upper on-grid 0 = 1
// lower center  2, upper on-grid 0 = 2
// lower center  2, upper center  2 = 4

template <typename T>
void FlexPA::genAPsFromPinShapes(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    T* pin,
    frInstTerm* inst_term,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  //  only VIA_ACCESS_LAYERNUM layer can have via access
  const bool allow_via = true;
  frLayerNum layer_num = (int) pin_shapes.size() - 1;
  for (auto it = pin_shapes.rbegin(); it != pin_shapes.rend(); it++) {
    if (!it->empty()
        && getDesign()->getTech()->getLayer(layer_num)->getType()
               == dbTechLayerType::ROUTING) {
      genAPsFromLayerShapes(aps,
                            apset,
                            inst_term,
                            *it,
                            layer_num,
                            allow_via,
                            lower_type,
                            upper_type);
    }
    layer_num--;
  }
}

bool FlexPA::check_endPointIsOutside(
    Point& end_point,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    const Point& begin_point,
    const frLayerNum layer_num,
    const frDirEnum dir,
    const bool is_block)
{
  const int step_size_multiplier = 3;
  frCoord x = begin_point.x();
  frCoord y = begin_point.y();
  const frCoord width = getDesign()->getTech()->getLayer(layer_num)->getWidth();
  const frCoord step_size = step_size_multiplier * width;
  const frCoord pitch = getDesign()->getTech()->getLayer(layer_num)->getPitch();
  gtl::rectangle_data<frCoord> rect;
  if (is_block) {
    gtl::extents(rect, layer_polys[0]);
    if (layer_polys.size() > 1) {
      logger_->warn(DRT, 6000, "Macro pin has more than 1 polygon");
    }
  }
  switch (dir) {
    case (frDirEnum::W):
      if (is_block) {
        x = gtl::xl(rect) - pitch;
      } else {
        x -= step_size;
      }
      break;
    case (frDirEnum::E):
      if (is_block) {
        x = gtl::xh(rect) + pitch;
      } else {
        x += step_size;
      }
      break;
    case (frDirEnum::S):
      if (is_block) {
        y = gtl::yl(rect) - pitch;
      } else {
        y -= step_size;
      }
      break;
    case (frDirEnum::N):
      if (is_block) {
        y = gtl::yh(rect) + pitch;
      } else {
        y += step_size;
      }
      break;
    default:
      logger_->error(DRT, 70, "Unexpected direction in getPlanarEP.");
  }
  end_point = {x, y};
  const gtl::point_data<frCoord> pt(x, y);
  bool outside = true;
  for (auto& layer_poly : layer_polys) {
    if (gtl::contains(layer_poly, pt)) {
      outside = false;
      break;
    }
  }

  return outside;
}

template <typename T>
void FlexPA::check_addPlanarAccess(
    frAccessPoint* ap,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    frDirEnum dir,
    T* pin,
    frInstTerm* inst_term)
{
  const Point begin_point = ap->getPoint();
  // skip viaonly access
  if (!ap->hasAccess(dir)) {
    return;
  }
  const bool is_block
      = inst_term
        && inst_term->getInst()->getMaster()->getMasterType().isBlock();
  Point end_point;
  const bool is_outside = check_endPointIsOutside(
      end_point, layer_polys, begin_point, ap->getLayerNum(), dir, is_block);
  // skip if two width within shape for standard cell
  if (!is_outside) {
    ap->setAccess(dir, false);
    return;
  }
  // TODO: EDIT HERE Wrongdirection segments
  auto layer = getDesign()->getTech()->getLayer(ap->getLayerNum());
  auto ps = std::make_unique<frPathSeg>();
  auto style = layer->getDefaultSegStyle();
  const bool vert_dir = (dir == frDirEnum::S || dir == frDirEnum::N);
  const bool wrong_dir
      = (layer->getDir() == dbTechLayerDir::HORIZONTAL && vert_dir)
        || (layer->getDir() == dbTechLayerDir::VERTICAL && !vert_dir);
  if (dir == frDirEnum::W || dir == frDirEnum::S) {
    ps->setPoints(end_point, begin_point);
    style.setEndStyle(frcTruncateEndStyle, 0);
  } else {
    ps->setPoints(begin_point, end_point);
    style.setBeginStyle(frcTruncateEndStyle, 0);
  }
  if (wrong_dir) {
    style.setWidth(layer->getWrongDirWidth());
  }
  ps->setLayerNum(ap->getLayerNum());
  ps->setStyle(style);
  if (inst_term && inst_term->hasNet()) {
    ps->addToNet(inst_term->getNet());
  } else {
    ps->addToPin(pin);
  }

  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = layer->getPitch();
  const auto extension = 5 * pitch;
  Rect tmp_box(begin_point, begin_point);
  Rect ext_box;
  tmp_box.bloat(extension, ext_box);
  design_rule_checker.setExtBox(ext_box);
  design_rule_checker.setDrcBox(ext_box);
  if (inst_term) {
    design_rule_checker.addTargetObj(inst_term->getInst());
  } else {
    design_rule_checker.addTargetObj(pin->getTerm());
  }
  design_rule_checker.initPA0(getDesign());
  auto pin_term = pin->getTerm();
  frBlockObject* owner;
  if (inst_term) {
    if (inst_term->hasNet()) {
      owner = inst_term->getNet();
    } else {
      owner = inst_term;
    }
  } else {
    if (pin_term->hasNet()) {
      owner = pin_term->getNet();
    } else {
      owner = pin_term;
    }
  }
  design_rule_checker.addPAObj(ps.get(), owner);
  for (auto& apPs : ap->getPathSegs()) {
    design_rule_checker.addPAObj(&apPs, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();
  design_rule_checker.end();

  const bool no_drv = design_rule_checker.getMarkers().empty();
  ap->setAccess(dir, no_drv);

  if (graphics_) {
    graphics_->setPlanarAP(ap, ps.get(), design_rule_checker.getMarkers());
  }
}

void FlexPA::getViasFromMetalWidthMap(
    const Point& pt,
    const frLayerNum layer_num,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    std::vector<std::pair<int, frViaDef*>>& via_defs)
{
  const auto tech = getTech();
  if (layer_num == tech->getTopLayerNum()) {
    return;
  }
  const auto cut_layer = tech->getLayer(layer_num + 1)->getDbLayer();
  // If the upper layer has an NDR special handling will be needed
  // here. Assuming normal min-width routing for now.
  const frCoord top_width = tech->getLayer(layer_num + 2)->getMinWidth();
  const auto width_orient
      = tech->isHorizontalLayer(layer_num) ? gtl::VERTICAL : gtl::HORIZONTAL;
  frCoord bottom_width = -1;
  auto viaMap = cut_layer->getTech()->getMetalWidthViaMap();
  for (auto entry : viaMap) {
    if (entry->getCutLayer() != cut_layer) {
      continue;
    }

    if (entry->isPgVia()) {
      continue;
    }

    if (entry->isViaCutClass()) {
      logger_->warn(
          DRT,
          519,
          "Via cut classes in LEF58_METALWIDTHVIAMAP are not supported.");
      continue;
    }

    if (entry->getAboveLayerWidthLow() > top_width
        || entry->getAboveLayerWidthHigh() < top_width) {
      continue;
    }

    if (bottom_width < 0) {  // compute bottom_width once
      std::vector<gtl::rectangle_data<frCoord>> maxrects;
      gtl::get_max_rectangles(maxrects, polyset);
      for (auto& rect : maxrects) {
        if (contains(rect, gtl::point_data<frCoord>(pt.x(), pt.y()))) {
          const frCoord width = delta(rect, width_orient);
          bottom_width = std::max(bottom_width, width);
        }
      }
    }

    if (entry->getBelowLayerWidthLow() > bottom_width
        || entry->getBelowLayerWidthHigh() < bottom_width) {
      continue;
    }

    via_defs.emplace_back(via_defs.size(), tech->getVia(entry->getViaName()));
  }
}

template <typename T>
void FlexPA::check_addViaAccess(
    frAccessPoint* ap,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    const frDirEnum dir,
    T* pin,
    frInstTerm* inst_term,
    bool deep_search)
{
  const Point begin_point = ap->getPoint();
  const auto layer_num = ap->getLayerNum();
  // skip planar only access
  if (!ap->isViaAllowed()) {
    return;
  }

  bool via_in_pin = false;
  const auto lower_type = ap->getType(true);
  const auto upper_type = ap->getType(false);
  if (layer_num >= VIAINPIN_BOTTOMLAYERNUM
      && layer_num <= VIAINPIN_TOPLAYERNUM) {
    via_in_pin = true;
  } else if ((lower_type == frAccessPointEnum::EncOpt
              && upper_type != frAccessPointEnum::NearbyGrid)
             || (upper_type == frAccessPointEnum::EncOpt
                 && lower_type != frAccessPointEnum::NearbyGrid)) {
    via_in_pin = true;
  }

  // check if ap is on the left/right boundary of the cell
  Rect boundary_bbox;
  bool is_side_bound = false;
  if (inst_term) {
    boundary_bbox = inst_term->getInst()->getBoundaryBBox();
    frCoord width = getDesign()->getTech()->getLayer(layer_num)->getWidth();
    if (begin_point.x() <= boundary_bbox.xMin() + 3 * width
        || begin_point.x() >= boundary_bbox.xMax() - 3 * width) {
      is_side_bound = true;
    }
  }
  const int max_num_via_trial = 2;
  // use std:pair to ensure deterministic behavior
  std::vector<std::pair<int, frViaDef*>> via_defs;
  getViasFromMetalWidthMap(begin_point, layer_num, polyset, via_defs);

  if (via_defs.empty()) {  // no via map entry
    // hardcode first two single vias
    for (auto& [tup, via_def] : layer_num_to_via_defs_[layer_num + 1][1]) {
      via_defs.emplace_back(via_defs.size(), via_def);
      if (via_defs.size() >= max_num_via_trial && !deep_search) {
        break;
      }
    }
  }

  std::set<std::tuple<frCoord, int, frViaDef*>> valid_via_defs;
  for (auto& [idx, via_def] : via_defs) {
    auto via = std::make_unique<frVia>(via_def);
    via->setOrigin(begin_point);
    const Rect box = via->getLayer1BBox();
    if (inst_term) {
      if (!boundary_bbox.contains(box)) {
        continue;
      }
      Rect layer2_boundary_box = via->getLayer2BBox();
      if (!boundary_bbox.contains(layer2_boundary_box)) {
        continue;
      }
    }

    frCoord max_ext = 0;
    const gtl::rectangle_data<frCoord> viarect(
        box.xMin(), box.yMin(), box.xMax(), box.yMax());
    using boost::polygon::operators::operator+=;
    using boost::polygon::operators::operator&=;
    gtl::polygon_90_set_data<frCoord> intersection;
    intersection += viarect;
    intersection &= polyset;
    // via ranking criteria: max extension distance beyond pin shape
    std::vector<gtl::rectangle_data<frCoord>> int_rects;
    intersection.get_rectangles(int_rects,
                                gtl::orientation_2d_enum::HORIZONTAL);
    for (const auto& r : int_rects) {
      max_ext = std::max(max_ext, box.xMax() - gtl::xh(r));
      max_ext = std::max(max_ext, gtl::xl(r) - box.xMin());
    }
    if (!is_side_bound) {
      if (int_rects.size() > 1) {
        int_rects.clear();
        intersection.get_rectangles(int_rects,
                                    gtl::orientation_2d_enum::VERTICAL);
      }
      for (const auto& r : int_rects) {
        max_ext = std::max(max_ext, box.yMax() - gtl::yh(r));
        max_ext = std::max(max_ext, gtl::yl(r) - box.yMin());
      }
    }
    if (via_in_pin && max_ext) {
      continue;
    }
    if (checkViaAccess(ap, via.get(), pin, inst_term, layer_polys)) {
      valid_via_defs.insert({max_ext, idx, via_def});
      if (valid_via_defs.size() >= max_num_via_trial) {
        break;
      }
    }
  }
  ap->setAccess(dir, !valid_via_defs.empty());
  for (auto& [ext, idx, via_def] : valid_via_defs) {
    ap->addViaDef(via_def);
  }
}

template <typename T>
bool FlexPA::checkViaAccess(
    frAccessPoint* ap,
    frVia* via,
    T* pin,
    frInstTerm* inst_term,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys)
{
  for (const frDirEnum dir : frDirEnumPlanar) {
    if (checkDirectionalViaAccess(ap, via, pin, inst_term, layer_polys, dir)) {
      return true;
    }
  }
  return false;
}

template <typename T>
bool FlexPA::checkDirectionalViaAccess(
    frAccessPoint* ap,
    frVia* via,
    T* pin,
    frInstTerm* inst_term,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    frDirEnum dir)
{
  auto upper_layer = getTech()->getLayer(via->getViaDef()->getLayer2Num());
  const bool vert_dir = (dir == frDirEnum::S || dir == frDirEnum::N);
  const bool wrong_dir = (upper_layer->isHorizontal() && vert_dir)
                         || (upper_layer->isVertical() && !vert_dir);
  auto style = upper_layer->getDefaultSegStyle();

  if (wrong_dir) {
    if (!USENONPREFTRACKS || upper_layer->isUnidirectional()) {
      return false;
    }
    style.setWidth(upper_layer->getWrongDirWidth());
  }

  const Point begin_point = ap->getPoint();
  const bool is_block
      = inst_term
        && inst_term->getInst()->getMaster()->getMasterType().isBlock();
  Point end_point;
  check_endPointIsOutside(end_point,
                          layer_polys,
                          begin_point,
                          via->getViaDef()->getLayer2Num(),
                          dir,
                          is_block);

  if (inst_term && inst_term->hasNet()) {
    via->addToNet(inst_term->getNet());
  } else {
    via->addToPin(pin);
  }
  // PS
  auto ps = std::make_unique<frPathSeg>();
  if (dir == frDirEnum::W || dir == frDirEnum::S) {
    ps->setPoints(end_point, begin_point);
    style.setEndStyle(frcTruncateEndStyle, 0);
  } else {
    ps->setPoints(begin_point, end_point);
    style.setBeginStyle(frcTruncateEndStyle, 0);
  }
  ps->setLayerNum(upper_layer->getLayerNum());
  ps->setStyle(style);
  if (inst_term && inst_term->hasNet()) {
    ps->addToNet(inst_term->getNet());
  } else {
    ps->addToPin(pin);
  }

  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreLongSideEOL();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = getTech()->getLayer(ap->getLayerNum())->getPitch();
  const auto extension = 5 * pitch;
  Rect tmp_box(begin_point, begin_point);
  Rect ext_box;
  tmp_box.bloat(extension, ext_box);
  auto pin_term = pin->getTerm();
  auto pin_net = pin_term->getNet();
  design_rule_checker.setExtBox(ext_box);
  design_rule_checker.setDrcBox(ext_box);
  if (inst_term) {
    if (!inst_term->getNet() || !inst_term->getNet()->getNondefaultRule()
        || AUTO_TAPER_NDR_NETS) {
      design_rule_checker.addTargetObj(inst_term->getInst());
    }
  } else {
    if (!pin_net || !pin_net->getNondefaultRule() || AUTO_TAPER_NDR_NETS) {
      design_rule_checker.addTargetObj(pin_term);
    }
  }

  design_rule_checker.initPA0(getDesign());
  frBlockObject* owner;
  if (inst_term) {
    if (inst_term->hasNet()) {
      owner = inst_term->getNet();
    } else {
      owner = inst_term;
    }
  } else {
    if (pin_term->hasNet()) {
      owner = pin_net;
    } else {
      owner = pin_term;
    }
  }
  design_rule_checker.addPAObj(ps.get(), owner);
  design_rule_checker.addPAObj(via, owner);
  for (auto& apPs : ap->getPathSegs()) {
    design_rule_checker.addPAObj(&apPs, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();
  design_rule_checker.end();

  const bool no_drv = design_rule_checker.getMarkers().empty();

  if (graphics_) {
    graphics_->setViaAP(ap, via, design_rule_checker.getMarkers());
  }
  return no_drv;
}

template <typename T>
void FlexPA::check_addAccess(
    frAccessPoint* ap,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    const std::vector<gtl::polygon_90_data<frCoord>>& polys,
    T* pin,
    frInstTerm* inst_term,
    bool deep_search)
{
  if (!deep_search) {
    for (const frDirEnum dir : frDirEnumPlanar) {
      check_addPlanarAccess(ap, polys, dir, pin, inst_term);
    }
  }
  check_addViaAccess(
      ap, polys, polyset, frDirEnum::U, pin, inst_term, deep_search);
}

template <typename T>
void FlexPA::check_setAPsAccesses(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const bool& is_std_cell_pin)
{
  std::vector<std::vector<gtl::polygon_90_data<frCoord>>> layer_polys(
      pin_shapes.size());
  for (int i = 0; i < (int) pin_shapes.size(); i++) {
    pin_shapes[i].get_polygons(layer_polys[i]);
  }
  bool has_access = false;
  for (auto& ap : aps) {
    const auto layer_num = ap->getLayerNum();
    check_addAccess(ap.get(),
                    pin_shapes[layer_num],
                    layer_polys[layer_num],
                    pin,
                    inst_term);
    if (is_std_cell_pin) {
      has_access
          |= ((layer_num == VIA_ACCESS_LAYERNUM && ap->hasAccess(frDirEnum::U))
              || (layer_num != VIA_ACCESS_LAYERNUM && ap->hasAccess()));
    } else {
      has_access |= ap->hasAccess();
    }
  }
  if (!has_access) {
    for (auto& ap : aps) {
      const auto layer_num = ap->getLayerNum();
      check_addAccess(ap.get(),
                      pin_shapes[layer_num],
                      layer_polys[layer_num],
                      pin,
                      inst_term,
                      true);
    }
  }
}

template <typename T>
void FlexPA::updatePinStats(
    const std::vector<std::unique_ptr<frAccessPoint>>& tmp_aps,
    T* pin,
    frInstTerm* inst_term)
{
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType
        = inst_term->getInst()->getMaster()->getMasterType();
    is_std_cell_pin = masterType == dbMasterType::CORE
                      || masterType == dbMasterType::CORE_TIEHIGH
                      || masterType == dbMasterType::CORE_TIELOW
                      || masterType == dbMasterType::CORE_ANTENNACELL;

    is_macro_cell_pin = masterType.isBlock() || masterType.isPad()
                        || masterType == dbMasterType::RING;
  }
  for (auto& ap : tmp_aps) {
    if (ap->hasAccess(frDirEnum::W) || ap->hasAccess(frDirEnum::E)
        || ap->hasAccess(frDirEnum::S) || ap->hasAccess(frDirEnum::N)) {
      if (is_std_cell_pin) {
#pragma omp atomic
        std_cell_pin_valid_planar_ap_cnt_++;
      }
      if (is_macro_cell_pin) {
#pragma omp atomic
        macro_cell_pin_valid_planar_ap_cnt_++;
      }
    }
    if (ap->hasAccess(frDirEnum::U)) {
      if (is_std_cell_pin) {
#pragma omp atomic
        std_cell_pin_valid_via_ap_cnt_++;
      }
      if (is_macro_cell_pin) {
#pragma omp atomic
        macro_cell_pin_valid_via_ap_cnt_++;
      }
    }
  }
}

template <typename T>
bool FlexPA::initPinAccessCostBounded(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType
        = inst_term->getInst()->getMaster()->getMasterType();
    is_std_cell_pin = masterType == dbMasterType::CORE
                      || masterType == dbMasterType::CORE_TIEHIGH
                      || masterType == dbMasterType::CORE_TIELOW
                      || masterType == dbMasterType::CORE_ANTENNACELL;

    is_macro_cell_pin = masterType.isBlock() || masterType.isPad()
                        || masterType == dbMasterType::RING;
  }
  const bool is_io_pin = (inst_term == nullptr);
  std::vector<std::unique_ptr<frAccessPoint>> tmp_aps;
  genAPsFromPinShapes(
      tmp_aps, apset, pin, inst_term, pin_shapes, lower_type, upper_type);
  check_setAPsAccesses(tmp_aps, pin_shapes, pin, inst_term, is_std_cell_pin);
  if (is_std_cell_pin) {
#pragma omp atomic
    std_cell_pin_gen_ap_cnt_ += tmp_aps.size();
  }
  if (is_macro_cell_pin) {
#pragma omp atomic
    macro_cell_pin_gen_ap_cnt_ += tmp_aps.size();
  }
  if (graphics_) {
    graphics_->setAPs(tmp_aps, lower_type, upper_type);
  }
  for (auto& ap : tmp_aps) {
    // for stdcell, add (i) planar access if layer_num != VIA_ACCESS_LAYERNUM,
    // and (ii) access if exist access for macro, allow pure planar ap
    if (is_std_cell_pin) {
      const auto layer_num = ap->getLayerNum();
      if ((layer_num == VIA_ACCESS_LAYERNUM && ap->hasAccess(frDirEnum::U))
          || (layer_num != VIA_ACCESS_LAYERNUM && ap->hasAccess())) {
        aps.push_back(std::move(ap));
      }
    } else if ((is_macro_cell_pin || is_io_pin) && ap->hasAccess()) {
      aps.push_back(std::move(ap));
    }
  }
  int n_sparse_access_points = (int) aps.size();
  Rect tbx;
  for (int i = 0; i < (int) aps.size();
       i++) {  // not perfect but will do the job
    int r = design_->getTech()->getLayer(aps[i]->getLayerNum())->getWidth() / 2;
    tbx.init(
        aps[i]->x() - r, aps[i]->y() - r, aps[i]->x() + r, aps[i]->y() + r);
    for (int j = i + 1; j < (int) aps.size(); j++) {
      if (aps[i]->getLayerNum() == aps[j]->getLayerNum()
          && tbx.intersects(aps[j]->getPoint())) {
        n_sparse_access_points--;
        break;
      }
    }
  }
  if (is_std_cell_pin
      && n_sparse_access_points >= MINNUMACCESSPOINT_STDCELLPIN) {
    updatePinStats(aps, pin, inst_term);
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  if (is_macro_cell_pin
      && n_sparse_access_points >= MINNUMACCESSPOINT_MACROCELLPIN) {
    updatePinStats(aps, pin, inst_term);
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  if (is_io_pin && (int) aps.size() > 0) {
    // IO term pin always only have one access
    for (auto& ap : aps) {
      pin->getPinAccess(0)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  return false;
}

// first create all access points with costs
template <typename T>
int FlexPA::initPinAccess(T* pin, frInstTerm* inst_term)
{
  // aps are after xform
  // before checkPoints, ap->hasAccess(dir) indicates whether to check drc
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  std::set<std::pair<Point, frLayerNum>> apset;
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    // TODO there should be a better way to get this info by getting the master
    // terms from OpenDB
    dbMasterType masterType
        = inst_term->getInst()->getMaster()->getMasterType();
    is_std_cell_pin = masterType == dbMasterType::CORE
                      || masterType == dbMasterType::CORE_TIEHIGH
                      || masterType == dbMasterType::CORE_TIELOW
                      || masterType == dbMasterType::CORE_ANTENNACELL;

    is_macro_cell_pin = masterType.isBlock() || masterType.isPad()
                        || masterType == dbMasterType::RING;
  }

  if (graphics_) {
    std::set<frInst*, frBlockObjectComp>* inst_class = nullptr;
    if (inst_term) {
      inst_class = unique_insts_.getClass(inst_term->getInst());
    }
    graphics_->startPin(pin, inst_term, inst_class);
  }

  std::vector<gtl::polygon_90_set_data<frCoord>> pin_shapes
      = mergePinShapes(pin, inst_term);

  for (auto upper : {frAccessPointEnum::OnGrid,
                     frAccessPointEnum::HalfGrid,
                     frAccessPointEnum::Center,
                     frAccessPointEnum::EncOpt,
                     frAccessPointEnum::NearbyGrid}) {
    for (auto lower : {frAccessPointEnum::OnGrid,
                       frAccessPointEnum::HalfGrid,
                       frAccessPointEnum::Center,
                       frAccessPointEnum::EncOpt}) {
      if (upper == frAccessPointEnum::NearbyGrid && !aps.empty()) {
        // Only use NearbyGrid as a last resort (at least until
        // nangate45/aes is resolved).
        continue;
      }
      if (initPinAccessCostBounded(
              aps, apset, pin_shapes, pin, inst_term, lower, upper)) {
        return aps.size();
      }
    }
  }

  // inst_term aps are written back here if not early stopped
  // IO term aps are are written back in initPinAccessCostBounded and always
  // early stopped
  updatePinStats(aps, pin, inst_term);
  const int n_aps = aps.size();
  if (n_aps == 0) {
    if (is_std_cell_pin) {
      std_cell_pin_no_ap_cnt_++;
    }
    if (is_macro_cell_pin) {
      macro_cell_pin_no_ap_cnt_++;
    }
  } else {
    if (inst_term == nullptr) {
      logger_->error(DRT, 254, "inst_term can not be nullptr");
    }
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
  }
  return n_aps;
}

static inline void serializePatterns(
    const std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern>>>&
        patterns,
    const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << patterns;
  file.close();
}
static inline void serializeInstRows(
    const std::vector<std::vector<frInst*>>& inst_rows,
    const std::string& file_name)
{
  paUpdate update;
  update.setInstRows(inst_rows);
  paUpdate::serialize(update, file_name);
}

void FlexPA::initAllAccessPoints()
{
  ProfileTask profile("PA:point");
  int cnt = 0;

  omp_set_num_threads(MAX_THREADS);
  ThreadException exception;
  const auto& unique = unique_insts_.getUnique();
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int) unique.size(); i++) {  // NOLINT
    try {
      auto& inst = unique[i];
      // only do for core and block cells
      dbMasterType masterType = inst->getMaster()->getMasterType();
      if (masterType != dbMasterType::CORE
          && masterType != dbMasterType::CORE_TIEHIGH
          && masterType != dbMasterType::CORE_TIELOW
          && masterType != dbMasterType::CORE_ANTENNACELL
          && !masterType.isBlock() && !masterType.isPad()
          && masterType != dbMasterType::RING) {
        continue;
      }
      ProfileTask profile("PA:uniqueInstance");
      for (auto& inst_term : inst->getInstTerms()) {
        // only do for normal and clock terms
        if (isSkipInstTerm(inst_term.get())) {
          continue;
        }
        int n_aps = 0;
        for (auto& pin : inst_term->getTerm()->getPins()) {
          n_aps += initPinAccess(pin.get(), inst_term.get());
        }
        if (!n_aps) {
          logger_->error(DRT,
                         73,
                         "No access point for {}/{}.",
                         inst_term->getInst()->getName(),
                         inst_term->getTerm()->getName());
        }
#pragma omp critical
        {
          cnt++;
          if (VERBOSE > 0) {
            if (cnt < 10000) {
              if (cnt % 1000 == 0) {
                logger_->info(DRT, 76, "  Complete {} pins.", cnt);
              }
            } else {
              if (cnt % 10000 == 0) {
                logger_->info(DRT, 77, "  Complete {} pins.", cnt);
              }
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();

  // PA for IO terms
  if (target_insts_.empty()) {
    omp_set_num_threads(MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
    for (unsigned i = 0;  // NOLINT
         i < getDesign()->getTopBlock()->getTerms().size();
         i++) {
      try {
        auto& term = getDesign()->getTopBlock()->getTerms()[i];
        if (term->getType().isSupply()) {
          continue;
        }
        auto net = term->getNet();
        if (!net || net->isSpecial()) {
          continue;
        }
        int n_aps = 0;
        for (auto& pin : term->getPins()) {
          n_aps += initPinAccess(pin.get(), nullptr);
        }
        if (!n_aps) {
          logger_->error(
              DRT, 74, "No access point for PIN/{}.", term->getName());
        }
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
  }

  if (VERBOSE > 0) {
    logger_->info(DRT, 78, "  Complete {} pins.", cnt);
  }
}

void FlexPA::prepPatternInstRows(std::vector<std::vector<frInst*>> inst_rows)
{
  ThreadException exception;
  int cnt = 0;
  if (isDistributed()) {
    omp_set_num_threads(cloud_sz_);
    const int batch_size = inst_rows.size() / cloud_sz_;
    paUpdate all_updates;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < cloud_sz_; i++) {
      try {
        std::vector<std::vector<frInst*>>::const_iterator start
            = inst_rows.begin() + (i * batch_size);
        std::vector<std::vector<frInst*>>::const_iterator end
            = (i == cloud_sz_ - 1) ? inst_rows.end() : start + batch_size;
        std::vector<std::vector<frInst*>> batch(start, end);
        std::string path = fmt::format("{}/batch_{}.bin", shared_vol_, i);
        serializeInstRows(batch, path);
        dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                            dst::JobMessage::UNICAST),
            result;
        std::unique_ptr<PinAccessJobDescription> uDesc
            = std::make_unique<PinAccessJobDescription>();
        uDesc->setPath(path);
        uDesc->setType(PinAccessJobDescription::INST_ROWS);
        msg.setJobDescription(std::move(uDesc));
        const bool ok
            = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
        if (!ok) {
          logger_->error(utl::DRT, 329, "Error sending INST_ROWS Job to cloud");
        }
        auto desc
            = static_cast<PinAccessJobDescription*>(result.getJobDescription());
        paUpdate update;
        paUpdate::deserialize(design_, update, desc->getPath());
        for (const auto& [term, aps] : update.getGroupResults()) {
          term->setAccessPoints(aps);
        }
#pragma omp critical
        {
          for (const auto& res : update.getGroupResults()) {
            all_updates.addGroupResult(res);
          }
          cnt += batch.size();
          if (VERBOSE > 0) {
            if (cnt < 100000) {
              if (cnt % 10000 == 0) {
                logger_->info(DRT, 110, "  Complete {} groups.", cnt);
              }
            } else {
              if (cnt % 100000 == 0) {
                logger_->info(DRT, 111, "  Complete {} groups.", cnt);
              }
            }
          }
        }
      } catch (...) {
        exception.capture();
      }
    }
    // send updates back to workers
    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
        result;
    const std::string updates_path
        = fmt::format("{}/final_updates.bin", shared_vol_);
    paUpdate::serialize(all_updates, updates_path);
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    uDesc->setPath(updates_path);
    uDesc->setType(PinAccessJobDescription::UPDATE_PA);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(utl::DRT, 332, "Error sending UPDATE_PA Job to cloud");
    }
  } else {
    omp_set_num_threads(MAX_THREADS);
    // choose access pattern of a row of insts
    int rowIdx = 0;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) inst_rows.size(); i++) {  // NOLINT
      try {
        auto& instRow = inst_rows[i];
        genInstRowPattern(instRow);
#pragma omp critical
        {
          rowIdx++;
          cnt++;
          if (VERBOSE > 0) {
            if (cnt < 100000) {
              if (cnt % 10000 == 0) {
                logger_->info(DRT, 82, "  Complete {} groups.", cnt);
              }
            } else {
              if (cnt % 100000 == 0) {
                logger_->info(DRT, 83, "  Complete {} groups.", cnt);
              }
            }
          }
        }
      } catch (...) {
        exception.capture();
      }
    }
  }
  exception.rethrow();
  if (VERBOSE > 0) {
    logger_->info(DRT, 84, "  Complete {} groups.", cnt);
  }
}

void FlexPA::prepPattern()
{
  ProfileTask profile("PA:pattern");

  const auto& unique = unique_insts_.getUnique();

  // revert access points to origin
  unique_inst_patterns_.resize(unique.size());

  int cnt = 0;

  omp_set_num_threads(MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (int curr_unique_inst_idx = 0; curr_unique_inst_idx < (int) unique.size();
       curr_unique_inst_idx++) {
    try {
      auto& inst = unique[curr_unique_inst_idx];
      // only do for core and block cells
      // TODO the above comment says "block cells" but that's not what the code
      // does?
      dbMasterType masterType = inst->getMaster()->getMasterType();
      if (masterType != dbMasterType::CORE
          && masterType != dbMasterType::CORE_TIEHIGH
          && masterType != dbMasterType::CORE_TIELOW
          && masterType != dbMasterType::CORE_ANTENNACELL) {
        continue;
      }

      int num_valid_pattern = prepPatternInst(inst, curr_unique_inst_idx, 1.0);

      if (num_valid_pattern == 0) {
        // In FAx1_ASAP7_75t_R (in asap7) the pins are mostly horizontal
        // and sorting in X works poorly.  So we try again sorting in Y.
        num_valid_pattern = prepPatternInst(inst, curr_unique_inst_idx, 0.0);
        if (num_valid_pattern == 0) {
          logger_->warn(
              DRT,
              87,
              "No valid pattern for unique instance {}, master is {}.",
              inst->getName(),
              inst->getMaster()->getName());
        }
      }
#pragma omp critical
      {
        cnt++;
        if (VERBOSE > 0) {
          if (cnt < 1000) {
            if (cnt % 100 == 0) {
              logger_->info(
                  DRT, 79, "  Complete {} unique inst patterns.", cnt);
            }
          } else {
            if (cnt % 1000 == 0) {
              logger_->info(
                  DRT, 80, "  Complete {} unique inst patterns.", cnt);
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  if (VERBOSE > 0) {
    logger_->info(DRT, 81, "  Complete {} unique inst patterns.", cnt);
  }
  if (isDistributed()) {
    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
        result;
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    std::string patterns_file = fmt::format("{}/patterns.bin", shared_vol_);
    serializePatterns(unique_inst_patterns_, patterns_file);
    uDesc->setPath(patterns_file);
    uDesc->setType(PinAccessJobDescription::UPDATE_PATTERNS);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(
          utl::DRT, 330, "Error sending UPDATE_PATTERNS Job to cloud");
    }
  }

  // prep pattern for each row
  std::vector<frInst*> insts;
  std::vector<std::vector<frInst*>> inst_rows;
  std::vector<frInst*> row_insts;

  auto instLocComp = [](frInst* const& a, frInst* const& b) {
    const Point originA = a->getOrigin();
    const Point originB = b->getOrigin();
    if (originA.y() == originB.y()) {
      return (originA.x() < originB.x());
    }
    return (originA.y() < originB.y());
  };

  getInsts(insts);
  std::sort(insts.begin(), insts.end(), instLocComp);

  // gen rows of insts
  int prev_y_coord = INT_MIN;
  int prev_x_end_coord = INT_MIN;
  for (auto inst : insts) {
    Point origin = inst->getOrigin();
    if (origin.y() != prev_y_coord || origin.x() > prev_x_end_coord) {
      if (!row_insts.empty()) {
        inst_rows.push_back(row_insts);
        row_insts.clear();
      }
    }
    row_insts.push_back(inst);
    prev_y_coord = origin.y();
    Rect inst_boundary_box = inst->getBoundaryBBox();
    prev_x_end_coord = inst_boundary_box.xMax();
  }
  if (!row_insts.empty()) {
    inst_rows.push_back(row_insts);
  }
  prepPatternInstRows(std::move(inst_rows));
}

void FlexPA::revertAccessPoints()
{
  const auto& unique = unique_insts_.getUnique();
  for (auto& inst : unique) {
    const dbTransform xform = inst->getTransform();
    const Point offset(xform.getOffset());
    dbTransform revertXform;
    revertXform.setOffset(Point(-offset.getX(), -offset.getY()));
    revertXform.setOrient(dbOrientType::R0);

    const auto pin_access_idx = unique_insts_.getPAIndex(inst);
    for (auto& inst_term : inst->getInstTerms()) {
      // if (isSkipInstTerm(inst_term.get())) {
      //   continue;
      // }

      for (auto& pin : inst_term->getTerm()->getPins()) {
        auto pin_access = pin->getPinAccess(pin_access_idx);
        for (auto& access_point : pin_access->getAccessPoints()) {
          Point unique_AP_point(access_point->getPoint());
          revertXform.apply(unique_AP_point);
          access_point->setPoint(unique_AP_point);
          for (auto& ps : access_point->getPathSegs()) {
            Point begin = ps.getBeginPoint();
            Point end = ps.getEndPoint();
            revertXform.apply(begin);
            revertXform.apply(end);
            if (end < begin) {
              Point tmp = begin;
              begin = end;
              end = tmp;
            }
            ps.setPoints(begin, end);
          }
        }
      }
    }
  }
}

// calculate which pattern to be used for each inst
// the insts must be in the same row and sorted from left to right
void FlexPA::genInstRowPattern(std::vector<frInst*>& insts)
{
  if (insts.empty()) {
    return;
  }

  const int numNode = (insts.size() + 2) * ACCESS_PATTERN_END_ITERATION_NUM;

  std::vector<FlexDPNode> nodes(numNode);

  genInstRowPatternInit(nodes, insts);
  genInstRowPatternPerform(nodes, insts);
  genInstRowPattern_commit(nodes, insts);
}

// init dp node array for valid access patterns
void FlexPA::genInstRowPatternInit(std::vector<FlexDPNode>& nodes,
                                   const std::vector<frInst*>& insts)
{
  // init virtual nodes
  const int start_node_idx
      = getFlatIdx(-1, 0, ACCESS_PATTERN_END_ITERATION_NUM);
  const int end_node_Idx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  nodes[start_node_idx].setNodeCost(0);
  nodes[start_node_idx].setPathCost(0);
  nodes[end_node_Idx].setNodeCost(0);

  // init inst nodes
  for (int idx_1 = 0; idx_1 < (int) insts.size(); idx_1++) {
    auto& inst = insts[idx_1];
    const int unique_inst_idx = unique_insts_.getIndex(inst);
    auto& inst_patterns = unique_inst_patterns_[unique_inst_idx];
    for (int idx_2 = 0; idx_2 < (int) inst_patterns.size(); idx_2++) {
      const int node_idx
          = getFlatIdx(idx_1, idx_2, ACCESS_PATTERN_END_ITERATION_NUM);
      auto access_pattern = inst_patterns[idx_2].get();
      nodes[node_idx].setNodeCost(access_pattern->getCost());
    }
  }
}

void FlexPA::genInstRowPatternPerform(std::vector<FlexDPNode>& nodes,
                                      const std::vector<frInst*>& insts)
{
  for (int curr_idx_1 = 0; curr_idx_1 <= (int) insts.size(); curr_idx_1++) {
    for (int curr_idx_2 = 0; curr_idx_2 < ACCESS_PATTERN_END_ITERATION_NUM;
         curr_idx_2++) {
      const auto curr_node_idx = getFlatIdx(
          curr_idx_1, curr_idx_2, ACCESS_PATTERN_END_ITERATION_NUM);
      auto& curr_node = nodes[curr_node_idx];
      if (curr_node.getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      const int prev_idx_1 = curr_idx_1 - 1;
      for (int prev_idx_2 = 0; prev_idx_2 < ACCESS_PATTERN_END_ITERATION_NUM;
           prev_idx_2++) {
        const int prev_node_idx = getFlatIdx(
            prev_idx_1, prev_idx_2, ACCESS_PATTERN_END_ITERATION_NUM);
        const auto& prev_node = nodes[prev_node_idx];
        if (prev_node.getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        const int edge_cost
            = getEdgeCost(prev_node_idx, curr_node_idx, nodes, insts);
        if (curr_node.getPathCost() == std::numeric_limits<int>::max()
            || curr_node.getPathCost() > prev_node.getPathCost() + edge_cost) {
          curr_node.setPathCost(prev_node.getPathCost() + edge_cost);
          curr_node.setPrevNodeIdx(prev_node_idx);
        }
      }
    }
  }
}

void FlexPA::genInstRowPattern_commit(std::vector<FlexDPNode>& nodes,
                                      const std::vector<frInst*>& insts)
{
  const bool is_debug_mode = false;
  int curr_node_idx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  auto curr_node = &(nodes[curr_node_idx]);
  int inst_cnt = insts.size();
  std::vector<int> inst_access_pattern_idx(insts.size(), -1);
  while (curr_node->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (inst_cnt != (int) insts.size()) {
      int curr_idx_1, curr_idx_2;
      getNestedIdx(curr_node_idx,
                   curr_idx_1,
                   curr_idx_2,
                   ACCESS_PATTERN_END_ITERATION_NUM);
      inst_access_pattern_idx[curr_idx_1] = curr_idx_2;

      auto& inst = insts[curr_idx_1];
      int access_point_idx = 0;
      const int unique_inst_idx = unique_insts_.getIndex(inst);
      auto access_pattern
          = unique_inst_patterns_[unique_inst_idx][curr_idx_2].get();
      auto& access_points = access_pattern->getPattern();

      // update inst_term ap
      for (auto& inst_term : inst->getInstTerms()) {
        if (isSkipInstTerm(inst_term.get())) {
          continue;
        }

        int pin_idx = 0;
        // to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size());
             i++) {
          auto& access_point = access_points[access_point_idx];
          inst_term->setAccessPoint(pin_idx, access_point);
          pin_idx++;
          access_point_idx++;
        }
      }
    }
    curr_node_idx = curr_node->getPrevNodeIdx();
    curr_node = &(nodes[curr_node->getPrevNodeIdx()]);
    inst_cnt--;
  }

  if (inst_cnt != -1) {
    std::string inst_names;
    for (frInst* inst : insts) {
      inst_names += '\n' + inst->getName();
    }
    logger_->error(DRT,
                   85,
                   "Valid access pattern combination not found for {}",
                   inst_names);
  }

  if (is_debug_mode) {
    genInstRowPattern_print(nodes, insts);
  }
}

void FlexPA::genInstRowPattern_print(std::vector<FlexDPNode>& nodes,
                                     const std::vector<frInst*>& insts)
{
  int curr_node_idx
      = getFlatIdx(insts.size(), 0, ACCESS_PATTERN_END_ITERATION_NUM);
  auto curr_node = &(nodes[curr_node_idx]);
  int inst_cnt = insts.size();
  std::vector<int> inst_access_pattern_idx(insts.size(), -1);

  while (curr_node->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (inst_cnt != (int) insts.size()) {
      int curr_idx_1, curr_idx_2;
      getNestedIdx(curr_node_idx,
                   curr_idx_1,
                   curr_idx_2,
                   ACCESS_PATTERN_END_ITERATION_NUM);
      inst_access_pattern_idx[curr_idx_1] = curr_idx_2;

      // print debug information
      auto& inst = insts[curr_idx_1];
      int access_point_idx = 0;
      const int unique_inst_idx = unique_insts_.getIndex(inst);
      auto access_pattern
          = unique_inst_patterns_[unique_inst_idx][curr_idx_2].get();
      auto& access_points = access_pattern->getPattern();

      for (auto& inst_term : inst->getInstTerms()) {
        if (isSkipInstTerm(inst_term.get())) {
          continue;
        }

        // for (auto &pin: inst_term->getTerm()->getPins()) {
        //  to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size());
             i++) {
          auto& access_point = access_points[access_point_idx];
          if (access_point) {
            const Point& pt(access_point->getPoint());
            if (inst_term->hasNet()) {
              std::cout << " gcclean2via " << inst->getName() << " "
                        << inst_term->getTerm()->getName() << " "
                        << access_point->getViaDef()->getName() << " " << pt.x()
                        << " " << pt.y() << " " << inst->getOrient().getString()
                        << "\n";
              inst_term_valid_via_ap_cnt_++;
            }
          }
          access_point_idx++;
        }
      }
    }
    curr_node_idx = curr_node->getPrevNodeIdx();
    curr_node = &(nodes[curr_node->getPrevNodeIdx()]);
    inst_cnt--;
  }

  std::cout << std::flush;

  if (inst_cnt != -1) {
    logger_->error(DRT, 276, "Valid access pattern combination not found.");
  }
}

int FlexPA::getEdgeCost(const int prev_node_idx,
                        const int curr_node_idx,
                        const std::vector<FlexDPNode>& nodes,
                        const std::vector<frInst*>& insts)
{
  int edge_cost = 0;
  int prev_idx_1, prev_idx_2, curr_idx_1, curr_idx_2;
  getNestedIdx(
      prev_node_idx, prev_idx_1, prev_idx_2, ACCESS_PATTERN_END_ITERATION_NUM);
  getNestedIdx(
      curr_node_idx, curr_idx_1, curr_idx_2, ACCESS_PATTERN_END_ITERATION_NUM);
  if (prev_idx_1 == -1 || curr_idx_1 == (int) insts.size()) {
    return edge_cost;
  }

  // check DRC
  std::vector<std::unique_ptr<frVia>> temp_vias;
  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
  // push the vias from prev inst access pattern and curr inst access pattern
  const auto prev_inst = insts[prev_idx_1];
  const auto prev_unique_inst_idx = unique_insts_.getIndex(prev_inst);
  const auto curr_inst = insts[curr_idx_1];
  const auto curr_unique_inst_idx = unique_insts_.getIndex(curr_inst);
  const auto prev_pin_access_pattern
      = unique_inst_patterns_[prev_unique_inst_idx][prev_idx_2].get();
  const auto curr_pin_access_pattern
      = unique_inst_patterns_[curr_unique_inst_idx][curr_idx_2].get();
  addAccessPatternObj(
      prev_inst, prev_pin_access_pattern, objs, temp_vias, true);
  addAccessPatternObj(
      curr_inst, curr_pin_access_pattern, objs, temp_vias, false);

  const bool has_vio = !genPatterns_gc({prev_inst, curr_inst}, objs, Edge);
  if (!has_vio) {
    const int prev_node_cost = nodes[prev_node_idx].getNodeCost();
    const int curr_node_cost = nodes[curr_node_idx].getNodeCost();
    edge_cost = (prev_node_cost + curr_node_cost) / 2;
  } else {
    edge_cost = 1000;
  }

  return edge_cost;
}

void FlexPA::addAccessPatternObj(
    frInst* inst,
    FlexPinAccessPattern* access_pattern,
    std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    std::vector<std::unique_ptr<frVia>>& vias,
    const bool isPrev)
{
  const dbTransform xform = inst->getUpdatedXform(true);
  int access_point_idx = 0;
  auto& access_points = access_pattern->getPattern();

  for (auto& inst_term : inst->getInstTerms()) {
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }

    // to avoid unused variable warning in GCC
    for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size()); i++) {
      auto& access_point = access_points[access_point_idx];
      if (!access_point
          || (isPrev && access_point != access_pattern->getBoundaryAP(false))) {
        access_point_idx++;
        continue;
      }
      if ((!isPrev) && access_point != access_pattern->getBoundaryAP(true)) {
        access_point_idx++;
        continue;
      }
      if (access_point->hasAccess(frDirEnum::U)) {
        auto via = std::make_unique<frVia>(access_point->getViaDef());
        Point pt(access_point->getPoint());
        xform.apply(pt);
        via->setOrigin(pt);
        auto rvia = via.get();
        if (inst_term->hasNet()) {
          objs.emplace_back(rvia, inst_term->getNet());
        } else {
          objs.emplace_back(rvia, inst_term.get());
        }
        vias.push_back(std::move(via));
      }
      access_point_idx++;
    }
  }
}

void FlexPA::getInsts(std::vector<frInst*>& insts)
{
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  }
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    if (!unique_insts_.hasUnique(inst.get())) {
      continue;
    }
    dbMasterType masterType = inst->getMaster()->getMasterType();
    if (masterType != dbMasterType::CORE
        && masterType != dbMasterType::CORE_TIEHIGH
        && masterType != dbMasterType::CORE_TIELOW
        && masterType != dbMasterType::CORE_ANTENNACELL) {
      continue;
    }
    bool is_skip = true;
    for (auto& inst_term : inst->getInstTerms()) {
      if (!isSkipInstTerm(inst_term.get())) {
        is_skip = false;
        break;
      }
    }
    if (!is_skip) {
      insts.push_back(inst.get());
    }
  }
}

// Skip power pins, pins connected to special nets, and dangling pins
// (since we won't route these).
//
// Checks only this inst_term and not an equivalent ones.  This
// is a helper to isSkipInstTerm and initSkipInstTerm.
bool FlexPA::isSkipInstTermLocal(frInstTerm* in)
{
  auto term = in->getTerm();
  if (term->getType().isSupply()) {
    return true;
  }
  auto in_net = in->getNet();
  if (in_net && !in_net->isSpecial()) {
    return false;
  }
  return true;
}

bool FlexPA::isSkipInstTerm(frInstTerm* in)
{
  auto inst_class = unique_insts_.getClass(in->getInst());
  if (inst_class == nullptr) {
    return isSkipInstTermLocal(in);
  }

  // This should be already computed in initSkipInstTerm()
  return skip_unique_inst_term_.at({inst_class, in->getTerm()});
}

// the input inst must be unique instance
int FlexPA::prepPatternInst(frInst* inst,
                            const int curr_unique_inst_idx,
                            const double x_weight)
{
  std::vector<std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>> pins;
  // TODO: add assert in case input inst is not unique inst
  int pin_access_idx = unique_insts_.getPAIndex(inst);
  for (auto& inst_term : inst->getInstTerms()) {
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }
    int n_aps = 0;
    for (auto& pin : inst_term->getTerm()->getPins()) {
      // container of access points
      auto pin_access = pin->getPinAccess(pin_access_idx);
      int sum_x_coord = 0;
      int sum_y_coord = 0;
      int cnt = 0;
      // get avg x coord for sort
      for (auto& access_point : pin_access->getAccessPoints()) {
        sum_x_coord += access_point->getPoint().x();
        sum_y_coord += access_point->getPoint().y();
        cnt++;
      }
      n_aps += cnt;
      if (cnt != 0) {
        const double coord
            = (x_weight * sum_x_coord + (1.0 - x_weight) * sum_y_coord) / cnt;
        pins.push_back({(int) std::round(coord), {pin.get(), inst_term.get()}});
      }
    }
    if (n_aps == 0 && !inst_term->getTerm()->getPins().empty()) {
      logger_->error(DRT, 86, "Pin does not have an access point.");
    }
  }
  std::sort(pins.begin(),
            pins.end(),
            [](const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& lhs,
               const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& rhs) {
              return lhs.first < rhs.first;
            });

  std::vector<std::pair<frMPin*, frInstTerm*>> pin_inst_term_pairs;
  pin_inst_term_pairs.reserve(pins.size());
  for (auto& [x, m] : pins) {
    pin_inst_term_pairs.push_back(m);
  }

  return genPatterns(pin_inst_term_pairs, curr_unique_inst_idx);
}

int FlexPA::genPatterns(
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int curr_unique_inst_idx)
{
  if (pins.empty()) {
    return -1;
  }

  int max_access_point_size = 0;
  int pin_access_idx = unique_insts_.getPAIndex(pins[0].second->getInst());
  for (auto& [pin, inst_term] : pins) {
    max_access_point_size
        = std::max(max_access_point_size,
                   pin->getPinAccess(pin_access_idx)->getNumAccessPoints());
  }
  if (max_access_point_size == 0) {
    return 0;
  }

  // moved for mt
  std::set<std::vector<int>> inst_access_patterns;
  std::set<std::pair<int, int>> used_access_points;
  std::set<std::pair<int, int>> viol_access_points;
  int num_valid_pattern = 0;

  num_valid_pattern += FlexPA::genPatterns_helper(pins,
                                                  inst_access_patterns,
                                                  used_access_points,
                                                  viol_access_points,
                                                  curr_unique_inst_idx,
                                                  max_access_point_size);
  // try reverse order if no valid pattern
  if (num_valid_pattern == 0) {
    auto reversed_pins = pins;
    reverse(reversed_pins.begin(), reversed_pins.end());

    num_valid_pattern += FlexPA::genPatterns_helper(reversed_pins,
                                                    inst_access_patterns,
                                                    used_access_points,
                                                    viol_access_points,
                                                    curr_unique_inst_idx,
                                                    max_access_point_size);
  }

  return num_valid_pattern;
}

int FlexPA::genPatterns_helper(
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
    const int max_access_point_size)
{
  int numNode = (pins.size() + 2) * max_access_point_size;
  int numEdge = numNode * max_access_point_size;
  int num_valid_pattern = 0;

  std::vector<FlexDPNode> nodes(numNode);
  std::vector<int> vioEdge(numEdge, -1);

  genPatternsInit(nodes,
                  pins,
                  inst_access_patterns,
                  used_access_points,
                  viol_access_points,
                  max_access_point_size);
  for (int i = 0; i < ACCESS_PATTERN_END_ITERATION_NUM; i++) {
    genPatterns_reset(nodes, pins, max_access_point_size);
    genPatterns_perform(nodes,
                        pins,
                        vioEdge,
                        used_access_points,
                        viol_access_points,
                        curr_unique_inst_idx,
                        max_access_point_size);
    bool is_valid = false;
    if (genPatterns_commit(nodes,
                           pins,
                           is_valid,
                           inst_access_patterns,
                           used_access_points,
                           viol_access_points,
                           curr_unique_inst_idx,
                           max_access_point_size)) {
      if (is_valid) {
        num_valid_pattern++;
      } else {
      }
    } else {
      break;
    }
  }
  return num_valid_pattern;
}

// init dp node array for valid access points
void FlexPA::genPatternsInit(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    int max_access_point_size)
{
  // clear temp storage and flag
  inst_access_patterns.clear();
  used_access_points.clear();
  viol_access_points.clear();

  // init virtual nodes
  int start_node_idx = getFlatIdx(-1, 0, max_access_point_size);
  int end_node_Idx = getFlatIdx(pins.size(), 0, max_access_point_size);
  nodes[start_node_idx].setNodeCost(0);
  nodes[start_node_idx].setPathCost(0);
  nodes[end_node_Idx].setNodeCost(0);
  // init pin nodes
  int pin_idx = 0;
  int apIdx = 0;
  int pin_access_idx = unique_insts_.getPAIndex(pins[0].second->getInst());

  for (auto& [pin, inst_term] : pins) {
    apIdx = 0;
    for (auto& ap : pin->getPinAccess(pin_access_idx)->getAccessPoints()) {
      int node_idx = getFlatIdx(pin_idx, apIdx, max_access_point_size);
      nodes[node_idx].setNodeCost(ap->getCost());
      apIdx++;
    }
    pin_idx++;
  }
}

void FlexPA::genPatterns_reset(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int max_access_point_size)
{
  for (auto& node : nodes) {
    node.setPathCost(std::numeric_limits<int>::max());
    node.setPrevNodeIdx(-1);
  }

  int start_node_idx = getFlatIdx(-1, 0, max_access_point_size);
  int end_node_Idx = getFlatIdx(pins.size(), 0, max_access_point_size);
  nodes[start_node_idx].setNodeCost(0);
  nodes[start_node_idx].setPathCost(0);
  nodes[end_node_Idx].setNodeCost(0);
}

bool FlexPA::genPatterns_gc(
    const std::set<frBlockObject*>& target_objs,
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    const PatternType pattern_type,
    std::set<frBlockObject*>* owners)
{
  if (objs.empty()) {
    if (VERBOSE > 1) {
      logger_->warn(DRT, 89, "genPattern_gc objs empty.");
    }
    return true;
  }

  FlexGCWorker design_rule_checker(getTech(), logger_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreLongSideEOL();
  design_rule_checker.setIgnoreCornerSpacing();

  frCoord llx = std::numeric_limits<frCoord>::max();
  frCoord lly = std::numeric_limits<frCoord>::max();
  frCoord urx = std::numeric_limits<frCoord>::min();
  frCoord ury = std::numeric_limits<frCoord>::min();
  for (auto& [connFig, owner] : objs) {
    Rect bbox = connFig->getBBox();
    llx = std::min(llx, bbox.xMin());
    lly = std::min(lly, bbox.yMin());
    urx = std::max(urx, bbox.xMax());
    ury = std::max(ury, bbox.yMax());
  }
  const Rect ext_box(llx - 3000, lly - 3000, urx + 3000, ury + 3000);
  design_rule_checker.setExtBox(ext_box);
  design_rule_checker.setDrcBox(ext_box);

  design_rule_checker.setTargetObjs(target_objs);
  if (target_objs.empty()) {
    design_rule_checker.setIgnoreDB();
  }
  design_rule_checker.initPA0(getDesign());
  for (auto& [connFig, owner] : objs) {
    design_rule_checker.addPAObj(connFig, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();
  design_rule_checker.end();

  const bool no_drv = design_rule_checker.getMarkers().empty();
  if (owners) {
    for (auto& marker : design_rule_checker.getMarkers()) {
      for (auto& src : marker->getSrcs()) {
        owners->insert(src);
      }
    }
  }
  if (graphics_) {
    graphics_->setObjsAndMakers(
        objs, design_rule_checker.getMarkers(), pattern_type);
  }
  return no_drv;
}

void FlexPA::genPatterns_perform(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
    const int max_access_point_size)
{
  for (int curr_idx_1 = 0; curr_idx_1 <= (int) pins.size(); curr_idx_1++) {
    for (int curr_idx_2 = 0; curr_idx_2 < max_access_point_size; curr_idx_2++) {
      auto curr_node_idx
          = getFlatIdx(curr_idx_1, curr_idx_2, max_access_point_size);
      auto& curr_node = nodes[curr_node_idx];
      if (curr_node.getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      int prev_idx_1 = curr_idx_1 - 1;
      for (int prev_idx_2 = 0; prev_idx_2 < max_access_point_size;
           prev_idx_2++) {
        const int prev_node_idx
            = getFlatIdx(prev_idx_1, prev_idx_2, max_access_point_size);
        auto& prev_node = nodes[prev_node_idx];
        if (prev_node.getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        const int edge_cost = getEdgeCost(prev_node_idx,
                                          curr_node_idx,
                                          nodes,
                                          pins,
                                          vio_edges,
                                          used_access_points,
                                          viol_access_points,
                                          curr_unique_inst_idx,
                                          max_access_point_size);
        if (curr_node.getPathCost() == std::numeric_limits<int>::max()
            || curr_node.getPathCost() > prev_node.getPathCost() + edge_cost) {
          curr_node.setPathCost(prev_node.getPathCost() + edge_cost);
          curr_node.setPrevNodeIdx(prev_node_idx);
        }
      }
    }
  }
}

int FlexPA::getEdgeCost(
    const int prev_node_idx,
    const int curr_node_idx,
    const std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
    const int max_access_point_size)
{
  int edge_cost = 0;
  int prev_idx_1, prev_idx_2, curr_idx_1, curr_idx_2;
  getNestedIdx(prev_node_idx, prev_idx_1, prev_idx_2, max_access_point_size);
  getNestedIdx(curr_node_idx, curr_idx_1, curr_idx_2, max_access_point_size);
  if (prev_idx_1 == -1 || curr_idx_1 == (int) pins.size()) {
    return edge_cost;
  }

  bool has_vio = false;
  // check if the edge has been calculated
  int edge_idx = getFlatEdgeIdx(
      prev_idx_1, prev_idx_2, curr_idx_2, max_access_point_size);
  if (vio_edges[edge_idx] != -1) {
    has_vio = (vio_edges[edge_idx] == 1);
  } else {
    auto curr_unique_inst = unique_insts_.getUnique(curr_unique_inst_idx);
    dbTransform xform = curr_unique_inst->getUpdatedXform(true);
    // check DRC
    std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
    const auto& [pin_1, inst_term_1] = pins[prev_idx_1];
    const auto target_obj = inst_term_1->getInst();
    const int pin_access_idx = unique_insts_.getPAIndex(target_obj);
    const auto pa_1 = pin_1->getPinAccess(pin_access_idx);
    std::unique_ptr<frVia> via_1;
    if (pa_1->getAccessPoint(prev_idx_2)->hasAccess(frDirEnum::U)) {
      via_1 = std::make_unique<frVia>(
          pa_1->getAccessPoint(prev_idx_2)->getViaDef());
      Point pt1(pa_1->getAccessPoint(prev_idx_2)->getPoint());
      xform.apply(pt1);
      via_1->setOrigin(pt1);
      if (inst_term_1->hasNet()) {
        objs.emplace_back(via_1.get(), inst_term_1->getNet());
      } else {
        objs.emplace_back(via_1.get(), inst_term_1);
      }
    }

    const auto& [pin_2, inst_term_2] = pins[curr_idx_1];
    const auto pa_2 = pin_2->getPinAccess(pin_access_idx);
    std::unique_ptr<frVia> via_2;
    if (pa_2->getAccessPoint(curr_idx_2)->hasAccess(frDirEnum::U)) {
      via_2 = std::make_unique<frVia>(
          pa_2->getAccessPoint(curr_idx_2)->getViaDef());
      Point pt2(pa_2->getAccessPoint(curr_idx_2)->getPoint());
      xform.apply(pt2);
      via_2->setOrigin(pt2);
      if (inst_term_2->hasNet()) {
        objs.emplace_back(via_2.get(), inst_term_2->getNet());
      } else {
        objs.emplace_back(via_2.get(), inst_term_2);
      }
    }

    has_vio = !genPatterns_gc({target_obj}, objs, Edge);
    vio_edges[edge_idx] = has_vio;

    // look back for GN14
    if (!has_vio && prev_node_idx != -1) {
      // check one more back
      auto prev_prev_node_idx = nodes[prev_node_idx].getPrevNodeIdx();
      if (prev_prev_node_idx != -1) {
        int prev_prev_idx_1, prev_prev_idx_2;
        getNestedIdx(prev_prev_node_idx,
                     prev_prev_idx_1,
                     prev_prev_idx_2,
                     max_access_point_size);
        if (prev_prev_idx_1 != -1) {
          const auto& [pin_3, inst_term_3] = pins[prev_prev_idx_1];
          auto pa_3 = pin_3->getPinAccess(pin_access_idx);
          std::unique_ptr<frVia> via3;
          if (pa_3->getAccessPoint(prev_prev_idx_2)->hasAccess(frDirEnum::U)) {
            via3 = std::make_unique<frVia>(
                pa_3->getAccessPoint(prev_prev_idx_2)->getViaDef());
            Point pt3(pa_3->getAccessPoint(prev_prev_idx_2)->getPoint());
            xform.apply(pt3);
            via3->setOrigin(pt3);
            if (inst_term_3->hasNet()) {
              objs.emplace_back(via3.get(), inst_term_3->getNet());
            } else {
              objs.emplace_back(via3.get(), inst_term_3);
            }
          }

          has_vio = !genPatterns_gc({target_obj}, objs, Edge);
        }
      }
    }
  }

  if (!has_vio) {
    if ((prev_idx_1 == 0
         && used_access_points.find(std::make_pair(prev_idx_1, prev_idx_2))
                != used_access_points.end())
        || (curr_idx_1 == (int) pins.size() - 1
            && used_access_points.find(std::make_pair(curr_idx_1, curr_idx_2))
                   != used_access_points.end())) {
      edge_cost = 100;
    } else if (viol_access_points.find(std::make_pair(prev_idx_1, prev_idx_2))
                   != viol_access_points.end()
               || viol_access_points.find(
                      std::make_pair(curr_idx_1, curr_idx_2))
                      != viol_access_points.end()) {
      edge_cost = 1000;
    } else if (prev_node_idx >= 0) {
      const int prev_node_cost = nodes[prev_node_idx].getNodeCost();
      const int curr_node_cost = nodes[curr_node_idx].getNodeCost();
      edge_cost = (prev_node_cost + curr_node_cost) / 2;
    }
  } else {
    edge_cost = 1000 /*violation cost*/;
  }

  return edge_cost;
}

bool FlexPA::genPatterns_commit(
    const std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    bool& is_valid,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
    const int max_access_point_size)
{
  bool has_new_pattern = false;
  int curr_node_idx = getFlatIdx(pins.size(), 0, max_access_point_size);
  auto curr_node = &(nodes[curr_node_idx]);
  int pin_cnt = pins.size();
  std::vector<int> access_pattern(pin_cnt, -1);
  while (curr_node->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      int curr_idx_1, curr_idx_2;
      getNestedIdx(
          curr_node_idx, curr_idx_1, curr_idx_2, max_access_point_size);
      access_pattern[curr_idx_1] = curr_idx_2;
      used_access_points.insert(std::make_pair(curr_idx_1, curr_idx_2));
    }

    curr_node_idx = curr_node->getPrevNodeIdx();
    curr_node = &(nodes[curr_node->getPrevNodeIdx()]);
    pin_cnt--;
  }

  if (pin_cnt != -1) {
    logger_->error(DRT, 90, "Valid access pattern not found.");
  }

  // add to pattern set if unique
  if (inst_access_patterns.find(access_pattern) == inst_access_patterns.end()) {
    inst_access_patterns.insert(access_pattern);
    // create new access pattern and push to uniqueInstances
    auto pin_access_pattern = std::make_unique<FlexPinAccessPattern>();
    std::map<frMPin*, frAccessPoint*> pin_to_access_pattern;
    // check DRC for the whole pattern
    std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
    std::vector<std::unique_ptr<frVia>> temp_vias;
    frInst* target_obj = nullptr;
    for (int idx_1 = 0; idx_1 < (int) pins.size(); idx_1++) {
      auto idx_2 = access_pattern[idx_1];
      auto& [pin, inst_term] = pins[idx_1];
      auto inst = inst_term->getInst();
      target_obj = inst;
      const int pin_access_idx = unique_insts_.getPAIndex(inst);
      const auto pa = pin->getPinAccess(pin_access_idx);
      const auto access_point = pa->getAccessPoint(idx_2);
      pin_to_access_pattern[pin] = access_point;

      // add objs
      std::unique_ptr<frVia> via;
      if (access_point->hasAccess(frDirEnum::U)) {
        via = std::make_unique<frVia>(access_point->getViaDef());
        auto rvia = via.get();
        temp_vias.push_back(std::move(via));

        dbTransform xform = inst->getUpdatedXform(true);
        Point pt(access_point->getPoint());
        xform.apply(pt);
        rvia->setOrigin(pt);
        if (inst_term->hasNet()) {
          objs.emplace_back(rvia, inst_term->getNet());
        } else {
          objs.emplace_back(rvia, inst_term);
        }
      }
    }

    frAccessPoint* leftAP = nullptr;
    frAccessPoint* rightAP = nullptr;
    frCoord leftPt = std::numeric_limits<frCoord>::max();
    frCoord rightPt = std::numeric_limits<frCoord>::min();

    const auto& [pin, inst_term] = pins[0];
    const auto inst = inst_term->getInst();
    for (auto& inst_term : inst->getInstTerms()) {
      if (isSkipInstTerm(inst_term.get())) {
        continue;
      }
      uint64_t n_no_ap_pins = 0;
      for (auto& pin : inst_term->getTerm()->getPins()) {
        if (pin_to_access_pattern.find(pin.get())
            == pin_to_access_pattern.end()) {
          n_no_ap_pins++;
          pin_access_pattern->addAccessPoint(nullptr);
        } else {
          const auto& ap = pin_to_access_pattern[pin.get()];
          const Point tmpPt = ap->getPoint();
          if (tmpPt.x() < leftPt) {
            leftAP = ap;
            leftPt = tmpPt.x();
          }
          if (tmpPt.x() > rightPt) {
            rightAP = ap;
            rightPt = tmpPt.x();
          }
          pin_access_pattern->addAccessPoint(ap);
        }
      }
      if (n_no_ap_pins == inst_term->getTerm()->getPins().size()) {
        logger_->error(DRT, 91, "Pin does not have valid ap.");
      }
    }
    pin_access_pattern->setBoundaryAP(true, leftAP);
    pin_access_pattern->setBoundaryAP(false, rightAP);

    std::set<frBlockObject*> owners;
    if (target_obj != nullptr
        && genPatterns_gc({target_obj}, objs, Commit, &owners)) {
      pin_access_pattern->updateCost();
      unique_inst_patterns_[curr_unique_inst_idx].push_back(
          std::move(pin_access_pattern));
      // genPatterns_print(nodes, pins, max_access_point_size);
      is_valid = true;
    } else {
      for (int idx_1 = 0; idx_1 < (int) pins.size(); idx_1++) {
        auto idx_2 = access_pattern[idx_1];
        auto& [pin, inst_term] = pins[idx_1];
        if (inst_term->hasNet()) {
          if (owners.find(inst_term->getNet()) != owners.end()) {
            viol_access_points.insert(std::make_pair(idx_1, idx_2));  // idx ;
          }
        } else {
          if (owners.find(inst_term) != owners.end()) {
            viol_access_points.insert(std::make_pair(idx_1, idx_2));  // idx ;
          }
        }
      }
    }

    has_new_pattern = true;
  } else {
    has_new_pattern = false;
  }

  return has_new_pattern;
}

void FlexPA::genPatternsPrintDebug(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int max_access_point_size)
{
  int curr_node_idx = getFlatIdx(pins.size(), 0, max_access_point_size);
  auto curr_node = &(nodes[curr_node_idx]);
  int pin_cnt = pins.size();

  dbTransform xform;
  auto& [pin, inst_term] = pins[0];
  if (inst_term) {
    frInst* inst = inst_term->getInst();
    xform = inst->getTransform();
    xform.setOrient(dbOrientType::R0);
  }

  std::cout << "failed pattern:";

  double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  while (curr_node->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      auto& [pin, inst_term] = pins[pin_cnt];
      auto inst = inst_term->getInst();
      std::cout << " " << inst_term->getTerm()->getName();
      const int pin_access_idx = unique_insts_.getPAIndex(inst);
      auto pa = pin->getPinAccess(pin_access_idx);
      int curr_idx_1, curr_idx_2;
      getNestedIdx(
          curr_node_idx, curr_idx_1, curr_idx_2, max_access_point_size);
      Point pt(pa->getAccessPoint(curr_idx_2)->getPoint());
      xform.apply(pt);
      std::cout << " (" << pt.x() / dbu << ", " << pt.y() / dbu << ")";
    }

    curr_node_idx = curr_node->getPrevNodeIdx();
    curr_node = &(nodes[curr_node->getPrevNodeIdx()]);
    pin_cnt--;
  }
  std::cout << std::endl;
  if (pin_cnt != -1) {
    logger_->error(DRT, 277, "Valid access pattern not found.");
  }
}

void FlexPA::genPatterns_print(
    std::vector<FlexDPNode>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    const int max_access_point_size)
{
  int curr_node_idx = getFlatIdx(pins.size(), 0, max_access_point_size);
  auto curr_node = &(nodes[curr_node_idx]);
  int pin_cnt = pins.size();

  std::cout << "new pattern\n";

  while (curr_node->getPrevNodeIdx() != -1) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      auto& [pin, inst_term] = pins[pin_cnt];
      auto inst = inst_term->getInst();
      const int pin_access_idx = unique_insts_.getPAIndex(inst);
      auto pa = pin->getPinAccess(pin_access_idx);
      int curr_idx_1, curr_idx_2;
      getNestedIdx(
          curr_node_idx, curr_idx_1, curr_idx_2, max_access_point_size);
      std::unique_ptr<frVia> via = std::make_unique<frVia>(
          pa->getAccessPoint(curr_idx_2)->getViaDef());
      Point pt(pa->getAccessPoint(curr_idx_2)->getPoint());
      std::cout << " gccleanvia " << inst->getMaster()->getName() << " "
                << inst_term->getTerm()->getName() << " "
                << via->getViaDef()->getName() << " " << pt.x() << " " << pt.y()
                << " " << inst->getOrient().getString() << "\n";
    }

    curr_node_idx = curr_node->getPrevNodeIdx();
    curr_node = &(nodes[curr_node->getPrevNodeIdx()]);
    pin_cnt--;
  }
  if (pin_cnt != -1) {
    logger_->error(DRT, 278, "Valid access pattern not found.");
  }
}

// get flat index
// idx_1 is outer index and idx_2 is inner index dpNodes[idx_1][idx_2]
int FlexPA::getFlatIdx(const int idx_1, const int idx_2, const int idx_2_dim)
{
  return ((idx_1 + 1) * idx_2_dim + idx_2);
}

// get idx_1 and idx_2 from flat index
void FlexPA::getNestedIdx(const int flat_idx,
                          int& idx_1,
                          int& idx_2,
                          const int idx_2_dim)
{
  idx_1 = flat_idx / idx_2_dim - 1;
  idx_2 = flat_idx % idx_2_dim;
}

// get flat edge index
int FlexPA::getFlatEdgeIdx(const int prev_idx_1,
                           const int prev_idx_2,
                           const int curr_idx_2,
                           const int idx_2_dim)
{
  return ((prev_idx_1 + 1) * idx_2_dim + prev_idx_2) * idx_2_dim + curr_idx_2;
}

}  // namespace drt
