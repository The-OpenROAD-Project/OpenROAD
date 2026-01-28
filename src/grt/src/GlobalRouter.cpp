// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/GlobalRouter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AbstractFastRouteRenderer.h"
#include "AbstractGrouteRenderer.h"
#include "AbstractRoutingCongestionDataSource.h"
#include "CUGR.h"
#include "DataType.h"
#include "FastRoute.h"
#include "Grid.h"
#include "Net.h"
#include "Pin.h"
#include "RepairAntennas.h"
#include "RoutingTracks.h"
#include "boost/icl/interval.hpp"
#include "boost/polygon/polygon.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GRoute.h"
#include "grt/PinGridLocation.h"
#include "grt/Rudy.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/wOrder.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Parasitics.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace grt {

using boost::icl::interval;
using utl::GRT;

GlobalRouter::GlobalRouter(utl::Logger* logger,
                           utl::CallBackHandler* callback_handler,
                           stt::SteinerTreeBuilder* stt_builder,
                           odb::dbDatabase* db,
                           sta::dbSta* sta,
                           ant::AntennaChecker* antenna_checker,
                           dpl::Opendp* opendp)
    : logger_(logger),
      callback_handler_(callback_handler),
      stt_builder_(stt_builder),
      antenna_checker_(antenna_checker),
      opendp_(opendp),
      fastroute_(nullptr),
      cugr_(nullptr),
      grid_origin_(0, 0),
      groute_renderer_(nullptr),
      grid_(new Grid),
      is_incremental_(false),
      adjustment_(0.0),
      congestion_report_iter_step_(0),
      allow_congestion_(false),
      macro_extension_(0),
      initialized_(false),
      total_diodes_count_(0),
      verbose_(false),
      seed_(0),
      caps_perturbation_percentage_(0),
      perturbation_amount_(1),
      sta_(sta),
      db_(db),
      block_(nullptr),
      repair_antennas_(nullptr),
      rudy_(nullptr),
      heatmap_(nullptr),
      heatmap_rudy_(nullptr),
      congestion_file_name_(nullptr),
      grouter_cbk_(nullptr)
{
  fastroute_
      = new FastRouteCore(db_, logger_, callback_handler_, stt_builder_, sta_);
  cugr_ = new CUGR(db_, logger_, callback_handler_, stt_builder_, sta_);
}

void GlobalRouter::initGui(std::unique_ptr<AbstractRoutingCongestionDataSource>
                               routing_congestion_data_source,
                           std::unique_ptr<AbstractRoutingCongestionDataSource>
                               routing_congestion_data_source_rudy)
{
  heatmap_ = std::move(routing_congestion_data_source);
  heatmap_->registerHeatMap();
  heatmap_rudy_ = std::move(routing_congestion_data_source_rudy);
  heatmap_rudy_->registerHeatMap();
}

void GlobalRouter::clear()
{
  routes_.clear();
  for (auto [ignored, net] : db_net_map_) {
    delete net;
  }
  db_net_map_.clear();
  routing_tracks_.clear();
  routing_layers_.clear();
  grid_->clear();
  fastroute_->clear();
  vertical_capacities_.clear();
  horizontal_capacities_.clear();
  initialized_ = false;
}

GlobalRouter::~GlobalRouter()
{
  delete fastroute_;
  delete cugr_;
  delete grid_;
  for (auto [ignored, net] : db_net_map_) {
    delete net;
  }
  delete repair_antennas_;
  delete rudy_;
}

std::vector<Net*> GlobalRouter::initFastRoute(int min_routing_layer,
                                              int max_routing_layer)
{
  fastroute_->clear();
  h_nets_in_pos_.clear();
  v_nets_in_pos_.clear();

  configFastRoute();

  initRoutingLayers(min_routing_layer, max_routing_layer);
  reportLayerSettings(min_routing_layer, max_routing_layer);
  initRoutingTracks(max_routing_layer);
  initCoreGrid(max_routing_layer);
  setCapacities(min_routing_layer, max_routing_layer);

  applyAdjustments(min_routing_layer, max_routing_layer);
  perturbCapacities();

  // Init the data structures to monitor 3D capacity during 2D phases
  fastroute_->initEdgesCapacityPerLayer();

  std::vector<Net*> nets = findNets(true);
  checkPinPlacement();
  initNetlist(nets);

  initialized_ = true;
  return nets;
}

void GlobalRouter::applyAdjustments(int min_routing_layer,
                                    int max_routing_layer)
{
  computeObstructionsAdjustments();
  std::vector<int> track_space = grid_->getTrackPitches();
  fastroute_->initBlockedIntervals(track_space);
  // Save global resources before add adjustment by layer
  fastroute_->saveResourcesBeforeAdjustments();
  computeUserGlobalAdjustments(min_routing_layer, max_routing_layer);
  computeUserLayerAdjustments(min_routing_layer, max_routing_layer);

  for (RegionAdjustment region_adjustment : region_adjustments_) {
    computeRegionAdjustments(region_adjustment.getRegion(),
                             region_adjustment.getLayer(),
                             region_adjustment.getAdjustment());
  }
}

// If file name is specified, save congestion report file.
// If there are no congestions, the empty file overwrites any
// previous congestion report file.
void GlobalRouter::saveCongestion()
{
  is_congested_ = fastroute_->totalOverflow() > 0;
  fastroute_->saveCongestion();
}

NetRouteMap& GlobalRouter::getRoutes()
{
  partial_routes_.clear();
  if (routes_.empty()) {
    partial_routes_ = fastroute_->getPlanarRoutes();
    return partial_routes_;
  }
  return routes_;
}

NetRouteMap GlobalRouter::getPartialRoutes()
{
  NetRouteMap net_routes;
  // TODO: still need to fix this during incremental grt
  if (is_incremental_) {
    for (const auto& [db_net, net] : db_net_map_) {
      // Do not add local nets, as they are not routed in incremental grt.
      if (routes_[db_net].empty() && !net->isLocal()) {
        GRoute route;
        net_routes.insert({db_net, route});
        fastroute_->getPlanarRoute(db_net, net_routes[db_net]);
      }
    }
  } else {
    partial_routes_.clear();
    if (routes_.empty()) {
      if (!use_cugr_) {
        partial_routes_ = fastroute_->getPlanarRoutes();
      } else {
        partial_routes_ = cugr_->getRoutes();
        updatePinAccessPoints();
      }
      net_routes = partial_routes_;
    }
  }

  return net_routes;
}

bool GlobalRouter::haveRoutes()
{
  if (!designIsPlaced()) {
    return false;
  }
  loadGuidesFromDB();
  bool congested_routes = is_congested_ && !allow_congestion_;
  return !routes_.empty() && !congested_routes;
}

bool GlobalRouter::haveDetailedRoutes()
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  for (odb::dbNet* db_net : block_->getNets()) {
    if (isDetailedRouted(db_net)) {
      return true;
    }
  }
  return false;
}

bool GlobalRouter::haveDbGuides()
{
  for (odb::dbNet* net : block_->getNets()) {
    // check term count due to 1-pin nets in multiple designs.
    if (!net->isSpecial() && net->getGuides().empty() && net->getTermCount() > 1
        && !net->isConnectedByAbutment()) {
      return false;
    }
  }
  return true;
}

bool GlobalRouter::designIsPlaced()
{
  if (db_->getChip() == nullptr) {
    logger_->error(
        GRT, 270, "Load a design before running the global router commands.");
  }
  block_ = db_->getChip()->getBlock();

  for (Pin* port : getAllPorts()) {
    if (port->getBTerm()->getFirstPinPlacementStatus()
        == odb::dbPlacementStatus::NONE) {
      return false;
    }
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->isSpecial()) {
      continue;
    }
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (!inst->isPlaced()) {
        return false;
      }
    }
  }

  return true;
}

bool GlobalRouter::haveDetailedRoutes(const std::vector<odb::dbNet*>& db_nets)
{
  for (odb::dbNet* db_net : db_nets) {
    if (isDetailedRouted(db_net)) {
      return true;
    }
  }
  return false;
}

void GlobalRouter::startIncremental()
{
  is_incremental_ = true;
  if (!initialized_ || haveDetailedRoutes()) {
    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);
    initFastRoute(min_layer, max_layer);
  }
  grouter_cbk_ = new GRouteDbCbk(this);
  grouter_cbk_->addOwner(block_);
}

void GlobalRouter::endIncremental(bool save_guides)
{
  is_incremental_ = true;
  fastroute_->setResistanceAware(resistance_aware_);
  updateDirtyRoutes();
  grouter_cbk_->removeOwner();
  delete grouter_cbk_;
  grouter_cbk_ = nullptr;
  finishGlobalRouting(save_guides);
}

void GlobalRouter::globalRoute(bool save_guides)
{
  auto start = std::chrono::steady_clock::now();
  bool has_routable_nets = false;

  for (auto net : db_->getChip()->getBlock()->getNets()) {
    if (net->getITerms().size() + net->getBTerms().size() > 1) {
      has_routable_nets = true;
      break;
    }
  }
  if (!has_routable_nets) {
    logger_->warn(GRT,
                  7,
                  "Design does not have any routable net "
                  "(with at least 2 terms)");
    return;
  }

  try {
    clear();
    block_ = db_->getChip()->getBlock();

    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);

    std::vector<Net*> nets = initFastRoute(min_layer, max_layer);
    if (use_cugr_) {
      std::set<odb::dbNet*> clock_nets;
      findClockNets(nets, clock_nets);
      cugr_->init(min_layer, max_layer, clock_nets);
      cugr_->route();
      routes_ = cugr_->getRoutes();
      updatePinAccessPoints();
      addRemainingGuides(routes_, nets, min_layer, max_layer);
    } else {
      if (verbose_) {
        reportResources();
      }

      routes_ = findRouting(nets, min_layer, max_layer);
    }
  } catch (...) {
    updateDbCongestion();
    saveCongestion();
    throw;
  }

  finishGlobalRouting(save_guides);
  auto end = std::chrono::steady_clock::now();
  if (verbose_) {
    auto runtime
        = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    int hour = runtime.count() / 3600;
    int min = (runtime.count() % 3600) / 60;
    int sec = runtime.count() % 60;
    logger_->info(
        GRT, 303, "Global routing runtime = {:02}:{:02}:{:02}", hour, min, sec);
  }
}

void GlobalRouter::finishGlobalRouting(bool save_guides)
{
  updateDbCongestion();
  saveCongestion();

  if (verbose_ && !use_cugr_) {
    reportCongestion();
  }
  computeWirelength();
  if (verbose_) {
    logger_->info(GRT, 14, "Routed nets: {}", routes_.size());
  }
  if (save_guides) {
    std::vector<odb::dbNet*> nets;
    nets.reserve(block_->getNets().size());
    for (odb::dbNet* db_net : block_->getNets()) {
      nets.push_back(db_net);
    }
    saveGuides(nets);
  }

  if (is_congested_) {
    // Suggest adjustment value
    suggestAdjustment();
    if (allow_congestion_) {
      logger_->warn(GRT,
                    115,
                    "Global routing finished with congestion. Check the "
                    "congestion regions in the DRC Viewer.");
    } else {
      logger_->error(GRT,
                     116,
                     "Global routing finished with congestion. Check the "
                     "congestion regions in the DRC Viewer.");
    }
  }
}

void GlobalRouter::suggestAdjustment()
{
  // Get min adjustment apply to layers
  int min_routing_layer, max_routing_layer;
  getMinMaxLayer(min_routing_layer, max_routing_layer);
  float min_adjustment = 1.0;
  for (int l = min_routing_layer; l <= max_routing_layer; l++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(l);
    if (tech_layer->getLayerAdjustment() != 0.0) {
      min_adjustment
          = std::min(min_adjustment, tech_layer->getLayerAdjustment());
    }
  }
  min_adjustment *= 100;
  // Suggest new adjustment value
  int suggest_adjustment;
  bool has_sug_adj = fastroute_->computeSuggestedAdjustment(suggest_adjustment);
  if (has_sug_adj && min_adjustment > suggest_adjustment) {
    logger_->warn(GRT,
                  704,
                  "Try reduce the layer adjustment from {}% to {}%",
                  min_adjustment,
                  suggest_adjustment);
  }
}

void GlobalRouter::updateDbCongestion()
{
  int min_layer, max_layer;
  getMinMaxLayer(min_layer, max_layer);
  if (use_cugr_) {
    cugr_->updateDbCongestion();
  } else {
    fastroute_->updateDbCongestion(min_layer, max_layer);
  }
  heatmap_->update();
}

int GlobalRouter::repairAntennas(odb::dbMTerm* diode_mterm,
                                 int iterations,
                                 float ratio_margin,
                                 bool jumper_only,
                                 bool diode_only,
                                 const int num_threads)
{
  if (!initialized_ || haveDetailedRoutes()) {
    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);
    initFastRoute(min_layer, max_layer);
  }
  if (repair_antennas_ == nullptr) {
    repair_antennas_
        = new RepairAntennas(this, antenna_checker_, opendp_, db_, logger_);
  }

  if (diode_mterm == nullptr) {
    diode_mterm = repair_antennas_->findDiodeMTerm();
    if (diode_mterm == nullptr) {
      logger_->metric("antenna_diodes_count", total_diodes_count_);
      logger_->warn(
          GRT, 246, "No diode with LEF class CORE ANTENNACELL found.");
      return 0;
    }
  }
  if (repair_antennas_->diffArea(diode_mterm) == 0.0) {
    logger_->error(GRT,
                   244,
                   "Diode {}/{} ANTENNADIFFAREA is zero.",
                   diode_mterm->getMaster()->getConstName(),
                   diode_mterm->getConstName());
  }

  bool violations = true;
  int itr = 0;
  std::vector<odb::dbNet*> nets_to_repair;
  for (odb::dbNet* db_net : block_->getNets()) {
    nets_to_repair.push_back(db_net);
  }

  if (haveDetailedRoutes(nets_to_repair) && iterations != 1) {
    logger_->warn(GRT,
                  121,
                  "repair_antennas should perform only one iteration when the "
                  "routing source is detailed routing.");
  }

  while (violations && itr < iterations) {
    if (verbose_) {
      logger_->info(GRT, 6, "Repairing antennas, iteration {}.", itr + 1);
    }
    violations = repair_antennas_->checkAntennaViolations(routes_,
                                                          nets_to_repair,
                                                          getMaxRoutingLayer(),
                                                          diode_mterm,
                                                          ratio_margin,
                                                          num_threads);
    // if run in GRT and it need run jumper insertion
    std::vector<odb::dbNet*> nets_with_jumpers;
    if (!haveDetailedRoutes(nets_to_repair)
        && repair_antennas_->hasNewViolations() && !diode_only) {
      // Run jumper insertion and clean
      repair_antennas_->jumperInsertion(routes_,
                                        grid_->getTileSize(),
                                        getMaxRoutingLayer(),
                                        nets_with_jumpers);
      repair_antennas_->clearViolations();

      saveGuides(nets_with_jumpers);
      // run again antenna checker
      violations
          = repair_antennas_->checkAntennaViolations(routes_,
                                                     nets_to_repair,
                                                     getMaxRoutingLayer(),
                                                     diode_mterm,
                                                     ratio_margin,
                                                     num_threads);
      updateDbCongestion();
    }
    if (violations && !jumper_only) {
      IncrementalGRoute incr_groute(this, block_);
      repair_antennas_->repairAntennas(diode_mterm);
      total_diodes_count_ += repair_antennas_->getDiodesCount();
      logger_->info(
          GRT, 15, "Inserted {} diodes.", repair_antennas_->getDiodesCount());
      nets_to_repair.clear();
      // store all dirty nets for repair to ensure violations are fixed, even in
      // nets whose route guides were not modified but may have changes in
      // instance positions
      for (odb::dbNet* db_net : dirty_nets_) {
        nets_to_repair.push_back(db_net);
      }
      incr_groute.updateRoutes();
      saveGuides(nets_to_repair);
    }
    repair_antennas_->clearViolations();
    itr++;
  }

  logger_->metric("antenna_diodes_count", total_diodes_count_);
  return total_diodes_count_;
}

NetRouteMap GlobalRouter::findRouting(std::vector<Net*>& nets,
                                      int min_routing_layer,
                                      int max_routing_layer)
{
  NetRouteMap routes;
  if (!nets.empty()) {
    routes = fastroute_->run();
    addRemainingGuides(routes, nets, min_routing_layer, max_routing_layer);
    connectPadPins(routes);
    for (auto& net_route : routes) {
      std::vector<Pin>& pins = db_net_map_[net_route.first]->getPins();
      GRoute& route = net_route.second;
      mergeSegments(pins, route);
    }
  }

  return routes;
}

std::vector<int> GlobalRouter::routeLayerLengths(odb::dbNet* db_net)
{
  odb::dbTech* tech = db_->getTech();
  loadGuidesFromDB();
  NetRouteMap& routes = routes_;

  // dbu wirelength for wires, via count for vias
  std::vector<int> layer_lengths(tech->getLayerCount());

  if (!db_net->getSigType().isSupply() && !db_net->isSpecial()
      && db_net->getSWires().empty() && !db_net->isConnectedByAbutment()) {
    GRoute& route = routes[db_net];
    std::set<RoutePt> route_pts;
    // Compute wirelengths from route segments
    for (GSegment& segment : route) {
      if (segment.isVia()) {
        auto& s = segment;
        // Mimic makeRouteParasitics
        int min_layer = std::min(s.init_layer, s.final_layer);
        odb::dbTechLayer* cut_layer
            = tech->findRoutingLayer(min_layer)->getUpperLayer();
        layer_lengths[cut_layer->getNumber()] += 1;
        route_pts.insert(RoutePt(s.init_x, s.init_y, s.init_layer));
        route_pts.insert(RoutePt(s.final_x, s.final_y, s.final_layer));
      } else {
        int layer = segment.init_layer;
        layer_lengths[tech->findRoutingLayer(layer)->getNumber()]
            += segment.length();
        route_pts.insert(RoutePt(segment.init_x, segment.init_y, layer));
        route_pts.insert(RoutePt(segment.final_x, segment.final_y, layer));
      }
    }

    Net* net = getNet(db_net);
    // Compute wirelength from pin position on grid to real pin location
    for (Pin& pin : net->getPins()) {
      int layer = pin.getConnectionLayer() + 1;
      odb::Point grid_pt = pin.getOnGridPosition();
      odb::Point pt = pin.getPosition();

      RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
      auto pt_itr = route_pts.find(grid_route);
      if (pt_itr == route_pts.end()) {
        layer--;
      }
      int wire_length_dbu
          = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
      layer_lengths[tech->findRoutingLayer(layer)->getNumber()]
          += wire_length_dbu;
    }
  }
  return layer_lengths;
}

////////////////////////////////////////////////////////////////

void GlobalRouter::initCoreGrid(int max_routing_layer)
{
  initGrid(max_routing_layer);
  findTrackPitches(max_routing_layer);

  fastroute_->setLowerLeft(grid_->getXMin(), grid_->getYMin());
  fastroute_->setTileSize(grid_->getTileSize());
  fastroute_->setGridsAndLayers(
      grid_->getXGrids(), grid_->getYGrids(), grid_->getNumLayers());
  fastroute_->setGridMax(grid_->getGridArea().xMax(),
                         grid_->getGridArea().yMax());

  odb::dbTech* tech = db_->getTech();
  for (int l = 1; l <= max_routing_layer; l++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l);
    fastroute_->addLayerDirection(l - 1, tech_layer->getDirection());
  }
}

void GlobalRouter::initRoutingLayers(int min_routing_layer,
                                     int max_routing_layer)
{
  odb::dbTech* tech = db_->getTech();

  int valid_layers = 1;
  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l);
    if (tech_layer == nullptr) {
      logger_->error(GRT, 85, "Routing layer {} not found.", l);
    }
    if (tech_layer->getRoutingLevel() != 0) {
      if (tech_layer->getDirection() != odb::dbTechLayerDir::HORIZONTAL
          && tech_layer->getDirection() != odb::dbTechLayerDir::VERTICAL) {
        logger_->error(GRT,
                       84,
                       "Layer {} does not have a valid direction.",
                       tech_layer->getName());
      }
      odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
      if (track_grid == nullptr
          && tech_layer->getRoutingLevel() >= min_routing_layer
          && tech_layer->getRoutingLevel() <= max_routing_layer) {
        logger_->error(GRT,
                       70,
                       "Layer {} does not have track grid.",
                       tech_layer->getName());
      }
      routing_layers_[valid_layers] = tech_layer;
      valid_layers++;
    }
  }
  checkAdjacentLayersDirection(min_routing_layer, max_routing_layer);
}

void GlobalRouter::checkAdjacentLayersDirection(int min_routing_layer,
                                                int max_routing_layer)
{
  odb::dbTech* tech = db_->getTech();
  for (int l = min_routing_layer; l < max_routing_layer; l++) {
    odb::dbTechLayer* layer_a = tech->findRoutingLayer(l);
    odb::dbTechLayer* layer_b = tech->findRoutingLayer(l + 1);
    if (layer_a->getDirection() == layer_b->getDirection()) {
      logger_->error(
          GRT,
          126,
          "Layers {} and {} have the same preferred routing direction ({}).",
          layer_a->getName(),
          layer_b->getName(),
          layer_a->getDirection().getString());
    }
  }
}

void GlobalRouter::setCapacities(int min_routing_layer, int max_routing_layer)
{
  fastroute_->initEdges();
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  for (int layer = 1; layer <= grid_->getNumLayers(); layer++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(layer);
    const bool inside_layer_range
        = (layer >= min_routing_layer && layer <= max_routing_layer);

    const RoutingTracks& tracks = getRoutingTracksByIndex(layer);
    const int track_init = tracks.getLocation();
    const int track_pitch = tracks.getTrackPitch();
    const int track_count = tracks.getNumTracks();
    const bool horizontal
        = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      int min_cap = std::numeric_limits<int>::max();
      for (int y = 1; y <= y_grids; y++) {
        for (int x = 1; x < x_grids; x++) {
          const int cap = inside_layer_range ? computeGCellCapacity(x - 1,
                                                                    y - 1,
                                                                    track_init,
                                                                    track_pitch,
                                                                    track_count,
                                                                    horizontal)
                                             : 0;
          min_cap = std::min(min_cap, cap);
          fastroute_->setEdgeCapacity(x - 1, y - 1, x, y - 1, layer, cap);
        }
      }
      min_cap = min_cap == std::numeric_limits<int>::max() ? 0 : min_cap;
      fastroute_->addHCapacity(min_cap, layer);
    } else {
      int min_cap = std::numeric_limits<int>::max();
      for (int x = 1; x <= x_grids; x++) {
        for (int y = 1; y < y_grids; y++) {
          const int cap = inside_layer_range ? computeGCellCapacity(x - 1,
                                                                    y - 1,
                                                                    track_init,
                                                                    track_pitch,
                                                                    track_count,
                                                                    horizontal)
                                             : 0;
          min_cap = std::min(min_cap, cap);
          fastroute_->setEdgeCapacity(x - 1, y - 1, x - 1, y, layer, cap);
        }
      }
      min_cap = min_cap == std::numeric_limits<int>::max() ? 0 : min_cap;
      fastroute_->addVCapacity(min_cap, layer);
    }
  }
  fastroute_->initLowerBoundCapacities();
}

int GlobalRouter::computeGCellCapacity(const int x,
                                       const int y,
                                       const int track_init,
                                       const int track_pitch,
                                       const int track_count,
                                       const bool horizontal)
{
  odb::Rect gcell_rect = getGCellRect(x, y);

  const int min_bound = horizontal ? gcell_rect.yMin() : gcell_rect.xMin();
  const int max_bound = horizontal ? gcell_rect.yMax() : gcell_rect.xMax();

  // indices where tracks enter the interval
  int first_track = (min_bound <= track_init)
                        ? 0
                        : ((min_bound - track_init + track_pitch - 1)
                           / track_pitch);  // ceil
  int last_track = (max_bound < track_init)
                       ? -1
                       : ((max_bound - track_init - 1) / track_pitch);  // floor

  // clamp to valid track indices
  first_track = std::max(0, first_track);
  last_track = std::min(track_count - 1, last_track);

  if (first_track > last_track) {
    return 0;
  }

  return last_track - first_track + 1;
}

