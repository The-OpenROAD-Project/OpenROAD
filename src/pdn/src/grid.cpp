// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "grid.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "connect.h"
#include "domain.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/isotropy.h"
#include "power_cells.h"
#include "rings.h"
#include "shape.h"
#include "straps.h"
#include "techlayer.h"
#include "utl/Logger.h"
#include "via.h"

namespace pdn {

namespace bgi = boost::geometry::index;

Grid::Grid(VoltageDomain* domain,
           const std::string& name,
           bool starts_with_power,
           const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : domain_(domain), name_(name), starts_with_power_(starts_with_power)
{
  obstruction_layers_ = generate_obstructions;
}

Grid::~Grid() = default;

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

std::string Grid::typeToString(Type type)
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

void Grid::makeShapes(const Shape::ShapeTreeMap& global_shapes,
                      const Shape::ObstructionTreeMap& obstructions)
{
  auto* logger = getLogger();
  logger->info(utl::PDN, 1, "Inserting grid: {}", getLongName());

  // copy obstructions
  Shape::ObstructionTreeMap local_obstructions = obstructions;

  Shape::ShapeTreeMap local_shapes = global_shapes;
  // make shapes
  std::vector<GridComponent*> deferred;
  for (auto* component : getGridComponents()) {
    if (!component->make(local_shapes, local_obstructions)) {
      debugPrint(logger,
                 utl::PDN,
                 "Make",
                 2,
                 "Deferring shape creation for component in \"{}\".",
                 getName());
      deferred.push_back(component);
    }
  }
  // make deferred components
  for (auto* component : deferred) {
    component->make(local_shapes, local_obstructions);
  }

  // refine shapes
  bool modified = false;
  do {
    modified = false;
    for (auto* component : getGridComponents()) {
      // attempt to refine shapes
      const bool comp_modified
          = component->refineShapes(local_shapes, local_obstructions);

      modified |= comp_modified;
    }
  } while (modified);

  Shape::ShapeTreeMap all_shapes = global_shapes;
  // insert power switches
  if (switched_power_cell_ != nullptr) {
    switched_power_cell_->build();
    for (const auto& [layer, cell_shapes] : switched_power_cell_->getShapes()) {
      auto& layer_shapes = all_shapes[layer];
      layer_shapes.insert(cell_shapes.begin(), cell_shapes.end());
    }
  }

  // Remove any poorly formed shapes
  cleanupShapes();

  // make vias
  makeVias(all_shapes, obstructions, local_obstructions);

  // find and repair disconnected channels
  RepairChannelStraps::repairGridChannels(
      this,
      all_shapes,
      local_obstructions,
      allow_repair_channels_,
      domain_->getPDNGen()->getDebugRenderer());
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
    const bool is_horizontal
        = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    const int min_width = techlayer.getMinWidth();
    const int min_spacing = techlayer.getSpacing(0);

    std::vector<ShapePtr> all_shapes;
    for (const auto& shape_value : itr->second) {
      all_shapes.push_back(shape_value);
    }

    // sort shapes so they get written to db in the same order.  Shapes
    // are non-overlapping so comparing one corner should be a total order.
    std::ranges::sort(all_shapes, [](const auto& l, const auto& r) {
      auto lc = l->getRect().ll();
      auto rc = r->getRect().ll();
      return lc < rc;
    });

    for (const auto& shape : all_shapes) {
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
        delta_y += min_width;
      } else {
        delta_x += min_width;
        delta_y -= min_spacing;
      }

      const odb::Rect obs(rect.xMin() - delta_x,
                          rect.yMin() - delta_y,
                          rect.xMax() + delta_x,
                          rect.yMax() + delta_y);

      if (techlayer.hasGrid()) {
        std::vector<int> grid = techlayer.getGrid();
        std::erase_if(grid, [&obs, is_horizontal](int pos) {
          if (is_horizontal) {
            return !(obs.yMin() <= pos && pos <= obs.yMax());
          }
          return !(obs.xMin() <= pos && pos <= obs.xMax());
        });
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
        odb::dbObstruction::create(
            block, layer, obs.xMin(), obs.yMin(), obs.xMax(), obs.yMax());
      }
    }
  }
}

bool Grid::repairVias(const Shape::ShapeTreeMap& global_shapes,
                      Shape::ObstructionTreeMap& obstructions)
{
  debugPrint(getLogger(),
             utl::PDN,
             "ViaRepair",
             1,
             "Start via repair: {}",
             getLongName());
  // find vias that do not overlap completely
  // attempt to extend straps to fit (if owned by grid)

  auto obs_filter = [this](const ShapePtr& other) -> bool {
    if (other->shapeType() != Shape::GRID_OBS) {
      return true;
    }
    const GridObsShape* shape = static_cast<GridObsShape*>(other.get());
    return !shape->belongsTo(this);
  };

  std::map<Shape*, std::unique_ptr<Shape>> replace_shapes;
  for (const auto& via : vias_) {
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
      Shape* extend_test = lower_shape.get();
      auto find_replace = replace_shapes.find(extend_test);
      if (find_replace != replace_shapes.end()) {
        extend_test = find_replace->second.get();
      }
      auto new_lower
          = extend_test->extendTo(upper_shape->getRect(),
                                  obstructions[extend_test->getLayer()],
                                  lower_shape.get(),
                                  obs_filter);
      if (new_lower != nullptr) {
        replace_shapes[lower_shape.get()] = std::move(new_lower);
      }
    }
    if (upper_belongs_to_grid && upper_shape->isModifiable()) {
      Shape* extend_test = upper_shape.get();
      auto find_replace = replace_shapes.find(extend_test);
      if (find_replace != replace_shapes.end()) {
        extend_test = find_replace->second.get();
      }
      auto new_upper
          = extend_test->extendTo(lower_shape->getRect(),
                                  obstructions[extend_test->getLayer()],
                                  upper_shape.get(),
                                  obs_filter);
      if (new_upper != nullptr) {
        replace_shapes[upper_shape.get()] = std::move(new_upper);
      }
    }
  }

  for (auto& [old_shape, new_shape] : replace_shapes) {
    auto* component = old_shape->getGridComponent();
    component->replaceShape(old_shape, std::move(new_shape));
  }

  debugPrint(getLogger(),
             utl::PDN,
             "ViaRepair",
             1,
             "End via repair: {}",
             getLongName());
  return !replace_shapes.empty();
}

