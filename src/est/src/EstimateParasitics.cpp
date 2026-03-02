// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/EstimateParasitics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#include "AbstractSteinerRenderer.h"
#include "EstimateParasiticsCallBack.h"
#include "MakeWireParasitics.h"
#include "OdbCallBack.h"
#include "db_sta/SpefWriter.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/SteinerTree.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Report.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"

namespace est {

using utl::EST;

using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::Parasitic;
using sta::Parasitics;
using sta::Pin;
using sta::PinSet;
using sta::Scene;

using odb::dbInst;
using odb::dbMasterType;
using odb::dbModInst;

EstimateParasitics::EstimateParasitics(utl::Logger* logger,
                                       utl::CallBackHandler* callback_handler,
                                       odb::dbDatabase* db,
                                       sta::dbSta* sta,
                                       stt::SteinerTreeBuilder* stt_builder,
                                       grt::GlobalRouter* global_router)
    : logger_(logger),
      estimate_parasitics_cbk_(
          std::make_unique<EstimateParasiticsCallBack>(this)),
      stt_builder_(stt_builder),
      global_router_(global_router),
      db_network_(sta->getDbNetwork()),
      db_(db),
      db_cbk_(std::make_unique<OdbCallBack>(this, network_, db_network_)),
      wire_signal_res_(0.0),
      wire_signal_cap_(0.0),
      wire_clk_res_(0.0),
      wire_clk_cap_(0.0)
{
  estimate_parasitics_cbk_->setOwner(callback_handler);
  dbStaState::init(sta);
  db_cbk_ = std::make_unique<OdbCallBack>(this, network_, db_network_);
}

EstimateParasitics::~EstimateParasitics() = default;

void EstimateParasitics::initSteinerRenderer(
    std::unique_ptr<est::AbstractSteinerRenderer> steiner_renderer)
{
  if (steiner_renderer_) {
    logger_->warn(EST, 4, "Steiner renderer already initialized.");
    return;
  }
  steiner_renderer_ = std::move(steiner_renderer);
}

////////////////////////////////////////////////////////////////

void EstimateParasitics::setLayerRC(odb::dbTechLayer* layer,
                                    const sta::Scene* corner,
                                    double res,
                                    double cap)
{
  if (layer_res_.empty()) {
    int layer_count = db_->getTech()->getLayerCount();
    int corner_count = sta_->scenes().size();
    layer_res_.resize(layer_count);
    layer_cap_.resize(layer_count);
    for (int i = 0; i < layer_count; i++) {
      layer_res_[i].resize(corner_count);
      layer_cap_[i].resize(corner_count);
    }
  }

  layer_res_[layer->getNumber()][corner->index()] = res;
  layer_cap_[layer->getNumber()][corner->index()] = cap;
}

void EstimateParasitics::layerRC(odb::dbTechLayer* layer,
                                 const sta::Scene* corner,
                                 // Return values.
                                 double& res,
                                 double& cap) const
{
  if (layer_res_.empty()) {
    res = 0.0;
    cap = 0.0;
  } else {
    const int layer_level = layer->getNumber();
    res = layer_res_[layer_level][corner->index()];
    cap = layer_cap_[layer_level][corner->index()];
  }
}

////////////////////////////////////////////////////////////////

void EstimateParasitics::addClkLayer(odb::dbTechLayer* layer)
{
  clk_layers_.push_back(layer);
}

void EstimateParasitics::addSignalLayer(odb::dbTechLayer* layer)
{
  signal_layers_.push_back(layer);
}

void EstimateParasitics::sortClkAndSignalLayers()
{
  auto sortLayers = [](const odb::dbTechLayer* a, const odb::dbTechLayer* b) {
    return a->getNumber() < b->getNumber();
  };

  std::ranges::sort(clk_layers_, sortLayers);
  std::ranges::sort(signal_layers_, sortLayers);
}

void EstimateParasitics::setHWireSignalRC(const sta::Scene* scene,
                                          double res,
                                          double cap)
{
  wire_signal_res_.resize(sta_->scenes().size());
  wire_signal_cap_.resize(sta_->scenes().size());
  wire_signal_res_[scene->index()].h_res = res;
  wire_signal_cap_[scene->index()].h_cap = cap;
}
void EstimateParasitics::setVWireSignalRC(const sta::Scene* scene,
                                          double res,
                                          double cap)
{
  wire_signal_res_.resize(sta_->scenes().size());
  wire_signal_cap_.resize(sta_->scenes().size());
  wire_signal_res_[scene->index()].v_res = res;
  wire_signal_cap_[scene->index()].v_cap = cap;
}

double EstimateParasitics::wireSignalResistance(const sta::Scene* scene) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }

  return (wire_signal_res_[scene->index()].h_res
          + wire_signal_res_[scene->index()].v_res)
         / 2;
}

double EstimateParasitics::wireSignalHResistance(const sta::Scene* scene) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }
  return wire_signal_res_[scene->index()].h_res;
}

double EstimateParasitics::wireSignalVResistance(const sta::Scene* scene) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }
  return wire_signal_res_[scene->index()].v_res;
}

double EstimateParasitics::wireSignalCapacitance(const sta::Scene* scene) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }

  return (wire_signal_cap_[scene->index()].h_cap
          + wire_signal_cap_[scene->index()].v_cap)
         / 2;
}

