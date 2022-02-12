//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "strap.h"

#include <limits>

#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Strap::Strap(Grid* grid,
             odb::dbTechLayer* layer,
             int width,
             int pitch,
             int spacing,
             int number_of_straps)
    : GridShape(grid),
      layer_(layer),
      width_(width),
      spacing_(spacing),
      pitch_(pitch),
      offset_(0),
      number_of_straps_(number_of_straps),
      direction_(odb::dbTechLayerDir::NONE),
      snap_(false),
      starts_with_power_(grid->startsWithPower()),
      extend_mode_(CORE)
{
  if (spacing_ == 0) {
    // spacing not defined, so use pitch / 2
    int net_count = 2;
    spacing_ = pitch_ / net_count - width_;
  }
  if (layer_ != nullptr) {
    direction_ = layer_->getDirection();
  }
}

void Strap::checkLayerSpecifications() const
{
  if (layer_ != nullptr) {
    checkLayerWidth(layer_, width_, direction_);
    checkLayerSpacing(layer_, width_, spacing_, direction_);
  }

  if (pitch_ != 0) {
    const int min_pitch = getStrapGroupWidth();
    if (pitch_ < min_pitch) {
      double lef_units = layer_->getTech()->getLefUnits();
      getLogger()->error(
          utl::PDN,
          175,
          "Pitch {:.4f} is too small for, must be atleast {:.4f}",
          pitch_ / lef_units,
          min_pitch / lef_units);
    }
  }
}

void Strap::setOffset(int offset)
{
  offset_ = offset;
}

void Strap::setSnapToGrid(bool snap)
{
  snap_ = snap;
}

void Strap::setExtend(Extend mode)
{
  extend_mode_ = mode;
}

std::vector<odb::dbNet*> Strap::getNets() const
{
  return getGrid()->getNets(starts_with_power_);
}

void Strap::makeShapes(const ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Strap start of make shapes for on layer {}",
             layer_->getName());
  clearShapes();

  auto* grid = getGrid();

  odb::Rect boundary;
  odb::Rect core = grid->getCoreArea();
  switch (extend_mode_) {
    case CORE:
      boundary = grid->getCoreBoundary();
      break;
    case RINGS:
      boundary = grid->getRingBoundary();
      break;
    case BOUNDARY:
      boundary = grid->getDieBoundary();
      break;
  }

  if (direction_ == odb::dbTechLayerDir::HORIZONTAL) {
    const int x_start = boundary.xMin();
    const int x_end = boundary.xMax();

    std::vector<int> snap_grid;
    if (snap_) {
      getBlock()->findTrackGrid(layer_)->getGridY(snap_grid);
    }

    makeStraps(x_start, core.yMin(), x_end, core.yMax(), false, snap_grid);
  } else {
    const int y_start = boundary.yMin();
    const int y_end = boundary.yMax();

    std::vector<int> snap_grid;
    if (snap_) {
      getBlock()->findTrackGrid(layer_)->getGridX(snap_grid);
    }

    makeStraps(core.xMin(), y_start, core.xMax(), y_end, true, snap_grid);
  }
}

void Strap::makeStraps(int x_start,
                       int y_start,
                       int x_end,
                       int y_end,
                       bool is_delta_x,
                       const std::vector<int>& snap_grid)
{
  const int half_width = width_ / 2;
  int strap_count = 0;

  int pos = is_delta_x ? x_start : y_start;
  const int pos_end = is_delta_x ? x_end : y_end;

  const auto nets = getNets();

  int last_strap_end = -spacing_;
  for (pos += offset_; pos <= pos_end; pos += pitch_) {
    int group_pos = pos;
    for (auto* net : nets) {
      // snap to grid if needed
      const int strap_start
          = snapToGrid(group_pos + half_width, snap_grid, last_strap_end + half_width + spacing_) - half_width;
      const int strap_end = strap_start + width_;
      if (strap_end > pos_end) {
        // do not add if strap will exceed the area allotted
        return;
      }

      odb::Rect strap_rect;
      if (is_delta_x) {
        strap_rect = odb::Rect(strap_start, y_start, strap_end, y_end);
      } else {
        strap_rect = odb::Rect(x_start, strap_start, x_end, strap_end);
      }
      last_strap_end = strap_end;
      addShape(
          new Shape(layer_, net, strap_rect, odb::dbWireShapeType::STRIPE));
      group_pos += spacing_ + width_;
    }
    strap_count++;
    if (number_of_straps_ != 0 && strap_count == number_of_straps_) {
      // if number of straps is met, stop adding
      return;
    }
  }
}

