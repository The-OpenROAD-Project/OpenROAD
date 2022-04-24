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
#include "renderer.h"
#include "rings.h"
#include "straps.h"
#include "utl/Logger.h"

namespace pdn {

using utl::IFP;
using utl::Logger;

using odb::dbBlock;
using odb::dbBox;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbMTerm;
using odb::dbNet;

using std::regex;

using utl::PDN;

PdnGen::PdnGen() : db_(nullptr), logger_(nullptr), global_connect_(nullptr)
{
}

void PdnGen::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void PdnGen::setSpecialITerms()
{
  if (global_connect_ == nullptr) {
    logger_->warn(PDN, 49, "Global connections not set up.");
    return;
  }

  std::set<dbNet*> global_nets = std::set<dbNet*>();
  for (auto& [box, net_regex_pairs] : *global_connect_) {
    for (auto& [net, regex_pairs] : *net_regex_pairs) {
      global_nets.insert(net);
    }
  }

  for (dbNet* net : global_nets)
    setSpecialITerms(net);
}

void PdnGen::setSpecialITerms(dbNet* net)
{
  for (dbITerm* iterm : net->getITerms()) {
    iterm->setSpecial();
  }
}

void PdnGen::globalConnect(dbBlock* block,
                           std::shared_ptr<regex>& instPattern,
                           std::shared_ptr<regex>& pinPattern,
                           dbNet* net)
{
  dbBox* bbox = nullptr;
  globalConnectRegion(block, bbox, instPattern, pinPattern, net);
}

void PdnGen::globalConnect(dbBlock* block)
{
  if (global_connect_ == nullptr) {
    logger_->warn(PDN, 50, "Global connections not set up.");
    return;
  }

  // Do non-regions first
  if (global_connect_->find(nullptr) != global_connect_->end()) {
    globalConnectRegion(block, nullptr, global_connect_->at(nullptr));
  }

  // Do regions
  for (auto& [region, net_regex_pairs] : *global_connect_) {
    if (region == nullptr)
      continue;
    globalConnectRegion(block, region, net_regex_pairs);
  }

  setSpecialITerms();
}

void PdnGen::globalConnectRegion(dbBlock* block,
                                 dbBox* region,
                                 std::shared_ptr<netRegexPairs> global_connect)
{
  for (auto& [net, regex_pairs] : *global_connect) {
    for (auto& [inst_regex, pin_regex] : *regex_pairs) {
      globalConnectRegion(block, region, inst_regex, pin_regex, net);
    }
  }
}

void PdnGen::globalConnectRegion(dbBlock* block,
                                 dbBox* region,
                                 std::shared_ptr<regex>& instPattern,
                                 std::shared_ptr<regex>& pinPattern,
                                 dbNet* net)
{
  if (net == nullptr) {
    logger_->warn(PDN, 60, "Unable to add invalid net.");
    return;
  }

  std::vector<dbInst*> insts;
  findInstsInArea(block, region, instPattern, insts);

  if (insts.empty()) {
    return;
  }

  std::map<dbMaster*, std::vector<dbMTerm*>> masterpins;
  buildMasterPinMatchingMap(block, pinPattern, masterpins);

  for (dbInst* inst : insts) {
    dbMaster* master = inst->getMaster();

    auto masterpin = masterpins.find(master);
    if (masterpin != masterpins.end()) {
      std::vector<dbMTerm*>* mterms = &masterpin->second;

      for (dbMTerm* mterm : *mterms) {
        inst->getITerm(mterm)->connect(net);
      }
    }
  }
}

void PdnGen::findInstsInArea(dbBlock* block,
                             dbBox* region,
                             std::shared_ptr<regex>& instPattern,
                             std::vector<dbInst*>& insts)
{
  for (dbInst* inst : block->getInsts()) {
    if (std::regex_match(inst->getName().c_str(), *instPattern)) {
      if (region == nullptr) {
        insts.push_back(inst);
      } else {
        dbBox* box = inst->getBBox();

        if (region->yMin() <= box->yMin() && region->xMin() <= box->xMin()
            && region->xMax() >= box->xMax() && region->yMax() >= box->yMax()) {
          insts.push_back(inst);
        }
      }
    }
  }
}

void PdnGen::buildMasterPinMatchingMap(
    dbBlock* block,
    std::shared_ptr<regex>& pinPattern,
    std::map<dbMaster*, std::vector<dbMTerm*>>& masterpins)
{
  std::vector<dbMaster*> masters;
  block->getMasters(masters);

  for (dbMaster* master : masters) {
    masterpins.emplace(master, std::vector<dbMTerm*>());

    std::vector<dbMTerm*>* mastermterms = &masterpins.at(master);
    for (dbMTerm* mterm : master->getMTerms()) {
      if (std::regex_match(mterm->getName().c_str(), *pinPattern)) {
        mastermterms->push_back(mterm);
      }
    }
  }
}

void PdnGen::addGlobalConnect(const char* instPattern,
                              const char* pinPattern,
                              dbNet* net)
{
  addGlobalConnect(nullptr, instPattern, pinPattern, net);
}

void PdnGen::addGlobalConnect(dbBox* region,
                              const char* instPattern,
                              const char* pinPattern,
                              dbNet* net)
{
  if (net == nullptr) {
    logger_->warn(PDN, 61, "Unable to add invalid net.");
    return;
  }

  if (global_connect_ == nullptr) {
    global_connect_ = std::make_unique<regionNetRegexPairs>();
  }

  // check if region is present in map, add if not
  if (global_connect_->find(region) == global_connect_->end()) {
    global_connect_->emplace(region, std::make_shared<netRegexPairs>());
  }

  std::shared_ptr<netRegexPairs> netRegexes = global_connect_->at(region);
  // check if net is present in region mapping
  if (netRegexes->find(net) == netRegexes->end()) {
    netRegexes->emplace(net, std::make_shared<regexPairs>());
  }

  netRegexes->at(net)->push_back(
      std::make_pair(std::make_shared<regex>(instPattern),
                     std::make_shared<regex>(pinPattern)));
}

void PdnGen::clearGlobalConnect()
{
  global_connect_ = nullptr;
}

void PdnGen::reset()
{
  core_domain_ = nullptr;
  domains_.clear();
}

void PdnGen::resetShapes()
{
  for (auto* grid : getGrids()) {
    grid->resetShapes();
  }
}

void PdnGen::buildGrids(bool trim)
{
  auto* block = db_->getChip()->getBlock();

  resetShapes();

  ShapeTreeMap block_obs;
  Grid::makeInitialObstructions(block, block_obs);

  ShapeTreeMap all_shapes;

  // get special shapes
  std::set<odb::dbNet*> nets;
  for (auto* domain : getDomains()) {
    for (auto* net : domain->getNets()) {
      nets.insert(net);
    }
  }
  Grid::makeInitialShapes(nets, all_shapes);
  for (const auto& [layer, layer_shapes] : all_shapes) {
    auto& layer_obs = block_obs[layer];
    for (const auto& [box, shape] : layer_shapes) {
      layer_obs.insert({shape->getObstructionBox(), shape});
    }
  }

  const std::vector<Grid*> grids = getGrids();
  for (auto* grid : grids) {
    ShapeTreeMap obs_local = block_obs;
    for (auto* grid_other : grids) {
      if (grid != grid_other) {
        grid_other->getGridLevelObstructions(obs_local);
        grid_other->getObstructions(obs_local);
      }
    }
    grid->makeShapes(all_shapes, obs_local);
    for (const auto& [layer, shapes] : grid->getShapes()) {
      auto& all_shapes_layer = all_shapes[layer];
      for (auto& shape : shapes) {
        all_shapes_layer.insert(shape);
      }
    }
  }

  if (trim) {
    trimShapes();

    cleanupVias();
  }

  if (debug_renderer_ != nullptr) {
    debug_renderer_->update();
  }
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
        // only trim shapes on non pin layers
        if (pin_layers.find(shape->getLayer()) != pin_layers.end()) {
          continue;
        }

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
          component->replaceShape(shape.get(), {new_shape});
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
    setCoreDomain(nullptr, nullptr, {});
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
                           odb::dbNet* ground,
                           const std::vector<odb::dbNet*>& secondary)
{
  auto* block = db_->getChip()->getBlock();
  if (core_domain_ != nullptr) {
    logger_->warn(utl::PDN, 183, "Replacing existing core voltage domain.");
  }
  core_domain_ = std::make_unique<VoltageDomain>(
      this, block, power, ground, secondary, logger_);
}

void PdnGen::makeRegionVoltageDomain(
    const std::string& name,
    odb::dbNet* power,
    odb::dbNet* ground,
    const std::vector<odb::dbNet*>& secondary_nets,
    odb::dbRegion* region)
{
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
  domains_.push_back(std::make_unique<VoltageDomain>(
      this, name, block, power, ground, secondary_nets, region, logger_));
}

void PdnGen::setVoltageDomainSwitchedPower(
    VoltageDomain *voltage_domain,
    odb::dbNet* switched_power)
{
  voltage_domain->setSwitchedPower(switched_power);
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

void PdnGen::makeCoreGrid(VoltageDomain* domain,
                          const std::string& name,
                          StartsWith starts_with,
                          const std::vector<odb::dbTechLayer*>& pin_layers,
                          const std::vector<odb::dbTechLayer*>& generate_obstructions)
{
  auto grid = std::make_unique<CoreGrid>(domain, name, starts_with == POWER, generate_obstructions);
  grid->setPinLayers(pin_layers);
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

void PdnGen::makeInstanceGrid(VoltageDomain* domain,
                              const std::string& name,
                              StartsWith starts_with,
                              odb::dbInst* inst,
                              const std::array<int, 4>& halo,
                              bool pg_pins_to_boundary,
                              bool default_grid,
                              const std::vector<odb::dbTechLayer*>& generate_obstructions)
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

  auto grid
      = std::make_unique<InstanceGrid>(domain, name, starts_with == POWER, inst, generate_obstructions);
  if (!std::all_of(halo.begin(), halo.end(), [](int v) { return v == 0; })) {
    grid->addHalo(halo);
  }
  grid->setGridToBoundary(pg_pins_to_boundary);

  grid->setReplaceable(default_grid);

  domain->addGrid(std::move(grid));
}

void PdnGen::makeExistingGrid(const std::string& name,
                              const std::vector<odb::dbTechLayer*>& generate_obstructions)
{
  auto grid = std::make_unique<ExistingGrid>(this, db_->getChip()->getBlock(), logger_, name, generate_obstructions);

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
                      const std::vector<odb::dbTechLayer*>& pad_pin_layers)
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
                       ExtensionMode extend)
{
  auto strap = std::make_unique<Straps>(
      grid, layer, width, pitch, spacing, number_of_straps);
  strap->setExtend(extend);
  strap->setOffset(offset);
  strap->setSnapToGrid(snap);
  if (starts_with != GRID) {
    strap->setStartWithPower(starts_with == POWER);
  }
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
      debug_renderer_->update();
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

void PdnGen::writeToDb(bool add_pins) const
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
      const auto nets = domain->getNets();
      if (std::find(nets.begin(), nets.end(), net) == nets.end()) {
        appear_in_all_grids = false;
      }
    }