double EstimateParasitics::wireSignalHCapacitance(const sta::Scene* scene) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }
  return wire_signal_cap_[scene->index()].h_cap;
}

double EstimateParasitics::wireSignalVCapacitance(const sta::Scene* scene) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }
  return wire_signal_cap_[scene->index()].v_cap;
}

void EstimateParasitics::wireSignalRC(const sta::Scene* scene,
                                      // Return values.
                                      double& res,
                                      double& cap) const
{
  if (wire_signal_res_.empty()) {
    res = 0.0;
  } else {
    auto resistance = wire_signal_res_[scene->index()];
    res = (resistance.h_res + resistance.v_res) / 2;
  }
  if (wire_signal_cap_.empty()) {
    cap = 0.0;
  } else {
    auto capacitance = wire_signal_cap_[scene->index()];
    cap = (capacitance.h_cap + capacitance.v_cap) / 2;
  }
}

void EstimateParasitics::setHWireClkRC(const sta::Scene* scene,
                                       double res,
                                       double cap)
{
  wire_clk_res_.resize(sta_->scenes().size());
  wire_clk_cap_.resize(sta_->scenes().size());
  wire_clk_res_[scene->index()].h_res = res;
  wire_clk_cap_[scene->index()].h_cap = cap;
}

void EstimateParasitics::setVWireClkRC(const sta::Scene* scene,
                                       double res,
                                       double cap)
{
  wire_clk_res_.resize(sta_->scenes().size());
  wire_clk_cap_.resize(sta_->scenes().size());
  wire_clk_res_[scene->index()].v_res = res;
  wire_clk_cap_[scene->index()].v_cap = cap;
}

double EstimateParasitics::wireClkResistance(const sta::Scene* scene) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }

  return (wire_clk_res_[scene->index()].h_res
          + wire_clk_res_[scene->index()].v_res)
         / 2;
}

double EstimateParasitics::wireClkHResistance(const sta::Scene* scene) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }

  return wire_clk_res_[scene->index()].h_res;
}

double EstimateParasitics::wireClkVResistance(const sta::Scene* scene) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }

  return wire_clk_res_[scene->index()].v_res;
}

double EstimateParasitics::wireClkCapacitance(const sta::Scene* scene) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return (wire_clk_cap_[scene->index()].h_cap
          + wire_clk_cap_[scene->index()].v_cap)
         / 2;
}

double EstimateParasitics::wireClkHCapacitance(const sta::Scene* scene) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return wire_clk_cap_[scene->index()].h_cap;
}

double EstimateParasitics::wireClkVCapacitance(const sta::Scene* scene) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return wire_clk_cap_[scene->index()].v_cap;
}

////////////////////////////////////////////////////////////////

void EstimateParasitics::setDbCbkOwner(odb::dbBlock* block)
{
  db_cbk_->addOwner(block);
}
void EstimateParasitics::removeDbCbkOwner()
{
  db_cbk_->removeOwner();
}

// block_ indicates core_, design_area_, db_network_ etc valid.
void EstimateParasitics::initBlock()
{
  if (db_->getChip() == nullptr) {
    logger_->error(EST, 162, "Database does not have a loaded design");
  }

  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }

  if (block_ == nullptr) {
    logger_->error(EST, 163, "Database has no block");
  }

  dbu_ = db_->getTech()->getDbUnitsPerMicron();
  if (!db_cbk_->hasOwner()) {
    db_cbk_->addOwner(block_);
  }
}

void EstimateParasitics::ensureParasitics()
{
  estimateParasitics(global_router_->haveRoutes()
                         ? ParasiticsSrc::global_routing
                         : ParasiticsSrc::placement);
}

void EstimateParasitics::estimateParasitics(ParasiticsSrc src)
{
  std::map<sta::Scene*, std::ostream*> spef_streams;
  estimateParasitics(src, spef_streams);
}

void EstimateParasitics::estimateParasitics(
    ParasiticsSrc src,
    std::map<sta::Scene*, std::ostream*>& spef_streams)
{
  initBlock();
  std::unique_ptr<sta::SpefWriter> spef_writer;
  if (!spef_streams.empty()) {
    spef_writer
        = std::make_unique<sta::SpefWriter>(logger_, sta_, spef_streams);
  }

  switch (src) {
    case ParasiticsSrc::placement:
      estimateWireParasitics(spef_writer.get());
      parasitics_src_ = ParasiticsSrc::placement;
      break;
    case ParasiticsSrc::global_routing:
      estimateGlobalRouteRC(spef_writer.get());
      parasitics_src_ = ParasiticsSrc::global_routing;
      break;
    case ParasiticsSrc::detailed_routing:
      // TODO: call rcx to extract parasitics and load them to STA
      parasitics_src_ = ParasiticsSrc::detailed_routing;
      break;
    case ParasiticsSrc::none:
      break;
  }
}

bool EstimateParasitics::haveEstimatedParasitics() const
{
  return parasitics_src_ != ParasiticsSrc::none;
}