int Strap::snapToGrid(int pos, const std::vector<int>& grid, int greater_than) const
{
  if (grid.empty()) {
    return pos;
  }

  int delta_pos = 0;
  int delta = std::numeric_limits<int>::max();
  for (size_t i = 0; i < grid.size(); i++) {
    const int grid_pos = grid[i];
    if (grid_pos < greater_than) {
      // ignore since it is lower than the minimum
      continue;
    }

    // look for smallest delta
    const int new_delta = std::abs(pos - grid_pos);
    if (new_delta < delta) {
      delta_pos = grid_pos;
      delta = new_delta;
    } else {
      break;
    }
  }
  return delta_pos;
}

void Strap::report() const
{
  auto* logger = getLogger();
  auto* block = getGrid()->getBlock();

  double dbu_per_micron = block->getDbUnitsPerMicron();

  logger->info(utl::PDN, 40, "  Type: {}", typeToString(type()));
  logger->info(utl::PDN, 41, "    Layer: {}", layer_->getName());
  logger->info(utl::PDN, 42, "    Width: {:.4f}", width_ / dbu_per_micron);
  logger->info(utl::PDN, 43, "    Spacing: {:.4f}", spacing_ / dbu_per_micron);
  logger->info(utl::PDN, 44, "    Pitch: {:.4f}", pitch_ / dbu_per_micron);
  logger->info(utl::PDN, 45, "    Offset: {:.4f}", offset_ / dbu_per_micron);
}

int Strap::getStrapGroupWidth() const
{
  const auto nets = getNets();
  const int net_count = nets.size();

  int width = net_count * width_;
  width += (net_count - 1) * spacing_;

  return width;
}

////

FollowPin::FollowPin(Grid* grid, odb::dbTechLayer* layer, int width)
    : Strap(grid, layer, width, 0)
{
  if (getWidth() == 0) {
    // width not specified, so attempt to find it
    determineWidth();
  }
}

void FollowPin::makeShapes(const ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Follow pin start of make shapes on layer {}",
             getLayer()->getName());
  clearShapes();

  const int width = getWidth();

  auto* grid = getGrid();

  const odb::Rect core = grid->getCoreArea();
  odb::Rect boundary;
  switch (getExtendMode()) {
    case CORE:
      // use core area for follow pins
      boundary = grid->getCoreArea();
      break;
    case RINGS:
      boundary = grid->getRingBoundary();
      break;
    case BOUNDARY:
      boundary = grid->getDieBoundary();
      break;
  }

  odb::dbNet* power = getDomain()->getPower();
  odb::dbNet* ground = getDomain()->getGround();

  const int x_start = boundary.xMin();
  const int x_end = boundary.xMax();
  odb::dbTechLayer* layer = getLayer();
  for (auto* row : getDomain()->getRows()) {
    odb::Rect bbox;
    row->getBBox(bbox);
    const bool power_on_top = row->getOrient() == odb::dbOrientType::R0;

    int x0 = bbox.xMin();
    if (x0 == core.xMin()) {
      x0 = x_start;
    }
    int x1 = bbox.xMax();
    if (x1 == core.xMax()) {
      x1 = x_end;
    }

    const int power_y_bot
        = (power_on_top ? bbox.yMax() : bbox.yMin()) - width / 2;
    const int ground_y_bot
        = (power_on_top ? bbox.yMin() : bbox.yMax()) - width / 2;

    auto* power_strap = new FollowPinShape(
        layer, power, odb::Rect(x0, power_y_bot, x1, power_y_bot + width));
    power_strap->addRow(row);
    addShape(power_strap);

    auto* ground_strap = new FollowPinShape(
        layer, ground, odb::Rect(x0, ground_y_bot, x1, ground_y_bot + width));
    ground_strap->addRow(row);
    addShape(ground_strap);
  }
}

