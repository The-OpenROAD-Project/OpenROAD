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

#include "grid_shape.h"

#include "grid.h"
#include "odb/db.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

GridShape::GridShape(Grid* grid) : grid_(grid)
{
}

odb::dbBlock* GridShape::getBlock() const
{
  return grid_->getBlock();
}

utl::Logger* GridShape::getLogger() const
{
  return grid_->getLogger();
}

VoltageDomain* GridShape::getDomain() const
{
  return grid_->getDomain();
}

const std::string GridShape::typeToString(Type type)
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

ShapePtr GridShape::addShape(Shape* shape)
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

  shape_ptr->setGridShape(this);
  const odb::Rect& shape_rect = shape_ptr->getRect();

  auto& shapes = shapes_[shape_ptr->getLayer()];

  debugPrint(getLogger(),
             utl::PDN,
             "Shape",
             3,
             "Checking against {} shapes",
             shapes.size());
  // check if shape will intersect anything already added
  std::vector<ShapeValue> intersecting;
  for (auto it = shapes.qbegin(bgi::intersects(shape_ptr->getRectBox()));
       it != shapes.qend();
       it++) {
    auto& intersecting_shape = it->second;
    const odb::Rect intersecting_area
        = shape_ptr->getRect().intersect(intersecting_shape->getRect());
    if (intersecting_area.area() == 0) {
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
    shape_ptr->merge(shape_entry.second.get());
    shapes.remove(shape_entry);
  }

  shape_ptr->generateObstruction();
  shapes.insert({shape_ptr->getRectBox(), shape_ptr});

  // add bpins that touch edges
  odb::Rect die_area;
  getBlock()->getDieArea(die_area);
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
    pin_rect.set_ylo(y);
    shape_ptr->addBTermConnection(pin_rect);
  }
  if (final_shape_rect.yMax() == die_area.yMax()) {
    const int y = std::max(static_cast<int>(die_area.yMax() - min_width),
                           final_shape_rect.yMin());
    odb::Rect pin_rect = final_shape_rect;
    pin_rect.set_yhi(y);
    shape_ptr->addBTermConnection(pin_rect);
  }

  return shape_ptr;
}

void GridShape::removeShape(Shape* shape)
{
  for (const auto& via : shape->getVias()) {
    via->removeShape(shape);
  }

  auto& shapes = shapes_[shape->getLayer()];
  for (const auto& [box, tree_shape] : shapes) {
    if (tree_shape.get() == shape) {
      shapes.remove({box, tree_shape});
      return;
    }
  }
}

void GridShape::replaceShape(Shape* shape,
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

void GridShape::getObstructions(ShapeTreeMap& obstructions) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             2,
             "Getting obstructions in \"{}\"",
             getGrid()->getName());
  for (const auto& [layer, shapes] : shapes_) {
    auto& obs = obstructions[layer];
    for (const auto& [box, shape] : shapes) {
      obs.insert({shape->getObstructionBox(), shape});
    }
  }
}

void GridShape::cutShapes(const ShapeTreeMap& obstructions)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Cutting shapes in \"{}\"",
             getGrid()->getName());

  for (const auto& [layer, shapes] : shapes_) {
    if (obstructions.count(layer) == 0) {
      continue;
    }
    const auto& obs = obstructions.at(layer);
    std::map<Shape*, std::vector<Shape*>> replacement_shapes;
    for (const auto& [box, shape] : shapes) {
      const auto replacements = shape->cut(obs);
      if (replacements.empty()) {
        continue;
      }

      replacement_shapes[shape.get()] = replacements;
    }

    for (const auto& [shape, replacement] : replacement_shapes) {
      replaceShape(shape, replacement);
    }
  }
}

