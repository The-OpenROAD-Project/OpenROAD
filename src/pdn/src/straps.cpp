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

#include "straps.h"

#include <boost/geometry.hpp>
#include <boost/polygon/polygon.hpp>
#include <limits>

#include "connect.h"
#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Straps::Straps(Grid* grid,
               odb::dbTechLayer* layer,
               int width,
               int pitch,
               int spacing,
               int number_of_straps)
    : GridComponent(grid),
      layer_(layer),
      width_(width),
      spacing_(spacing),
      pitch_(pitch),
      number_of_straps_(number_of_straps)
{
  if (spacing_ == 0 && pitch_ != 0) {
    // spacing not defined, so use pitch / (# of nets)
    spacing_ = pitch_ / getNetCount() - width_;
    // round down, later check will fail if spacing does not meet min spacing,
    // but rounding up will cause pitch check to fail.
    spacing_ = TechLayer::snapToManufacturingGrid(
        getBlock()->getDataBase()->getTech(), spacing_, false);
  }
  if (layer_ != nullptr) {
    direction_ = layer_->getDirection();
  }
}

void Straps::checkLayerSpecifications() const
{
  if (layer_ == nullptr) {
    return;
  }

  if (direction_ == odb::dbTechLayerDir::NONE) {
    getLogger()->error(
        utl::PDN,
        187,
        "Unable to place strap on {} with unknown routing direction.",
        layer_->getName());
  }

  const TechLayer layer(layer_);

  checkLayerWidth(layer_, width_, direction_);
  checkLayerSpacing(layer_, width_, spacing_, direction_);
  layer.checkIfManufacturingGrid(width_, getLogger(), "Width");
  layer.checkIfManufacturingGrid(spacing_, getLogger(), "Spacing");
  layer.checkIfManufacturingGrid(pitch_, getLogger(), "Pitch");
  layer.checkIfManufacturingGrid(offset_, getLogger(), "Offset");

  const int strap_width = getStrapGroupWidth();
  if (pitch_ != 0) {
    const int min_pitch = strap_width + spacing_;
    if (pitch_ < min_pitch) {
      getLogger()->error(
          utl::PDN,
          175,
          "Pitch {:.4f} is too small for, must be atleast {:.4f}",
          layer.dbuToMicron(pitch_),
          layer.dbuToMicron(min_pitch));
    }
  }

  checkLayerOffsetSpecification(true);
}

bool Straps::checkLayerOffsetSpecification(bool error) const
{
  const int strap_width = getStrapGroupWidth();
  const odb::Rect grid_area = getGrid()->getDomainArea();
  int grid_width = 0;
  if (isHorizontal()) {
    grid_width = grid_area.dy();
  } else {
    grid_width = grid_area.dx();
  }
  if (grid_width < offset_ + strap_width) {
    if (error) {
      const TechLayer layer(layer_);
      getLogger()->error(
          utl::PDN,
          185,
          "Insufficient width ({} um) to add straps on layer {} in grid \"{}\" "
          "with total strap width {} um and offset {} um.",
          layer.dbuToMicron(grid_width),
          layer_->getName(),
          getGrid()->getLongName(),
          layer.dbuToMicron(strap_width),
          layer.dbuToMicron(offset_));
    } else {
      return false;
    }
  }
  return true;
}

void Straps::setOffset(int offset)
{
  offset_ = offset;
}

void Straps::setSnapToGrid(bool snap)
{
  snap_ = snap;
}

void Straps::setExtend(ExtensionMode mode)
{
  extend_mode_ = mode;
}

void Straps::setStrapStartEnd(int start, int end)
{
  extend_mode_ = FIXED;
  strap_start_ = start;
  strap_end_ = end;
}

void Straps::makeShapes(const ShapeTreeMap& other_shapes)
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
  odb::Rect core = grid->getDomainArea();
  switch (extend_mode_) {
    case CORE:
      boundary = grid->getDomainBoundary();
      break;
    case RINGS:
      boundary = grid->getRingArea();
      break;
    case BOUNDARY:
      boundary = grid->getGridBoundary();
      break;
    case FIXED:
      boundary = odb::Rect(strap_start_, strap_start_, strap_end_, strap_end_);
      break;
  }

  TechLayer layer(layer_);
  if (snap_) {
    layer.populateGrid(getBlock(), getDirection());
  }

  // collect shapes to avoid placing straps over
  ShapeTree avoid;
  if (other_shapes.count(layer_) != 0) {
    for (const auto& [box, shape] : other_shapes.at(layer_)) {
      if (shape->getType() == odb::dbWireShapeType::RING) {
        // avoid ring shapes
        avoid.insert({shape->getObstructionBox(), shape});
      }
    }
  }

  if (isHorizontal()) {
    const int x_start = boundary.xMin();
    const int x_end = boundary.xMax();

    makeStraps(x_start, core.yMin(), x_end, core.yMax(), false, layer, avoid);
  } else {
    const int y_start = boundary.yMin();
    const int y_end = boundary.yMax();

    makeStraps(core.xMin(), y_start, core.xMax(), y_end, true, layer, avoid);
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Straps",
             1,
             "Generated {} straps",
             getShapeCount());
}

void Straps::makeStraps(int x_start,
                        int y_start,
                        int x_end,
                        int y_end,
                        bool is_delta_x,
                        const TechLayer& layer,
                        const ShapeTree& avoid)
{
  const int half_width = width_ / 2;
  int strap_count = 0;

  int pos = is_delta_x ? x_start : y_start;
  const int pos_end = is_delta_x ? x_end : y_end;

  const auto nets = getNets();

  const int group_pitch = spacing_ + width_;

  int next_minimum_track = 0;
  for (pos += offset_; pos <= pos_end; pos += pitch_) {
    int group_pos = pos;
    for (auto* net : nets) {
      // snap to grid if needed
      group_pos = layer.snapToGrid(group_pos, next_minimum_track);
      const int strap_start = group_pos - half_width;
      const int strap_end = strap_start + width_;
      if (strap_start >= pos_end) {
        // no portion of the strap is inside the limit
        return;
      }
      if (group_pos > pos_end) {
        // strap center is outside of alotted area
        return;
      }

      odb::Rect strap_rect;
      if (is_delta_x) {
        strap_rect = odb::Rect(strap_start, y_start, strap_end, y_end);
      } else {
        strap_rect = odb::Rect(x_start, strap_start, x_end, strap_end);
      }
      group_pos += group_pitch;
      next_minimum_track = group_pos;

      Box shape_check(Point(strap_rect.xMin(), strap_rect.yMin()),
                      Point(strap_rect.xMax(), strap_rect.yMax()));

      if (avoid.qbegin(bgi::intersects(shape_check)) != avoid.qend()) {
        // dont add this strap as it intersects an avoidance
        continue;
      }

      addShape(
          new Shape(layer_, net, strap_rect, odb::dbWireShapeType::STRIPE));
    }
    strap_count++;
    if (number_of_straps_ != 0 && strap_count == number_of_straps_) {
      // if number of straps is met, stop adding
      return;
    }
  }
}

