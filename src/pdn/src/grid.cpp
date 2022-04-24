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

#include "grid.h"

#include <boost/geometry.hpp>

#include "techlayer.h"
#include "connect.h"
#include "domain.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "rings.h"
#include "straps.h"
#include "utl/Logger.h"

namespace pdn {

namespace bgi = boost::geometry::index;

Grid::Grid(VoltageDomain* domain,
           const std::string& name,
           bool starts_with_power,
           const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : domain_(domain),
      name_(name),
      starts_with_power_(starts_with_power),
      allow_repair_channels_(false)
{
  obstruction_layers_.insert(generate_obstructions.begin(), generate_obstructions.end());
}

Grid::~Grid()
{
}

odb::dbBlock* Grid::getBlock() const
{
  return domain_->getBlock();
}

utl::Logger* Grid::getLogger() const
{
  return domain_->getLogger();
}

std::vector<odb::dbNet*> Grid::getNets(bool starts_with_power) const
{
  return domain_->getNets(starts_with_power);
}

const std::string Grid::typeToString(Type type)
{
  switch (type) {
    case Core:
      return "Core";
    case Instance:
      return "Instance";
    case Existing:
      return "Existing";
  }

  return "Unknown";
}

void Grid::addRing(std::unique_ptr<Rings> ring)
{
  if (ring != nullptr) {
    ring->setGrid(this);
    ring->checkLayerSpecifications();
    rings_.push_back(std::move(ring));
  }
}

void Grid::addStrap(std::unique_ptr<Straps> strap)
{
  if (strap != nullptr) {
    strap->setGrid(this);
    strap->checkLayerSpecifications();
    straps_.push_back(std::move(strap));
  }
}

void Grid::addConnect(std::unique_ptr<Connect> connect)
{
  if (connect != nullptr) {
    for (const auto& conn : connect_) {
      if (conn->getLowerLayer() == connect->getLowerLayer()
          && conn->getUpperLayer() == connect->getUpperLayer()) {
        getLogger()->error(
            utl::PDN,
            186,
            "Connect between layers {} and {} already exists in \"{}\".",
            connect->getLowerLayer()->getName(),
            connect->getUpperLayer()->getName(),
            getLongName());
      }
    }
    connect->setGrid(this);
    connect_.push_back(std::move(connect));
  }
}

void Grid::makeShapes(const ShapeTreeMap& global_shapes,
                      const ShapeTreeMap& obstructions)
{
  auto* logger = getLogger();
  logger->info(utl::PDN, 1, "Inserting grid: {}", getLongName());

  // copy obstructions
  ShapeTreeMap local_obstructions = obstructions;

  // make shapes
  for (auto* component : getGridComponents()) {
    // make initial shapes
    component->makeShapes(global_shapes);
    // cut shapes to avoid obstructions
    component->cutShapes(local_obstructions);
    // add obstructions to they are accounted for in future shapes
    component->getObstructions(local_obstructions);
  }

  // insert power switches
  // make vias
  makeVias(global_shapes, obstructions);

  // find and repair disconnected channels
  RepairChannelStraps::repairGridChannels(
      this, global_shapes, local_obstructions, allow_repair_channels_);

  // repair vias that are only partially overlapping straps
  if (repairVias(global_shapes, local_obstructions)) {
    // rebuild vias since shapes changed
    makeVias(global_shapes, obstructions);
  }
}

void Grid::makeRoutingObstructions(odb::dbBlock* block) const
{
  if (obstruction_layers_.empty()) {
    return;
  }

  const auto shapes = getShapes();
  for (auto* layer : obstruction_layers_) {
    auto itr = shapes.find(layer);
    if (itr == shapes.end()) {
      continue;
    }

    TechLayer techlayer(layer);
    techlayer.populateGrid(block);
    const bool is_horizontal = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    const int min_spacing = techlayer.getSpacing(0);

    for (const auto& [box, shape] : itr->second) {
      const auto& rect = shape->getRect();
      // bloat to block routing based on spacing
      const int width = is_horizontal ? rect.dy() : rect.dx();
      const int length = is_horizontal ? rect.dx() : rect.dy();

      const int width_spacing = techlayer.getSpacing(width, length);
      const int length_spacing = techlayer.getSpacing(length, width);

      int delta_x = is_horizontal ? length_spacing : width_spacing;
      int delta_y = is_horizontal ? width_spacing : length_spacing;

      if (is_horizontal) {
        delta_x -= min_spacing;
      } else {
        delta_y -= min_spacing;
      }

      const odb::Rect obs(
          rect.xMin() - delta_x,
          rect.yMin() - delta_y,
          rect.xMax() + delta_x,
          rect.yMax() + delta_y);

      if (techlayer.hasGrid()) {
        std::vector<int> grid = techlayer.getGrid();
        grid.erase(std::remove_if(grid.begin(), grid.end(), [&obs, is_horizontal](int pos) {
          if (is_horizontal) {
            return !(obs.yMin() <= pos && pos <= obs.yMax());
          } else {
            return !(obs.xMin() <= pos && pos <= obs.xMax());
          }
        }), grid.end());
        // add by tracks
        const int min_width = techlayer.getMinWidth();
        const int half0_min_width = min_width / 2;
        const int half1_min_width = min_width - half0_min_width;
        for (int track : grid) {
          const int low = track - half0_min_width;
          const int high = track + half1_min_width;
          odb::Rect new_obs = obs;
          if (is_horizontal) {
            new_obs.set_ylo(low);
            new_obs.set_yhi(high);
          } else {
            new_obs.set_xlo(low);
            new_obs.set_xhi(high);
          }
          odb::dbObstruction::create(block,
                                     layer,
                                     new_obs.xMin(),
                                     new_obs.yMin(),
                                     new_obs.xMax(),
                                     new_obs.yMax());
        }
      } else {
        // add blob
        odb::dbObstruction::create(block,
                                   layer,
                                   obs.xMin(),
                                   obs.yMin(),
                                   obs.xMax(),
                                   obs.yMax());
      }
    }
  }
}

bool Grid::repairVias(const ShapeTreeMap& global_shapes,
                      ShapeTreeMap& obstructions)
{
  debugPrint(getLogger(),
             utl::PDN,
             "ViaRepair",
             1,
             "Start via repair: {}",
             getLongName());
  // find vias that do not overlap completely
  // attempt to extend straps to fit (if owned by grid)
  std::map<Shape*, Shape*> replace_shapes;
  for (const auto& [box, via] : vias_) {
    // ensure shapes belong to something
    const auto& lower_shape = via->getLowerShape();
    if (lower_shape->getGridComponent() == nullptr) {
      continue;
    }
    const auto& upper_shape = via->getUpperShape();
    if (upper_shape->getGridComponent() == nullptr) {
      continue;
    }
    // ensure atleast one shape belongs to this grid
    const bool lower_belongs_to_grid
        = lower_shape->getGridComponent()->getGrid() == this;
    const bool upper_belongs_to_grid
        = upper_shape->getGridComponent()->getGrid() == this;
    if (!lower_belongs_to_grid && !upper_belongs_to_grid) {
      continue;
    }

    if (lower_belongs_to_grid && lower_shape->isModifiable()) {
      auto* new_lower = lower_shape->extendTo(
          upper_shape->getRect(), obstructions[lower_shape->getLayer()]);
      if (new_lower != nullptr) {
        replace_shapes[lower_shape.get()] = new_lower;
      }
    }
    if (upper_belongs_to_grid && upper_shape->isModifiable()) {
      auto* new_upper = upper_shape->extendTo(
          lower_shape->getRect(), obstructions[upper_shape->getLayer()]);
      if (new_upper != nullptr) {
        replace_shapes[upper_shape.get()] = new_upper;
      }
    }
  }

  for (const auto& [old_shape, new_shape] : replace_shapes) {
    auto* component = old_shape->getGridComponent();
    component->replaceShape(old_shape, {new_shape});
  }

  debugPrint(getLogger(),
             utl::PDN,
             "ViaRepair",
             1,
             "End via repair: {}",
             getLongName());
  return !replace_shapes.empty();
}

const ShapeTreeMap Grid::getShapes() const
{
  ShapeTreeMap shapes;

  for (auto* component : getGridComponents()) {
    for (const auto& [layer, component_shapes] : component->getShapes()) {
      auto& layer_shapes = shapes[layer];
      for (const auto& shape : component_shapes) {
        layer_shapes.insert(shape);
      }
    }
  }

  return shapes;
}

const odb::Rect Grid::getDomainArea() const
{
  return domain_->getDomainArea();
}

const odb::Rect Grid::getDomainBoundary() const
{
  return getDomainArea();
}

const odb::Rect Grid::getGridArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  odb::Rect rect;
  getBlock()->getDieArea(rect);
  return rect;
}

const odb::Rect Grid::getGridBoundary() const
{
  return getGridArea();
}

const odb::Rect Grid::getRingArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  // get the outline of the rings
  odb::Rect rect = getDomainBoundary();
  for (const auto& ring : rings_) {
    for (const auto& [layer, shapes] : ring->getShapes()) {
      for (const auto& [box, shape] : shapes) {
        const odb::Rect& ring_shape = shape->getRect();
        if (ring_shape.dx() == ring_shape.dy()) {
          rect.merge(ring_shape);
        } else if (ring_shape.dx() > ring_shape.dy()) {
          if (ring_shape.yMin() < rect.yMin()) {
            rect.set_ylo(ring_shape.yMin());
          }
          if (ring_shape.yMax() > rect.yMax()) {
            rect.set_yhi(ring_shape.yMax());
          }
        } else {
          if (ring_shape.xMin() < rect.xMin()) {
            rect.set_xlo(ring_shape.xMin());
          }
          if (ring_shape.xMax() > rect.xMax()) {
            rect.set_xhi(ring_shape.xMax());
          }
        }
      }
    }
  }