odb::Rect GlobalRouter::getGCellRect(const int x, const int y)
{
  const int tile_size = grid_->getTileSize();
  const odb::Rect die_bounds = grid_->getGridArea();
  const odb::Point gcell_center = grid_->getPositionFromGridPoint(x, y);

  const int x_min = gcell_center.getX() - (tile_size / 2);
  const int y_min = gcell_center.getY() - (tile_size / 2);
  int x_max = gcell_center.getX() + (tile_size / 2);
  int y_max = gcell_center.getY() + (tile_size / 2);
  if ((die_bounds.xMax() - x_max) / grid_->getTileSize() < 1) {
    x_max = die_bounds.xMax();
  }
  if ((die_bounds.yMax() - y_max) / grid_->getTileSize() < 1) {
    y_max = die_bounds.yMax();
  }

  return odb::Rect(x_min, y_min, x_max, y_max);
}

void GlobalRouter::setPerturbationAmount(int perturbation)
{
  perturbation_amount_ = perturbation;
};

void GlobalRouter::updateDirtyNets(std::vector<Net*>& dirty_nets)
{
  int min_layer, max_layer;
  getMinMaxLayer(min_layer, max_layer);
  initRoutingLayers(min_layer, max_layer);
  for (odb::dbNet* db_net : dirty_nets_) {
    Net* net = db_net_map_[db_net];
    updateNetPins(net);
    destroyNetWire(net);
    std::string pins_not_covered;
    // compare new positions with last positions & add on vector
    if (pinPositionsChanged(net)
        && (!net->isMergedNet() || !netIsCovered(db_net, pins_not_covered))) {
      dirty_nets.push_back(db_net_map_[db_net]);
      routes_[db_net].clear();
      db_net->clearGuides();
      fastroute_->clearNetRoute(db_net);
    } else if (net->isMergedNet()) {
      if (!isConnected(db_net)) {
        logger_->error(
            GRT, 267, "Net {} has disconnected segments.", net->getName());
      }
    }
    net->setIsMergedNet(false);
    net->setDirtyNet(false);
    net->clearLastPinPositions();
  }
  dirty_nets_.clear();
}

// This function is not currently enabled
void GlobalRouter::shrinkNetRoute(odb::dbNet* db_net)
{
  Net* net = db_net_map_[db_net];
  GRoute& segments = routes_[db_net];
  const int total_segments = segments.size();

  if (net->getNumPins() < 2) {
    return;
  }

  std::string dump;
  if (!netIsCovered(db_net, dump)) {
    logger_->error(
        GRT, 266, "Net {} does not cover all its pins.", net->getName());
  }
  int root = -1, alternate_root = -1;
  std::vector<bool> covers_pin(total_segments, false);

  for (int s = 0; s < total_segments; s++) {
    for (Pin& pin : net->getPins()) {
      if (segmentCoversPin(segments[s], pin)) {
        covers_pin[s] = true;
        alternate_root = s;
        if (pin.isDriver()) {
          root = s;
        }
      }
    }
  }

  if (root == -1) {
    root = alternate_root;
    // If driverless nets issue is fixed there should be no alternate_root and
    // this should become an Error
    logger_->error(GRT, 268, "Net {} has no driver pin.", net->getName());
  }
  AdjacencyList graph = buildNetGraph(db_net);

  // Runs a BFS trough the graph
  std::vector<SegmentIndex> parent(total_segments,
                                   std::numeric_limits<SegmentIndex>::max());
  std::vector<SegmentIndex> total_children(total_segments, 0);
  std::queue<int> q, leafs;
  q.push(root);
  parent[root] = root;

  while (!q.empty()) {
    int node = q.front();
    q.pop();
    for (int child : graph[node]) {
      if (parent[child] != std::numeric_limits<uint16_t>::max()) {
        continue;
      }
      parent[child] = node;
      total_children[node]++;
      q.push(child);
    }
    if (!total_children[node]) {
      leafs.push(node);
    }
  }

  net->setSegmentParent(parent);

  // Prunes branches that dont end on leafs
  std::set<int> segments_to_delete;
  while (!leafs.empty()) {
    int leaf = leafs.front();
    leafs.pop();
    if (!covers_pin[leaf]) {
      segments_to_delete.insert(leaf);
      if (!--total_children[parent[leaf]]) {
        leafs.push(parent[leaf]);
      };
    }
  }

  int total_deleted_segments = 0;
  for (int deleted : segments_to_delete) {
    const int adjusted_seg_id = deleted - total_deleted_segments;
    deleteSegment(net, segments, adjusted_seg_id);
    total_deleted_segments++;
  }
}

void GlobalRouter::deleteSegment(Net* net, GRoute& segments, const int seg_id)
{
  GSegment& seg = segments[seg_id];
  if (seg.init_layer == seg.final_layer) {
    bool is_horizontal = (seg.init_x != seg.final_x);
    bool is_vertical = (seg.init_y != seg.final_y);
    const int tile_size = grid_->getTileSize();
    auto [x0, max_x]
        = std::minmax({(int) ((seg.init_x - grid_->getXMin()) / tile_size),
                       (int) ((seg.final_x - grid_->getXMin()) / tile_size)});

    auto [y0, max_y]
        = std::minmax({(int) ((seg.init_y - grid_->getYMin()) / tile_size),
                       (int) ((seg.final_y - grid_->getYMin()) / tile_size)});

    while (x0 <= max_x && y0 <= max_y) {
      const int x1 = x0 + (is_horizontal ? 1 : 0);
      const int y1 = y0 + (is_vertical ? 1 : 0);
      const int edge_cap
          = fastroute_->getEdgeCapacity(x0, y0, x1, y1, seg.init_layer);
      fastroute_->addAdjustment(
          x0, y0, x1, y1, seg.init_layer, edge_cap + 1, false);
      if (is_horizontal) {
        x0++;
      } else if (is_vertical) {
        y0++;
      }
    }
  }
  net->deleteSegment(seg_id, segments);
}

void GlobalRouter::destroyNetWire(Net* net)
{
  odb::dbWire* wire = net->getDbNet()->getWire();
  if (wire != nullptr) {
    removeWireUsage(wire);
    odb::dbWire::destroy(wire);
  }
  net->setHasWires(false);
}

void GlobalRouter::removeWireUsage(odb::dbWire* wire)
{
  std::vector<odb::dbShape> via_boxes;

  odb::dbWirePath path;
  odb::dbWirePathShape pshape;
  odb::dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      const odb::dbShape& shape = pshape.shape;
      if (shape.isVia()) {
        odb::dbShape::getViaBoxes(shape, via_boxes);
        for (const odb::dbShape& box : via_boxes) {
          odb::dbTechLayer* tech_layer = box.getTechLayer();
          const int layer_routing_level = tech_layer->getRoutingLevel();
          int min_layer, max_layer;
          getMinMaxLayer(min_layer, max_layer);
          if (layer_routing_level == 0 || layer_routing_level < min_layer
              || layer_routing_level > max_layer) {
            continue;
          }
          odb::Rect via_rect = box.getBox();
          removeRectUsage(via_rect, tech_layer);
        }
      } else {
        odb::Rect wire_rect = shape.getBox();
        odb::dbTechLayer* tech_layer = shape.getTechLayer();
        removeRectUsage(wire_rect, tech_layer);
      }
    }
  }
}

void GlobalRouter::removeRectUsage(const odb::Rect& rect,
                                   odb::dbTechLayer* tech_layer)
{
  // Release resources of Rect same like was used
  applyObstructionAdjustment(rect, tech_layer, false, true);
}

bool GlobalRouter::isDetailedRouted(odb::dbNet* db_net)
{
  return (!db_net->isSpecial()
          && db_net->getWireType() == odb::dbWireType::ROUTED
          && db_net->getWire());
}

Rudy* GlobalRouter::getRudy()
{
  if (rudy_ == nullptr) {
    rudy_ = new Rudy(db_->getChip()->getBlock(), this);
  }

  return rudy_;
}

bool GlobalRouter::findPinAccessPointPositions(
    const Pin& pin,
    std::map<int, std::vector<PointPair>>& ap_positions,
    bool all_access_points)
{
  std::vector<odb::dbAccessPoint*> access_points;
  // get APs from odb
  if (pin.isPort()) {
    for (const odb::dbBPin* bpin : pin.getBTerm()->getBPins()) {
      const std::vector<odb::dbAccessPoint*>& bpin_pas
          = bpin->getAccessPoints();
      access_points.insert(
          access_points.begin(), bpin_pas.begin(), bpin_pas.end());
    }
  } else if (pin.isCorePin() && !all_access_points) {
    access_points = pin.getITerm()->getPrefAccessPoints();
  } else {
    // For non-core cells, DRT does not assign preferred APs.
    // Use all APs to ensure the guides covering at least one AP.
    for (const auto& [pin, aps] : pin.getITerm()->getAccessPoints()) {
      access_points.insert(access_points.end(), aps.begin(), aps.end());
    }
  }

  if (access_points.empty()) {
    return false;
  }

  for (const odb::dbAccessPoint* ap : access_points) {
    odb::Point ap_position = ap->getPoint();
    if (!pin.isPort()) {
      odb::dbTransform xform;
      int x, y;
      pin.getITerm()->getInst()->getLocation(x, y);
      xform.setOffset({x, y});
      xform.setOrient(odb::dbOrientType(odb::dbOrientType::R0));
      xform.apply(ap_position);
    }

    const int ap_layer = ap->getLayer()->getRoutingLevel();
    ap_positions[ap_layer].emplace_back(ap_position,
                                        grid_->getPositionOnGrid(ap_position));
  }

  return true;
}

std::vector<RoutePt> GlobalRouter::findOnGridPositions(
    const Pin& pin,
    bool& has_access_points,
    odb::Point& pos_on_grid,
    bool ignore_db_access_points)
{
  std::map<int, std::vector<PointPair>> ap_positions;

  // temporarily ignore odb access points when incremental changes
  // are made, in order to avoid getting invalid APs
  has_access_points = findPinAccessPointPositions(pin, ap_positions);

  std::vector<RoutePt> positions_on_grid;

  if (has_access_points && !ignore_db_access_points) {
    for (const auto& [layer, positions] : ap_positions) {
      for (const PointPair& position : positions) {
        pos_on_grid = position.second;
        positions_on_grid.emplace_back(
            pos_on_grid.getX(), pos_on_grid.getY(), layer);
      }
    }
  } else {
    // if odb doesn't have any APs, run the grt version considering the
    // center of the pin shapes
    const int conn_layer = pin.getConnectionLayer();
    const std::vector<odb::Rect>& pin_boxes = pin.getBoxes().at(conn_layer);
    for (const odb::Rect& pin_box : pin_boxes) {
      odb::Point rect_middle = getRectMiddle(pin_box);
      const int pin_box_length = std::max(pin_box.dx(), pin_box.dy());

      // if a macro/pad pin crosses multiple gcells, ensure the position closest
      // to the macro/pad boundary will be selected as its on grid position.
      if (pin.getEdge() != PinEdge::none
          && pin_box_length >= grid_->getTileSize()) {
        pos_on_grid = grid_->getPositionOnGrid(
            pin.getPositionNearInstEdge(pin_box, rect_middle));
      } else {
        pos_on_grid = grid_->getPositionOnGrid(rect_middle);
        // if a macro/pad pin is unreachable due to not having enough resources
        // at its on grid position, get the position closest to the macro/pad
        // boundary to ensure routability
        if (pin.isConnectedToPadOrMacro()
            && !isPinReachable(pin, pos_on_grid)) {
          pos_on_grid = grid_->getPositionOnGrid(
              pin.getPositionNearInstEdge(pin_box, rect_middle));
        }
      }
      positions_on_grid.emplace_back(
          pos_on_grid.getX(), pos_on_grid.getY(), conn_layer);
      has_access_points = false;
    }
  }

  return positions_on_grid;
}

void GlobalRouter::findPins(Net* net)
{
  for (Pin& pin : net->getPins()) {
    bool has_access_points;
    odb::Point pos_on_grid;
    std::vector<RoutePt> pin_positions_on_grid
        = findOnGridPositions(pin, has_access_points, pos_on_grid);

    computePinPositionOnGrid(
        pin_positions_on_grid, pin, pos_on_grid, has_access_points);
  }
}

void GlobalRouter::computePinPositionOnGrid(
    std::vector<RoutePt>& pin_positions_on_grid,
    Pin& pin,
    odb::Point& pos_on_grid,
    const bool has_access_points)
{
  int votes = -1;

  RoutePt pin_position(pin.getPosition().getX(),
                       pin.getPosition().getY(),
                       pin.getConnectionLayer());
  for (const RoutePt& pos : pin_positions_on_grid) {
    int equals = std::count(
        pin_positions_on_grid.begin(), pin_positions_on_grid.end(), pos);
    if (equals > votes) {
      pin_position = pos;
      votes = equals;
    }
  }

  // check if the pin has access points to avoid changing the position on grid
  // when the pin overlaps with a single track.
  // this way, the result based on drt APs is maintained
  if (!has_access_points && pinOverlapsWithSingleTrack(pin, pos_on_grid)) {
    const int conn_layer = pin.getConnectionLayer();
    odb::dbTechLayer* layer = routing_layers_[conn_layer];
    pos_on_grid = grid_->getPositionOnGrid(pos_on_grid);
    if (!(pos_on_grid == odb::Point(pin_position.x(), pin_position.y()))
        && ((layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL
             && pos_on_grid.y() != pin_position.y())
            || (layer->getDirection() == odb::dbTechLayerDir::VERTICAL
                && pos_on_grid.x() != pin_position.x()))) {
      pin_position = RoutePt(
          pos_on_grid.getX(), pos_on_grid.getY(), pin_position.layer());
    }
  }

  pin.setOnGridPosition(odb::Point(pin_position.x(), pin_position.y()));
  pin.setConnectionLayer(pin_position.layer());
}

void GlobalRouter::updatePinAccessPoints()
{
  for (const auto& [db_net, net] : db_net_map_) {
    std::map<odb::dbITerm*, odb::Point3D> iterm_to_aps;
    std::map<odb::dbBTerm*, odb::Point3D> bterm_to_aps;
    cugr_->getITermsAccessPoints(db_net, iterm_to_aps);
    cugr_->getBTermsAccessPoints(db_net, bterm_to_aps);

    auto updatePinPos = [&](Pin& pin, auto* term, const auto& ap_map) {
      if (auto it = ap_map.find(term); it != ap_map.end()) {
        const auto& ap = it->second;
        pin.setConnectionLayer(ap.z());
        pin.setOnGridPosition(
            grid_->getPositionOnGrid(odb::Point(ap.x(), ap.y())));
      }
    };

    for (Pin& pin : net->getPins()) {
      if (pin.isPort()) {
        updatePinPos(pin, pin.getBTerm(), bterm_to_aps);
      } else {
        updatePinPos(pin, pin.getITerm(), iterm_to_aps);
      }
    }
  }
}

int GlobalRouter::getNetMaxRoutingLayer(const Net* net)
{
  return net->getSignalType() == odb::dbSigType::CLOCK
                 && getMaxLayerForClock() > 0
             ? getMaxLayerForClock()
             : getMaxRoutingLayer();
}

void GlobalRouter::findFastRoutePins(Net* net,
                                     std::vector<RoutePt>& pins_on_grid,
                                     int& root_idx)
{
  root_idx = 0;
  const int max_routing_layer = getNetMaxRoutingLayer(net);

  for (Pin& pin : net->getPins()) {
    odb::Point pin_position = pin.getOnGridPosition();
    int conn_layer = pin.getConnectionLayer();
    conn_layer = std::min(conn_layer, max_routing_layer);

    int pinX = ((pin_position.x() - grid_->getXMin()) / grid_->getTileSize());
    int pinY = ((pin_position.y() - grid_->getYMin()) / grid_->getTileSize());

    if (!(pinX < 0 || pinX >= grid_->getXGrids() || pinY < -1
          || pinY >= grid_->getYGrids() || conn_layer > grid_->getNumLayers()
          || conn_layer <= 0)) {
      bool duplicated = false;
      for (RoutePt& pin_pos : pins_on_grid) {
        if (pinX == pin_pos.x() && pinY == pin_pos.y()
            && conn_layer == pin_pos.layer()) {
          duplicated = true;
          break;
        }
      }

      if (!duplicated) {
        pins_on_grid.emplace_back(pinX, pinY, conn_layer);
        if (pin.isDriver()) {
          root_idx = pins_on_grid.size() - 1;
        }
      }
    }
  }
}

float GlobalRouter::getNetSlack(Net* net)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Net* sta_net = network->dbToSta(net->getDbNet());
  sta::Slack slack = sta_->slack(sta_net, sta::MinMax::max());
  return slack;
}

void GlobalRouter::initNetlist(std::vector<Net*>& nets)
{
  pad_pins_connections_.clear();

  int min_degree = std::numeric_limits<int>::max();
  // Do NOT use numeric_limits<int>::min() to init
  // this because if there are no FR nets the max
  // degree will be big negative and cannot be used
  // to init the vectors in FR.
  int max_degree = 1;

  if (nets.size() > 1 && seed_ != 0) {
    std::mt19937 g;
    g.seed(seed_);

    utl::shuffle(nets.begin(), nets.end(), g);
  }
  for (Net* net : nets) {
    int pin_count = net->getNumPins();
    int min_layer, max_layer;
    getNetLayerRange(net->getDbNet(), min_layer, max_layer);
    odb::dbTechLayer* max_routing_layer
        = db_->getTech()->findRoutingLayer(max_layer);
    if (pin_count > 1
        && (!net->hasWires() || net->hasStackedVias(max_routing_layer))) {
      min_degree = std::min(pin_count, min_degree);

      max_degree = std::max(pin_count, max_degree);
      makeFastrouteNet(net);
    }
  }
  fastroute_->setMaxNetDegree(max_degree);

  if (verbose_) {
    min_degree = nets.empty() ? 0 : min_degree;
    max_degree = nets.empty() ? 0 : max_degree;
    logger_->info(GRT, 1, "Minimum degree: {}", min_degree);
    logger_->info(GRT, 2, "Maximum degree: {}", max_degree);
  }

  // add resources for pin access in macro/pad pins after defining their on grid
  // position
  addResourcesForPinAccess();
  fastroute_->initAuxVar();
}

bool GlobalRouter::pinPositionsChanged(Net* net)
{
  bool is_diferent = false;
  std::map<RoutePt, int> cnt_pos;
  const std::multiset<RoutePt>& last_pos = net->getLastPinPositions();
  for (const Pin& pin : net->getPins()) {
    const odb::Point& pos = pin.getOnGridPosition();
    cnt_pos[RoutePt(pos.getX(), pos.getY(), pin.getConnectionLayer())]++;
  }
  for (const RoutePt& last : last_pos) {
    cnt_pos[last]--;
  }
  for (const auto& [pos, count] : cnt_pos) {
    if (count != 0) {
      is_diferent = true;
      break;
    }
  }
  return is_diferent;
}

bool GlobalRouter::newPinOnGrid(Net* net, std::multiset<RoutePt>& last_pos)
{
  for (const Pin& pin : net->getPins()) {
    if (last_pos.find(RoutePt(pin.getOnGridPosition().getX(),
                              pin.getOnGridPosition().getY(),
                              pin.getConnectionLayer()))
        == last_pos.end()) {
      return true;
    }
  }

  return false;
}

void GlobalRouter::makeFastrouteNet(Net* net)
{
  std::vector<RoutePt> pins_on_grid;
  int root_idx;
  findFastRoutePins(net, pins_on_grid, root_idx);

  bool is_clock = (net->getSignalType() == odb::dbSigType::CLOCK);
  std::vector<int8_t>* edge_cost_per_layer;
  int8_t edge_cost_for_net;
  computeTrackConsumption(net, edge_cost_for_net, edge_cost_per_layer);

  // set layer restriction only to clock nets that are not connected to
  // leaf iterms
  int min_layer, max_layer;
  getNetLayerRange(net->getDbNet(), min_layer, max_layer);

  FrNet* fr_net = fastroute_->addNet(net->getDbNet(),
                                     is_clock,
                                     net->isLocal(),
                                     root_idx,
                                     edge_cost_for_net,
                                     min_layer - 1,
                                     max_layer - 1,
                                     net->getSlack(),
                                     edge_cost_per_layer);
  // TODO: improve net layer range with more dynamic layer restrictions
  // when there's no room in the specified range
  // See https://github.com/The-OpenROAD-Project/OpenROAD/pull/2893 and
  // https://github.com/The-OpenROAD-Project/OpenROAD/discussions/2870
  // for a detailed discussion

  for (RoutePt& pin_pos : pins_on_grid) {
    fr_net->addPin(pin_pos.x(), pin_pos.y(), pin_pos.layer() - 1);
  }

  // Save stt input on debug file
  if (fastroute_->hasSaveSttInput()
      && net->getDbNet() == fastroute_->getDebugNet()) {
    saveSttInputFile(net);
  }
}

void GlobalRouter::saveSttInputFile(Net* net)
{
  std::string file_name = fastroute_->getSttInputFileName();
  const float net_alpha = stt_builder_->getAlpha(net->getDbNet());
  std::ofstream out(file_name.c_str());
  out << "Net " << net->getName() << " " << net_alpha << "\n";
  for (Pin& pin : net->getPins()) {
    odb::Point position = pin.getOnGridPosition();  // Pin position on grid
    out << pin.getName() << " " << position.getX() << " " << position.getY()
        << "\n";
  }
  out.close();
}

void GlobalRouter::getNetLayerRange(odb::dbNet* db_net,
                                    int& min_layer,
                                    int& max_layer)
{
  Net* net = db_net_map_[db_net];
  int pin_min_layer = std::numeric_limits<int>::max();
  for (const Pin& pin : net->getPins()) {
    pin_min_layer = std::min(pin_min_layer, pin.getConnectionLayer());
  }

  bool is_non_leaf_clock = isNonLeafClock(db_net);
  min_layer = (is_non_leaf_clock && getMinLayerForClock() > 0)
                  ? getMinLayerForClock()
                  : getMinRoutingLayer();
  min_layer = std::max(min_layer, pin_min_layer);
  max_layer = (is_non_leaf_clock && getMaxLayerForClock() > 0)
                  ? getMaxLayerForClock()
                  : getMaxRoutingLayer();
}

void GlobalRouter::getGridSize(int& x_grids, int& y_grids)
{
  x_grids = grid_->getXGrids();
  y_grids = grid_->getYGrids();
}

int GlobalRouter::getGridTileSize()
{
  return grid_->getTileSize();
}

void GlobalRouter::getCapacityReductionData(CapacityReductionData& cap_red_data)
{
  fastroute_->getCapacityReductionData(cap_red_data);
}

void GlobalRouter::computeTrackConsumption(
    const Net* net,
    int8_t& track_consumption,
    std::vector<int8_t>*& edge_costs_per_layer)
{
  edge_costs_per_layer = nullptr;
  track_consumption = 1;
  odb::dbNet* db_net = net->getDbNet();
  int net_min_layer;
  int net_max_layer;
  getNetLayerRange(db_net, net_min_layer, net_max_layer);

  odb::dbTechNonDefaultRule* ndr = db_net->getNonDefaultRule();
  if (ndr) {
    int num_layers = grid_->getNumLayers();
    edge_costs_per_layer = new std::vector<int8_t>(num_layers + 1, 1);
    std::vector<odb::dbTechLayerRule*> layer_rules;
    ndr->getLayerRules(layer_rules);

    for (odb::dbTechLayerRule* layer_rule : layer_rules) {
      int layerIdx = layer_rule->getLayer()->getRoutingLevel();
      if (layerIdx > net_max_layer || layerIdx < net_min_layer) {
        continue;
      }
      RoutingTracks routing_tracks = getRoutingTracksByIndex(layerIdx);
      int default_width = layer_rule->getLayer()->getWidth();
      int default_pitch = routing_tracks.getTrackPitch();

      int ndr_spacing = layer_rule->getSpacing();
      int ndr_width = layer_rule->getWidth();
      int ndr_pitch = ndr_width / 2 + ndr_spacing + default_width / 2;

      int consumption = std::ceil((float) ndr_pitch / default_pitch);
      if (consumption > std::numeric_limits<int8_t>::max()) {
        logger_->error(GRT,
                       272,
                       "NDR consumption {} exceeds {} and is unsupported",
                       consumption,
                       std::numeric_limits<int8_t>::max());
      }
      (*edge_costs_per_layer)[layerIdx - 1] = consumption;

      track_consumption
          = std::max(track_consumption, static_cast<int8_t>(consumption));

      if (logger_->debugCheck(GRT, "ndrInfo", 1)) {
        logger_->report(
            "Net: {} NDR cost in {} (float/int): {}/{}  Edge cost: {}",
            net->getConstName(),
            layer_rule->getLayer()->getConstName(),
            (float) ndr_pitch / default_pitch,
            consumption,
            track_consumption);
      }
    }
  }
}