void Straps::report() const
{
  auto* logger = getLogger();
  auto* block = getGrid()->getBlock();

  const double dbu_per_micron = block->getDbUnitsPerMicron();

  logger->report("  Type: {}", typeToString(type()));
  logger->report("    Layer: {}", layer_->getName());
  logger->report("    Width: {:.4f}", width_ / dbu_per_micron);
  logger->report("    Spacing: {:.4f}", spacing_ / dbu_per_micron);
  logger->report("    Pitch: {:.4f}", pitch_ / dbu_per_micron);
  logger->report("    Offset: {:.4f}", offset_ / dbu_per_micron);
  logger->report("    Snap to grid: {}", snap_);
  if (number_of_straps_ > 0) {
    logger->report("    Number of strap sets: {}", number_of_straps_);
  }
}

int Straps::getStrapGroupWidth() const
{
  const auto nets = getNets();
  const int net_count = nets.size();

  int width = net_count * width_;
  width += (net_count - 1) * spacing_;

  return width;
}

////

FollowPins::FollowPins(Grid* grid, odb::dbTechLayer* layer, int width)
    : Straps(grid, layer, width, 0)
{
  if (getWidth() == 0) {
    // width not specified, so attempt to find it
    determineWidth();
  }

  // set the pitch of the straps
  auto rows = getDomain()->getRows();
  if (!rows.empty()) {
    auto* row = *rows.begin();
    odb::Rect bbox = row->getBBox();
    setPitch(2 * bbox.dy());

    if (row->getDirection() == odb::dbRowDir::HORIZONTAL) {
      setDirection(odb::dbTechLayerDir::HORIZONTAL);
    } else {
      setDirection(odb::dbTechLayerDir::VERTICAL);
    }
  } else {
    if (getPitch() == 0) {
      getLogger()->error(
          utl::PDN, 190, "Unable to determine the pitch of the rows.");
    }
  }
}

void FollowPins::makeShapes(const ShapeTreeMap& other_shapes)
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

  const odb::Rect core = grid->getDomainArea();
  odb::Rect boundary;
  switch (getExtendMode()) {
    case CORE:
    case FIXED:
      // use core area for follow pins
      boundary = grid->getDomainArea();
      break;
    case RINGS:
      boundary = grid->getRingArea();
      break;
    case BOUNDARY:
      boundary = grid->getGridBoundary();
      break;
  }

  odb::dbNet* power = getDomain()->getPower();
  odb::dbNet* ground = getDomain()->getGround();

  const int x_start = boundary.xMin();
  const int x_end = boundary.xMax();
  odb::dbTechLayer* layer = getLayer();
  for (auto* row : getDomain()->getRows()) {
    odb::Rect bbox = row->getBBox();
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

void FollowPins::determineWidth()
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

void FollowPins::checkLayerSpecifications() const
{
  checkLayerWidth(getLayer(), getWidth(), getDirection());
}

///////////

PadDirectConnectionStraps::PadDirectConnectionStraps(
    Grid* grid,
    odb::dbITerm* iterm,
    const std::vector<odb::dbTechLayer*>& connect_pad_layers)
    : Straps(grid, nullptr, 0, 0), iterm_(iterm), layers_(connect_pad_layers)
{
  initialize(type_);
}

bool PadDirectConnectionStraps::canConnect() const
{
  return pad_edge_ != odb::dbDirection::NONE && !pins_.empty() && type_ != None;
}

void PadDirectConnectionStraps::initialize(ConnectionType type)
{
  pins_.clear();

  auto* inst = iterm_->getInst();
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  auto* block = inst->getBlock();
  const odb::Rect core_rect = block->getCoreArea();

  std::set<odb::dbDirection> pad_edges;
  if (inst_rect.yMin() > core_rect.yMax()) {
    pad_edges.insert(odb::dbDirection::NORTH);
  }
  if (inst_rect.yMax() < core_rect.yMin()) {
    pad_edges.insert(odb::dbDirection::SOUTH);
  }
  if (inst_rect.xMax() < core_rect.xMin()) {
    pad_edges.insert(odb::dbDirection::WEST);
  }
  if (inst_rect.xMin() > core_rect.xMax()) {
    pad_edges.insert(odb::dbDirection::EAST);
  }

  if (pad_edges.empty() || pad_edges.size() >= 2) {
    pad_edge_ = odb::dbDirection::NONE;
  } else {
    pad_edge_ = *pad_edges.begin();
  }

  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             1,
             "{} connecting on edge: north {}, south {}, east {}, west {}",
             getName(),
             pad_edge_ == odb::dbDirection::NORTH,
             pad_edge_ == odb::dbDirection::SOUTH,
             pad_edge_ == odb::dbDirection::EAST,
             pad_edge_ == odb::dbDirection::WEST);

  switch (type) {
    case None:
      pins_ = getPinsFacingCore();
      if (pins_.empty()) {
        // check if pins are accessible from above
        pins_ = getPinsFormingRing();
      }
      break;
    case Edge:
      pins_ = getPinsFacingCore();
      break;
    case OverPads:
      pins_ = getPinsFormingRing();
      break;
  }
}

std::map<odb::dbTechLayer*, std::vector<odb::dbBox*>>
PadDirectConnectionStraps::getPinsByLayer() const
{
  std::map<odb::dbTechLayer*, std::vector<odb::dbBox*>> pins;

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

      pins[layer].push_back(box);
    }
  }

  return pins;
}

