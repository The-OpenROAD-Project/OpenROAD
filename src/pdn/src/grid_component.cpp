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

#include "grid_component.h"

#include "connect.h"
#include "grid.h"
#include "odb/db.h"
#include "techlayer.h"
#include "utl/Logger.h"
#include "via.h"

namespace pdn {

GridComponent::GridComponent(Grid* grid)
    : grid_(grid), starts_with_power_(grid_->startsWithPower())
{
}

odb::dbBlock* GridComponent::getBlock() const
{
  return grid_->getBlock();
}

utl::Logger* GridComponent::getLogger() const
{
  return grid_->getLogger();
}

VoltageDomain* GridComponent::getDomain() const
{
  return grid_->getDomain();
}

std::string GridComponent::typeToString(Type type)
{
  switch (type) {
    case Ring:
      return "Ring";
    case Strap:
      return "Strap";
    case Followpin:
      return "Followpin";
    case PadConnect:
      return "Pad connect";
    case RepairChannel:
      return "Repair channel";
  }

  return "Unknown";
}

ShapePtr GridComponent::addShape(Shape* shape)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Shape",
             3,
             "Adding shape {}.",
             shape->getReportText());
  auto shape_ptr = std::shared_ptr<Shape>(shape);
  if (!shape_ptr->isValid()) {
    // do not add invalid shapes
    return nullptr;
  }

  shape_ptr->setGridComponent(this);
  const odb::Rect& shape_rect = shape_ptr->getRect();

  auto& shapes = shapes_[shape_ptr->getLayer()];

  debugPrint(getLogger(),
             utl::PDN,
             "Shape",
             3,
             "Checking against {} shapes",
             shapes.size());
  // check if shape will intersect anything already added
  std::vector<ShapePtr> intersecting;
  for (auto it = shapes.qbegin(bgi::intersects(shape_ptr->getRect()));
       it != shapes.qend();
       it++) {
    auto& intersecting_shape = *it;
    if (!shape_ptr->getRect().overlaps(intersecting_shape->getRect())) {
      continue;
    }
    if (intersecting_shape->getNet() != shape_ptr->getNet()) {
      // short
      debugPrint(getLogger(),
                 utl::PDN,
                 "Shape",
                 4,
                 "Short between {} and {}",
                 intersecting_shape->getReportText(),
                 shape_ptr->getReportText());
      return nullptr;
    }

    if (areIntersectionsAllowed()) {
      continue;
    }

    const odb::Rect& other_rect = intersecting_shape->getRect();
    const bool is_x_overlap = shape_rect.xMin() == other_rect.xMin()
                              && shape_rect.xMax() == other_rect.xMax();
    const bool is_y_overlap = shape_rect.yMin() == other_rect.yMin()
                              && shape_rect.yMax() == other_rect.yMax();

    if (!is_x_overlap && !is_y_overlap) {
      debugPrint(getLogger(),
                 utl::PDN,
                 "Shape",
                 4,
                 "Unable to merge {} and {}",
                 intersecting_shape->getReportText(),
                 shape_ptr->getReportText());
      return nullptr;
    }

    // save intersection so they can be processed later
    intersecting.push_back(*it);
  }

  for (const auto& shape_entry : intersecting) {
    // merge and delete intersection
    shape_ptr->merge(shape_entry.get());
    shapes.remove(shape_entry);
  }

  shape_ptr->generateObstruction();
  shapes.insert(shape_ptr);

  // add bpins that touch edges
  odb::Rect die_area = getBlock()->getDieArea();
  const odb::Rect& final_shape_rect = shape_ptr->getRect();
  const int min_width = shape_ptr->getLayer()->getMinWidth();
  if (final_shape_rect.xMin() == die_area.xMin()) {
    const int x = std::min(static_cast<int>(die_area.xMin() + min_width),
                           final_shape_rect.xMax());
    odb::Rect pin_rect = final_shape_rect;
    pin_rect.set_xhi(x);
    shape_ptr->addBTermConnection(pin_rect);
  }
  if (final_shape_rect.xMax() == die_area.xMax()) {
    const int x = std::max(static_cast<int>(die_area.xMax() - min_width),
                           final_shape_rect.xMin());
    odb::Rect pin_rect = final_shape_rect;
    pin_rect.set_xlo(x);
    shape_ptr->addBTermConnection(pin_rect);
  }
  if (final_shape_rect.yMin() == die_area.yMin()) {
    const int y = std::min(static_cast<int>(die_area.yMin() + min_width),
                           final_shape_rect.yMax());
    odb::Rect pin_rect = final_shape_rect;
    pin_rect.set_yhi(y);
    shape_ptr->addBTermConnection(pin_rect);
  }
  if (final_shape_rect.yMax() == die_area.yMax()) {
    const int y = std::max(static_cast<int>(die_area.yMax() - min_width),
                           final_shape_rect.yMin());
    odb::Rect pin_rect = final_shape_rect;
    pin_rect.set_ylo(y);
    shape_ptr->addBTermConnection(pin_rect);
  }

  return shape_ptr;
}