std::vector<LayerId> GlobalRouter::findTransitionLayers()
{
  std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias
      = block_->getDefaultVias();
  std::vector<LayerId> transition_layers;
  for (const auto [tech_layer, via] : default_vias) {
    if (tech_layer->getRoutingLevel() > getMaxRoutingLayer()) {
      continue;
    }

    const bool vertical
        = tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
    int via_width = 0;
    for (const auto box : default_vias[tech_layer]->getBoxes()) {
      if (box->getTechLayer() == tech_layer) {
        via_width = vertical ? box->getDY() : box->getDX();
        break;
      }
    }

    const double track_pitch
        = grid_->getTrackPitches()[tech_layer->getRoutingLevel() - 1];
    // threshold to define what is a transition layer based on the width of the
    // fat via. using 0.8 to consider transition layers for wide vias
    const float fat_via_threshold = 0.8;
    if (via_width / track_pitch > fat_via_threshold) {
      transition_layers.push_back(tech_layer->getRoutingLevel());
    }
  }

  return transition_layers;
}

// reduce the capacity of transition layers in 50% to have less wires in
// regions where a fat via is necessary.
// this way, the detailed router will have more room to fix violations near
// the fat vias.
void GlobalRouter::adjustTransitionLayers(
    const std::vector<LayerId>& transition_layers,
    std::map<int, std::vector<odb::Rect>>& layer_obs_map)
{
  odb::dbTech* tech = db_->getTech();
  for (LayerId layer : transition_layers) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
    TileSet tiles_to_reduce;
    for (const auto& obs : layer_obs_map[layer - 1]) {
      odb::Rect first_tile_bds, last_tile_bds;
      odb::Point first_tile, last_tile;
      grid_->getBlockedTiles(
          obs, first_tile_bds, last_tile_bds, first_tile, last_tile);
      if (first_tile.x() == last_tile.x() && first_tile.y() == last_tile.y()) {
        continue;
      }
      for (int y = first_tile.y(); y < last_tile.y(); y++) {
        for (int x = first_tile.x(); x < last_tile.x(); x++) {
          tiles_to_reduce.emplace(std::make_pair(x, y));
        }
      }
    }
    adjustTileSet(tiles_to_reduce, tech_layer);
  }
}

void GlobalRouter::adjustTileSet(const TileSet& tiles_to_reduce,
                                 odb::dbTechLayer* tech_layer)
{
  const int layer = tech_layer->getRoutingLevel();
  for (const auto& [x, y] : tiles_to_reduce) {
    int end_x = x;
    int end_y = y;
    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      end_x = x + 1;
    } else {
      end_y = y + 1;
    }
    const float edge_cap
        = fastroute_->getEdgeCapacity(x, y, end_x, end_y, layer);
    int new_cap = std::floor(edge_cap * 0.5);
    new_cap = edge_cap > 0 ? std::max(new_cap, 1) : new_cap;
    fastroute_->addAdjustment(x, y, end_x, end_y, layer, new_cap, true);
  }
}

void GlobalRouter::computeUserGlobalAdjustments(int min_routing_layer,
                                                int max_routing_layer)
{
  if (adjustment_ == 0.0) {
    return;
  }

  for (int l = min_routing_layer; l <= max_routing_layer; l++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(l);
    if (tech_layer->getLayerAdjustment() == 0.0) {
      tech_layer->setLayerAdjustment(adjustment_);
    }
  }
}

void GlobalRouter::computeUserLayerAdjustments(const int min_routing_layer,
                                               const int max_routing_layer)
{
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  for (int layer = 1; layer <= max_routing_layer; layer++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(layer);
    const bool inside_layer_range
        = (layer >= min_routing_layer && layer <= max_routing_layer);
    float adjustment = tech_layer->getLayerAdjustment();
    const bool is_reduce = adjustment > 0;

    if (adjustment != 0) {
      if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL
          && inside_layer_range) {
        for (int y = 1; y <= y_grids; y++) {
          for (int x = 1; x < x_grids; x++) {
            int edge_cap
                = fastroute_->getEdgeCapacity(x - 1, y - 1, x, y - 1, layer);
            int new_h_capacity
                = std::floor((float) edge_cap * (1 - adjustment));
            new_h_capacity = edge_cap > 0 && adjustment != 1
                                 ? std::max(new_h_capacity, 1)
                                 : new_h_capacity;
            fastroute_->addAdjustment(
                x - 1, y - 1, x, y - 1, layer, new_h_capacity, is_reduce);
          }
        }
      }

      if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL
          && inside_layer_range) {
        for (int x = 1; x <= x_grids; x++) {
          for (int y = 1; y < y_grids; y++) {
            int edge_cap
                = fastroute_->getEdgeCapacity(x - 1, y - 1, x - 1, y, layer);
            int new_v_capacity
                = std::floor((float) edge_cap * (1 - adjustment));
            new_v_capacity = edge_cap > 0 && adjustment != 1
                                 ? std::max(new_v_capacity, 1)
                                 : new_v_capacity;
            fastroute_->addAdjustment(
                x - 1, y - 1, x - 1, y, layer, new_v_capacity, is_reduce);
          }
        }
      }
    }
  }
}

void GlobalRouter::computeRegionAdjustments(const odb::Rect& region,
                                            int layer,
                                            float reduction_percentage)
{
  odb::Rect die_box = grid_->getGridArea();

  if ((die_box.xMin() > region.ll().x() && die_box.yMin() > region.ll().y())
      || (die_box.xMax() < region.ur().x()
          && die_box.yMax() < region.ur().y())) {
    logger_->error(GRT, 72, "Informed region is outside die area.");
  }

  odb::dbTechLayer* routing_layer = routing_layers_[layer];
  bool vertical
      = routing_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;

  odb::Rect first_tile_box, last_tile_box;
  odb::Point first_tile, last_tile;
  grid_->getBlockedTiles(
      region, first_tile_box, last_tile_box, first_tile, last_tile);

  RoutingTracks routing_tracks = getRoutingTracksByIndex(layer);
  int track_space = routing_tracks.getUsePitch();

  int first_tile_reduce = grid_->computeTileReduce(
      region, first_tile_box, track_space, true, routing_layer->getDirection());

  int last_tile_reduce = grid_->computeTileReduce(
      region, last_tile_box, track_space, false, routing_layer->getDirection());

  for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
    for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
      double edge_cap
          = vertical ? fastroute_->getEdgeCapacity(x, y, x, y + 1, layer)
                     : fastroute_->getEdgeCapacity(x, y, x + 1, y, layer);
      int new_cap = std::floor(edge_cap * (1 - reduction_percentage));

      if (x == first_tile.getX() || y == first_tile.getY()) {
        new_cap = edge_cap - (first_tile_reduce * (1 - reduction_percentage));
      } else if (x == last_tile.getX() || y == last_tile.getY()) {
        new_cap = edge_cap - (last_tile_reduce * (1 - reduction_percentage));
      }

      new_cap = edge_cap > 0 && reduction_percentage != 1 ? std::max(new_cap, 1)
                                                          : new_cap;
      if (vertical) {
        fastroute_->addAdjustment(x, y, x, y + 1, layer, new_cap, true);
      } else {
        fastroute_->addAdjustment(x, y, x + 1, y, layer, new_cap, true);
      }
    }
  }
}

bool GlobalRouter::hasAvailableResources(bool is_horizontal,
                                         const int& pos_x,
                                         const int& pos_y,
                                         const int& layer_level,
                                         odb::dbNet* db_net)
{
  // transform from real position to grid pos of fastroute
  int grid_x = ((pos_x - grid_->getXMin()) / grid_->getTileSize());
  int grid_y = ((pos_y - grid_->getYMin()) / grid_->getTileSize());
  int cap = 0;
  if (is_horizontal) {
    cap = fastroute_->getAvailableResources(
        grid_x, grid_y, grid_x + 1, grid_y, layer_level);
  } else {
    cap = fastroute_->getAvailableResources(
        grid_x, grid_y, grid_x, grid_y + 1, layer_level);
  }
  return cap >= fastroute_->getDbNetLayerEdgeCost(db_net, layer_level);
}

// Find the position of the middle of a GCell closest to the position
odb::Point GlobalRouter::getPositionOnGrid(const odb::Point& real_position)
{
  return grid_->getPositionOnGrid(real_position);
}

void GlobalRouter::updateResources(const int& init_x,
                                   const int& init_y,
                                   const int& final_x,
                                   const int& final_y,
                                   const int& layer_level,
                                   int used,
                                   odb::dbNet* db_net)
{
  // transform from real position to grid pos of fastrouter
  int grid_init_x = ((init_x - grid_->getXMin()) / grid_->getTileSize());
  int grid_init_y = ((init_y - grid_->getYMin()) / grid_->getTileSize());
  int grid_final_x = ((final_x - grid_->getXMin()) / grid_->getTileSize());
  int grid_final_y = ((final_y - grid_->getYMin()) / grid_->getTileSize());

  fastroute_->updateEdge2DAnd3DUsage(grid_init_x,
                                     grid_init_y,
                                     grid_final_x,
                                     grid_final_y,
                                     layer_level,
                                     used,
                                     db_net);
}

void GlobalRouter::updateFastRouteGridsLayer(const int& init_x,
                                             const int& init_y,
                                             const int& final_x,
                                             const int& final_y,
                                             const int& layer_level,
                                             const int& new_layer_level,
                                             odb::dbNet* db_net)
{
  // transform from real position to grid pos of fastrouter
  int grid_init_x = ((init_x - grid_->getXMin()) / grid_->getTileSize());
  int grid_init_y = ((init_y - grid_->getYMin()) / grid_->getTileSize());
  int grid_final_x = ((final_x - grid_->getXMin()) / grid_->getTileSize());
  int grid_final_y = ((final_y - grid_->getYMin()) / grid_->getTileSize());
  // update treeedges
  fastroute_->updateRouteGridsLayer(grid_init_x,
                                    grid_init_y,
                                    grid_final_x,
                                    grid_final_y,
                                    layer_level - 1,
                                    new_layer_level - 1,
                                    db_net);
}

// Use release flag to increase rather than reduce resources on obstruction
void GlobalRouter::applyObstructionAdjustment(const odb::Rect& obstruction,
                                              odb::dbTechLayer* tech_layer,
                                              bool is_macro,
                                              bool release)
{
  // compute the intersection between obstruction and the die area
  // only when they are overlapping to avoid assert error during
  // intersect() function
  const odb::Rect& die_area = grid_->getGridArea();
  odb::Rect obstruction_rect;
  if (die_area.overlaps(obstruction)) {
    obstruction_rect = die_area.intersect(obstruction);
    // ignores obstructions completely outside the die area
    if (obstruction_rect.isInverted()) {
      return;
    }
  }

  odb::Rect first_tile_box, last_tile_box;
  odb::Point first_tile, last_tile;
  grid_->getBlockedTiles(
      obstruction_rect, first_tile_box, last_tile_box, first_tile, last_tile);

  bool vertical = tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;

  int layer = tech_layer->getRoutingLevel();

  int track_space = grid_->getTrackPitches()[layer - 1];

  const int first_cap = vertical
                            ? fastroute_->getEdgeCapacity(first_tile.getX(),
                                                          first_tile.getY(),
                                                          first_tile.getX(),
                                                          first_tile.getY() + 1,
                                                          layer)
                            : fastroute_->getEdgeCapacity(first_tile.getX(),
                                                          first_tile.getY(),
                                                          first_tile.getX() + 1,
                                                          first_tile.getY(),
                                                          layer);
  const int last_cap = vertical
                           ? fastroute_->getEdgeCapacity(last_tile.getX(),
                                                         last_tile.getY(),
                                                         last_tile.getX(),
                                                         last_tile.getY() + 1,
                                                         layer)
                           : fastroute_->getEdgeCapacity(last_tile.getX(),
                                                         last_tile.getY(),
                                                         last_tile.getX() + 1,
                                                         last_tile.getY(),
                                                         layer);

  interval<int>::type first_tile_reduce_interval
      = grid_->computeTileReduceInterval(obstruction_rect,
                                         first_tile_box,
                                         track_space,
                                         true,
                                         tech_layer->getDirection(),
                                         first_cap,
                                         is_macro);
  interval<int>::type last_tile_reduce_interval
      = grid_->computeTileReduceInterval(obstruction_rect,
                                         last_tile_box,
                                         track_space,
                                         false,
                                         tech_layer->getDirection(),
                                         last_cap,
                                         is_macro);

  int grid_limit = vertical ? grid_->getYGrids() : grid_->getXGrids();

  std::vector<int> track_spaces;
  if (release) {
    track_spaces = grid_->getTrackPitches();
  }

  if (!vertical) {
    // if obstruction is inside a single gcell, block the edge between current
    // gcell and the adjacent gcell
    if (first_tile.getX() == last_tile.getX()
        && last_tile.getX() + 1 < grid_limit) {
      int last_tile_x = last_tile.getX() + 1;
      last_tile.setX(last_tile_x);
    }
    fastroute_->addHorizontalAdjustments(first_tile,
                                         last_tile,
                                         layer,
                                         first_tile_reduce_interval,
                                         last_tile_reduce_interval,
                                         track_spaces,
                                         release);
  } else {
    // if obstruction is inside a single gcell, block the edge between current
    // gcell and the adjacent gcell
    if (first_tile.getY() == last_tile.getY()
        && last_tile.getY() + 1 < grid_limit) {
      int last_tile_y = last_tile.getY() + 1;
      last_tile.setY(last_tile_y);
    }
    fastroute_->addVerticalAdjustments(first_tile,
                                       last_tile,
                                       layer,
                                       first_tile_reduce_interval,
                                       last_tile_reduce_interval,
                                       track_spaces,
                                       release);
  }
}

// For macro pins in the east and north edges of the macros, and pins on west
// and south edges that have their APs in GCells completely blocked by macro
// obstructions, the access for them can be blocked by the macro obstructions.
// This function adds the resources necessary to route these pins. Only pins in
// the east and north edges are affected because FastRoute routes from left to
// right, and bottom to top.
void GlobalRouter::addResourcesForPinAccess()
{
  odb::dbTech* tech = db_->getTech();
  for (const auto& [db_net, net] : db_net_map_) {
    for (const Pin& pin : net->getPins()) {
      if (pin.isConnectedToPadOrMacro() && (pin.getEdge() != PinEdge::none)) {
        const odb::Point& pos = pin.getOnGridPosition();
        int pin_x = ((pos.x() - grid_->getXMin()) / grid_->getTileSize());
        int pin_y = ((pos.y() - grid_->getYMin()) / grid_->getTileSize());
        const int layer = pin.getConnectionLayer();
        odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
        if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
          const bool north_pin = pin.getEdge() == PinEdge::north;
          const int pin_y1 = north_pin ? pin_y : pin_y - 1;
          const int pin_y2 = north_pin ? pin_y + 1 : pin_y;

          // Ensure we do not go out of bounds when the pin is at the edge of
          // the grid. If the pin is on the south edge and at y=0, there is no
          // room for adding resources.
          if (pin_y1 < 0) {
            continue;
          }

          const int edge_cap = fastroute_->getEdgeCapacity(
              pin_x, pin_y1, pin_x, pin_y2, layer);
          fastroute_->addAdjustment(
              pin_x, pin_y1, pin_x, pin_y2, layer, edge_cap + 1, false);
        } else {
          const bool east_pin = pin.getEdge() == PinEdge::east;
          const int pin_x1 = east_pin ? pin_x : pin_x - 1;
          const int pin_x2 = east_pin ? pin_x + 1 : pin_x;

          // Ensure we do not go out of bounds when the pin is at the edge of
          // the grid. If the pin is on the west edge and at x=0, there is no
          // room for adding resources.
          if (pin_x1 < 0) {
            continue;
          }

          const int edge_cap = fastroute_->getEdgeCapacity(
              pin_x1, pin_y, pin_x2, pin_y, layer);
          fastroute_->addAdjustment(
              pin_x1, pin_y, pin_x2, pin_y, layer, edge_cap + 1, false);
        }
      }
    }
  }
}

bool GlobalRouter::isPinReachable(const Pin& pin, const odb::Point& pos_on_grid)
{
  odb::dbTech* tech = db_->getTech();
  const int layer = pin.getConnectionLayer();
  int pin_x = ((pos_on_grid.x() - grid_->getXMin()) / grid_->getTileSize());
  int pin_y = ((pos_on_grid.y() - grid_->getYMin()) / grid_->getTileSize());
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);

  // pins on the east and north edges of macros will always have enough
  // resources due to the function addResourcesForPinAccess.
  if (pin.getEdge() == PinEdge::east || pin.getEdge() == PinEdge::north) {
    return true;
  }

  int edge_cap = 0;
  if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    if (pin_y != 0) {
      edge_cap
          = fastroute_->getEdgeCapacity(pin_x, pin_y - 1, pin_x, pin_y, layer);
    }
  } else if (pin_x != 0) {
    edge_cap
        = fastroute_->getEdgeCapacity(pin_x - 1, pin_y, pin_x, pin_y, layer);
  }

  return edge_cap > 0;
}

void GlobalRouter::setAdjustment(const float adjustment)
{
  adjustment_ = adjustment;
}

int GlobalRouter::getMinRoutingLayer()
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  return block_->getMinRoutingLayer();
}

int GlobalRouter::getMaxRoutingLayer()
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  return block_->getMaxRoutingLayer();
}

int GlobalRouter::getMinLayerForClock()
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  return block_->getMinLayerForClock();
}

int GlobalRouter::getMaxLayerForClock()
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  return block_->getMaxLayerForClock();
}

void GlobalRouter::setMinRoutingLayer(const int min_layer)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  block_->setMinRoutingLayer(min_layer);
}

void GlobalRouter::setMaxRoutingLayer(const int max_layer)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  block_->setMaxRoutingLayer(max_layer);
}

void GlobalRouter::setMinLayerForClock(const int min_layer)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  block_->setMinLayerForClock(min_layer);
}

void GlobalRouter::setMaxLayerForClock(const int max_layer)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  block_->setMaxLayerForClock(max_layer);
}

void GlobalRouter::setCriticalNetsPercentage(float critical_nets_percentage)
{
  if (sta_->getDbNetwork()->defaultLibertyLibrary() == nullptr) {
    critical_nets_percentage = 0;
    logger_->warn(
        GRT,
        301,
        "Timing is not available, setting critical nets percentage to 0.");
  }
  if (use_cugr_) {
    cugr_->setCriticalNetsPercentage(critical_nets_percentage);
  } else {
    fastroute_->setCriticalNetsPercentage(critical_nets_percentage);
  }
}

void GlobalRouter::addLayerAdjustment(int layer, float reduction_percentage)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
  if (layer > getMaxRoutingLayer() && getMaxRoutingLayer() > 0) {
    if (verbose_) {
      odb::dbTechLayer* max_tech_layer
          = tech->findRoutingLayer(getMaxRoutingLayer());
      logger_->warn(GRT,
                    30,
                    "Specified layer {} for adjustment is greater than max "
                    "routing layer {} and will be ignored.",
                    tech_layer->getName(),
                    max_tech_layer->getName());
    }
  } else {
    tech_layer->setLayerAdjustment(reduction_percentage);
  }
}

void GlobalRouter::addRegionAdjustment(int min_x,
                                       int min_y,
                                       int max_x,
                                       int max_y,
                                       int layer,
                                       float reduction_percentage)
{
  region_adjustments_.emplace_back(
      min_x, min_y, max_x, max_y, layer, reduction_percentage);
}

void GlobalRouter::setVerbose(const bool v)
{
  verbose_ = v;
}

void GlobalRouter::setCongestionIterations(int iterations)
{
  congestion_iterations_ = iterations;
}

void GlobalRouter::setCongestionReportIterStep(int congestion_report_iter_step)
{
  congestion_report_iter_step_ = congestion_report_iter_step;
}

void GlobalRouter::setCongestionReportFile(const char* file_name)
{
  congestion_file_name_ = file_name;
}

void GlobalRouter::setGridOrigin(int x, int y)
{
  grid_origin_ = odb::Point(x, y);
}

void GlobalRouter::setAllowCongestion(bool allow_congestion)
{
  allow_congestion_ = allow_congestion;
}

void GlobalRouter::setResistanceAware(bool resistance_aware)
{
  resistance_aware_ = resistance_aware;
}

void GlobalRouter::setMacroExtension(int macro_extension)
{
  macro_extension_ = macro_extension;
}

void GlobalRouter::setCapacitiesPerturbationPercentage(float percentage)
{
  caps_perturbation_percentage_ = percentage;
}

void GlobalRouter::perturbCapacities()
{
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  int num_2d_grids = x_grids * y_grids;
  int num_perturbations = (caps_perturbation_percentage_ / 100) * num_2d_grids;

  std::mt19937 g;
  g.seed(seed_);

  for (int layer = 1; layer <= getMaxRoutingLayer(); layer++) {
    std::uniform_int_distribution<int> uni_x(1, std::max(x_grids - 1, 1));
    std::uniform_int_distribution<int> uni_y(1, std::max(y_grids - 1, 1));
    std::bernoulli_distribution add_or_subtract;

    for (int i = 0; i < num_perturbations; i++) {
      int x = uni_x(g);
      int y = uni_y(g);
      bool subtract = add_or_subtract(g);
      int perturbation
          = subtract ? -perturbation_amount_ : perturbation_amount_;
      if (horizontal_capacities_[layer - 1] != 0) {
        int edge_cap
            = fastroute_->getEdgeCapacity(x - 1, y - 1, x, y - 1, layer);
        int new_h_capacity = (edge_cap + perturbation);
        new_h_capacity = new_h_capacity < 0 ? 0 : new_h_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, x, y - 1, layer, new_h_capacity, subtract);
      } else if (vertical_capacities_[layer - 1] != 0) {
        int edge_cap
            = fastroute_->getEdgeCapacity(x - 1, y - 1, x - 1, y, layer);
        int new_v_capacity = (edge_cap + perturbation);
        new_v_capacity = new_v_capacity < 0 ? 0 : new_v_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, x - 1, y, layer, new_v_capacity, subtract);
      }
    }
  }
}

void GlobalRouter::initGridAndNets()
{
  if (db_->getChip() == nullptr) {
    logger_->error(
        GRT, 170, "Load a design before running the global router commands.");
  }
  block_ = db_->getChip()->getBlock();
  routes_.clear();
  nets_to_route_.clear();
  for (auto [ignored, net] : db_net_map_) {
    delete net;
  }
  db_net_map_.clear();
  if (getMaxRoutingLayer() == -1) {
    setMaxRoutingLayer(computeMaxRoutingLayer());
  }
  if (routing_layers_.empty()) {
    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);

    initRoutingLayers(min_layer, max_layer);
    initRoutingTracks(max_layer);
    initCoreGrid(max_layer);
    setCapacities(min_layer, max_layer);
    applyAdjustments(min_layer, max_layer);
  }
  std::vector<Net*> nets = findNets(false);
  initNetlist(nets);
}

void GlobalRouter::configFastRoute()
{
  fastroute_->setVerbose(verbose_);
  fastroute_->setOverflowIterations(congestion_iterations_);
  fastroute_->setCongestionReportIterStep(congestion_report_iter_step_);
  fastroute_->setResistanceAware(resistance_aware_);

  if (congestion_file_name_ != nullptr) {
    fastroute_->setCongestionReportFile(congestion_file_name_);
  }

  if (sta_->getDbNetwork()->defaultLibertyLibrary() == nullptr) {
    logger_->warn(
        GRT,
        300,
        "Timing is not available, setting critical nets percentage to 0.");
    fastroute_->setCriticalNetsPercentage(0);
  }
}