std::vector<odb::dbBox*> PadDirectConnectionStraps::getPinsFacingCore()
{
  auto* inst = iterm_->getInst();
  const odb::Rect inst_rect = inst->getBBox()->getBox();

  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  const bool is_horizontal = is_west || is_east;

  std::vector<odb::dbBox*> pins;

  auto pins_by_layer = getPinsByLayer();
  if (!layers_.empty()) {
    // remove unspecified layers
    for (auto itr = pins_by_layer.begin(); itr != pins_by_layer.end();) {
      auto layer = itr->first;
      if (std::find(layers_.begin(), layers_.end(), layer) == layers_.end()) {
        // remove layer
        itr = pins_by_layer.erase(itr);
      } else {
        itr++;
      }
    }
  }

  // check for pin directions
  bool has_horizontal_pins = false;
  bool has_vertical_pins = false;
  for (const auto& [layer, layer_pins] : pins_by_layer) {
    if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      has_horizontal_pins = true;
    }
    if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      has_vertical_pins = true;
    }
  }

  const bool has_multiple_directions = has_horizontal_pins && has_vertical_pins;

  for (const auto& [layer, layer_pins] : pins_by_layer) {
    if (has_multiple_directions) {
      // only add pins that would yield correct routing directions,
      // otherwise keep non-preferred directions too
      if (is_horizontal) {
        if (layer->getDirection() != odb::dbTechLayerDir::HORIZONTAL) {
          continue;
        }
      } else {
        if (layer->getDirection() != odb::dbTechLayerDir::VERTICAL) {
          continue;
        }
      }
    }
    pins.insert(pins.end(), layer_pins.begin(), layer_pins.end());
  }

  odb::dbTransform transform;
  inst->getTransform(transform);

  // remove all pins that do not face the core
  std::function<bool(odb::dbBox*)> remove_func;
  if (is_north) {
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.yMin() < box_rect.yMin();
    };
  } else if (is_south) {
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.yMax() > box_rect.yMax();
    };
  } else if (is_west) {
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.xMax() > box_rect.xMax();
    };
  } else {
    remove_func = [inst_rect, transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.xMin() < box_rect.xMin();
    };
  }

  pins.erase(std::remove_if(pins.begin(), pins.end(), remove_func), pins.end());

  if (!pins.empty()) {
    type_ = Edge;
  }
  return pins;
}

std::vector<odb::dbBox*> PadDirectConnectionStraps::getPinsFormingRing()
{
  auto pins_by_layer = getPinsByLayer();

  std::vector<odb::dbBox*> pins;
  odb::dbTechLayer* top_layer = nullptr;
  for (const auto& [layer, layer_pins] : getPinsByLayer()) {
    bool use_pins = false;
    if (top_layer == nullptr) {
      use_pins = true;
    } else {
      if (top_layer->getRoutingLevel() < layer->getRoutingLevel()) {
        use_pins = true;
      }
    }
    if (use_pins) {
      pins = layer_pins;
      top_layer = layer;
    }
  }

  if (top_layer == nullptr) {
    return {};
  }

  odb::dbTechLayer* routing_layer = top_layer;
  do {
    routing_layer = routing_layer->getUpperLayer();
  } while (routing_layer != nullptr && routing_layer->getRoutingLevel() == 0);

  if (routing_layer == nullptr) {
    return {};
  }

  auto* master = iterm_->getInst()->getMaster();
  for (auto* obs : master->getObstructions()) {
    auto* obs_layer = obs->getTechLayer();
    if (obs_layer == nullptr) {
      continue;
    }
    if (obs_layer->getNumber() > routing_layer->getNumber()) {
      // pins will be obstructed
      return {};
    }
  }
  for (auto* mterm : master->getMTerms()) {
    for (auto* mpin : mterm->getMPins()) {
      for (auto* geo : mpin->getGeometry()) {
        auto* geo_layer = geo->getTechLayer();
        if (geo_layer == nullptr) {
          continue;
        }
        if (geo_layer->getNumber() > routing_layer->getNumber()) {
          // pins might be obstructed
          return {};
        }
      }
    }
  }

  // remove pins that do not form a complete ring
  auto remove_itr
      = std::remove_if(pins.begin(), pins.end(), [master](odb::dbBox* box) {
          const odb::Rect rect = box->getBox();
          const bool matches_x = rect.dx() == master->getWidth()
                                 || rect.dx() == master->getHeight();
          const bool matches_y = rect.dy() == master->getWidth()
                                 || rect.dy() == master->getHeight();
          return !matches_x && !matches_y;
        });
  pins.erase(remove_itr, pins.end());

  if (pad_edge_ == odb::dbDirection::EAST
      || pad_edge_ == odb::dbDirection::WEST) {
    setDirection(odb::dbTechLayerDir::HORIZONTAL);
  } else if (pad_edge_ == odb::dbDirection::SOUTH
             || pad_edge_ == odb::dbDirection::NORTH) {
    setDirection(odb::dbTechLayerDir::VERTICAL);
  } else {
    return {};
  }
  setLayer(routing_layer);
  if (!pins.empty()) {
    type_ = OverPads;
  }
  return pins;
}

void PadDirectConnectionStraps::report() const
{
  auto* logger = getLogger();

  logger->report("  Type: {}", typeToString(type()));
  logger->report("    Pin: {}", getName());
  std::string connection_type = "Unknown";
  switch (type_) {
    case None:
      connection_type = "None";
      break;
    case Edge:
      connection_type = "Edge";
      break;
    case OverPads:
      connection_type = "Over pads";
      break;
  }
  logger->report("    Connection type: {}", connection_type);
}

std::string PadDirectConnectionStraps::getName() const
{
  return iterm_->getInst()->getName() + "/" + iterm_->getMTerm()->getName();
}

bool PadDirectConnectionStraps::isConnectHorizontal() const
{
  return pad_edge_ == odb::dbDirection::WEST
         || pad_edge_ == odb::dbDirection::EAST;
}

void PadDirectConnectionStraps::makeShapes(const ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Direct connect pin start of make shapes for {}",
             getName());
  switch (type_) {
    case None:
      break;
    case Edge:
      makeShapesFacingCore(other_shapes);
      break;
    case OverPads:
      makeShapesOverPads(other_shapes);
      break;
  }
}

ShapePtr PadDirectConnectionStraps::getClosestShape(
    const ShapeTree& search_shapes,
    const odb::Rect& pin_shape,
    odb::dbNet* net) const
{
  auto* block = iterm_->getInst()->getBlock();
  const odb::Rect die_rect = block->getDieArea();

  // generate search box
  odb::Rect search_rect = pin_shape;
  if (isConnectHorizontal()) {
    search_rect.set_xlo(die_rect.xMin());
    search_rect.set_xhi(die_rect.xMax());
  } else {
    search_rect.set_ylo(die_rect.yMin());
    search_rect.set_yhi(die_rect.yMax());
  }
  Box search = Shape::rectToBox(search_rect);

  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  if (!(is_south || is_north || is_west || is_east)) {
    return nullptr;
  }

  ShapePtr closest_shape = nullptr;
  int closest_dist = std::numeric_limits<int>::max();

  for (auto it = search_shapes.qbegin(
           bgi::intersects(search) && bgi::satisfies([&](const auto& other) {
             const auto& shape = other.second;
             return shape->getNet() == net
                    && shape->getType() == target_shapes_;
           }));
       it != search_shapes.qend();
       it++) {
    const auto& shape = it->second;
    const auto& shape_rect = shape->getRect();

    // check if shapes will make good intersections
    if (isConnectHorizontal()) {
      if (shape_rect.dy() < shape_rect.dx()) {
        // connection needs to be a vertical shape
        continue;
      }
      const odb::Rect intersect = shape_rect.intersect(search_rect);
      if (intersect.dy() != search_rect.dy()) {
        continue;
      }
    } else {
      if (shape_rect.dx() < shape_rect.dy()) {
        // connection needs to be a horizontal shape
        continue;
      }
      const odb::Rect intersect = shape_rect.intersect(search_rect);
      if (intersect.dx() != search_rect.dx()) {
        continue;
      }
    }

    // check if shape is closer than before
    int new_dist = std::numeric_limits<int>::max();
    if (is_west) {
      new_dist = shape_rect.xMin() - pin_shape.xMax();
    } else if (is_east) {
      new_dist = pin_shape.xMin() - shape_rect.xMax();
    } else if (is_south) {
      new_dist = shape_rect.yMin() - pin_shape.yMax();
    } else if (is_north) {
      new_dist = pin_shape.yMin() - shape_rect.yMax();
    } else {
      continue;
    }

    // determine if this is closer
    if (closest_dist > new_dist) {
      closest_shape = shape;
      closest_dist = new_dist;
    }
  }

  return closest_shape;
}