    if (appear_in_all_grids) {
      // should this be based on the global connect?
      net->setWildConnected();
    }

    swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  }

  for (auto* domain : domains) {
    for (const auto& grid : domain->getGrids()) {
      grid->writeToDb(net_map, add_pins);
      grid->makeRoutingObstructions(db_->getChip()->getBlock());
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

    for (odb::dbNet* net : nets) {
      ripUp(net);
    }

    return;
  }

  ShapeTreeMap net_shapes;
  Shape::populateMapFromDb(net, net_shapes);
  // remove bterms that connect to swires
  for (auto* bterm : net->getBTerms()) {
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

        odb::Rect rect;
        box->getBox(rect);
        const auto& shapes = net_shapes[layer];
        Box search_box(Point(rect.xMin(), rect.yMin()),
                       Point(rect.xMax(), rect.yMax()));
        if (shapes.qbegin(bgi::intersects(search_box)) != shapes.qend()) {
          remove = true;
          break;
        }
      }
      if (remove) {
        odb::dbBPin::destroy(pin);
      }
    }
    if (bterm->getBPins().empty()) {
      odb::dbBTerm::destroy(bterm);
    }
  }
  auto swires = net->getSWires();
  for (auto iter = swires.begin(); iter != swires.end(); ) {
    iter = odb::dbSWire::destroy(iter);
  }
}

void PdnGen::report()
{
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
    for (auto* term : inst->getITerms()) {
      if (term->getSigType().isSupply() && term->getNet() == nullptr) {
        logger_->warn(utl::PDN,
                      189,
                      "Supply pin {} of instance {} is not connected to any net.",
                      term->getMTerm()->getName(),
                      inst->getName());
      }
    }
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

}  // namespace pdn
