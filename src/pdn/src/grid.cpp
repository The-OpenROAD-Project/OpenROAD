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

#include "connect.h"
#include "domain.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace pdn {

namespace bgi = boost::geometry::index;

Grid::Grid(VoltageDomain* domain,
           const std::string& name,
           bool starts_with_power)
    : domain_(domain), name_(name), starts_with_power_(starts_with_power)
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
  }

  return "Unknown";
}

void Grid::addRing(std::unique_ptr<Ring> ring)
{
  if (ring != nullptr) {
    ring->setGrid(this);
    ring->checkLayerSpecifications();
    rings_.push_back(std::move(ring));
  }
}

void Grid::addStrap(std::unique_ptr<Strap> strap)
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
  for (const auto& shape : getGridShapes()) {
    // make initial shapes
    shape->makeShapes(global_shapes);
    // cut shapes to avoid obstructions
    shape->cutShapes(local_obstructions);
    // add obstructions to they are accounted for in future shapes
    shape->getObstructions(local_obstructions);
  }

  // make vias
  makeVias(global_shapes, obstructions);

  // find and repair disconnected channels
  repairChannels(global_shapes, local_obstructions);
}

std::vector<Grid::RepairChannelArea> Grid::findRepairChannels() const
{
  std::vector<RepairChannelArea> channels;
  const odb::Rect core = getCoreArea();

  for (const auto& [layer, shapes] : getShapes()) {
    Strap* target = getTargetStrap(layer);
    if (target == nullptr) {
      continue;
    }
    // track channels by their center line
    std::map<int, std::vector<RepairChannelArea>> channel_nets;
    for (const auto& [box, shape] : shapes) {
      // only looking for followpins
      if (shape->getType() != odb::dbWireShapeType::FOLLOWPIN) {
        continue;
      }
      if (shape->getNumberOfConnections() == 0) {
        // nothing is connected to this shape, so record it
        const odb::Rect rect = shape->getRect();
        const int center_x = 0.5 * (rect.xMin() + rect.xMax());
        const int center_y = 0.5 * (rect.yMin() + rect.yMax());
        int location;
        if (shape->isHorizontal()) {
          location = center_x;
        } else {
          location = center_y;
        }

        odb::Rect core_rect = core.intersect(shape->getRect());
        channel_nets[location].push_back(RepairChannelArea{
            core_rect, target, shape->getLayer(), {shape->getNet()}});
      }
    }
    // combine found channels
    for (const auto& [location, channel_set] : channel_nets) {
      odb::Rect channel;
      channel.mergeInit();

      odb::dbTechLayer* connect_to = nullptr;
      std::set<odb::dbNet*> nets;
      for (const auto& [area, layer, connect, net] : channel_set) {
        channel.merge(area);
        nets.insert(net.begin(), net.end());
        connect_to = connect;
      }

      channels.push_back(RepairChannelArea{channel, target, connect_to, nets});
    }
  }

  return channels;
}

void Grid::repairChannels(const ShapeTreeMap& global_shapes,
                          ShapeTreeMap& obstructions)
{
  std::vector<RepairChannel*> repairs;

  const auto channels = findRepairChannels();
  for (const auto& channel : channels) {
    // create strap repair channel
    auto* strap = new RepairChannel(this,
                                    channel.target,
                                    channel.connect_to,
                                    obstructions,
                                    channel.nets,
                                    channel.area);
    repairs.push_back(strap);
    addStrap(std::unique_ptr<RepairChannel>(strap));
  }

  // attempt to build repairs
  for (auto* strap : repairs) {
    strap->makeShapes(global_shapes);
    strap->cutShapes(obstructions);
    strap->getObstructions(obstructions);
  }

  // make the vias
  if (!repairs.empty()) {
    makeVias(global_shapes, obstructions);
  }

  const auto remaining_channels = findRepairChannels();
  if (!remaining_channels.empty()) {
    // if channels remain, report them and generate error
    double dbu_to_microns = getBlock()->getDbUnitsPerMicron();
    for (const auto& channel : remaining_channels) {
      std::string nets;
      for (auto* net : channel.nets) {
        nets += net->getName() + " ";
      }
      getLogger()->warn(utl::PDN,
                        178,
                        "Remaining channel {} for nets: {}",
                        Shape::getRectText(channel.area, dbu_to_microns),
                        nets);
    }
    getLogger()->error(utl::PDN, 179, "Unable to repair all channels.");
  }
}

