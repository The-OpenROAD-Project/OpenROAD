// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pdn/PdnGen.hh"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "connect.h"
#include "domain.h"
#include "grid.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "power_cells.h"
#include "renderer.h"
#include "rings.h"
#include "shape.h"
#include "sroute.h"
#include "straps.h"
#include "techlayer.h"
#include "utl/Logger.h"
#include "via.h"
#include "via_repair.h"

namespace pdn {

using utl::Logger;

PdnGen::PdnGen(odb::dbDatabase* db, Logger* logger) : db_(db), logger_(logger)
{
  sroute_ = std::make_unique<SRoute>(this, db, logger_);
}

PdnGen::~PdnGen() = default;

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

  std::set<odb::dbNet*> grid_nets;
  for (auto* grid : grids) {
    const auto nets = grid->getNets();
    grid_nets.insert(nets.begin(), nets.end());
  }

  ShapeVectorMap block_obs_vec;
  Grid::makeInitialObstructions(
      block, block_obs_vec, insts_in_grids, grid_nets, logger_);
  for (auto* grid : grids) {
    grid->getGridLevelObstructions(block_obs_vec);
  }
  ShapeVectorMap all_shapes_vec;

  // get special shapes
  Grid::makeInitialShapes(block, all_shapes_vec, logger_);
  for (const auto& [layer, layer_shapes] : all_shapes_vec) {
    auto& layer_obs = block_obs_vec[layer];
    for (const auto& shape : layer_shapes) {
      layer_obs.push_back(shape);
    }
  }

  Shape::ObstructionTreeMap block_obs;
  for (const auto& [layer, shapes] : block_obs_vec) {
    block_obs[layer] = Shape::ObstructionTree(shapes.begin(), shapes.end());
  }
  block_obs_vec.clear();

  Shape::ShapeTreeMap all_shapes;
  for (const auto& [layer, shapes] : all_shapes_vec) {
    all_shapes[layer] = Shape::ShapeTree(shapes.begin(), shapes.end());
  }
  all_shapes_vec.clear();

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

  updateVias();

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
                  "The grid \"{}\" ({}) does not contain any shapes or vias.",
                  grid->getLongName(),
                  Grid::typeToString(grid->type()));
    failed = true;
  }

  updateRenderer();

  if (failed) {
    logger_->error(utl::PDN, 233, "Failed to generate full power grid.");
  }

  debugPrint(logger_, utl::PDN, "Make", 1, "Build - end");
}

void PdnGen::cleanupVias()
{
  debugPrint(logger_, utl::PDN, "Make", 2, "Cleanup vias - begin");
  for (auto* grid : getGrids()) {
    grid->removeInvalidVias();
  }
  updateVias();
  debugPrint(logger_, utl::PDN, "Make", 2, "Cleanup vias - end");
}

void PdnGen::updateVias()
{
  debugPrint(logger_, utl::PDN, "Make", 2, "Update vias - start");

  const auto grids = getGrids();

  for (auto* grid : grids) {
    for (const auto& [layer, shapes] : grid->getShapes()) {
      for (const auto& shape : shapes) {
        shape->clearVias();
      }
    }

    std::vector<ViaPtr> all_vias;
    grid->getVias(all_vias);

    for (const auto& via : all_vias) {
      via->getLowerShape()->addVia(via);
      via->getUpperShape()->addVia(via);
    }
  }

  debugPrint(logger_, utl::PDN, "Make", 2, "Update vias - end");
}