void GlobalRouter::getMinMaxLayer(int& min_layer, int& max_layer)
{
  if (getMaxRoutingLayer() == -1) {
    setMaxRoutingLayer(computeMaxRoutingLayer());
  }
  min_layer = getMinLayerForClock() > 0
                  ? std::min(getMinRoutingLayer(), getMinLayerForClock())
                  : getMinRoutingLayer();
  max_layer = std::max(getMaxRoutingLayer(), getMaxLayerForClock());
}

void GlobalRouter::readGuides(const char* file_name)
{
  logger_->warn(GRT,
                8,
                "The read_guides command does not allow parasitics estimation "
                "from the guides file.");
  if (db_->getChip() == nullptr || db_->getChip()->getBlock() == nullptr
      || db_->getTech() == nullptr) {
    logger_->error(GRT, 249, "Load design before reading guides");
  }

  initGridAndNets();

  odb::dbTech* tech = db_->getTech();

  std::ifstream fin(file_name);
  std::string line;
  odb::dbNet* net = nullptr;
  std::unordered_map<odb::dbNet*, Guides> guides;

  if (!fin.is_open()) {
    logger_->error(GRT, 233, "Failed to open guide file {}.", file_name);
  }

  bool skip = false;
  std::string net_name;
  while (fin.good()) {
    getline(fin, line);
    if (line == "(" || line.empty() || line == ")") {
      continue;
    }

    std::stringstream ss(line);
    std::string word;
    std::vector<std::string> tokens;
    while (!ss.eof()) {
      ss >> word;
      tokens.push_back(word);
    }

    if (tokens.size() == 1) {
      net = block_->findNet(tokens[0].c_str());
      net_name = tokens[0];
      if (!net) {
        logger_->error(GRT, 234, "Cannot find net {}.", tokens[0]);
      }
      skip = false;
    } else if (tokens.size() == 5) {
      if (skip) {
        continue;
      }

      if (db_net_map_.find(net) == db_net_map_.end()) {
        logger_->warn(GRT,
                      250,
                      "Net {} has guides but is not routed by the global "
                      "router and will be skipped.",
                      net_name);
        skip = true;
        continue;
      }

      auto layer = tech->findLayer(tokens[4].c_str());
      if (!layer) {
        logger_->error(GRT, 235, "Cannot find layer {}.", tokens[4]);
      }

      odb::Rect rect(
          stoi(tokens[0]), stoi(tokens[1]), stoi(tokens[2]), stoi(tokens[3]));
      guides[net].emplace_back(layer->getRoutingLevel(), rect);
      int layer_idx = layer->getRoutingLevel();
      boxToGlobalRouting(rect, layer_idx, layer_idx, routes_[net]);
    } else {
      logger_->error(GRT, 236, "Error reading guide file {}.", file_name);
    }
  }

  updateVias();

  for (auto& net_route : routes_) {
    std::vector<Pin>& pins = db_net_map_[net_route.first]->getPins();
    GRoute& route = net_route.second;
    mergeSegments(pins, route);
  }

  updateEdgesUsage();
  computeGCellGridPatternFromGuides(guides);
  updateDbCongestionFromGuides();
  heatmap_->update();
  saveGuidesFromFile(guides);
}

void GlobalRouter::loadGuidesFromDB()
{
  if (!routes_.empty()) {
    return;
  }

  if (!haveDbGuides()) {
    return;
  }

  initGridAndNets();
  for (odb::dbNet* net : block_->getNets()) {
    for (odb::dbGuide* guide : net->getGuides()) {
      int layer_idx = guide->getLayer()->getRoutingLevel();
      int via_layer_idx = guide->getViaLayer()->getRoutingLevel();
      boxToGlobalRouting(
          guide->getBox(), layer_idx, via_layer_idx, routes_[net]);
      is_congested_ = is_congested_ || guide->isCongested();
    }
  }

  for (auto& net_route : routes_) {
    std::vector<Pin>& pins = db_net_map_[net_route.first]->getPins();
    GRoute& route = net_route.second;
    mergeSegments(pins, route);
  }

  for (auto& [db_net, groute] : routes_) {
    ensurePinsPositions(db_net);
  }

  updateEdgesUsage();
  if (block_->getGCellGrid() == nullptr) {
    updateDbCongestion();
  }
  heatmap_->update();
}

void GlobalRouter::ensurePinsPositions(odb::dbNet* db_net)
{
  std::string pins_not_covered;
  if (!netIsCovered(db_net, pins_not_covered)) {
    Net* net = db_net_map_[db_net];
    for (Pin& pin : net->getPins()) {
      if (pins_not_covered.find(pin.getName()) != std::string::npos
          && !findCoveredAccessPoint(net, pin)) {
        bool has_aps;
        odb::Point pos_on_grid;
        std::vector<RoutePt> pin_positions_on_grid
            = findOnGridPositions(pin, has_aps, pos_on_grid, true);
        computePinPositionOnGrid(
            pin_positions_on_grid, pin, pos_on_grid, has_aps);
      }
    }
  }
}