void EstimateParasitics::updateParasitics(bool save_guides)
{
  if (!incremental_parasitics_enabled_) {
    logger_->error(
        EST,
        109,
        "updateParasitics() called with incremental parasitics disabled");
  }

  // Call clearNetDrvrPinMap only without full blown ConcreteNetwork::clear()
  // This is because netlist changes may invalidate cached net driver pin data
  sta::LibertyLibrary* default_lib = network_->defaultLibertyLibrary();
  network_->sta::Network::clear();
  network_->setDefaultLibertyLibrary(default_lib);

  switch (parasitics_src_) {
    case ParasiticsSrc::placement:
      for (const sta::Net* net : parasitics_invalid_) {
        //
        // TODO: remove this check (we expect all to be flat net)
        //
        if (!(db_network_->isFlat(net))) {
          debugPrint(logger_,
                     EST,
                     "estimate_parasitics",
                     1,
                     "non-flat net {} is skipped",
                     sdc_network_->pathName(net));
          continue;
        }
        debugPrint(logger_,
                   EST,
                   "estimate_parasitics",
                   1,
                   "net {} para is estimated for placement",
                   sdc_network_->pathName(net));
        estimateWireParasitic(net);
      }
      break;
    case ParasiticsSrc::global_routing:
    case ParasiticsSrc::detailed_routing: {
      // TODO: update detailed route for modified nets
      incr_groute_->updateRoutes(save_guides);
      for (const sta::Net* net : parasitics_invalid_) {
        debugPrint(logger_,
                   EST,
                   "estimate_parasitics",
                   1,
                   "net {} para is estimated for GR or DR",
                   sdc_network_->pathName(net));
        estimateGlobalRouteRC(db_network_->staToDb(net));
      }
      break;
    }
    case ParasiticsSrc::none:
      break;
  }

  // Router calls into the timer. This means the timer could be caching
  // delays calculated in the interim period before we had put new parasitic
  // annotations on the nets affected by a network edit. We need to explicitly
  // invalidate those delays. Do it in bulk instead of interleaving with each
  // groute call.
  if (parasitics_src_ != ParasiticsSrc::none) {
    for (const sta::Net* net : parasitics_invalid_) {
      debugPrint(logger_,
                 EST,
                 "estimate_parasitics",
                 1,
                 "net {} delays from fanin invalidated",
                 sdc_network_->pathName(net));
      sta_->delaysInvalidFromFanin(net);
    }
  }
  parasitics_invalid_.clear();
}

bool EstimateParasitics::parasiticsValid() const
{
  return parasitics_invalid_.empty();
}

void EstimateParasitics::ensureWireParasitic(const sta::Pin* drvr_pin)
{
  const sta::Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));

  if (net) {
    ensureWireParasitic(drvr_pin, net);
  }
}

void EstimateParasitics::ensureWireParasitic(const sta::Pin* drvr_pin,
                                             const sta::Net* net)
{
  // Sufficient to check for parasitic for one corner because
  // they are all made at the same time.
  const Scene* corner = sta_->scenes().front();
  Parasitics* parasitics = corner->parasitics(max_);
  if (parasitics_invalid_.contains(net)
      || parasitics->findPiElmore(drvr_pin, sta::RiseFall::rise(), max_)
             == nullptr) {
    switch (parasitics_src_) {
      case ParasiticsSrc::placement:
        estimateWireParasitic(drvr_pin, net);
        parasitics_invalid_.erase(net);
        break;
      case ParasiticsSrc::global_routing: {
        incr_groute_->updateRoutes();
        estimateGlobalRouteRC(db_network_->staToDb(net));
        parasitics_invalid_.erase(net);
        break;
      }
      case ParasiticsSrc::detailed_routing:
        // TODO: call incremental drt for the modified net
        break;
      case ParasiticsSrc::none:
        break;
    }
  }
}

void EstimateParasitics::estimateGlobalRouteRC(sta::SpefWriter* spef_writer)
{
  sta_->deleteParasitics();
  for (Scene* scene : scenes_) {
    Parasitics* parasitics = sta_->makeConcreteParasitics(scene->name(), "");
    scene->setParasitics(parasitics, sta::MinMaxAll::minMax());
  }

  MakeWireParasitics builder(
      logger_, this, sta_, db_->getTech(), block_, global_router_);

  for (auto& [db_net, route] : global_router_->getRoutes()) {
    if (!route.empty()) {
      builder.estimateParasitics(db_net, route, spef_writer);
    }
  }
}

void EstimateParasitics::estimateGlobalRouteRC(odb::dbNet* db_net)
{
  MakeWireParasitics builder(
      logger_, this, sta_, db_->getTech(), block_, global_router_);
  auto& routes = global_router_->getRoutes();
  auto iter = routes.find(db_net);
  if (iter == routes.end()) {
    return;
  }
  grt::GRoute& route = iter->second;
  if (!route.empty()) {
    builder.estimateParasitics(db_net, route, nullptr);
  }
}

void EstimateParasitics::estimateGlobalRouteParasitics(odb::dbNet* net,
                                                       grt::GRoute& route)
{
  initBlock();
  MakeWireParasitics builder(
      logger_, this, sta_, db_->getTech(), block_, global_router_);

  // Check if we are estimating parasitics after layer assignment
  if (route.at(0).is3DRoute()) {
    builder.estimateParasitics(net, route, nullptr);
  } else {
    builder.estimateParasitics(net, route);
  }
}

void EstimateParasitics::clearParasitics()
{
  MakeWireParasitics builder(
      logger_, this, sta_, db_->getTech(), block_, global_router_);
  builder.clearParasitics();
}

////////////////////////////////////////////////////////////////

