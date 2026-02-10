// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/infra/frSegStyle.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/tech/frLayer.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "pa/FlexPA_unique.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using odb::dbTechLayerDir;
using utl::ThreadException;

namespace drt {

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

  // If there are less than 3 coords OnGrid will create a Centered Access
  // odb::Point
  frCoord manu_grid = getDesign()->getTech()->getManufacturingGrid();
  frCoord coord = (low + high) / 2 / manu_grid * manu_grid;

  if (coords.find(coord) == coords.end()) {
    coords.insert(std::make_pair(coord, frAccessPointEnum::Center));
  } else {
    coords[coord] = std::min(coords[coord], frAccessPointEnum::Center);
  }
}

void FlexPA::genViaEnclosedCoords(std::map<frCoord, frAccessPointEnum>& coords,
                                  const gtl::rectangle_data<frCoord>& rect,
                                  const frViaDef* via_def,
                                  const frLayerNum layer_num,
                                  const bool is_curr_layer_horz)
{
  const auto rect_width = gtl::delta(rect, gtl::HORIZONTAL);
  const auto rect_height = gtl::delta(rect, gtl::VERTICAL);
  frVia via(via_def);
  const odb::Rect box = via.getLayer1BBox();
  const auto via_width = box.dx();
  const auto via_height = box.dy();
  if (via_width > rect_width || via_height > rect_height) {
    return;
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
  if (layer_num + 1 > getDesign()->getTech()->getTopLayerNum()) {
    return;
  }
  // hardcode first two single vias
  const int max_num_via_trial = 2;
  int cnt = 0;
  for (auto& [tup, via] : layer_num_to_via_defs_[layer_num + 1][1]) {
    genViaEnclosedCoords(coords, rect, via, layer_num, is_curr_layer_horz);
    cnt++;
    if (cnt >= max_num_via_trial) {
      break;
    }
  }
}

void FlexPA::genAPCosted(
    const frAccessPointEnum cost,
    std::map<frCoord, frAccessPointEnum>& coords,
    const std::map<frCoord, frAccessPointEnum>& track_coords,
    const frLayerNum base_layer_num,
    const frLayerNum layer_num,
    const gtl::rectangle_data<frCoord>& rect,
    const int offset)
{
  auto layer = getDesign()->getTech()->getLayer(layer_num);
  const bool is_curr_layer_horz = layer->isHorizontal();
  const auto min_width_layer = layer->getMinWidth();
  const int rect_min = is_curr_layer_horz ? gtl::yl(rect) : gtl::xl(rect);
  const int rect_max = is_curr_layer_horz ? gtl::yh(rect) : gtl::xh(rect);

  switch (cost) {
    case (frAccessPointEnum::OnGrid):
      genAPOnTrack(coords, track_coords, rect_min + offset, rect_max - offset);
      break;

      // frAccessPointEnum::Halfgrid not defined

    case (frAccessPointEnum::Center):
      genAPCentered(
          coords, base_layer_num, rect_min + offset, rect_max - offset);
      break;

    case (frAccessPointEnum::EncOpt):
      genAPEnclosedBoundary(coords, rect, base_layer_num, is_curr_layer_horz);
      break;

    case (frAccessPointEnum::NearbyGrid):
      genAPOnTrack(
          coords, track_coords, rect_min - min_width_layer, rect_min, true);
      genAPOnTrack(
          coords, track_coords, rect_max, rect_max + min_width_layer, true);
      break;

    default:
      logger_->error(DRT, 257, "Invalid frAccessPointEnum type");
  }
}

// Responsible for checking if an AP is valid and configuring it
void FlexPA::createSingleAccessPoint(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<odb::Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& maxrect,
    const frCoord x,
    const frCoord y,
    const frLayerNum layer_num,
    const bool allow_planar,
    const bool allow_via,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  gtl::point_data<frCoord> pt(x, y);
  if (!gtl::contains(maxrect, pt) && lower_type != frAccessPointEnum::NearbyGrid
      && upper_type != frAccessPointEnum::NearbyGrid) {
    return;
  }
  odb::Point fpt(x, y);
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
      if (lower_layer->isUnidirectional() || !router_cfg_->USENONPREFTRACKS) {
        ap->setMultipleAccesses(frDirEnumVert, false);
      }
      if (lower_layer->getLef58RightWayOnGridOnlyConstraint()
          && lower_type != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
    }
    // vert layer
    if (lower_layer->getDir() == dbTechLayerDir::VERTICAL) {
      if (lower_layer->isUnidirectional() || !router_cfg_->USENONPREFTRACKS) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
      if (lower_layer->getLef58RightWayOnGridOnlyConstraint()
          && lower_type != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumVert, false);
      }
    }
  }

  ap->setAllowVia(allow_via);
  ap->setType((frAccessPointEnum) lower_type, true);
  ap->setType((frAccessPointEnum) upper_type, false);
  if ((lower_type == frAccessPointEnum::NearbyGrid
       || upper_type == frAccessPointEnum::NearbyGrid)) {
    odb::Point end;
    const int hwidth
        = design_->getTech()->getLayer(ap->getLayerNum())->getMinWidth() / 2;

    end.setX(std::clamp(
        fpt.x(), gtl::xl(maxrect) + hwidth, gtl::xh(maxrect) - hwidth));
    end.setY(std::clamp(
        fpt.y(), gtl::yl(maxrect) + hwidth, gtl::yh(maxrect) - hwidth));

    odb::Point e = fpt;
    if (fpt.x() != end.x()) {
      e.setX(end.x());
    } else if (fpt.y() != end.y()) {
      e.setY(end.y());
    }
    if (e != fpt) {
      frPathSeg ps;
      ps.setPoints_safe(fpt, e);
      if (ps.getBeginPoint() == end) {
        ps.setBeginStyle(frEndStyle(frcTruncateEndStyle));
      } else if (ps.getEndPoint() == end) {
        ps.setEndStyle(frEndStyle(frcTruncateEndStyle));
      }
      ap->addPathSeg(ps);
      if (e != end) {
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

void FlexPA::createMultipleAccessPoints(
    frInstTerm* inst_term,
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<odb::Point, frLayerNum>>& apset,
    const gtl::rectangle_data<frCoord>& rect,
    const frLayerNum layer_num,
    const std::map<frCoord, frAccessPointEnum>& x_coords,
    const std::map<frCoord, frAccessPointEnum>& y_coords,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  auto layer = getDesign()->getTech()->getLayer(layer_num);
  bool allow_via = true;
  bool allow_planar = true;
  //  only VIA_ACCESS_LAYERNUM layer can have via access
  if (isStdCellTerm(inst_term)) {
    if ((layer_num >= router_cfg_->VIAINPIN_BOTTOMLAYERNUM
         && layer_num <= router_cfg_->VIAINPIN_TOPLAYERNUM)
        || layer_num <= router_cfg_->VIA_ACCESS_LAYERNUM) {
      allow_planar = false;
    }
  }
  const int aps_size_before = aps.size();
  // build points;
  for (auto& [x_coord, cost_x] : x_coords) {
    for (auto& [y_coord, cost_y] : y_coords) {
      // lower full/half/center
      auto& low_layer_type = layer->isHorizontal() ? cost_y : cost_x;
      auto& up_layer_type = layer->isVertical() ? cost_y : cost_x;
      if (low_layer_type == lower_type && up_layer_type == upper_type) {
        createSingleAccessPoint(aps,
                                apset,
                                rect,
                                x_coord,
                                y_coord,
                                layer_num,
                                allow_planar,
                                allow_via,
                                lower_type,
                                upper_type);
      }
    }
  }
  const int new_aps_size = aps.size() - aps_size_before;
  if (inst_term && isStdCell(inst_term->getInst())) {
#pragma omp atomic
    std_cell_pin_gen_ap_cnt_ += new_aps_size;
  }
  if (inst_term && isMacroCell(inst_term->getInst())) {
#pragma omp atomic
    macro_cell_pin_gen_ap_cnt_ += new_aps_size;
  }
}

/**
 * @details Generates all necessary access points from a rectangle shape
 * In this case a rectangle is one of the pin shapes of the pin
 */
void FlexPA::genAPsFromRect(const gtl::rectangle_data<frCoord>& rect,
                            const frLayerNum layer_num,
                            std::map<frCoord, frAccessPointEnum>& x_coords,
                            std::map<frCoord, frAccessPointEnum>& y_coords,
                            const frAccessPointEnum lower_type,
                            const frAccessPointEnum upper_type,
                            const bool is_macro_cell_pin)
{
  if (OnlyAllowOnGridAccess(layer_num, is_macro_cell_pin)
      && upper_type != frAccessPointEnum::OnGrid) {
    return;
  }
  frLayer* layer = getDesign()->getTech()->getLayer(layer_num);
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
  auto& layer1_track_coords = track_coords_[layer_num];
  auto& layer2_track_coords = track_coords_[second_layer_num];
  const bool is_layer1_horz = layer->isHorizontal();

  int hwidth = layer->getWidth() / 2;
  bool use_center_line = false;
  if (is_macro_cell_pin && !layer->getLef58RightWayOnGridOnlyConstraint()) {
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
  /** offset used to only be used after an if (!is_macro_cell_pin ||
   * !use_center_line), so this logic was combined with offset is_macro_cell_pin
   * ? hwidth : 0;
   */
  const int offset = is_macro_cell_pin && !use_center_line ? hwidth : 0;
  const int layer1_rect_min = is_layer1_horz ? gtl::yl(rect) : gtl::xl(rect);
  const int layer1_rect_max = is_layer1_horz ? gtl::yh(rect) : gtl::xh(rect);
  auto& layer1_coords = is_layer1_horz ? y_coords : x_coords;
  auto& layer2_coords = is_layer1_horz ? x_coords : y_coords;

  const frAccessPointEnum frDirEnums[] = {frAccessPointEnum::OnGrid,
                                          frAccessPointEnum::Center,
                                          frAccessPointEnum::EncOpt,
                                          frAccessPointEnum::NearbyGrid};

  for (const auto cost : frDirEnums) {
    if (upper_type >= cost) {
      genAPCosted(cost,
                  layer2_coords,
                  layer2_track_coords,
                  layer_num,
                  second_layer_num,
                  rect,
                  offset);
    }
  }
  if (!use_center_line) {
    for (const auto cost : frDirEnums) {
      if (lower_type >= cost) {
        genAPCosted(cost,
                    layer1_coords,
                    layer1_track_coords,
                    layer_num,
                    layer_num,
                    rect);
      }
    }
  } else {
    genAPCentered(layer1_coords, layer_num, layer1_rect_min, layer1_rect_max);
    for (auto& [layer1_coord, cost] : layer1_coords) {
      layer1_coords[layer1_coord] = frAccessPointEnum::OnGrid;
    }
  }
}

bool FlexPA::OnlyAllowOnGridAccess(const frLayerNum layer_num,
                                   const bool is_macro_cell_pin)
{
  // lower layer is current layer
  // rightway on grid only forbid off track up via access on upper layer
  const auto upper_layer
      = (layer_num + 2 <= getDesign()->getTech()->getTopLayerNum())
            ? getDesign()->getTech()->getLayer(layer_num + 2)
            : nullptr;
  if (!is_macro_cell_pin && upper_layer
      && upper_layer->getLef58RightWayOnGridOnlyConstraint()) {
    return true;
  }
  return false;
}

void FlexPA::genAPsFromLayerShapes(
    LayerToRectCoordsMap& layer_rect_to_coords,
    frInstTerm* inst_term,
    const gtl::polygon_90_set_data<frCoord>& layer_shapes,
    const frLayerNum layer_num,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  // IO term is treated as the MacroCellPin as the top block
  bool is_macro_cell_pin = isMacroCellTerm(inst_term) || isIOTerm(inst_term);

  std::vector<gtl::rectangle_data<frCoord>> maxrects;
  gtl::get_max_rectangles(maxrects, layer_shapes);
  for (auto& bbox_rect : maxrects) {
    std::map<frCoord, frAccessPointEnum> x_coords;
    std::map<frCoord, frAccessPointEnum> y_coords;

    genAPsFromRect(bbox_rect,
                   layer_num,
                   x_coords,
                   y_coords,
                   lower_type,
                   upper_type,
                   is_macro_cell_pin);

    layer_rect_to_coords[layer_num].push_back(
        {bbox_rect, {x_coords, y_coords}});
  }
}

// filter off-grid coordinate
// lower on-grid 0, upper on-grid 0 = 0
// lower 1/2     1, upper on-grid 0 = 1
// lower center  2, upper on-grid 0 = 2
// lower center  2, upper center  2 = 4

void FlexPA::createAPsFromLayerToRectCoordsMap(
    const LayerToRectCoordsMap& layer_rect_to_coords,
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<odb::Point, frLayerNum>>& apset,
    frInstTerm* inst_term,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  for (const auto& [layer_num, rect_coords] : layer_rect_to_coords) {
    for (const auto& [rect, coords] : rect_coords) {
      const auto& [x_coords, y_coords] = coords;
      createMultipleAccessPoints(inst_term,
                                 aps,
                                 apset,
                                 rect,
                                 layer_num,
                                 x_coords,
                                 y_coords,
                                 lower_type,
                                 upper_type);
    }
  }
}

void FlexPA::genAPsFromPinShapes(
    LayerToRectCoordsMap& layer_rect_to_coords,
    frInstTerm* inst_term,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  //  only VIA_ACCESS_LAYERNUM layer can have via access
  frLayerNum layer_num = 0;
  for (const auto& layer_shapes : pin_shapes) {
    if (!layer_shapes.empty()
        && getDesign()->getTech()->getLayer(layer_num)->isRoutable()) {
      genAPsFromLayerShapes(layer_rect_to_coords,
                            inst_term,
                            layer_shapes,
                            layer_num,
                            lower_type,
                            upper_type);
    }
    layer_num++;
  }
}

odb::Point FlexPA::genEndPoint(
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    const odb::Point& begin_point,
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
  return {x, y};
}

bool FlexPA::isPointOutsideShapes(
    const odb::Point& point,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys)
{
  const gtl::point_data<frCoord> pt(point.getX(), point.getY());
  for (auto& layer_poly : layer_polys) {
    if (gtl::contains(layer_poly, pt)) {
      return false;
      break;
    }
  }
  return true;
}

template <typename T>
bool FlexPA::filterPlanarAccess(
    frAccessPoint* ap,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    frDirEnum dir,
    T* pin,
    frInstTerm* inst_term)
{
  const odb::Point begin_point = ap->getPoint();
  // skip viaonly access
  if (!ap->hasAccess(dir)) {
    return false;
  }
  const bool is_block
      = inst_term
        && inst_term->getInst()->getMaster()->getMasterType().isBlock();
  const odb::Point end_point
      = genEndPoint(layer_polys, begin_point, ap->getLayerNum(), dir, is_block);
  const bool is_outside = isPointOutsideShapes(end_point, layer_polys);
  // skip if two width within shape for standard cell
  if (!is_outside) {
    ap->setAccess(dir, false);
    return false;
  }
  // TODO: EDIT HERE Wrongdirection segments
  frLayer* layer = getDesign()->getTech()->getLayer(ap->getLayerNum());
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

  const bool no_drv
      = isPlanarViolationFree(ap, pin, ps.get(), inst_term, begin_point, layer);
  ap->setAccess(dir, no_drv);

  return no_drv;
}

template <typename T>
bool FlexPA::isPlanarViolationFree(frAccessPoint* ap,
                                   T* pin,
                                   frPathSeg* ps,
                                   frInstTerm* inst_term,
                                   const odb::Point point,
                                   frLayer* layer)
{
  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_, router_cfg_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = layer->getPitch();
  const auto extension = 5 * pitch;
  odb::Rect tmp_box(point, point);
  odb::Rect ext_box;
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
  design_rule_checker.addPAObj(ps, owner);
  for (auto& apPs : ap->getPathSegs()) {
    design_rule_checker.addPAObj(&apPs, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();

  if (graphics_) {
    graphics_->setPlanarAP(ap, ps, design_rule_checker.getMarkers());
  }

  return design_rule_checker.getMarkers().empty();
}

void FlexPA::getViasFromMetalWidthMap(
    const odb::Point& pt,
    const frLayerNum layer_num,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    std::vector<std::pair<int, const frViaDef*>>& via_defs)
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

frCoord FlexPA::viaMaxExt(frInstTerm* inst_term,
                          frAccessPoint* ap,
                          const gtl::polygon_90_set_data<frCoord>& polyset,
                          const frViaDef* via_def)
{
  const odb::Point begin_point = ap->getPoint();
  const auto layer_num = ap->getLayerNum();
  auto via = std::make_unique<frVia>(via_def);
  via->setOrigin(begin_point);
  const odb::Rect box = via->getLayer1BBox();

  // check if ap is on the left/right boundary of the cell
  odb::Rect boundary_bbox;
  bool is_side_bound = false;
  if (inst_term) {
    boundary_bbox = inst_term->getInst()->getBoundaryBBox();
    frCoord width = getDesign()->getTech()->getLayer(layer_num)->getWidth();
    if (begin_point.x() <= boundary_bbox.xMin() + 3 * width
        || begin_point.x() >= boundary_bbox.xMax() - 3 * width) {
      is_side_bound = true;
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
  intersection.get_rectangles(int_rects, gtl::orientation_2d_enum::HORIZONTAL);
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
  return max_ext;
}

template <typename T>
void FlexPA::filterViaAccess(
    frAccessPoint* ap,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    T* pin,
    frInstTerm* inst_term,
    bool deep_search)
{
  const odb::Point begin_point = ap->getPoint();
  const auto layer_num = ap->getLayerNum();

  // skip planar only access
  if (!ap->isViaAllowed()) {
    return;
  }

  bool via_in_pin = false;
  const auto lower_type = ap->getType(true);
  const auto upper_type = ap->getType(false);
  if (layer_num >= router_cfg_->VIAINPIN_BOTTOMLAYERNUM
      && layer_num <= router_cfg_->VIAINPIN_TOPLAYERNUM) {
    via_in_pin = true;
  } else if ((lower_type == frAccessPointEnum::EncOpt
              && upper_type != frAccessPointEnum::NearbyGrid)
             || (upper_type == frAccessPointEnum::EncOpt
                 && lower_type != frAccessPointEnum::NearbyGrid)) {
    via_in_pin = true;
  }

  const int max_num_via_trial = 2;
  // use std:pair to ensure deterministic behavior
  std::vector<std::pair<int, const frViaDef*>> via_defs;
  getViasFromMetalWidthMap(begin_point, layer_num, polyset, via_defs);

  if (via_defs.empty()) {  // no via map entry
    // hardcode first two single vias
    auto collect_vias = [&](int adj_layer_num, int max_trial) {
      if (adj_layer_num > router_cfg_->TOP_ROUTING_LAYER) {
        return;
      }
      if (layer_num_to_via_defs_.find(adj_layer_num)
          != layer_num_to_via_defs_.end()) {
        for (auto& [tup, via_def] : layer_num_to_via_defs_[adj_layer_num][1]) {
          if (inst_term && inst_term->isStubborn()
              && avoid_via_defs_.contains(via_def)) {
            continue;
          }
          via_defs.emplace_back(via_defs.size(), via_def);
          if (via_defs.size() >= max_trial && !deep_search) {
            break;
          }
        }
      }
    };

    // UP Vias
    collect_vias(layer_num + 1, max_num_via_trial);

    // DOWN Vias
    if (isIOTerm(inst_term)) {
      collect_vias(layer_num - 1, max_num_via_trial);
    }
  }

  int valid_via_count = 0;
  for (auto& [idx, via_def] : via_defs) {
    auto via = std::make_unique<frVia>(via_def, begin_point);
    const odb::Rect box = via->getLayer1BBox();
    if (inst_term && !deep_search) {
      odb::Rect boundary_bbox = inst_term->getInst()->getBoundaryBBox();
      if (!boundary_bbox.contains(box)) {
        continue;
      }
      odb::Rect layer2_boundary_box = via->getLayer2BBox();
      if (!boundary_bbox.contains(layer2_boundary_box)) {
        continue;
      }
    }

    frCoord max_ext = viaMaxExt(inst_term, ap, polyset, via_def);

    if (via_in_pin && max_ext) {
      continue;
    }
    if (checkViaPlanarAccess(ap, via.get(), pin, inst_term, layer_polys)) {
      ap->addViaDef(via_def);
      if (via_def->getLayer1Num() == layer_num) {
        ap->setAccess(frDirEnum::U);
      } else {
        ap->setAccess(frDirEnum::D);
      }
      valid_via_count++;
      if (valid_via_count >= max_num_via_trial) {
        break;
      }
    }
  }
}

template <typename T>
bool FlexPA::validateAPForPlanarAccess(
    frAccessPoint* ap,
    const std::vector<std::vector<gtl::polygon_90_data<frCoord>>>& layer_polys,
    T* pin,
    frInstTerm* inst_term)
{
  bool allow_planar_access = false;
  for (const frDirEnum dir : frDirEnumPlanar) {
    allow_planar_access |= filterPlanarAccess(
        ap, layer_polys[ap->getLayerNum()], dir, pin, inst_term);
  }
  return allow_planar_access;
}

template <typename T>
bool FlexPA::checkViaPlanarAccess(
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
  frLayerNum target_layer_num;
  if (via->getViaDef()->getLayer1Num() == ap->getLayerNum()) {
    target_layer_num = via->getViaDef()->getLayer2Num();
  } else {
    target_layer_num = via->getViaDef()->getLayer1Num();
  }
  auto target_layer = getTech()->getLayer(target_layer_num);
  const bool vert_dir = (dir == frDirEnum::S || dir == frDirEnum::N);
  const bool wrong_dir = (target_layer->isHorizontal() && vert_dir)
                         || (target_layer->isVertical() && !vert_dir);
  auto style = target_layer->getDefaultSegStyle();

  if (wrong_dir) {
    if (!router_cfg_->USENONPREFTRACKS || target_layer->isUnidirectional()) {
      return false;
    }
    style.setWidth(target_layer->getWrongDirWidth());
  }

  const odb::Point begin_point = ap->getPoint();
  const bool is_block
      = inst_term
        && inst_term->getInst()->getMaster()->getMasterType().isBlock();
  const odb::Point end_point
      = genEndPoint(layer_polys, begin_point, target_layer_num, dir, is_block);

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
  ps->setLayerNum(target_layer->getLayerNum());
  ps->setStyle(style);
  if (inst_term && inst_term->hasNet()) {
    ps->addToNet(inst_term->getNet());
  } else {
    ps->addToPin(pin);
  }
  return isViaViolationFree(ap, via, pin, ps.get(), inst_term, begin_point);
}

template <typename T>
bool FlexPA::isViaViolationFree(frAccessPoint* ap,
                                frVia* via,
                                T* pin,
                                frPathSeg* ps,
                                frInstTerm* inst_term,
                                const odb::Point point)
{
  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_, router_cfg_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreLongSideEOL();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = getTech()->getLayer(ap->getLayerNum())->getPitch();
  const auto extension = 5 * pitch;
  odb::Rect tmp_box(point, point);
  odb::Rect ext_box;
  tmp_box.bloat(extension, ext_box);
  auto pin_term = pin->getTerm();
  auto pin_net = pin_term->getNet();
  design_rule_checker.setExtBox(ext_box);
  design_rule_checker.setDrcBox(ext_box);
  if (inst_term) {
    if (!inst_term->getNet() || !inst_term->getNet()->getNondefaultRule()
        || router_cfg_->AUTO_TAPER_NDR_NETS) {
      design_rule_checker.addTargetObj(inst_term->getInst());
    }
  } else {
    if (!pin_net || !pin_net->getNondefaultRule()
        || router_cfg_->AUTO_TAPER_NDR_NETS) {
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
  design_rule_checker.addPAObj(ps, owner);
  design_rule_checker.addPAObj(via, owner);
  for (auto& apPs : ap->getPathSegs()) {
    design_rule_checker.addPAObj(&apPs, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();

  const bool no_drv = design_rule_checker.getMarkers().empty();

  if (graphics_) {
    graphics_->setViaAP(ap, via, design_rule_checker.getMarkers());
  }
  return no_drv;
}

template <typename T>
void FlexPA::filterMultipleAPAccesses(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const bool& is_std_cell_pin)
{
  if (isIOTerm(inst_term)) {
    // if the pin is an I/O pin, and there is planar access, return
    for (auto& ap : aps) {
      if (ap->hasPlanarAccess()) {
        return;
      }
    }
  }
  std::vector<std::vector<gtl::polygon_90_data<frCoord>>> layer_polys(
      pin_shapes.size());
  for (int i = 0; i < (int) pin_shapes.size(); i++) {
    pin_shapes[i].get_polygons(layer_polys[i]);
  }
  bool has_access = false;
  for (auto& ap : aps) {
    const auto layer_num = ap->getLayerNum();
    filterViaAccess(ap.get(),
                    layer_polys[layer_num],
                    pin_shapes[layer_num],
                    pin,
                    inst_term);
    if (is_std_cell_pin) {
      has_access |= ((layer_num <= router_cfg_->VIA_ACCESS_LAYERNUM
                      && ap->hasAccess(frDirEnum::U))
                     || (layer_num > router_cfg_->VIA_ACCESS_LAYERNUM
                         && ap->hasAccess()));
    } else {
      has_access |= ap->hasAccess();
    }
  }
  if (!has_access) {
    for (auto& ap : aps) {
      const auto layer_num = ap->getLayerNum();
      filterViaAccess(ap.get(),
                      layer_polys[layer_num],
                      pin_shapes[layer_num],
                      pin,
                      inst_term,
                      true);
    }
  }
}

void FlexPA::updatePinStats(
    const std::vector<std::unique_ptr<frAccessPoint>>& new_aps,
    frInstTerm* inst_term)
{
  if (isIOTerm(inst_term)) {
    return;
  }

  bool is_std_cell_pin = isStdCellTerm(inst_term);
  bool is_macro_cell_pin = isMacroCellTerm(inst_term);

  if (new_aps.empty()) {
    if (is_std_cell_pin) {
      std_cell_pin_no_ap_cnt_++;
    } else if (is_macro_cell_pin) {
      macro_cell_pin_no_ap_cnt_++;
    }
  }

  for (auto& ap : new_aps) {
    if (ap->hasPlanarAccess()) {
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

bool FlexPA::EnoughSparsePoints(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    frInstTerm* inst_term)
{
  const bool is_std_cell_pin = isStdCellTerm(inst_term);
  const bool is_macro_cell_pin = isMacroCellTerm(inst_term);
  /* This is a Max Clique problem, each ap is a node, draw an edge between two
   aps if they are far away as to not intersect. n_sparse_access_points,
   ideally, is the Max Clique of this graph. the current implementation gives a
   very rough approximation, it works, but I think it can be improved.
   */
  int n_sparse_access_points = (int) aps.size();
  for (int i = 0; i < (int) aps.size(); i++) {
    const int colision_dist
        = design_->getTech()->getLayer(aps[i]->getLayerNum())->getWidth() / 2;
    odb::Rect ap_colision_box;
    odb::Rect(aps[i]->getPoint(), aps[i]->getPoint())
        .bloat(colision_dist, ap_colision_box);
    for (int j = i + 1; j < (int) aps.size(); j++) {
      if (aps[i]->getLayerNum() == aps[j]->getLayerNum()
          && ap_colision_box.intersects(aps[j]->getPoint())) {
        n_sparse_access_points--;
        break;
      }
    }
  }

  if (is_std_cell_pin
      && n_sparse_access_points >= router_cfg_->MINNUMACCESSPOINT_STDCELLPIN) {
    return true;
  }
  if (is_macro_cell_pin
      && n_sparse_access_points
             >= router_cfg_->MINNUMACCESSPOINT_MACROCELLPIN) {
    return true;
  }
  return false;
}

bool FlexPA::EnoughPointsFarFromEdge(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    frInstTerm* inst_term)
{
  const int far_from_edge_requirement = 1;
  odb::Rect cell_box = inst_term->getInst()->getBBox();
  int total_far_from_edge = 0;
  for (auto& ap : aps) {
    const int colision_dist
        = design_->getTech()->getLayer(ap->getLayerNum())->getWidth() * 2;
    odb::Rect ap_colision_box;
    odb::Rect(ap->getPoint(), ap->getPoint())
        .bloat(colision_dist, ap_colision_box);
    if (cell_box.contains(ap_colision_box)) {
      total_far_from_edge++;
      if (total_far_from_edge >= far_from_edge_requirement) {
        return true;
      }
    }
  }
  return false;
}

bool FlexPA::EnoughAccessPoints(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    frInstTerm* inst_term,
    pa_requirements_met& reqs)
{
  if (isIOTerm(inst_term)) {
    return !aps.empty();
  }

  reqs.sparse_points = EnoughSparsePoints(aps, inst_term);

  return (reqs.sparse_points);
}

template <typename T>
bool FlexPA::genPinAccessCostBounded(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<odb::Point, frLayerNum>>& apset,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    const std::vector<std::vector<gtl::polygon_90_data<frCoord>>>& layer_polys,
    T* pin,
    frInstTerm* inst_term,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type,
    pa_requirements_met& reqs)
{
  const bool is_std_cell_pin = isStdCellTerm(inst_term);
  std::vector<std::unique_ptr<frAccessPoint>> new_aps;
  LayerToRectCoordsMap layer_rect_to_coords;
  genAPsFromPinShapes(
      layer_rect_to_coords, inst_term, pin_shapes, lower_type, upper_type);
  createAPsFromLayerToRectCoordsMap(
      layer_rect_to_coords, new_aps, apset, inst_term, lower_type, upper_type);
  for (auto& ap : new_aps) {
    validateAPForPlanarAccess(ap.get(), layer_polys, pin, inst_term);
  }
  filterMultipleAPAccesses(
      new_aps, pin_shapes, pin, inst_term, is_std_cell_pin);
  if (graphics_) {
    graphics_->setAPs(new_aps, lower_type, upper_type);
  }
  for (auto& ap : new_aps) {
    if (!ap->hasAccess()) {
      continue;
    }
    // for stdcell, add (i) planar access if layer_num != VIA_ACCESS_LAYERNUM,
    // and (ii) access if exist access for macro, allow pure planar ap
    if (is_std_cell_pin) {
      if (ap->getLayerNum() <= router_cfg_->VIA_ACCESS_LAYERNUM
          && !ap->hasAccess(frDirEnum::U)) {
        continue;
      }
    }
    aps.push_back(std::move(ap));
  }

  return EnoughAccessPoints(aps, inst_term, reqs);
}

template <typename T>
std::vector<gtl::polygon_90_set_data<frCoord>>
FlexPA::mergePinShapes(T* pin, frInstTerm* inst_term, const bool is_shrink)
{
  frInst* inst = nullptr;
  if (inst_term) {
    inst = inst_term->getInst();
  }

  odb::dbTransform xform;
  if (inst) {
    xform = inst->getDBTransform();
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
      if (!layer->isRoutable()) {
        continue;
      }
      odb::Rect box = obj->getBBox();
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
      for (odb::Point pt : obj->getPoints()) {
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

// first create all access points with costs
template <typename T>
int FlexPA::genPinAccess(T* pin, frInstTerm* inst_term)
{
  // IO term pin always only have one access
  const int pin_access_idx
      = inst_term ? inst_term->getInst()->getPinAccessIdx() : 0;
  if (pin->getPinAccess(pin_access_idx)->getNumAccessPoints() > 0) {
    pin->getPinAccess(pin_access_idx)->clearAccessPoints();
  }

  // aps are after xform
  // before checkPoints, ap->hasAccess(dir) indicates whether to check drc
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  std::set<std::pair<odb::Point, frLayerNum>> apset;

  if (graphics_) {
    UniqueClass* inst_class = nullptr;
    if (inst_term) {
      inst_class = unique_insts_.getUniqueClass(inst_term->getInst());
    }
    graphics_->startPin(pin, inst_term, inst_class);
  }

  std::vector<gtl::polygon_90_set_data<frCoord>> pin_shapes
      = mergePinShapes(pin, inst_term);

  std::vector<std::vector<gtl::polygon_90_data<frCoord>>> layer_polys(
      pin_shapes.size());
  for (int i = 0; i < (int) pin_shapes.size(); i++) {
    pin_shapes[i].get_polygons(layer_polys[i]);
  }

  pa_requirements_met reqs;

  bool enough_access_points = false;

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

      if (enough_access_points) {
        break;
      }

      enough_access_points = genPinAccessCostBounded(aps,
                                                     apset,
                                                     pin_shapes,
                                                     layer_polys,
                                                     pin,
                                                     inst_term,
                                                     lower,
                                                     upper,
                                                     reqs);
    }
  }

  if (!enough_access_points && inst_term) {
    std::string unmet_requirements;
    if (!reqs.sparse_points) {
      unmet_requirements
          += "\n\tAt least "
             + (isStdCellTerm(inst_term)
                    ? std::to_string(router_cfg_->MINNUMACCESSPOINT_STDCELLPIN)
                    : std::to_string(
                          router_cfg_->MINNUMACCESSPOINT_MACROCELLPIN))
             + " sparse access points";
    }
    debugPrint(logger_,
               utl::DRT,
               "pin_access",
               1,
               "Access point generation for {} did not meet :{}",
               inst_term->getName(),
               unmet_requirements);
  }

  // Sorts via_defs in each ap
  for (auto& ap : aps) {
    std::map<const frViaDef*, int> cost_map;
    for (const auto& viaDefsLayer : ap->getAllViaDefs()) {
      for (const frViaDef* via_def : viaDefsLayer) {
        cost_map[via_def] = viaMaxExt(
            inst_term, ap.get(), pin_shapes[ap->getLayerNum()], via_def);
      }
    }

    ap->sortViaDefs(cost_map);
  }

  updatePinStats(aps, inst_term);
  // write to pa
  for (auto& ap : aps) {
    pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
  }
  return aps.size();
}

void FlexPA::genInstAccessPoints(frInst* unique_inst)
{
  ProfileTask profile("PA:uniqueInstance");
  for (auto& inst_term : unique_inst->getInstTerms()) {
    // only do for normal and clock terms
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }
    int n_aps = 0;
    for (auto& pin : inst_term->getTerm()->getPins()) {
      n_aps += genPinAccess(pin.get(), inst_term.get());
    }
    if (!n_aps) {
      logger_->error(DRT,
                     73,
                     "No access point for {}/{} ({}).",
                     inst_term->getInst()->getName(),
                     inst_term->getTerm()->getName(),
                     inst_term->getInst()->getMaster()->getName());
    }
  }
}

void FlexPA::genAllAccessPoints()
{
  ProfileTask profile("PA:point");
  int pin_count = 0;

  omp_set_num_threads(router_cfg_->MAX_THREADS);
  ThreadException exception;

  const auto& unique = unique_insts_.getUniqueClasses();
#pragma omp parallel for schedule(dynamic)
  for (const auto& unique_class : unique) {  // NOLINT
    try {
      auto candidate_inst = unique_class->getFirstInst();
      // only do for core and block cells
      if (!isStdCell(candidate_inst) && !isMacroCell(candidate_inst)) {
        continue;
      }

      genInstAccessPoints(candidate_inst);
      if (router_cfg_->VERBOSE <= 0) {
        continue;
      }

      int inst_terms_cnt
          = static_cast<int>(candidate_inst->getInstTerms().size());
#pragma omp critical
      for (int j = 0; j < inst_terms_cnt; j++) {
        pin_count++;
        if (pin_count % (pin_count > 10000 ? 10000 : 1000) == 0) {
          logger_->info(DRT, 76, "  Complete {} pins.", pin_count);
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();

  // PA for IO terms
  if (target_insts_.empty()) {
    omp_set_num_threads(router_cfg_->MAX_THREADS);
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
          n_aps += genPinAccess(pin.get(), nullptr);
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

  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 78, "  Complete {} pins.", pin_count);
  }
}
void FlexPA::revertAccessPoints(frInst* inst)
{
  const odb::dbTransform xform = inst->getTransform();
  const odb::Point offset(xform.getOffset());
  odb::dbTransform revertXform(odb::Point(-offset.getX(), -offset.getY()));

  const auto pin_access_idx = inst->getPinAccessIdx();
  for (auto& inst_term : inst->getInstTerms()) {
    for (auto& pin : inst_term->getTerm()->getPins()) {
      auto pin_access = pin->getPinAccess(pin_access_idx);
      for (auto& access_point : pin_access->getAccessPoints()) {
        odb::Point unique_AP_point(access_point->getPoint());
        revertXform.apply(unique_AP_point);
        access_point->setPoint(unique_AP_point);
        for (auto& ps : access_point->getPathSegs()) {
          odb::Point begin = ps.getBeginPoint();
          odb::Point end = ps.getEndPoint();
          revertXform.apply(begin);
          revertXform.apply(end);
          if (end < begin) {
            odb::Point tmp = begin;
            begin = end;
            end = tmp;
          }
          ps.setPoints(begin, end);
        }
      }
    }
  }
}

void FlexPA::revertAccessPoints()
{
  const auto& unique = unique_insts_.getUniqueClasses();
  for (const auto& unique_class : unique) {
    if (unique_class->getInsts().empty()) {
      continue;
    }
    auto inst = unique_class->getFirstInst();
    revertAccessPoints(inst);
  }
}

}  // namespace drt
