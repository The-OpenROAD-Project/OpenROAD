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

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

#include "connect.h"
#include "renderer.h"

#include <map>
#include <set>

#include <iostream>

namespace pdn {

using utl::Logger;
using utl::IFP;

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbMaster;
using odb::dbNet;
using odb::dbBox;

using std::regex;

using utl::PDN;

PdnGen::PdnGen() : db_(nullptr),
    logger_(nullptr),
    global_connect_(nullptr)
{
}

void
PdnGen::init(dbDatabase *db, Logger* logger) {
  db_ = db;
  logger_ = logger;
}

void
PdnGen::setSpecialITerms() {
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

void
PdnGen::setSpecialITerms(dbNet *net) {
  for (dbITerm* iterm : net->getITerms()) {
    iterm->setSpecial();
  }
}

void
PdnGen::globalConnect(dbBlock* block, std::shared_ptr<regex>& instPattern, std::shared_ptr<regex>& pinPattern, dbNet *net) {
  dbBox* bbox = nullptr;
  globalConnectRegion(block, bbox, instPattern, pinPattern, net);
}

void
PdnGen::globalConnect(dbBlock* block) {
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
    if (region == nullptr) continue;
    globalConnectRegion(block, region, net_regex_pairs);
  }

  setSpecialITerms();
}

void
PdnGen::globalConnectRegion(dbBlock* block, dbBox* region, std::shared_ptr<netRegexPairs> global_connect) {
  for (auto& [net, regex_pairs] : *global_connect) {
    for (auto& [inst_regex, pin_regex] : *regex_pairs) {
      globalConnectRegion(block, region, inst_regex, pin_regex, net);
    }
  }
}

void
PdnGen::globalConnectRegion(dbBlock* block, dbBox* region, std::shared_ptr<regex>& instPattern, std::shared_ptr<regex>& pinPattern, dbNet *net) {
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

void
PdnGen::findInstsInArea(dbBlock* block, dbBox* region, std::shared_ptr<regex>& instPattern, std::vector<dbInst*>& insts) {
  for (dbInst* inst : block->getInsts()) {
    if (std::regex_match(inst->getName().c_str(), *instPattern)) {
      if (region == nullptr) {
        insts.push_back(inst);
      } else {
        dbBox* box = inst->getBBox();

        if (region->yMin() <= box->yMin() &&
            region->xMin() <= box->xMin() &&
            region->xMax() >= box->xMax() &&
            region->yMax() >= box->yMax()) {
          insts.push_back(inst);
        }
      }
    }
  }
}

void
PdnGen::buildMasterPinMatchingMap(dbBlock* block, std::shared_ptr<regex>& pinPattern, std::map<dbMaster*, std::vector<dbMTerm*>>& masterpins) {
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

void
PdnGen::addGlobalConnect(const char* instPattern, const char* pinPattern, dbNet *net) {
  addGlobalConnect(nullptr, instPattern, pinPattern, net);
}

void
PdnGen::addGlobalConnect(dbBox* region, const char* instPattern, const char* pinPattern, dbNet *net) {
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

  netRegexes->at(net)->push_back(std::make_pair(std::make_shared<regex>(instPattern), std::make_shared<regex>(pinPattern)));
}

void
PdnGen::clearGlobalConnect() {
  if (global_connect_ != nullptr) {
    global_connect_ = nullptr;
  }
}

void PdnGen::reset()
{
  core_domain_ = nullptr;
  domains_.clear();
  rendererRedraw();
}

void PdnGen::buildGrids(bool trim)
{
  const std::vector<Grid*> grids = getGrids();
  for (auto* grid : grids) {
    grid->resetShapes();
  }

  ShapeTreeMap block_obs;
  makeInitialObstructions(block_obs);

  ShapeTreeMap all_shapes;
  makeInitialShapes(all_shapes);
  for (const auto& [layer, layer_shapes] : all_shapes) {
    auto& layer_obs = block_obs[layer];
    for (const auto& [box, shape] : layer_shapes) {
      layer_obs.insert({shape->getObstructionBox(), shape});
    }
  }

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
    for (const auto& [layer, shapes] : grid->getShapes()) {
      for (const auto& [box, shape] : shapes) {
        if (!shape->isModifiable()) {
          continue;
        }

        Shape* new_shape = nullptr;
        const odb::Rect new_rect = shape->getMinimumRect();
        if (new_rect == shape->getRect()) { // no change to shape
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

        auto* grid_shape = shape->getGridShape();
        if (new_shape == nullptr) {
          if (shape->isRemovable()) {
            grid_shape->removeShape(shape.get());
          }
        } else {
          grid_shape->replaceShape(shape.get(), {new_shape});
        }
      }
    }
  }
  debugPrint(logger_, utl::PDN, "Make", 2, "Trim shapes - end");
}

std::vector<VoltageDomain*> PdnGen::getDomains()
{
  std::vector<VoltageDomain*> domains;
  if (core_domain_ == nullptr) {
    this->setCoreDomain(nullptr, nullptr, {});
  }
  domains.push_back(core_domain_.get());

  for (const auto& domain : domains_) {
    domains.push_back(domain.get());
  }
  return domains;
}

std::vector<VoltageDomain*> PdnGen::getConstDomains() const
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
  auto domains = getDomains();
  if (name.empty()) {
    if (domains.empty()) {
      return nullptr;
    }

    return domains.back();
  }

  for (const auto& domain : domains) {
    if (domain->getName() == name) {
      return domain;
    }
  }

  return nullptr;
}

void PdnGen::setCoreDomain(odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary)
{
  auto* block = db_->getChip()->getBlock();
  core_domain_ = std::make_unique<CoreVoltageDomain>(block, power, ground, secondary, logger_);
}

void PdnGen::makeRegionVoltageDomain(const std::string& name, odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary_nets, odb::dbRegion* region)
{
  auto* block = db_->getChip()->getBlock();
  domains_.push_back(std::make_unique<VoltageDomain>(name, block, power, ground, secondary_nets, region, logger_));
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

    return {grids.back()};
  }

  for (auto* grid : grids) {
    if (grid->getName() == name || grid->getLongName() == name) {
      found_grids.push_back(grid);
    }
  }

  return found_grids;
}

void PdnGen::makeCoreGrid(VoltageDomain* domain, const std::string& name, bool starts_with_power, const std::vector<odb::dbTechLayer*>& pin_layers)
{
  auto grid = std::make_unique<CoreGrid>(domain, name, starts_with_power);
  grid->setPinLayers(pin_layers);
  domain->addGrid(std::move(grid));
}

void PdnGen::makeInstanceGrid(VoltageDomain* domain, const std::string& name, bool starts_with_power, odb::dbInst* inst, const std::array<int, 4>& halo)
{
  auto grid = std::make_unique<InstanceGrid>(domain, name, starts_with_power, inst);
  if (!std::all_of(halo.begin(), halo.end(), [](int v) { return v == 0; })) {
    grid->addHalo(halo);
  }
  domain->addGrid(std::move(grid));
}

void PdnGen::makeRing(Grid* grid, const std::array<Ring::Layer, 2>& layers, const std::array<int, 4>& offset, const std::array<int, 4>& pad_offset, bool extend, const std::vector<odb::dbTechLayer*>& pad_pin_layers)
{
  auto ring = std::make_unique<Ring>(grid, layers);
  ring->setOffset(offset);
  if (std::any_of(pad_offset.begin(), pad_offset.end(), [](int o) { return o != 0; })) {
    ring->setPadOffset(pad_offset);
  }
  ring->setExtendToBoundary(extend);
  grid->addRing(std::move(ring));
  if (!pad_pin_layers.empty() && grid->type() == Grid::Core) {
    auto* core_grid = static_cast<CoreGrid*>(grid);
    core_grid->setupDirectConnect(pad_pin_layers);
  }
  rendererRedraw();
}

void PdnGen::makeFollowpin(Grid* grid, odb::dbTechLayer* layer, int width, Strap::Extend extend)
{
  auto strap = std::make_unique<FollowPin>(grid, layer, width);
  strap->setExtend(extend);

  grid->addStrap(std::move(strap));
  rendererRedraw();
}

void PdnGen::makeStrap(Grid* grid, odb::dbTechLayer* layer, int width, int spacing, int pitch, int offset, int number_of_straps, bool snap, bool use_grid_power_order, bool power_first, Strap::Extend extend)
{
  auto strap = std::make_unique<Strap>(grid, layer, width, pitch, spacing, number_of_straps);
  strap->setExtend(extend);
  strap->setOffset(offset);
  strap->setSnapToGrid(snap);
  if (!use_grid_power_order) {
    strap->setStartWithPower(power_first);
  }
  grid->addStrap(std::move(strap));
  rendererRedraw();
}

void PdnGen::makeConnect(Grid* grid, odb::dbTechLayer* layer0, odb::dbTechLayer* layer1, int cut_pitch_x, int cut_pitch_y, const std::vector<odb::dbTechViaGenerateRule*>& vias, const std::vector<odb::dbTechVia*>& techvias)
{
  auto con = std::make_unique<Connect>(layer0, layer1);
  con->setCutPitch(cut_pitch_x, cut_pitch_y);

  for (auto* via : vias) {
    con->addFixedVia(via);
  }

  for (auto* via : techvias) {
    con->addFixedVia(via);
  }

  grid->addConnect(std::move(con));
  rendererRedraw();
}

void PdnGen::toggleDebugRenderer(bool on)
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
    buildGrids(false);
    debug_renderer_->update();
  }
}