bool GlobalRouter::findCoveredAccessPoint(const Net* net, Pin& pin)
{
  std::map<int, std::vector<PointPair>> ap_positions;
  if (findPinAccessPointPositions(pin, ap_positions, true)) {
    const GRoute& segments = routes_[net->getDbNet()];
    for (const auto& [layer, aps] : ap_positions) {
      pin.setConnectionLayer(layer);
      for (const auto& point_pair : aps) {
        pin.setOnGridPosition(grid_->getPositionOnGrid(point_pair.second));
        for (const GSegment& seg : segments) {
          if (segmentCoversPin(seg, pin)) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

void GlobalRouter::updateVias()
{
  for (auto& net_route : routes_) {
    GRoute& route = net_route.second;
    if (route.empty()) {
      continue;
    }
    for (int i = 0; i < route.size() - 1; i++) {
      GSegment& seg1 = route[i];
      GSegment& seg2 = route[i + 1];

      odb::Point seg1_init(seg1.init_x, seg1.init_y);
      odb::Point seg1_final(seg1.final_x, seg1.final_y);
      odb::Point seg2_init(seg2.init_x, seg2.init_y);
      odb::Point seg2_final(seg2.final_x, seg2.final_y);

      // if a via segment is adjacent to the next wire segment, ensure
      // the via will connect to the segment
      if (seg1.isVia() && seg1.init_layer == seg2.init_layer - 1
          && (seg1_init == seg2_init || seg1_init == seg2_final)) {
        seg1.final_layer = seg2.init_layer;
      } else if (seg2.isVia() && seg2.init_layer == seg1.init_layer - 1
                 && (seg2_init == seg1_init || seg2_init == seg1_final)) {
        seg2.init_layer = seg1.final_layer;
      }
    }
  }
}

void GlobalRouter::updateEdgesUsage()
{
  for (const auto& [db_net, groute] : routes_) {
    if (isDetailedRouted(db_net)) {
      continue;
    }

    for (const GSegment& seg : groute) {
      int x0 = (seg.init_x - grid_->getXMin()) / grid_->getTileSize();
      int y0 = (seg.init_y - grid_->getYMin()) / grid_->getTileSize();
      int l0 = seg.init_layer;

      int x1 = (seg.final_x - grid_->getXMin()) / grid_->getTileSize();
      int y1 = (seg.final_y - grid_->getYMin()) / grid_->getTileSize();

      // The last gcell is oversized and includes space that the above
      // calculation doesn't represent so correct it:
      x1 = std::min(x1, grid_->getXGrids() - 1);
      y1 = std::min(y1, grid_->getYGrids() - 1);

      fastroute_->incrementEdge3DUsage(x0, y0, x1, y1, l0);
    }
  }
}

void GlobalRouter::updateDbCongestionFromGuides()
{
  auto block = db_->getChip()->getBlock();
  auto db_gcell = block->getGCellGrid();
  if (db_gcell == nullptr) {
    db_gcell = odb::dbGCellGrid::create(block);
  }

  auto db_tech = db_->getTech();
  for (int k = 0; k < grid_->getNumLayers(); k++) {
    auto layer = db_tech->findRoutingLayer(k + 1);
    if (layer == nullptr) {
      continue;
    }

    const auto& h_edges_3D = fastroute_->getHorizontalEdges3D();
    const auto& v_edges_3D = fastroute_->getVerticalEdges3D();
    const uint16_t capH = fastroute_->getHorizontalCapacities()[k];
    const uint16_t capV = fastroute_->getVerticalCapacities()[k];
    const uint16_t last_row_capH
        = fastroute_->getLastRowHorizontalCapacities()[k];
    const uint16_t last_col_capV
        = fastroute_->getLastColumnVerticalCapacities()[k];
    for (int y = 0; y < grid_->getYGrids(); y++) {
      for (int x = 0; x < grid_->getXGrids(); x++) {
        const uint16_t thisCapH
            = (y == grid_->getYGrids() - 1 ? last_row_capH : capH);
        const uint16_t thisCapV
            = (x == grid_->getXGrids() - 1 ? last_col_capV : capV);
        const uint16_t blockageH = thisCapH - h_edges_3D[k][y][x].cap;
        const uint16_t blockageV = thisCapV - v_edges_3D[k][y][x].cap;
        const uint16_t usageH = h_edges_3D[k][y][x].usage + blockageH;
        const uint16_t usageV = v_edges_3D[k][y][x].usage + blockageV;
        db_gcell->setCapacity(layer, x, y, thisCapH + thisCapV);
        db_gcell->setUsage(layer, x, y, usageH + usageV);
      }
    }
  }
}

void GlobalRouter::computeGCellGridPatternFromGuides(
    std::unordered_map<odb::dbNet*, Guides>& net_guides)
{
  int width = grid_->getXMax() - grid_->getXMin();
  int height = grid_->getYMax() - grid_->getYMin();

  // use the maps to detect the most used tile size. some designs may have
  // guides larger than others, but also smaller than others.
  std::map<int, int> tile_size_x_map;
  std::map<int, int> tile_size_y_map;
  int min_loc_x = std::numeric_limits<int>::max();
  int min_loc_y = std::numeric_limits<int>::max();
  fillTileSizeMaps(
      net_guides, tile_size_x_map, tile_size_y_map, min_loc_x, min_loc_y);

  int tile_size_x = 0;
  int tile_size_y = 0;
  findTileSize(tile_size_x_map, tile_size_y_map, tile_size_x, tile_size_y);

  if (tile_size_x == 0 || tile_size_y == 0) {
    logger_->error(utl::GRT,
                   253,
                   "Detected invalid guide dimensions: ({}, {}).",
                   tile_size_x,
                   tile_size_y);
  }

  int x_grids = width / tile_size_x;
  int guide_x_idx = std::floor((min_loc_x - grid_->getXMin()) / tile_size_x);
  int origin_x = min_loc_x - guide_x_idx * tile_size_x;

  int y_grids = height / tile_size_y;
  int guide_y_idx = std::floor((min_loc_y - grid_->getYMin()) / tile_size_y);
  int origin_y = min_loc_y - guide_y_idx * tile_size_y;

  auto db_gcell = block_->getGCellGrid();
  if (db_gcell) {
    db_gcell->resetGrid();
  } else {
    db_gcell = odb::dbGCellGrid::create(block_);
  }
  db_gcell->addGridPatternX(origin_x, x_grids, tile_size_x);
  db_gcell->addGridPatternY(origin_y, y_grids, tile_size_y);

  grid_->setXGrids(x_grids);
  grid_->setYGrids(y_grids);

  // update fastroute grid info with grid pattern calculated from guides
  fastroute_->setTileSize(std::min(tile_size_x, tile_size_y));
  fastroute_->setGridsAndLayers(
      grid_->getXGrids(), grid_->getYGrids(), grid_->getNumLayers());
  fastroute_->init3DEdges();
}

void GlobalRouter::fillTileSizeMaps(
    std::unordered_map<odb::dbNet*, Guides>& net_guides,
    std::map<int, int>& tile_size_x_map,
    std::map<int, int>& tile_size_y_map,
    int& min_loc_x,
    int& min_loc_y)
{
  for (const auto& [net, guides] : net_guides) {
    for (const auto& guide : guides) {
      if (tile_size_x_map.find(guide.second.dx()) == tile_size_x_map.end()) {
        tile_size_x_map[guide.second.dx()] = 1;
      } else {
        tile_size_x_map[guide.second.dx()]++;
      }
      if (tile_size_y_map.find(guide.second.dy()) == tile_size_y_map.end()) {
        tile_size_y_map[guide.second.dy()] = 1;
      } else {
        tile_size_y_map[guide.second.dy()]++;
      }

      min_loc_x = std::min(guide.second.xMin(), min_loc_x);
      min_loc_y = std::min(guide.second.yMin(), min_loc_y);
    }
  }
}

void GlobalRouter::findTileSize(const std::map<int, int>& tile_size_x_map,
                                const std::map<int, int>& tile_size_y_map,
                                int& tile_size_x,
                                int& tile_size_y)
{
  int cnt_x = 0;
  for (const auto& [size_x, count] : tile_size_x_map) {
    if (count > cnt_x) {
      tile_size_x = size_x;
      cnt_x = count;
    }
  }

  int cnt_y = 0;
  for (const auto& [size_y, count] : tile_size_y_map) {
    if (count > cnt_y) {
      tile_size_y = size_y;
      cnt_y = count;
    }
  }
}

void GlobalRouter::saveGuidesFromFile(
    std::unordered_map<odb::dbNet*, Guides>& guides)
{
  odb::dbTechLayer* ph_layer_final = nullptr;

  for (odb::dbNet* db_net : block_->getNets()) {
    db_net->clearGuides();
    const Guides& guide_boxes = guides[db_net];

    if (!guide_boxes.empty()) {
      for (const auto& guide : guide_boxes) {
        ph_layer_final = routing_layers_[guide.first];
        odb::dbGuide::create(
            db_net, ph_layer_final, ph_layer_final, guide.second, false);
      }
    }
  }
}

void GlobalRouter::saveGuides(const std::vector<odb::dbNet*>& nets)
{
  int offset_x = grid_origin_.x();
  int offset_y = grid_origin_.y();

  bool guide_is_congested = is_congested_ && !allow_congestion_;

  int net_with_jumpers, total_jumpers;
  net_with_jumpers = 0;
  total_jumpers = 0;
  for (odb::dbNet* db_net : nets) {
    auto iter = routes_.find(db_net);
    if (iter == routes_.end()) {
      continue;
    }
    Net* net = db_net_map_[db_net];
    GRoute& route = iter->second;
    RoutePointToPinsMap point_to_pins;
    if (!use_cugr_) {
      point_to_pins = findRoutePtPins(net);
    }

    int jumper_count = 0;
    if (!route.empty()) {
      db_net->clearGuides();
      for (GSegment& segment : route) {
        odb::Rect box = globalRoutingToBox(segment);
        box.moveDelta(offset_x, offset_y);
        if (segment.isVia()) {
          if (abs(segment.final_layer - segment.init_layer) > 1) {
            logger_->error(GRT,
                           75,
                           "Connection between non-adjacent layers in net {}.",
                           db_net->getConstName());
          }

          if (net->isLocal() || (isCoveringPin(net, segment))) {
            int layer_idx1 = segment.init_layer;
            int layer_idx2 = segment.final_layer;
            odb::dbTechLayer* layer1 = routing_layers_[layer_idx1];
            odb::dbTechLayer* layer2 = routing_layers_[layer_idx2];
            auto guide1 = odb::dbGuide::create(
                db_net, layer1, layer2, box, guide_is_congested);
            auto guide2 = odb::dbGuide::create(
                db_net, layer2, layer1, box, guide_is_congested);

            if (!use_cugr_) {
              RoutePt route_pt1(
                  segment.init_x, segment.init_y, segment.init_layer);
              RoutePt route_pt2(
                  segment.final_x, segment.final_y, segment.final_layer);
              addPinsConnectedToGuides(point_to_pins, route_pt1, guide1);
              addPinsConnectedToGuides(point_to_pins, route_pt2, guide2);
            }
          } else {
            int layer_idx = std::min(segment.init_layer, segment.final_layer);
            int via_layer_idx
                = std::max(segment.init_layer, segment.final_layer);
            odb::dbTechLayer* layer = routing_layers_[layer_idx];
            odb::dbTechLayer* via_layer = routing_layers_[via_layer_idx];
            auto guide = odb::dbGuide::create(
                db_net, layer, via_layer, box, guide_is_congested);

            if (!use_cugr_) {
              RoutePt route_pt1(
                  segment.init_x, segment.init_y, segment.init_layer);
              RoutePt route_pt2(
                  segment.final_x, segment.final_y, segment.final_layer);
              addPinsConnectedToGuides(point_to_pins, route_pt1, guide);
              addPinsConnectedToGuides(point_to_pins, route_pt2, guide);
            }
          }
        } else if (segment.init_layer == segment.final_layer) {
          if (segment.init_layer < getMinRoutingLayer()
              && segment.init_x != segment.final_x
              && segment.init_y != segment.final_y && !use_cugr_) {
            logger_->error(GRT,
                           74,
                           "Routing with guides in blocked metal for net {}.",
                           db_net->getConstName());
          }

          odb::dbTechLayer* layer = routing_layers_[segment.init_layer];
          // Set guide flag when it is jumper
          bool is_jumper = segment.isJumper();
          auto guide = odb::dbGuide::create(
              db_net, layer, layer, box, guide_is_congested);
          if (is_jumper) {
            guide->setIsJumper(true);
            jumper_count++;
          }

          if (!use_cugr_) {
            RoutePt route_pt1(
                segment.init_x, segment.init_y, segment.init_layer);
            RoutePt route_pt2(
                segment.final_x, segment.final_y, segment.final_layer);
            addPinsConnectedToGuides(point_to_pins, route_pt1, guide);
            addPinsConnectedToGuides(point_to_pins, route_pt2, guide);
          }
        }
      }
    }
    if (jumper_count) {
      total_jumpers += jumper_count;
      net_with_jumpers++;
    }
    auto dbGuides = db_net->getGuides();
    if (dbGuides.orderReversed() && dbGuides.reversible()) {
      dbGuides.reverse();
    }
  }
  debugPrint(logger_,
             GRT,
             "jumper_insertion",
             2,
             "Remaining jumpers {} in {} repaired nets after GRT",
             total_jumpers,
             net_with_jumpers);
}

RoutePointToPinsMap GlobalRouter::findRoutePtPins(Net* net)
{
  RoutePointToPinsMap route_pt_pins;
  for (Pin& pin : net->getPins()) {
    int conn_layer = pin.getConnectionLayer();
    odb::Point grid_pt = pin.getOnGridPosition();
    RoutePt route_pt(grid_pt.x(), grid_pt.y(), conn_layer);
    route_pt_pins[route_pt].pins.push_back(&pin);
  }
  return route_pt_pins;
}

void GlobalRouter::addPinsConnectedToGuides(RoutePointToPinsMap& point_to_pins,
                                            const RoutePt& route_pt,
                                            odb::dbGuide* guide)
{
  auto itr = point_to_pins.find(route_pt);
  if (itr != point_to_pins.end() && !itr->second.connected) {
    itr->second.connected = true;
    guide->setIsConnectedToTerm(true);
  }
}

void GlobalRouter::writeSegments(const char* file_name)
{
  std::ofstream segs_file;
  segs_file.open(file_name);
  if (!segs_file) {
    logger_->error(GRT, 255, "Global route segments file could not be opened.");
  }

  odb::dbTech* tech = db_->getTech();

  for (const auto [db_net, net] : db_net_map_) {
    auto iter = routes_.find(db_net);
    if (iter == routes_.end()) {
      continue;
    }
    const GRoute& route = iter->second;

    if (!route.empty()) {
      segs_file << net->getName() << "\n";
      segs_file << "(\n";
      for (const GSegment& segment : route) {
        odb::dbTechLayer* init_layer
            = tech->findRoutingLayer(segment.init_layer);
        odb::dbTechLayer* final_layer
            = tech->findRoutingLayer(segment.final_layer);
        segs_file << segment.init_x << " " << segment.init_y << " "
                  << init_layer->getName() << " " << segment.final_x << " "
                  << segment.final_y << " " << final_layer->getName() << "\n";
      }
      segs_file << ")\n";
    }
  }
  segs_file.close();
}

void GlobalRouter::readSegments(const char* file_name)
{
  if (db_->getChip() == nullptr || db_->getChip()->getBlock() == nullptr
      || db_->getTech() == nullptr) {
    logger_->error(GRT, 256, "Load design before reading guides");
  }

  initGridAndNets();

  odb::dbTech* tech = db_->getTech();

  std::ifstream fin(file_name);
  std::string line;
  odb::dbNet* db_net = nullptr;
  std::unordered_map<odb::dbNet*, Guides> guides;

  if (!fin.is_open()) {
    logger_->error(
        GRT, 257, "Failed to open global route segments file {}.", file_name);
  }

  int line_count = 0;
  while (fin.good()) {
    getline(fin, line);
    line_count++;
    if (line == "(" || line.empty() || line == ")") {
      continue;
    }

    std::stringstream ss(line);
    std::string word;
    std::vector<std::string> tokens;
    while (!ss.eof()) {
      ss >> word;
      tokens.push_back(word);
    }

    if (tokens.size() == 1) {
      db_net = block_->findNet(tokens[0].c_str());
      if (!db_net) {
        logger_->error(GRT, 258, "Cannot find net {}.", tokens[0]);
      }
    } else if (tokens.size() == 6) {
      auto layer1 = tech->findLayer(tokens[2].c_str());
      auto layer2 = tech->findLayer(tokens[5].c_str());
      if (!layer1) {
        logger_->error(GRT, 259, "Cannot find layer {}.", tokens[2]);
      }
      if (!layer2) {
        logger_->error(GRT, 260, "Cannot find layer {}.", tokens[5]);
      }
      GSegment segment(stoi(tokens[0]),
                       stoi(tokens[1]),
                       layer1->getRoutingLevel(),
                       stoi(tokens[3]),
                       stoi(tokens[4]),
                       layer2->getRoutingLevel());
      routes_[db_net].push_back(segment);
    } else {
      logger_->error(GRT,
                     261,
                     "Error reading global route segments file {} at line {}.\n"
                     "\t\t Line content: \"{}\".",
                     file_name,
                     line_count,
                     line);
    }
  }
  for (auto& [db_net, segments] : routes_) {
    if (!isConnected(db_net)) {
      logger_->error(
          GRT, 262, "Net {} has disconnected segments.", db_net->getName());
    }
    std::string pins_not_covered;
    if (!netIsCovered(db_net, pins_not_covered)) {
      logger_->error(GRT,
                     263,
                     "Pin(s) {}not covered in net {}.",
                     pins_not_covered,
                     db_net->getName());
    }
  }
  if (block_->getGCellGrid() == nullptr) {
    updateDbCongestion();
  }
}

bool GlobalRouter::netIsCovered(odb::dbNet* db_net,
                                std::string& pins_not_covered)
{
  bool net_is_covered = true;
  Net* net = db_net_map_[db_net];
  const GRoute& segments = routes_[db_net];
  for (const Pin& pin : net->getPins()) {
    bool pin_is_covered = false;
    for (const GSegment& seg : segments) {
      if (segmentCoversPin(seg, pin)) {
        pin_is_covered = true;
        break;
      }
    }
    if (!pin_is_covered) {
      pins_not_covered += pin.getName() + " ";
      net_is_covered = false;
    }
  }

  return net_is_covered;
}

// Checks if segment is a line, i.e. only varies in one dimension
// (vias are lines)
bool GlobalRouter::segmentIsLine(const GSegment& segment)
{
  int dimensionality = (segment.init_x != segment.final_x)
                       + (segment.init_y != segment.final_y)
                       + (segment.init_layer != segment.final_layer);
  return (dimensionality == 1);
}

bool GlobalRouter::segmentCoversPin(const GSegment& segment, const Pin& pin)
{
  auto [min_x, max_x] = std::minmax(segment.init_x, segment.final_x);
  auto [min_y, max_y] = std::minmax(segment.init_y, segment.final_y);
  auto [min_layer, max_layer]
      = std::minmax(segment.init_layer, segment.final_layer);
  return (pin.getOnGridPosition().getX() >= min_x
          && pin.getOnGridPosition().getX() <= max_x
          // Pin is vertically covered by the segment
          && pin.getOnGridPosition().getY() >= min_y
          && pin.getOnGridPosition().getY() <= max_y
          // Pin and segment share a layer
          && pin.getConnectionLayer() >= min_layer
          && pin.getConnectionLayer() <= max_layer);
}

// Builds the Net Graph in O(N²)
AdjacencyList GlobalRouter::buildNetGraph(odb::dbNet* net)
{
  const GRoute& segments = routes_[net];
  const int total_segments = segments.size();
  AdjacencyList graph(total_segments, std::vector<int>());
  for (int i = 0; i < total_segments; i++) {
    for (int j = i - 1; j >= 0; j--) {
      if (!segmentsConnect(segments[i], segments[j])) {
        continue;
      }
      graph[i].push_back(j);
      graph[j].push_back(i);
    }
  }
  return graph;
}

// Implements Union-Find algorithm to determine connectivity
bool GlobalRouter::isConnected(odb::dbNet* net)
{
  int total_segments = routes_[net].size();
  std::vector<int> parent(total_segments), rank(total_segments, 0);
  for (int i = 0; i < total_segments; i++) {
    parent[i] = i;
  }
  int initialized_groups = 1;

  if (!segmentIsLine(routes_[net][0])) {
    logger_->error(
        GRT,
        264,
        "Segment {} of net {} is not a horizontal/vertical line or via.",
        1,
        net->getName());
  }

  std::function<int(int)> find = [&](int x) -> int {
    if (parent[x] != x) {
      parent[x] = find(parent[x]);
    }
    return parent[x];
  };

  std::function<void(int, int)> uniteGroups = [&](int u, int v) {
    int root_u = find(u), root_v = find(v);
    if (rank[root_u] > rank[root_v]) {
      parent[root_v] = root_u;
    } else if (rank[root_u] < rank[root_v]) {
      parent[root_u] = root_v;
    } else {
      parent[root_v] = root_u;
      rank[root_u]++;
    }
  };

  for (int i = 1; i < total_segments; i++) {
    if (!segmentIsLine(routes_[net][i])) {
      logger_->error(
          GRT,
          265,
          "Segment {} of net {} is not a horizontal/vertical line or via.",
          i + 1,
          net->getName());
    }

    initialized_groups++;

    for (int j = i - 1; j >= 0 && initialized_groups > 1; --j) {
      if (segmentsConnect(routes_[net][i], routes_[net][j])) {
        uniteGroups(i, j);
        initialized_groups--;
      }
    }
  }
  return (initialized_groups == 1);
}

bool GlobalRouter::segmentsConnect(const GSegment& segment1,
                                   const GSegment& segment2)
{
  auto [s1_min_x, s1_max_x] = std::minmax(segment1.init_x, segment1.final_x);
  auto [s1_min_y, s1_max_y] = std::minmax(segment1.init_y, segment1.final_y);
  auto [s1_min_z, s1_max_z]
      = std::minmax(segment1.init_layer, segment1.final_layer);
  auto [s2_min_x, s2_max_x] = std::minmax(segment2.init_x, segment2.final_x);
  auto [s2_min_y, s2_max_y] = std::minmax(segment2.init_y, segment2.final_y);
  auto [s2_min_z, s2_max_z]
      = std::minmax(segment2.init_layer, segment2.final_layer);
  return (s1_max_x >= s2_min_x && s1_min_x <= s2_max_x)
         && (s1_max_y >= s2_min_y && s1_min_y <= s2_max_y)
         && (s1_max_z >= s2_min_z && s1_min_z <= s2_max_z);
}

bool GlobalRouter::isCoveringPin(Net* net, GSegment& segment)
{
  for (const auto& pin : net->getPins()) {
    int seg_top_layer = std::max(segment.final_layer, segment.init_layer);
    int seg_x = segment.final_x;
    int seg_y = segment.final_y;
    if (pin.getConnectionLayer() == seg_top_layer
        && pin.getOnGridPosition() == odb::Point(seg_x, seg_y)
        && (pin.isPort() || pin.isConnectedToPadOrMacro())) {
      return true;
    }
  }

  return false;
}

RoutingTracks GlobalRouter::getRoutingTracksByIndex(int layer)
{
  for (RoutingTracks routing_tracks : routing_tracks_) {
    if (routing_tracks.getLayerIndex() == layer) {
      return routing_tracks;
    }
  }
  return RoutingTracks();
}

void GlobalRouter::addGuidesForLocalNets(odb::dbNet* db_net,
                                         GRoute& route,
                                         int min_routing_layer,
                                         int max_routing_layer)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  int last_layer = -1;
  for (size_t p = 0; p < pins.size(); p++) {
    if (p > 0) {
      odb::Point pin_pos0 = pins[p - 1].getOnGridPosition();
      odb::Point pin_pos1 = pins[p].getOnGridPosition();
      // If the net is not local, FR core result is invalid
      if (pin_pos1.x() != pin_pos0.x() || pin_pos1.y() != pin_pos0.y()) {
        logger_->error(GRT,
                       76,
                       "Net {} does not have route guides.",
                       db_net->getConstName());
      }
    }

    last_layer = std::max(pins[p].getConnectionLayer(), last_layer);
  }

  // last_layer can be greater than max routing layer for nets with bumps
  // at top routing layer
  if (last_layer >= max_routing_layer) {
    last_layer--;
  }

  for (int l = 1; l <= last_layer; l++) {
    odb::Point pin_pos = pins[0].getOnGridPosition();
    GSegment segment = GSegment(
        pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
    route.push_back(segment);
  }
}

void GlobalRouter::connectTopLevelPins(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  for (Pin& pin : pins) {
    if (pin.getConnectionLayer() > getMaxRoutingLayer()) {
      odb::Point pin_pos = pin.getOnGridPosition();
      for (int l = getMaxRoutingLayer(); l < pin.getConnectionLayer(); l++) {
        GSegment segment = GSegment(
            pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
        route.push_back(segment);
      }
    }
  }
}

void GlobalRouter::addRemainingGuides(NetRouteMap& routes,
                                      std::vector<Net*>& nets,
                                      int min_routing_layer,
                                      int max_routing_layer)
{
  for (Net* net : nets) {
    int min_layer, max_layer;
    getNetLayerRange(net->getDbNet(), min_layer, max_layer);
    odb::dbTechLayer* max_tech_layer
        = db_->getTech()->findRoutingLayer(max_layer);
    if (net->getNumPins() > 1
        && (!net->hasWires() || net->hasStackedVias(max_tech_layer))) {
      odb::dbNet* db_net = net->getDbNet();
      GRoute& route = routes[db_net];
      if (route.empty()) {
        addGuidesForLocalNets(
            db_net, route, min_routing_layer, max_routing_layer);
      } else {
        connectTopLevelPins(db_net, route);
      }
    }
  }
}

void GlobalRouter::connectPadPins(NetRouteMap& routes)
{
  for (auto& net_route : routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    Net* net = getNet(db_net);
    if (pad_pins_connections_.find(db_net) != pad_pins_connections_.end()
        && net->getNumPins() > 1) {
      for (GSegment& segment : pad_pins_connections_[db_net]) {
        route.push_back(segment);
      }
    }
  }
}

odb::Rect GlobalRouter::globalRoutingToBox(const GSegment& route)
{
  odb::Rect die_bounds = grid_->getGridArea();

  const auto [init_x, final_x] = std::minmax(route.init_x, route.final_x);
  const auto [init_y, final_y] = std::minmax(route.init_y, route.final_y);

  int llX = init_x - (grid_->getTileSize() / 2);
  int llY = init_y - (grid_->getTileSize() / 2);
  int urX = final_x + (grid_->getTileSize() / 2);
  int urY = final_y + (grid_->getTileSize() / 2);

  if ((die_bounds.xMax() - urX) / grid_->getTileSize() < 1) {
    urX = die_bounds.xMax();
  }
  if ((die_bounds.yMax() - urY) / grid_->getTileSize() < 1) {
    urY = die_bounds.yMax();
  }

  odb::Point lower_left = odb::Point(llX, llY);
  odb::Point upper_right = odb::Point(urX, urY);

  odb::Rect route_bds = odb::Rect(lower_left, upper_right);
  return route_bds;
}

void GlobalRouter::boxToGlobalRouting(const odb::Rect& route_bds,
                                      int layer,
                                      int via_layer,
                                      GRoute& route)
{
  const int tile_size = grid_->getTileSize();
  int x0 = (tile_size * (route_bds.xMin() / tile_size)) + (tile_size / 2);
  int y0 = (tile_size * (route_bds.yMin() / tile_size)) + (tile_size / 2);

  const int x1 = (tile_size * (route_bds.xMax() / tile_size)) - (tile_size / 2);
  const int y1 = (tile_size * (route_bds.yMax() / tile_size)) - (tile_size / 2);

  if (x0 == x1 && y0 == y1) {
    route.emplace_back(x0, y0, layer, x1, y1, via_layer);
  }

  while (y0 == y1 && (x0 + tile_size) <= x1) {
    route.emplace_back(x0, y0, layer, x0 + tile_size, y0, layer);
    x0 += tile_size;
  }

  while (x0 == x1 && (y0 + tile_size) <= y1) {
    route.emplace_back(x0, y0, layer, x0, y0 + tile_size, layer);
    y0 += tile_size;
  }
}

void GlobalRouter::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<odb::Point>> layer_positions_map;

  odb::dbTechLayer* tech_layer;
  for (Pin* port : getAllPorts()) {
    if (port->getNumLayers() == 0) {
      logger_->error(GRT,
                     79,
                     "Pin {} does not have layer assignment.",
                     port->getName().c_str());
    }
    int layer = port->getConnectionLayer();  // port have only one layer

    tech_layer = routing_layers_[layer];
    if (layer_positions_map[layer].empty()) {
      layer_positions_map[layer].push_back(port->getPosition());
    } else {
      for (odb::Point& pos : layer_positions_map[layer]) {
        if (pos == port->getPosition()) {
          logger_->warn(
              GRT,
              31,
              "At least 2 pins in position ({}, {}), layer {}, port {}.",
              pos.x(),
              pos.y(),
              tech_layer->getName(),
              port->getName().c_str());
          invalid = true;
        }
      }
      layer_positions_map[layer].push_back(port->getPosition());
    }
  }

  if (invalid) {
    logger_->error(GRT, 80, "Invalid pin placement.");
  }
}

double GlobalRouter::dbuToMicrons(int dbu)
{
  return (double) dbu / db_->getDbuPerMicron();
}

float GlobalRouter::getLayerResistance(int layer,
                                       int length,
                                       odb::dbNet* db_net)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
  if (!tech_layer) {
    return 0;
  }

  int width = tech_layer->getWidth();
  double resistance = tech_layer->getResistance();

  // If net has NDR, get the correct width value
  odb::dbTechNonDefaultRule* ndr = db_net->getNonDefaultRule();
  if (ndr != nullptr) {
    odb::dbTechLayerRule* layerRule = ndr->getLayerRule(tech_layer);
    if (layerRule) {
      width = layerRule->getWidth();
    }
  }

  const float layer_width = dbuToMicrons(width);
  const float res_ohm_per_micron = resistance / layer_width;
  float final_resistance = res_ohm_per_micron * dbuToMicrons(length);

  return final_resistance;
}

float GlobalRouter::getViaResistance(int from_layer, int to_layer)
{
  if (abs(to_layer - from_layer) == 0) {
    return 0.0;
  }

  odb::dbTech* tech = db_->getTech();
  float total_via_resistance = 0.0;
  int start = std::min(from_layer, to_layer);
  int end = std::max(from_layer, to_layer);

  for (int i = start; i < end; i++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(i);
    odb::dbTechLayer* cut_layer = tech_layer->getUpperLayer();
    if (cut_layer) {
      double resistance = cut_layer->getResistance();
      total_via_resistance += resistance;
    }
  }

  return total_via_resistance;
}

float GlobalRouter::estimatePathResistance(odb::dbObject* pin1,
                                           odb::dbObject* pin2,
                                           bool verbose)
{
  odb::dbNet* db_net = nullptr;
  if (pin1->getObjectType() == odb::dbITermObj) {
    db_net = ((odb::dbITerm*) pin1)->getNet();
  } else if (pin1->getObjectType() == odb::dbBTermObj) {
    db_net = ((odb::dbBTerm*) pin1)->getNet();
  } else {
    logger_->error(GRT, 81, "Invalid pin type. Expected Iterm or Bterm.");
  }

  if (routes_.find(db_net) == routes_.end()) {
    logger_->error(
        GRT, 82, "Didn't find a route for net {}", db_net->getName());
  }

  std::vector<PinGridLocation> pin_locs = getPinGridPositions(db_net);
  PinGridLocation* start_loc = nullptr;
  PinGridLocation* end_loc = nullptr;
  odb::dbTech* tech = db_->getTech();

  for (auto& loc : pin_locs) {
    if (loc.iterm == pin1 || loc.bterm == pin1) {
      start_loc = &loc;
    } else if (loc.iterm == pin2 || loc.bterm == pin2) {
      end_loc = &loc;
    }
  }

  if (!start_loc || !end_loc) {
    logger_->error(GRT, 83, "There is no path between the two pins.");
  }

  if (verbose) {
    std::string pin1_name = (pin1->getObjectType() == odb::dbITermObj)
                                ? ((odb::dbITerm*) pin1)->getName()
                                : ((odb::dbBTerm*) pin1)->getName();
    std::string pin2_name = (pin2->getObjectType() == odb::dbITermObj)
                                ? ((odb::dbITerm*) pin2)->getName()
                                : ((odb::dbBTerm*) pin2)->getName();
    logger_->report(
        "Estimating Path Resistance between pin ({}) and pin ({}) through net "
        "({})",
        pin1_name,
        pin2_name,
        db_net->getConstName());
  }

  RoutePt start_pt(start_loc->grid_pt.getX(),
                   start_loc->grid_pt.getY(),
                   start_loc->conn_layer);
  RoutePt end_pt(
      end_loc->grid_pt.getX(), end_loc->grid_pt.getY(), end_loc->conn_layer);

  // Build graph
  std::map<RoutePt, std::vector<RoutePt>> adj;
  GRoute& route = routes_[db_net];

  for (const GSegment& seg : route) {
    RoutePt p1(seg.init_x, seg.init_y, seg.init_layer);
    RoutePt p2(seg.final_x, seg.final_y, seg.final_layer);
    adj[p1].push_back(p2);
    adj[p2].push_back(p1);
  }

  // BFS
  std::queue<RoutePt> q;
  q.push(start_pt);
  std::map<RoutePt, RoutePt> parent;
  std::set<RoutePt> visited;
  visited.insert(start_pt);
  bool found = false;

  while (!q.empty()) {
    RoutePt curr = q.front();
    q.pop();

    if (curr == end_pt) {
      found = true;
      break;
    }

    for (const RoutePt& neighbor : adj[curr]) {
      if (visited.find(neighbor) == visited.end()) {
        visited.insert(neighbor);
        parent[neighbor] = curr;
        q.push(neighbor);
      }
    }
  }

  if (!found) {
    return 0.0;
  }

  // Calculate resistance
  float total_resistance = 0.0;

  // TODO: Resistance from pin to grid

  // Path resistance
  RoutePt curr = end_pt;
  while (!(curr == start_pt)) {
    RoutePt prev = parent[curr];

    if (curr.layer() != prev.layer()) {
      // Via
      total_resistance += getViaResistance(prev.layer(), curr.layer());
      if (verbose) {
        logger_->report("Via resistance: {} - From {} to {} at ({}, {})",
                        getViaResistance(prev.layer(), curr.layer()),
                        tech->findRoutingLayer(curr.layer())->getConstName(),
                        tech->findRoutingLayer(prev.layer())->getConstName(),
                        curr.x(),
                        curr.y());
      }
    } else {
      // Wire
      int length = abs(curr.x() - prev.x()) + abs(curr.y() - prev.y());
      total_resistance += getLayerResistance(curr.layer(), length, db_net);
      if (verbose) {
        logger_->report(
            "Wire resistance: {} - From ({}, {}) - To ({}, {}) - Layer {}",
            getLayerResistance(curr.layer(), length, db_net),
            prev.x(),
            prev.y(),
            curr.x(),
            curr.y(),
            tech->findRoutingLayer(curr.layer())->getConstName());
      }
    }
    curr = prev;
  }

  if (verbose) {
    logger_->report("Total Resistance: {} ohms", total_resistance);
  }

  return total_resistance;
}

// Estimate Path Resistance between two pins considering the vertical and
// horizontal metal layers defined by the user
float GlobalRouter::estimatePathResistance(odb::dbObject* pin1,
                                           odb::dbObject* pin2,
                                           odb::dbTechLayer* layer1,
                                           odb::dbTechLayer* layer2,
                                           bool verbose)
{
  odb::dbNet* db_net = nullptr;
  if (pin1->getObjectType() == odb::dbITermObj) {
    db_net = ((odb::dbITerm*) pin1)->getNet();
  } else if (pin1->getObjectType() == odb::dbBTermObj) {
    db_net = ((odb::dbBTerm*) pin1)->getNet();
  } else {
    logger_->error(GRT, 87, "Invalid pin type. Expected Iterm or Bterm.");
  }

  std::vector<PinGridLocation> pin_locs = getPinGridPositions(db_net);
  PinGridLocation* start_loc = nullptr;
  PinGridLocation* end_loc = nullptr;
  odb::dbTech* tech = db_->getTech();

  for (auto& loc : pin_locs) {
    if (loc.iterm == pin1 || loc.bterm == pin1) {
      start_loc = &loc;
    } else if (loc.iterm == pin2 || loc.bterm == pin2) {
      end_loc = &loc;
    }
  }

  if (!start_loc || !end_loc) {
    logger_->error(GRT, 89, "There is no path between the two pins.");
  }

  if (verbose) {
    std::string pin1_name = (pin1->getObjectType() == odb::dbITermObj)
                                ? ((odb::dbITerm*) pin1)->getName()
                                : ((odb::dbBTerm*) pin1)->getName();
    std::string pin2_name = (pin2->getObjectType() == odb::dbITermObj)
                                ? ((odb::dbITerm*) pin2)->getName()
                                : ((odb::dbBTerm*) pin2)->getName();
    logger_->report(
        "Estimating Path Resistance between pin ({}) and pin ({}) using layers "
        "{} and {}",
        pin1_name,
        pin2_name,
        layer1->getName(),
        layer2->getName());
  }

  odb::dbTechLayer* h_layer = nullptr;
  odb::dbTechLayer* v_layer = nullptr;

  if (layer1->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    h_layer = layer1;
  } else if (layer1->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    v_layer = layer1;
  }

  if (layer2->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    h_layer = layer2;
  } else if (layer2->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    v_layer = layer2;
  }

  if (!h_layer || !v_layer) {
    logger_->error(
        GRT,
        91,
        "Could not identify horizontal and vertical layers from {} and "
        "{}. Please provide one horizontal and one vertical layer.",
        layer1->getName(),
        layer2->getName());
  }

  RoutePt start_pt(start_loc->grid_pt.getX(),
                   start_loc->grid_pt.getY(),
                   start_loc->conn_layer);
  RoutePt end_pt(
      end_loc->grid_pt.getX(), end_loc->grid_pt.getY(), end_loc->conn_layer);

  // Build graph
  std::map<RoutePt, std::vector<RoutePt>> adj;
  GRoute& route = routes_[db_net];

  for (const GSegment& seg : route) {
    RoutePt p1(seg.init_x, seg.init_y, seg.init_layer);
    RoutePt p2(seg.final_x, seg.final_y, seg.final_layer);
    adj[p1].push_back(p2);
    adj[p2].push_back(p1);
  }

  // BFS
  std::queue<RoutePt> q;
  q.push(start_pt);
  std::map<RoutePt, RoutePt> parent;
  std::set<RoutePt> visited;
  visited.insert(start_pt);
  bool found = false;

  while (!q.empty()) {
    RoutePt curr = q.front();
    q.pop();

    if (curr == end_pt) {
      found = true;
      break;
    }

    for (const RoutePt& neighbor : adj[curr]) {
      if (visited.find(neighbor) == visited.end()) {
        visited.insert(neighbor);
        parent[neighbor] = curr;
        q.push(neighbor);
      }
    }
  }

  if (!found) {
    return 0.0;
  }

  // Calculate resistance
  float total_resistance = 0.0;

  // TODO: Resistance from pin to grid

  // Calculate via resistance between the two user layers
  float user_via_res = getViaResistance(h_layer->getRoutingLevel(),
                                        v_layer->getRoutingLevel());

  // Reconstruct path from end to start
  std::vector<RoutePt> path;
  RoutePt curr = end_pt;
  path.push_back(curr);
  while (!(curr == start_pt)) {
    curr = parent[curr];
    path.push_back(curr);
  }
  std::ranges::reverse(path.begin(), path.end());

  // Filter out vias (points with same x,y as previous)
  std::vector<RoutePt> clean_path;
  if (!path.empty()) {
    clean_path.push_back(path[0]);
    for (size_t i = 1; i < path.size(); ++i) {
      if (path[i].x() != clean_path.back().x()
          || path[i].y() != clean_path.back().y()) {
        clean_path.push_back(path[i]);
      }
    }
  }

  // Process segments
  for (size_t i = 1; i < clean_path.size(); ++i) {
    RoutePt p0 = clean_path[i - 1];
    RoutePt p1 = clean_path[i];

    int length = abs(p1.x() - p0.x()) + abs(p1.y() - p0.y());
    bool is_horizontal = (p1.y() == p0.y());
    int mapped_layer = is_horizontal ? h_layer->getRoutingLevel()
                                     : v_layer->getRoutingLevel();

    // Start via
    if (i == 1) {
      int start_layer_id = is_horizontal ? h_layer->getRoutingLevel()
                                         : v_layer->getRoutingLevel();
      total_resistance
          += getViaResistance(start_loc->conn_layer, start_layer_id);
      if (verbose) {
        logger_->report(
            "Via resistance (Start): {} - From Layer {} to {}",
            getViaResistance(start_loc->conn_layer, start_layer_id),
            tech->findRoutingLayer(start_loc->conn_layer)->getConstName(),
            tech->findRoutingLayer(start_layer_id)->getConstName());
      }
    }

    // Check for orientation change (corner)
    if (i > 1) {
      RoutePt p_prev = clean_path[i - 2];
      bool prev_is_horizontal = (p0.y() == p_prev.y());
      if (prev_is_horizontal != is_horizontal) {
        total_resistance += user_via_res;
        if (verbose) {
          logger_->report("Via resistance (Corner): {} - Between {} and {}",
                          user_via_res,
                          h_layer->getConstName(),
                          v_layer->getConstName());
        }
      }
    }

    float wire_res = getLayerResistance(mapped_layer, length, db_net);
    total_resistance += wire_res;
    if (verbose && wire_res > 0) {
      logger_->report(
          "Wire resistance: {} - From ({}, {}) - To ({}, {}) - Mapped Layer {}",
          wire_res,
          p0.x(),
          p0.y(),
          p1.x(),
          p1.y(),
          tech->findRoutingLayer(mapped_layer)->getConstName());
    }

    // End via
    if (i == clean_path.size() - 1) {
      int end_layer_id = is_horizontal ? h_layer->getRoutingLevel()
                                       : v_layer->getRoutingLevel();
      total_resistance += getViaResistance(end_loc->conn_layer, end_layer_id);
      if (verbose) {
        logger_->report(
            "Via resistance (End): {} - From Layer {} to {}",
            getViaResistance(end_loc->conn_layer, end_layer_id),
            tech->findRoutingLayer(end_loc->conn_layer)->getConstName(),
            tech->findRoutingLayer(end_layer_id)->getConstName());
      }
    }
  }

  if (verbose) {
    logger_->report("Total Resistance: {} ohms", total_resistance);
  }

  return total_resistance;
}

int GlobalRouter::computeNetWirelength(odb::dbNet* db_net)
{
  auto iter = routes_.find(db_net);
  if (iter == routes_.end()) {
    return 0;
  }
  const GRoute& route = iter->second;
  int net_wl = 0;
  for (const GSegment& segment : route) {
    const int segment_wl = std::abs(segment.final_x - segment.init_x)
                           + std::abs(segment.final_y - segment.init_y);
    if (segment_wl > 0) {
      net_wl += segment_wl + grid_->getTileSize();
    }
  }

  return net_wl;
}

void GlobalRouter::computeWirelength()
{
  int64_t total_wirelength = 0;
  for (auto& net_route : routes_) {
    total_wirelength += computeNetWirelength(net_route.first);
  }
  logger_->metric("global_route__wirelength",
                  total_wirelength / block_->getDefUnits());
  if (verbose_) {
    logger_->info(GRT,
                  18,
                  "Total wirelength: {} um",
                  total_wirelength / block_->getDefUnits());
  }
}

void GlobalRouter::mergeSegments(const std::vector<Pin>& pins, GRoute& route)
{
  if (route.empty()) {
    return;
  }
  GRoute& segments = route;
  std::map<RoutePt, int> segs_at_point;
  for (const GSegment& seg : segments) {
    RoutePt pt0 = RoutePt(seg.init_x, seg.init_y, seg.init_layer);
    RoutePt pt1 = RoutePt(seg.final_x, seg.final_y, seg.final_layer);
    segs_at_point[pt0] += 1;
    segs_at_point[pt1] += 1;
  }

  for (const Pin& pin : pins) {
    RoutePt pinPt = RoutePt(pin.getOnGridPosition().x(),
                            pin.getOnGridPosition().y(),
                            pin.getConnectionLayer());
    segs_at_point[pinPt] += 1;
  }

  size_t read = 0;
  size_t write = 0;

  while (read < segments.size() - 1) {
    GSegment& segment0 = segments[read];
    GSegment& segment1 = segments[read + 1];

    // both segments are not vias
    if (segment0.init_layer == segment0.final_layer
        && segment1.init_layer == segment1.final_layer
        // segments are on the same layer
        && segment0.init_layer == segment1.init_layer
        // prevent merging guides below the min routing layer (guides for
        // pin access)
        && segment0.init_layer >= getMinRoutingLayer()) {
      // if segment 0 connects to the end of segment 1
      if (!segmentsConnect(segment0, segment1, segment1, segs_at_point)) {
        segments[write++] = segment0;
      }
    } else {
      segments[write++] = segment0;
    }
    read++;
  }

  segments[write] = segments[read];

  segments.resize(write + 1);
}

bool GlobalRouter::segmentsConnect(const GSegment& seg0,
                                   const GSegment& seg1,
                                   GSegment& new_seg,
                                   const std::map<RoutePt, int>& segs_at_point)
{
  int init_x0 = std::min(seg0.init_x, seg0.final_x);
  int init_y0 = std::min(seg0.init_y, seg0.final_y);
  int final_x0 = std::max(seg0.final_x, seg0.init_x);
  int final_y0 = std::max(seg0.final_y, seg0.init_y);

  int init_x1 = std::min(seg1.init_x, seg1.final_x);
  int init_y1 = std::min(seg1.init_y, seg1.final_y);
  int final_x1 = std::max(seg1.final_x, seg1.init_x);
  int final_y1 = std::max(seg1.final_y, seg1.init_y);

  // vertical segments aligned
  if (init_x0 == final_x0 && init_x1 == final_x1 && init_x0 == init_x1) {
    bool merge = false;
    if (init_y0 == final_y1) {
      RoutePt pt = RoutePt(init_x0, init_y0, seg0.init_layer);
      merge = segs_at_point.at(pt) == 2;
    } else if (final_y0 == init_y1) {
      RoutePt pt = RoutePt(init_x1, init_y1, seg1.init_layer);
      merge = segs_at_point.at(pt) == 2;
    }
    if (merge) {
      new_seg.init_x = std::min(init_x0, init_x1);
      new_seg.init_y = std::min(init_y0, init_y1);
      new_seg.final_x = std::max(final_x0, final_x1);
      new_seg.final_y = std::max(final_y0, final_y1);
      return true;
    }
    // horizontal segments aligned
  } else if (init_y0 == final_y0 && init_y1 == final_y1 && init_y0 == init_y1) {
    bool merge = false;
    if (init_x0 == final_x1) {
      RoutePt pt = RoutePt(init_x0, init_y0, seg0.init_layer);
      merge = segs_at_point.at(pt) == 2;
    } else if (final_x0 == init_x1) {
      RoutePt pt = RoutePt(init_x1, init_y1, seg1.init_layer);
      merge = segs_at_point.at(pt) == 2;
    }
    if (merge) {
      new_seg.init_x = std::min(init_x0, init_x1);
      new_seg.init_y = std::min(init_y0, init_y1);
      new_seg.final_x = std::max(final_x0, final_x1);
      new_seg.final_y = std::max(final_y0, final_y1);
      return true;
    }
  }

  return false;
}

void GlobalRouter::mergeResults(NetRouteMap& routes)
{
  for (auto& net_route : routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    routes_[db_net] = route;
  }
}

bool GlobalRouter::pinOverlapsWithSingleTrack(const Pin& pin,
                                              odb::Point& track_position)
{
  int min, max;

  int conn_layer = pin.getConnectionLayer();
  std::vector<odb::Rect> pin_boxes = pin.getBoxes().at(conn_layer);

  odb::dbTechLayer* layer = routing_layers_[conn_layer];
  RoutingTracks tracks = getRoutingTracksByIndex(conn_layer);

  odb::Rect pin_rect;
  pin_rect.mergeInit();
  for (odb::Rect pin_box : pin_boxes) {
    pin_rect.merge(pin_box);
  }

  bool horizontal = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  min = horizontal ? pin_rect.yMin() : pin_rect.xMin();
  max = horizontal ? pin_rect.yMax() : pin_rect.xMax();

  if ((float) (max - min) / tracks.getTrackPitch() <= 3) {
    int nearest_track = std::floor((float) (max - tracks.getLocation())
                                   / tracks.getTrackPitch())
                            * tracks.getTrackPitch()
                        + tracks.getLocation();
    int nearest_track2
        = std::floor(
              (float) (max - tracks.getLocation()) / tracks.getTrackPitch() - 1)
              * tracks.getTrackPitch()
          + tracks.getLocation();

    if ((nearest_track >= min && nearest_track <= max)
        && (nearest_track2 >= min && nearest_track2 <= max)) {
      return false;
    }

    if (nearest_track > min && nearest_track < max) {
      track_position = horizontal
                           ? odb::Point(track_position.x(), nearest_track)
                           : odb::Point(nearest_track, track_position.y());
      return true;
    }
    if (nearest_track2 > min && nearest_track2 < max) {
      track_position = horizontal
                           ? odb::Point(track_position.x(), nearest_track2)
                           : odb::Point(nearest_track2, track_position.y());
      return true;
    }
    return false;
  }

  return false;
}

std::vector<Pin*> GlobalRouter::getAllPorts()
{
  std::vector<Pin*> ports;
  for (auto [ignored, net] : db_net_map_) {
    for (Pin& pin : net->getPins()) {
      if (pin.isPort()) {
        ports.push_back(&pin);
      }
    }
  }
  return ports;
}

odb::Point GlobalRouter::getRectMiddle(const odb::Rect& rect)
{
  return odb::Point((rect.xMin() + (rect.xMax() - rect.xMin()) / 2.0),
                    (rect.yMin() + (rect.yMax() - rect.yMin()) / 2.0));
}

void GlobalRouter::initGrid(int max_layer)
{
  odb::Rect rect = block_->getDieArea();

  int dx = rect.dx();
  int dy = rect.dy();

  int tile_size = block_->getGCellTileSize();

  int x_grids = std::max(1, dx / tile_size);
  int y_grids = std::max(1, dy / tile_size);

  bool perfect_regular_x = (x_grids * tile_size) == dx;
  bool perfect_regular_y = (y_grids * tile_size) == dy;

  fastroute_->setRegularX(perfect_regular_x);
  fastroute_->setRegularY(perfect_regular_y);

  int num_layers = routing_layers_.size();
  if (max_layer > -1) {
    num_layers = max_layer;
  }

  grid_->init(rect,
              tile_size,
              x_grids,
              y_grids,
              perfect_regular_x,
              perfect_regular_y,
              num_layers);
}

static void getViaDims(
    std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias,
    odb::dbTechLayer* tech_layer,
    odb::dbTechLayer* bottom_layer,
    int& width_up,
    int& prl_up,
    int& width_down,
    int& prl_down)
{
  width_up = -1;
  prl_up = -1;
  width_down = -1;
  prl_down = -1;
  if (default_vias.find(tech_layer) != default_vias.end()) {
    for (auto box : default_vias[tech_layer]->getBoxes()) {
      if (box->getTechLayer() == tech_layer) {
        width_up = std::min(box->getDX(), box->getDY());
        prl_up = std::max(box->getDX(), box->getDY());
        break;
      }
    }
  }
  if (default_vias.find(bottom_layer) != default_vias.end()) {
    for (auto box : default_vias[bottom_layer]->getBoxes()) {
      if (box->getTechLayer() == tech_layer) {
        width_down = std::min(box->getDX(), box->getDY());
        prl_down = std::max(box->getDX(), box->getDY());
        break;
      }
    }
  }
}

std::vector<std::pair<int, int>> GlobalRouter::calcLayerPitches(int max_layer)
{
  std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias
      = block_->getDefaultVias();
  odb::dbTech* tech = db_->getTech();
  std::vector<std::pair<int, int>> pitches(tech->getRoutingLayerCount() + 1);
  for (auto const& [level, layer] : routing_layers_) {
    if (layer->getType() != odb::dbTechLayerType::ROUTING) {
      continue;
    }
    if (level > max_layer && max_layer > -1) {
      break;
    }
    pitches.emplace_back(-1, -1);

    int width_up, prl_up, width_down, prl_down;
    odb::dbTechLayer* bottom_layer
        = tech->findRoutingLayer(layer->getRoutingLevel() - 1);
    getViaDims(default_vias,
               layer,
               bottom_layer,
               width_up,
               prl_up,
               width_down,
               prl_down);
    bool up_via_valid = width_up != -1;
    bool down_via_valid = width_down != -1;
    if (!up_via_valid && !down_via_valid) {
      continue;  // no default vias found
    }
    int layer_width = layer->getWidth();
    int L2V_up = -1;
    int L2V_down = -1;
    // Priority for minSpc rule is SPACINGTABLE TWOWIDTHS > SPACINGTABLE PRL
    // > SPACING
    bool min_spc_valid = false;
    int min_spc_up = -1;
    int min_spc_down = -1;
    if (layer->hasTwoWidthsSpacingRules()) {
      min_spc_valid = true;
      if (up_via_valid) {
        min_spc_up = layer->findTwSpacing(layer_width, width_up, prl_up);
      }
      if (down_via_valid) {
        min_spc_down = layer->findTwSpacing(layer_width, width_down, prl_down);
      }
    } else if (layer->hasV55SpacingRules()) {
      min_spc_valid = true;
      if (up_via_valid) {
        min_spc_up
            = layer->findV55Spacing(std::max(layer_width, width_up), prl_up);
      }
      if (down_via_valid) {
        min_spc_down = layer->findV55Spacing(std::max(layer_width, width_down),
                                             prl_down);
      }
    } else {
      if (!layer->getV54SpacingRules().empty()) {
        min_spc_valid = true;
        int minSpc = 0;
        for (auto rule : layer->getV54SpacingRules()) {
          if (rule->hasRange()) {
            uint32_t rmin;
            uint32_t rmax;
            rule->getRange(rmin, rmax);
            if (layer_width < rmin || layer_width > rmax) {
              continue;
            }
          }
          minSpc = std::max<int>(minSpc, rule->getSpacing());
        }
        if (up_via_valid) {
          min_spc_up = minSpc;
        }
        if (down_via_valid) {
          min_spc_down = minSpc;
        }
      }
    }
    if (min_spc_valid) {
      if (up_via_valid) {
        L2V_up = (level != getMaxRoutingLayer())
                     ? (layer_width / 2) + (width_up / 2) + min_spc_up
                     : -1;
      }
      if (down_via_valid) {
        L2V_down = (level != getMinRoutingLayer())
                       ? (layer_width / 2) + (width_down / 2) + min_spc_down
                       : -1;
      }
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "routing level {} : layer_width = {:.4f}",
                 layer->getName(),
                 block_->dbuToMicrons(layer_width));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_up : viaWidth = {:.4f} , prl = {:.4f} , minSpc = {:.4f} "
                 ", L2V = {:.4f} ",
                 block_->dbuToMicrons(width_up),
                 block_->dbuToMicrons(prl_up),
                 block_->dbuToMicrons(min_spc_up),
                 block_->dbuToMicrons(L2V_up));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_down : viaWidth = {:.4f} , prl = {:.4f} , minSpc = "
                 "{:.4f} , L2V = {:.4f} ",
                 block_->dbuToMicrons(width_down),
                 block_->dbuToMicrons(prl_down),
                 block_->dbuToMicrons(min_spc_down),
                 block_->dbuToMicrons(L2V_down));
    }
    pitches[level] = {L2V_up, L2V_down};
  }
  return pitches;
}

void GlobalRouter::initRoutingTracks(int max_routing_layer)
{
  auto l2vPitches = calcLayerPitches(max_routing_layer);
  for (auto const& [level, tech_layer] : routing_layers_) {
    if (level > max_routing_layer && max_routing_layer > -1) {
      break;
    }

    odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
    if (track_grid == nullptr) {
      logger_->error(
          GRT, 86, "Track for layer {} not found.", tech_layer->getName());
    }

    int track_step, track_init, num_tracks;
    track_grid->getAverageTrackSpacing(track_step, track_init, num_tracks);

    RoutingTracks layer_tracks = RoutingTracks(level,
                                               track_step,
                                               l2vPitches[level].first,
                                               l2vPitches[level].second,
                                               track_init,
                                               num_tracks);
    routing_tracks_.push_back(layer_tracks);
    if (verbose_) {
      logger_->info(
          GRT,
          88,
          "Layer {:7s} Track-Pitch = {:.4f}  line-2-Via Pitch: {:.4f}",
          tech_layer->getName(),
          block_->dbuToMicrons(layer_tracks.getTrackPitch()),
          block_->dbuToMicrons(layer_tracks.getLineToViaPitch()));
    }
  }
}

void GlobalRouter::findTrackPitches(int max_layer)
{
  for (auto const& [level, tech_layer] : routing_layers_) {
    if (level > max_layer && max_layer > -1) {
      break;
    }

    odb::dbTrackGrid* track = block_->findTrackGrid(tech_layer);

    if (track == nullptr) {
      logger_->error(
          GRT, 90, "Track for layer {} not found.", tech_layer->getName());
    }

    int track_step, track_init, num_tracks;
    track->getAverageTrackSpacing(track_step, track_init, num_tracks);

    grid_->addTrackPitch(track_step, level - 1);
  }
}

static bool nameLess(const Net* a, const Net* b)
{
  return a->getName() < b->getName();
}

std::vector<Net*> GlobalRouter::findNets(bool init_clock_nets)
{
  if (init_clock_nets) {
    initClockNets();
  }

  std::vector<odb::dbNet*> db_nets;
  if (nets_to_route_.empty()) {
    db_nets.insert(
        db_nets.end(), block_->getNets().begin(), block_->getNets().end());
  } else {
    db_nets = nets_to_route_;
  }
  std::vector<Net*> clk_nets;
  const int large_fanout_threshold = 1000;
  for (odb::dbNet* db_net : db_nets) {
    const bool is_special
        = db_net->getSigType().isSupply() && db_net->isSpecial();
    if (!is_special && db_net->getTermCount() > skip_large_fanout_) {
      logger_->info(GRT,
                    280,
                    "Skipping net {} with {} terminals.",
                    db_net->getConstName(),
                    db_net->getTermCount(),
                    skip_large_fanout_);
      continue;
    }

    if (!is_special && db_net->getTermCount() > large_fanout_threshold) {
      logger_->warn(GRT,
                    281,
                    "Net {} has a large fanout of {} terminals.",
                    db_net->getConstName(),
                    db_net->getTermCount());
    }

    Net* net = addNet(db_net);
    // add clock nets not connected to a leaf first
    if (net) {
      bool is_non_leaf_clock = isNonLeafClock(net->getDbNet());
      if (is_non_leaf_clock) {
        net->setIsClockNet(true);
        clk_nets.push_back(net);
      }
    }
  }

  std::vector<Net*> non_clk_nets;
  for (auto [ignored, net] : db_net_map_) {
    bool is_non_leaf_clock = isNonLeafClock(net->getDbNet());
    if (!is_non_leaf_clock) {
      non_clk_nets.push_back(net);
    }
  }

  // Sort the nets to ensure stable results, but keep clk nets
  // at the front.
  std::ranges::sort(clk_nets, nameLess);
  std::ranges::sort(non_clk_nets, nameLess);

  std::vector<Net*> nets = std::move(clk_nets);
  nets.insert(nets.end(), non_clk_nets.begin(), non_clk_nets.end());

  return nets;
}

void GlobalRouter::findClockNets(const std::vector<Net*>& nets,
                                 std::set<odb::dbNet*>& clock_nets)
{
  for (Net* net : nets) {
    if (net->isClockNet()) {
      clock_nets.insert(net->getDbNet());
    }
  }
}

Net* GlobalRouter::addNet(odb::dbNet* db_net)
{
  if (!db_net->getSigType().isSupply() && !db_net->isSpecial()
      && db_net->getSWires().empty() && !db_net->isConnectedByAbutment()) {
    Net* net = new Net(db_net, db_net->getWire() != nullptr);
    if (db_net_map_.find(db_net) != db_net_map_.end()) {
      delete db_net_map_[db_net];
    }
    db_net_map_[db_net] = net;
    updateNetPins(net);
    return net;
  }
  return nullptr;
}

void GlobalRouter::removeNet(odb::dbNet* db_net)
{
  Net* net = db_net_map_[db_net];
  if (net != nullptr && net->isMergedNet()) {
    fastroute_->mergeNet(db_net, net->getMergedNet());
  } else {
    fastroute_->removeNet(db_net);
  }
  delete net;
  db_net_map_.erase(db_net);
  dirty_nets_.erase(db_net);
  routes_.erase(db_net);
}

void GlobalRouter::updateNetPins(Net* net)
{
  odb::dbNet* db_net = net->getDbNet();
  net->destroyPins();
  makeItermPins(net, db_net, grid_->getGridArea());
  makeBtermPins(net, db_net, grid_->getGridArea());
  findPins(net);
}

Net* GlobalRouter::getNet(odb::dbNet* db_net)
{
  return db_net_map_[db_net];
}

int GlobalRouter::getTileSize() const
{
  return grid_->getTileSize();
}

void GlobalRouter::initClockNets()
{
  std::set<odb::dbNet*> clock_nets;

  auto db_network = sta_->getDbNetwork();
  if (db_network != nullptr && db_network->isLinked()
      && db_network->defaultLibertyLibrary() != nullptr) {
    clock_nets = sta_->findClkNets();
  }

  if (verbose_) {
    logger_->info(GRT, 19, "Found {} clock nets.", clock_nets.size());
  }

  for (odb::dbNet* net : clock_nets) {
    net->setSigType(odb::dbSigType::CLOCK);
  }
}

bool GlobalRouter::isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network)
{
  const sta::Pin* pin = network->dbToSta(iterm);
  sta::LibertyPort* lib_port = network->libertyPort(pin);
  bool connected_to_pad = false;
  if (lib_port != nullptr) {
    sta::LibertyCell* lib_cell = lib_port->libertyCell();
    connected_to_pad = lib_cell != nullptr && lib_cell->isPad();
  }

  return lib_port && (lib_port->isRegClk() || connected_to_pad);
}