  return rect;
}

void Grid::report() const
{
  auto* logger = getLogger();
  logger->info(utl::PDN, 20, "Grid name: {}", getLongName());
  logger->info(utl::PDN, 21, "Type: {}", typeToString(type()));

  if (!rings_.empty()) {
    logger->info(utl::PDN, 22, "Rings:");
    for (const auto& ring : rings_) {
      ring->report();
    }
  }
  if (!straps_.empty()) {
    logger->info(utl::PDN, 23, "Straps:");
    for (const auto& strap : straps_) {
      strap->report();
    }
  }
  if (!connect_.empty()) {
    logger->info(utl::PDN, 24, "Connect:");
    for (const auto& connect : connect_) {
      connect->report();
    }
  }
  if (!pin_layers_.empty()) {
    std::string layers;
    for (auto* layer : pin_layers_) {
      if (!layers.empty()) {
        layers += " ";
      }
      layers += layer->getName();
    }
    logger->info(utl::PDN, 25, "Pin layers: {}", layers);
  }
  if (!obstruction_layers_.empty()) {
    std::string layers;
    for (auto* layer : pin_layers_) {
      if (!layers.empty()) {
        layers += " ";
      }
      layers += layer->getName();
    }
    logger->info(utl::PDN, 26, "Routing obstruction layers: {}", layers);
  }
}

