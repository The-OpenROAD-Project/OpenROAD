// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "straps.h"

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "connect.h"
#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "pdn/PdnGen.hh"
#include "renderer.h"
#include "shape.h"
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
  odb::Rect grid_area = getGrid()->getDomainArea();
  if (allow_out_of_core_) {
    const odb::Rect die = getGrid()->getBlock()->getDieArea();
    grid_area.set_xhi(die.xMax());
    grid_area.set_yhi(die.yMax());
  }
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
          "Insufficient width ({:.2f} um) to add straps on layer {} in grid "
          "\"{}\" "
          "with total strap width {:.1f} um and offset {:.1f} um.",
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

void Straps::makeShapes(const Shape::ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Strap start of make shapes for on layer {}",
             layer_->getName());
  clearShapes();

  auto* grid = getGrid();

  const odb::Rect die = grid->getBlock()->getDieArea();
  odb::Rect boundary;
  const odb::Rect core = grid->getDomainArea();
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
  Shape::ObstructionTree avoid;
  if (other_shapes.contains(layer_)) {
    for (const auto& shape : other_shapes.at(layer_)) {
      if (shape->getType() == odb::dbWireShapeType::RING) {
        // avoid ring shapes
        avoid.insert(shape);
      }
    }
  }

  debugPrint(
      getLogger(),
      utl::PDN,
      "Straps",
      1,
      "Make straps on {} / horizontal {} / die {} / core {} / boundary {}",
      layer_->getName(),
      isHorizontal(),
      Shape::getRectText(die, layer.getLefUnits()),
      Shape::getRectText(core, layer.getLefUnits()),
      Shape::getRectText(boundary, layer.getLefUnits()));

  if (isHorizontal()) {
    const int x_start = boundary.xMin();
    const int x_end = boundary.xMax();

    const int abs_min = die.yMin();
    const int abs_max = die.yMax();

    makeStraps(x_start,
               core.yMin(),
               x_end,
               allow_out_of_core_ ? die.yMax() : core.yMax(),
               abs_min,
               abs_max,
               false,
               layer,
               avoid);
  } else {
    const int y_start = boundary.yMin();
    const int y_end = boundary.yMax();

    const int abs_min = die.xMin();
    const int abs_max = die.xMax();

    makeStraps(core.xMin(),
               y_start,
               allow_out_of_core_ ? die.xMax() : core.xMax(),
               y_end,
               abs_min,
               abs_max,
               true,
               layer,
               avoid);
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Straps",
             1,
             "Generated {} straps on {}",
             getShapeCount(),
             layer_->getName());
}