bool GlobalRouter::isNonLeafClock(odb::dbNet* db_net)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (db_net->getSigType() != odb::dbSigType::CLOCK) {
    return false;
  }

  for (odb::dbITerm* iterm : db_net->getITerms()) {
    if (isClkTerm(iterm, network)) {
      return false;
    }
  }
  return true;
}

void GlobalRouter::makeItermPins(Net* net,
                                 odb::dbNet* db_net,
                                 const odb::Rect& die_area)
{
  bool is_clock = (net->getSignalType() == odb::dbSigType::CLOCK);
  int max_routing_layer = (is_clock && getMaxLayerForClock() > 0)
                              ? getMaxLayerForClock()
                              : getMaxRoutingLayer();
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    odb::dbMaster* master = mterm->getMaster();
    const odb::dbMasterType type = master->getType();

    if (type.isCover() && verbose_) {
      logger_->warn(
          GRT,
          34,
          "Net connected to instance of class COVER added for routing.");
    }

    const bool connected_to_pad = type.isPad();
    const bool connected_to_macro = master->isBlock();

    odb::dbInst* inst = iterm->getInst();
    if (!inst->isPlaced()) {
      logger_->error(GRT, 10, "Instance {} is not placed.", inst->getName());
    }
    const odb::dbTransform transform = inst->getTransform();

    odb::Point pin_pos;
    std::vector<odb::dbTechLayer*> pin_layers;
    std::map<odb::dbTechLayer*, std::vector<odb::Rect>> pin_boxes;

    for (odb::dbMPin* mterm : mterm->getMPins()) {
      int last_layer = -1;

      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        odb::Rect rect = box->getBox();
        transform.apply(rect);

        if (!die_area.contains(rect) && verbose_) {
          logger_->warn(
              GRT, 35, "Pin {} is outside die area.", getITermName(iterm));
        }
        pin_boxes[tech_layer].push_back(rect);
        if (tech_layer->getRoutingLevel() > last_layer) {
          pin_pos = rect.ll();
        }
      }
    }

    for (auto& layer_boxes : pin_boxes) {
      if (layer_boxes.first->getRoutingLevel() <= max_routing_layer) {
        pin_layers.push_back(layer_boxes.first);
      }
    }

    if (pin_layers.empty()) {
      logger_->error(GRT,
                     29,
                     "Pin {} does not have geometries below the max "
                     "routing layer ({}).",
                     getITermName(iterm),
                     getLayerName(max_routing_layer, db_));
    }

    Pin pin(iterm,
            pin_pos,
            pin_layers,
            pin_boxes,
            (connected_to_pad || connected_to_macro));

    net->addPin(pin);
  }
}

void GlobalRouter::makeBtermPins(Net* net,
                                 odb::dbNet* db_net,
                                 const odb::Rect& die_area)
{
  for (odb::dbBTerm* bterm : db_net->getBTerms()) {
    int posX, posY;
    bterm->getFirstPinLocation(posX, posY);

    std::map<odb::dbTechLayer*, std::vector<odb::Rect>> pin_boxes;

    const std::string pin_name = bterm->getConstName();
    odb::Point pin_pos;

    for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
      int last_layer = -1;
      if (!bterm_pin->getPlacementStatus().isPlaced()) {
        logger_->error(GRT, 11, "Pin {} is not placed.", pin_name);
      }

      for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
        odb::dbTechLayer* tech_layer = bpin_box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        odb::Rect rect = bpin_box->getBox();
        if (!die_area.contains(rect) && verbose_) {
          logger_->warn(GRT, 36, "Pin {} is outside die area.", pin_name);
          odb::Rect intersection;
          rect.intersection(die_area, intersection);
          rect = intersection;
          if (rect.area() == 0) {
            logger_->error(GRT,
                           209,
                           "Pin {} is completely outside the die area and "
                           "cannot bet routed.",
                           pin_name);
          }
        }
        pin_boxes[tech_layer].push_back(rect);

        if (tech_layer->getRoutingLevel() > last_layer) {
          pin_pos = rect.ll();
        }
      }
    }

    std::vector<odb::dbTechLayer*> pin_layers;
    pin_layers.reserve(pin_boxes.size());
    for (auto& layer_boxes : pin_boxes) {
      pin_layers.push_back(layer_boxes.first);
    }

    if (pin_layers.empty()) {
      logger_->error(
          GRT,
          42,
          "Pin {} does not have geometries in a valid routing layer.",
          pin_name);
    }

    Pin pin(bterm, pin_pos, pin_layers, pin_boxes, getRectMiddle(die_area));
    net->addPin(pin);
  }
}