void Grid::getIntersections(std::vector<ViaPtr>& shape_intersections,
                            const ShapeTreeMap& shapes) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             1,
             "Getting via intersections in \"{}\" - start",
             name_);
  // loop over connect statements
  for (const auto& connect : connect_) {
    odb::dbTechLayer* lower_layer = connect->getLowerLayer();
    odb::dbTechLayer* upper_layer = connect->getUpperLayer();

    // check if both layers have shapes
    if (shapes.count(lower_layer) == 0) {
      continue;
    }
    if (shapes.count(upper_layer) == 0) {
      continue;
    }

    const auto& lower_shapes = shapes.at(lower_layer);
    const auto& upper_shapes = shapes.at(upper_layer);

    debugPrint(getLogger(),
               utl::PDN,
               "Via",
               2,
               "Getting via intersections in \"{}\" - layers {} ({} shapes) - "
               "{} ({} shapes)",
               name_,
               lower_layer->getName(),
               lower_shapes.size(),
               upper_layer->getName(),
               upper_shapes.size());

    // loop over lower layer shapes
    for (const auto& [lower_box, lower_shape] : lower_shapes) {
      auto* lower_net = lower_shape->getNet();
      // check for intersections in higher layer shapes
      for (auto it = upper_shapes.qbegin(
               bgi::intersects(lower_box)
               && bgi::satisfies([lower_net](const auto& other) {
                    // not the same net, so ignore
                    return lower_net == other.second->getNet();
                  }));
           it != upper_shapes.qend();
           it++) {
        const auto& upper_shape = it->second;
        const odb::Rect via_rect
            = lower_shape->getRect().intersect(upper_shape->getRect());
        if (via_rect.area() == 0) {
          // intersection did not overlap, so ignore
          continue;
        }

        auto* via = new Via(connect.get(),
                            lower_shape->getNet(),
                            via_rect,
                            lower_shape,
                            upper_shape);
        shape_intersections.push_back(ViaPtr(via));
      }
    }
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             1,
             "Getting via intersections in \"{}\" - end",
             name_);
}

void Grid::resetShapes()
{
  vias_.clear();
  for (auto* component : getGridComponents()) {
    component->clearShapes();
  }

  for (const auto& connect : connect_) {
    connect->clearShapes();
  }
}