void GridShape::writeToDb(
    const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
    bool add_pins,
    const std::set<odb::dbTechLayer*>& convert_layer_to_pin) const
{
  std::vector<ShapePtr> all_shapes;
  for (const auto& [layer, shapes] : shapes_) {
    for (const auto& [box, shape] : shapes) {
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
    odb::dbNet* net = shape->getNet();
    const bool is_pin_layer = convert_layer_to_pin.find(shape->getLayer())
                              != convert_layer_to_pin.end();
    shape->writeToDb(net_map.at(net), add_pins, is_pin_layer);
  }
}

void GridShape::checkLayerWidth(odb::dbTechLayer* layer,
                                int width,
                                odb::dbTechLayerDir direction) const
{
  const TechLayer tech_layer(layer);
  const double dbu_to_microns = tech_layer.getLefUnits();

  // check for min width violation
  const int min_width = tech_layer.getMinWidth();
  if (width < min_width) {
    getLogger()->error(utl::PDN,
                       106,
                       "Width ({:.4f} um) specified for layer {} is less than "
                       "minimum width ({:.4f} um).",
                       width / dbu_to_microns,
                       layer->getName(),
                       min_width / dbu_to_microns);
  }

  // check for max width violation
  const int max_width = tech_layer.getMaxWidth();
  if (width > max_width) {
    getLogger()->error(utl::PDN,
                       107,
                       "Width ({:.4f} um) specified for layer {} is greater "
                       "than maximum width ({:.4f} um).",
                       width / dbu_to_microns,
                       layer->getName(),
                       max_width / dbu_to_microns);
  }

  // check if width table in use and chekc widths
  bool check_table = false;
  const auto width_table = getWidthTable(&tech_layer);
  if (width_table.wrongdirection) {
    if (direction != layer->getDirection()) {
      check_table = true;
    }
  } else {
    check_table = true;
  }

  if (width_table.widths.empty()) {
    check_table = false;
  } else {
    if (width > width_table.widths.back()) {
      // width is outside of table
      check_table = false;
    }
  }

  bool found_width = false;
  for (auto table_width : width_table.widths) {
    if (table_width == width) {
      found_width = true;
    }
  }

  if (check_table && !found_width) {
    getLogger()->error(
        utl::PDN,
        114,
        "Width ({:.4f} um) specified for layer {} in not a valid width.",
        width / dbu_to_microns,
        layer->getName());
  }
}

GridShape::WidthTable GridShape::getWidthTable(const TechLayer* layer) const
{
  WidthTable table{false, false, {}};
  auto width_table = layer->tokenizeStringProperty("LEF58_WIDTHTABLE");
  if (width_table.empty()) {
    return table;
  }

  width_table.erase(
      std::find(width_table.begin(), width_table.end(), "WIDTHTABLE"));
  auto find_wrongdirection
      = std::find(width_table.begin(), width_table.end(), "WRONGDIRECTION");
  if (find_wrongdirection != width_table.end()) {
    table.wrongdirection = true;
    width_table.erase(find_wrongdirection);
  }
  auto find_orthogonal
      = std::find(width_table.begin(), width_table.end(), "ORTHOGONAL");
  if (find_orthogonal != width_table.end()) {
    table.orthogonal = true;
    width_table.erase(find_orthogonal);
  }

  for (const auto& width : width_table) {
    table.widths.push_back(layer->micronToDbu(width));
  }

  return table;
}

void GridShape::checkLayerSpacing(odb::dbTechLayer* layer,
                                  int width,
                                  int spacing,
                                  odb::dbTechLayerDir /* direction */) const
{
  const TechLayer tech_layer(layer);
  const double dbu_to_microns = tech_layer.getLefUnits();

  // check min spacing violation
  const int min_spacing = tech_layer.getSpacing(width);
  if (spacing < min_spacing) {
    getLogger()->error(utl::PDN,
                       108,
                       "Spacing ({:.4f} um) specified for layer {} is less "
                       "than minimum spacing ({:.4f} um).",
                       spacing / dbu_to_microns,
                       layer->getName(),
                       min_spacing / dbu_to_microns);
  }
}

}  // namespace pdn