void PadDirectConnectionStraps::makeShapesFacingCore(
    const ShapeTreeMap& other_shapes)
{
  if (other_shapes.empty()) {
    return;
  }

  std::set<odb::dbTechLayer*> connectable_layers;
  for (const auto& [layer, shapes] : other_shapes) {
    for (const auto& [box, shape] : shapes) {
      if (shape->getType() == target_shapes_) {
        const auto layers = getGrid()->connectableLayers(layer);
        connectable_layers.insert(layers.begin(), layers.end());
      }
    }
  }

  auto* inst = iterm_->getInst();
  odb::dbTransform transform;
  inst->getTransform(transform);

  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  const bool is_horizontal_strap = isConnectHorizontal();

  auto* net = iterm_->getNet();
  for (auto* pin : pins_) {
    odb::Rect pin_rect = pin->getBox();
    transform.apply(pin_rect);

    auto* layer = pin->getTechLayer();
    if (connectable_layers.find(layer) == connectable_layers.end()) {
      // layer is not connectable to a target
      continue;
    }

    // find nearest target
    for (const auto& [search_layer, search_shape_tree] : other_shapes) {
      if (layer == search_layer) {
        continue;
      }

      ShapePtr closest_shape
          = getClosestShape(search_shape_tree, pin_rect, net);
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
      } else if (is_north) {
        shape_rect.set_ylo(closest_shape->getRect().yMin());
      } else {
        continue;
      }

      if (layer->hasMaxWidth()) {
        const int max_width = layer->getMaxWidth();
        // check max width and adjust if needed
        if (is_horizontal_strap) {
          // shape will be horizontal
          if (shape_rect.dy() > max_width) {
            // fix width to max
            shape_rect.set_yhi(shape_rect.yMin() + max_width);
          }
        } else {
          // shape will be vertical
          if (shape_rect.dx() > max_width) {
            // fix width to max
            shape_rect.set_xhi(shape_rect.xMin() + max_width);
          }
        }
      }

      auto* shape
          = new Shape(layer, net, shape_rect, odb::dbWireShapeType::STRIPE);
      // use intersection of pin_rect to ensure max width limitation is
      // preserved
      shape->addITermConnection(pin_rect.intersect(shape_rect));
      addShape(shape);
    }
  }
}

std::vector<PadDirectConnectionStraps*>
PadDirectConnectionStraps::getAssociatedStraps() const
{
  const odb::dbInst* inst = iterm_->getInst();
  std::vector<PadDirectConnectionStraps*> straps;
  for (const auto& strap : getGrid()->getStraps()) {
    if (strap->type() == GridComponent::PadConnect) {
      PadDirectConnectionStraps* pad_strap
          = dynamic_cast<PadDirectConnectionStraps*>(strap.get());
      if (pad_strap != nullptr && pad_strap->getITerm()->getInst() == inst) {
        straps.push_back(pad_strap);
      }
    }
  }

  std::sort(straps.begin(),
            straps.end(),
            [](PadDirectConnectionStraps* lhs, PadDirectConnectionStraps* rhs) {
              std::set<odb::Rect> lhs_pins;
              std::set<odb::Rect> rhs_pins;
              for (auto* box : lhs->getPins()) {
                lhs_pins.insert(box->getBox());
              }
              for (auto* box : rhs->getPins()) {
                rhs_pins.insert(box->getBox());
              }
              return *lhs_pins.begin() < *rhs_pins.begin();
            });

  return straps;
}

void PadDirectConnectionStraps::makeShapesOverPads(
    const ShapeTreeMap& other_shapes)
{
  if (other_shapes.empty()) {
    return;
  }

  auto straps = getAssociatedStraps();
  int index = std::distance(straps.begin(),
                            std::find(straps.begin(), straps.end(), this));

  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             2,
             "Pad connections for {} has {} connections and this on is at "
             "index ({}), will use {} to connect.",
             getName(),
             straps.size(),
             index,
             getLayer()->getName())

      const bool is_horizontal
      = isConnectHorizontal();

  odb::dbInst* inst = iterm_->getInst();
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  odb::dbTransform transform;
  inst->getTransform(transform);

  const int inst_width = is_horizontal ? inst_rect.dy() : inst_rect.dx();
  const int inst_offset = is_horizontal ? inst_rect.yMin() : inst_rect.xMin();

  const int max_width = inst_width / (2 * (straps.size() + 1));
  TechLayer layer(getLayer());
  const int target_width = layer.snapToManufacturingGrid(max_width, false);
  if (target_width < layer.getMinWidth()) {
    // dont build anything
    debugPrint(
        getLogger(),
        utl::PDN,
        "Pad",
        3,
        "Skipping because strap would be {} and needed to be atleast {}.",
        layer.dbuToMicron(target_width),
        layer.dbuToMicron(layer.getMinWidth()));
    return;
  }
  setWidth(std::min(target_width, layer.getMaxWidth()));
  setSpacing(std::max(getWidth(), layer.getSpacing(getWidth())));
  Straps::checkLayerSpecifications();

  const int target_offset = layer.snapToManufacturingGrid(
      inst_offset + getSpacing() + getWidth() / 2, false);
  const int offset = target_offset + index * (getSpacing() + getWidth());

  odb::Rect pin_shape;
  pin_shape.mergeInit();
  for (auto* pin : pins_) {
    pin_shape.merge(pin->getBox());
  }
  transform.apply(pin_shape);

  if (is_horizontal) {
    pin_shape.set_ylo(offset - getWidth() / 2);
    pin_shape.set_yhi(pin_shape.yMin() + getWidth());
  } else {
    pin_shape.set_xlo(offset - getWidth() / 2);
    pin_shape.set_xhi(pin_shape.xMin() + getWidth());
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             3,
             "Connecting using shape: {}",
             Shape::getRectText(pin_shape, layer.getLefUnits()));

  ShapePtr closest_shape = nullptr;
  for (const auto& [layer, layer_shapes] : other_shapes) {
    ShapePtr layer_closest_shape
        = getClosestShape(layer_shapes, pin_shape, iterm_->getNet());
    if (layer_closest_shape != nullptr) {
      closest_shape = layer_closest_shape;
    }
  }
  if (closest_shape == nullptr) {
    debugPrint(getLogger(), utl::PDN, "Pad", 3, "No connecting shape found.");
    return;
  }

  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  odb::Rect shape_rect = pin_shape;
  if (is_west) {
    shape_rect.set_xhi(closest_shape->getRect().xMax());
  } else if (is_east) {
    shape_rect.set_xlo(closest_shape->getRect().xMin());
  } else if (is_south) {
    shape_rect.set_yhi(closest_shape->getRect().yMax());
  } else if (is_north) {
    shape_rect.set_ylo(closest_shape->getRect().yMin());
  } else {
    return;
  }

  auto* shape = new Shape(
      getLayer(), iterm_->getNet(), shape_rect, odb::dbWireShapeType::STRIPE);

  if (getDirection() != getLayer()->getDirection()) {
    shape->setAllowsNonPreferredDirectionChange();
  }
  addShape(shape);
}