void Grid::checkSetup() const
{
  // check if follow pins have connect statements
  std::set<odb::dbTechLayer*> follow_pin_layers;
  for (const auto& strap : straps_) {
    if (strap->type() == Straps::Followpin) {
      follow_pin_layers.insert(strap->getLayer());
    }
  }
  if (follow_pin_layers.empty()) {
    return;
  }
  std::set<Connect*> follow_pin_connect;
  for (auto* lower : follow_pin_layers) {
    for (const auto& connect : connect_) {
      if (connect->getLowerLayer() != lower) {
        continue;
      }
      for (auto* upper : follow_pin_layers) {
        if (connect->getUpperLayer() == upper) {
          follow_pin_connect.insert(connect.get());
          break;
        }
      }
    }
  }

  if (follow_pin_layers.size() > 1 && follow_pin_connect.empty()) {
    // found no connect statements between followpins
    getLogger()->error(utl::PDN, 192, "There are multiple ({}) followpin definitions in {}, but no connect statements between them.", follow_pin_layers.size(), getName());
  }
  if (follow_pin_layers.size() - 1 != follow_pin_connect.size()) {
    getLogger()->error(utl::PDN, 193, "There are only ({}) followpin connect statements when {} is/are required.", follow_pin_connect.size(), follow_pin_layers.size() - 1);
  }

  for (auto* connect0 : follow_pin_connect) {
    for (auto* connect1 : follow_pin_connect) {
      if (connect0 == connect1) {
        continue;
      }

      // ensure order of connects is consistent
      const int c0_lower = connect0->getLowerLayer()->getRoutingLevel();
      const int c0_upper = connect0->getUpperLayer()->getRoutingLevel();
      const int c1_lower = connect1->getLowerLayer()->getRoutingLevel();
      const int c1_upper = connect1->getUpperLayer()->getRoutingLevel();
      if (std::tie(c0_lower, c0_upper) >
          std::tie(c1_lower, c1_upper)) {
        std::swap(connect0, connect1);
      }

      if (connect0->overlaps(connect1) || connect1->overlaps(connect0)) {
        getLogger()->error(utl::PDN,
                           194,
                           "Connect statements for followpins overlap between layers: {} -> {} and {} -> {}",
                           connect0->getLowerLayer()->getName(),
                           connect0->getUpperLayer()->getName(),
                           connect1->getLowerLayer()->getName(),
                           connect1->getUpperLayer()->getName());
      }
    }
  }
}

void Grid::getObstructions(ShapeTreeMap& obstructions) const
{
  for (const auto& [layer, shapes] : getShapes()) {
    auto& obs = obstructions[layer];
    for (const auto& [box, shape] : shapes) {
      obs.insert({shape->getObstructionBox(), shape});
    }
  }
}

void Grid::makeVias(const ShapeTreeMap& global_shapes,
                    const ShapeTreeMap& obstructions)
{
  debugPrint(getLogger(), utl::PDN, "Make", 1, "Making vias in \"{}\"", name_);
  ShapeTreeMap search_shapes = getShapes();

  odb::Rect search_area = getDomainBoundary();
  for (const auto& [layer, shapes] : search_shapes) {
    for (const auto& [box, shape] : shapes) {
      search_area.merge(shape->getRect());
    }
  }

  // populate shapes and obstructions
  Box search_box(Point(search_area.xMin(), search_area.yMin()),
                 Point(search_area.xMax(), search_area.yMax()));
  for (auto& [layer, layer_global_shape] : global_shapes) {
    auto& shapes = search_shapes[layer];
    for (auto it = layer_global_shape.qbegin(bgi::intersects(search_box));
         it != layer_global_shape.qend();
         it++) {
      shapes.insert(*it);
    }
  }

  ShapeTreeMap search_obstructions = obstructions;
  for (const auto& [layer, shapes] : search_shapes) {
    auto& obs = search_obstructions[layer];
    for (const auto& [box, search_shape] : shapes) {
      obs.insert({search_shape->getObstructionBox(), search_shape});
    }
  }

  // get possible vias
  std::vector<ViaPtr> vias;
  getIntersections(vias, search_shapes);

  auto remove_set_of_vias = [&vias](std::set<ViaPtr>& remove_vias) {
    auto remove
        = std::remove_if(vias.begin(), vias.end(), [&](const ViaPtr& via) {
            return remove_vias.count(via) != 0;
          });
    vias.erase(remove, vias.end());
    remove_vias.clear();
  };

  std::set<ViaPtr> remove_vias;
  // remove vias with obstructions in their stack
  for (const auto& via : vias) {
    for (auto* layer : via->getConnect()->getIntermediteLayers()) {
      auto& search_obs = search_obstructions[layer];
      if (search_obs.qbegin(bgi::intersects(via->getBox()))
          != search_obs.qend()) {
        remove_vias.insert(via);
        break;
      }
    }
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             2,
             "Removing {} vias due to obstructions.",
             remove_vias.size());
  remove_set_of_vias(remove_vias);

  // remove vias are only partially overlapping
  for (const auto& via : vias) {
    const auto& via_area = via->getArea();
    const auto& lower = via->getLowerShape()->getRect();
    const auto& upper = via->getUpperShape()->getRect();
    if (via_area.xMin() == lower.xMin() && via_area.xMax() == lower.xMax() &&
        via_area.yMin() == upper.yMin() && via_area.yMax() == upper.yMax()) {
      continue;
    } else if (via_area.yMin() == lower.yMin() && via_area.yMax() == lower.yMax() &&
               via_area.xMin() == upper.xMin() && via_area.xMax() == upper.xMax()) {
      continue;
    } else if (lower.contains(upper) || upper.contains(lower)) {
      continue;
    }
    remove_vias.insert(via);
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             2,
             "Removing {} vias due to partial overlap.",
             remove_vias.size());
  remove_set_of_vias(remove_vias);

  // build via tree
  vias_.clear();
  for (auto& via : vias) {
    vias_.insert({via->getBox(), via});
    via->getLowerShape()->addVia(via);
    via->getUpperShape()->addVia(via);
  }
}