void FollowPin::determineWidth()
{
  auto* db = getBlock()->getDb();

  int width = std::numeric_limits<int>::max();
  for (auto* lib : db->getLibs()) {
    for (auto* master : lib->getMasters()) {
      if (master->isCore()) {
        // only check core cells
        debugPrint(getLogger(),
                   utl::PDN,
                   "Followpin",
                   1,
                   "Checking master {} (current width: {})",
                   master->getName(),
                   width);
        for (auto* mterm : master->getMTerms()) {
          if (!mterm->getSigType().isSupply()) {
            continue;
          }
          debugPrint(getLogger(),
                     utl::PDN,
                     "Followpin",
                     2,
                     "Checking master/pin {}/{} (current width: {})",
                     master->getName(),
                     mterm->getName(),
                     width);
          for (auto* mpin : mterm->getMPins()) {
            for (auto* box : mpin->getGeometry()) {
              auto* layer = box->getTechLayer();
              if (layer == nullptr) {
                continue;
              }
              if (layer->getType() != odb::dbTechLayerType::ROUTING) {
                continue;
              }

              width = std::min(width, static_cast<int>(box->getDY()));
              debugPrint(getLogger(),
                         utl::PDN,
                         "Followpin",
                         3,
                         "Updating based on pin: {}",
                         width);
            }
          }
        }
      }
    }
  }

  // nothing was found
  if (width == std::numeric_limits<int>::max()) {
    getLogger()->error(
        utl::PDN,
        109,
        "Unable to determine width of followpin straps from standard cells.");
  }

  setWidth(width);
}

void FollowPin::checkLayerSpecifications() const
{
  checkLayerWidth(getLayer(), getWidth(), getDirection());
}

///////////

PadDirectConnect::PadDirectConnect(
    Grid* grid,
    odb::dbITerm* iterm,
    const std::vector<odb::dbTechLayer*>& connect_pad_layers)
    : Strap(grid, nullptr, 0, 0),
      iterm_(iterm),
      target_shapes_(odb::dbWireShapeType::RING),
      pad_edge_(odb::dbDirection::NONE)
{
  initialize(connect_pad_layers);
}

bool PadDirectConnect::canConnect() const
{
  return pad_edge_ != odb::dbDirection::NONE && !pins_.empty();
}

void PadDirectConnect::initialize(const std::vector<odb::dbTechLayer*>& layers)
{
  auto* inst = iterm_->getInst();

  odb::Rect inst_rect;
  inst->getBBox()->getBox(inst_rect);

  auto* block = inst->getBlock();
  odb::Rect core_rect;
  block->getCoreArea(core_rect);

  const bool is_north = inst_rect.yMin() > core_rect.yMax();
  const bool is_south = inst_rect.yMax() < core_rect.yMin();
  const bool is_west = inst_rect.xMax() < core_rect.xMin();
  const bool is_east = inst_rect.xMin() > core_rect.xMax();

  const bool is_horizontal = is_west || is_east;

  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             1,
             "{} connecting on edge: north {}, south {}, east {}, west {}",
             getName(),
             is_north,
             is_south,
             is_east,
             is_east);

  auto* mterm = iterm_->getMTerm();
  for (auto* pin : mterm->getMPins()) {
    for (auto* box : pin->getGeometry()) {
      auto* layer = box->getTechLayer();
      if (layer == nullptr) {
        continue;
      }

      // only look at routing layers
      if (layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }

      // check if layer should be used
      if (!layers.empty()
          && std::find(layers.begin(), layers.end(), layer) == layers.end()) {
        continue;
      }

      // only add pins that would yield correct routing directions
      if (is_horizontal) {
        if (layer->getDirection() != odb::dbTechLayerDir::HORIZONTAL) {
          continue;
        }
      } else {
        if (layer->getDirection() != odb::dbTechLayerDir::VERTICAL) {
          continue;
        }
      }

      pins_.push_back(box);
    }
  }

  odb::dbTransform transform;
  inst->getTransform(transform);

  // remove all pins that do not face the core
  std::function<bool(odb::dbBox*)> remove_func;
  if (is_north) {
    pad_edge_ = odb::dbDirection::NORTH;
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect;
      box->getBox(box_rect);
      transform.apply(box_rect);
      return inst_rect.yMin() != box_rect.yMin();
    };
  } else if (is_south) {
    pad_edge_ = odb::dbDirection::SOUTH;
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect;
      box->getBox(box_rect);
      transform.apply(box_rect);
      return inst_rect.yMax() != box_rect.yMax();
    };
  } else if (is_west) {
    pad_edge_ = odb::dbDirection::WEST;
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect;
      box->getBox(box_rect);
      transform.apply(box_rect);
      return inst_rect.xMax() != box_rect.xMax();
    };
  } else {
    pad_edge_ = odb::dbDirection::EAST;
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect;
      box->getBox(box_rect);
      transform.apply(box_rect);
      return inst_rect.xMin() != box_rect.xMin();
    };
  }

  pins_.erase(std::remove_if(pins_.begin(), pins_.end(), remove_func),
              pins_.end());
}