Strap* Grid::getTargetStrap(odb::dbTechLayer* layer) const
{
  std::set<odb::dbTechLayer*> connects_to;
  for (const auto& connect : connect_) {
    if (connect->getLowerLayer() == layer) {
      connects_to.insert(connect->getUpperLayer());
    }
  }

  if (connects_to.empty()) {
    return nullptr;
  }

  Strap* lowest_target = nullptr;

  for (const auto& strap : straps_) {
    if (strap->type() != GridShape::Strap) {
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

  return lowest_target;
}

const ShapeTreeMap Grid::getShapes() const
{
  ShapeTreeMap shapes;

  for (const auto& grid_shape : getGridShapes()) {
    for (const auto& [layer, grid_shapes] : grid_shape->getShapes()) {
      auto& layer_shapes = shapes[layer];
      for (const auto& shape : grid_shapes) {
        layer_shapes.insert(shape);
      }
    }
  }

  return shapes;
}

const odb::Rect Grid::getCoreArea() const
{
  return domain_->getCoreArea();
}

const odb::Rect Grid::getCoreBoundary() const
{
  return getCoreArea();
}

const odb::Rect Grid::getDieArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  odb::Rect rect;
  getBlock()->getDieArea(rect);
  return rect;
}

const odb::Rect Grid::getDieBoundary() const
{
  return getDieArea();
}

const odb::Rect Grid::getRingArea() const
{
  if (getBlock() == nullptr) {
    return odb::Rect();
  }

  // get the outline of the rings
  odb::Rect rect = getCoreArea();
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

const odb::Rect Grid::getRingBoundary() const
{
  return getRingArea();
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
    for (auto& [lower_box, lower_shape] : lower_shapes) {
      // check for intersections in higher layer shapes
      for (auto it = upper_shapes.qbegin(bgi::intersects(lower_box));
           it != upper_shapes.qend();
           it++) {
        auto upper_shape = it->second;
        if (lower_shape->getNet() != upper_shape->getNet()) {
          // not the same net, so ignore
          continue;
        }

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
  for (const auto& shape : getGridShapes()) {
    shape->clearShapes();
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

  odb::Rect search_area;
  search_area.mergeInit();
  for (const auto& [layer, shapes] : search_shapes) {
    for (const auto& [box, shape] : shapes) {
      search_area.merge(shape->getRect());
    }
  }

  // populate shapes and obstructions
  Box search_box(Point(search_area.xMin(), search_area.yMin()),
                 Point(search_area.xMax(), search_area.yMax()));
  for (auto& [layer, layer_gloabl_shape] : global_shapes) {
    auto& shapes = search_shapes[layer];
    for (auto it = layer_gloabl_shape.qbegin(bgi::intersects(search_box));
         it != layer_gloabl_shape.qend();
         it++) {
      auto& box = it->first;
      auto& shape = it->second;
      shapes.insert({box, shape});
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

  auto remove_set_of_vias = [this, &vias](std::set<ViaPtr>& remove_vias) {
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

  // remove vias that are too small to make on any layer
  for (const auto& via : vias) {
    const odb::Rect via_size = via->getArea();
    const int width = via_size.minDXDY();
    for (auto* layer : via->getConnect()->getIntermediteRoutingLayers()) {
      if (width < layer->getMinWidth()) {
        remove_vias.insert(via);
        break;
      }
    }
  }
  debugPrint(getLogger(),
             utl::PDN,
             "Via",
             2,
             "Removing {} vias due to sizing limitations.",
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

const std::vector<GridShape*> Grid::getGridShapes() const
{
  std::vector<GridShape*> shapes;
  for (const auto& ring : rings_) {
    shapes.push_back(ring.get());
  }

  for (const auto& strap : straps_) {
    shapes.push_back(strap.get());
  }

  return shapes;
}

void Grid::writeToDb(const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
                     bool do_pins) const
{
  std::set<odb::dbTechLayer*> pin_layers(pin_layers_.begin(),
                                         pin_layers_.end());
  for (const auto& grid_shape : getGridShapes()) {
    grid_shape->writeToDb(net_map, do_pins, pin_layers);
  }

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
}

void Grid::getGridLevelObstructions(ShapeTreeMap& obstructions) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Obs",
             1,
             "Collecting grid obstructions from: {}",
             name_);
  const odb::Rect core = getCoreArea();

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
    auto obs = std::make_shared<Shape>(layer, nullptr, core);
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
      auto obs = std::make_shared<Shape>(layer, nullptr, ring_rect);
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

///////////////

CoreGrid::CoreGrid(VoltageDomain* domain,
                   const std::string& name,
                   bool start_with_power)
    : Grid(domain, name, start_with_power)
{
}

const odb::Rect CoreGrid::getCoreBoundary() const
{
  // account for the width of the follow pins for straps
  odb::Rect core = Grid::getCoreBoundary();

  int follow_pin_width = 0;
  for (const auto& strap : getStraps()) {
    if (strap->type() == GridShape::Followpin) {
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
          const std::string r_name
              = l->getInst()->getName() + "/" + l->getMTerm()->getName();
          const std::string l_name
              = l->getInst()->getName() + "/" + l->getMTerm()->getName();
          return r_name < l_name;
        });

    for (auto* iterm : iterms) {
      auto pad_connect
          = std::make_unique<PadDirectConnect>(this, iterm, connect_pad_layers);
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
                           odb::dbInst* inst)
    : Grid(domain, name, start_with_power),
      inst_(inst),
      halos_({0, 0, 0, 0}),
      grid_to_boundary_(false)
{
  auto* halo = inst->getHalo();
  if (halo != nullptr) {
    odb::Rect halo_box;
    halo->getBox(halo_box);

    odb::Rect inst_box;
    inst->getBBox()->getBox(inst_box);

    // copy halo from db
    halos_[0] = halo_box.xMin() - inst_box.xMin();
    halos_[1] = halo_box.yMin() - inst_box.yMin();
    halos_[2] = inst_box.xMin() - halo_box.xMax();
    halos_[3] = inst_box.yMin() - halo_box.yMax();
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

const odb::Rect InstanceGrid::getCoreArea() const
{
  auto* bbox = inst_->getBBox();
  odb::Rect inst_box;
  bbox->getBox(inst_box);

  // apply halo
  inst_box.set_xlo(inst_box.xMin() - halos_[0]);
  inst_box.set_ylo(inst_box.yMin() - halos_[1]);
  inst_box.set_xhi(inst_box.xMax() + halos_[2]);
  inst_box.set_yhi(inst_box.yMax() + halos_[3]);

  return inst_box;
}

const odb::Rect InstanceGrid::getCoreBoundary() const
{
  if (grid_to_boundary_) {
    // use instance boundary
    return getCoreArea();
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

const odb::Rect InstanceGrid::getDieArea() const
{
  return getCoreArea();
}

const odb::Rect InstanceGrid::getDieBoundary() const
{
  return getCoreBoundary();
}

void InstanceGrid::getGridLevelObstructions(ShapeTreeMap& obstructions) const
{
  Grid::getGridLevelObstructions(obstructions);

  const odb::Rect inst_box = getCoreArea();

  std::set<odb::dbTechLayer*> layers;

  // find layers with obstructions
  auto* master = inst_->getMaster();
  for (auto* obs : master->getObstructions()) {
    auto* layer = obs->getTechLayer();
    if (layer != nullptr) {
      layers.insert(layer);
    }
  }

  // add obstruction covering ensure instance
  for (auto* layer : layers) {
    auto obs = std::make_shared<Shape>(layer, nullptr, inst_box);
    obstructions[layer].insert({obs->getObstructionBox(), obs});
  }
}

void InstanceGrid::getIntersections(std::vector<ViaPtr>& vias,
                                    const ShapeTreeMap& shapes) const
{
  // add instance pins
  std::vector<ShapePtr> pins;
  odb::dbTransform transform;
  inst_->getTransform(transform);
  for (auto* net : getNets()) {
    for (auto* iterm : inst_->getITerms()) {
      if (iterm->getNet() == net) {
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
  }

  ShapeTreeMap inst_shapes = shapes;
  for (auto& pin : pins) {
    inst_shapes[pin->getLayer()].insert({pin->getRectBox(), pin});
  }

  Grid::getIntersections(vias, inst_shapes);
}

}  // namespace pdn