void PadDirectConnectionStraps::getConnectableShapes(ShapeTreeMap& shapes) const
{
  if (type_ != OverPads) {
    return;
  }

  odb::dbTechLayer* pin_layer = pins_[0]->getTechLayer();
  auto& pin_shapes = shapes[pin_layer];

  for (const auto& [layer, layer_pins] :
       InstanceGrid::getInstancePins(iterm_->getInst())) {
    if (layer != pin_layer) {
      continue;
    }

    for (const auto& [box, shape] : layer_pins) {
      if (shape->getNet() != iterm_->getNet()) {
        continue;
      }

      pin_shapes.insert({box, shape});
    }
  }
}

void PadDirectConnectionStraps::cutShapes(const ShapeTreeMap& obstructions)
{
  Straps::cutShapes(obstructions);

  if (type_ != OverPads) {
    return;
  }

  odb::dbInst* inst = iterm_->getInst();
  const odb::Rect inst_shape = inst->getBBox()->getBox();

  // filter out segments that are full enclosed by the pin shape (ie doesnt
  // connect to ring)
  std::vector<Shape*> remove_shapes;
  for (const auto& [layer, layer_shapes] : getShapes()) {
    for (const auto& entry : layer_shapes) {
      const auto& shape = entry.second;
      if (inst_shape.contains(shape->getRect())) {
        remove_shapes.push_back(shape.get());
      }
    }
  }

  // remove shapes
  for (auto* shape : remove_shapes) {
    removeShape(shape);
  }
}

void PadDirectConnectionStraps::unifyConnectionTypes(
    const std::set<PadDirectConnectionStraps*>& straps)
{
  std::set<ConnectionType> types;
  for (auto* strap : straps) {
    if (strap->getConnectionType() == None) {
      continue;
    }
    types.insert(strap->getConnectionType());
  }

  ConnectionType global_connection = None;
  if (types.size() == 1) {
    global_connection = *types.begin();
  } else {
    // Multiple methods found, pick Edge
    global_connection = Edge;
  }

  for (auto* strap : straps) {
    strap->setConnectionType(global_connection);
  }
}

void PadDirectConnectionStraps::setConnectionType(ConnectionType type)
{
  if (type_ == type) {
    return;
  }

  initialize(type);
}

////////

RepairChannelStraps::RepairChannelStraps(Grid* grid,
                                         Straps* target,
                                         odb::dbTechLayer* connect_to,
                                         const ShapeTreeMap& other_shapes,
                                         const std::set<odb::dbNet*>& nets,
                                         const odb::Rect& area,
                                         const odb::Rect& obs_check_area)
    : Straps(grid,
             target->getLayer(),
             target->getWidth(),
             0,
             target->getSpacing(),
             1),
      nets_(nets),
      connect_to_(connect_to),
      area_(area),
      obs_check_area_(obs_check_area)
{
  // use snap to grid
  setSnapToGrid(true);

  // set start and end points of straps to match area
  if (isHorizontal()) {
    setStrapStartEnd(area_.xMin(), area_.xMax());
  } else {
    setStrapStartEnd(area_.yMin(), area_.yMax());
  }

  determineParameters(other_shapes);

  if (invalid_) {
    const TechLayer layer(getLayer());
    debugPrint(getLogger(),
               utl::PDN,
               "Channel",
               1,
               "Cannot repair channel {} on {} with straps on {} for: {}",
               Shape::getRectText(area_, layer.getLefUnits()),
               connect_to_->getName(),
               layer.getName(),
               getNetString());
  }
}

std::string RepairChannelStraps::getNetString() const
{
  std::string nets;

  for (auto* net : nets_) {
    if (!nets.empty()) {
      nets += ", ";
    }
    nets += net->getName();
  }

  return nets;
}

int RepairChannelStraps::getMaxLength() const
{
  const odb::Rect core = getGrid()->getDomainArea();
  if (isHorizontal()) {
    return core.dx();
  }
  return core.dy();
}

bool RepairChannelStraps::isAtEndOfRepairOptions() const
{
  const TechLayer layer(getLayer());
  if (getWidth() != layer.getMinWidth()) {
    return false;
  }

  if (getSpacing() > layer.getSpacing(getWidth(), getMaxLength())) {
    return false;
  }

  return true;
}

void RepairChannelStraps::continueRepairs(const ShapeTreeMap& other_shapes)
{
  clearShapes();
  const int next_width = getNextWidth();
  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      1,
      "Continue repair at {} on {} with straps on {} for {}: changing width "
      "from {} um to {} um",
      Shape::getRectText(area_, getBlock()->getDbUnitsPerMicron()),
      connect_to_->getName(),
      getLayer()->getName(),
      getNetString(),
      getWidth() / static_cast<double>(getBlock()->getDbUnitsPerMicron()),
      next_width / static_cast<double>(getBlock()->getDbUnitsPerMicron()));
  setWidth(next_width);
  determineParameters(other_shapes);
}