void PadDirectConnect::report() const
{
  auto* logger = getLogger();

  logger->info(utl::PDN, 80, "  Type: {}", typeToString(type()));
  logger->info(utl::PDN, 81, "    Pin: {}", getName());
}

std::string PadDirectConnect::getName() const
{
  return iterm_->getInst()->getName() + "/" + iterm_->getMTerm()->getName();
}

bool PadDirectConnect::isConnectHorizontal() const
{
  return pad_edge_ == odb::dbDirection::WEST
         || pad_edge_ == odb::dbDirection::EAST;
}

void PadDirectConnect::makeShapes(const ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Direct connect pin start of make shapes for {}",
             getName());
  auto search_shapes = getGrid()->getShapes();
  if (search_shapes.empty()) {
    return;
  }

  auto* inst = iterm_->getInst();

  odb::Rect inst_rect;
  inst->getBBox()->getBox(inst_rect);

  auto* block = inst->getBlock();
  odb::Rect die_rect;
  block->getDieArea(die_rect);

  odb::dbTransform transform;
  inst->getTransform(transform);

  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  auto* net = iterm_->getNet();
  for (auto* pin : pins_) {
    odb::Rect pin_rect;
    pin->getBox(pin_rect);
    transform.apply(pin_rect);

    auto* layer = pin->getTechLayer();

    // genertae search box
    Box search;
    if (isConnectHorizontal()) {
      search = Box(Point(die_rect.xMin(), pin_rect.yMin()),
                   Point(die_rect.xMax(), pin_rect.yMax()));
    } else {
      search = Box(Point(pin_rect.xMin(), die_rect.yMin()),
                   Point(pin_rect.xMax(), die_rect.yMax()));
    }

    // find nearest target
    ShapePtr closest_shape = nullptr;
    int closest_dist = std::numeric_limits<int>::max();
    for (const auto& [search_layer, search_shape_tree] : search_shapes) {
      if (layer == search_layer) {
        continue;
      }

      for (auto it = search_shape_tree.qbegin(bgi::intersects(search));
           it != search_shape_tree.qend();
           it++) {
        const auto& shape = it->second;
        if (shape->getNet() != net) {
          continue;
        }

        if (shape->getType() != target_shapes_) {
          continue;
        }

        int new_dist;
        if (is_west) {
          new_dist = shape->getRect().xMin() - pin_rect.xMax();
        } else if (is_east) {
          new_dist = pin_rect.xMin() - shape->getRect().xMax();
        } else if (is_south) {
          new_dist = shape->getRect().yMin() - pin_rect.yMax();
        } else {
          new_dist = pin_rect.yMin() - shape->getRect().yMax();
        }

        // determine if this is closer
        if (closest_dist > new_dist) {
          closest_shape = shape;
          closest_dist = new_dist;
        }
      }

      if (closest_shape == nullptr) {
        continue;
      }

      odb::Rect shape_rect = pin_rect;
      if (is_west) {
        shape_rect.set_xhi(closest_shape->getRect().xMax());
      } else if (is_east) {
        shape_rect.set_xlo(closest_shape->getRect().xMin());
      } else if (is_south) {
        shape_rect.set_yhi(closest_shape->getRect().yMax());
      } else {
        shape_rect.set_ylo(closest_shape->getRect().yMin());
      }

      auto* shape
          = new Shape(layer, net, shape_rect, odb::dbWireShapeType::STRIPE);
      shape->addITermConnection(pin_rect);
      addShape(shape);
    }
  }
}