void GridComponent::removeShape(Shape* shape)
{
  for (const auto& via : shape->getVias()) {
    via->removeShape(shape);
  }

  auto& shapes = shapes_[shape->getLayer()];
  for (const auto& tree_shape : shapes) {
    if (tree_shape.get() == shape) {
      shapes.remove(tree_shape);
      return;
    }
  }
}

void GridComponent::replaceShape(Shape* shape,
                                 const std::vector<Shape*>& replacements)
{
  auto vias = shape->getVias();

  removeShape(shape);

  for (auto* new_shape : replacements) {
    const auto& new_shape_ptr = addShape(new_shape);

    if (new_shape_ptr == nullptr) {
      continue;
    }

    for (const auto& via : vias) {
      if (via->getArea().intersects(new_shape_ptr->getRect())) {
        Connect* connect = via->getConnect();
        if (connect->getLowerLayer() == new_shape_ptr->getLayer()) {
          via->setLowerShape(new_shape_ptr);
        } else if (connect->getUpperLayer() == new_shape_ptr->getLayer()) {
          via->setUpperShape(new_shape_ptr);
        }
      }
    }
  }
}

void GridComponent::getObstructions(
    Shape::ObstructionTreeMap& obstructions) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             2,
             "Getting obstructions in \"{}\"",
             getGrid()->getName());
  for (const auto& [layer, shapes] : shapes_) {
    auto& obs = obstructions[layer];
    for (const auto& shape : shapes) {
      obs.insert(shape);
    }
  }
}

void GridComponent::removeObstructions(
    Shape::ObstructionTreeMap& obstructions) const
{
  for (const auto& [layer, shapes] : shapes_) {
    auto& obs = obstructions[layer];
    for (const auto& shape : shapes) {
      obs.remove(shape);
    }
  }
}

void GridComponent::getShapes(Shape::ShapeTreeMap& shapes) const
{
  for (const auto& [layer, layer_shapes] : shapes_) {
    shapes[layer].insert(layer_shapes.begin(), layer_shapes.end());
  }
}

void GridComponent::removeShapes(Shape::ShapeTreeMap& shapes) const
{
  for (const auto& [layer, layer_shapes] : shapes_) {
    auto& other_shapes = shapes[layer];
    other_shapes.remove(layer_shapes.begin(), layer_shapes.end());
  }
}

void GridComponent::cutShapes(const Shape::ObstructionTreeMap& obstructions)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Cutting shapes in \"{}\"",
             getGrid()->getName());

  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             2,
             "Initial shape count: {}",
             getShapeCount());

  for (const auto& [layer, shapes] : shapes_) {
    if (obstructions.count(layer) == 0) {
      continue;
    }
    const auto& obs = obstructions.at(layer);
    std::map<Shape*, std::vector<Shape*>> replacement_shapes;
    for (const auto& shape : shapes) {
      std::vector<Shape*> replacements;
      if (!shape->cut(obs, getGrid(), replacements)) {
        continue;
      }

      replacement_shapes[shape.get()] = std::move(replacements);
    }

    for (const auto& [shape, replacement] : replacement_shapes) {
      replaceShape(shape, replacement);
    }
  }

  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             2,
             "Final shape count: {}",
             getShapeCount());
}

void GridComponent::writeToDb(
    const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
    bool add_pins,
    const std::set<odb::dbTechLayer*>& convert_layer_to_pin) const
{
  std::vector<ShapePtr> all_shapes;
  for (const auto& [layer, shapes] : shapes_) {
    for (const auto& shape : shapes) {
      all_shapes.push_back(shape);
    }
  }

  // sort shapes so they get written to db in the same order
  std::sort(
      all_shapes.begin(), all_shapes.end(), [](const auto& l, const auto& r) {
        auto* l_layer = l->getLayer();
        int l_level = l_layer->getNumber();
        auto* r_layer = l->getLayer();
        int r_level = r_layer->getNumber();

        return std::tie(l_level, l->getRect())
               < std::tie(r_level, r->getRect());
      });

  for (const auto& shape : all_shapes) {
    auto net = net_map.find(shape->getNet());
    if (net == net_map.end()) {
      continue;
    }
    const bool is_pin_layer = convert_layer_to_pin.find(shape->getLayer())
                              != convert_layer_to_pin.end();
    shape->writeToDb(net->second, add_pins, is_pin_layer);
  }
}