void RepairChannelStraps::determineParameters(const ShapeTreeMap& obstructions)
{
  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      1,
      "Determining channel parameters for {} on {} with straps on {} for {}",
      Shape::getRectText(area_, getBlock()->getDbUnitsPerMicron()),
      connect_to_->getName(),
      getLayer()->getName(),
      getNetString());
  const int max_length = getMaxLength();
  int area_width = 0;
  if (isHorizontal()) {
    area_width = area_.dy();
  } else {
    area_width = area_.dx();
  }

  auto check = [&]() -> bool {
    const int group_width = getStrapGroupWidth();
    if (group_width > area_width) {
      debugPrint(
          getLogger(),
          utl::PDN,
          "Channel",
          2,
          "Failed on channel width check, group {:.4f} and channel {.4f}.",
          group_width / static_cast<double>(getBlock()->getDbUnitsPerMicron()),
          area_width / static_cast<double>(getBlock()->getDbUnitsPerMicron()));
      return false;
    }
    const bool done = determineOffset(obstructions);
    debugPrint(
        getLogger(), utl::PDN, "Channel", 2, "Determine offset: {}", done);
    return done;
  };

  if (check()) {
    // straps already fit
    return;
  }

  const TechLayer layer(getLayer());

  // adjust spacing first to minimum spacing for the given width
  setSpacing(layer.getSpacing(getWidth(), max_length));

  debugPrint(getLogger(),
             utl::PDN,
             "Channel",
             2,
             "Adjust spacing to {:.4f} um.",
             layer.dbuToMicron(getSpacing()));
  if (check()) {
    // adjusting spacing works
    return;
  }

  const int min_width = layer.getMinWidth();
  while (getWidth() > min_width) {
    // adjust the width and spacing until something works
    const int new_width = getNextWidth();
    setWidth(new_width);
    setSpacing(layer.getSpacing(new_width, max_length));

    debugPrint(getLogger(),
               utl::PDN,
               "Channel",
               2,
               "Adjust width to {:.4f} um and spacing to {:.4f} um.",
               layer.dbuToMicron(getWidth()),
               layer.dbuToMicron(getSpacing()));
    if (check()) {
      return;
    }
  }

  // unable to determine how to place repair straps
  invalid_ = true;
}

bool RepairChannelStraps::determineOffset(const ShapeTreeMap& obstructions,
                                          int extra_offset,
                                          int bisect_dist,
                                          int level)
{
  const TechLayer layer(getLayer());
  const bool is_horizontal = isHorizontal();
  const int group_width = getStrapGroupWidth();
  int offset = -group_width / 2 + extra_offset;
  if (is_horizontal) {
    offset += 0.5 * (area_.yMin() + area_.yMax());
  } else {
    offset += 0.5 * (area_.xMin() + area_.xMax());
  }
  const int half_width = getWidth() / 2;
  offset += half_width;
  offset = layer.snapToManufacturingGrid(offset);

  odb::Rect estimated_straps;
  const int strap_start = offset - half_width;
  if (is_horizontal) {
    estimated_straps = odb::Rect(
        area_.xMin(), strap_start, area_.xMax(), strap_start + group_width);
  } else {
    estimated_straps = odb::Rect(
        strap_start, area_.yMin(), strap_start + group_width, area_.yMax());
  }

  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      3,
      "Estimating strap to be {}.",
      Shape::getRectText(estimated_straps, getBlock()->getDbUnitsPerMicron()));

  // check if straps will fit
  if (is_horizontal) {
    if (estimated_straps.dy() > area_.dy()) {
      return false;
    }
  } else {
    if (estimated_straps.dx() > area_.dx()) {
      return false;
    }
  }

  // check if straps will intersect anything on the layers below
  std::vector<odb::dbTechLayer*> check_layers;
  for (odb::dbTechLayer* next_layer = connect_to_->getUpperLayer();
       next_layer != getLayer();) {
    if (next_layer->getRoutingLevel() != 0) {
      check_layers.push_back(next_layer);
    }

    next_layer = next_layer->getUpperLayer();
  }
  check_layers.push_back(getLayer());

  Shape estimated_shape(getLayer(), estimated_straps, Shape::SHAPE);
  estimated_shape.generateObstruction();

  bool has_obs = false;
  odb::Rect obs_check = estimated_shape.getObstruction();
  if (is_horizontal) {
    obs_check.set_xlo(obs_check_area_.xMin());
    obs_check.set_xhi(obs_check_area_.xMax());
  } else {
    obs_check.set_ylo(obs_check_area_.yMin());
    obs_check.set_yhi(obs_check_area_.yMax());
  }
  auto check_obstructions = [&obs_check](const ShapeTree& shapes) -> bool {
    for (auto itr = shapes.qbegin(bgi::intersects(Shape::rectToBox(obs_check)));
         itr != shapes.qend();
         itr++) {
      const auto& shape = itr->second;
      if (obs_check.overlaps(shape->getObstruction())) {
        return true;
      }
    }
    return false;
  };
  for (auto* layer : check_layers) {
    if (obstructions.count(layer) != 0) {
      // obstructions possible on this layer
      const auto& shapes = obstructions.at(layer);
      if (check_obstructions(shapes)) {
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
      const int width = is_horizontal ? area_.dy() : area_.dx();
      new_bisect_dist = width / 4;
    } else {
      // not first time, so search half the distance of the current bisection.
      new_bisect_dist = bisect_dist / 2;
    }
    new_bisect_dist = layer.snapToManufacturingGrid(new_bisect_dist);

    if (new_bisect_dist == 0) {
      // no more searches possible
      return false;
    }

    debugPrint(getLogger(),
               utl::PDN,
               "ChannelBisect",
               1,
               "Current level {}, offset {:.4f} um, group width {:.4f} um.",
               level,
               layer.dbuToMicron(extra_offset),
               layer.dbuToMicron(group_width));
    debugPrint(getLogger(),
               utl::PDN,
               "ChannelBisect",
               2,
               "  Bisect distance {:.4f} um, low {:.4f} um, high {:.4f} um.",
               layer.dbuToMicron(new_bisect_dist),
               layer.dbuToMicron(extra_offset - new_bisect_dist),
               layer.dbuToMicron(extra_offset + new_bisect_dist));
    // check lower range first
    if (determineOffset(obstructions,
                        extra_offset - new_bisect_dist,
                        new_bisect_dist,
                        level + 1)) {
      return true;
    }
    // check higher range next
    if (determineOffset(obstructions,
                        extra_offset + new_bisect_dist,
                        new_bisect_dist,
                        level + 1)) {
      return true;
    }

    // no offset found
    return false;
  }

  const odb::Rect core = getGrid()->getDomainArea();
  if (is_horizontal) {
    offset -= core.yMin();
  } else {
    offset -= core.xMin();
  }

  // apply offset found
  setOffset(offset);

  return checkLayerOffsetSpecification(false);
}

std::vector<odb::dbNet*> RepairChannelStraps::getNets() const
{
  std::vector<odb::dbNet*> nets(nets_.begin(), nets_.end());

  return nets;
}

void RepairChannelStraps::cutShapes(const ShapeTreeMap& obstructions)
{
  Straps::cutShapes(obstructions);

  // filter out cut segments that do not overlap with the area
  std::vector<Shape*> remove_shapes;
  for (const auto& [layer, layer_shapes] : getShapes()) {
    for (const auto& entry : layer_shapes) {
      const auto& shape = entry.second;
      if (!shape->getRect().intersects(obs_check_area_)) {
        remove_shapes.push_back(shape.get());
      }
    }
  }

  // remove shapes
  for (auto* shape : remove_shapes) {
    removeShape(shape);
  }

  // determine min/max
  odb::Rect region;
  region.mergeInit();
  for (const auto& [layer, layer_shapes] : getShapes()) {
    for (const auto& [box, shape] : layer_shapes) {
      region.merge(shape->getRect());
    }
  }
  if (isHorizontal()) {
    setStrapStartEnd(region.xMin(), region.xMax());
  } else {
    setStrapStartEnd(region.yMin(), region.yMax());
  }
}