std::string getITermName(odb::dbITerm* iterm)
{
  odb::dbMTerm* mterm = iterm->getMTerm();
  std::string pin_name = iterm->getInst()->getConstName();
  pin_name += "/";
  pin_name += mterm->getConstName();
  return pin_name;
}

std::string getLayerName(int layer_idx, odb::dbDatabase* db)
{
  odb::dbTech* tech = db->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer_idx);
  return tech_layer->getName();
}

void GlobalRouter::computeObstructionsAdjustments()
{
  odb::Rect die_area = grid_->getGridArea();
  std::vector<int> layer_extensions;
  std::map<int, std::vector<odb::Rect>> layer_obs_map;

  findLayerExtensions(layer_extensions);
  int obstructions_cnt = findObstructions(die_area);
  obstructions_cnt
      += findInstancesObstructions(die_area, layer_extensions, layer_obs_map);
  findNetsObstructions(die_area);

  std::vector<LayerId> transition_layers = findTransitionLayers();
  adjustTransitionLayers(transition_layers, layer_obs_map);

  if (verbose_) {
    logger_->info(GRT, 4, "Blockages: {}", obstructions_cnt);
  }
}

void GlobalRouter::findLayerExtensions(std::vector<int>& layer_extensions)
{
  layer_extensions.resize(routing_layers_.size() + 1, 0);

  int min_layer, max_layer;
  getMinMaxLayer(min_layer, max_layer);

  for (auto const& [level, obstruct_layer] : routing_layers_) {
    if (level >= min_layer && level <= max_layer) {
      int max_int = std::numeric_limits<int>::max();

      // Gets the smallest possible minimum spacing that won't cause
      // violations for ANY configuration of PARALLELRUNLENGTH (the biggest
      // value in the table)

      int spacing_extension = obstruct_layer->getSpacing(max_int, max_int);

      // Check for EOL spacing values and, if the spacing is higher than the
      // one found, use them as the macro extension instead of
      // PARALLELRUNLENGTH

      for (auto rule : obstruct_layer->getV54SpacingRules()) {
        int spacing = rule->getSpacing();
        spacing_extension = std::max(spacing, spacing_extension);
      }

      // Check for TWOWIDTHS table values and, if the spacing is higher than
      // the one found, use them as the macro extension instead of
      // PARALLELRUNLENGTH

      if (obstruct_layer->hasTwoWidthsSpacingRules()) {
        std::vector<std::vector<uint32_t>> spacing_table;
        obstruct_layer->getTwoWidthsSpacingTable(spacing_table);
        if (!spacing_table.empty()) {
          const std::vector<uint32_t>& last_row = spacing_table.back();
          uint32_t last_value = last_row.back();
          spacing_extension = std::max<uint32_t>(last_value, spacing_extension);
        }
      }

      // Save the extension to use when defining Macros

      layer_extensions[level] = spacing_extension;
    }
  }
}

int GlobalRouter::findObstructions(odb::Rect& die_area)
{
  int obstructions_cnt = 0;
  for (odb::dbObstruction* obstruction : block_->getObstructions()) {
    odb::dbBox* obstruction_box = obstruction->getBBox();

    int layer = obstruction_box->getTechLayer()->getRoutingLevel();
    if (getMinRoutingLayer() <= layer && layer <= getMaxRoutingLayer()) {
      odb::Point lower_bound
          = odb::Point(obstruction_box->xMin(), obstruction_box->yMin());
      odb::Point upper_bound
          = odb::Point(obstruction_box->xMax(), obstruction_box->yMax());
      odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
      if (!die_area.contains(obstruction_rect)) {
        if (verbose_) {
          logger_->warn(GRT, 37, "Found blockage outside die area.");
        }
      }
      odb::dbTechLayer* tech_layer = obstruction_box->getTechLayer();
      applyObstructionAdjustment(obstruction_rect, tech_layer);
      obstructions_cnt++;
    }
  }

  return obstructions_cnt;
}

bool GlobalRouter::layerIsBlocked(
    int layer,
    const std::unordered_map<int, std::vector<odb::Rect>>& macro_obs_per_layer,
    std::vector<odb::Rect>& extended_obs)
{
  using boost::polygon::operators::operator&;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

  // if layer is max or min, then all obs the nearest layer are added
  if (layer == getMaxRoutingLayer()
      && macro_obs_per_layer.find(layer - 1) != macro_obs_per_layer.end()) {
    extended_obs = macro_obs_per_layer.at(layer - 1);
  }
  if (layer == getMinRoutingLayer()
      && macro_obs_per_layer.find(layer + 1) != macro_obs_per_layer.end()) {
    extended_obs = macro_obs_per_layer.at(layer + 1);
  }

  std::vector<odb::Rect> upper_obs;
  std::vector<odb::Rect> lower_obs;

  // Get Rect vector to layer + 1 and layer - 1
  if (macro_obs_per_layer.find(layer + 1) != macro_obs_per_layer.end()) {
    upper_obs = macro_obs_per_layer.at(layer + 1);
  }
  if (macro_obs_per_layer.find(layer - 1) != macro_obs_per_layer.end()) {
    lower_obs = macro_obs_per_layer.at(layer - 1);
  }

  const Polygon90Set upper_obs_set(
      boost::polygon::HORIZONTAL, upper_obs.begin(), upper_obs.end());
  const Polygon90Set lower_obs_set(
      boost::polygon::HORIZONTAL, lower_obs.begin(), lower_obs.end());

  const Polygon90Set intersections_set = lower_obs_set & upper_obs_set;
  std::vector<odb::Rect> intersections;
  intersections_set.get_rectangles(intersections);
  extended_obs.insert(
      extended_obs.end(), intersections.begin(), intersections.end());

  return !extended_obs.empty();
}

// Add obstructions if they appear on upper and lower layer
void GlobalRouter::extendObstructions(
    std::unordered_map<int, std::vector<odb::Rect>>& macro_obs_per_layer,
    int bottom_layer,
    int top_layer)
{
  // if it has obs on min_layer + 1, then the min_layer needs to be block
  if (bottom_layer - 1 == getMinRoutingLayer()) {
    bottom_layer--;
  }
  // if it has obs on max_layer - 1, then the max_layer needs to be block
  if (top_layer + 1 == getMaxRoutingLayer()) {
    top_layer++;
  }

  for (int layer = bottom_layer; layer <= top_layer; layer++) {
    std::vector<odb::Rect>& obs = macro_obs_per_layer[layer];
    std::vector<odb::Rect> extended_obs;
    // check if layer+1 and layer-1 have obstructions
    // if they have then add to layer Rect vector
    if (layerIsBlocked(layer, macro_obs_per_layer, extended_obs)) {
      obs.insert(obs.end(), extended_obs.begin(), extended_obs.end());
    }
  }
}

int GlobalRouter::findInstancesObstructions(
    odb::Rect& die_area,
    const std::vector<int>& layer_extensions,
    std::map<int, std::vector<odb::Rect>>& layer_obs_map)
{
  int macros_cnt = 0;
  int obstructions_cnt = 0;
  int pin_out_of_die_count = 0;
  odb::dbTech* tech = db_->getTech();
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();

    const odb::dbTransform transform = inst->getTransform();

    bool is_macro = false;
    if (master->isBlock()) {
      macros_cnt++;
      is_macro = true;
    }

    if (is_macro) {
      std::unordered_map<int, std::vector<odb::Rect>> macro_obs_per_layer;
      int bottom_layer = std::numeric_limits<int>::max();
      int top_layer = std::numeric_limits<int>::min();

      for (odb::dbBox* box : master->getObstructions()) {
        int layer = box->getTechLayer()->getRoutingLevel();
        if (getMinRoutingLayer() <= layer && layer <= getMaxRoutingLayer()) {
          odb::Rect rect = box->getBox();
          transform.apply(rect);

          macro_obs_per_layer[layer].push_back(rect);
          obstructions_cnt++;

          bottom_layer = std::min(bottom_layer, layer);
          top_layer = std::max(top_layer, layer);
        }
      }

      extendObstructions(macro_obs_per_layer, bottom_layer, top_layer);

      // iterate all Rects for each layer and apply adjustment in FastRoute
      for (auto& [layer, obs] : macro_obs_per_layer) {
        odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
        int layer_extension = layer_extensions[layer];
        layer_extension += macro_extension_ * grid_->getTileSize();
        for (odb::Rect& cur_obs : obs) {
          if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
            cur_obs.set_ylo(cur_obs.yMin() - layer_extension);
            cur_obs.set_yhi(cur_obs.yMax() + layer_extension);
          } else if (tech_layer->getDirection()
                     == odb::dbTechLayerDir::VERTICAL) {
            cur_obs.set_xlo(cur_obs.xMin() - layer_extension);
            cur_obs.set_xhi(cur_obs.xMax() + layer_extension);
          }
          layer_obs_map[layer].push_back(cur_obs);
          applyObstructionAdjustment(
              cur_obs, tech->findRoutingLayer(layer), is_macro);
        }
      }
    } else {
      for (odb::dbBox* box : master->getObstructions()) {
        int layer = box->getTechLayer()->getRoutingLevel();
        if (getMinRoutingLayer() <= layer && layer <= getMaxRoutingLayer()) {
          odb::Rect rect = box->getBox();
          transform.apply(rect);

          odb::Point lower_bound = odb::Point(rect.xMin(), rect.yMin());
          odb::Point upper_bound = odb::Point(rect.xMax(), rect.yMax());
          odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
          if (!die_area.contains(obstruction_rect)) {
            if (verbose_) {
              logger_->warn(GRT,
                            38,
                            "Found blockage outside die area in instance {}.",
                            inst->getConstName());
            }
          }
          odb::dbTechLayer* tech_layer = box->getTechLayer();
          applyObstructionAdjustment(obstruction_rect, tech_layer);
          obstructions_cnt++;
        }
      }
    }

    for (odb::dbMTerm* mterm : master->getMTerms()) {
      for (odb::dbMPin* mpin : mterm->getMPins()) {
        odb::Point lower_bound;
        odb::Point upper_bound;
        odb::Rect pin_box;
        int pin_layer;

        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::Rect rect = box->getBox();
          transform.apply(rect);

          odb::dbTechLayer* tech_layer = box->getTechLayer();
          if (!tech_layer
              || tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
            continue;
          }

          pin_layer = tech_layer->getRoutingLevel();
          if (getMinRoutingLayer() <= pin_layer
              && pin_layer <= getMaxRoutingLayer()) {
            lower_bound = odb::Point(rect.xMin(), rect.yMin());
            upper_bound = odb::Point(rect.xMax(), rect.yMax());
            pin_box = odb::Rect(lower_bound, upper_bound);
            if (!die_area.contains(pin_box)
                && !mterm->getSigType().isSupply()) {
              logger_->warn(GRT,
                            39,
                            "Found pin {} outside die area in instance {}.",
                            mterm->getConstName(),
                            inst->getConstName());
              pin_out_of_die_count++;
            }
            odb::dbTechLayer* tech_layer = box->getTechLayer();
            applyObstructionAdjustment(pin_box, tech_layer);
          }
        }
      }
    }
  }

  if (pin_out_of_die_count > 0) {
    if (verbose_) {
      logger_->error(
          GRT, 28, "Found {} pins outside die area.", pin_out_of_die_count);
    }
  }

  if (verbose_) {
    logger_->info(GRT, 3, "Macros: {}", macros_cnt);
  }
  return obstructions_cnt;
}

void GlobalRouter::findNetsObstructions(odb::Rect& die_area)
{
  odb::dbSet<odb::dbNet> nets = block_->getNets();

  if (nets.empty()) {
    logger_->error(GRT, 94, "Design with no nets.");
  }

  for (odb::dbNet* db_net : nets) {
    uint32_t wire_cnt = 0, via_cnt = 0;
    db_net->getWireCount(wire_cnt, via_cnt);
    if (wire_cnt == 0) {
      continue;
    }

    std::vector<odb::dbShape> via_boxes;
    if (db_net->getSigType().isSupply()) {
      for (odb::dbSWire* swire : db_net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            s->getViaBoxes(via_boxes);
            for (const odb::dbShape& box : via_boxes) {
              odb::dbTechLayer* tech_layer = box.getTechLayer();
              if (tech_layer->getRoutingLevel() == 0) {
                continue;
              }
              odb::Rect via_rect = box.getBox();
              applyNetObstruction(via_rect, tech_layer, die_area, db_net);
            }
          } else {
            odb::Rect wire_rect = s->getBox();
            odb::dbTechLayer* tech_layer = s->getTechLayer();
            applyNetObstruction(wire_rect, tech_layer, die_area, db_net);
          }
        }
      }
    } else {
      odb::dbWirePath path;
      odb::dbWirePathShape pshape;
      odb::dbWire* wire = db_net->getWire();

      odb::dbWirePathItr pitr;
      for (pitr.begin(wire); pitr.getNextPath(path);) {
        while (pitr.getNextShape(pshape)) {
          const odb::dbShape& shape = pshape.shape;
          if (shape.isVia()) {
            odb::dbShape::getViaBoxes(shape, via_boxes);
            for (const odb::dbShape& box : via_boxes) {
              odb::dbTechLayer* tech_layer = box.getTechLayer();
              if (tech_layer->getRoutingLevel() == 0) {
                continue;
              }
              odb::Rect via_rect = box.getBox();
              applyNetObstruction(via_rect, tech_layer, die_area, db_net);
            }
          } else {
            odb::Rect wire_rect = shape.getBox();
            odb::dbTechLayer* tech_layer = shape.getTechLayer();

            applyNetObstruction(wire_rect, tech_layer, die_area, db_net);
          }
        }
      }
    }
  }
}

void GlobalRouter::applyNetObstruction(const odb::Rect& rect,
                                       odb::dbTechLayer* tech_layer,
                                       const odb::Rect& die_area,
                                       odb::dbNet* db_net)
{
  int l = tech_layer->getRoutingLevel();

  if (getMinRoutingLayer() <= l && l <= getMaxRoutingLayer()) {
    odb::Point lower_bound = odb::Point(rect.xMin(), rect.yMin());
    odb::Point upper_bound = odb::Point(rect.xMax(), rect.yMax());
    odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
    if (!die_area.contains(obstruction_rect)) {
      if (verbose_) {
        logger_->warn(GRT,
                      41,
                      "Net {} has wires/vias outside die area.",
                      db_net->getConstName());
      }
    }
    applyObstructionAdjustment(obstruction_rect, tech_layer);
    // Save position where the net's wires reduced resources
    savePositionWithReducedResources(obstruction_rect, tech_layer, db_net);
  }
}

void GlobalRouter::savePositionWithReducedResources(
    const odb::Rect& rect,
    odb::dbTechLayer* tech_layer,
    odb::dbNet* db_net)
{
  odb::Rect first_tile_box, last_tile_box;
  odb::Point first_tile, last_tile;
  grid_->getBlockedTiles(
      rect, first_tile_box, last_tile_box, first_tile, last_tile);
  // Divide by horizontal and vertical resources
  if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    for (int x = first_tile.getX(); x < last_tile.getX(); x++) {
      for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
        odb::Point pos = odb::Point(x, y);
        h_nets_in_pos_[pos].push_back(db_net);
      }
    }
  } else {
    for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
      for (int y = first_tile.getY(); y < last_tile.getY(); y++) {
        odb::Point pos = odb::Point(x, y);
        v_nets_in_pos_[pos].push_back(db_net);
      }
    }
  }
}

int GlobalRouter::computeMaxRoutingLayer()
{
  int max_routing_layer = -1;

  odb::dbTech* tech = db_->getTech();

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
    odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
    if (track_grid == nullptr) {
      break;
    }
    max_routing_layer = layer;
  }

  if (max_routing_layer == -1) {
    logger_->error(GRT, 701, "Missing track structure for routing layers.");
  }
  return max_routing_layer;
}

void GlobalRouter::addNetToRoute(odb::dbNet* db_net)
{
  nets_to_route_.push_back(db_net);
}

std::vector<odb::dbNet*> GlobalRouter::getNetsToRoute()
{
  return nets_to_route_;
}

void GlobalRouter::mergeNetsRouting(odb::dbNet* db_net1, odb::dbNet* db_net2)
{
  Net* net1 = db_net_map_[db_net1];
  Net* net2 = db_net_map_[db_net2];
  // Try to connect the routing of the two nets
  if (connectRouting(db_net1, db_net2)) {
    net1->setIsMergedNet(true);
    net1->setMergedNet(db_net2);
    net1->setDirtyNet(false);
    net2->setIsMergedNet(true);
    net2->setMergedNet(db_net1);
  } else {
    // After failing to connect the routing, the survivor net still have
    // uncovered pins and needs to be re-routed
    net1->setDirtyNet(true);
  }
}

bool GlobalRouter::connectRouting(odb::dbNet* db_net1, odb::dbNet* db_net2)
{
  Net* net1 = db_net_map_[db_net1];
  Net* net2 = db_net_map_[db_net2];

  // Find the pin positions in the buffer that connects the two nets.
  // At the time this function is called, the buffer information still preserved
  // on GRT data structures, allowing us to use it to define the connection
  // position.
  odb::Point pin_pos1;
  odb::Point pin_pos2;
  findBufferPinPostions(net1, net2, pin_pos1, pin_pos2);

  GRoute& net1_route = routes_[db_net1];
  GRoute& net2_route = routes_[db_net2];
  if (pin_pos1 != pin_pos2) {
    const int layer1 = findTopLayerOverPosition(pin_pos1, net1_route);
    const int layer2 = findTopLayerOverPosition(pin_pos2, net2_route);
    std::vector<GSegment> connection
        = createConnectionForPositions(pin_pos1, pin_pos2, layer1, layer2);
    net1_route.insert(net1_route.end(), net2_route.begin(), net2_route.end());
    net1_route.insert(net1_route.end(), connection.begin(), connection.end());
  } else {
    net1_route.insert(net1_route.end(), net2_route.begin(), net2_route.end());
  }

  updateNetPins(net1);
  std::string dump;
  return netIsCovered(db_net1, dump);
}

void GlobalRouter::findBufferPinPostions(Net* net1,
                                         Net* net2,
                                         odb::Point& pin_pos1,
                                         odb::Point& pin_pos2)
{
  for (const Pin& pin1 : net1->getPins()) {
    if (!pin1.isPort()) {
      for (const Pin& pin2 : net2->getPins()) {
        if (!pin2.isPort()) {
          if (pin1.getITerm()->getInst() == pin2.getITerm()->getInst()) {
            pin_pos1 = pin1.getOnGridPosition();
            pin_pos2 = pin2.getOnGridPosition();
            break;
          }
        }
      }
    }
  }
}

int GlobalRouter::findTopLayerOverPosition(const odb::Point& pin_pos,
                                           const GRoute& route)
{
  int top_layer = -1;
  for (const GSegment& seg : route) {
    odb::Point pt1(seg.init_x, seg.init_y);
    odb::Point pt2(seg.final_x, seg.final_y);
    int layer = std::max(seg.init_layer, seg.final_layer);
    if (pt1 == pin_pos || pt2 == pin_pos) {
      top_layer = std::max(top_layer, layer);
    }
  }

  if (top_layer == -1) {
    logger_->error(GRT,
                   703,
                   "No segment was found in the routing that connects to the "
                   "pin position.");
  }
  return top_layer;
}

std::vector<GSegment> GlobalRouter::createConnectionForPositions(
    const odb::Point& pin_pos1,
    const odb::Point& pin_pos2,
    const int layer1,
    const int layer2)
{
  std::vector<GSegment> connection;

  odb::dbTech* tech = db_->getTech();
  int conn_layer = std::max(layer1, layer2);
  odb::dbTechLayer* tech_conn_layer = tech->findRoutingLayer(conn_layer);

  bool vertical = pin_pos1.getX() == pin_pos2.getX();
  bool horizontal = pin_pos1.getY() == pin_pos2.getY();
  const auto dir = tech_conn_layer->getDirection();
  if (vertical || horizontal) {
    auto [x1, x2] = std::minmax({pin_pos1.getX(), pin_pos2.getX()});
    auto [y1, y2] = std::minmax({pin_pos1.getY(), pin_pos2.getY()});
    if ((vertical && dir != odb::dbTechLayerDir::VERTICAL)
        || (horizontal && dir != odb::dbTechLayerDir::HORIZONTAL)) {
      if (conn_layer > getMinRoutingLayer()) {
        conn_layer--;
      } else {
        conn_layer++;
      }
    }
    connection.emplace_back(x1, y1, conn_layer, x2, y2, conn_layer);
  } else {
    const int layer_fix = conn_layer <= getMinRoutingLayer() ? 1 : -1;
    const int layer_hor = dir == odb::dbTechLayerDir::HORIZONTAL
                              ? conn_layer
                              : conn_layer + layer_fix;
    const int layer_ver = dir == odb::dbTechLayerDir::VERTICAL
                              ? conn_layer
                              : conn_layer + layer_fix;
    int x1 = pin_pos1.getX();
    int y1 = pin_pos1.getY();
    int x2 = pin_pos2.getX();
    int y2 = pin_pos2.getY();
    connection.emplace_back(x1, y1, layer_hor, x2, y1, layer_hor);
    connection.emplace_back(x2, y1, conn_layer + layer_fix, x2, y1, conn_layer);
    connection.emplace_back(x2, y1, layer_ver, x2, y2, layer_ver);

    // Add vias if the additional connections are not touching the existing
    // routing. The via stack can cross multiple routing layers.
    if (layer1 < layer_hor) {
      for (int l = layer1; l < layer_hor; l++) {
        connection.emplace_back(x1, y1, l, x1, y1, l + 1);
      }
    }
    if (layer2 < layer_ver) {
      for (int l = layer2; l < layer_ver; l++) {
        connection.emplace_back(x2, y2, l, x2, y2, l + 1);
      }
    }
  }

  odb::Point via_pos1 = pin_pos1;
  odb::Point via_pos2 = pin_pos2;
  insertViasForConnection(connection, via_pos1, layer1, conn_layer);
  insertViasForConnection(connection, via_pos2, layer2, conn_layer);

  return connection;
}

void GlobalRouter::insertViasForConnection(std::vector<GSegment>& connection,
                                           const odb::Point& via_pos,
                                           const int layer,
                                           const int conn_layer)
{
  auto [min_l, max_l] = std::minmax(layer, conn_layer);
  for (int l = min_l; l < max_l; l++) {
    connection.emplace_back(via_pos.getX(),
                            via_pos.getY(),
                            l,
                            via_pos.getX(),
                            via_pos.getY(),
                            l + 1);
  }
}

void GlobalRouter::getBlockage(odb::dbTechLayer* layer,
                               int x,
                               int y,
                               uint8_t& blockage_h,
                               uint8_t& blockage_v)
{
  int max_layer = std::max(getMaxRoutingLayer(), getMaxLayerForClock());
  if (layer->getRoutingLevel() <= max_layer) {
    fastroute_->getBlockage(layer, x, y, blockage_h, blockage_v);
  }
}

RegionAdjustment::RegionAdjustment(int min_x,
                                   int min_y,
                                   int max_x,
                                   int max_y,
                                   int l,
                                   float adjst)
{
  region = odb::Rect(min_x, min_y, max_x, max_y);
  layer = l;
  adjustment = adjst;
}

// Called from src/fastroute/FastRoute.cpp to so DB headers
// do not have to be included in the core code.
const char* getNetName(odb::dbNet* db_net)
{
  return db_net->getConstName();
}

// Useful for debugging.
void GlobalRouter::print(GRoute& route)
{
  for (GSegment& segment : route) {
    printSegment(segment);
  }
}

void GlobalRouter::printSegment(const GSegment& segment)
{
  logger_->report("{:6d} {:6d} {:2d} -> {:6d} {:6d} {:2d}",
                  segment.init_x,
                  segment.init_y,
                  segment.init_layer,
                  segment.final_x,
                  segment.final_y,
                  segment.final_layer);
}