void Grid::getVias(std::vector<ViaPtr>& vias) const
{
  for (const auto& [box, via] : vias_) {
    vias.push_back(via);
  }
}

void Grid::removeVia(const ViaPtr& via)
{
  auto find
      = std::find_if(vias_.begin(), vias_.end(), [via](const auto& other) {
          return via == other.second;
        });
  if (find != vias_.end()) {
    vias_.remove(*find);
  }
}

void Grid::removeInvalidVias()
{
  std::vector<ViaValue> remove_vias;
  for (const auto& via_value : vias_) {
    if (!via_value.second->isValid()) {
      remove_vias.push_back(via_value);
    }
  }
  for (const auto& remove_via : remove_vias) {
    vias_.remove(remove_via);
  }
}

const std::vector<GridComponent*> Grid::getGridComponents() const
{
  std::vector<GridComponent*> components;
  for (const auto& ring : rings_) {
    components.push_back(ring.get());
  }

  for (const auto& strap : straps_) {
    components.push_back(strap.get());
  }

  return components;
}

void Grid::writeToDb(const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
                     bool do_pins) const
{
  // write vias first do shapes can be adjusted if needed
  std::vector<ViaPtr> vias;
  getVias(vias);
  // sort the vias so they are written to db in the same order
  std::sort(vias.begin(), vias.end(), [](const auto& l, const auto& r) {
    auto* l_low_layer = l->getLowerLayer();
    int l_low_level = l_low_layer->getNumber();
    auto* l_high_layer = l->getUpperLayer();
    int l_high_level = l_high_layer->getNumber();
    const odb::Rect l_area = l->getArea();

    auto* r_low_layer = r->getLowerLayer();
    int r_low_level = r_low_layer->getNumber();
    auto* r_high_layer = r->getUpperLayer();
    int r_high_level = r_high_layer->getNumber();
    const odb::Rect r_area = r->getArea();

    return std::tie(l_low_level, l_high_level, l_area)
           < std::tie(r_low_level, r_high_level, r_area);
  });
  for (const auto& via : vias) {
    via->writeToDb(net_map.at(via->getNet()), getBlock());
  }
  for (const auto& connect : connect_) {
    connect->printViaReport();
  }

  std::set<odb::dbTechLayer*> pin_layers(pin_layers_.begin(),
                                         pin_layers_.end());
  for (auto* component : getGridComponents()) {
    component->writeToDb(net_map, do_pins, pin_layers);
  }
}

