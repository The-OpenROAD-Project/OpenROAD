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
      offset_(0),
      number_of_straps_(number_of_straps),
      direction_(odb::dbTechLayerDir::NONE),
      snap_(false),
      extend_mode_(ExtensionMode::CORE),
      strap_start_(0),
      strap_end_(0)
{
  if (spacing_ == 0 && pitch_ != 0) {
    // spacing not defined, so use pitch / (# of nets)
    spacing_ = pitch_ / getNetCount() - width_;
    spacing_ = TechLayer::snapToManufacturingGrid(getBlock()->getDataBase()->getTech(), spacing_, true);
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

  checkLayerWidth(layer_, width_, direction_);
  checkLayerSpacing(layer_, width_, spacing_, direction_);

  const int strap_width = getStrapGroupWidth();
  if (pitch_ != 0) {
    const int min_pitch = strap_width + spacing_;
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

  const odb::Rect grid_area = getGrid()->getDomainArea();
  int grid_width = 0;
  if (isHorizontal()) {
    grid_width = grid_area.dy();
  } else {
    grid_width = grid_area.dx();
  }
  if (grid_width < offset_ + strap_width) {
    getLogger()->error(
        utl::PDN,
        185,
        "Insufficient width to add straps on layer {} in grid \"{}\".",
        layer_->getName(),
        getGrid()->getLongName());
  }
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
  const auto other_grid_shapes = getGrid()->getShapes();
  ShapeTree avoid;
  if (other_grid_shapes.count(layer_) != 0) {
    for (const auto& [box, shape] : other_grid_shapes.at(layer_)) {
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

  int last_strap_end = -spacing_;
  for (pos += offset_; pos <= pos_end; pos += pitch_) {
    int group_pos = pos;
    for (auto* net : nets) {
      // snap to grid if needed
      const int strap_center
          = layer.snapToGrid(group_pos, last_strap_end + spacing_);
      const int strap_start = strap_center - half_width;
      const int strap_end = strap_start + width_;
      if (strap_start >= pos_end) {
        // no portion of the strap is inside the limit
        return;
      }
      if (strap_center > pos_end) {
        // strap center is outside of alotted area
        return;
      }

      odb::Rect strap_rect;
      if (is_delta_x) {
        strap_rect = odb::Rect(strap_start, y_start, strap_end, y_end);
      } else {
        strap_rect = odb::Rect(x_start, strap_start, x_end, strap_end);
      }
      last_strap_end = strap_end;
      group_pos += spacing_ + width_;

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

  logger->info(utl::PDN, 40, "  Type: {}", typeToString(type()));
  logger->info(utl::PDN, 41, "    Layer: {}", layer_->getName());
  logger->info(utl::PDN, 42, "    Width: {:.4f}", width_ / dbu_per_micron);
  logger->info(utl::PDN, 43, "    Spacing: {:.4f}", spacing_ / dbu_per_micron);
  logger->info(utl::PDN, 44, "    Pitch: {:.4f}", pitch_ / dbu_per_micron);
  logger->info(utl::PDN, 45, "    Offset: {:.4f}", offset_ / dbu_per_micron);
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
    odb::Rect bbox;
    row->getBBox(bbox);
    setPitch(2 * bbox.dy());

    if (row->getDirection() == odb::dbRowDir::HORIZONTAL) {
      setDirection(odb::dbTechLayerDir::HORIZONTAL);
    } else {
      setDirection(odb::dbTechLayerDir::VERTICAL);
    }
  } else {
    if (getPitch() == 0) {
      getLogger()->error(utl::PDN, 190, "Unable to determine the pitch of the rows.");
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
    : Straps(grid, nullptr, 0, 0),
      iterm_(iterm),
      target_shapes_(odb::dbWireShapeType::RING),
      pad_edge_(odb::dbDirection::NONE)
{
  initialize(connect_pad_layers);
}

bool PadDirectConnectionStraps::canConnect() const
{
  return pad_edge_ != odb::dbDirection::NONE && !pins_.empty();
}

void PadDirectConnectionStraps::initialize(
    const std::vector<odb::dbTechLayer*>& layers)
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

void PadDirectConnectionStraps::report() const
{
  auto* logger = getLogger();

  logger->info(utl::PDN, 80, "  Type: {}", typeToString(type()));
  logger->info(utl::PDN, 81, "    Pin: {}", getName());
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
  auto search_shapes = getGrid()->getShapes();
  if (search_shapes.empty()) {
    return;
  }

  std::set<odb::dbTechLayer*> connectable_layers;
  for (const auto& [layer, shapes] : search_shapes) {
    for (const auto& [box, shape] : shapes) {
      if (shape->getType() == target_shapes_) {
        const auto layers = getGrid()->connectableLayers(layer);
        connectable_layers.insert(layers.begin(), layers.end());
      }
    }
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
  const bool is_north = pad_edge_ == odb::dbDirection::NORTH;
  const bool is_west = pad_edge_ == odb::dbDirection::WEST;
  const bool is_east = pad_edge_ == odb::dbDirection::EAST;

  const bool is_horizontal_strap = isConnectHorizontal();

  auto* net = iterm_->getNet();
  for (auto* pin : pins_) {
    odb::Rect pin_rect;
    pin->getBox(pin_rect);
    transform.apply(pin_rect);

    auto* layer = pin->getTechLayer();
    if (connectable_layers.find(layer) == connectable_layers.end()) {
      // layer is not connectable to a target
      continue;
    }

    // generate search box
    Box search;
    if (is_horizontal_strap) {
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

      for (auto it = search_shape_tree.qbegin(
               bgi::intersects(search)
               && bgi::satisfies([&](const auto& other) {
                    const auto& shape = other.second;
                    return shape->getNet() == net
                           && shape->getType() == target_shapes_;
                  }));
           it != search_shape_tree.qend();
           it++) {
        const auto& shape = it->second;
        int new_dist = std::numeric_limits<int>::max();
        if (is_west) {
          new_dist = shape->getRect().xMin() - pin_rect.xMax();
        } else if (is_east) {
          new_dist = pin_rect.xMin() - shape->getRect().xMax();
        } else if (is_south) {
          new_dist = shape->getRect().yMin() - pin_rect.yMax();
        } else if (is_north) {
          new_dist = pin_rect.yMin() - shape->getRect().yMax();
        } else {
          continue;
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

////////

RepairChannelStraps::RepairChannelStraps(Grid* grid,
                                         Straps* target,
                                         odb::dbTechLayer* connect_to,
                                         const ShapeTreeMap& other_shapes,
                                         const std::set<odb::dbNet*>& nets,
                                         const odb::Rect& area,
                                         bool allow)
    : Straps(grid,
             target->getLayer(),
             target->getWidth(),
             0,
             target->getSpacing(),
             1),
      nets_(nets),
      connect_to_(connect_to),
      area_(area),
      invalid_(false)
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

  if (invalid_ && !allow) {
    const TechLayer layer(getLayer());
    getLogger()->error(
        utl::PDN,
        176,
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
  int max_length = 0;
  int area_width = 0;
  const odb::Rect core = getGrid()->getDomainArea();
  if (isHorizontal()) {
    max_length = core.dx();
    area_width = area_.dy();
  } else {
    max_length = core.dy();
    area_width = area_.dx();
  }

  auto check = [&]() -> bool {
    if (getStrapGroupWidth() > area_width) {
      return false;
    }
    return determineOffset(obstructions);
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
    const int new_width = std::max(layer.snapToManufacturingGrid(getWidth() / 2), min_width);
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

  // unable to determine how to place repair straps
  invalid_ = true;
}

bool RepairChannelStraps::determineOffset(const ShapeTreeMap& obstructions,
                                          int extra_offset,
                                          int bisect_dist,
                                          int level)
{
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

  odb::Rect estimated_straps;
  const int strap_start = offset - half_width;
  if (is_horizontal) {
    estimated_straps = odb::Rect(
        area_.xMin(), strap_start, area_.xMax(), strap_start + group_width);
  } else {
    estimated_straps = odb::Rect(
        strap_start, area_.yMin(), strap_start + group_width, area_.yMax());
  }

  // check if straps will fit
  if (is_horizontal) {
    if (estimated_straps.yMin() < area_.yMin()
        || estimated_straps.yMax() > area_.yMax()) {
      return false;
    }
  } else {
    if (estimated_straps.xMin() < area_.xMin()
        || estimated_straps.xMax() > area_.xMax()) {
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
  debugPrint(
      getLogger(),
      utl::PDN,
      "Channel",
      3,
      "Estimating strap to be {}.",
      Shape::getRectText(estimated_straps, getBlock()->getDbUnitsPerMicron()));
  Box estimated_straps_box(
      Point(estimated_straps.xMin(), estimated_straps.yMin()),
      Point(estimated_straps.xMax(), estimated_straps.yMax()));
  for (auto* layer : check_layers) {
    if (obstructions.count(layer) != 0) {
      // obstructions possible on this layer
      const auto& shapes = obstructions.at(layer);
      if (shapes.qbegin(bgi::intersects(estimated_straps_box))
          != shapes.qend()) {
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
    const TechLayer layer(getLayer());
    layer.snapToManufacturingGrid(new_bisect_dist);

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
    if (new_bisect_dist < group_width) {
      // new offset to too small to matter, so stop
      return false;
    } else {
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

  return true;
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
      if (!shape->getRect().intersects(area_)) {
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

  logger->info(utl::PDN,
               82,
               "    Repair area: {}",
               Shape::getRectText(area_, block->getDbUnitsPerMicron()));
  logger->info(utl::PDN, 83, "    Nets: {}", getNetString());
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

    auto* grid_strap = dynamic_cast<Straps*>(grid_compomponent);
    if (grid_strap == nullptr) {
      continue;
    }

    // determine bloat factor
    int bloat = grid_strap->getPitch();
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

  std::vector<Rectangle> rects;
  shape_set.get_rectangles(rects);
  std::vector<RepairChannelArea> channels;
  for (const auto& rect : rects) {
    odb::Rect area(xl(rect), yl(rect), xh(rect), yh(rect));
    if (!area.intersects(grid_core)) {
      continue;
    }
    RepairChannelArea channel{area.intersect(grid_core), target, layer, {}};

    int followpin_count = 0;
    int strap_count = 0;
    // find all the nets in a given repair area
    for (auto* shape : shapes_used) {
      if (channel.area.overlaps(shape->getRect())) {
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
                                                       allow);

    if (!strap->isRepairValid()) {
      continue;
    }

    // build strap
    strap->makeShapes(local_shapes);
    strap->cutShapes(obstructions);
    if (strap->getShapeCount() == 0) {
      // nothing was added
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

  if (channels.size() != areas_repaired.size() && areas_repaired.size() != 0) {
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
          nets += net->getName() + " ";
        }
        grid->getLogger()->warn(
            utl::PDN,
            178,
            "Remaining channel {} for nets: {}",
            Shape::getRectText(channel.area, dbu_to_microns),
            nets);
      }
      if (!allow) {
        grid->getLogger()->error(
            utl::PDN, 179, "Unable to repair all channels.");
      }
    }
  }
}

}  // namespace pdn