////////

RepairChannel::RepairChannel(Grid* grid,
                             Strap* target,
                             odb::dbTechLayer* connect_to,
                             const ShapeTreeMap& other_shapes,
                             const std::set<odb::dbNet*>& nets,
                             const odb::Rect& area)
    : Strap(grid,
            target->getLayer(),
            target->getWidth(),
            0,
            target->getSpacing(),
            1),
      nets_(nets),
      connect_to_(connect_to)
{
  // copy extend mode
  setExtend(target->getExtendMode());
  // use snap to grid
  setSnapToGrid(true);

  determineParameters(area, other_shapes);
}

std::string RepairChannel::getNetString() const
{
  std::string nets;

  for (auto* net : nets_) {
    nets += net->getName() + " ";
  }

  return nets;
}

void RepairChannel::determineParameters(const odb::Rect& area,
                                        const ShapeTreeMap& obstructions)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Channel",
             1,
             "Determining channel parameters for {} with {}",
             Shape::getRectText(area, getBlock()->getDbUnitsPerMicron()),
             getNetString());
  int max_length = 0;
  int area_width = 0;
  const odb::Rect core = getGrid()->getCoreArea();
  if (getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    max_length = core.dx();
    area_width = area.dy();
  } else {
    max_length = core.dy();
    area_width = area.dx();
  }

  auto check = [&]() -> bool {
    if (getStrapGroupWidth() > area_width) {
      return false;
    }
    return determineOffset(area, obstructions);
  };

  if (check()) {
    // straps already fit
    return;
  }

  const TechLayer layer(getLayer());
  const double dbu_to_microns = layer.getLefUnits();

  // adjust spacing first to minimum spacing for the given width
  setSpacing(layer.getSpacing(getWidth(), max_length));

  debugPrint(getLogger(),
             utl::PDN,
             "Channel",
             2,
             "Adjust spacing to {:.4f} um.",
             getSpacing() / dbu_to_microns);
  if (check()) {
    // adjusting spacing works
    return;
  }

  const int min_width = layer.getMinWidth();
  while (getWidth() > min_width) {
    // adjust the width and spacing until something works
    const int new_width = std::max(getWidth() / 2, min_width);
    setWidth(new_width);
    setSpacing(layer.getSpacing(new_width, max_length));

    debugPrint(getLogger(),
               utl::PDN,
               "Channel",
               2,
               "Adjust width to {:.4f} um and spacing to {:.4f} um.",
               getWidth() / dbu_to_microns,
               getSpacing() / dbu_to_microns);
    if (check()) {
      return;
    }
  }

  getLogger()->error(
      utl::PDN,
      176,
      "Channel {} is too narrow, it must be atleast {:.4f} wide.",
      Shape::getRectText(area, dbu_to_microns),
      getStrapGroupWidth() / dbu_to_microns);
}