Shape::ShapeTreeMap Grid::getShapes() const
{
  ShapeVectorMap shapes;

  for (auto* component : getGridComponents()) {
    for (const auto& [layer, component_shapes] : component->getShapes()) {
      shapes[layer].insert(shapes[layer].end(),
                           component_shapes.begin(),
                           component_shapes.end());
    }
  }

  return Shape::convertVectorToTree(shapes);
}

odb::Rect Grid::getDomainArea() const
{
  return domain_->getDomainArea();
}

odb::Rect Grid::getDomainBoundary() const
{
  return getDomainArea();
}

odb::Rect Grid::getGridArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  odb::Rect rect = getBlock()->getDieArea();
  return rect;
}

odb::Rect Grid::getGridBoundary() const
{
  return getGridArea();
}

odb::Rect Grid::getRingArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  // get the outline of the rings
  odb::Rect rect = getDomainBoundary();
  for (const auto& ring : rings_) {
    for (const auto& [layer, shapes] : ring->getShapes()) {
      for (const auto& shape : shapes) {
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
  logger->report("Grid name: {}", getLongName());
  logger->report("Type: {}", typeToString(type()));

  if (!rings_.empty()) {
    logger->report("Rings:");
    for (const auto& ring : rings_) {
      ring->report();
    }
  }
  if (!straps_.empty()) {
    logger->report("Straps:");
    for (const auto& strap : straps_) {
      strap->report();
    }
  }
  if (!connect_.empty()) {
    std::vector<Connect*> connect;
    connect.reserve(connect_.size());
    for (const auto& conn : connect_) {
      connect.push_back(conn.get());
    }
    std::ranges::sort(connect, [](const Connect* l, const Connect* r) {
      int l_lower = l->getLowerLayer()->getRoutingLevel();
      int l_upper = l->getUpperLayer()->getRoutingLevel();
      int r_lower = r->getLowerLayer()->getRoutingLevel();
      int r_upper = r->getUpperLayer()->getRoutingLevel();
      return std::tie(l_lower, l_upper) < std::tie(r_lower, r_upper);
    });
    logger->report("Connect:");
    for (Connect* conn : connect) {
      conn->report();
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
    logger->report("Pin layers: {}", layers);
  }
  if (!obstruction_layers_.empty()) {
    std::string layers;
    for (auto* layer : pin_layers_) {
      if (!layers.empty()) {
        layers += " ";
      }
      layers += layer->getName();
    }
    logger->report("Routing obstruction layers: {}", layers);
  }
  if (switched_power_cell_ != nullptr) {
    switched_power_cell_->report();
  }
}

void Grid::getIntersections(std::vector<ViaPtr>& shape_intersections,
                            const Shape::ShapeTreeMap& search_shapes) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             1,
             "Getting via intersections in \"{}\" - start",
             name_);

  Shape::ShapeTreeMap shapes = search_shapes;
  // Populate with additional shapes from grid components
  for (auto* comp : getGridComponents()) {
    comp->getConnectableShapes(shapes);
  }

  // loop over connect statements
  for (const auto& connect : connect_) {
    odb::dbTechLayer* lower_layer = connect->getLowerLayer();
    odb::dbTechLayer* upper_layer = connect->getUpperLayer();

    // check if both layers have shapes
    if (!shapes.contains(lower_layer)) {
      continue;
    }
    if (!shapes.contains(upper_layer)) {
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
    for (const auto& lower_shape : lower_shapes) {
      auto* lower_net = lower_shape->getNet();
      // check for intersections in higher layer shapes
      for (auto it = upper_shapes.qbegin(
               bgi::intersects(lower_shape->getRect())
               && bgi::satisfies([lower_net](const auto& other) {
                    // not the same net, so ignore
                    return lower_net == other->getNet();
                  }));
           it != upper_shapes.qend();
           it++) {
        const auto& upper_shape = *it;
        if (!lower_shape->getRect().overlaps(upper_shape->getRect())) {
          // no overlap, so ignore
          continue;
        }

        const odb::Rect via_rect
            = lower_shape->getRect().intersect(upper_shape->getRect());
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
  std::set<GridComponent*> remove;
  for (auto* component : getGridComponents()) {
    component->clearShapes();

    if (component->isAutoInserted()) {
      remove.insert(component);
    }
  }

  for (auto* component : remove) {
    removeGridComponent(component);
  }

  for (const auto& connect : connect_) {
    connect->clearShapes();
  }
}

void Grid::ripup()
{
  if (switched_power_cell_ != nullptr) {
    switched_power_cell_->ripup();
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
    getLogger()->error(utl::PDN,
                       192,
                       "There are multiple ({}) followpin definitions in {}, "
                       "but no connect statements between them.",
                       follow_pin_layers.size(),
                       getName());
  }
  if (follow_pin_layers.size() - 1 != follow_pin_connect.size()) {
    getLogger()->error(utl::PDN,
                       193,
                       "There are only ({}) followpin connect statements when "
                       "{} is/are required.",
                       follow_pin_connect.size(),
                       follow_pin_layers.size() - 1);
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
      if (std::tie(c0_lower, c0_upper) > std::tie(c1_lower, c1_upper)) {
        std::swap(connect0, connect1);
      }

      if (connect0->overlaps(connect1) || connect1->overlaps(connect0)) {
        getLogger()->error(utl::PDN,
                           194,
                           "Connect statements for followpins overlap between "
                           "layers: {} -> {} and {} -> {}",
                           connect0->getLowerLayer()->getName(),
                           connect0->getUpperLayer()->getName(),
                           connect1->getLowerLayer()->getName(),
                           connect1->getUpperLayer()->getName());
      }
    }
  }

  // Check connectivity
  std::set<odb::dbTechLayer*> check_layers;
  for (const auto& ring : rings_) {
    for (auto* layer : ring->getLayers()) {
      check_layers.insert(layer);
    }
  }
  for (const auto& strap : straps_) {
    check_layers.insert(strap->getLayer());
  }

  // Check that pin layers actually exists in stack
  for (auto* layer : pin_layers_) {
    if (check_layers.find(layer) == check_layers.end()) {
      getLogger()->error(utl::PDN,
                         111,
                         "Pin layer {} is not a valid shape in {}",
                         layer->getName(),
                         name_);
    }
  }

  // add instance layers
  const auto nets_vec = getNets();
  const std::set<odb::dbNet*> nets(nets_vec.begin(), nets_vec.end());

  for (auto* inst : getInstances()) {
    if (!inst->isFixed()) {
      continue;
    }
    for (auto* iterm : inst->getITerms()) {
      if (nets.find(iterm->getNet()) != nets.end()) {
        for (const auto& [layer, shape] : iterm->getGeometries()) {
          check_layers.insert(layer);
        }
      }
    }
  }
  if (domain_->hasSwitchedPower()) {
    for (const auto& powercell :
         domain_->getPDNGen()->getSwitchedPowerCells()) {
      for (auto* mterm : powercell->getMaster()->getMTerms()) {
        for (auto* mpin : mterm->getMPins()) {
          for (auto* box : mpin->getGeometry()) {
            auto* layer = box->getTechLayer();
            if (layer) {
              check_layers.insert(layer);
            }
          }
        }
      }
    }
  }

  // add bterms and exisiting routing
  for (auto* net : nets) {
    for (auto* swire : net->getSWires()) {
      for (auto* box : swire->getWires()) {
        auto* layer = box->getTechLayer();
        if (layer) {
          check_layers.insert(layer);
        }
      }
    }
    for (auto* bterm : net->getBTerms()) {
      for (auto* bpin : bterm->getBPins()) {
        if (!bpin->getPlacementStatus().isFixed()) {
          continue;
        }
        for (auto* box : bpin->getBoxes()) {
          auto* layer = box->getTechLayer();
          if (layer) {
            check_layers.insert(layer);
          }
        }
      }
    }
  }

  // Check that connect statement actually point to something
  for (const auto& connect : connect_) {
    if (check_layers.find(connect->getLowerLayer()) == check_layers.end()) {
      getLogger()->error(utl::PDN,
                         112,
                         "Cannot find shapes to connect to on {}",
                         connect->getLowerLayer()->getName());
    }
    if (check_layers.find(connect->getUpperLayer()) == check_layers.end()) {
      getLogger()->error(utl::PDN,
                         113,
                         "Cannot find shapes to connect to on {}",
                         connect->getUpperLayer()->getName());
    }
  }
}

void Grid::getObstructions(Shape::ObstructionTreeMap& obstructions) const
{
  for (const auto& [layer, shapes] : getShapes()) {
    auto& obs = obstructions[layer];
    obs.insert(shapes.begin(), shapes.end());
  }
}

void Grid::makeVias(const Shape::ShapeTreeMap& global_shapes,
                    const Shape::ObstructionTreeMap& obstructions,
                    Shape::ObstructionTreeMap& local_obstructions)
{
  makeVias(global_shapes, obstructions);

  // repair vias that are only partially overlapping straps
  if (repairVias(global_shapes, local_obstructions)) {
    // rebuild vias since shapes changed
    makeVias(global_shapes, obstructions);
  }
}

void Grid::makeVias(const Shape::ShapeTreeMap& global_shapes,
                    const Shape::ObstructionTreeMap& obstructions)
{
  debugPrint(
      getLogger(), utl::PDN, "Make", 1, "Making vias in \"{}\" - start", name_);
  Shape::ShapeTreeMap search_shapes = getShapes();

  odb::Rect search_area = getDomainBoundary();
  for (const auto& [layer, shapes] : search_shapes) {
    for (const auto& shape : shapes) {
      search_area.merge(shape->getRect());
    }
  }

  // populate shapes and obstructions
  for (auto& [layer, layer_global_shape] : global_shapes) {
    auto& shapes = search_shapes[layer];
    for (auto it = layer_global_shape.qbegin(bgi::intersects(search_area));
         it != layer_global_shape.qend();
         it++) {
      shapes.insert(*it);
    }
  }

  auto obs_filter = [this](const ShapePtr& other) -> bool {
    if (other->shapeType() != Shape::GRID_OBS) {
      return true;
    }
    const GridObsShape* shape = static_cast<GridObsShape*>(other.get());
    return !shape->belongsTo(this);
  };

  Shape::ObstructionTreeMap search_obstructions = obstructions;
  for (const auto& [layer, shapes] : search_shapes) {
    auto& obs = search_obstructions[layer];
    for (auto& search_shape : shapes) {
      obs.insert(search_shape);
    }
  }

  // get possible vias
  std::vector<ViaPtr> vias;
  getIntersections(vias, search_shapes);

  auto remove_set_of_vias = [&vias](std::set<ViaPtr>& remove_vias) {
    std::erase_if(vias,
                  [&](const ViaPtr& via) { return remove_vias.contains(via); });
    remove_vias.clear();
  };

  std::set<ViaPtr> remove_vias;
  // remove vias with obstructions in their stack
  for (const auto& via : vias) {
    for (auto* layer : via->getConnect()->getIntermediteLayers()) {
      auto& search_obs = search_obstructions[layer];
      if (search_obs.qbegin(bgi::intersects(via->getArea())
                            && bgi::satisfies(obs_filter))
          != search_obs.qend()) {
        remove_vias.insert(via);
        via->markFailed(failedViaReason::OBSTRUCTED);
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

  // Remove overlapping vias and keep largest
  Via::ViaTree overlapping_via_tree;
  for (const auto& via : vias) {
    overlapping_via_tree.insert(via);
  }
  for (const auto& via : vias) {
    if (via->isFailed()) {
      continue;
    }
    if (overlapping_via_tree.qbegin(
            bgi::intersects(via->getArea())
            && bgi::satisfies([&via](const ViaPtr& other) -> bool {
                 if (via == other) {
                   // ignore the same via
                   return false;
                 }

                 if (other->isFailed()) {
                   return false;
                 }

                 if (via->getLowerLayer() != other->getLowerLayer()) {
                   return false;
                 }

                 if (via->getUpperLayer() != other->getUpperLayer()) {
                   return false;
                 }

                 // Remove the smaller of the two vias
                 return via->getArea().area() <= other->getArea().area();
               }))
        != overlapping_via_tree.qend()) {
      remove_vias.insert(via);
      via->markFailed(failedViaReason::OVERLAPPING);
    }
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             2,
             "Removing {} vias due to overlaps.",
             remove_vias.size());
  remove_set_of_vias(remove_vias);

  // build via tree
  vias_.clear();
  for (auto& via : vias) {
    vias_.insert(via);
    via->getLowerShape()->addVia(via);
    via->getUpperShape()->addVia(via);
  }
  debugPrint(
      getLogger(), utl::PDN, "Make", 1, "Making vias in \"{}\" - end", name_);
}

void Grid::getVias(std::vector<ViaPtr>& vias) const
{
  for (const auto& via : vias_) {
    vias.push_back(via);
  }
}

void Grid::removeVia(const ViaPtr& via)
{
  auto find
      = std::ranges::find_if(vias_,

                             [via](const auto& other) { return via == other; });
  if (find != vias_.end()) {
    vias_.remove(*find);
  }
}

void Grid::removeInvalidVias()
{
  std::vector<ViaPtr> remove_vias;
  for (const auto& via : vias_) {
    if (!via->isValid()) {
      remove_vias.push_back(via);
    }
  }
  for (const auto& remove_via : remove_vias) {
    vias_.remove(remove_via);
  }
}

std::vector<GridComponent*> Grid::getGridComponents() const
{
  std::vector<GridComponent*> components;
  components.reserve(rings_.size() + straps_.size());
  for (const auto& ring : rings_) {
    components.push_back(ring.get());
  }

  for (const auto& strap : straps_) {
    components.push_back(strap.get());
  }

  return components;
}

void Grid::removeGridComponent(GridComponent* component)
{
  for (auto itr = rings_.begin(); itr != rings_.end();) {
    if (itr->get() == component) {
      itr = rings_.erase(itr);
    } else {
      itr++;
    }
  }

  for (auto itr = straps_.begin(); itr != straps_.end();) {
    if (itr->get() == component) {
      itr = straps_.erase(itr);
    } else {
      itr++;
    }
  }
}

std::map<Shape*, std::vector<odb::dbBox*>> Grid::writeToDb(
    const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
    bool do_pins,
    const Shape::ObstructionTreeMap& obstructions) const
{
  // write vias first do shapes can be adjusted if needed
  std::vector<ViaPtr> vias;
  getVias(vias);
  // sort the vias so they are written to db in the same order
  std::ranges::sort(vias, [](const auto& l, const auto& r) {
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
    auto net = net_map.find(via->getNet());
    if (net == net_map.end()) {
      continue;
    }
    via->writeToDb(net->second, getBlock(), obstructions);
  }
  for (const auto& connect : connect_) {
    connect->printViaReport();
  }

  std::map<Shape*, std::vector<odb::dbBox*>> shape_map;

  std::set<odb::dbTechLayer*> pin_layers(pin_layers_.begin(),
                                         pin_layers_.end());
  for (auto* component : getGridComponents()) {
    const auto db_shapes = component->writeToDb(net_map, do_pins, pin_layers);
    shape_map.insert(db_shapes.begin(), db_shapes.end());
  }

  return shape_map;
}

void Grid::getGridLevelObstructions(ShapeVectorMap& obstructions) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Obs",
             1,
             "Collecting grid obstructions from: {}",
             getLongName());
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
    auto obs = std::make_shared<GridObsShape>(layer, core, this);
    debugPrint(getLogger(),
               utl::PDN,
               "Obs",
               2,
               "Adding obstruction on layer {} covering {}",
               layer->getName(),
               Shape::getRectText(core, getBlock()->getDbUnitsPerMicron()));
    obstructions[layer].push_back(obs);
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
      auto obs = std::make_shared<GridObsShape>(layer, ring_rect, this);
      obs->generateObstruction();
      debugPrint(
          getLogger(),
          utl::PDN,
          "Obs",
          2,
          "Adding obstruction on layer {} covering {}",
          layer->getName(),
          Shape::getRectText(ring_rect, getBlock()->getDbUnitsPerMicron()));
      obstructions[layer].push_back(obs);
    }
  }
}

void Grid::makeInitialObstructions(odb::dbBlock* block,
                                   ShapeVectorMap& obs,
                                   const std::set<odb::dbInst*>& skip_insts,
                                   const std::set<odb::dbNet*>& skip_nets,
                                   utl::Logger* logger)
{
  debugPrint(logger, utl::PDN, "Make", 2, "Get initial obstructions - begin");
  // routing obs
  for (auto* ob : block->getObstructions()) {
    if (ob->isSlotObstruction() || ob->isFillObstruction()) {
      continue;
    }

    auto* box = ob->getBBox();
    odb::Rect obs_rect = box->getBox();
    if (ob->hasMinSpacing()) {
      obs_rect.bloat(ob->getMinSpacing(), obs_rect);
    }

    if (box->getTechLayer() == nullptr) {
      for (auto* layer : block->getDb()->getTech()->getLayers()) {
        auto shape = std::make_shared<Shape>(layer, obs_rect, Shape::BLOCK_OBS);
        obs[layer].push_back(std::move(shape));
      }
    } else {
      auto shape = std::make_shared<Shape>(
          box->getTechLayer(), obs_rect, Shape::BLOCK_OBS);
      obs[box->getTechLayer()].push_back(std::move(shape));
    }
  }

  // placed instances obs
  for (auto* inst : block->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }

    auto* master = inst->getMaster();
    if (master->isCore()) {
      continue;
    }
    if (master->isEndCap()) {
      switch (master->getType()) {
        case odb::dbMasterType::ENDCAP_TOPLEFT:
        case odb::dbMasterType::ENDCAP_TOPRIGHT:
        case odb::dbMasterType::ENDCAP_BOTTOMLEFT:
        case odb::dbMasterType::ENDCAP_BOTTOMRIGHT:
          // Master is a pad corner
          break;
        default:
          // Master is a std cell endcap
          continue;
      }
    }

    if (skip_insts.find(inst) != skip_insts.end()) {
      continue;
    }

    debugPrint(logger,
               utl::PDN,
               "Make",
               3,
               "Get instance {} obstructions",
               inst->getName());

    for (const auto& [layer, shapes] :
         InstanceGrid::getInstanceObstructions(inst)) {
      obs[layer].insert(obs[layer].end(), shapes.begin(), shapes.end());
    }
  }

  // fixed pins obs
  for (auto* bterm : block->getBTerms()) {
    if (skip_nets.find(bterm->getNet()) != skip_nets.end()) {
      // these shapes will be collected as existing to the grid.
      continue;
    }

    for (auto* bpin : bterm->getBPins()) {
      if (!bpin->getPlacementStatus().isFixed()) {
        continue;
      }

      for (auto* geom : bpin->getBoxes()) {
        auto* layer = geom->getTechLayer();
        auto shape
            = std::make_shared<Shape>(layer, geom->getBox(), Shape::BLOCK_OBS);
        shape->generateObstruction();
        shape->setRect(shape->getRect());
        obs[layer].push_back(std::move(shape));
      }
    }
  }

  debugPrint(logger, utl::PDN, "Make", 2, "Get initial obstructions - end");
}

void Grid::makeInitialShapes(odb::dbBlock* block,
                             ShapeVectorMap& shapes,
                             utl::Logger* logger)
{
  debugPrint(logger, utl::PDN, "Make", 2, "Get initial shapes - start");
  for (auto* net : block->getNets()) {
    Shape::populateMapFromDb(net, shapes);
  }
  debugPrint(logger, utl::PDN, "Make", 2, "Get initial shapes - end");
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

void Grid::setSwitchedPower(GridSwitchedPower* cell)
{
  switched_power_cell_ = std::unique_ptr<GridSwitchedPower>(cell);
  cell->setGrid(this);
}

std::set<odb::dbInst*> Grid::getInstances() const
{
  std::set<odb::dbInst*> insts;

  for (auto* comp : getGridComponents()) {
    if (comp->type() == GridComponent::PadConnect) {
      auto* pad_connect = dynamic_cast<PadDirectConnectionStraps*>(comp);
      if (pad_connect != nullptr) {
        insts.insert(pad_connect->getITerm()->getInst());
      }
    }
  }

  return insts;
}

void Grid::removeStrap(Straps* strap)
{
  straps_.erase(std::ranges::find_if(
      straps_, [strap](const std::unique_ptr<Straps>& other) {
        return strap == other.get();
      }));
}

bool Grid::hasShapes() const
{
  for (const auto& [layer, shapes] : getShapes()) {
    if (!shapes.empty()) {
      return true;
    }
  }
  return false;
}

bool Grid::hasVias() const
{
  return !vias_.empty();
}

///////////////

CoreGrid::CoreGrid(VoltageDomain* domain,
                   const std::string& name,
                   bool start_with_power,
                   const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : Grid(domain, name, start_with_power, generate_obstructions)
{
}

odb::Rect CoreGrid::getDomainBoundary() const
{
  // account for the width of the follow pins for straps
  const odb::Rect core = Grid::getDomainBoundary();

  int follow_pin_width = 0;
  for (const auto& strap : getStraps()) {
    if (strap->type() == GridComponent::Followpin) {
      follow_pin_width = std::max(follow_pin_width, strap->getWidth());
    }
  }

  return core.bloat(follow_pin_width / 2, odb::Orientation2D::Vertical);
}

void CoreGrid::setupDirectConnect(
    const std::vector<odb::dbTechLayer*>& connect_pad_layers)
{
  std::vector<PadDirectConnectionStraps*> straps;
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

    for (auto* iterm : iterms) {
      auto pad_connect = std::make_unique<PadDirectConnectionStraps>(
          this, iterm, connect_pad_layers);
      if (pad_connect->canConnect()) {
        straps.push_back(pad_connect.get());
        addStrap(std::move(pad_connect));
      } else {
        debugPrint(getLogger(),
                   utl::PDN,
                   "Pad",
                   2,
                   "Rejecting pad cell pin {} due to lack of connectivity",
                   iterm->getName())
      }
    }
  }

  PadDirectConnectionStraps::unifyConnectionTypes(straps);
  // check if unified connections are still connectable and remove if not
  for (auto* strap : straps) {
    if (!strap->canConnect()) {
      removeStrap(strap);
    }
  }
}

void CoreGrid::getGridLevelObstructions(ShapeVectorMap& obstructions) const
{
  if (getDomain()->hasRegion()) {
    // core grids only have grid level obstructions if they have a region
    Grid::getGridLevelObstructions(obstructions);
  }
}

void CoreGrid::cleanupShapes()
{
  // remove shapes that are wholly contained inside a macro
  Shape::ShapeTreeMap macros;
  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }

    const odb::Rect outline = inst->getBBox()->getBox();

    for (auto* obs : inst->getMaster()->getObstructions()) {
      auto shape = std::make_shared<Shape>(
          obs->getTechLayer(), outline, Shape::ShapeType::MACRO_OBS);
      shape->setObstruction(outline);
      macros[obs->getTechLayer()].insert(shape);
    }
    for (auto* term : inst->getMaster()->getMTerms()) {
      for (auto* pin : term->getMPins()) {
        for (auto* geom : pin->getGeometry()) {
          auto shape = std::make_shared<Shape>(
              geom->getTechLayer(), outline, Shape::ShapeType::MACRO_OBS);
          shape->setObstruction(outline);
          macros[geom->getTechLayer()].insert(shape);
        }
      }
    }
  }

  std::set<Shape*> remove;
  for (const auto& [layer, shapes] : getShapes()) {
    const auto& layer_avoid = macros[layer];
    for (const auto& shape : shapes) {
      if (layer_avoid.qbegin(bgi::contains(shape->getRect()))
          != layer_avoid.qend()) {
        remove.insert(shape.get());
      }
    }
  }

  for (auto* shape : remove) {
    auto* grid_comp = shape->getGridComponent();
    grid_comp->removeShape(shape);
  }
}

///////////////

InstanceGrid::InstanceGrid(
    VoltageDomain* domain,
    const std::string& name,
    bool start_with_power,
    odb::dbInst* inst,
    const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : Grid(domain, name, start_with_power, generate_obstructions), inst_(inst)
{
  auto* halo = inst->getHalo();
  if (halo != nullptr) {
    odb::Rect halo_box = halo->getBox();

    odb::Rect inst_box = inst->getBBox()->getBox();

    // copy halo from db
    addHalo({halo_box.xMin() - inst_box.xMin(),
             halo_box.yMin() - inst_box.yMin(),
             inst_box.xMin() - halo_box.xMax(),
             inst_box.yMin() - halo_box.yMax()});
  }
}

std::string InstanceGrid::getLongName() const
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

odb::Rect InstanceGrid::getDomainArea() const
{
  auto* bbox = inst_->getBBox();
  odb::Rect inst_box = bbox->getBox();

  return inst_box;
}

odb::Rect InstanceGrid::getDomainBoundary() const
{
  if (grid_to_boundary_) {
    // use instance boundary
    return getDomainArea();
  }

  // find the outline of the supply pins
  odb::Rect pin_box;
  pin_box.mergeInit();

  const odb::dbTransform transform = inst_->getTransform();

  for (auto* pin : inst_->getMaster()->getMTerms()) {
    if (!pin->getSigType().isSupply()) {
      continue;
    }

    pin_box.merge(pin->getBBox());
  }

  transform.apply(pin_box);

  return pin_box;
}

odb::Rect InstanceGrid::getGridArea() const
{
  return applyHalo(getDomainArea(), false, true, true);
}

odb::Rect InstanceGrid::applyHalo(const odb::Rect& rect,
                                  bool rect_is_min,
                                  bool apply_horizontal,
                                  bool apply_vertical) const
{
  return applyHalo(rect, halos_, rect_is_min, apply_horizontal, apply_vertical);
}

odb::Rect InstanceGrid::applyHalo(const odb::Rect& rect,
                                  const InstanceGrid::Halo& halo,
                                  bool rect_is_min,
                                  bool apply_horizontal,
                                  bool apply_vertical)
{
  odb::Rect halo_rect = rect;
  if (apply_horizontal) {
    halo_rect.set_xlo(halo_rect.xMin() - halo[0]);
    halo_rect.set_xhi(halo_rect.xMax() + halo[2]);
  }
  if (apply_vertical) {
    halo_rect.set_ylo(halo_rect.yMin() - halo[1]);
    halo_rect.set_yhi(halo_rect.yMax() + halo[3]);
  }
  if (rect_is_min) {
    halo_rect.merge(rect);
  }
  return halo_rect;
}

odb::Rect InstanceGrid::getGridBoundary() const
{
  return getDomainBoundary();
}

ShapeVectorMap InstanceGrid::getInstanceObstructions(
    odb::dbInst* inst,
    const InstanceGrid::Halo& halo)
{
  ShapeVectorMap obs;

  const odb::dbTransform transform = inst->getTransform();

  auto* master = inst->getMaster();

  for (auto* ob : master->getObstructions()) {
    odb::Rect obs_rect = ob->getBox();

    // add max of min spacing and the halo
    auto* layer = ob->getTechLayer();
    odb::Rect spacing_rect;
    obs_rect.bloat(layer->getSpacing(), spacing_rect);
    obs_rect = applyHalo(obs_rect, halo, true, true, true);
    obs_rect.merge(spacing_rect);

    transform.apply(obs_rect);
    auto shape = std::make_shared<Shape>(layer, obs_rect, Shape::BLOCK_OBS);

    obs[layer].push_back(std::move(shape));
  }

  // generate obstructions based on pins
  for (const auto& [layer, pin_shapes] : getInstancePins(inst)) {
    const bool is_horizontal
        = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    const bool is_vertical
        = layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
    for (const auto& pin_shape : pin_shapes) {
      pin_shape->setShapeType(Shape::BLOCK_OBS);
      pin_shape->generateObstruction();
      pin_shape->setRect(applyHalo(
          pin_shape->getObstruction(), halo, true, is_horizontal, is_vertical));
      pin_shape->setObstruction(pin_shape->getRect());
      obs[layer].push_back(pin_shape);
    }
  }

  return obs;
}

void InstanceGrid::getGridLevelObstructions(ShapeVectorMap& obstructions) const
{
  ShapeVectorMap local_obs;
  Grid::getGridLevelObstructions(local_obs);

  const odb::Rect inst_box = getGridArea();

  // copy layer obs
  for (const auto& [layer, shapes] : local_obs) {
    auto obs = std::make_shared<GridObsShape>(layer, inst_box, this);
    local_obs[layer].push_back(obs);
  }

  // copy instance obstructions
  for (const auto& [layer, shapes] : getInstanceObstructions(inst_, halos_)) {
    for (const auto& shape : shapes) {
      auto obs = std::make_shared<GridObsShape>(layer, shape->getRect(), this);
      local_obs[layer].push_back(obs);
    }
  }

  // merge local and global obs
  for (const auto& [layer, obs] : local_obs) {
    obstructions[layer].insert(
        obstructions[layer].end(), obs.begin(), obs.end());
  }
}

ShapeVectorMap InstanceGrid::getInstancePins(odb::dbInst* inst)
{
  // add instance pins
  std::vector<ShapePtr> pins;
  const odb::dbTransform transform = inst->getTransform();
  for (auto* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        if (box->isVia()) {
          odb::dbTechVia* tech_via = box->getTechVia();
          if (tech_via == nullptr) {
            continue;
          }

          const odb::dbTransform via_transform(box->getViaXY());
          for (auto* via_box : tech_via->getBoxes()) {
            odb::Rect box_rect = via_box->getBox();
            via_transform.apply(box_rect);
            transform.apply(box_rect);
            auto shape = std::make_shared<Shape>(
                via_box->getTechLayer(), net, box_rect);
            shape->setShapeType(Shape::FIXED);
            pins.push_back(std::move(shape));
          }
        } else {
          odb::Rect box_rect = box->getBox();
          transform.apply(box_rect);
          auto shape
              = std::make_shared<Shape>(box->getTechLayer(), net, box_rect);
          shape->setShapeType(Shape::FIXED);
          pins.push_back(std::move(shape));
        }
      }
    }
  }

  ShapeVectorMap shapes;
  for (auto& pin : pins) {
    shapes[pin->getLayer()].push_back(pin);
  }

  return shapes;
}

void InstanceGrid::getIntersections(std::vector<ViaPtr>& vias,
                                    const Shape::ShapeTreeMap& shapes) const
{
  // add instance pins
  Shape::ShapeTreeMap inst_shapes = shapes;
  for (auto* net : getNets(false)) {
    for (const auto& [layer, shapes_on_layer] : getInstancePins(inst_)) {
      auto& layer_shapes = inst_shapes[layer];
      for (const auto& shape : shapes_on_layer) {
        if (shape->getNet() == net) {
          layer_shapes.insert(shape);
        }
      }
    }
  }

  Grid::getIntersections(vias, inst_shapes);
}

std::vector<odb::dbNet*> InstanceGrid::getNets(bool starts_with_power) const
{
  auto nets = Grid::getNets(starts_with_power);

  std::set<odb::dbNet*> connected_nets;
  for (auto* iterm : inst_->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (net != nullptr) {
      connected_nets.insert(net);
    }
  }

  std::erase_if(nets, [&connected_nets](odb::dbNet* net) {
    return connected_nets.find(net) == connected_nets.end();
  });

  return nets;
}

void InstanceGrid::report() const
{
  Grid::report();
  auto* logger = getLogger();

  const double units = getDomain()->getBlock()->getDbUnitsPerMicron();
  logger->report("Halo:");
  logger->report("  Left: {:.4f}", halos_[0] / units);
  logger->report("  Bottom: {:.4f}", halos_[1] / units);
  logger->report("  Right: {:.4f}", halos_[2] / units);
  logger->report("  Top: {:.4f}", halos_[3] / units);
}

bool InstanceGrid::isValid() const
{
  if (getNets(startsWithPower()).empty()) {
    if (!inst_->getITerms().empty()) {
      // only warn when instance has something that could be connected to
      getLogger()->warn(utl::PDN,
                        231,
                        "{} is not connected to any power/ground nets.",
                        inst_->getName());
    }
    return false;
  }
  return true;
}

void InstanceGrid::checkSetup() const
{
  Grid::checkSetup();

  // check blockages above pins
  const auto nets = getNets(startsWithPower());
  for (auto* iterm : inst_->getITerms()) {
    if (std::ranges::find(nets, iterm->getNet()) == nets.end()) {
      continue;
    }
    odb::dbTechLayer* top = nullptr;
    std::set<odb::Rect> boxes;
    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* box : mpin->getGeometry()) {
        auto* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        if (top == nullptr
            || top->getRoutingLevel() < layer->getRoutingLevel()) {
          top = layer;
          boxes.clear();
        }
        if (layer == top) {
          boxes.insert(box->getBox());
        }
      }
    }

    if (top != nullptr) {
      const int top_idx = top->getNumber();
      std::map<odb::Rect, int64_t> overlap_area;
      std::set<odb::dbTechLayer*> layers;
      for (auto* master_obs : inst_->getMaster()->getObstructions()) {
        auto* obs_layer = master_obs->getTechLayer();
        if (obs_layer == nullptr) {
          continue;
        }
        if (obs_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }
        if (obs_layer->getNumber() > top_idx) {
          for (const auto& pin : boxes) {
            const odb::Rect mobs = master_obs->getBox();

            if (mobs.intersects(pin)) {
              // Determine level of obstruction
              const odb::Rect overlap = mobs.intersect(pin);
              overlap_area[pin] += overlap.area();
              layers.insert(obs_layer);
            }
          }
        }
      }

      if (!overlap_area.empty()) {
        int64_t total_pin_area = 0;
        int64_t total_overlap = 0;
        std::string layer_txt;
        for (const auto* layer : layers) {
          if (!layer_txt.empty()) {
            layer_txt += ", ";
          }
          layer_txt += layer->getName();
        }
        for (const auto& [pin, overlap] : overlap_area) {
          const int64_t pinarea = pin.area();
          total_overlap += overlap;
          total_pin_area += pinarea;

          if (overlap >= pinarea) {
            // pin completely obstructed
            getLogger()->error(
                utl::PDN,
                6,
                "{} on {} is blocked by obstructions on {} for {}",
                iterm->getMTerm()->getName(),
                top->getName(),
                layer_txt,
                inst_->getName());
          }
        }

        if (total_pin_area == 0) {
          // should not occur, implies all blocked pins have 0 area
          continue;
        }

        const float pct
            = 100 * static_cast<float>(total_overlap) / total_pin_area;
        getLogger()->warn(utl::PDN,
                          7,
                          "{} on {} is partially blocked ({:.1f}%) by "
                          "obstructions on {} for {}",
                          iterm->getMTerm()->getName(),
                          top->getName(),
                          pct,
                          layer_txt,
                          inst_->getName());
      }
    }
  }
}