void GridComponent::checkLayerWidth(odb::dbTechLayer* layer,
                                    int width,
                                    const odb::dbTechLayerDir& direction) const
{
  const TechLayer tech_layer(layer);

  // check for min width violation
  const int min_width = tech_layer.getMinWidth();
  if (width < min_width) {
    getLogger()->error(utl::PDN,
                       106,
                       "Width ({:.4f} um) specified for layer {} is less than "
                       "minimum width ({:.4f} um).",
                       tech_layer.dbuToMicron(width),
                       layer->getName(),
                       tech_layer.dbuToMicron(min_width));
  }

  // check for max width violation
  const int max_width = tech_layer.getMaxWidth();
  if (width > max_width) {
    getLogger()->error(utl::PDN,
                       107,
                       "Width ({:.4f} um) specified for layer {} is greater "
                       "than maximum width ({:.4f} um).",
                       tech_layer.dbuToMicron(width),
                       layer->getName(),
                       tech_layer.dbuToMicron(max_width));
  }

  // check if width table in use and check widths
  for (auto* rule : layer->getTechLayerWidthTableRules()) {
    bool check_table = false;
    if (rule->isWrongDirection()) {
      if (direction != layer->getDirection()) {
        check_table = true;
      }
    } else {
      if (direction == layer->getDirection()) {
        check_table = true;
      }
    }

    const auto width_table = rule->getWidthTable();

    if (width_table.empty()) {
      check_table = false;
    } else {
      if (width > width_table.back()) {
        // width is outside of table
        check_table = false;
      }
    }

    bool found_width = false;
    for (auto table_width : width_table) {
      if (table_width == width) {
        found_width = true;
      }
    }

    if (check_table && !found_width) {
      std::string widths;
      for (auto width : width_table) {
        if (!widths.empty()) {
          widths += ", ";
        }
        widths += fmt::format("{:.4f}", tech_layer.dbuToMicron(width));
      }
      getLogger()->error(utl::PDN,
                         114,
                         "Width ({:.4f} um) specified for layer {} in not a "
                         "valid width, must be {}.",
                         tech_layer.dbuToMicron(width),
                         layer->getName(),
                         widths);
    }
  }

  odb::dbTech* tech = layer->getTech();
  if (tech->hasManufacturingGrid()) {
    const int double_grid = 2 * tech->getManufacturingGrid();
    if (width % double_grid != 0) {
      getLogger()->error(
          utl::PDN,
          117,
          "Width ({:.4f} um) specified must be a multiple of {:.4f} um.",
          tech_layer.dbuToMicron(width),
          tech_layer.dbuToMicron(double_grid));
    }
  }
}

void GridComponent::checkLayerSpacing(
    odb::dbTechLayer* layer,
    int width,
    int spacing,
    const odb::dbTechLayerDir& /* direction */) const
{
  const TechLayer tech_layer(layer);

  // check min spacing violation
  const int min_spacing = tech_layer.getSpacing(width);
  if (spacing < min_spacing) {
    getLogger()->error(utl::PDN,
                       108,
                       "Spacing ({:.4f} um) specified for layer {} is less "
                       "than minimum spacing ({:.4f} um).",
                       tech_layer.dbuToMicron(spacing),
                       layer->getName(),
                       tech_layer.dbuToMicron(min_spacing));
  }

  odb::dbTech* tech = layer->getTech();
  if (tech->hasManufacturingGrid()) {
    const int grid = tech->getManufacturingGrid();
    if (spacing % grid != 0) {
      getLogger()->error(
          utl::PDN,
          118,
          "Spacing ({:.4f} um) specified must be a multiple of {:.4f} um.",
          tech_layer.dbuToMicron(spacing),
          tech_layer.dbuToMicron(grid));
    }
  }
}

int GridComponent::getShapeCount() const
{
  int count = 0;

  for (const auto& [layer, layer_shapes] : shapes_) {
    count += layer_shapes.size();
  }

  return count;
}

void GridComponent::setNets(const std::vector<odb::dbNet*>& nets)
{
  const auto grid_nets = grid_->getNets();
  for (auto* net : nets) {
    if (std::find(grid_nets.begin(), grid_nets.end(), net) == grid_nets.end()) {
      getLogger()->error(utl::PDN,
                         224,
                         "{} is not a net in {}.",
                         net->getName(),
                         grid_->getLongName());
    }
  }

  nets_ = nets;
}

std::vector<odb::dbNet*> GridComponent::getNets() const
{
  if (nets_.empty()) {
    return grid_->getNets(starts_with_power_);
  }
  return nets_;
}

int GridComponent::getNetCount() const
{
  return getNets().size();
}

}  // namespace pdn
