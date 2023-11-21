/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "pdn/PdnGen.hh"

#include <map>
#include <set>

#include "connect.h"
#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "ord/OpenRoad.hh"
#include "power_cells.h"
#include "renderer.h"
#include "rings.h"
#include "sroute.h"
#include "straps.h"
#include "techlayer.h"
#include "utl/Logger.h"
#include "via_repair.h"

namespace pdn {

using utl::Logger;

using odb::dbBlock;
using odb::dbInst;
using odb::dbMaster;
using odb::dbMTerm;
using odb::dbNet;

using utl::PDN;

PdnGen::PdnGen() : db_(nullptr), logger_(nullptr)
{
}

PdnGen::~PdnGen() = default;

void PdnGen::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  sroute_ = std::make_unique<SRoute>(this, db, logger_);
}

void PdnGen::reset()
{
  core_domain_ = nullptr;
  domains_.clear();
  updateRenderer();
}

void PdnGen::resetShapes()
{
  for (auto* grid : getGrids()) {
    grid->resetShapes();
  }
  updateRenderer();
}

void PdnGen::buildGrids(bool trim)
{
  debugPrint(logger_, utl::PDN, "Make", 1, "Build - begin");
  auto* block = db_->getChip()->getBlock();

  resetShapes();

  const std::vector<Grid*> grids = getGrids();

  // connect instances already assigned to grids
  std::set<odb::dbInst*> insts_in_grids;
  for (auto* grid : grids) {
    auto insts_in_grid = grid->getInstances();
    insts_in_grids.insert(insts_in_grid.begin(), insts_in_grid.end());
  }

  ShapeTreeMap block_obs;
  Grid::makeInitialObstructions(block, block_obs, insts_in_grids, logger_);
  for (auto* grid : grids) {
    grid->getGridLevelObstructions(block_obs);
  }
  ShapeTreeMap all_shapes;

  // get special shapes
  Grid::makeInitialShapes(block, all_shapes, logger_);
  for (const auto& [layer, layer_shapes] : all_shapes) {
    auto& layer_obs = block_obs[layer];
    for (const auto& [box, shape] : layer_shapes) {
      layer_obs.insert({shape->getObstructionBox(), shape});
    }
  }

  for (auto* grid : grids) {
    debugPrint(
        logger_, utl::PDN, "Make", 2, "Build start grid - {}", grid->getName());
    grid->makeShapes(all_shapes, block_obs);
    for (const auto& [layer, shapes] : grid->getShapes()) {
      auto& all_shapes_layer = all_shapes[layer];
      for (auto& shape : shapes) {
        all_shapes_layer.insert(shape);
      }
    }
    grid->getObstructions(block_obs);
    debugPrint(
        logger_, utl::PDN, "Make", 2, "Build end grid - {}", grid->getName());
  }

  if (trim) {
    trimShapes();

    cleanupVias();
  }

  bool failed = false;
  for (auto* grid : grids) {
    if (grid->hasShapes() || grid->hasVias()) {
      continue;
    }
    logger_->warn(utl::PDN,
                  232,
                  "{} does not contain any shapes or vias.",
                  grid->getLongName());
    failed = true;
  }
  if (failed) {
    logger_->error(utl::PDN, 233, "Failed to generate full power grid.");
  }

  updateRenderer();
  debugPrint(logger_, utl::PDN, "Make", 1, "Build - end");
}

void PdnGen::cleanupVias()
{
  debugPrint(logger_, utl::PDN, "Make", 2, "Cleanup vias - begin");
  for (auto* grid : getGrids()) {
    grid->removeInvalidVias();
  }
  debugPrint(logger_, utl::PDN, "Make", 2, "Cleanup vias - end");
}