void Straps::makeStraps(int x_start,
                        int y_start,
                        int x_end,
                        int y_end,
                        int abs_start,
                        int abs_end,
                        bool is_delta_x,
                        const TechLayer& layer,
                        const Shape::ObstructionTree& avoid)
{
  const int half_width = width_ / 2;
  int strap_count = 0;

  int pos = is_delta_x ? x_start : y_start;
  const int pos_end = is_delta_x ? x_end : y_end;

  const auto nets = getNets();

  const int group_pitch = spacing_ + width_;

  debugPrint(getLogger(),
             utl::PDN,
             "Straps",
             2,
             "Generating straps on {} from ({:.4f}, {:.4f}) to ({:.4f}, "
             "{:.4f}) with an {}-offset of {:.4f} and must be within {:.4f} "
             "and {:.4f}",
             layer_->getName(),
             layer.dbuToMicron(x_start),
             layer.dbuToMicron(y_start),
             layer.dbuToMicron(x_end),
             layer.dbuToMicron(y_end),
             is_delta_x ? "x" : "y",
             layer.dbuToMicron(offset_),
             layer.dbuToMicron(abs_start),
             layer.dbuToMicron(abs_end));

  int next_minimum_track = std::numeric_limits<int>::lowest();
  for (pos += offset_; pos <= pos_end; pos += pitch_) {
    int group_pos = pos;
    for (auto* net : nets) {
      // snap to grid if needed
      const int org_group_pos = group_pos;
      group_pos = layer.snapToGrid(org_group_pos, next_minimum_track);
      const int strap_start = group_pos - half_width;
      const int strap_end = strap_start + width_;
      debugPrint(getLogger(),
                 utl::PDN,
                 "Straps",
                 3,
                 "Snapped from {:.4f} -> {:.4f} resulting in strap from {:.4f} "
                 "to {:.4f}",
                 layer.dbuToMicron(org_group_pos),
                 layer.dbuToMicron(group_pos),
                 layer.dbuToMicron(strap_start),
                 layer.dbuToMicron(strap_end));

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

      if (avoid.qbegin(bgi::intersects(strap_rect)) != avoid.qend()) {
        // dont add this strap as it intersects an avoidance
        continue;
      }

      if (is_delta_x) {
        if (strap_rect.xMin() < abs_start || strap_rect.xMax() > abs_end) {
          continue;
        }
      } else {
        if (strap_rect.yMin() < abs_start || strap_rect.yMax() > abs_end) {
          continue;
        }
      }

      addShape(std::make_unique<Shape>(
          layer_, net, strap_rect, odb::dbWireShapeType::STRIPE));
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
  if (getNets() != getGrid()->getNets()) {
    logger->report("    Nets: {}", getNetString());
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

std::string Straps::getNetString() const
{
  std::string nets;

  for (auto* net : getNets()) {
    if (!nets.empty()) {
      nets += ", ";
    }
    nets += net->getName();
  }

  return nets;
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

void FollowPins::makeShapes(const Shape::ShapeTreeMap& other_shapes)
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

    auto power_strap = std::make_unique<FollowPinShape>(
        layer, power, odb::Rect(x0, power_y_bot, x1, power_y_bot + width));
    power_strap->addRow(row);
    addShape(std::move(power_strap));

    auto ground_strap = std::make_unique<FollowPinShape>(
        layer, ground, odb::Rect(x0, ground_y_bot, x1, ground_y_bot + width));
    ground_strap->addRow(row);
    addShape(std::move(ground_strap));
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
  return pad_edge_ != odb::dbDirection::NONE && !pins_.empty()
         && type_ != ConnectionType::None;
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
    case ConnectionType::None:
      pins_ = getPinsFacingCore();
      if (pins_.empty()) {
        // check if pins are accessible from above
        pins_ = getPinsFormingRing();
      }
      break;
    case ConnectionType::Edge:
      pins_ = getPinsFacingCore();
      break;
    case ConnectionType::OverPads:
      pins_ = getPinsFormingRing();
      break;
  }

  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             2,
             "{} has {} pins",
             getName(),
             pins_.size());
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

  auto pins_by_layer = getPinsByLayer();
  if (!layers_.empty()) {
    // remove unspecified layers
    for (auto itr = pins_by_layer.begin(); itr != pins_by_layer.end();) {
      auto layer = itr->first;
      if (std::ranges::find(layers_, layer) == layers_.end()) {
        // remove layer
        itr = pins_by_layer.erase(itr);
      } else {
        itr++;
      }
    }
  }

  const odb::dbTransform transform = inst->getTransform();

  // remove all pins that do not face the core
  std::function<bool(odb::dbBox*)> remove_func;
  if (is_north) {
    remove_func = [&inst_rect, &transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.yMin() < box_rect.yMin();
    };
  } else if (is_south) {
    remove_func = [&inst_rect, &transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.yMax() > box_rect.yMax();
    };
  } else if (is_west) {
    remove_func = [&inst_rect, &transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.xMax() > box_rect.xMax();
    };
  } else {
    remove_func = [&inst_rect, &transform](odb::dbBox* box) {
      odb::Rect box_rect = box->getBox();
      transform.apply(box_rect);
      return inst_rect.xMin() < box_rect.xMin();
    };
  }

  for (auto& [layer, layerpins] : pins_by_layer) {
    std::erase_if(layerpins, remove_func);
  }

  for (auto itr = pins_by_layer.begin(); itr != pins_by_layer.end();) {
    if (itr->second.empty()) {
      // remove empty layer
      itr = pins_by_layer.erase(itr);
    } else {
      itr++;
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

  std::vector<odb::dbBox*> pins;
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

  if (!pins.empty()) {
    type_ = ConnectionType::Edge;
  }
  return pins;
}

std::vector<odb::dbBox*> PadDirectConnectionStraps::getPinsFormingRing()
{
  auto pins_by_layer = getPinsByLayer();

  // cleanup pins

  // remove pins that do not form a complete ring
  auto* master = iterm_->getInst()->getMaster();
  auto remove_filter = [&master](odb::dbBox* box) {
    const odb::Rect rect = box->getBox();
    const bool matches_x
        = rect.dx() == master->getWidth() || rect.dx() == master->getHeight();
    const bool matches_y
        = rect.dy() == master->getWidth() || rect.dy() == master->getHeight();
    return !matches_x && !matches_y;
  };
  for (auto& [layer, layer_pins] : pins_by_layer) {
    std::erase_if(layer_pins, remove_filter);
  }

  std::vector<odb::dbBox*> pins;
  odb::dbTechLayer* top_layer = nullptr;
  for (const auto& [layer, layer_pins] : pins_by_layer) {
    if (layer_pins.empty()) {
      continue;
    }

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
        if (geo_layer->getNumber() == routing_layer->getNumber()) {
          for (auto* pin : pins) {
            if (pin->getBox().intersects(geo->getBox())) {
              // pins will be obstructed
              return {};
            }
          }
        }
      }
    }
  }

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
    type_ = ConnectionType::OverPads;
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
    case ConnectionType::None:
      connection_type = "None";
      break;
    case ConnectionType::Edge:
      connection_type = "Edge";
      break;
    case ConnectionType::OverPads:
      connection_type = "Over pads";
      break;
  }
  logger->report("    Connection type: {}", connection_type);
  if (type_ == ConnectionType::Edge) {
    logger->report("    Edge: {}", pad_edge_.getString());
  }
  logger->report("    Net: {}", iterm_->getNet()->getName());
}

std::string PadDirectConnectionStraps::getName() const
{
  return iterm_->getName();
}

bool PadDirectConnectionStraps::isConnectHorizontal() const
{
  return pad_edge_ == odb::dbDirection::WEST
         || pad_edge_ == odb::dbDirection::EAST;
}

void PadDirectConnectionStraps::makeShapes(
    const Shape::ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Direct connect pin start of make shapes for {}",
             getName());
  target_shapes_.clear();
  target_pin_shape_.clear();
  switch (type_) {
    case ConnectionType::None:
      break;
    case ConnectionType::Edge:
      makeShapesFacingCore(other_shapes);
      break;
    case ConnectionType::OverPads:
      makeShapesOverPads(other_shapes);
      break;
  }
}