void EstimateParasitics::estimateWireParasitics(sta::SpefWriter* spef_writer)
{
  initBlock();
  if (!wire_signal_cap_.empty()) {
    for (auto mode : sta_->modes()) {
      sta_->ensureClkNetwork(mode);
    }
    // Make separate parasitics for each corner
    sta_->deleteParasitics();
    for (Scene* scene : scenes_) {
      Parasitics* parasitics = sta_->makeConcreteParasitics(scene->name(), "");
      scene->setParasitics(parasitics, sta::MinMaxAll::minMax());
    }

    sta::LibertyLibrary* default_lib = network_->defaultLibertyLibrary();
    // Call clearNetDrvrPinMap only without full blown ConcreteNetwork::clear()
    // This is because netlist changes may invalidate cached net driver pin data
    network_->sta::Network::clear();
    network_->setDefaultLibertyLibrary(default_lib);

    // Hierarchy flow change
    // go through all nets, not just the ones in the instance
    // Get the net set from the block
    // old code:
    // NetIterator* net_iter = network_->netIterator(network_->topInstance());
    // Note that in hierarchy mode, this will not present all the nets,
    // which is intent here. So get all flat nets from block
    //

    sortClkAndSignalLayers();

    odb::dbSet<odb::dbNet> nets = block_->getNets();
    for (auto db_net : nets) {
      sta::Net* cur_net = db_network_->dbToSta(db_net);
      estimateWireParasitic(cur_net, spef_writer);
    }
    parasitics_src_ = ParasiticsSrc::placement;
    parasitics_invalid_.clear();
  }
}

void EstimateParasitics::estimateWireParasitic(const sta::Net* net,
                                               sta::SpefWriter* spef_writer)
{
  PinSet* drivers = network_->drivers(net);
  if (drivers && !drivers->empty()) {
    const Pin* drvr_pin = *drivers->begin();
    estimateWireParasitic(drvr_pin, net, spef_writer);
  }
}

void EstimateParasitics::estimateWireParasitic(const sta::Pin* drvr_pin,
                                               const sta::Net* net,
                                               sta::SpefWriter* spef_writer)
{
  if (!network_->isPower(net) && !network_->isGround(net)
      && !db_network_->staToDb(net)->isSpecial()) {
    if (isPadNet(net)) {
      // When an input port drives a pad instance with huge input
      // cap the elmore delay is gigantic. Annotate with zero
      // wire capacitance to prevent wireload model parasitics from being used.
      makePadParasitic(net, spef_writer);
    } else {
      estimateWireParasiticSteiner(drvr_pin, net, spef_writer);
    }
  }
}

void EstimateParasitics::makeWireParasitic(sta::Net* net,
                                           sta::Pin* drvr_pin,
                                           sta::Pin* load_pin,
                                           double wire_length,  // meters
                                           const Scene* corner)
{
  sta::Parasitics* parasitics = corner->parasitics(max_);
  sta::Parasitic* parasitic = parasitics->makeParasiticNetwork(net, false);
  sta::ParasiticNode* n1
      = parasitics->ensureParasiticNode(parasitic, drvr_pin, network_);
  sta::ParasiticNode* n2
      = parasitics->ensureParasiticNode(parasitic, load_pin, network_);
  double wire_cap = wire_length * wireSignalCapacitance(corner);
  double wire_res = wire_length * wireSignalResistance(corner);
  parasitics->incrCap(n1, wire_cap / 2.0);

  // Reduce resistance if the net has NDR with increased width
  odb::dbTechNonDefaultRule* ndr
      = db_network_->staToDb(net)->getNonDefaultRule();
  if (ndr) {
    std::vector<odb::dbTechLayerRule*> layer_rules;
    ndr->getLayerRules(layer_rules);
    float ndr_ratio = (float) layer_rules.at(0)->getWidth()
                      / layer_rules.at(0)->getLayer()->getWidth();
    wire_res /= ndr_ratio;
  }

  parasitics->makeResistor(parasitic, 1, wire_res, n1, n2);
  parasitics->incrCap(n2, wire_cap / 2.0);
}

bool EstimateParasitics::isPadNet(const sta::Net* net) const
{
  const sta::Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  return pin1 && pin2
         && ((network_->isTopLevelPort(pin1) && isPadPin(pin2))
             || (network_->isTopLevelPort(pin2) && isPadPin(pin1)));
}

void EstimateParasitics::makePadParasitic(const sta::Net* net,
                                          sta::SpefWriter* spef_writer)
{
  const sta::Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  for (sta::Scene* corner : sta_->scenes()) {
    sta::Parasitics* parasitics = corner->parasitics(max_);
    sta::Parasitic* parasitic = parasitics->makeParasiticNetwork(net, false);
    sta::ParasiticNode* n1
        = parasitics->ensureParasiticNode(parasitic, pin1, network_);
    sta::ParasiticNode* n2
        = parasitics->ensureParasiticNode(parasitic, pin2, network_);

    // Use a small resistor to keep the connectivity intact.
    parasitics->makeResistor(parasitic, 1, .001, n1, n2);
    if (spef_writer) {
      spef_writer->writeNet(corner, net, parasitic, parasitics);
    }
    arc_delay_calc_->reduceParasitic(
        parasitic, net, corner, sta::MinMaxAll::all());
    parasitics->deleteParasiticNetwork(net);
  }
}