void PdnGen::trimShapes()
{
  debugPrint(logger_, utl::PDN, "Make", 2, "Trim shapes - start");
  auto grids = getGrids();

  std::vector<ViaPtr> all_vias;
  for (auto* grid : grids) {
    grid->getVias(all_vias);
  }

  for (auto* grid : grids) {
    for (const auto& [layer, shapes] : grid->getShapes()) {
      for (const auto& [box, shape] : shapes) {
        shape->clearVias();
      }
    }
  }

  for (const auto& via : all_vias) {
    via->getLowerShape()->addVia(via);
    via->getUpperShape()->addVia(via);
  }

  for (auto* grid : grids) {
    if (grid->type() == Grid::Existing) {
      // fixed shapes, so nothing to do
      continue;
    }
    const auto& pin_layers = grid->getPinLayers();
    for (const auto& [layer, shapes] : grid->getShapes()) {
      for (const auto& [box, shape] : shapes) {
        if (!shape->isModifiable()) {
          continue;
        }

        // if pin layer, do not modify the shapes, but allow them to be
        // removed if they are not connected to anything
        const bool is_pin_layer
            = pin_layers.find(shape->getLayer()) != pin_layers.end();

        Shape* new_shape = nullptr;
        const odb::Rect new_rect = shape->getMinimumRect();
        if (new_rect == shape->getRect()) {  // no change to shape
          continue;
        }

        // check if vias and shape form a stack without any other connections
        bool effectively_vias_stack = true;
        for (const auto& via : shape->getVias()) {
          if (via->getArea() != new_rect) {
            effectively_vias_stack = false;
            break;
          }
        }
        if (!effectively_vias_stack) {
          new_shape = shape->copy();
          new_shape->setRect(new_rect);
        }

        auto* component = shape->getGridComponent();
        if (new_shape == nullptr) {
          if (shape->isRemovable()) {
            component->removeShape(shape.get());
          }
        } else {
          if (!is_pin_layer) {
            component->replaceShape(shape.get(), {new_shape});
          }
        }
      }
    }
  }
  debugPrint(logger_, utl::PDN, "Make", 2, "Trim shapes - end");
}

VoltageDomain* PdnGen::getCoreDomain() const
{
  return core_domain_.get();
}

void PdnGen::ensureCoreDomain()
{
  if (core_domain_ == nullptr) {
    setCoreDomain(nullptr, nullptr, nullptr, {});
  }
}

std::vector<VoltageDomain*> PdnGen::getDomains() const
{
  std::vector<VoltageDomain*> domains;
  if (core_domain_ != nullptr) {
    domains.push_back(core_domain_.get());
  }

  for (const auto& domain : domains_) {
    domains.push_back(domain.get());
  }
  return domains;
}

VoltageDomain* PdnGen::findDomain(const std::string& name)
{
  ensureCoreDomain();
  auto domains = getDomains();
  if (name.empty()) {
    return domains.back();
  }

  for (const auto& domain : domains) {
    if (domain->getName() == name) {
      return domain;
    }
  }

  return nullptr;
}

void PdnGen::setCoreDomain(odb::dbNet* power,
                           odb::dbNet* switched_power,
                           odb::dbNet* ground,
                           const std::vector<odb::dbNet*>& secondary)
{
  auto* block = db_->getChip()->getBlock();
  if (core_domain_ != nullptr) {
    logger_->warn(utl::PDN, 183, "Replacing existing core voltage domain.");
  }
  core_domain_ = std::make_unique<VoltageDomain>(
      this, block, power, ground, secondary, logger_);

  core_domain_->setSwitchedPower(switched_power);
}

void PdnGen::makeRegionVoltageDomain(
    const std::string& name,
    odb::dbNet* power,
    odb::dbNet* switched_power,
    odb::dbNet* ground,
    const std::vector<odb::dbNet*>& secondary_nets,
    odb::dbRegion* region)
{
  if (region == nullptr) {
    logger_->error(utl::PDN, 229, "Region must be specified.");
  }
  for (const auto& domain : domains_) {
    if (domain->getName() == name) {
      logger_->error(utl::PDN,
                     184,
                     "Cannot have region voltage domain with the same name "
                     "already exists: {}",
                     name);
    }
  }
  auto* block = db_->getChip()->getBlock();
  auto domain = std::make_unique<VoltageDomain>(
      this, name, block, power, ground, secondary_nets, region, logger_);
  domain->setSwitchedPower(switched_power);
  domains_.push_back(std::move(domain));
}

