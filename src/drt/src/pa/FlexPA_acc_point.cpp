// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <omp.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "AbstractPAGraphics.h"
#include "FlexPA.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

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

void FlexPA::genViaEnclosedCoords(std::map<frCoord, frAccessPointEnum>& coords,
                                  const gtl::rectangle_data<frCoord>& rect,
                                  const frViaDef* via_def,
                                  const frLayerNum layer_num,
                                  const bool is_curr_layer_horz)
{
  const auto rect_width = gtl::delta(rect, gtl::HORIZONTAL);
  const auto rect_height = gtl::delta(rect, gtl::VERTICAL);
  frVia via(via_def);
  const Rect box = via.getLayer1BBox();
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
    const bool is_curr_layer_horz,
    const int offset)
{
  auto layer = getDesign()->getTech()->getLayer(layer_num);
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
    std::set<std::pair<Point, frLayerNum>>& apset,
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
          && lower_type != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
    }
    // vert layer
    if (lower_layer->getDir() == dbTechLayerDir::VERTICAL) {
      if (lower_layer->isUnidirectional()) {
        ap->setMultipleAccesses(frDirEnumHorz, false);
      }
      if (lower_layer->getLef58RightWayOnGridOnlyConstraint()
          && lower_type != frAccessPointEnum::OnGrid) {
        ap->setMultipleAccesses(frDirEnumVert, false);
      }
    }
  }
  ap->setAccess(frDirEnum::D, false);
  ap->setAccess(frDirEnum::U, allow_via);

  ap->setAllowVia(allow_via);
  ap->setType((frAccessPointEnum) lower_type, true);
  ap->setType((frAccessPointEnum) upper_type, false);
  if ((lower_type == frAccessPointEnum::NearbyGrid
       || upper_type == frAccessPointEnum::NearbyGrid)) {
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

void FlexPA::createMultipleAccessPoints(
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
      auto& low_layer_type = is_layer1_horz ? cost_y : cost_x;
      auto& up_layer_type = (!is_layer1_horz) ? cost_y : cost_x;
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
  auto& layer1_track_coords = track_coords_[layer_num];
  auto& layer2_track_coords = track_coords_[second_layer_num];
  const bool is_layer1_horz = (layer->getDir() == dbTechLayerDir::HORIZONTAL);

  std::map<frCoord, frAccessPointEnum> x_coords;
  std::map<frCoord, frAccessPointEnum> y_coords;
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
                  !is_layer1_horz,
                  offset);
    }
  }
  if (!(is_macro_cell_pin && use_center_line)) {
    for (const auto cost : frDirEnums) {
      if (lower_type >= cost) {
        genAPCosted(cost,
                    layer1_coords,
                    layer1_track_coords,
                    layer_num,
                    layer_num,
                    rect,
                    is_layer1_horz);
      }
    }
  } else {
    genAPCentered(layer1_coords, layer_num, layer1_rect_min, layer1_rect_max);
    for (auto& [layer1_coord, cost] : layer1_coords) {
      layer1_coords[layer1_coord] = frAccessPointEnum::OnGrid;
    }
  }

  if (is_macro_cell_pin && use_center_line && is_layer1_horz) {
    lower_type = frAccessPointEnum::OnGrid;
  }

  createMultipleAccessPoints(aps,
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
    if (isStdCell(inst_term->getInst())) {
      if ((layer_num >= router_cfg_->VIAINPIN_BOTTOMLAYERNUM
           && layer_num <= router_cfg_->VIAINPIN_TOPLAYERNUM)
          || layer_num <= router_cfg_->VIA_ACCESS_LAYERNUM) {
        allow_planar = false;
      }
    }
    is_macro_cell_pin = isMacroCell(inst_term->getInst());
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

Point FlexPA::genEndPoint(
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
  return {x, y};
}

bool FlexPA::isPointOutsideShapes(
    const Point& point,
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
void FlexPA::filterPlanarAccess(
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
  const Point end_point
      = genEndPoint(layer_polys, begin_point, ap->getLayerNum(), dir, is_block);
  const bool is_outside = isPointOutsideShapes(end_point, layer_polys);
  // skip if two width within shape for standard cell
  if (!is_outside) {
    ap->setAccess(dir, false);
    return;
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
}

template <typename T>
bool FlexPA::isPlanarViolationFree(frAccessPoint* ap,
                                   T* pin,
                                   frPathSeg* ps,
                                   frInstTerm* inst_term,
                                   const Point point,
                                   frLayer* layer)
{
  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_, router_cfg_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = layer->getPitch();
  const auto extension = 5 * pitch;
  Rect tmp_box(point, point);
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
    const Point& pt,
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

template <typename T>
void FlexPA::filterViaAccess(
    frAccessPoint* ap,
    const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
    const gtl::polygon_90_set_data<frCoord>& polyset,
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
  if (layer_num >= router_cfg_->VIAINPIN_BOTTOMLAYERNUM
      && layer_num <= router_cfg_->VIAINPIN_TOPLAYERNUM) {
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
  std::vector<std::pair<int, const frViaDef*>> via_defs;
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

  std::set<std::tuple<frCoord, int, const frViaDef*>> valid_via_defs;
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
    if (checkViaPlanarAccess(ap, via.get(), pin, inst_term, layer_polys)) {
      valid_via_defs.insert({max_ext, idx, via_def});
      if (valid_via_defs.size() >= max_num_via_trial) {
        break;
      }
    }
  }
  ap->setAccess(frDirEnum::U, !valid_via_defs.empty());
  for (auto& [ext, idx, via_def] : valid_via_defs) {
    ap->addViaDef(via_def);
  }
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
  auto upper_layer = getTech()->getLayer(via->getViaDef()->getLayer2Num());
  const bool vert_dir = (dir == frDirEnum::S || dir == frDirEnum::N);
  const bool wrong_dir = (upper_layer->isHorizontal() && vert_dir)
                         || (upper_layer->isVertical() && !vert_dir);
  auto style = upper_layer->getDefaultSegStyle();

  if (wrong_dir) {
    if (!router_cfg_->USENONPREFTRACKS || upper_layer->isUnidirectional()) {
      return false;
    }
    style.setWidth(upper_layer->getWrongDirWidth());
  }

  const Point begin_point = ap->getPoint();
  const bool is_block
      = inst_term
        && inst_term->getInst()->getMaster()->getMasterType().isBlock();
  const Point end_point = genEndPoint(layer_polys,
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
  return isViaViolationFree(ap, via, pin, ps.get(), inst_term, begin_point);
}

template <typename T>
bool FlexPA::isViaViolationFree(frAccessPoint* ap,
                                frVia* via,
                                T* pin,
                                frPathSeg* ps,
                                frInstTerm* inst_term,
                                const Point point)
{
  // Runs the DRC Engine to check for any violations
  FlexGCWorker design_rule_checker(getTech(), logger_, router_cfg_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreLongSideEOL();
  design_rule_checker.setIgnoreCornerSpacing();
  const auto pitch = getTech()->getLayer(ap->getLayerNum())->getPitch();
  const auto extension = 5 * pitch;
  Rect tmp_box(point, point);
  Rect ext_box;
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
void FlexPA::filterSingleAPAccesses(
    frAccessPoint* ap,
    const gtl::polygon_90_set_data<frCoord>& polyset,
    const std::vector<gtl::polygon_90_data<frCoord>>& polys,
    T* pin,
    frInstTerm* inst_term,
    bool deep_search)
{
  if (!deep_search) {
    for (const frDirEnum dir : frDirEnumPlanar) {
      filterPlanarAccess(ap, polys, dir, pin, inst_term);
    }
  }
  filterViaAccess(ap, polys, polyset, pin, inst_term, deep_search);
}

template <typename T>
void FlexPA::filterMultipleAPAccesses(
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
    filterSingleAPAccesses(ap.get(),
                           pin_shapes[layer_num],
                           layer_polys[layer_num],
                           pin,
                           inst_term);
    if (is_std_cell_pin) {
      has_access |= ((layer_num == router_cfg_->VIA_ACCESS_LAYERNUM
                      && ap->hasAccess(frDirEnum::U))
                     || (layer_num != router_cfg_->VIA_ACCESS_LAYERNUM
                         && ap->hasAccess()));
    } else {
      has_access |= ap->hasAccess();
    }
  }
  if (!has_access) {
    for (auto& ap : aps) {
      const auto layer_num = ap->getLayerNum();
      filterSingleAPAccesses(ap.get(),
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
    const std::vector<std::unique_ptr<frAccessPoint>>& new_aps,
    T* pin,
    frInstTerm* inst_term)
{
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    is_std_cell_pin = isStdCell(inst_term->getInst());
    is_macro_cell_pin = isMacroCell(inst_term->getInst());
  }
  for (auto& ap : new_aps) {
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

bool FlexPA::EnoughAccessPoints(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    frInstTerm* inst_term)
{
  const bool is_std_cell_pin = inst_term && isStdCell(inst_term->getInst());
  const bool is_macro_cell_pin = inst_term && isMacroCell(inst_term->getInst());
  const bool is_io_pin = (inst_term == nullptr);
  bool enough_sparse_acc_points = false;
  bool enough_far_from_edge_points = false;

  if (is_io_pin) {
    return (aps.size() > 0);
  }

  /* This is a Max Clique problem, each ap is a node, draw an edge between two
   aps if they are far away as to not intersect. n_sparse_access_points,
   ideally, is the Max Clique of this graph. the current implementation gives a
   very rough approximation, it works, but I think it can be improved.
   */
  int n_sparse_access_points = (int) aps.size();
  for (int i = 0; i < (int) aps.size(); i++) {
    const int colision_dist
        = design_->getTech()->getLayer(aps[i]->getLayerNum())->getWidth() / 2;
    Rect ap_colision_box;
    Rect(aps[i]->getPoint(), aps[i]->getPoint())
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
    enough_sparse_acc_points = true;
  }
  if (is_macro_cell_pin
      && n_sparse_access_points
             >= router_cfg_->MINNUMACCESSPOINT_MACROCELLPIN) {
    enough_sparse_acc_points = true;
  }

  Rect cell_box = inst_term->getInst()->getBBox();
  for (auto& ap : aps) {
    const int colision_dist
        = design_->getTech()->getLayer(ap->getLayerNum())->getWidth() * 2;
    Rect ap_colision_box;
    Rect(ap->getPoint(), ap->getPoint()).bloat(colision_dist, ap_colision_box);
    if (cell_box.contains(ap_colision_box)) {
      enough_far_from_edge_points = true;
      break;
    }
  }

  return (enough_sparse_acc_points && enough_far_from_edge_points);
}

template <typename T>
bool FlexPA::genPinAccessCostBounded(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  const bool is_std_cell_pin = inst_term && isStdCell(inst_term->getInst());
  ;
  const bool is_macro_cell_pin = inst_term && isMacroCell(inst_term->getInst());
  const bool is_io_pin = (inst_term == nullptr);
  std::vector<std::unique_ptr<frAccessPoint>> new_aps;
  genAPsFromPinShapes(
      new_aps, apset, pin, inst_term, pin_shapes, lower_type, upper_type);
  filterMultipleAPAccesses(
      new_aps, pin_shapes, pin, inst_term, is_std_cell_pin);
  if (is_std_cell_pin) {
#pragma omp atomic
    std_cell_pin_gen_ap_cnt_ += new_aps.size();
  }
  if (is_macro_cell_pin) {
#pragma omp atomic
    macro_cell_pin_gen_ap_cnt_ += new_aps.size();
  }
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
      const bool ap_in_via_acc_layer
          = (ap->getLayerNum() == router_cfg_->VIA_ACCESS_LAYERNUM);
      if (!ap_in_via_acc_layer || ap->hasAccess(frDirEnum::U)) {
        aps.push_back(std::move(ap));
      }
    } else if (is_macro_cell_pin || is_io_pin) {
      aps.push_back(std::move(ap));
    }
  }

  if (!EnoughAccessPoints(aps, inst_term)) {
    return false;
  }

  if (is_std_cell_pin || is_macro_cell_pin) {
    updatePinStats(aps, pin, inst_term);
    // write to pa
    const int pin_access_idx = inst_term->getInst()->getPinAccessIdx();
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
    return true;
  }

  if (is_io_pin) {
    // IO term pin always only have one access
    for (auto& ap : aps) {
      pin->getPinAccess(0)->addAccessPoint(std::move(ap));
    }
    return true;
  }

  // weird edge case where pin is not from std_cell, macro or io, not sure it
  // can even happen
  return false;
}

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

// first create all access points with costs
template <typename T>
int FlexPA::genPinAccess(T* pin, frInstTerm* inst_term)
{
  // aps are after xform
  // before checkPoints, ap->hasAccess(dir) indicates whether to check drc
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  std::set<std::pair<Point, frLayerNum>> apset;

  if (graphics_) {
    frOrderedIdSet<frInst*>* inst_class = nullptr;
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
      if (genPinAccessCostBounded(
              aps, apset, pin_shapes, pin, inst_term, lower, upper)) {
        return aps.size();
      }
    }
  }

  if (inst_term) {
    logger_->warn(
        DRT,
        88,
        "Exhaustive access point generation for {} ({}) is unsatisfactory.",
        inst_term->getName(),
        inst_term->getInst()->getMaster()->getName());
  }

  // inst_term aps are written back here if not early stopped
  // IO term aps are are written back in genPinAccessCostBounded and always
  // early stopped
  updatePinStats(aps, pin, inst_term);
  const int n_aps = aps.size();
  if (n_aps == 0) {
    if (inst_term && isStdCell(inst_term->getInst())) {
      std_cell_pin_no_ap_cnt_++;
    }
    if (inst_term && isMacroCell(inst_term->getInst())) {
      macro_cell_pin_no_ap_cnt_++;
    }
  } else {
    if (inst_term == nullptr) {
      logger_->error(DRT, 254, "inst_term can not be nullptr");
    }
    // write to pa
    const int pin_access_idx = inst_term->getInst()->getPinAccessIdx();
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
  }
  return n_aps;
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

  const std::vector<frInst*>& unique = unique_insts_.getUnique();
#pragma omp parallel for schedule(dynamic)
  for (frInst* unique_inst : unique) {  // NOLINT
    try {
      // only do for core and block cells
      if (!isStdCell(unique_inst) && !isMacroCell(unique_inst)) {
        continue;
      }

      genInstAccessPoints(unique_inst);
      if (router_cfg_->VERBOSE <= 0) {
        continue;
      }

      int inst_terms_cnt = static_cast<int>(unique_inst->getInstTerms().size());
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

void FlexPA::revertAccessPoints()
{
  const auto& unique = unique_insts_.getUnique();
  for (frInst* inst : unique) {
    const dbTransform xform = inst->getTransform();
    const Point offset(xform.getOffset());
    dbTransform revertXform(Point(-offset.getX(), -offset.getY()));

    const auto pin_access_idx = inst->getPinAccessIdx();
    ;
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

}  // namespace drt