void Grid::getGridLevelObstructions(ShapeTreeMap& obstructions) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Obs",
             1,
             "Collecting grid obstructions from: {}",
             name_);
  const odb::Rect core = getDomainArea();

  std::set<odb::dbTechLayer*> layers;

  for (const auto& strap : straps_) {
    layers.insert(strap->getLayer());
  }

  for (const auto& connect : connect_) {
    for (auto* layer : connect->getIntermediteRoutingLayers()) {
      layers.insert(layer);
    }
  }

  for (auto* layer : layers) {
    auto obs = std::make_shared<Shape>(layer, core, Shape::GRID_OBS);
    debugPrint(getLogger(),
               utl::PDN,
               "Obs",
               2,
               "Adding obstruction on layer {} covering {}",
               layer->getName(),
               Shape::getRectText(core, getBlock()->getDbUnitsPerMicron()));
    obstructions[layer].insert({obs->getObstructionBox(), obs});
  }

  for (const auto& ring : rings_) {
    int hor_size, ver_size;
    ring->getTotalWidth(hor_size, ver_size);
    auto offset = ring->getOffset();

    const odb::Rect ring_rect(core.xMin() - ver_size - offset[0],
                              core.yMin() - hor_size - offset[1],
                              core.xMax() + ver_size + offset[2],
                              core.yMax() + hor_size + offset[3]);
    for (auto* layer : ring->getLayers()) {
      auto obs = std::make_shared<Shape>(layer, ring_rect, Shape::GRID_OBS);
      obs->generateObstruction();
      debugPrint(
          getLogger(),
          utl::PDN,
          "Obs",
          2,
          "Adding obstruction on layer {} covering {}",
          layer->getName(),
          Shape::getRectText(ring_rect, getBlock()->getDbUnitsPerMicron()));
      obstructions[layer].insert({obs->getObstructionBox(), obs});
    }
  }
}

void Grid::makeInitialObstructions(odb::dbBlock* block, ShapeTreeMap& obs)
{
  // routing obs
  for (auto* ob : block->getObstructions()) {
    if (ob->isSlotObstruction() || ob->isFillObstruction()) {
      continue;
    }

    auto* box = ob->getBBox();
    odb::Rect obs_rect;
    box->getBox(obs_rect);
    if (ob->hasMinSpacing()) {
      obs_rect.bloat(ob->getMinSpacing(), obs_rect);
    }

    if (box->getTechLayer() == nullptr) {
      for (auto* layer : block->getDb()->getTech()->getLayers()) {
        auto shape = std::make_shared<Shape>(layer, obs_rect, Shape::BLOCK_OBS);
        obs[shape->getLayer()].insert({shape->getObstructionBox(), shape});
      }
    } else {
      auto shape = std::make_shared<Shape>(
          box->getTechLayer(), obs_rect, Shape::BLOCK_OBS);

      obs[shape->getLayer()].insert({shape->getObstructionBox(), shape});
    }
  }

  // placed block obs
  for (auto* inst : block->getInsts()) {
    if (!inst->isBlock()) {
      continue;
    }

    if (!inst->isPlaced()) {
      continue;
    }

    for (const auto& [layer, shapes] : InstanceGrid::getInstanceObstructions(inst)) {
      obs[layer].insert(shapes.begin(), shapes.end());
    }
  }
}

void Grid::makeInitialShapes(const std::set<odb::dbNet*>& nets,
                             ShapeTreeMap& shapes)
{
  for (auto* net : nets) {
    Shape::populateMapFromDb(net, shapes);
  }
}

std::set<odb::dbTechLayer*> Grid::connectableLayers(
    odb::dbTechLayer* layer) const
{
  std::set<odb::dbTechLayer*> layers;

  for (const auto& connect : connect_) {
    if (connect->getLowerLayer() == layer) {
      layers.insert(connect->getUpperLayer());
    } else if (connect->getUpperLayer() == layer) {
      layers.insert(connect->getLowerLayer());
    }
  }
  return layers;
}

///////////////