bool RepairChannel::determineOffset(const odb::Rect& area,
                                    const ShapeTreeMap& obstructions,
                                    int extra_offset,
                                    int bisect_dist,
                                    int level)
{
  const bool is_horizontal = getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  const int group_width = getStrapGroupWidth();
  int offset = -group_width / 2 + extra_offset;
  if (is_horizontal) {
    offset += 0.5 * (area.yMin() + area.yMax());
  } else {
    offset += 0.5 * (area.xMin() + area.xMax());
  }

  // check if straps will fit
  if (is_horizontal) {
    if (offset < area.yMin() || offset + group_width > area.yMax()) {
      return false;
    }
  } else {
    if (offset < area.xMin() || offset + group_width > area.xMax()) {
      return false;
    }
  }

  // check if straps will intersect anything on the layers below
  std::vector<odb::dbTechLayer*> check_layers;
  for (odb::dbTechLayer* next_layer = connect_to_->getUpperLayer();
       next_layer != getLayer();) {
    if (next_layer->getType() == odb::dbTechLayerType::ROUTING) {
      check_layers.push_back(next_layer);
    }

    next_layer = next_layer->getUpperLayer();
  }
  check_layers.push_back(getLayer());
  bool has_obs = false;
  odb::Rect fake_straps;
  if (is_horizontal) {
    fake_straps
        = odb::Rect(area.xMin(), offset, area.xMax(), offset + group_width);
  } else {
    fake_straps
        = odb::Rect(offset, area.yMin(), offset + group_width, area.yMax());
  }
  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      3,
      "Estimating strap to be {}.",
      Shape::getRectText(fake_straps, getBlock()->getDbUnitsPerMicron()));
  Box fake_straps_box(Point(fake_straps.xMin(), fake_straps.yMin()),
                      Point(fake_straps.xMax(), fake_straps.yMax()));
  for (auto* layer : check_layers) {
    if (obstructions.count(layer) != 0) {
      // obstructions possible on this layer
      const auto& shapes = obstructions.at(layer);
      auto obs_find = shapes.qbegin(bgi::intersects(fake_straps_box));
      if (obs_find != shapes.qend()) {
        debugPrint(getLogger(),
                   utl::PDN,
                   "Channel",
                   3,
                   "Obstruction encountered at {} on {}.",
                   Shape::getRectText(obs_find->second->getObstruction(),
                                      getBlock()->getDbUnitsPerMicron()),
                   obs_find->second->getLayer()->getName());
        has_obs = true;
        // found an obstruction, so it is safe to stop
        break;
      }
    }
  }
  if (has_obs) {
    // obstruction found, so try looking for a different offset
    int new_bisect_dist;
    if (bisect_dist == 0) {
      // first time, so use offset of width / 4
      const int width = is_horizontal ? area.dy() : area.dx();
      new_bisect_dist = width / 4;
    } else {
      // not first time, so search half the distance of the current bisection.
      new_bisect_dist = bisect_dist / 2;
    }

    if (new_bisect_dist == 0) {
      // no more searches possible
      return false;
    }

    debugPrint(
        getLogger(),
        utl::PDN,
        "ChannelBisect",
        1,
        "Current level {}, offset {:.4f} um, group width {:.4f} um.",
        level,
        static_cast<double>(extra_offset) / getBlock()->getDbUnitsPerMicron(),
        static_cast<double>(group_width) / getBlock()->getDbUnitsPerMicron());
    debugPrint(getLogger(),
               utl::PDN,
               "ChannelBisect",
               2,
               "  Bisect distance {:.4f} um, low {:.4f} um, high {:.4f} um.",
               static_cast<double>(new_bisect_dist)
                   / getBlock()->getDbUnitsPerMicron(),
               static_cast<double>(extra_offset - new_bisect_dist)
                   / getBlock()->getDbUnitsPerMicron(),
               static_cast<double>(extra_offset + new_bisect_dist)
                   / getBlock()->getDbUnitsPerMicron());
    if (new_bisect_dist < group_width) {
      // new offset to too small to matter, so stop
      return false;
    } else {
      // check lower range first
      if (determineOffset(area,
                          obstructions,
                          extra_offset - new_bisect_dist,
                          new_bisect_dist,
                          level + 1)) {
        return true;
      }
      // check higher range next
      if (determineOffset(area,
                          obstructions,
                          extra_offset + new_bisect_dist,
                          new_bisect_dist,
                          level + 1)) {
        return true;
      }
    }
    // no offset found
    return false;
  }

  odb::Rect core = getGrid()->getCoreArea();
  if (is_horizontal) {
    offset -= core.yMin();
  } else {
    offset -= core.xMin();
  }

  // apply offset found
  setOffset(offset);

  return true;
}

std::vector<odb::dbNet*> RepairChannel::getNets() const
{
  std::vector<odb::dbNet*> nets(nets_.begin(), nets_.end());

  return nets;
}

}  // namespace pdn