void PdnGen::makeInitialObstructions(ShapeTreeMap& obs)
{
  // routing obs
  for (auto* ob : db_->getChip()->getBlock()->getObstructions()) {
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
      for (auto* layer : db_->getTech()->getLayers()) {
        auto shape = std::make_shared<Shape>(layer, nullptr, obs_rect);
        obs[shape->getLayer()].insert({shape->getObstructionBox(), shape});
      }
    } else {
      auto shape = std::make_shared<Shape>(box->getTechLayer(), nullptr, obs_rect);

      obs[shape->getLayer()].insert({shape->getObstructionBox(), shape});
    }
  }

  // placed block obs
  for (auto* inst : db_->getChip()->getBlock()->getInsts()) {
    if (!inst->isBlock()) {
      continue;
    }

    if (!inst->isPlaced()) {
      continue;
    }

    odb::dbTransform transform;
    inst->getTransform(transform);

    for (auto* ob : inst->getMaster()->getObstructions()) {
      odb::Rect obs_rect;
      ob->getBox(obs_rect);

      transform.apply(obs_rect);
      auto shape = std::make_shared<Shape>(ob->getTechLayer(), nullptr, obs_rect);

      obs[ob->getTechLayer()].insert({shape->getObstructionBox(), shape});
    }
  }
}

void PdnGen::makeInitialShapes(ShapeTreeMap& shapes)
{
  // get special shapes
  std::set<odb::dbNet*> nets;
  for (auto* domain : getConstDomains()) {
    for (auto* net : domain->getNets()) {
      nets.insert(net);
    }
  }

  for (auto* net : nets) {
    Shape::populateMapFromDb(net, shapes);
  }
}

void PdnGen::writeToDb(bool add_pins) const
{
  std::map<odb::dbNet*, odb::dbSWire*> net_map;

  auto domains = getConstDomains();
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
    }
  }
}

void PdnGen::ripUp(odb::dbNet* net)
{
  if (net == nullptr) {
    std::set<odb::dbNet*> nets;
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
        Box search_box(Point(rect.xMin(), rect.yMin()), Point(rect.xMax(), rect.yMax()));
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
  for (auto* swire : net->getSWires()) {
    odb::dbSWire::destroy(swire);
  }
}

void PdnGen::report()
{
  for (auto* domain : getDomains()) {
    domain->report();
  }
}

} // namespace