void EstimateParasitics::estimateWireParasiticSteiner(
    const sta::Pin* drvr_pin,
    const sta::Net* net,
    sta::SpefWriter* spef_writer)
{
  bool all_modes_ideal_clock = true;
  for (sta::Mode* mode : sta_->modes()) {
    if (!sta_->isIdealClock(drvr_pin, mode)) {
      all_modes_ideal_clock = false;
      break;
    }
  }

  if (!all_modes_ideal_clock) {
    SteinerTree* tree = makeSteinerTree(drvr_pin);
    if (tree) {
      debugPrint(logger_,
                 EST,
                 "estimate_parasitics",
                 1,
                 "estimate wire {}",
                 sdc_network_->pathName(net));
      for (sta::Scene* corner : sta_->scenes()) {
        if (sta_->isIdealClock(drvr_pin, corner->mode())) {
          continue;
        }
        std::set<const Pin*> connected_pins;
        Parasitics* parasitics = corner->parasitics(max_);
        Parasitic* parasitic = parasitics->makeParasiticNetwork(net, false);
        bool is_clk = global_router_->isNonLeafClock(db_network_->staToDb(net));
        double wire_cap = 0.0;
        double wire_res = 0.0;
        const int branch_count = tree->branchCount();
        int max_node_index = tree->getMaxIndex();
        size_t resistor_id = 1;
        for (int i = 0; i < branch_count; i++) {
          odb::Point pt1, pt2;
          SteinerPt steiner_pt1, steiner_pt2;
          int wire_length_dbu;
          tree->branch(i, pt1, steiner_pt1, pt2, steiner_pt2, wire_length_dbu);
          if (wire_length_dbu) {
            double dx = dbuToMeters(abs(pt1.x() - pt2.x()))
                        / dbuToMeters(wire_length_dbu);
            double dy = dbuToMeters(abs(pt1.y() - pt2.y()))
                        / dbuToMeters(wire_length_dbu);

            if (is_clk) {
              wire_cap = dx * wireClkHCapacitance(corner)
                         + dy * wireClkVCapacitance(corner);
              wire_res = dx * wireClkHResistance(corner)
                         + dy * wireClkVResistance(corner);
            } else {
              wire_cap = dx * wireSignalHCapacitance(corner)
                         + dy * wireSignalVCapacitance(corner);
              wire_res = dx * wireSignalHResistance(corner)
                         + dy * wireSignalVResistance(corner);
            }
          } else {
            wire_cap = is_clk ? wireClkCapacitance(corner)
                              : wireSignalCapacitance(corner);
            wire_res = is_clk ? wireClkResistance(corner)
                              : wireSignalResistance(corner);
          }
          sta::ParasiticNode* n1 = parasitics->ensureParasiticNode(
              parasitic, net, steiner_pt1, network_);
          sta::ParasiticNode* n2 = parasitics->ensureParasiticNode(
              parasitic, net, steiner_pt2, network_);
          if (wire_length_dbu == 0) {
            // Use a small resistor to keep the connectivity intact.
            parasitics->makeResistor(parasitic, resistor_id++, 1.0e-3, n1, n2);
          } else {
            double length = dbuToMeters(wire_length_dbu);
            double cap = length * wire_cap;
            double res = length * wire_res;

            // Reduce resistance if the net has NDR with increased width
            odb::dbTechNonDefaultRule* ndr
                = db_network_->staToDb(net)->getNonDefaultRule();
            if (ndr) {
              std::vector<odb::dbTechLayerRule*> layer_rules;
              ndr->getLayerRules(layer_rules);
              float ratio = (float) layer_rules.at(0)->getWidth()
                            / layer_rules.at(0)->getLayer()->getWidth();
              res /= ratio;
            }

            // Make pi model for the wire.
            debugPrint(logger_,
                       EST,
                       "estimate_parasitics",
                       2,
                       " pi {} l={} c2={} rpi={} c1={} {}",
                       parasitics->name(n1),
                       units_->distanceUnit()->asString(length),
                       units_->capacitanceUnit()->asString(cap / 2.0),
                       units_->resistanceUnit()->asString(res),
                       units_->capacitanceUnit()->asString(cap / 2.0),
                       parasitics->name(n2));
            parasitics->incrCap(n1, cap / 2.0);
            parasitics->makeResistor(parasitic, resistor_id++, res, n1, n2);
            parasitics->incrCap(n2, cap / 2.0);
          }
          parasiticNodeConnectPins(parasitics,
                                   parasitic,
                                   n1,
                                   tree,
                                   steiner_pt1,
                                   resistor_id,
                                   corner,
                                   connected_pins,
                                   net,
                                   max_node_index,
                                   is_clk);
          parasiticNodeConnectPins(parasitics,
                                   parasitic,
                                   n2,
                                   tree,
                                   steiner_pt2,
                                   resistor_id,
                                   corner,
                                   connected_pins,
                                   net,
                                   max_node_index,
                                   is_clk);
        }
        if (spef_writer) {
          spef_writer->writeNet(corner, net, parasitic, parasitics);
        }
        arc_delay_calc_->reduceParasitic(
            parasitic, net, corner, sta::MinMaxAll::all());
        parasitics->deleteParasiticNetwork(net);
      }
      delete tree;
    }
  }
}