PowerCell* PdnGen::findSwitchedPowerCell(const std::string& name) const
{
  for (const auto& cell : switched_power_cells_) {
    if (cell->getName() == name) {
      return cell.get();
    }
  }
  return nullptr;
}

void PdnGen::makeSwitchedPowerCell(odb::dbMaster* master,
                                   odb::dbMTerm* control,
                                   odb::dbMTerm* acknowledge,
                                   odb::dbMTerm* switched_power,
                                   odb::dbMTerm* alwayson_power,
                                   odb::dbMTerm* ground)
{
  auto* check = findSwitchedPowerCell(master->getName());
  if (check != nullptr) {
    logger_->error(utl::PDN, 196, "{} is already defined.", master->getName());
  }

  switched_power_cells_.push_back(std::make_unique<PowerCell>(logger_,
                                                              master,
                                                              control,
                                                              acknowledge,
                                                              switched_power,
                                                              alwayson_power,
                                                              ground));
}

std::vector<Grid*> PdnGen::getGrids() const
{
  std::vector<Grid*> grids;
  if (core_domain_ != nullptr) {
    for (const auto& grid : core_domain_->getGrids()) {
      grids.push_back(grid.get());
    }
  }
  for (const auto& domain : domains_) {
    for (const auto& grid : domain->getGrids()) {
      grids.push_back(grid.get());
    }
  }

  return grids;
}

std::vector<Grid*> PdnGen::findGrid(const std::string& name) const
{
  std::vector<Grid*> found_grids;
  auto grids = getGrids();

  if (name.empty()) {
    if (grids.empty()) {
      return {};
    }

    return findGrid(grids.back()->getName());
  }

  for (auto* grid : grids) {
    if (grid->getName() == name || grid->getLongName() == name) {
      found_grids.push_back(grid);
    }
  }

  return found_grids;
}

void PdnGen::makeCoreGrid(
    VoltageDomain* domain,
    const std::string& name,
    StartsWith starts_with,
    const std::vector<odb::dbTechLayer*>& pin_layers,
    const std::vector<odb::dbTechLayer*>& generate_obstructions,
    PowerCell* powercell,
    odb::dbNet* powercontrol,
    const char* powercontrolnetwork)
{
  auto grid = std::make_unique<CoreGrid>(
      domain, name, starts_with == POWER, generate_obstructions);
  grid->setPinLayers(pin_layers);
  if (powercell != nullptr) {
    grid->setSwitchedPower(new GridSwitchedPower(
        grid.get(),
        powercell,
        powercontrol,
        GridSwitchedPower::fromString(powercontrolnetwork, logger_)));
  }
  domain->addGrid(std::move(grid));
}

Grid* PdnGen::instanceGrid(odb::dbInst* inst) const
{
  for (auto* check_grid : getGrids()) {
    auto* other_grid = dynamic_cast<InstanceGrid*>(check_grid);
    if (other_grid != nullptr) {
      if (other_grid->getInstance() == inst) {
        return check_grid;
      }
    }
  }

  return nullptr;
}

void PdnGen::makeInstanceGrid(
    VoltageDomain* domain,
    const std::string& name,
    StartsWith starts_with,
    odb::dbInst* inst,
    const std::array<int, 4>& halo,
    bool pg_pins_to_boundary,
    bool default_grid,
    const std::vector<odb::dbTechLayer*>& generate_obstructions,
    bool is_bump)
{
  auto* check_grid = instanceGrid(inst);
  if (check_grid != nullptr) {
    if (check_grid->isReplaceable()) {
      // remove the old grid and replace with this one
      debugPrint(logger_,
                 utl::PDN,
                 "Setup",
                 1,
                 "Replacing {} with {} for instance: {}",
                 check_grid->getName(),
                 name,
                 inst->getName());
      auto* check_domain = check_grid->getDomain();
      check_domain->removeGrid(check_grid);
    } else if (default_grid) {
      // this is a default grid so we can ignore this assignment
      return;
    } else {
      logger_->warn(utl::PDN,
                    182,
                    "Instance {} already belongs to another grid \"{}\" and "
                    "therefore cannot belong to \"{}\".",
                    inst->getName(),
                    check_grid->getName(),
                    name);
      return;
    }
  }

  std::unique_ptr<InstanceGrid> grid = nullptr;
  if (is_bump) {
    grid = std::make_unique<BumpGrid>(domain, name, inst);
  } else {
    grid = std::make_unique<InstanceGrid>(
        domain, name, starts_with == POWER, inst, generate_obstructions);
  }
  if (!std::all_of(halo.begin(), halo.end(), [](int v) { return v == 0; })) {
    grid->addHalo(halo);
  }
  grid->setGridToBoundary(pg_pins_to_boundary);

  grid->setReplaceable(default_grid);

  if (!grid->isValid()) {
    return;
  }

  domain->addGrid(std::move(grid));
}