ShapePtr PadDirectConnectionStraps::getClosestShape(
    const Shape::ShapeTree& search_shapes,
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

  const bool is_south = pad_edge_ == odb::dbDirection::SOUTH;
  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  if (!(is_south || is_north || is_west || is_east)) {
    return nullptr;
  }

  ShapePtr closest_shape = nullptr;
  int closest_dist = std::numeric_limits<int>::max();

  for (auto it = search_shapes.qbegin(bgi::intersects(search_rect)
                                      && bgi::satisfies([&](const auto& other) {
                                           return other->getNet() == net
                                                  && isTargetShape(other.get());
                                         }));
       it != search_shapes.qend();
       it++) {
    const auto& shape = *it;
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
    const Shape::ShapeTreeMap& other_shapes)
{
  if (other_shapes.empty()) {
    return;
  }

  std::set<odb::dbTechLayer*> pin_layers;
  std::map<odb::dbTechLayer*, std::set<odb::dbTechLayer*>> connectable_layers;
  for (const auto& [layer, shapes] : other_shapes) {
    for (const auto& shape : shapes) {
      if (isTargetShape(shape.get())) {
        const auto layers = getGrid()->connectableLayers(layer);
        pin_layers.insert(layers.begin(), layers.end());
        for (auto* clayer : layers) {
          connectable_layers[clayer].insert(layer);
        }
      }
    }
  }

  auto* inst = iterm_->getInst();
  const odb::dbTransform transform = inst->getTransform();

  const bool is_horizontal_strap = isConnectHorizontal();

  auto* net = iterm_->getNet();
  for (auto* pin : pins_) {
    odb::Rect pin_rect = pin->getBox();
    transform.apply(pin_rect);

    auto* layer = pin->getTechLayer();
    if (pin_layers.find(layer) == pin_layers.end()) {
      // layer is not connectable to a target
      continue;
    }

    const auto& connect_layers = connectable_layers[layer];

    // find nearest target
    for (const auto& [search_layer, search_shape_tree] : other_shapes) {
      if (layer == search_layer) {
        continue;
      }
      if (connect_layers.find(search_layer) == connect_layers.end()) {
        // cannot connect to this layer
        continue;
      }

      ShapePtr closest_shape
          = getClosestShape(search_shape_tree, pin_rect, net);
      if (closest_shape == nullptr) {
        continue;
      }

      debugPrint(getLogger(),
                 utl::PDN,
                 "Pad",
                 2,
                 "Connect iterm {} ({}/{}) -> {}",
                 iterm_->getName(),
                 layer->getName(),
                 pin->getName(),
                 closest_shape->getReportText());

      odb::Rect shape_rect;
      if (!snapRectToClosestShape(closest_shape, pin_rect, shape_rect)) {
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

      auto shape = std::make_unique<Shape>(
          layer, net, shape_rect, odb::dbWireShapeType::STRIPE);
      // use intersection of pin_rect to ensure max width limitation is
      // preserved
      shape->addITermConnection(pin_rect.intersect(shape_rect));
      const auto added = addShape(std::move(shape));
      if (added == nullptr) {
        continue;
      }

      target_shapes_[added.get()] = closest_shape.get();
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

  std::ranges::sort(
      straps,
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
    const Shape::ShapeTreeMap& other_shapes)
{
  if (other_shapes.empty()) {
    return;
  }

  auto straps = getAssociatedStraps();
  int index = std::distance(straps.begin(), std::ranges::find(straps, this));

  debugPrint(getLogger(),
             utl::PDN,
             "Pad",
             2,
             "Pad connections for {} has {} connections and this one is at "
             "index ({}), will use {} to connect.",
             getName(),
             straps.size(),
             index,
             getLayer()->getName());

  const bool is_horizontal = isConnectHorizontal();

  odb::dbInst* inst = iterm_->getInst();
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  const odb::dbTransform transform = inst->getTransform();

  const int inst_width = is_horizontal ? inst_rect.dy() : inst_rect.dx();
  const int inst_offset = is_horizontal ? inst_rect.yMin() : inst_rect.xMin();

  const int max_width = inst_width / (2 * (straps.size() + 1));
  TechLayer layer(getLayer());
  const int target_width = layer.snapToManufacturingGrid(max_width, false, 2);
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

  odb::Rect pin_shape;
  pin_shape.mergeInit();
  for (auto* pin : pins_) {
    pin_shape.merge(pin->getBox());
  }
  transform.apply(pin_shape);
  odb::Rect org_pin_shape = pin_shape;

  const int target_offset = layer.snapToManufacturingGrid(
      inst_offset + getSpacing() + getWidth() / 2, false);
  const int offset = target_offset + index * (getSpacing() + getWidth());

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
      closest_shape = std::move(layer_closest_shape);
    }
  }
  if (closest_shape == nullptr) {
    debugPrint(getLogger(), utl::PDN, "Pad", 3, "No connecting shape found.");
    return;
  }

  odb::Rect shape_rect;
  if (!snapRectToClosestShape(closest_shape, pin_shape, shape_rect)) {
    return;
  }

  auto shape = std::make_unique<Shape>(
      getLayer(), iterm_->getNet(), shape_rect, odb::dbWireShapeType::STRIPE);

  if (getDirection() != getLayer()->getDirection()) {
    shape->setAllowsNonPreferredDirectionChange();
  }
  const auto added = addShape(std::move(shape));
  if (added == nullptr) {
    return;
  }

  target_shapes_[added.get()] = closest_shape.get();
  target_pin_shape_[added.get()] = org_pin_shape;
}

bool PadDirectConnectionStraps::snapRectToClosestShape(
    const ShapePtr& closest_shape,
    const odb::Rect& pin_shape,
    odb::Rect& new_shape) const
{
  new_shape = pin_shape;
  switch (pad_edge_) {
    case odb::dbDirection::WEST:
      new_shape.set_xhi(closest_shape->getRect().xMax());
      break;
    case odb::dbDirection::EAST:
      new_shape.set_xlo(closest_shape->getRect().xMin());
      break;
    case odb::dbDirection::SOUTH:
      new_shape.set_yhi(closest_shape->getRect().yMax());
      break;
    case odb::dbDirection::NORTH:
      new_shape.set_ylo(closest_shape->getRect().yMin());
      break;
    default:
      return false;
  }

  return true;
}

void PadDirectConnectionStraps::getConnectableShapes(
    Shape::ShapeTreeMap& shapes) const
{
  if (type_ != ConnectionType::OverPads) {
    return;
  }

  odb::dbTechLayer* pin_layer = pins_[0]->getTechLayer();
  auto& pin_shapes = shapes[pin_layer];

  for (const auto& [layer, layer_pins] :
       InstanceGrid::getInstancePins(iterm_->getInst())) {
    if (layer != pin_layer) {
      continue;
    }

    for (const auto& shape : layer_pins) {
      if (shape->getNet() != iterm_->getNet()) {
        continue;
      }

      pin_shapes.insert(shape);
    }
  }
}

void PadDirectConnectionStraps::cutShapes(
    const Shape::ObstructionTreeMap& obstructions)
{
  Straps::cutShapes(obstructions);

  if (type_ != ConnectionType::OverPads) {
    return;
  }

  odb::dbInst* inst = iterm_->getInst();
  const odb::Rect inst_shape = inst->getBBox()->getBox();

  // filter out segments that are full enclosed by the pin shape (ie doesnt
  // connect to ring)
  std::vector<Shape*> remove_shapes;
  for (const auto& [layer, layer_shapes] : getShapes()) {
    for (const auto& shape : layer_shapes) {
      if (inst_shape.contains(shape->getRect())) {
        // reject shapes that only connect to pad
        remove_shapes.push_back(shape.get());
      } else if (!inst_shape.intersects(shape->getRect())) {
        // reject shapes that do not connect to pad
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
    const std::vector<PadDirectConnectionStraps*>& straps)
{
  std::set<ConnectionType> types;
  for (auto* strap : straps) {
    if (strap->getConnectionType() == ConnectionType::None) {
      continue;
    }
    types.insert(strap->getConnectionType());
  }

  ConnectionType global_connection = ConnectionType::None;
  if (types.size() == 1) {
    global_connection = *types.begin();
  } else {
    // Multiple methods found, pick Edge
    global_connection = ConnectionType::Edge;
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

bool PadDirectConnectionStraps::strapViaIsObstructed(
    Shape* shape,
    const Shape::ShapeTreeMap& other_shapes,
    const Shape::ObstructionTreeMap& other_obstructions,
    bool recheck) const
{
  auto find_shape = target_shapes_.find(shape);
  if (find_shape == target_shapes_.end()) {
    return false;
  }

  Shape* target = find_shape->second;
  int layer0 = target->getLayer()->getRoutingLevel();
  int layer1 = shape->getLayer()->getRoutingLevel();
  if (layer0 > layer1) {
    std::swap(layer0, layer1);
  }
  if (layer1 - layer0 <= 1) {
    return false;
  }

  if (!shape->getRect().intersects(find_shape->second->getRect())) {
    // if new shape doesn't intersects target reject
    return true;
  }

  const odb::Rect expected_via
      = shape->getRect().intersect(find_shape->second->getRect());

  auto* tech = target->getLayer()->getTech();
  for (int layer = layer0 + 1; layer < layer1; layer++) {
    auto* tech_layer = tech->findRoutingLayer(layer);

    if (!other_obstructions.contains(tech_layer)) {
      continue;
    }
    const auto& layer_shapes = other_obstructions.at(tech_layer);
    const bool has_obstruction
        = layer_shapes.qbegin(bgi::intersects(expected_via))
          != layer_shapes.qend();

    if (has_obstruction) {
      debugPrint(
          getLogger(),
          utl::PDN,
          "Pad",
          recheck ? 4 : 3,
          "Direct connect shape {} with obstruction {} using pin {} on {}",
          Shape::getRectText(expected_via, tech->getDbUnitsPerMicron()),
          tech_layer->getName(),
          Shape::getRectText(target_pin_shape_.at(shape),
                             tech->getDbUnitsPerMicron()),
          shape->getNet()->getName());
      return true;
    }
  }

  return false;
}

bool PadDirectConnectionStraps::refineShapes(
    Shape::ShapeTreeMap& all_shapes,
    Shape::ObstructionTreeMap& all_obstructions)
{
  if (type_ != ConnectionType::OverPads) {
    return GridComponent::refineShapes(all_shapes, all_obstructions);
  }

  std::vector<Shape*> refine;
  for (const auto& [layer, shapes] : getShapes()) {
    for (const auto& shape : shapes) {
      if (!strapViaIsObstructed(
              shape.get(), all_shapes, all_obstructions, false)) {
        continue;
      }

      refine.push_back(shape.get());
    }
  }

  if (refine.empty()) {
    return false;
  }

  const auto [first, last] = std::ranges::unique(refine.begin(), refine.end());
  refine.erase(first, last);

  for (auto* refine_shape : refine) {
    std::unique_ptr<Shape> shape = refine_shape->copy();
    removeShape(refine_shape);

    // remove shape from all_shapes and all_obstructions
    auto* layer = shape->getLayer();
    auto find_shape = [&refine_shape](const ShapePtr& other) {
      return other.get() == refine_shape;
    };
    // remove from all_shapes
    auto& layer_shapes = all_shapes[layer];
    auto find_all_shapes_itr = layer_shapes.qbegin(bgi::satisfies(find_shape));
    if (find_all_shapes_itr != layer_shapes.qend()) {
      layer_shapes.remove(*find_all_shapes_itr);
    }
    // remove from all_obstructions
    auto& layer_obstruction = all_obstructions[layer];
    auto find_all_obstructions_itr
        = layer_obstruction.qbegin(bgi::satisfies(find_shape));
    if (find_all_obstructions_itr != layer_obstruction.qend()) {
      layer_obstruction.remove(*find_all_obstructions_itr);
    }

    const TechLayer tech_layer(layer);
    for (int width : {getWidth(), tech_layer.getMinWidth()}) {
      setWidth(width);

      if (refineShape(shape.get(),
                      target_pin_shape_[refine_shape],
                      all_shapes,
                      all_obstructions)) {
        break;
      }
    }
  }

  return true;
}

bool PadDirectConnectionStraps::refineShape(
    Shape* shape,
    const odb::Rect& pin_shape,
    Shape::ShapeTreeMap& all_shapes,
    Shape::ObstructionTreeMap& all_obstructions)
{
  const TechLayer tech_layer(shape->getLayer());

  const int delta = tech_layer.getMinIncrementStep();

  int search_min;
  int search_max;

  const bool horizontal = isHorizontal();

  if (horizontal) {
    search_min = pin_shape.yMin();
    search_max = pin_shape.yMax() - getWidth();
  } else {
    search_min = pin_shape.xMin();
    search_max = pin_shape.xMax() - getWidth();
  }

  for (int check_loc = search_min; check_loc <= search_max;
       check_loc += delta) {
    odb::Rect new_rect = shape->getRect();

    if (horizontal) {
      new_rect.set_ylo(check_loc);
      new_rect.set_yhi(check_loc + getWidth());
    } else {
      new_rect.set_xlo(check_loc);
      new_rect.set_xhi(check_loc + getWidth());
    }

    std::unique_ptr<Shape> new_shape = shape->copy();
    new_shape->setRect(new_rect);

    debugPrint(getLogger(),
               utl::PDN,
               "Pad",
               4,
               "Checking new shape: {} on {}",
               Shape::getRectText(
                   new_shape->getRect(),
                   new_shape->getLayer()->getTech()->getDbUnitsPerMicron()),
               new_shape->getLayer()->getName());

    // check if legal
    if (strapViaIsObstructed(
            new_shape.get(), all_shapes, all_obstructions, true)) {
      continue;
    }
    const ShapePtr& added_shape = addShape(std::move(new_shape));
    if (added_shape != nullptr) {
      added_shape->clearITermConnections();
      added_shape->addITermConnection(
          pin_shape.intersect(added_shape->getRect()));

      // shape was added
      cutShapes(all_obstructions);

      // check if shape was removed during cutting
      if (getShapeCount() == 0) {
        continue;
      }

      // add shape to all_shapes and all_obstructions
      getObstructions(all_obstructions);
      getShapes(all_shapes);

      return true;
    }
  }

  return false;
}

bool PadDirectConnectionStraps::isTargetShape(const Shape* shape) const
{
  if (target_shapes_type_) {
    return shape->getType() == target_shapes_type_.value();
  }

  switch (shape->getType().getValue()) {
    case odb::dbWireShapeType::STRIPE:
    case odb::dbWireShapeType::RING:
      return true;
    case odb::dbWireShapeType::NONE:
    case odb::dbWireShapeType::PADRING:
    case odb::dbWireShapeType::BLOCKRING:
    case odb::dbWireShapeType::FOLLOWPIN:
    case odb::dbWireShapeType::IOWIRE:
    case odb::dbWireShapeType::COREWIRE:
    case odb::dbWireShapeType::BLOCKWIRE:
    case odb::dbWireShapeType::BLOCKAGEWIRE:
    case odb::dbWireShapeType::FILLWIRE:
    case odb::dbWireShapeType::DRCFILL:
      return false;
  }

  return false;
}

////////

RepairChannelStraps::RepairChannelStraps(
    Grid* grid,
    Straps* target,
    odb::dbTechLayer* connect_to,
    const Shape::ObstructionTreeMap& other_shapes,
    const std::set<odb::dbNet*>& nets,
    const odb::Rect& area,
    const odb::Rect& available_area,
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
      available_area_(available_area),
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

  odb::dbTechLayerDir connect_direction = connect_to->getDirection();
  // find the connecting strap and use it's direction
  for (const auto& comp : grid->getStraps()) {
    if (comp->getLayer() == connect_to) {
      connect_direction = comp->getDirection();
    }
  }

  if (connect_direction == odb::dbTechLayerDir::NONE) {
    // Assume this layer is horizontal if not set
    connect_direction = odb::dbTechLayerDir::HORIZONTAL;
  }

  if (connect_direction == getDirection()) {
    debugPrint(
        getLogger(),
        utl::PDN,
        "Channel",
        1,
        "Reject repair channel due to layer directions {} ({} / {}) -> {} ({})",
        connect_to->getName(),
        connect_to->getDirection().getString(),
        connect_direction.getString(),
        getLayer()->getName(),
        getDirection().getString());
    invalid_ = true;
  }

  if (!invalid_) {
    determineParameters(other_shapes);
  }

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

void RepairChannelStraps::continueRepairs(
    const Shape::ObstructionTreeMap& other_shapes)
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

void RepairChannelStraps::determineParameters(
    const Shape::ObstructionTreeMap& obstructions)
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
    area_width = available_area_.dy();
  } else {
    area_width = available_area_.dx();
  }

  auto check = [this, &area_width, &obstructions]() -> bool {
    const int group_width = getStrapGroupWidth();
    if (group_width > area_width) {
      debugPrint(
          getLogger(),
          utl::PDN,
          "Channel",
          2,
          "Failed on channel width check, group {:.4f} and channel {:.4f}.",
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

bool RepairChannelStraps::determineOffset(
    const Shape::ObstructionTreeMap& obstructions,
    int extra_offset,
    int bisect_dist,
    int level)
{
  const TechLayer layer(getLayer());
  const bool is_horizontal = isHorizontal();
  const int group_width = getStrapGroupWidth();
  int offset = -group_width / 2 + extra_offset;
  if (is_horizontal) {
    offset += 0.5 * (available_area_.yMin() + available_area_.yMax());
  } else {
    offset += 0.5 * (available_area_.xMin() + available_area_.xMax());
  }
  const int half_width = getWidth() / 2;
  offset += half_width;
  offset = layer.snapToManufacturingGrid(offset);

  odb::Rect estimated_straps;
  const int strap_start = offset - half_width;
  if (is_horizontal) {
    estimated_straps = odb::Rect(available_area_.xMin(),
                                 strap_start,
                                 available_area_.xMax(),
                                 strap_start + group_width);
  } else {
    estimated_straps = odb::Rect(strap_start,
                                 available_area_.yMin(),
                                 strap_start + group_width,
                                 available_area_.yMax());
  }

  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      3,
      "Estimating strap to be {} within {}.",
      Shape::getRectText(estimated_straps, getBlock()->getDbUnitsPerMicron()),
      Shape::getRectText(available_area_, getBlock()->getDbUnitsPerMicron()));

  // check if straps will fit
  if (is_horizontal) {
    if (estimated_straps.dy() > available_area_.dy()) {
      return false;
    }
    if (estimated_straps.yMin() < available_area_.yMin()
        || estimated_straps.yMax() > available_area_.yMax()) {
      return false;
    }
  } else {
    if (estimated_straps.dx() > available_area_.dx()) {
      return false;
    }
    if (estimated_straps.xMin() < available_area_.xMin()
        || estimated_straps.xMax() > available_area_.xMax()) {
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
  auto check_obstructions
      = [&obs_check](const Shape::ObstructionTree& shapes) -> bool {
    for (auto itr = shapes.qbegin(bgi::intersects(obs_check));
         itr != shapes.qend();
         itr++) {
      const auto& shape = *itr;
      if (obs_check.overlaps(shape->getObstruction())) {
        return true;
      }
    }
    return false;
  };
  for (auto* layer : check_layers) {
    if (obstructions.contains(layer)) {
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
      const int width
          = is_horizontal ? available_area_.dy() : available_area_.dx();
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

void RepairChannelStraps::cutShapes(
    const Shape::ObstructionTreeMap& obstructions)
{
  Straps::cutShapes(obstructions);

  // filter out cut segments that do not overlap with the area
  std::vector<Shape*> remove_shapes;
  for (const auto& [layer, layer_shapes] : getShapes()) {
    for (const auto& shape : layer_shapes) {
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
    for (const auto& shape : layer_shapes) {
      region.merge(shape->getRect());
    }
  }
  if (!region.isInverted()) {
    if (isHorizontal()) {
      setStrapStartEnd(region.xMin(), region.xMax());
    } else {
      setStrapStartEnd(region.yMin(), region.yMax());
    }
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
                                        const Shape::ShapeTree& shapes,
                                        odb::dbTechLayer* layer)
{
  Straps* target = getTargetStrap(grid, layer);
  if (target == nullptr) {
    // no possible connect statements for this layer
    return {};
  }

  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  const auto grid_core = grid->getDomainBoundary();

  std::vector<Shape*> shapes_used;
  Polygon90Set shape_set;
  for (const auto& shape : shapes) {
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
      if (shape->getNumberOfConnections() == 0
          || !shape->hasInternalConnections()) {
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

    const auto& min_corner = shape->getRect().ll();
    const auto& max_corner = shape->getRect().ur();
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
    RepairChannelArea channel{area, area, odb::Rect(), target, layer, {}};
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
      channel.available_area = channel.area;

      // trim area of channel if it is partially covered by an exisiting shape
      for (const auto& [layer, layer_shapes] : grid->getShapes()) {
        if (layer != channel.target->getLayer()) {
          continue;
        }

        for (auto itr = layer_shapes.qbegin(bgi::intersects(channel.area));
             itr != layer_shapes.qend();
             itr++) {
          const auto& shape = *itr;
          const auto& obs = shape->getObstruction();

          if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
            if (channel.available_area.xMax() > obs.xMin()
                && channel.available_area.xMax() <= obs.xMax()) {
              channel.available_area.set_xhi(obs.xMin());
            }
            if (channel.available_area.xMin() < obs.xMax()
                && channel.available_area.xMin() >= obs.xMin()) {
              channel.available_area.set_xlo(obs.xMax());
            }
          } else {
            if (channel.available_area.yMax() > obs.yMin()
                && channel.available_area.yMax() <= obs.yMax()) {
              channel.available_area.set_yhi(obs.yMin());
            }
            if (channel.available_area.yMin() < obs.yMax()
                && channel.available_area.yMin() >= obs.yMin()) {
              channel.available_area.set_ylo(obs.yMax());
            }
          }
        }
      }
      channel.obs_area = channel.obs_area.intersect(grid_core);

      channels.push_back(std::move(channel));
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

bool RepairChannelStraps::testBuild(
    const Shape::ShapeTreeMap& local_shapes,
    const Shape::ObstructionTreeMap& obstructions)
{
  makeShapes(local_shapes);
  cutShapes(obstructions);
  return !isEmpty();
}

void RepairChannelStraps::repairGridChannels(
    Grid* grid,
    const Shape::ShapeTreeMap& global_shapes,
    Shape::ObstructionTreeMap& obstructions,
    bool allow,
    PDNRenderer* renderer)
{
  // create copy of shapes so they can be used to determine repair locations
  Shape::ShapeTreeMap local_shapes = global_shapes;

  std::set<odb::Rect> areas_repaired;

  const auto channels = findRepairChannels(grid);
  debugPrint(grid->getLogger(),
             utl::PDN,
             "Channel",
             1,
             "Channels to repair {}.",
             channels.size());

  if (!channels.empty() && renderer != nullptr) {
    renderer->update();
    renderer->pause();
  }

  if (channels.empty()) {
    return;
  }

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
                                                       channel.available_area,
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

  if (!areas_repaired.empty()) {
    // channels changed so try again
    repairGridChannels(grid, global_shapes, obstructions, allow, renderer);
  } else {
    const auto remaining_channels = findRepairChannels(grid);
    if (!remaining_channels.empty()) {
      odb::dbMarkerCategory* tool_category
          = grid->getBlock()->findMarkerCategory("PDN");
      if (tool_category == nullptr) {
        tool_category = odb::dbMarkerCategory::create(grid->getBlock(), "PDN");
        tool_category->setSource("PDN");
      }
      odb::dbMarkerCategory* category = odb::dbMarkerCategory::createOrReplace(
          tool_category, "Repair channels");
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

        odb::dbMarker* marker = odb::dbMarker::create(category);
        if (marker == nullptr) {
          continue;
        }
        marker->addShape(channel.area);
        for (auto* net : channel.nets) {
          marker->addSource(net);
        }
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

  int new_width = layer.snapToManufacturingGrid(getWidth() / 2, false, 2);

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