odb::dbTechLayer* EstimateParasitics::getPinLayer(const sta::Pin* pin)
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  db_network_->staToDb(pin, iterm, bterm, moditerm);

  odb::dbTechLayer* pin_layer = nullptr;
  odb::dbShape pin_shape;
  if (iterm) {
    int min_layer_idx = std::numeric_limits<int>::max();
    for (const auto& [layer, rect] : iterm->getGeometries()) {
      if (layer->getType() == odb::dbTechLayerType::ROUTING
          && layer->getRoutingLevel() < min_layer_idx) {
        min_layer_idx = layer->getRoutingLevel();
        pin_layer = layer;
      }
    }
  } else if (bterm && bterm->getFirstPin(pin_shape)) {
    pin_layer = pin_shape.getTechLayer();
  } else {
    logger_->error(EST,
                   164,
                   "sta::Pin {} has no placed iterm or bterm.",
                   network_->pathName(pin));
  }

  return pin_layer;
}

double EstimateParasitics::computeAverageCutResistance(sta::Scene* scene)
{
  if (layer_res_.empty()) {
    return 0.0;
  }

  double total_resistance = 0.0;
  int count = 0;

  int min_layer = block_->getMinRoutingLayer();
  int max_layer = block_->getMaxRoutingLayer();

  if (max_layer < 0) {
    max_layer = db_->getTech()->getRoutingLayerCount() / 2;
  }

  odb::dbTechLayer* min_tech_layer
      = db_->getTech()->findRoutingLayer(min_layer);
  odb::dbTechLayer* max_tech_layer
      = db_->getTech()->findRoutingLayer(max_layer);

  for (int layer_idx = min_tech_layer->getNumber();
       layer_idx <= max_tech_layer->getNumber();
       layer_idx++) {
    odb::dbTechLayer* layer = db_->getTech()->findLayer(layer_idx);
    if (layer && layer->getType() == odb::dbTechLayerType::CUT) {
      const float resistance = layer_res_[layer_idx][scene->index()];
      total_resistance += resistance;
      count++;
    }
  }

  return count > 0 ? total_resistance / count : 0.0;
}

void EstimateParasitics::parasiticNodeConnectPins(
    sta::Parasitics* parasitics,
    sta::Parasitic* parasitic,
    sta::ParasiticNode* node,
    SteinerTree* tree,
    SteinerPt pt,
    size_t& resistor_id,
    sta::Scene* corner,
    std::set<const Pin*>& connected_pins,
    const sta::Net* net,
    int& max_node_index,
    const bool is_clk)
{
  const sta::PinSeq* pins = tree->pins(pt);
  if (pins) {
    odb::dbTechLayer* tree_layer;
    if (is_clk) {
      tree_layer = clk_layers_.empty() ? nullptr : clk_layers_[0];
    } else {
      tree_layer = signal_layers_.empty() ? nullptr : signal_layers_[0];
    }

    for (const sta::Pin* pin : *pins) {
      sta::ParasiticNode* pin_node
          = parasitics->ensureParasiticNode(parasitic, pin, network_);
      if (connected_pins.find(pin) == connected_pins.end()) {
        if (tree_layer != nullptr && !layer_res_.empty()) {
          odb::dbTechLayer* pin_layer = getPinLayer(pin);

          insertViaResistances(pin_layer,
                               tree_layer,
                               parasitics,
                               parasitic,
                               pin_node,
                               node,
                               resistor_id,
                               corner,
                               net,
                               max_node_index);
        } else {
          double cut_res
              = std::max(computeAverageCutResistance(corner), 1.0e-3);
          parasitics->makeResistor(
              parasitic, resistor_id++, cut_res, node, pin_node);
        }
        connected_pins.insert(pin);
      }
    }
  }
}

void EstimateParasitics::insertViaResistances(odb::dbTechLayer* pin_layer,
                                              odb::dbTechLayer* tree_layer,
                                              sta::Parasitics* parasitics,
                                              sta::Parasitic* parasitic,
                                              sta::ParasiticNode* pin_node,
                                              sta::ParasiticNode* node,
                                              size_t& resistor_id,
                                              sta::Scene* corner,
                                              const sta::Net* net,
                                              int& max_node_index)
{
  sta::ParasiticNode* prev_node = nullptr;

  const int pin_layer_idx = pin_layer->getNumber();
  const int tree_layer_idx = tree_layer->getNumber();
  if (std::abs(pin_layer_idx - tree_layer->getNumber()) == 2) {
    // Directly connect pin node and tree node if they are one cut layer apart
    const int cut_layer_idx = pin_layer_idx < tree_layer_idx
                                  ? pin_layer_idx + 1
                                  : pin_layer_idx - 1;
    const double cut_res
        = std::max(layer_res_[cut_layer_idx][corner->index()], 1.0e-3);
    parasitics->makeResistor(parasitic, resistor_id++, cut_res, pin_node, node);
  } else if (pin_layer_idx == tree_layer_idx) {
    // Add a small resistor between the pin node and tree node to keep
    // connectivity
    parasitics->makeResistor(parasitic, resistor_id++, 1.0e-3, pin_node, node);
  } else {
    const auto [start_idx, end_idx]
        = std::minmax(pin_layer_idx, tree_layer_idx);
    const bool pin_is_below = (pin_layer_idx < tree_layer_idx);

    for (int layer_idx = start_idx; layer_idx < end_idx; layer_idx++) {
      odb::dbTechLayer* cut_layer = db_->getTech()->findLayer(layer_idx);
      if (cut_layer->getType() != odb::dbTechLayerType::CUT) {
        continue;
      }
      sta::ParasiticNode* mid_node = parasitics->ensureParasiticNode(
          parasitic, net, ++max_node_index, network_);

      const double cut_res
          = std::max(layer_res_[layer_idx][corner->index()], 1.0e-3);

      sta::ParasiticNode* from_node = prev_node;
      sta::ParasiticNode* to_node = mid_node;
      if (pin_is_below) {
        if (layer_idx - 1 == pin_layer_idx) {
          from_node = pin_node;
        } else if (layer_idx + 1 == tree_layer_idx) {
          to_node = node;
        }
      } else {
        if (layer_idx - 1 == tree_layer_idx) {
          from_node = node;
        } else if (layer_idx + 1 == pin_layer_idx) {
          to_node = pin_node;
        }
      }

      parasitics->makeResistor(
          parasitic, resistor_id++, cut_res, from_node, to_node);

      prev_node = mid_node;
    }
  }
}