CoreGrid::CoreGrid(VoltageDomain* domain,
                   const std::string& name,
                   bool start_with_power,
                   const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : Grid(domain, name, start_with_power, generate_obstructions)
{
}

const odb::Rect CoreGrid::getDomainBoundary() const
{
  // account for the width of the follow pins for straps
  odb::Rect core = Grid::getDomainBoundary();

  int follow_pin_width = 0;
  for (const auto& strap : getStraps()) {
    if (strap->type() == GridComponent::Followpin) {
      follow_pin_width = std::max(follow_pin_width, strap->getWidth());
    }
  }

  core.bloat(follow_pin_width / 2, core);
  return core;
}

void CoreGrid::setupDirectConnect(
    const std::vector<odb::dbTechLayer*>& connect_pad_layers)
{
  // look for pads that need to be connected
  for (auto* net : getNets()) {
    std::vector<odb::dbITerm*> iterms;
    for (auto* iterm : net->getITerms()) {
      auto* inst = iterm->getInst();
      if (!inst->getPlacementStatus().isPlaced()) {
        continue;
      }

      if (!inst->getMaster()->isPad()) {
        continue;
      }

      iterms.push_back(iterm);
    }

    // sort by name to keep stable
    std::stable_sort(
        iterms.begin(), iterms.end(), [](odb::dbITerm* l, odb::dbITerm* r) {
          const int name_compare = r->getInst()->getName().compare(l->getInst()->getName());
          if (name_compare != 0) {
            return name_compare < 0;
          }

          return r->getMTerm()->getName() < l->getMTerm()->getName();
        });

    for (auto* iterm : iterms) {
      auto pad_connect = std::make_unique<PadDirectConnectionStraps>(
          this, iterm, connect_pad_layers);
      if (pad_connect->canConnect()) {
        addStrap(std::move(pad_connect));
      }
    }
  }
}

void CoreGrid::getGridLevelObstructions(ShapeTreeMap& obstructions) const
{
  if (getDomain()->hasRegion()) {
    // core grids only have grid level obstructions if they have a region
    Grid::getGridLevelObstructions(obstructions);
  }
}

///////////////

InstanceGrid::InstanceGrid(VoltageDomain* domain,
                           const std::string& name,
                           bool start_with_power,
                           odb::dbInst* inst,
                           const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : Grid(domain, name, start_with_power, generate_obstructions),
      inst_(inst),
      halos_({0, 0, 0, 0}),
      grid_to_boundary_(false),
      replaceable_(false)
{
  auto* halo = inst->getHalo();
  if (halo != nullptr) {
    odb::Rect halo_box;
    halo->getBox(halo_box);

    odb::Rect inst_box;
    inst->getBBox()->getBox(inst_box);

    // copy halo from db
    addHalo({halo_box.xMin() - inst_box.xMin(),
             halo_box.yMin() - inst_box.yMin(),
             inst_box.xMin() - halo_box.xMax(),
             inst_box.yMin() - halo_box.yMax()});
  }
}

const std::string InstanceGrid::getLongName() const
{
  return getName() + " - " + inst_->getName();
}

void InstanceGrid::addHalo(const std::array<int, 4>& halos)
{
  halos_ = halos;
}

void InstanceGrid::setGridToBoundary(bool value)
{
  grid_to_boundary_ = value;
}

const odb::Rect InstanceGrid::getDomainArea() const
{
  auto* bbox = inst_->getBBox();
  odb::Rect inst_box;
  bbox->getBox(inst_box);

  return inst_box;
}

const odb::Rect InstanceGrid::getDomainBoundary() const
{
  if (grid_to_boundary_) {
    // use instance boundary
    return getDomainArea();
  } else {
    // find the outline of the supply pins
    odb::Rect pin_box;
    pin_box.mergeInit();

    odb::dbTransform transform;
    inst_->getTransform(transform);

    for (auto* pin : inst_->getMaster()->getMTerms()) {
      if (!pin->getSigType().isSupply()) {
        continue;
      }

      pin_box.merge(pin->getBBox());
    }

    transform.apply(pin_box);

    return pin_box;
  }
}

const odb::Rect InstanceGrid::getGridArea() const
{
  odb::Rect inst_box = getDomainArea();

  // apply halo
  inst_box.set_xlo(inst_box.xMin() - halos_[0]);
  inst_box.set_ylo(inst_box.yMin() - halos_[1]);
  inst_box.set_xhi(inst_box.xMax() + halos_[2]);
  inst_box.set_yhi(inst_box.yMax() + halos_[3]);

  return inst_box;
}

const odb::Rect InstanceGrid::getGridBoundary() const
{
  return getDomainBoundary();
}

ShapeTreeMap InstanceGrid::getInstanceObstructions(odb::dbInst* inst)
{
  ShapeTreeMap obs;

  odb::dbTransform transform;
  inst->getTransform(transform);

  auto* master = inst->getMaster();

  for (auto* ob : master->getObstructions()) {
    odb::Rect obs_rect;
    ob->getBox(obs_rect);

    // add min spacing
    auto* layer = ob->getTechLayer();
    obs_rect.bloat(layer->getSpacing(), obs_rect);

    transform.apply(obs_rect);
    auto shape = std::make_shared<Shape>(layer, obs_rect, Shape::BLOCK_OBS);

    obs[layer].insert({shape->getObstructionBox(), shape});
  }

  for (auto* mterm : master->getMTerms()) {
    for (auto* mpin : mterm->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        odb::Rect obs_rect;
        box->getBox(obs_rect);

        // add min spacing
        auto* layer = box->getTechLayer();
        obs_rect.bloat(layer->getSpacing(), obs_rect);

        transform.apply(obs_rect);
        auto shape = std::make_shared<Shape>(layer, obs_rect, Shape::BLOCK_OBS);
        obs[layer].insert({shape->getObstructionBox(), shape});
      }
    }
  }

  return obs;
}

void InstanceGrid::getGridLevelObstructions(ShapeTreeMap& obstructions) const
{
  Grid::getGridLevelObstructions(obstructions);

  const odb::Rect inst_box = getGridArea();

  // copy layer obs
  for (const auto& [layer, shapes] : obstructions) {
    auto obs = std::make_shared<Shape>(layer, inst_box, Shape::GRID_OBS);
    obstructions[layer].insert({obs->getObstructionBox(), obs});
  }

  // add obstruction covering ensure instance
  for (const auto& [layer, shapes] : getInstanceObstructions(inst_)) {
    auto obs = std::make_shared<Shape>(layer, inst_box, Shape::GRID_OBS);
    obstructions[layer].insert({obs->getObstructionBox(), obs});
  }
}

ShapeTreeMap InstanceGrid::getInstancePins(odb::dbInst* inst)
{
  // add instance pins
  std::vector<ShapePtr> pins;
  odb::dbTransform transform;
  inst->getTransform(transform);
  for (auto* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net != nullptr) {
      for (auto* mpin : iterm->getMTerm()->getMPins()) {
        for (auto* box : mpin->getGeometry()) {
          odb::Rect box_rect;
          box->getBox(box_rect);
          transform.apply(box_rect);
          pins.push_back(
              std::make_shared<Shape>(box->getTechLayer(), net, box_rect));
        }
      }
    }
  }

  ShapeTreeMap shapes;
  for (auto& pin : pins) {
    shapes[pin->getLayer()].insert({pin->getRectBox(), pin});
  }

  return shapes;
}