void PdnGen::makeExistingGrid(
    const std::string& name,
    const std::vector<odb::dbTechLayer*>& generate_obstructions)
{
  auto grid = std::make_unique<ExistingGrid>(
      this, db_->getChip()->getBlock(), logger_, name, generate_obstructions);

  ensureCoreDomain();
  getCoreDomain()->addGrid(std::move(grid));
}

void PdnGen::makeRing(Grid* grid,
                      odb::dbTechLayer* layer0,
                      int width0,
                      int spacing0,
                      odb::dbTechLayer* layer1,
                      int width1,
                      int spacing1,
                      StartsWith starts_with,
                      const std::array<int, 4>& offset,
                      const std::array<int, 4>& pad_offset,
                      bool extend,
                      const std::vector<odb::dbTechLayer*>& pad_pin_layers,
                      const std::vector<odb::dbNet*>& nets)
{
  std::array<Rings::Layer, 2> layers{Rings::Layer{layer0, width0, spacing0},
                                     Rings::Layer{layer1, width1, spacing1}};
  auto ring = std::make_unique<Rings>(grid, layers);
  ring->setOffset(offset);
  if (std::any_of(
          pad_offset.begin(), pad_offset.end(), [](int o) { return o != 0; })) {
    ring->setPadOffset(pad_offset);
  }
  ring->setExtendToBoundary(extend);
  if (starts_with != GRID) {
    ring->setStartWithPower(starts_with == POWER);
  }
  ring->setNets(nets);
  grid->addRing(std::move(ring));
  if (!pad_pin_layers.empty() && grid->type() == Grid::Core) {
    auto* core_grid = static_cast<CoreGrid*>(grid);
    core_grid->setupDirectConnect(pad_pin_layers);
  }
}

void PdnGen::makeFollowpin(Grid* grid,
                           odb::dbTechLayer* layer,
                           int width,
                           ExtensionMode extend)
{
  auto strap = std::make_unique<FollowPins>(grid, layer, width);
  strap->setExtend(extend);

  grid->addStrap(std::move(strap));
}

void PdnGen::makeStrap(Grid* grid,
                       odb::dbTechLayer* layer,
                       int width,
                       int spacing,
                       int pitch,
                       int offset,
                       int number_of_straps,
                       bool snap,
                       StartsWith starts_with,
                       ExtensionMode extend,
                       const std::vector<odb::dbNet*>& nets)
{
  auto strap = std::make_unique<Straps>(
      grid, layer, width, pitch, spacing, number_of_straps);
  strap->setExtend(extend);
  strap->setOffset(offset);
  strap->setSnapToGrid(snap);
  if (starts_with != GRID) {
    strap->setStartWithPower(starts_with == POWER);
  }
  strap->setNets(nets);
  grid->addStrap(std::move(strap));
}