void EstimateParasitics::net2Pins(const sta::Net* net,
                                  const sta::Pin*& pin1,
                                  const sta::Pin*& pin2) const
{
  pin1 = nullptr;
  pin2 = nullptr;

  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  if (pin_iter->hasNext()) {
    pin1 = pin_iter->next();
  }
  if (pin_iter->hasNext()) {
    pin2 = pin_iter->next();
  }
  delete pin_iter;
}

bool EstimateParasitics::isPadPin(const sta::Pin* pin) const
{
  sta::Instance* inst = network_->instance(pin);
  return inst && !network_->isTopInstance(inst) && isPad(inst);
}

bool EstimateParasitics::isPad(const sta::Instance* inst) const
{
  dbInst* db_inst;
  dbModInst* mod_inst;
  db_network_->staToDb(inst, db_inst, mod_inst);
  if (mod_inst) {
    return false;
  }
  const auto type = db_inst->getMaster()->getType().getValue();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
    case dbMasterType::COVER:
    case dbMasterType::RING:
      return false;
    case dbMasterType::COVER_BUMP:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return true;
  }
  // gcc warniing
  return false;
}

void EstimateParasitics::parasiticsInvalid(const sta::Net* net)
{
  odb::dbNet* db_net = db_network_->findFlatDbNet(net);
  if (haveEstimatedParasitics() && db_net) {
    debugPrint(logger_,
               EST,
               "estimate_parasitics",
               2,
               "parasitics invalid {}",
               network_->pathName(net));
    parasitics_invalid_.insert(db_network_->dbToSta(db_net));
  }
}

void EstimateParasitics::parasiticsInvalid(const odb::dbNet* net)
{
  parasiticsInvalid(db_network_->dbToSta(net));
}

void EstimateParasitics::setParasiticsSrc(ParasiticsSrc src)
{
  if (incremental_parasitics_enabled_) {
    logger_->error(EST,
                   108,
                   "cannot change parasitics source while incremental "
                   "parasitics enabled");
  }

  parasitics_src_ = src;
}

void EstimateParasitics::eraseParasitics(const sta::Net* net)
{
  parasitics_invalid_.erase(net);
}

static void connectedPins(const sta::Net* net,
                          sta::Network* network,
                          sta::dbNetwork* db_network,
                          // Return value.
                          std::vector<PinLoc>& pins);

static void connectedPins(const sta::Net* net,
                          sta::Network* network,
                          sta::dbNetwork* db_network,
                          // Return value.
                          std::vector<PinLoc>& pins)
{
  NetConnectedPinIterator* pin_iter = network->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;
    db_network->staToDb(pin, iterm, bterm, moditerm);
    //
    // only accumuate the flat pins (in hierarchical mode we may
    // hit moditerms/modbterms).
    //
    if (iterm || bterm) {
      odb::Point loc = db_network->location(pin);
      pins.push_back({pin, loc});
    }
  }
  delete pin_iter;
}

SteinerTree* EstimateParasitics::makeSteinerTree(
    odb::Point drvr_location,
    const std::vector<odb::Point>& sink_locations)
{
  SteinerTree* tree = new SteinerTree(drvr_location, logger_);
  std::vector<PinLoc>& pinlocs = tree->pinlocs();
  for (auto loc : sink_locations) {
    pinlocs.push_back(PinLoc{nullptr, loc});
  }
  // Sort pins by location
  sort(pinlocs.begin(),
       pinlocs.end(),
       [=](const PinLoc& pin1, const PinLoc& pin2) {
         return pin1.loc.getX() < pin2.loc.getX()
                || (pin1.loc.getX() == pin2.loc.getX()
                    && pin1.loc.getY() < pin2.loc.getY());
       });
  int pin_count = pinlocs.size();
  if (pin_count >= 1) {
    // Two separate vectors of coordinates needed by flute.
    std::vector<int> x, y;
    int drvr_idx = pinlocs.size();
    pinlocs.push_back(PinLoc{nullptr, drvr_location});
    for (int i = 0; i < pin_count + 1; i++) {
      const PinLoc& pinloc = pinlocs[i];
      x.push_back(pinloc.loc.x());
      y.push_back(pinloc.loc.y());
    }
    stt::Tree ftree = stt_builder_->makeSteinerTree(x, y, drvr_idx);
    tree->setTree(ftree);
    tree->populateSides();
    return tree;
  }
  delete tree;
  return nullptr;
}