void RepairChannelStraps::report() const
{
  Straps::report();

  auto* block = getGrid()->getBlock();
  auto* logger = getLogger();

  logger->report("    Repair area: {}",
                 Shape::getRectText(area_, block->getDbUnitsPerMicron()));
  logger->report("    Nets: {}", getNetString());
}

Straps* RepairChannelStraps::getTargetStrap(Grid* grid, odb::dbTechLayer* layer)
{
  std::set<odb::dbTechLayer*> connects_to;
  for (const auto& connect : grid->getConnect()) {
    if (connect->getLowerLayer() == layer) {
      connects_to.insert(connect->getUpperLayer());
    }
  }

  if (connects_to.empty()) {
    return nullptr;
  }

  Straps* lowest_target = nullptr;

  for (const auto& strap : grid->getStraps()) {
    if (strap->type() != GridComponent::Strap) {
      continue;
    }

    if (connects_to.find(strap->getLayer()) != connects_to.end()) {
      if (lowest_target == nullptr) {
        lowest_target = strap.get();
      } else {
        if (strap->getLayer()->getNumber()
            < lowest_target->getLayer()->getNumber()) {
          lowest_target = strap.get();
        }
      }
    }
  }

  debugPrint(grid->getLogger(),
             utl::PDN,
             "Channel",
             2,
             "Target strap for repair on layer {} is on layer {}.",
             layer->getName(),
             lowest_target == nullptr ? "\"not found\""
                                      : lowest_target->getLayer()->getName());

  return lowest_target;
}

odb::dbTechLayer* RepairChannelStraps::getHighestStrapLayer(Grid* grid)
{
  odb::dbTechLayer* highest_layer = nullptr;
  for (const auto& strap : grid->getStraps()) {
    if (strap->type() != GridComponent::Strap) {
      // only look for straps
      continue;
    }
    auto* strap_layer = strap->getLayer();
    if (highest_layer == nullptr) {
      highest_layer = strap_layer;
    } else {
      if (strap_layer == nullptr) {
        continue;
      }

      if (strap_layer->getRoutingLevel() > highest_layer->getRoutingLevel()) {
        highest_layer = strap_layer;
      }
    }
  }

  debugPrint(
      grid->getLogger(),
      utl::PDN,
      "Channel",
      2,
      "Highest strap layer in grid {} is: {}.",
      grid->getName(),
      highest_layer == nullptr ? "\"not found\"" : highest_layer->getName());

  return highest_layer;
}

std::vector<RepairChannelStraps::RepairChannelArea>
RepairChannelStraps::findRepairChannels(Grid* grid,
                                        const ShapeTree& shapes,
                                        odb::dbTechLayer* layer)
{
  Straps* target = getTargetStrap(grid, layer);
  if (target == nullptr) {
    // no possible connect statements for this layer
    return {};
  }

  using namespace boost::polygon::operators;
  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  const auto grid_core = grid->getDomainBoundary();

  std::vector<Shape*> shapes_used;
  Polygon90Set shape_set;
  for (const auto& [box, shape] : shapes) {
    if (shape->getNumberOfConnectionsAbove() != 0) {
      // shape already connected to something
      continue;
    }
    auto* grid_compomponent = shape->getGridComponent();
    if (grid_compomponent->type() != GridComponent::Strap
        && grid_compomponent->type() != GridComponent::Followpin) {
      // only attempt to repair straps and followpins
      continue;
    }

    if (grid_compomponent->type() == GridComponent::Strap) {
      if (shape->getNumberOfConnections() == 0) {
        // strap is floating and will be removed
        continue;
      }
    }

    auto* grid_strap = dynamic_cast<Straps*>(grid_compomponent);
    if (grid_strap == nullptr) {
      continue;
    }

    // determine bloat factor
    const int bloat = grid_strap->getPitch();
    const int bloat_x = grid_strap->isHorizontal() ? 0 : bloat;
    const int bloat_y = grid_strap->isHorizontal() ? bloat : 0;

    const auto& min_corner = box.min_corner();
    const auto& max_corner = box.max_corner();
    std::array<Pt, 4> pts
        = {Pt(min_corner.x() - bloat_x, min_corner.y() - bloat_y),
           Pt(max_corner.x() + bloat_x, min_corner.y() - bloat_y),
           Pt(max_corner.x() + bloat_x, max_corner.y() + bloat_y),
           Pt(min_corner.x() - bloat_x, max_corner.y() + bloat_y)};
    Polygon90 poly;
    poly.set(pts.begin(), pts.end());

    shapes_used.push_back(shape.get());
    shape_set.insert(poly);
  }

  // get all possible channel rects
  std::set<odb::Rect> channels_rects;
  std::vector<Rectangle> channel_set;
  shape_set.get_rectangles(channel_set);
  for (const auto& channel : channel_set) {
    const odb::Rect area(xl(channel), yl(channel), xh(channel), yh(channel));

    if (area.intersects(grid_core)) {
      channels_rects.insert(area.intersect(grid_core));
    }
  }

  // setup channels with information needed to build
  std::vector<RepairChannelArea> channels;
  for (const auto& area : channels_rects) {
    RepairChannelArea channel{area, odb::Rect(), target, layer, {}};
    channel.obs_area.mergeInit();

    int followpin_count = 0;
    int strap_count = 0;
    // find all the nets in a given repair area
    for (auto* shape : shapes_used) {
      const auto& shape_rect = shape->getRect();
      if (channel.area.overlaps(shape_rect)) {
        channel.area.merge(shape_rect);
        channel.obs_area.merge(shape_rect);
        channel.nets.insert(shape->getNet());
        if (shape->getType() == odb::dbWireShapeType::FOLLOWPIN) {
          followpin_count++;
        } else {
          strap_count++;
        }
      }
    }

    // all followpins must be repaired
    const bool channel_has_followpin = followpin_count >= 1;
    // single straps can be skipped
    const bool channel_has_more_than_one_strap = strap_count > 1;
    if (channel_has_followpin || channel_has_more_than_one_strap) {
      if (!channel.area.intersects(grid_core)) {
        // channel is not in the core
        continue;
      }

      // ensure areas are inside the core
      channel.area = channel.area.intersect(grid_core);
      channel.obs_area = channel.obs_area.intersect(grid_core);

      channels.push_back(channel);
    }
  }

  return channels;
}