////////

BumpGrid::BumpGrid(VoltageDomain* domain,
                   const std::string& name,
                   odb::dbInst* inst)
    : InstanceGrid(domain, name, true, inst, {})
{
}

bool BumpGrid::isValid() const
{
  if (!InstanceGrid::isValid()) {
    return false;
  }

  const auto nets = getNets(startsWithPower());
  const int net_count = nets.size();
  if (net_count > 1) {
    getLogger()->warn(utl::PDN,
                      241,
                      "Bump grid for {} is connected to {} power nets",
                      getInstance()->getName(),
                      net_count);
    return false;
  }

  return !isRouted();
}

bool BumpGrid::isRouted() const
{
  odb::dbNet* net = *getNets(startsWithPower()).begin();

  auto inst_pins = InstanceGrid::getInstancePins(getInstance());
  const auto pins = Shape::convertVectorToTree(inst_pins);

  for (auto* swire : net->getSWires()) {
    for (auto* sbox : swire->getWires()) {
      if (sbox->isVia()) {
        std::vector<odb::dbShape> shapes;
        sbox->getViaBoxes(shapes);
        for (const auto& shape : shapes) {
          auto* layer = shape.getTechLayer();

          auto find_layer = pins.find(layer);
          if (find_layer == pins.end()) {
            continue;
          }

          const odb::Rect rect = shape.getBox();
          const auto& layer_pins = find_layer->second;
          if (layer_pins.qbegin(bgi::intersects(rect)) != layer_pins.qend()) {
            return true;
          }
        }
      } else {
        auto* layer = sbox->getTechLayer();

        auto find_layer = pins.find(layer);
        if (find_layer == pins.end()) {
          continue;
        }

        const odb::Rect rect = sbox->getBox();
        const auto& layer_pins = find_layer->second;
        if (layer_pins.qbegin(bgi::intersects(rect)) != layer_pins.qend()) {
          return true;
        }
      }
    }
  }

  return false;
}

////////

ExistingGrid::ExistingGrid(
    PdnGen* pdngen,
    odb::dbBlock* block,
    utl::Logger* logger,
    const std::string& name,
    const std::vector<odb::dbTechLayer*>& generate_obstructions)
    : Grid(nullptr, name, false, generate_obstructions), domain_(nullptr)
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
  domain_ = std::make_unique<VoltageDomain>(
      pdngen, block, power, ground, nets_vec, logger);

  setDomain(domain_.get());

  populate();
}

void ExistingGrid::populate()
{
  ShapeVectorMap shapes;
  Grid::makeInitialShapes(domain_->getBlock(), shapes, getLogger());

  for (auto* inst : getBlock()->getInsts()) {
    if (inst->getPlacementStatus().isFixed()) {
      for (const auto& [layer, inst_shapes] :
           InstanceGrid::getInstancePins(inst)) {
        shapes[layer].insert(
            shapes[layer].end(), inst_shapes.begin(), inst_shapes.end());
      }
    }
  }

  shapes_ = Shape::convertVectorToTree(shapes);
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
  getLogger()->error(utl::PDN,
                     188,
                     "Existing grid does not support adding {}.",
                     GridComponent::typeToString(component->type()));
}

}  // namespace pdn