// Returns nullptr if net has less than 2 pins or any pin is not placed.
SteinerTree* EstimateParasitics::makeSteinerTree(const sta::Pin* drvr_pin)
{
  sta::Network* sdc_network = network_->sdcNetwork();

  /*
    Handle hierarchy. Make sure all traversal on dbNets.
   */
  odb::dbNet* db_net = db_network_->findFlatDbNet(drvr_pin);
  sta::Net* net = db_network_->dbToSta(db_net);

  debugPrint(
      logger_, EST, "steiner", 1, "sta::Net {}", sdc_network->pathName(net));
  SteinerTree* tree = new SteinerTree(drvr_pin, db_network_, logger_);
  std::vector<PinLoc>& pinlocs = tree->pinlocs();
  // Find all the connected pins
  connectedPins(net, network_, db_network_, pinlocs);
  // Sort pins by location because connectedPins order is not deterministic.
  sort(pinlocs.begin(),
       pinlocs.end(),
       [=](const PinLoc& pin1, const PinLoc& pin2) {
         return pin1.loc.getX() < pin2.loc.getX()
                || (pin1.loc.getX() == pin2.loc.getX()
                    && pin1.loc.getY() < pin2.loc.getY());
       });
  int pin_count = pinlocs.size();
  bool is_placed = true;
  if (pin_count >= 2) {
    std::vector<int> x;  // Two separate vectors of coordinates needed by flute.
    std::vector<int> y;
    int drvr_idx = 0;  // The "driver_pin" or the root of the Steiner tree.
    for (int i = 0; i < pin_count; i++) {
      const PinLoc& pinloc = pinlocs[i];
      if (pinloc.pin == drvr_pin) {
        drvr_idx = i;  // drvr_index is needed by flute.
      }
      x.push_back(pinloc.loc.x());
      y.push_back(pinloc.loc.y());
      debugPrint(logger_,
                 EST,
                 "steiner",
                 3,
                 " {} ({} {})",
                 sdc_network->pathName(pinloc.pin),
                 pinloc.loc.x(),
                 pinloc.loc.y());
      // Track that all our pins are placed.
      is_placed &= db_network_->isPlaced(pinloc.pin);

      // Flute may reorder the input points, so it takes some unravelling
      // to find the mapping back to the original pins. The complication is
      // that multiple pins can occupy the same location.
      tree->locAddPin(pinloc.loc, pinloc.pin);
    }
    if (is_placed) {
      stt::Tree ftree = stt_builder_->makeSteinerTree(db_net, x, y, drvr_idx);

      tree->setTree(ftree);
      tree->createSteinerPtToPinMap();
      return tree;
    }
  }
  delete tree;
  return nullptr;
}

double EstimateParasitics::dbuToMeters(int dist) const
{
  return dist / (dbu_ * 1e+6);
}

void EstimateParasitics::highlightSteiner(const sta::Pin* drvr)
{
  if (steiner_renderer_) {
    est::SteinerTree* tree = nullptr;
    if (drvr) {
      tree = makeSteinerTree(drvr);
    }
    steiner_renderer_->highlight(tree);
  }
}

////////////////////////////////////////////////////////////////

IncrementalParasiticsGuard::IncrementalParasiticsGuard(
    est::EstimateParasitics* estimate_parasitics)
    : estimate_parasitics_(estimate_parasitics), need_unregister_(false)
{
  // check to allow reentrancy
  if (!estimate_parasitics_->isIncrementalParasiticsEnabled()) {
    if (!estimate_parasitics_->getBlock()) {
      estimate_parasitics_->initBlock();
    }

    if (estimate_parasitics_->hasParasiticsInvalid()) {
      estimate_parasitics_->getLogger()->error(
          EST, 104, "inconsistent parasitics state");
    }

    switch (estimate_parasitics_->getParasiticsSrc()) {
      case ParasiticsSrc::placement:
        break;
      case ParasiticsSrc::global_routing:
      case ParasiticsSrc::detailed_routing:
        // TODO: add IncrementalDRoute
        estimate_parasitics_->setIncrementalGRT(
            new grt::IncrementalGRoute(estimate_parasitics_->getGlobalRouter(),
                                       estimate_parasitics_->getBlock()));
        // Don't print verbose messages for incremental routing
        estimate_parasitics_->getGlobalRouter()->setVerbose(false);
        break;
      case ParasiticsSrc::none:
        break;
    }

    estimate_parasitics_->setIncrementalParasiticsEnabled(true);
    estimate_parasitics_->setDbCbkOwner(estimate_parasitics_->getBlock());
    need_unregister_ = true;
  }
}

void IncrementalParasiticsGuard::update()
{
  estimate_parasitics_->updateParasitics();
}

IncrementalParasiticsGuard::~IncrementalParasiticsGuard()
{
  if (need_unregister_) {
    estimate_parasitics_->removeDbCbkOwner();
    estimate_parasitics_->updateParasitics();

    switch (estimate_parasitics_->getParasiticsSrc()) {
      case ParasiticsSrc::placement:
        break;
      case ParasiticsSrc::global_routing:
      case ParasiticsSrc::detailed_routing:
        // TODO: add IncrementalDRoute
        delete estimate_parasitics_->getIncrementalGRT();
        estimate_parasitics_->setIncrementalGRT(nullptr);
        break;
      case ParasiticsSrc::none:
        break;
    }

    estimate_parasitics_->setIncrementalParasiticsEnabled(false);
  }
}

}  // namespace est