std::vector<RepairChannelStraps::RepairChannelArea>
RepairChannelStraps::findRepairChannels(Grid* grid)
{
  odb::dbTechLayer* highest_layer = getHighestStrapLayer(grid);

  if (highest_layer == nullptr) {
    // no repairs possible
    return {};
  }

  std::vector<RepairChannelArea> channels;
  for (const auto& [layer, shapes] : grid->getShapes()) {
    if (highest_layer == layer) {
      // stop there are no possible connections above
      break;
    }

    auto layerchannels = findRepairChannels(grid, shapes, layer);
    for (const auto& channel : layerchannels) {
      channels.push_back(channel);
    }
  }

  return channels;
}

bool RepairChannelStraps::testBuild(const ShapeTreeMap& local_shapes,
                                    const ShapeTreeMap& obstructions)
{
  makeShapes(local_shapes);
  cutShapes(obstructions);
  return !isEmpty();
}

void RepairChannelStraps::repairGridChannels(Grid* grid,
                                             const ShapeTreeMap& global_shapes,
                                             ShapeTreeMap& obstructions,
                                             bool allow)
{
  // create copy of shapes so they can be used to determine repair locations
  ShapeTreeMap local_shapes = global_shapes;

  std::set<odb::Rect> areas_repaired;

  const auto channels = findRepairChannels(grid);
  debugPrint(grid->getLogger(),
             utl::PDN,
             "Channel",
             1,
             "Channels to repair {}.",
             channels.size());

  // check for recurring channels
  for (const auto& channel : channels) {
    for (const auto& strap : grid->getStraps()) {
      if (strap->type() == GridComponent::RepairChannel) {
        RepairChannelStraps* repair_strap
            = dynamic_cast<RepairChannelStraps*>(strap.get());
        if (repair_strap == nullptr) {
          continue;
        }
        if (repair_strap->getLayer() == channel.target->getLayer()
            && channel.area == repair_strap->getArea()) {
          if (!repair_strap->isAtEndOfRepairOptions()) {
            repair_strap->addNets(channel.nets);
            repair_strap->removeShapes(local_shapes);
            repair_strap->removeObstructions(obstructions);
            repair_strap->continueRepairs(obstructions);

            if (repair_strap->testBuild(local_shapes, obstructions)) {
              strap->getShapes(local_shapes);  // need new shapes
              strap->getObstructions(obstructions);
              areas_repaired.insert(channel.area);
            }
          }
        }
      }
    }
  }
  for (const auto& channel : channels) {
    bool dont_repair = false;
    for (const auto& other : areas_repaired) {
      if (!(channel.area.xMax() <= other.xMin()
            || channel.area.xMin() >= other.xMax())) {
        // channels cover similar x regions
        dont_repair = true;
        break;
      }
      if (!(channel.area.yMax() <= other.yMin()
            || channel.area.yMin() >= other.yMax())) {
        // channels cover similar y regions
        dont_repair = true;
        break;
      }
    }
    if (dont_repair) {
      debugPrint(grid->getLogger(),
                 utl::PDN,
                 "Channel",
                 1,
                 "Skipping repair at {} in {}.",
                 Shape::getRectText(channel.area,
                                    grid->getBlock()->getDbUnitsPerMicron()),
                 channel.target->getLayer()->getName());
      continue;
    }

    // create strap repair channel
    auto strap = std::make_unique<RepairChannelStraps>(grid,
                                                       channel.target,
                                                       channel.connect_to,
                                                       obstructions,
                                                       channel.nets,
                                                       channel.area,
                                                       channel.obs_area);

    if (!strap->isRepairValid()) {
      continue;
    }

    // build strap
    bool built_straps = strap->testBuild(local_shapes, obstructions);
    if (!built_straps) {
      if (!strap->isAtEndOfRepairOptions()) {
        // try to build the straps with next set of options (width / spacing)
        strap->continueRepairs(obstructions);
        built_straps = strap->testBuild(local_shapes, obstructions);
      }
    }
    if (!built_straps) {
      // try to build the straps without snapping to grid
      strap->setSnapToGrid(false);
      built_straps = strap->testBuild(local_shapes, obstructions);
    }

    if (!built_straps) {
      // nothing was built so move on
      continue;
    }

    strap->getShapes(local_shapes);  // need new shapes
    strap->getObstructions(obstructions);

    grid->addStrap(std::move(strap));
    areas_repaired.insert(channel.area);
  }

  // make the vias
  if (!areas_repaired.empty()) {
    grid->makeVias(global_shapes, obstructions);
  }

  if (channels.size() != areas_repaired.size() && !areas_repaired.empty()) {
    // channels were skipped so, try again
    repairGridChannels(grid, global_shapes, obstructions, allow);
  } else {
    const auto remaining_channels = findRepairChannels(grid);
    if (!remaining_channels.empty()) {
      // if channels remain, report them and generate error
      const double dbu_to_microns = grid->getBlock()->getDbUnitsPerMicron();
      for (const auto& channel : remaining_channels) {
        std::string nets;
        for (auto* net : channel.nets) {
          if (!nets.empty()) {
            nets += ", ";
          }
          nets += net->getName();
        }
        grid->getLogger()->warn(
            utl::PDN,
            178,
            "Remaining channel {} on {} for nets: {}",
            Shape::getRectText(channel.area, dbu_to_microns),
            channel.connect_to->getName(),
            nets);
      }
      if (!allow) {
        grid->getLogger()->error(
            utl::PDN, 179, "Unable to repair all channels.");
      }
    }
  }
}

int RepairChannelStraps::getNextWidth() const
{
  const TechLayer layer(getLayer());

  int new_width = layer.snapToManufacturingGrid(getWidth() / 2);

  // if new is smaller than min width use min width
  const int min_width = layer.getMinWidth();
  if (new_width <= min_width) {
    return min_width;
  }

  // check if width tables apply and use those
  for (auto* rule : layer.getLayer()->getTechLayerWidthTableRules()) {
    if (rule->isWrongDirection()) {
      if (getDirection() == layer.getLayer()->getDirection()) {
        continue;
      }
    } else {
      if (getDirection() != layer.getLayer()->getDirection()) {
        continue;
      }
    }

    const auto widths = rule->getWidthTable();
    if (!widths.empty()) {
      if (new_width > widths.back()) {
        continue;
      }

      int use_width = 0;
      for (int width : widths) {
        if (width <= new_width) {
          use_width = width;
        }
      }
      new_width = use_width;
    }
  }

  return new_width;
}

bool RepairChannelStraps::isEmpty() const
{
  const auto& shapes = getShapes();

  if (shapes.empty()) {
    return true;
  }

  for (const auto& [layer, layer_shapes] : shapes) {
    if (!layer_shapes.empty()) {
      return false;
    }
  }

  return true;
}

}  // namespace pdn