void PdnGen::trimShapes()
{
  debugPrint(logger_, utl::PDN, "Make", 2, "Trim shapes - start");
  auto grids = getGrids();

  for (auto* grid : grids) {
    if (grid->type() == Grid::Existing) {
      // fixed shapes, so nothing to do
      continue;
    }
    const auto& pin_layers = grid->getPinLayers();
    for (const auto& [layer, shapes] : grid->getShapes()) {
      for (const auto& shape : shapes) {
        if (!shape->isModifiable()) {
          continue;
        }

        // if pin layer, do not modify the shapes, but allow them to be
        // removed if they are not connected to anything
        const bool is_pin_layer
            = pin_layers.find(shape->getLayer()) != pin_layers.end();

        std::unique_ptr<Shape> new_shape = nullptr;
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
          if (shape->isRemovable(is_pin_layer)) {
            component->removeShape(shape.get());
          }
        } else {
          if (!is_pin_layer) {
            component->replaceShape(shape.get(), std::move(new_shape));
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

  if (importUPF(core_domain_.get())) {
    if (switched_power) {
      logger_->error(
          utl::PDN, 210, "Cannot specify switched power net when using UPF.");
    }
  } else {
    core_domain_->setSwitchedPower(switched_power);
  }
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

  if (importUPF(domain.get())) {
    if (switched_power) {
      logger_->error(
          utl::PDN, 199, "Cannot specify switched power net when using UPF.");
    }
  } else {
    domain->setSwitchedPower(switched_power);
  }

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
    const char* powercontrolnetwork,
    const std::vector<odb::dbTechLayer*>& pad_pin_layers)
{
  auto grid = std::make_unique<CoreGrid>(
      domain, name, starts_with == POWER, generate_obstructions);
  grid->setPinLayers(pin_layers);

  PowerSwitchNetworkType control_network = PowerSwitchNetworkType::DAISY;
  if (strlen(powercontrolnetwork) > 0) {
    control_network
        = GridSwitchedPower::fromString(powercontrolnetwork, logger_);
  }
  if (importUPF(grid.get(), control_network)) {
    if (powercell != nullptr) {
      logger_->error(
          utl::PDN, 201, "Cannot specify power switch when UPF is available.");
    }
    if (powercontrol != nullptr) {
      logger_->error(
          utl::PDN, 202, "Cannot specify power control when UPF is available.");
    }
  } else {
    if (powercell != nullptr) {
      grid->setSwitchedPower(new GridSwitchedPower(
          grid.get(),
          powercell,
          powercontrol,
          GridSwitchedPower::fromString(powercontrolnetwork, logger_)));
    }
  }
  if (!pad_pin_layers.empty()) {
    grid->setupDirectConnect(pad_pin_layers);
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
  if (!std::ranges::all_of(halo, [](int v) { return v == 0; })) {
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
                      const std::vector<odb::dbNet*>& nets,
                      bool allow_out_of_die)
{
  auto ring = std::make_unique<Rings>(grid,
                                      Rings::Layer{layer0, width0, spacing0},
                                      Rings::Layer{layer1, width1, spacing1});
  ring->setOffset(offset);
  if (std::ranges::any_of(pad_offset, [](int o) { return o != 0; })) {
    ring->setPadOffset(pad_offset);
  }
  ring->setExtendToBoundary(extend);
  if (starts_with != GRID) {
    ring->setStartWithPower(starts_with == POWER);
  }
  if (allow_out_of_die) {
    ring->setAllowOutsideDieArea();
  }
  ring->setNets(nets);
  grid->addRing(std::move(ring));
  if (!pad_pin_layers.empty() && grid->type() == Grid::Core) {
    auto* core_grid = static_cast<CoreGrid*>(grid);
    core_grid->setupDirectConnect(pad_pin_layers);
    for (const auto& comp : core_grid->getStraps()) {
      PadDirectConnectionStraps* straps
          = dynamic_cast<PadDirectConnectionStraps*>(comp.get());
      if (straps) {
        straps->setTargetType(odb::dbWireShapeType::RING);
      }
    }
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
                       const std::vector<odb::dbNet*>& nets,
                       bool allow_out_of_core)
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
  strap->setAllowOutsideCoreArea(allow_out_of_core);
  grid->addStrap(std::move(strap));
}

void PdnGen::makeConnect(
    Grid* grid,
    odb::dbTechLayer* layer0,
    odb::dbTechLayer* layer1,
    int cut_pitch_x,
    int cut_pitch_y,
    const std::vector<odb::dbTechViaGenerateRule*>& vias,
    const std::vector<odb::dbTechVia*>& techvias,
    int max_rows,
    int max_columns,
    const std::vector<odb::dbTechLayer*>& ongrid,
    const std::map<odb::dbTechLayer*, std::pair<int, bool>>& split_cuts,
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

  std::map<odb::dbTechLayer*, Connect::SplitCut> split_cuts_map;
  for (const auto& [layer, cut_def] : split_cuts) {
    split_cuts_map[layer]
        = Connect::SplitCut{std::get<0>(cut_def), std::get<1>(cut_def)};
  }
  con->setSplitCuts(split_cuts_map);

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
    const std::vector<int>& metalwidths,
    const std::vector<int>& metalspaces,
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
    swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  }

  // collect all the SWires from the block
  auto* block = db_->getChip()->getBlock();
  ShapeVectorMap net_shapes_vec;
  for (auto* net : block->getNets()) {
    Shape::populateMapFromDb(net, net_shapes_vec);
  }
  const Shape::ObstructionTreeMap obstructions(net_shapes_vec.begin(),
                                               net_shapes_vec.end());
  net_shapes_vec.clear();

  // Remove existing non-fixed bpins
  for (auto& [net, swire] : net_map) {
    for (auto* bterm : net->getBTerms()) {
      auto bpins = bterm->getBPins();
      std::set<odb::dbBPin*> pins(bpins.begin(), bpins.end());
      for (auto* bpin : pins) {
        if (!bpin->getPlacementStatus().isFixed()) {
          odb::dbBPin::destroy(bpin);
        }
      }
    }
  }

  std::map<Shape*, std::vector<odb::dbBox*>> shape_map;
  for (auto* domain : domains) {
    for (const auto& grid : domain->getGrids()) {
      const auto db_shapes = grid->writeToDb(net_map, add_pins, obstructions);
      shape_map.insert(db_shapes.begin(), db_shapes.end());

      grid->makeRoutingObstructions(db_->getChip()->getBlock());
    }
  }

  // Cleanup floating shapes due to failed vias
  for (const auto& [shape, db_shapes] : shape_map) {
    if (!shape->isLocked() && !shape->hasInternalConnections()) {
      for (odb::dbBox* db_box : db_shapes) {
        if (db_box == nullptr) {
          continue;
        }
        if (db_box->getObjectType() == odb::dbObjectType::dbSBoxObj) {
          odb::dbSBox::destroy((odb::dbSBox*) db_box);
        } else {
          odb::dbBox::destroy(db_box);
        }
      }
    }
  }

  // Remove empty swires
  for (auto& [net, swire] : net_map) {
    if (swire->getWires().empty()) {
      odb::dbSWire::destroy(swire);
      logger_->warn(
          utl::PDN, 213, "No shapes were created for net {}.", net->getName());
    }
  }

  // remove stale results
  odb::dbMarkerCategory* category = block->findMarkerCategory("PDN");
  if (category != nullptr) {
    odb::dbMarkerCategory::destroy(category);
  }

  for (auto* grid : getGrids()) {
    for (const auto& connect : grid->getConnect()) {
      connect->recordFailedVias();
    }
  }

  if (!report_file.empty()) {
    odb::dbMarkerCategory* category = block->findMarkerCategory("PDN");
    if (category != nullptr) {
      category->writeTR(report_file);
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

  ShapeVectorMap net_shapes_vec;
  Shape::populateMapFromDb(net, net_shapes_vec);
  Shape::ShapeTreeMap net_shapes = Shape::convertVectorToTree(net_shapes_vec);

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
        if (!net_shapes.contains(layer)) {
          continue;
        }

        odb::Rect rect = box->getBox();
        const auto& shapes = net_shapes[layer];
        if (shapes.qbegin(bgi::intersects(rect)) != shapes.qend()) {
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
        logger_->warn(utl::PDN,
                      189,
                      "Supply pin {} is not connected to any net.",
                      term->getName());
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

bool PdnGen::importUPF(VoltageDomain* domain)
{
  auto* block = db_->getChip()->getBlock();

  bool has_upf = false;
  odb::dbPowerDomain* power_domain = nullptr;
  for (auto* upf_domain : block->getPowerDomains()) {
    has_upf = true;

    if (domain == core_domain_.get()) {
      if (upf_domain->isTop()) {
        power_domain = upf_domain;
        break;
      }
    } else {
      odb::dbGroup* upf_group = upf_domain->getGroup();

      if (upf_group != nullptr
          && upf_group->getRegion() == domain->getRegion()) {
        power_domain = upf_domain;
        break;
      }
    }
  }

  if (power_domain != nullptr) {
    const auto power_switches = power_domain->getPowerSwitches();
    if (power_switches.size() > 1) {
      logger_->error(
          utl::PDN,
          203,
          "Unable to process power domain with more than 1 power switch");
    }

    for (auto* pswitch : power_switches) {
      auto port_map = pswitch->getPortMap();
      if (port_map.empty()) {
        logger_->error(
            utl::PDN,
            204,
            "Unable to process power switch, {}, without port mapping",
            pswitch->getName());
      }

      odb::dbMaster* master = pswitch->getLibCell();
      odb::dbMTerm* control = nullptr;
      odb::dbMTerm* acknowledge = nullptr;
      odb::dbMTerm* switched_power = nullptr;
      odb::dbMTerm* alwayson_power = nullptr;
      odb::dbMTerm* ground = nullptr;

      const auto control_port = pswitch->getControlPorts();
      const auto input_supply = pswitch->getInputSupplyPorts();
      const auto output_supply = pswitch->getOutputSupplyPort();
      const auto ack_port = pswitch->getAcknowledgePorts();

      for (auto* mterm : master->getMTerms()) {
        if (mterm->getSigType() == odb::dbSigType::GROUND) {
          ground = mterm;
        }
      }

      control = port_map[control_port[0].port_name];
      alwayson_power = port_map[input_supply[0].port_name];
      switched_power = port_map[output_supply.port_name];
      acknowledge = port_map[ack_port[0].port_name];

      if (control == nullptr) {
        logger_->error(utl::PDN,
                       205,
                       "Unable to determine control port for: {}",
                       master->getName());
      }

      if (alwayson_power == nullptr) {
        logger_->error(utl::PDN,
                       206,
                       "Unable to determine always on power port for: {}",
                       master->getName());
      }

      if (switched_power == nullptr) {
        logger_->error(utl::PDN,
                       207,
                       "Unable to determine switched power port for: {}",
                       master->getName());
      }

      if (ground == nullptr) {
        logger_->error(utl::PDN,
                       208,
                       "Unable to determine ground port for: {}",
                       master->getName());
      }

      auto* switched_net
          = block->findNet(output_supply.supply_net_name.c_str());
      if (switched_net == nullptr) {
        logger_->error(utl::PDN, 238, "Unable to determine switched power net");
      }
      domain->setSwitchedPower(switched_net);

      if (findSwitchedPowerCell(master->getName())) {
        logger_->warn(utl::PDN,
                      209,
                      "Power switch for {} already exists",
                      pswitch->getName());
      } else {
        makeSwitchedPowerCell(master,
                              control,
                              acknowledge,
                              switched_power,
                              alwayson_power,
                              ground);
      }
    }
  }

  return has_upf;
}

bool PdnGen::importUPF(Grid* grid, PowerSwitchNetworkType type) const
{
  auto* block = db_->getChip()->getBlock();

  auto* domain = grid->getDomain();

  bool has_upf = false;
  odb::dbPowerDomain* power_domain = nullptr;
  for (auto* upf_domain : block->getPowerDomains()) {
    has_upf = true;

    if (domain == core_domain_.get()) {
      if (upf_domain->isTop()) {
        power_domain = upf_domain;
        break;
      }
    } else {
      odb::dbGroup* upf_group = upf_domain->getGroup();

      if (upf_group != nullptr
          && upf_group->getRegion() == domain->getRegion()) {
        power_domain = upf_domain;
        break;
      }
    }
  }

  if (power_domain != nullptr) {
    auto power_switches = power_domain->getPowerSwitches();
    if (!power_switches.empty()) {
      auto pswitch = power_switches[0];

      auto* pdn_switch
          = findSwitchedPowerCell(pswitch->getLibCell()->getName());

      const auto& control_ports = pswitch->getControlPorts();
      const auto& control_net = control_ports[0].net_name;
      if (control_net.empty()) {
        logger_->error(
            utl::PDN,
            236,
            "Cannot handle undefined control net for power switch: {}",
            pswitch->getName());
      }
      auto control = block->findNet(control_net.c_str());
      if (control == nullptr) {
        logger_->error(
            utl::PDN, 237, "Unable to find control net: {}", control_net);
      }

      grid->setSwitchedPower(
          new GridSwitchedPower(grid, pdn_switch, control, type));
    }
  }

  return has_upf;
}

}  // namespace pdn