void GlobalRouter::reportLayerSettings(int min_routing_layer,
                                       int max_routing_layer)
{
  odb::dbTechLayer* min_layer = routing_layers_[min_routing_layer];
  odb::dbTechLayer* max_layer = routing_layers_[max_routing_layer];
  if (verbose_) {
    logger_->info(GRT, 20, "Min routing layer: {}", min_layer->getName());
    logger_->info(GRT, 21, "Max routing layer: {}", max_layer->getName());
    logger_->info(GRT, 22, "Global adjustment: {}%", int(adjustment_ * 100));
    logger_->info(
        GRT, 23, "Grid origin: ({}, {})", grid_origin_.x(), grid_origin_.y());
  }
}

void GlobalRouter::reportResources()
{
  fastroute_->computeCongestionInformation();
  std::vector<int> original_resources = fastroute_->getOriginalResources();
  std::vector<int> derated_resources = fastroute_->getTotalCapacityPerLayer();

  logger_->report("");
  logger_->info(GRT, 53, "Routing resources analysis:");
  logger_->report("          Routing      Original      Derated      Resource");
  logger_->report(
      "Layer     Direction    Resources     Resources    Reduction (%)");
  logger_->report(
      "---------------------------------------------------------------");

  for (size_t l = 0; l < original_resources.size(); l++) {
    odb::dbTechLayer* layer = routing_layers_[l + 1];
    std::string routing_direction
        = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)
              ? "Horizontal"
              : "Vertical";

    float reduciton_percent = 0;
    if (original_resources[l] > 0) {
      reduciton_percent
          = (1.0
             - ((float) derated_resources[l] / (float) original_resources[l]))
            * 100;
    }

    logger_->report("{:7s}    {:10}   {:8}      {:8}          {:3.2f}%",
                    layer->getName(),
                    routing_direction,
                    original_resources[l],
                    derated_resources[l],
                    reduciton_percent);
  }
  logger_->report(
      "---------------------------------------------------------------");
  logger_->report("");
}

void GlobalRouter::reportCongestion()
{
  fastroute_->computeCongestionInformation();
  const std::vector<int>& resources = fastroute_->getTotalCapacityPerLayer();
  const std::vector<int>& demands = fastroute_->getTotalUsagePerLayer();
  const std::vector<int>& overflows = fastroute_->getTotalOverflowPerLayer();
  const std::vector<int>& max_h_overflows
      = fastroute_->getMaxHorizontalOverflows();
  const std::vector<int>& max_v_overflows
      = fastroute_->getMaxVerticalOverflows();

  int total_resource = 0;
  int total_demand = 0;
  int total_overflow = 0;
  int total_h_overflow = 0;
  int total_v_overflow = 0;

  logger_->report("");
  logger_->info(GRT, 96, "Final congestion report:");
  logger_->report(
      "Layer         Resource        Demand        Usage (%)    Max H / "
      "Max "
      "V "
      "/ Total Overflow");
  logger_->report(
      "--------------------------------------------------------------------"
      "--"
      "-----------------");

  for (size_t l = 0; l < resources.size(); l++) {
    float usage_percentage;
    if (resources[l] == 0) {
      usage_percentage = 0.0;
    } else {
      usage_percentage = (float) demands[l] / (float) resources[l];
      usage_percentage *= 100;
    }

    total_resource += resources[l];
    total_demand += demands[l];
    total_overflow += overflows[l];
    total_h_overflow += max_h_overflows[l];
    total_v_overflow += max_v_overflows[l];

    odb::dbTechLayer* layer = routing_layers_[l + 1];
    logger_->report(
        "{:7s}      {:9}       {:7}        {:8.2f}%            {:2} / {:2} "
        "/ "
        "{:2}",
        layer->getName(),
        resources[l],
        demands[l],
        usage_percentage,
        max_h_overflows[l],
        max_v_overflows[l],
        overflows[l]);
  }
  float total_usage = (total_resource == 0)
                          ? 0
                          : (float) total_demand / (float) total_resource * 100;
  logger_->report(
      "--------------------------------------------------------------------"
      "--"
      "-----------------");
  logger_->report(
      "Total        {:9}       {:7}        {:8.2f}%            {:2} / {:2} "
      "/ "
      "{:2}",
      total_resource,
      total_demand,
      total_usage,
      total_h_overflow,
      total_v_overflow,
      total_overflow);
  logger_->report("");
}

void GlobalRouter::reportNetLayerWirelengths(odb::dbNet* db_net,
                                             std::ofstream& out)
{
  std::vector<int64_t> lengths;
  lengths.resize(db_->getTech()->getRoutingLayerCount() + 1);
  GRoute& route = routes_[db_net];
  int via_count = 0;
  for (GSegment& seg : route) {
    int layer1 = seg.init_layer;
    int layer2 = seg.final_layer;
    if (layer1 == layer2) {
      const int seg_length
          = abs(seg.init_x - seg.final_x) + abs(seg.init_y - seg.final_y);
      if (seg_length > 0) {
        lengths[layer1] += seg_length + grid_->getTileSize();
      }
    } else {
      via_count++;
    }
  }
  out << " " << via_count;
  for (size_t i = 0; i < lengths.size(); i++) {
    int64_t length = lengths[i];
    odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
    if (i > 0 && out.is_open()) {
      out << " " << block_->dbuToMicrons(length);
    }
    if (length > 0) {
      logger_->report("\tLayer {:5s}: {:5.2f}um",
                      layer->getName(),
                      block_->dbuToMicrons(length));
    }
  }
}

void GlobalRouter::reportLayerWireLengths(bool global_route,
                                          bool detailed_route)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }

  if (global_route) {
    logger_->info(GRT, 278, "Global route wire length by layer:");
    std::vector<int64_t> lengths(db_->getTech()->getRoutingLayerCount() + 1);
    int64_t total_length = 0;
    for (auto& net_route : routes_) {
      GRoute& route = net_route.second;
      for (GSegment& seg : route) {
        int layer1 = seg.init_layer;
        int layer2 = seg.final_layer;
        if (layer1 == layer2) {
          int seg_length = seg.length();
          lengths[layer1] += seg_length;
          total_length += seg_length;
        }
      }
    }
    if (total_length > 0) {
      logger_->report("Layer    Wire length  Percentage");
      logger_->report("--------------------------------");
      for (size_t i = 0; i < lengths.size(); i++) {
        int64_t length = lengths[i];
        if (length > 0) {
          odb::dbTechLayer* layer = routing_layers_[i];
          logger_->report("{:7s} {:8.2f}um {:5}%",
                          layer->getName(),
                          block_->dbuToMicrons(length),
                          static_cast<int>((100.0 * length) / total_length));
        }
      }
      logger_->report("--------------------------------");
    }
  }

  if (detailed_route) {
    logger_->info(GRT, 279, "Detailed route wire length by layer:");
    std::vector<int64_t> lengths(db_->getTech()->getRoutingLayerCount() + 1);
    int64_t total_length = 0;
    odb::dbSet<odb::dbNet> nets = block_->getNets();
    for (odb::dbNet* db_net : nets) {
      odb::dbWire* wire = db_net->getWire();
      if (wire == nullptr || db_net->getSigType().isSupply()
          || db_net->isSpecial() || !db_net->getSWires().empty()
          || db_net->isConnectedByAbutment()) {
        continue;
      }

      odb::dbWirePath path;
      odb::dbWirePathShape pshape;
      odb::dbWirePathItr pitr;
      for (pitr.begin(wire); pitr.getNextPath(path);) {
        while (pitr.getNextShape(pshape)) {
          const odb::dbShape& shape = pshape.shape;
          if (!shape.isVia()) {
            int layer = shape.getTechLayer()->getRoutingLevel();
            int seg_length = shape.getLength();
            lengths[layer] += seg_length;
            total_length += seg_length;
          }
        }
      }
    }

    if (total_length > 0) {
      logger_->report("Layer    Wire length  Percentage");
      logger_->report("--------------------------------");
      for (size_t i = 0; i < lengths.size(); i++) {
        int64_t length = lengths[i];
        if (length > 0) {
          odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
          logger_->report("{:7s} {:8.2f}um {:5}%",
                          layer->getName(),
                          block_->dbuToMicrons(length),
                          static_cast<int>((100.0 * length) / total_length));
        }
      }
      logger_->report("--------------------------------");
    }
  }
}

void GlobalRouter::reportNetWireLength(odb::dbNet* net,
                                       bool global_route,
                                       bool detailed_route,
                                       bool verbose,
                                       const char* file_name)
{
  std::string file(file_name);
  std::ofstream out;
  if (!file.empty()) {
    out.open(file, std::ios::app);
  }

  int pin_count = net->getITermCount() + net->getBTermCount();

  block_ = db_->getChip()->getBlock();
  if (global_route) {
    if (routes_.find(net) == routes_.end()) {
      logger_->warn(
          GRT, 241, "Net {} does not have global route.", net->getName());
      return;
    }
    int wl = computeNetWirelength(net);
    logger_->info(GRT,
                  237,
                  "Net {} global route wire length: {:.2f}um",
                  net->getName(),
                  block_->dbuToMicrons(wl));

    if (out.is_open()) {
      out << "grt: " << net->getName() << " " << block_->dbuToMicrons(wl) << " "
          << pin_count;
    }

    if (verbose) {
      reportNetLayerWirelengths(net, out);
    }

    if (out.is_open()) {
      out << "\n";
    }
  }

  if (detailed_route) {
    odb::dbWire* wire = net->getWire();

    if (wire == nullptr) {
      logger_->warn(
          GRT, 239, "Net {} does not have detailed route.", net->getName());
      return;
    }

    int64_t wl = wire->getLength();
    logger_->info(GRT,
                  240,
                  "Net {} detailed route wire length: {:.2f}um",
                  net->getName(),
                  block_->dbuToMicrons(wl));

    if (out.is_open()) {
      out << "drt: " << net->getName() << " " << block_->dbuToMicrons(wl) << " "
          << pin_count;
    }

    if (verbose) {
      reportNetDetailedRouteWL(wire, out);
    }

    if (out.is_open()) {
      out << "\n";
    }
  }
}

void GlobalRouter::reportNetDetailedRouteWL(odb::dbWire* wire,
                                            std::ofstream& out)
{
  std::vector<int64_t> lengths;
  lengths.resize(db_->getTech()->getRoutingLayerCount() + 1);
  odb::dbWireShapeItr shapes;
  odb::dbShape s;
  int via_count = 0;
  for (shapes.begin(wire); shapes.next(s);) {
    if (!s.isVia()) {
      lengths[s.getTechLayer()->getRoutingLevel()] += s.getLength();
    } else {
      via_count++;
    }
  }

  out << " " << via_count;
  for (size_t i = 1; i < lengths.size(); i++) {
    int64_t length = lengths[i];
    odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
    if (i > 0 && out.is_open()) {
      out << " " << block_->dbuToMicrons(length);
    }
    if (length > 0) {
      logger_->report("\tLayer {:5s}: {:5.2f}um",
                      layer->getName(),
                      block_->dbuToMicrons(length));
    }
  }
}

void GlobalRouter::createWLReportFile(const char* file_name, bool verbose)
{
  std::ofstream out(file_name);
  out << "tool net total_wl #pins ";

  if (verbose) {
    out << "#vias ";
    for (int i = 1; i <= db_->getTech()->getRoutingLayerCount(); i++) {
      odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
      out << layer->getName() << "_wl ";
    }
  }
  out << "\n";
}

void GlobalRouter::initDebugFastRoute(
    std::unique_ptr<AbstractFastRouteRenderer> renderer)
{
  fastroute_->setDebugOn(std::move(renderer));
}
AbstractFastRouteRenderer* GlobalRouter::getDebugFastRoute() const
{
  return fastroute_->fastrouteRender();
}
void GlobalRouter::setDebugSteinerTree(bool steinerTree)
{
  fastroute_->setDebugSteinerTree(steinerTree);
}
void GlobalRouter::setDebugNet(const odb::dbNet* net)
{
  fastroute_->setDebugNet(net);
}
void GlobalRouter::setDebugRectilinearSTree(bool rectilinearSTree)
{
  fastroute_->setDebugRectilinearSTree(rectilinearSTree);
}
void GlobalRouter::setDebugTree2D(bool tree2D)
{
  fastroute_->setDebugTree2D(tree2D);
}
void GlobalRouter::setDebugTree3D(bool tree3D)
{
  fastroute_->setDebugTree3D(tree3D);
}
void GlobalRouter::setSttInputFilename(const char* file_name)
{
  fastroute_->setSttInputFilename(file_name);
}

// For rsz::makeBufferedNetGlobalRoute so Pin/Net classes do not have to be
// exported.
std::vector<PinGridLocation> GlobalRouter::getPinGridPositions(
    odb::dbNet* db_net)
{
  Net* net = getNet(db_net);
  std::vector<PinGridLocation> pin_locs;
  for (Pin& pin : net->getPins()) {
    pin_locs.emplace_back(pin.getITerm(),
                          pin.getBTerm(),
                          pin.getPosition(),
                          pin.getOnGridPosition(),
                          pin.getConnectionLayer());
  }
  return pin_locs;
}

void GlobalRouter::writePinLocations(const char* file_name)
{
  std::ofstream pin_loc_file;
  pin_loc_file.open(file_name);
  if (!pin_loc_file) {
    logger_->error(
        GRT, 271, "Global route pin locations file could not be opened.");
  }

  for (const auto [db_net, net] : db_net_map_) {
    if (!net->getPins().empty()) {
      pin_loc_file << net->getName() << " " << net->getNumPins() << "\n";
      for (const Pin& pin : net->getPins()) {
        const odb::Point& pin_pos = pin.getOnGridPosition();
        pin_loc_file << pin.getName() << " " << pin_pos.getX() << " "
                     << pin_pos.getY() << "\n";
      }
      pin_loc_file << "\n";
    }
  }
  pin_loc_file.close();
}

////////////////////////////////////////////////////////////////

bool operator<(const RoutePt& p1, const RoutePt& p2)
{
  return (p1.x_ < p2.x_) || (p1.x_ == p2.x_ && p1.y_ < p2.y_)
         || (p1.x_ == p2.x_ && p1.y_ == p2.y_ && p1.layer_ < p2.layer_);
}

bool operator==(const RoutePt& p1, const RoutePt& p2)
{
  return (p1.x_ == p2.x_ && p1.y_ == p2.y_ && p1.layer_ == p2.layer_);
}

////////////////////////////////////////////////////////////////

IncrementalGRoute::IncrementalGRoute(GlobalRouter* groute, odb::dbBlock* block)
    : groute_(groute), db_cbk_(groute)
{
  db_cbk_.addOwner(block);
}

std::vector<Net*> IncrementalGRoute::updateRoutes(bool save_guides)
{
  return groute_->updateDirtyRoutes(save_guides);
}

IncrementalGRoute::~IncrementalGRoute()
{
  db_cbk_.removeOwner();
}

void GlobalRouter::setRenderer(
    std::unique_ptr<AbstractGrouteRenderer> groute_renderer)
{
  groute_renderer_ = std::move(groute_renderer);
}
AbstractGrouteRenderer* GlobalRouter::getRenderer()
{
  return groute_renderer_.get();
}

void GlobalRouter::addDirtyNet(odb::dbNet* net)
{
  db_net_map_[net]->setDirtyNet(true);
  db_net_map_[net]->saveLastPinPositions();
  dirty_nets_.insert(net);
}

std::vector<Net*> GlobalRouter::updateDirtyRoutes(bool save_guides)
{
  callback_handler_->triggerOnPinAccessUpdateRequired();
  std::vector<Net*> dirty_nets;

  if (!initialized_) {
    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);
    initFastRoute(min_layer, max_layer);
  }

  if (!dirty_nets_.empty()) {
    fastroute_->setVerbose(false);
    fastroute_->clearNetsToRoute();

    updateDirtyNets(dirty_nets);

    std::vector<odb::dbNet*> modified_nets;
    modified_nets.reserve(dirty_nets.size());
    for (const Net* net : dirty_nets) {
      modified_nets.push_back(net->getDbNet());
    }

    if (verbose_) {
      logger_->info(GRT, 9, "rerouting {} nets.", dirty_nets.size());
    }
    if (logger_->debugCheck(GRT, "incr", 2)) {
      debugPrint(logger_, GRT, "incr", 2, "Dirty nets:");
      for (auto net : dirty_nets) {
        debugPrint(logger_, GRT, "incr", 2, " {}", net->getConstName());
      }
    }

    if (dirty_nets.empty()) {
      return dirty_nets;
    }

    const float old_critical_nets_percentage
        = fastroute_->getCriticalNetsPercentage();
    fastroute_->setCriticalNetsPercentage(0);
    fastroute_->setCongestionReportIterStep(0);

    initFastRouteIncr(dirty_nets);

    NetRouteMap new_route
        = findRouting(dirty_nets, getMinRoutingLayer(), getMaxRoutingLayer());
    mergeResults(new_route);

    bool reroutingOverflow = true;
    if (fastroute_->has2Doverflow() && !allow_congestion_) {
      // The maximum number of times that the nets traversing the congestion
      // area will be added
      int add_max = 30;
      // The set will contain the nets for routing
      std::set<odb::dbNet*> congestion_nets;
      // The dirty nets that could not be routed are added
      for (auto& it : dirty_nets) {
        congestion_nets.insert(it->getDbNet());
      }
      while (fastroute_->has2Doverflow() && reroutingOverflow && add_max >= 0) {
        // The nets that cross the congestion area are obtained and added to
        // the set
        // if the nets have wires
        if (haveDetailedRoutes()) {
          // find nets on congestion areas using wires
          getCongestionNets(congestion_nets);
        } else {
          // find nets on congestion areas using guides
          fastroute_->getCongestionNets(congestion_nets);
        }
        // When every attempt to increase the congestion region failed, try
        // legalizing the buffers inserted
        if (add_max == 0) {
          opendp_->detailedPlacement(0, 0, "");
          updateDirtyNets(dirty_nets);
          for (auto& it : dirty_nets) {
            congestion_nets.insert(it->getDbNet());
          }
        }
        // Copy the nets from the set to the vector of dirty nets
        dirty_nets.clear();
        for (odb::dbNet* db_net : congestion_nets) {
          dirty_nets.push_back(db_net_map_[db_net]);
          // Release resources on FastRouter
          fastroute_->clearNetRoute(db_net);
          // if the net has wires, release resources used by wires
          Net* net = db_net_map_[db_net];
          destroyNetWire(net);
        }
        // The dirty nets are initialized and then routed
        initFastRouteIncr(dirty_nets);
        NetRouteMap new_route = findRouting(
            dirty_nets, getMinRoutingLayer(), getMaxRoutingLayer());
        mergeResults(new_route);
        add_max--;
      }
      if (fastroute_->has2Doverflow()) {
        is_congested_ = true;
        updateDbCongestion();
        saveCongestion();
        // Suggest adjustment value
        suggestAdjustment();
        logger_->error(GRT,
                       232,
                       "Routing congestion too high. Check the congestion "
                       "heatmap in the GUI.");
      }
    }
    fastroute_->setCriticalNetsPercentage(old_critical_nets_percentage);
    fastroute_->setCongestionReportIterStep(congestion_report_iter_step_);
    if (save_guides) {
      saveGuides(modified_nets);
    }
  }

  fastroute_->setIncrementalGrt(false);

  return dirty_nets;
}

// Get the nets that pass through the congestion area based on their wires
void GlobalRouter::getCongestionNets(std::set<odb::dbNet*>& congestion_nets)
{
  std::vector<std::pair<odb::Point, bool>> pos_with_overflow;
  // Get GCell positions with congestion
  fastroute_->getOverflowPositions(pos_with_overflow);
  for (auto& itr : pos_with_overflow) {
    odb::Point& position = itr.first;
    bool& is_horizontal = itr.second;
    // Find nets with horizontal wires on congestion areas
    if (is_horizontal
        && h_nets_in_pos_.find(position) != h_nets_in_pos_.end()) {
      for (odb::dbNet* db_net : h_nets_in_pos_.at(position)) {
        // Avoid modifying supply nets
        if (!db_net->getSigType().isSupply()) {
          congestion_nets.insert(db_net);
        }
      }
    }
    // Find nets with vertical wires on congestion areas
    if (!is_horizontal
        && v_nets_in_pos_.find(position) != v_nets_in_pos_.end()) {
      for (odb::dbNet* db_net : v_nets_in_pos_.at(position)) {
        // Avoid modifying supply nets
        if (!db_net->getSigType().isSupply()) {
          congestion_nets.insert(db_net);
        }
      }
    }
  }
}

void GlobalRouter::initFastRouteIncr(std::vector<Net*>& nets)
{
  initNetlist(nets);
  fastroute_->initAuxVar();
  fastroute_->setIncrementalGrt(true);
}

GRouteDbCbk::GRouteDbCbk(GlobalRouter* grouter) : grouter_(grouter)
{
}

void GRouteDbCbk::inDbPostMoveInst(odb::dbInst* inst)
{
  instItermsDirty(inst);
}

void GRouteDbCbk::inDbInstSwapMasterAfter(odb::dbInst* inst)
{
  instItermsDirty(inst);
}

void GRouteDbCbk::instItermsDirty(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* db_net = iterm->getNet();
    if (db_net != nullptr && !db_net->isSpecial()) {
      grouter_->addDirtyNet(db_net);
    }
  }
}

void GRouteDbCbk::inDbNetCreate(odb::dbNet* net)
{
  if (net != nullptr && !net->isSpecial()) {
    grouter_->addNet(net);
  }
}

void GRouteDbCbk::inDbNetDestroy(odb::dbNet* net)
{
  grouter_->removeNet(net);
}

void GRouteDbCbk::inDbNetPostMerge(odb::dbNet* preserved_net,
                                   odb::dbNet* removed_net)
{
  grouter_->mergeNetsRouting(preserved_net, removed_net);
}

void GRouteDbCbk::inDbITermPreDisconnect(odb::dbITerm* iterm)
{
  odb::dbNet* db_net = iterm->getNet();
  if (db_net != nullptr && !db_net->isSpecial()) {
    grouter_->addDirtyNet(iterm->getNet());
  }
}

void GRouteDbCbk::inDbITermPostConnect(odb::dbITerm* iterm)
{
  // missing net pin update
  odb::dbNet* net = iterm->getNet();
  if (net != nullptr && !net->isSpecial()) {
    grouter_->addDirtyNet(iterm->getNet());
  }
}

void GRouteDbCbk::inDbITermPostSetAccessPoints(odb::dbITerm* iterm)
{
  odb::dbNet* net = iterm->getNet();
  if (net != nullptr && !net->isSpecial()) {
    grouter_->addDirtyNet(iterm->getNet());
  }
}

void GRouteDbCbk::inDbBTermPostConnect(odb::dbBTerm* bterm)
{
  // missing net pin update
  odb::dbNet* net = bterm->getNet();
  if (net != nullptr && !net->isSpecial()) {
    grouter_->addDirtyNet(bterm->getNet());
  }
}

void GRouteDbCbk::inDbBTermPreDisconnect(odb::dbBTerm* bterm)
{
  odb::dbNet* db_net = bterm->getNet();
  if (db_net != nullptr && !db_net->isSpecial()) {
    grouter_->addDirtyNet(bterm->getNet());
  }
}

////////////////////////////////////////////////////////////////

GSegment::GSegment(int x0, int y0, int l0, int x1, int y1, int l1, bool jumper)
{
  init_x = std::min(x0, x1);
  init_y = std::min(y0, y1);
  init_layer = l0;
  final_x = std::max(x0, x1);
  final_y = std::max(y0, y1);
  final_layer = l1;
  is_jumper = jumper;
}

bool GSegment::operator==(const GSegment& segment) const
{
  return init_layer == segment.init_layer && final_layer == segment.final_layer
         && init_x == segment.init_x && init_y == segment.init_y
         && final_x == segment.final_x && final_y == segment.final_y;
}

std::size_t GSegmentHash::operator()(const GSegment& seg) const
{
  return boost::hash<std::tuple<int, int, int, int, int, int>>()(
      {seg.init_x,
       seg.init_y,
       seg.init_layer,
       seg.final_x,
       seg.final_y,
       seg.final_layer});
}

}  // namespace grt