void InstanceGrid::getIntersections(std::vector<ViaPtr>& vias,
                                    const ShapeTreeMap& shapes) const
{
  // add instance pins
  ShapeTreeMap inst_shapes = shapes;
  for (auto* net : getNets()) {
    for (const auto& [layer, shapes_on_layer] : getInstancePins(inst_)) {
      auto& layer_shapes = inst_shapes[layer];
      for (const auto& [box, shape] : shapes_on_layer) {
        if (shape->getNet() == net) {
          layer_shapes.insert({box, shape});
        }
      }
    }
  }

  Grid::getIntersections(vias, inst_shapes);
}

////////

ExistingGrid::ExistingGrid(PdnGen* pdngen,
                           odb::dbBlock* block,
                           utl::Logger* logger,
                           const std::string& name,
                           const std::vector<odb::dbTechLayer*>& generate_obstructions)
  : Grid(nullptr, name, false, generate_obstructions),
    shapes_(),
    domain_(nullptr)
{
  std::set<odb::dbNet*> nets;

  for (odb::dbNet* net : block->getNets()) {
    if (!net->getSigType().isSupply()) {
      continue;
    }

    nets.insert(net);
  }

  odb::dbNet* power = nullptr;
  odb::dbNet* ground = nullptr;

  // find a power and ground net for the domain
  for (auto* net : nets) {
    if (power == nullptr) {
      if (net->getSigType() == odb::dbSigType::POWER) {
        power = net;
      }
    }
    if (ground == nullptr) {
      if (net->getSigType() == odb::dbSigType::GROUND) {
        ground = net;
      }
    }
  }
  if (power != nullptr) {
    nets.erase(power);
  }
  if (ground != nullptr) {
    nets.erase(ground);
  }

  std::vector<odb::dbNet*> nets_vec;
  nets_vec.insert(nets_vec.end(), nets.begin(), nets.end());
  domain_ = std::make_unique<VoltageDomain>(pdngen,
                                            block,
                                            power,
                                            ground,
                                            nets_vec,
                                            logger);

  setDomain(domain_.get());

  populate();
}

void ExistingGrid::populate()
{
  const auto nets = getNets();
  std::set<odb::dbNet*> nets_set(nets.begin(), nets.end());
  Grid::makeInitialShapes(nets_set, shapes_);

  for (auto* inst : getBlock()->getInsts()) {
    if (inst->getPlacementStatus().isFixed()) {
      for (const auto& [layer, shapes] : InstanceGrid::getInstancePins(inst)) {
        shapes_[layer].insert(shapes.begin(), shapes.end());
      }
    }
  }
}

void ExistingGrid::addRing(std::unique_ptr<Rings> ring)
{
  addGridComponent(ring.get());
}

void ExistingGrid::addStrap(std::unique_ptr<Straps> strap)
{
  addGridComponent(strap.get());
}

void ExistingGrid::addGridComponent(GridComponent* component) const
{
  getLogger()->error(utl::PDN, 188, "Existing grid does not support adding {}.", GridComponent::typeToString(component->type()));
}

}  // namespace pdn