void PdnGen::makeConnect(Grid* grid,
                         odb::dbTechLayer* layer0,
                         odb::dbTechLayer* layer1,
                         int cut_pitch_x,
                         int cut_pitch_y,
                         const std::vector<odb::dbTechViaGenerateRule*>& vias,
                         const std::vector<odb::dbTechVia*>& techvias,
                         int max_rows,
                         int max_columns,
                         const std::vector<odb::dbTechLayer*>& ongrid,
                         const std::map<odb::dbTechLayer*, int>& split_cuts,
                         const std::string& dont_use_vias)
{
  auto con = std::make_unique<Connect>(grid, layer0, layer1);
  con->setCutPitch(cut_pitch_x, cut_pitch_y);

  for (auto* via : vias) {
    con->addFixedVia(via);
  }

  for (auto* via : techvias) {
    con->addFixedVia(via);
  }

  con->setMaxRows(max_rows);
  con->setMaxColumns(max_columns);
  con->setOnGrid(ongrid);
  con->setSplitCuts(split_cuts);

  if (!dont_use_vias.empty()) {
    con->filterVias(dont_use_vias);
  }

  grid->addConnect(std::move(con));
}

void PdnGen::setDebugRenderer(bool on)
{
  if (on && gui::Gui::enabled()) {
    if (debug_renderer_ == nullptr) {
      debug_renderer_ = std::make_unique<PDNRenderer>(this);
      rendererRedraw();
    }
  } else {
    debug_renderer_ = nullptr;
  }
}

void PdnGen::rendererRedraw()
{
  if (debug_renderer_ != nullptr) {
    try {
      buildGrids(false);
    } catch (const std::runtime_error& /* e */) {
      // do nothing, dont want grid error to prevent debug renderer
      debug_renderer_->update();
    }
  }
}

void PdnGen::updateRenderer() const
{
  if (debug_renderer_ != nullptr) {
    debug_renderer_->update();
  }
}

void PdnGen::createSrouteWires(
    const char* net,
    const char* outerNet,
    odb::dbTechLayer* layer0,
    odb::dbTechLayer* layer1,
    int cut_pitch_x,
    int cut_pitch_y,
    const std::vector<odb::dbTechViaGenerateRule*>& vias,
    const std::vector<odb::dbTechVia*>& techvias,
    int max_rows,
    int max_columns,
    const std::vector<odb::dbTechLayer*>& ongrid,
    std::vector<int> metalwidths,
    std::vector<int> metalspaces,
    const std::vector<odb::dbInst*>& insts)
{
  sroute_->createSrouteWires(net,
                             outerNet,
                             layer0,
                             layer1,
                             cut_pitch_x,
                             cut_pitch_y,
                             vias,
                             techvias,
                             max_rows,
                             max_columns,
                             ongrid,
                             metalwidths,
                             metalspaces,
                             insts);
}

void PdnGen::writeToDb(bool add_pins, const std::string& report_file) const
{
  std::map<odb::dbNet*, odb::dbSWire*> net_map;

  auto domains = getDomains();
  for (auto* domain : domains) {
    for (auto* net : domain->getNets()) {
      net_map[net] = nullptr;
    }
  }

  for (auto& [net, swire] : net_map) {
    net->setSpecial();

    // determine if unique and set WildConnected
    bool appear_in_all_grids = true;
    for (auto* domain : domains) {
      for (const auto& grid : domain->getGrids()) {
        const auto nets = grid->getNets();
        if (std::find(nets.begin(), nets.end(), net) == nets.end()) {
          appear_in_all_grids = false;
        }
      }
    }

    if (appear_in_all_grids) {
      // should this be based on the global connect?
      net->setWildConnected();
    }

    swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  }

  // collect all the SWires from the block
  auto* block = db_->getChip()->getBlock();
  ShapeTreeMap obstructions;
  for (auto* net : block->getNets()) {
    ShapeTreeMap net_shapes;
    Shape::populateMapFromDb(net, net_shapes);
    for (const auto& [layer, net_obs_layer] : net_shapes) {
      auto& obs_layer = obstructions[layer];
      for (const auto& [box, shape] : net_obs_layer) {
        obs_layer.insert({shape->getObstructionBox(), shape});
      }
    }
  }

  for (auto* domain : domains) {
    for (const auto& grid : domain->getGrids()) {
      grid->writeToDb(net_map, add_pins, obstructions);
      grid->makeRoutingObstructions(db_->getChip()->getBlock());
    }
  }

  if (!report_file.empty()) {
    std::ofstream file(report_file);
    if (!file) {
      logger_->warn(
          utl::PDN, 228, "Unable to open \"{}\" to write.", report_file);
      return;
    }

    for (auto* grid : getGrids()) {
      for (const auto& connect : grid->getConnect()) {
        connect->writeFailedVias(file);
      }
    }
  }
}

void PdnGen::ripUp(odb::dbNet* net)
{
  if (net == nullptr) {
    resetShapes();
    std::set<odb::dbNet*> nets;
    ensureCoreDomain();
    for (auto* domain : getDomains()) {
      for (auto* net : domain->getNets()) {
        nets.insert(net);
      }
    }

    for (auto* grid : getGrids()) {
      grid->ripup();
    }

    for (odb::dbNet* net : nets) {
      ripUp(net);
    }

    return;
  }

  ShapeTreeMap net_shapes;
  Shape::populateMapFromDb(net, net_shapes);
  // remove bterms that connect to swires
  std::set<odb::dbBTerm*> terms;
  for (auto* bterm : net->getBTerms()) {
    std::set<odb::dbBPin*> pins;
    for (auto* pin : bterm->getBPins()) {
      bool remove = false;
      for (auto* box : pin->getBoxes()) {
        auto* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        if (net_shapes.count(layer) == 0) {
          continue;
        }

        odb::Rect rect = box->getBox();
        const auto& shapes = net_shapes[layer];
        Box search_box(Point(rect.xMin(), rect.yMin()),
                       Point(rect.xMax(), rect.yMax()));
        if (shapes.qbegin(bgi::intersects(search_box)) != shapes.qend()) {
          remove = true;
          break;
        }
      }
      if (remove) {
        pins.insert(pin);
      }
    }
    for (auto* pin : pins) {
      odb::dbBPin::destroy(pin);
    }
    if (bterm->getBPins().empty()) {
      terms.insert(bterm);
    }
  }
  for (auto* term : terms) {
    odb::dbBTerm::destroy(term);
  }
  auto swires = net->getSWires();
  for (auto iter = swires.begin(); iter != swires.end();) {
    iter = odb::dbSWire::destroy(iter);
  }
}

void PdnGen::report()
{
  for (const auto& cell : switched_power_cells_) {
    cell->report();
  }
  ensureCoreDomain();
  for (auto* domain : getDomains()) {
    domain->report();
  }
}

void PdnGen::setAllowRepairChannels(bool allow)
{
  for (auto* grid : getGrids()) {
    grid->setAllowRepairChannels(allow);
  }
}

void PdnGen::filterVias(const std::string& filter)
{
  for (auto* grid : getGrids()) {
    for (const auto& connect : grid->getConnect()) {
      connect->filterVias(filter);
    }
  }
}

void PdnGen::checkDesign(odb::dbBlock* block) const
{
  for (auto* inst : block->getInsts()) {
    if (!inst->getPlacementStatus().isFixed()) {
      continue;
    }
    for (auto* term : inst->getITerms()) {
      if (term->getSigType().isSupply() && term->getNet() == nullptr) {
        logger_->warn(
            utl::PDN,
            189,
            "Supply pin {} of instance {} is not connected to any net.",
            term->getMTerm()->getName(),
            inst->getName());
      }
    }
  }

  bool unplaced_macros = false;
  for (auto* inst : block->getInsts()) {
    if (!inst->isBlock()) {
      continue;
    }
    if (!inst->getPlacementStatus().isFixed()) {
      unplaced_macros = true;
      logger_->warn(
          utl::PDN, 234, "{} has not been placed and fixed.", inst->getName());
    }
  }
  if (unplaced_macros) {
    logger_->error(utl::PDN, 235, "Design has unplaced macros.");
  }
}

void PdnGen::checkSetup() const
{
  auto* block = db_->getChip()->getBlock();

  checkDesign(block);

  for (auto* domain : getDomains()) {
    domain->checkSetup();
  }
}

void PdnGen::repairVias(const std::set<odb::dbNet*>& nets)
{
  ViaRepair repair(logger_, nets);
  repair.repair();
  repair.report();
}

}  // namespace pdn
