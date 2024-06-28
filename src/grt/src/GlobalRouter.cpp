/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "grt/GlobalRouter.h"

#include <algorithm>
#include <boost/icl/interval.hpp>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AbstractFastRouteRenderer.h"
#include "AbstractGrouteRenderer.h"
#include "AbstractRoutingCongestionDataSource.h"
#include "FastRoute.h"
#include "Grid.h"
#include "MakeWireParasitics.h"
#include "RepairAntennas.h"
#include "RoutingTracks.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GRoute.h"
#include "grt/Rudy.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/wOrder.h"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/Parasitics.hh"
#include "sta/Set.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace grt {

using boost::icl::interval;
using utl::GRT;

GlobalRouter::GlobalRouter()
    : logger_(nullptr),
      stt_builder_(nullptr),
      antenna_checker_(nullptr),
      opendp_(nullptr),
      resizer_(nullptr),
      fastroute_(nullptr),
      grid_origin_(0, 0),
      groute_renderer_(nullptr),
      grid_(new Grid),
      adjustment_(0.0),
      min_routing_layer_(1),
      max_routing_layer_(-1),
      layer_for_guide_dimension_(3),
      gcells_offset_(2),
      overflow_iterations_(50),
      congestion_report_iter_step_(0),
      allow_congestion_(false),
      macro_extension_(0),
      initialized_(false),
      verbose_(false),
      min_layer_for_clock_(-1),
      max_layer_for_clock_(-2),
      seed_(0),
      caps_perturbation_percentage_(0),
      perturbation_amount_(1),
      sta_(nullptr),
      db_(nullptr),
      block_(nullptr),
      repair_antennas_(nullptr),
      rudy_(nullptr),
      heatmap_(nullptr),
      heatmap_rudy_(nullptr),
      congestion_file_name_(nullptr),
      grouter_cbk_(nullptr)
{
}

void GlobalRouter::init(utl::Logger* logger,
                        stt::SteinerTreeBuilder* stt_builder,
                        odb::dbDatabase* db,
                        sta::dbSta* sta,
                        rsz::Resizer* resizer,
                        ant::AntennaChecker* antenna_checker,
                        dpl::Opendp* opendp,
                        std::unique_ptr<AbstractRoutingCongestionDataSource>
                            routing_congestion_data_source,
                        std::unique_ptr<AbstractRoutingCongestionDataSource>
                            routing_congestion_data_source_rudy)
{
  logger_ = logger;
  stt_builder_ = stt_builder;
  db_ = db;
  stt_builder_ = stt_builder;
  antenna_checker_ = antenna_checker;
  opendp_ = opendp;
  fastroute_ = new FastRouteCore(db_, logger_, stt_builder_);
  sta_ = sta;
  resizer_ = resizer;

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
  delete grid_;
  for (auto [ignored, net] : db_net_map_) {
    delete net;
  }
  delete repair_antennas_;
}

std::vector<Net*> GlobalRouter::initFastRoute(int min_routing_layer,
                                              int max_routing_layer)
{
  ensureLayerForGuideDimension(max_routing_layer);

  configFastRoute();

  initRoutingLayers(min_routing_layer, max_routing_layer);
  reportLayerSettings(min_routing_layer, max_routing_layer);
  initRoutingTracks(max_routing_layer);
  initCoreGrid(max_routing_layer);
  setCapacities(min_routing_layer, max_routing_layer);

  std::vector<Net*> nets = findNets();
  checkPinPlacement();
  initNetlist(nets);

  applyAdjustments(min_routing_layer, max_routing_layer);
  perturbCapacities();
  initialized_ = true;
  return nets;
}

void GlobalRouter::applyAdjustments(int min_routing_layer,
                                    int max_routing_layer)
{
  fastroute_->initEdges();
  computeGridAdjustments(min_routing_layer, max_routing_layer);
  computeTrackAdjustments(min_routing_layer, max_routing_layer);
  computeObstructionsAdjustments();
  std::vector<int> track_space = grid_->getTrackPitches();
  fastroute_->initBlockedIntervals(track_space);
  computeUserGlobalAdjustments(min_routing_layer, max_routing_layer);
  computeUserLayerAdjustments(max_routing_layer);

  computePinOffsetAdjustments();

  for (RegionAdjustment region_adjustment : region_adjustments_) {
    computeRegionAdjustments(region_adjustment.getRegion(),
                             region_adjustment.getLayer(),
                             region_adjustment.getAdjustment());
  }
  fastroute_->initAuxVar();
}

// If file name is specified, save congestion report file.
// If there are no congestions, the empty file overwrites any
// previous congestion report file.
void GlobalRouter::saveCongestion()
{
  if (congestion_file_name_ != nullptr) {
    fastroute_->saveCongestion();
  }
}

bool GlobalRouter::haveRoutes()
{
  if (routes_.empty()) {
    loadGuidesFromDB();
  }

  return !routes_.empty();
}

bool GlobalRouter::haveDetailedRoutes()
{
  for (odb::dbNet* db_net : block_->getNets()) {
    if (isDetailedRouted(db_net)) {
      return true;
    }
  }
  return false;
}

void GlobalRouter::globalRoute(bool save_guides,
                               bool start_incremental,
                               bool end_incremental)
{
  if (start_incremental && end_incremental) {
    logger_->error(GRT,
                   251,
                   "The start_incremental and end_incremental flags cannot be "
                   "defined together");
  } else if (start_incremental) {
    grouter_cbk_ = new GRouteDbCbk(this);
    grouter_cbk_->addOwner(block_);
  } else {
    try {
      if (end_incremental) {
        updateDirtyRoutes();
        grouter_cbk_->removeOwner();
        delete grouter_cbk_;
        grouter_cbk_ = nullptr;
      } else {
        clear();
        block_ = db_->getChip()->getBlock();

        int min_layer, max_layer;
        getMinMaxLayer(min_layer, max_layer);

        std::vector<Net*> nets = initFastRoute(min_layer, max_layer);

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

    updateDbCongestion();
    saveCongestion();
    checkOverflow();
    if (fastroute_->totalOverflow() > 0 && verbose_) {
      logger_->warn(GRT, 115, "Global routing finished with overflow.");
    }

    if (verbose_) {
      reportCongestion();
    }
    computeWirelength();
    if (verbose_) {
      logger_->info(GRT, 14, "Routed nets: {}", routes_.size());
    }
    if (save_guides) {
      saveGuides();
    }
  }
}

void GlobalRouter::updateDbCongestion()
{
  fastroute_->updateDbCongestion();
  heatmap_->update();
}

void GlobalRouter::repairAntennas(odb::dbMTerm* diode_mterm,
                                  int iterations,
                                  float ratio_margin,
                                  const int num_threads)
{
  if (!initialized_) {
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
      logger_->warn(
          GRT, 246, "No diode with LEF class CORE ANTENNACELL found.");
      return;
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

  while (violations && itr < iterations) {
    if (verbose_) {
      logger_->info(GRT, 6, "Repairing antennas, iteration {}.", itr + 1);
    }
    violations = repair_antennas_->checkAntennaViolations(routes_,
                                                          nets_to_repair,
                                                          max_routing_layer_,
                                                          diode_mterm,
                                                          ratio_margin,
                                                          num_threads);
    if (violations) {
      IncrementalGRoute incr_groute(this, block_);
      repair_antennas_->repairAntennas(diode_mterm);
      logger_->info(
          GRT, 15, "Inserted {} diodes.", repair_antennas_->getDiodesCount());
      int illegal_diode_placement_count
          = repair_antennas_->illegalDiodePlacementCount();
      if (illegal_diode_placement_count > 0) {
        logger_->info(GRT,
                      54,
                      "Using detailed placer to place {} diodes.",
                      illegal_diode_placement_count);
      }
      repair_antennas_->legalizePlacedCells();
      nets_to_repair.clear();
      for (const Net* net : incr_groute.updateRoutes()) {
        nets_to_repair.push_back(net->getDbNet());
      }
    }
    repair_antennas_->clearViolations();
    itr++;
  }
  saveGuides();
}

void GlobalRouter::makeNetWires()
{
  std::vector<odb::dbNet*> nets_to_repair;
  for (odb::dbNet* db_net : block_->getNets()) {
    nets_to_repair.push_back(db_net);
  }

  if (repair_antennas_ == nullptr) {
    repair_antennas_
        = new RepairAntennas(this, antenna_checker_, opendp_, db_, logger_);
  }
  repair_antennas_->makeNetWires(routes_, nets_to_repair, max_routing_layer_);
}

void GlobalRouter::destroyNetWires()
{
  std::vector<odb::dbNet*> nets_to_repair;
  for (odb::dbNet* db_net : block_->getNets()) {
    nets_to_repair.push_back(db_net);
  }

  repair_antennas_->destroyNetWires(nets_to_repair);
}

NetRouteMap GlobalRouter::findRouting(std::vector<Net*>& nets,
                                      int min_routing_layer,
                                      int max_routing_layer)
{
  NetRouteMap routes;
  if (!nets.empty()) {
    MakeWireParasitics builder(
        logger_, resizer_, sta_, db_->getTech(), block_, this);
    fastroute_->setMakeWireParasiticsBuilder(&builder);
    routes = fastroute_->run();
    fastroute_->setMakeWireParasiticsBuilder(nullptr);
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

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta_->deleteParasitics();

  // Make separate parasitics for each corner.
  sta_->setParasiticAnalysisPts(true);

  MakeWireParasitics builder(
      logger_, resizer_, sta_, db_->getTech(), block_, this);
  for (auto& net_route : routes_) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    if (!route.empty()) {
      Net* net = getNet(db_net);
      builder.estimateParasitcs(db_net, net->getPins(), route);
    }
  }
}

void GlobalRouter::estimateRC(odb::dbNet* db_net)
{
  MakeWireParasitics builder(
      logger_, resizer_, sta_, db_->getTech(), block_, this);
  auto iter = routes_.find(db_net);
  if (iter == routes_.end()) {
    return;
  }
  GRoute& route = iter->second;
  if (!route.empty()) {
    Net* net = getNet(db_net);
    builder.estimateParasitcs(db_net, net->getPins(), route);
  }
}

std::vector<int> GlobalRouter::routeLayerLengths(odb::dbNet* db_net)
{
  MakeWireParasitics builder(
      logger_, resizer_, sta_, db_->getTech(), block_, this);
  return builder.routeLayerLengths(db_net);
}

////////////////////////////////////////////////////////////////

void GlobalRouter::initCoreGrid(int max_routing_layer)
{
  initGrid(max_routing_layer);

  computeCapacities(max_routing_layer);
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
  for (int l = 1; l <= grid_->getNumLayers(); l++) {
    if (l < min_routing_layer || l > max_routing_layer) {
      fastroute_->addHCapacity(0, l);
      fastroute_->addVCapacity(0, l);

      horizontal_capacities_.push_back(0);
      vertical_capacities_.push_back(0);
    } else {
      const int h_cap = grid_->getHorizontalEdgesCapacities()[l - 1];
      const int v_cap = grid_->getVerticalEdgesCapacities()[l - 1];
      fastroute_->addHCapacity(h_cap, l);
      fastroute_->addVCapacity(v_cap, l);

      horizontal_capacities_.push_back(h_cap);
      vertical_capacities_.push_back(v_cap);

      grid_->setHorizontalCapacity(h_cap * 100, l - 1);
      grid_->setVerticalCapacity(v_cap * 100, l - 1);
    }
  }
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
    // get last pin positions
    std::vector<odb::Point> last_pos;
    for (const Pin& pin : net->getPins()) {
      last_pos.push_back(pin.getOnGridPosition());
    }
    net->destroyPins();
    // update pin positions
    makeItermPins(net, db_net, grid_->getGridArea());
    makeBtermPins(net, db_net, grid_->getGridArea());
    findPins(net);
    destroyNetWire(net);
    // compare new positions with last positions & add on vector
    if (pinPositionsChanged(net, last_pos)) {
      dirty_nets.push_back(db_net_map_[db_net]);
    }
  }
  dirty_nets_.clear();
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
          if (tech_layer->getRoutingLevel() == 0) {
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
  bool vertical = tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
  int layer_idx = tech_layer->getRoutingLevel();
  odb::Rect first_tile_box, last_tile_box;
  odb::Point first_tile, last_tile;

  grid_->getBlockedTiles(
      rect, first_tile_box, last_tile_box, first_tile, last_tile);

  if (vertical) {
    for (int x = first_tile.getX(); x <= last_tile.getX(); x++) {
      for (int y = first_tile.getY(); y < last_tile.getY(); y++) {
        int cap = fastroute_->getEdgeCapacity(x, y, x, y + 1, layer_idx);
        fastroute_->addAdjustment(x, y, x, y + 1, layer_idx, cap + 1, false);
      }
    }
  } else {
    for (int x = first_tile.getX(); x < last_tile.getX(); x++) {
      for (int y = first_tile.getY(); y <= last_tile.getY(); y++) {
        int cap = fastroute_->getEdgeCapacity(x, y, x, y + 1, layer_idx);
        fastroute_->addAdjustment(x, y, x + 1, y, layer_idx, cap + 1, false);
      }
    }
  }
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
    std::vector<std::pair<odb::Point, odb::Point>>& ap_positions)
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
  } else {
    access_points = pin.getITerm()->getPrefAccessPoints();
    if (access_points.empty()) {
      // get all APs if there are no preferred access points
      for (const auto& [mpin, aps] : pin.getITerm()->getAccessPoints()) {
        access_points.insert(access_points.end(), aps.begin(), aps.end());
      }
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

    ap_positions.push_back(
        {ap_position, grid_->getPositionOnGrid(ap_position)});
  }

  return true;
}

std::vector<odb::Point> GlobalRouter::findOnGridPositions(
    const Pin& pin,
    bool& has_access_points,
    odb::Point& pos_on_grid)
{
  std::vector<std::pair<odb::Point, odb::Point>> ap_positions;

  has_access_points = findPinAccessPointPositions(pin, ap_positions);

  std::vector<odb::Point> positions_on_grid;

  if (has_access_points) {
    for (const auto& ap_position : ap_positions) {
      pos_on_grid = ap_position.second;
      positions_on_grid.push_back(pos_on_grid);
    }
  } else {
    // if odb doesn't have any APs, run the grt version considering the
    // center of the pin shapes
    const int conn_layer = pin.getConnectionLayer();
    const std::vector<odb::Rect>& pin_boxes = pin.getBoxes().at(conn_layer);
    for (const odb::Rect& pin_box : pin_boxes) {
      pos_on_grid = grid_->getPositionOnGrid(getRectMiddle(pin_box));
      positions_on_grid.push_back(pos_on_grid);
    }
  }

  return positions_on_grid;
}

void GlobalRouter::findPins(Net* net)
{
  for (Pin& pin : net->getPins()) {
    bool has_access_points;
    odb::Point pos_on_grid;
    std::vector<odb::Point> pin_positions_on_grid
        = findOnGridPositions(pin, has_access_points, pos_on_grid);

    int votes = -1;

    odb::Point pin_position;
    for (odb::Point pos : pin_positions_on_grid) {
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
      if (!(pos_on_grid == pin_position)
          && ((layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL
               && pos_on_grid.y() != pin_position.y())
              || (layer->getDirection() == odb::dbTechLayerDir::VERTICAL
                  && pos_on_grid.x() != pin_position.x()))) {
        pin_position = pos_on_grid;
      }
    }

    pin.setOnGridPosition(pin_position);
  }
}

int GlobalRouter::getNetMaxRoutingLayer(const Net* net)
{
  return net->getSignalType() == odb::dbSigType::CLOCK
                 && max_layer_for_clock_ > 0
             ? max_layer_for_clock_
             : max_routing_layer_;
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
    odb::dbTechLayer* layer = routing_layers_[conn_layer];
    // If pin is connected to PAD, create a "fake" location in routing
    // grid to avoid PAD obstructions
    if ((pin.isConnectedToPadOrMacro() || pin.isPort()) && !net->isLocal()
        && gcells_offset_ != 0 && conn_layer <= max_routing_layer) {
      createFakePin(pin, pin_position, layer, net);
    }

    conn_layer = std::min(conn_layer, max_routing_layer);

    int pinX
        = (int) ((pin_position.x() - grid_->getXMin()) / grid_->getTileSize());
    int pinY
        = (int) ((pin_position.y() - grid_->getYMin()) / grid_->getTileSize());

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
        pins_on_grid.push_back(RoutePt(pinX, pinY, conn_layer));
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
  sta::Slack slack = sta_->netSlack(sta_net, sta::MinMax::max());
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
    if (pin_count > 1 && !net->isLocal()
        && (!net->hasWires() || net->hasStackedVias(max_routing_layer))) {
      if (pin_count < min_degree) {
        min_degree = pin_count;
      }

      if (pin_count > max_degree) {
        max_degree = pin_count;
      }
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
}

bool GlobalRouter::pinPositionsChanged(Net* net,
                                       std::vector<odb::Point>& last_pos)
{
  bool is_diferent = false;
  std::map<odb::Point, int> cnt_pos;
  for (const Pin& pin : net->getPins()) {
    cnt_pos[pin.getOnGridPosition()]++;
  }
  for (const odb::Point& last : last_pos) {
    cnt_pos[last]--;
  }
  for (const auto& it : cnt_pos) {
    if (it.second != 0) {
      is_diferent = true;
      break;
    }
  }
  return is_diferent;
}

bool GlobalRouter::makeFastrouteNet(Net* net)
{
  std::vector<RoutePt> pins_on_grid;
  int root_idx;
  findFastRoutePins(net, pins_on_grid, root_idx);

  if (pins_on_grid.size() <= 1) {
    return false;
  }

  // check if net is local in the global routing grid position
  // the (x,y) pin positions here may be different from the original
  // (x,y) pin positions because of findFakePinPosition function
  bool on_grid_local = true;
  RoutePt position = pins_on_grid[0];
  for (RoutePt& pin_pos : pins_on_grid) {
    if (pin_pos.x() != position.x() || pin_pos.y() != position.y()) {
      on_grid_local = false;
      break;
    }
  }

  if (!on_grid_local) {
    bool is_clock = (net->getSignalType() == odb::dbSigType::CLOCK);
    std::vector<int>* edge_cost_per_layer;
    int edge_cost_for_net;
    computeTrackConsumption(net, edge_cost_for_net, edge_cost_per_layer);

    // set layer restriction only to clock nets that are not connected to
    // leaf iterms
    int min_layer, max_layer;
    getNetLayerRange(net->getDbNet(), min_layer, max_layer);

    FrNet* fr_net = fastroute_->addNet(net->getDbNet(),
                                       is_clock,
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
    return true;
  }
  return false;
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
  min_layer = (is_non_leaf_clock && min_layer_for_clock_ > 0)
                  ? min_layer_for_clock_
                  : min_routing_layer_;
  min_layer = std::max(min_layer, pin_min_layer);
  max_layer = (is_non_leaf_clock && max_layer_for_clock_ > 0)
                  ? max_layer_for_clock_
                  : max_routing_layer_;
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
    int& track_consumption,
    std::vector<int>*& edge_costs_per_layer)
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
    edge_costs_per_layer = new std::vector<int>(num_layers + 1, 1);
    std::vector<odb::dbTechLayerRule*> layer_rules;
    ndr->getLayerRules(layer_rules);

    for (odb::dbTechLayerRule* layer_rule : layer_rules) {
      int layerIdx = layer_rule->getLayer()->getRoutingLevel();
      if (layerIdx > net_max_layer) {
        continue;
      }
      RoutingTracks routing_tracks = getRoutingTracksByIndex(layerIdx);
      int default_width = layer_rule->getLayer()->getWidth();
      int default_pitch = routing_tracks.getTrackPitch();

      int ndr_spacing = layer_rule->getSpacing();
      int ndr_width = layer_rule->getWidth();
      int ndr_pitch = ndr_width / 2 + ndr_spacing + default_width / 2;

      int consumption = std::ceil((float) ndr_pitch / default_pitch);
      (*edge_costs_per_layer)[layerIdx - 1] = consumption;

      track_consumption = std::max(track_consumption, consumption);
    }
  }
}

std::vector<LayerId> GlobalRouter::findTransitionLayers()
{
  odb::dbTech* tech = db_->getTech();
  const int max_layer = std::max(max_routing_layer_, max_layer_for_clock_);
  std::map<int, odb::dbTechVia*> default_vias = getDefaultVias(max_layer);
  std::vector<LayerId> transition_layers;
  for (const auto [layer, via] : default_vias) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
    const bool vertical
        = tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
    int via_width = 0;
    for (const auto box : default_vias[layer]->getBoxes()) {
      if (box->getTechLayer()->getRoutingLevel() == layer) {
        via_width = vertical ? box->getWidth() : box->getLength();
        break;
      }
    }

    const double track_pitch = grid_->getTrackPitches()[layer - 1];
    // threshold to define what is a transition layer based on the width of the
    // fat via. using 0.8 to consider transition layers for wide vias
    const float fat_via_threshold = 0.8;
    if (via_width / track_pitch > fat_via_threshold) {
      transition_layers.push_back(layer);
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

void GlobalRouter::computeGridAdjustments(int min_routing_layer,
                                          int max_routing_layer)
{
  const odb::Rect& die_area = grid_->getGridArea();
  odb::Point upper_die_bounds(die_area.dx(), die_area.dy());
  int h_space;
  int v_space;

  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  odb::Point upper_grid_bounds(x_grids * grid_->getTileSize(),
                               y_grids * grid_->getTileSize());
  int x_extra = upper_die_bounds.x() - upper_grid_bounds.x();
  int y_extra = upper_die_bounds.y() - upper_grid_bounds.y();

  for (auto const& [level, routing_layer] : routing_layers_) {
    h_space = 0;
    v_space = 0;

    if (level < min_routing_layer
        || (level > max_routing_layer && max_routing_layer > 0))
      continue;

    int new_v_capacity = 0;
    int new_h_capacity = 0;

    if (routing_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      h_space = grid_->getTrackPitches()[level - 1];
      new_h_capacity = std::floor((grid_->getTileSize() + y_extra) / h_space);
    } else if (routing_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      v_space = grid_->getTrackPitches()[level - 1];
      new_v_capacity = std::floor((grid_->getTileSize() + x_extra) / v_space);
    } else {
      logger_->error(GRT, 71, "Layer spacing not found.");
    }

    int num_adjustments = y_grids - 1 + x_grids - 1;
    fastroute_->setNumAdjustments(num_adjustments);

    if (!grid_->isPerfectRegularX()) {
      fastroute_->setLastColVCapacity(new_v_capacity, level - 1);
      for (int i = 1; i < y_grids; i++) {
        fastroute_->addAdjustment(
            x_grids - 1, i - 1, x_grids - 1, i, level, new_v_capacity, false);
      }
    }
    if (!grid_->isPerfectRegularY()) {
      fastroute_->setLastRowHCapacity(new_h_capacity, level - 1);
      for (int i = 1; i < x_grids; i++) {
        fastroute_->addAdjustment(
            i - 1, y_grids - 1, i, y_grids - 1, level, new_h_capacity, false);
      }
    }
  }
}

/*
 * Remove any routing capacity between the die boundary and the first and last
 * routing tracks on each layer.
 */
void GlobalRouter::computeTrackAdjustments(int min_routing_layer,
                                           int max_routing_layer)
{
  for (auto const& [level, layer] : routing_layers_) {
    if (level < min_routing_layer
        || (level > max_routing_layer && max_routing_layer > 0))
      continue;

    const RoutingTracks routing_tracks = getRoutingTracksByIndex(level);
    const int track_location = routing_tracks.getLocation();
    const int track_space = routing_tracks.getUsePitch();
    const int num_tracks = routing_tracks.getNumTracks();
    const int final_track_location
        = track_location + (track_space * (num_tracks - 1));

    if (num_tracks == 0) {
      continue;
    }

    if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      /* bottom most obstruction */
      const int yh = track_location - track_space;
      if (yh > grid_->getYMin()) {
        odb::Rect init_track_obs(
            grid_->getXMin(), grid_->getYMin(), grid_->getXMax(), yh);
        applyObstructionAdjustment(init_track_obs, layer);
      }

      /* top most obstruction */
      const int yl = final_track_location + track_space;
      if (yl < grid_->getYMax()) {
        odb::Rect final_track_obs(
            grid_->getXMin(), yl, grid_->getXMax(), grid_->getYMax());
        applyObstructionAdjustment(final_track_obs, layer);
      }
    } else {
      /* left most obstruction */
      const int xh = track_location - track_space;
      if (xh > grid_->getXMin()) {
        const odb::Rect init_track_obs(
            grid_->getXMin(), grid_->getYMin(), xh, grid_->getYMax());
        applyObstructionAdjustment(init_track_obs, layer);
      }

      /* right most obstruction */
      const int xl = final_track_location + track_space;
      if (xl < grid_->getXMax()) {
        const odb::Rect final_track_obs(
            xl, grid_->getYMin(), grid_->getXMax(), grid_->getYMax());
        applyObstructionAdjustment(final_track_obs, layer);
      }
    }
  }
}

void GlobalRouter::computePinOffsetAdjustments()
{
  for (auto& [db_net, segments] : pad_pins_connections_) {
    std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
    GRoute& route = segments;
    mergeSegments(pins, route);
    for (auto& segment : segments) {
      int tile_size = grid_->getTileSize();
      int die_area_min_x = grid_->getXMin();
      int die_area_min_y = grid_->getYMin();
      int gcell_id_x
          = floor((float) ((segment.init_x - die_area_min_x) / tile_size));
      int gcell_id_y
          = floor((float) ((segment.init_y - die_area_min_y) / tile_size));
      if (!segment.isVia()) {
        if (segment.init_y == segment.final_y) {
          for (int i = 0; i < gcells_offset_; i++) {
            int curr_cap = fastroute_->getEdgeCapacity(gcell_id_x + i,
                                                       gcell_id_y,
                                                       gcell_id_x + i + 1,
                                                       gcell_id_y,
                                                       segment.init_layer);
            if (curr_cap == 0) {
              continue;
            }
            curr_cap -= 1;
            fastroute_->addAdjustment(gcell_id_x + i,
                                      gcell_id_y,
                                      gcell_id_x + i + 1,
                                      gcell_id_y,
                                      segment.init_layer,
                                      curr_cap,
                                      true);
          }
        } else if (segment.init_x == segment.final_x) {
          for (int i = 0; i < gcells_offset_; i++) {
            int curr_cap = fastroute_->getEdgeCapacity(gcell_id_x,
                                                       gcell_id_y + i,
                                                       gcell_id_x,
                                                       gcell_id_y + i + 1,
                                                       segment.init_layer);
            if (curr_cap == 0) {
              continue;
            }
            curr_cap -= 1;
            fastroute_->addAdjustment(gcell_id_x,
                                      gcell_id_y + i,
                                      gcell_id_x,
                                      gcell_id_y + i + 1,
                                      segment.init_layer,
                                      curr_cap,
                                      true);
          }
        }
      }
    }
  }
}

void GlobalRouter::computeUserGlobalAdjustments(int min_routing_layer,
                                                int max_routing_layer)
{
  if (adjustment_ == 0.0)
    return;

  for (int l = min_routing_layer; l <= max_routing_layer; l++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(l);
    if (tech_layer->getLayerAdjustment() == 0.0) {
      tech_layer->setLayerAdjustment(adjustment_);
    }
  }
}

void GlobalRouter::computeUserLayerAdjustments(int max_routing_layer)
{
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  const std::vector<int>& hor_capacities
      = grid_->getHorizontalEdgesCapacities();
  const std::vector<int>& ver_capacities = grid_->getVerticalEdgesCapacities();

  for (int layer = 1; layer <= max_routing_layer; layer++) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(layer);
    float adjustment = tech_layer->getLayerAdjustment();
    if (adjustment != 0) {
      if (horizontal_capacities_[layer - 1] != 0) {
        int new_cap = hor_capacities[layer - 1] * (1 - adjustment);
        grid_->setHorizontalCapacity(new_cap, layer - 1);

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
                x - 1, y - 1, x, y - 1, layer, new_h_capacity, true);
          }
        }
      }

      if (vertical_capacities_[layer - 1] != 0) {
        int new_cap = ver_capacities[layer - 1] * (1 - adjustment);
        grid_->setVerticalCapacity(new_cap, layer - 1);

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
                x - 1, y - 1, x - 1, y, layer, new_v_capacity, true);
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

void GlobalRouter::applyObstructionAdjustment(const odb::Rect& obstruction,
                                              odb::dbTechLayer* tech_layer)
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

  interval<int>::type first_tile_reduce_interval
      = grid_->computeTileReduceInterval(obstruction_rect,
                                         first_tile_box,
                                         track_space,
                                         true,
                                         tech_layer->getDirection());
  interval<int>::type last_tile_reduce_interval
      = grid_->computeTileReduceInterval(obstruction_rect,
                                         last_tile_box,
                                         track_space,
                                         false,
                                         tech_layer->getDirection());

  if (!vertical) {
    fastroute_->addHorizontalAdjustments(first_tile,
                                         last_tile,
                                         layer,
                                         first_tile_reduce_interval,
                                         last_tile_reduce_interval);
  } else {
    fastroute_->addVerticalAdjustments(first_tile,
                                       last_tile,
                                       layer,
                                       first_tile_reduce_interval,
                                       last_tile_reduce_interval);
  }
}

void GlobalRouter::setAdjustment(const float adjustment)
{
  adjustment_ = adjustment;
}

void GlobalRouter::setMinRoutingLayer(const int min_layer)
{
  min_routing_layer_ = min_layer;
}

void GlobalRouter::setMaxRoutingLayer(const int max_layer)
{
  max_routing_layer_ = max_layer;
}

void GlobalRouter::setMinLayerForClock(const int min_layer)
{
  min_layer_for_clock_ = min_layer;
}

void GlobalRouter::setMaxLayerForClock(const int max_layer)
{
  max_layer_for_clock_ = max_layer;
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
  fastroute_->setCriticalNetsPercentage(critical_nets_percentage);
}

void GlobalRouter::addLayerAdjustment(int layer, float reduction_percentage)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
  if (layer > max_routing_layer_ && max_routing_layer_ > 0) {
    if (verbose_) {
      odb::dbTechLayer* max_tech_layer
          = tech->findRoutingLayer(max_routing_layer_);
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
  region_adjustments_.push_back(RegionAdjustment(
      min_x, min_y, max_x, max_y, layer, reduction_percentage));
}

void GlobalRouter::setVerbose(const bool v)
{
  verbose_ = v;
}

void GlobalRouter::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
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

void GlobalRouter::setMacroExtension(int macro_extension)
{
  macro_extension_ = macro_extension;
}

void GlobalRouter::setPinOffset(int pin_offset)
{
  gcells_offset_ = pin_offset;
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

  for (int layer = 1; layer <= max_routing_layer_; layer++) {
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
        int new_cap
            = grid_->getHorizontalEdgesCapacities()[layer - 1] + perturbation;
        new_cap = new_cap < 0 ? 0 : new_cap;
        grid_->setHorizontalCapacity(new_cap, layer - 1);
        int edge_cap
            = fastroute_->getEdgeCapacity(x - 1, y - 1, x, y - 1, layer);
        int new_h_capacity = (edge_cap + perturbation);
        new_h_capacity = new_h_capacity < 0 ? 0 : new_h_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, x, y - 1, layer, new_h_capacity, subtract);
      } else if (vertical_capacities_[layer - 1] != 0) {
        int new_cap
            = grid_->getVerticalEdgesCapacities()[layer - 1] + perturbation;
        new_cap = new_cap < 0 ? 0 : new_cap;
        grid_->setVerticalCapacity(new_cap, layer - 1);
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
  block_ = db_->getChip()->getBlock();
  routes_.clear();
  if (max_routing_layer_ == -1) {
    max_routing_layer_ = computeMaxRoutingLayer();
  }
  if (routing_layers_.empty()) {
    int min_layer, max_layer;
    getMinMaxLayer(min_layer, max_layer);

    initRoutingLayers(min_layer, max_layer);
    initRoutingTracks(max_layer);
    initCoreGrid(max_layer);
    setCapacities(min_layer, max_layer);
    applyAdjustments(min_layer, max_layer);
    updateDbCongestion();
  }
  std::vector<Net*> nets = findNets();
  initNetlist(nets);
}

void GlobalRouter::ensureLayerForGuideDimension(int max_routing_layer)
{
  if (max_routing_layer < layer_for_guide_dimension_) {
    layer_for_guide_dimension_ = max_routing_layer;
  }
}

void GlobalRouter::configFastRoute()
{
  fastroute_->setVerbose(verbose_);
  fastroute_->setOverflowIterations(overflow_iterations_);
  fastroute_->setCongestionReportIterStep(congestion_report_iter_step_);

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
  if (max_routing_layer_ == -1) {
    max_routing_layer_ = computeMaxRoutingLayer();
  }
  min_layer = min_layer_for_clock_ > 0
                  ? std::min(min_routing_layer_, min_layer_for_clock_)
                  : min_routing_layer_;
  max_layer = std::max(max_routing_layer_, max_layer_for_clock_);
}

void GlobalRouter::checkOverflow()
{
  if (fastroute_->has2Doverflow()) {
    if (!allow_congestion_) {
      if (congestion_file_name_ != nullptr) {
        logger_->error(
            GRT,
            119,
            "Routing congestion too high. Check the congestion heatmap "
            "in the GUI and load {} in the DRC viewer.",
            congestion_file_name_);
      } else {
        logger_->error(
            GRT,
            118,
            "Routing congestion too high. Check the congestion heatmap "
            "in the GUI.");
      }
    }
  }
}

void GlobalRouter::readGuides(const char* file_name)
{
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
    if (line == "(" || line == "" || line == ")") {
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
      guides[net].push_back(std::make_pair(layer->getRoutingLevel(), rect));
      boxToGlobalRouting(rect, layer->getRoutingLevel(), routes_[net]);
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
  initGridAndNets();
  for (odb::dbNet* net : block_->getNets()) {
    for (odb::dbGuide* guide : net->getGuides()) {
      boxToGlobalRouting(
          guide->getBox(), guide->getLayer()->getRoutingLevel(), routes_[net]);
    }
  }

  updateVias();

  for (auto& net_route : routes_) {
    std::vector<Pin>& pins = db_net_map_[net_route.first]->getPins();
    GRoute& route = net_route.second;
    mergeSegments(pins, route);
  }

  updateEdgesUsage();
  heatmap_->update();
}

void GlobalRouter::updateVias()
{
  for (auto& net_route : routes_) {
    GRoute& route = net_route.second;
    if (route.empty())
      continue;
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

  auto db_tech = db_->getTech();
  for (int k = 0; k < grid_->getNumLayers(); k++) {
    auto layer = db_tech->findRoutingLayer(k + 1);
    if (layer == nullptr) {
      continue;
    }

    const auto& h_edges_3D = fastroute_->getHorizontalEdges3D();
    const auto& v_edges_3D = fastroute_->getVerticalEdges3D();

    const unsigned short capH = fastroute_->getHorizontalCapacities()[k];
    const unsigned short capV = fastroute_->getVerticalCapacities()[k];
    for (int y = 0; y < grid_->getYGrids(); y++) {
      for (int x = 0; x < grid_->getXGrids(); x++) {
        const unsigned short blockageH = capH - h_edges_3D[k][y][x].cap;
        const unsigned short blockageV = capV - v_edges_3D[k][y][x].cap;
        const unsigned short usageH = h_edges_3D[k][y][x].usage + blockageH;
        const unsigned short usageV = v_edges_3D[k][y][x].usage + blockageV;
        db_gcell->setCapacity(layer, x, y, capH + capV);
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
        odb::dbGuide::create(db_net, ph_layer_final, guide.second);
      }
    }
  }
}

void GlobalRouter::saveGuides()
{
  int offset_x = grid_origin_.x();
  int offset_y = grid_origin_.y();

  for (odb::dbNet* db_net : block_->getNets()) {
    auto iter = routes_.find(db_net);
    if (iter == routes_.end()) {
      continue;
    }
    Net* net = db_net_map_[db_net];
    GRoute& route = iter->second;

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
            odb::dbGuide::create(db_net, layer1, box);
            odb::dbGuide::create(db_net, layer2, box);
          } else {
            int layer_idx = std::min(segment.init_layer, segment.final_layer);
            odb::dbTechLayer* layer1 = routing_layers_[layer_idx];
            odb::dbGuide::create(db_net, layer1, box);
          }
        } else if (segment.init_layer == segment.final_layer) {
          if (segment.init_layer < min_routing_layer_
              && segment.init_x != segment.final_x
              && segment.init_y != segment.final_y) {
            logger_->error(GRT,
                           74,
                           "Routing with guides in blocked metal for net {}.",
                           db_net->getConstName());
          }

          odb::dbTechLayer* layer = routing_layers_[segment.init_layer];
          odb::dbGuide::create(db_net, layer, box);
        }
      }
    }
    auto dbGuides = db_net->getGuides();
    if (dbGuides.orderReversed() && dbGuides.reversible())
      dbGuides.reverse();
  }
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
      odb::Point pin_pos0 = findFakePinPosition(pins[p - 1], db_net);
      odb::Point pin_pos1 = findFakePinPosition(pins[p], db_net);
      // If the net is not local, FR core result is invalid
      if (pin_pos1.x() != pin_pos0.x() || pin_pos1.y() != pin_pos0.y()) {
        logger_->error(GRT,
                       76,
                       "Net {} does not have route guides.",
                       db_net->getConstName());
      }
    }

    if (pins[p].getConnectionLayer() > last_layer)
      last_layer = pins[p].getConnectionLayer();
  }

  // last_layer can be greater than max routing layer for nets with bumps
  // at top routing layer
  if (last_layer >= max_routing_layer) {
    last_layer--;
  }

  for (int l = 1; l <= last_layer; l++) {
    odb::Point pin_pos = findFakePinPosition(pins[0], db_net);
    GSegment segment = GSegment(
        pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
    route.push_back(segment);
  }
}

void GlobalRouter::connectTopLevelPins(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  for (Pin& pin : pins) {
    if (pin.getConnectionLayer() > max_routing_layer_) {
      odb::Point pin_pos = pin.getOnGridPosition();
      for (int l = max_routing_layer_; l < pin.getConnectionLayer(); l++) {
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

void GlobalRouter::mergeBox(std::vector<odb::Rect>& guide_box,
                            const std::set<odb::Point>& via_positions)
{
  std::vector<odb::Rect> final_box;
  if (guide_box.empty()) {
    logger_->error(GRT, 78, "Guides vector is empty.");
  }
  final_box.push_back(guide_box[0]);
  for (size_t i = 1; i < guide_box.size(); i++) {
    odb::Rect box = guide_box[i];
    odb::Rect& lastBox = final_box.back();

    GRoute segs;
    boxToGlobalRouting(box, 0, segs);
    odb::Point seg_init(segs[0].init_x, segs[0].init_y);
    odb::Point seg_final(segs.back().init_x, segs.back().init_y);

    if (lastBox.overlaps(box)
        && (via_positions.find(seg_init) == via_positions.end()
            || via_positions.find(seg_final) == via_positions.end())) {
      int lowerX = std::min(lastBox.xMin(), box.xMin());
      int lowerY = std::min(lastBox.yMin(), box.yMin());
      int upperX = std::max(lastBox.xMax(), box.xMax());
      int upperY = std::max(lastBox.yMax(), box.yMax());
      lastBox = odb::Rect(lowerX, lowerY, upperX, upperY);
    } else
      final_box.push_back(box);
  }
  guide_box.clear();
  guide_box = std::move(final_box);
}

odb::Rect GlobalRouter::globalRoutingToBox(const GSegment& route)
{
  odb::Rect die_bounds = grid_->getGridArea();
  int init_x, init_y;
  int final_x, final_y;

  if (route.init_x < route.final_x) {
    init_x = route.init_x;
    final_x = route.final_x;
  } else {
    init_x = route.final_x;
    final_x = route.init_x;
  }

  if (route.init_y < route.final_y) {
    init_y = route.init_y;
    final_y = route.final_y;
  } else {
    init_y = route.final_y;
    final_y = route.init_y;
  }

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
                                      GRoute& route)
{
  const int tile_size = grid_->getTileSize();
  int x0 = (tile_size * (route_bds.xMin() / tile_size)) + (tile_size / 2);
  int y0 = (tile_size * (route_bds.yMin() / tile_size)) + (tile_size / 2);

  const int x1 = (tile_size * (route_bds.xMax() / tile_size)) - (tile_size / 2);
  const int y1 = (tile_size * (route_bds.yMax() / tile_size)) - (tile_size / 2);

  if (x0 == x1 && y0 == y1)
    route.push_back(GSegment(x0, y0, layer, x1, y1, layer));

  while (y0 == y1 && (x0 + tile_size) <= x1) {
    route.push_back(GSegment(x0, y0, layer, x0 + tile_size, y0, layer));
    x0 += tile_size;
  }

  while (x0 == x1 && (y0 + tile_size) <= y1) {
    route.push_back(GSegment(x0, y0, layer, x0, y0 + tile_size, layer));
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
  long total_wirelength = 0;
  for (auto& net_route : routes_) {
    total_wirelength += computeNetWirelength(net_route.first);
  }
  if (verbose_)
    logger_->info(GRT,
                  18,
                  "Total wirelength: {} um",
                  total_wirelength / block_->getDefUnits());
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
        && segment1.init_layer == segment1.final_layer &&
        // segments are on the same layer
        segment0.init_layer == segment1.init_layer) {
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
    } else if (nearest_track2 > min && nearest_track2 < max) {
      track_position = horizontal
                           ? odb::Point(track_position.x(), nearest_track2)
                           : odb::Point(nearest_track2, track_position.y());
      return true;
    } else {
      return false;
    }
  }

  return false;
}

void GlobalRouter::createFakePin(Pin pin,
                                 odb::Point& pin_position,
                                 odb::dbTechLayer* layer,
                                 Net* net)
{
  int original_x = pin_position.x();
  int original_y = pin_position.y();
  int conn_layer = layer->getRoutingLevel();
  GSegment pin_connection;
  pin_connection.init_layer = conn_layer;
  pin_connection.final_layer = conn_layer;

  pin_connection.init_x = pin_position.x();
  pin_connection.final_x = pin_position.x();
  pin_connection.init_y = pin_position.y();
  pin_connection.final_y = pin_position.y();
  const bool is_port = pin.isPort();
  const PinEdge edge = pin.getEdge();

  if ((edge == PinEdge::east && !is_port)
      || (edge == PinEdge::west && is_port)) {
    const int new_x_position
        = pin_position.x() + (gcells_offset_ * grid_->getTileSize());
    if (new_x_position <= grid_->getXMax()) {
      pin_connection.init_x = new_x_position;
      pin_position.setX(new_x_position);
    }
  } else if ((edge == PinEdge::west && !is_port)
             || (edge == PinEdge::east && is_port)) {
    const int new_x_position
        = pin_position.x() - (gcells_offset_ * grid_->getTileSize());
    if (new_x_position >= grid_->getXMin()) {
      pin_connection.init_x = new_x_position;
      pin_position.setX(new_x_position);
    }
  } else if ((edge == PinEdge::north && !is_port)
             || (edge == PinEdge::south && is_port)) {
    const int new_y_position
        = pin_position.y() + (gcells_offset_ * grid_->getTileSize());
    if (new_y_position <= grid_->getYMax()) {
      pin_connection.init_y = new_y_position;
      pin_position.setY(new_y_position);
    }
  } else if ((edge == PinEdge::south && !is_port)
             || (edge == PinEdge::north && is_port)) {
    const int new_y_position
        = pin_position.y() - (gcells_offset_ * grid_->getTileSize());
    if (new_y_position >= grid_->getYMin()) {
      pin_connection.init_y = new_y_position;
      pin_position.setY(new_y_position);
    }
  } else {
    if (verbose_)
      logger_->warn(GRT, 33, "Pin {} has invalid edge.", pin.getName());
  }

  // keep init_x/y <= final_x/y
  int x_tmp = pin_connection.init_x;
  int y_tmp = pin_connection.init_y;
  pin_connection.init_x = std::min(x_tmp, pin_connection.final_x);
  pin_connection.init_y = std::min(y_tmp, pin_connection.final_y);

  pin_connection.final_x = std::max(x_tmp, pin_connection.final_x);
  pin_connection.final_y = std::max(y_tmp, pin_connection.final_y);

  // if there is already a pin with that fake position, don't add the gcell
  // capacity adjustment.
  auto& net_pad_pin_connection = pad_pins_connections_[net->getDbNet()];
  if (std::find(net_pad_pin_connection.begin(),
                net_pad_pin_connection.end(),
                pin_connection)
      != net_pad_pin_connection.end()) {
    return;
  }

  int pin_conn_init_x = pin_connection.init_x;
  int pin_conn_init_y = pin_connection.init_y;

  int pin_conn_final_x = pin_connection.final_x;
  int pin_conn_final_y = pin_connection.final_y;

  for (Pin& net_pin : net->getPins()) {
    if (net_pin.getName() != pin.getName()
        && !(net_pin.isConnectedToPadOrMacro() || net_pin.isPort())) {
      auto net_pin_pos = net_pin.getOnGridPosition();
      if (pin_connection.init_y == pin_connection.final_y) {
        if ((net_pin_pos.x() >= pin_conn_init_x)
            && (net_pin_pos.x() <= pin_conn_final_x)
            && (net_pin_pos.y() == pin_conn_init_y)) {
          pin_position.setX(original_x);
          return;
        }
      } else {
        if ((net_pin_pos.y() >= pin_conn_init_y)
            && (net_pin_pos.y() <= pin_conn_final_y)
            && (net_pin_pos.x() == pin_conn_init_x)) {
          pin_position.setY(original_y);
          return;
        }
      }
    }
  }

  pad_pins_connections_[net->getDbNet()].push_back(pin_connection);
}

odb::Point GlobalRouter::findFakePinPosition(Pin& pin, odb::dbNet* db_net)
{
  odb::Point fake_position = pin.getOnGridPosition();
  Net* net = db_net_map_[db_net];
  const int max_routing_layer = getNetMaxRoutingLayer(net);
  if ((pin.isConnectedToPadOrMacro() || pin.isPort()) && !net->isLocal()
      && gcells_offset_ != 0 && pin.getConnectionLayer() <= max_routing_layer) {
    odb::dbTechLayer* layer = routing_layers_[pin.getConnectionLayer()];
    createFakePin(pin, fake_position, layer, net);
  }

  return fake_position;
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
  odb::dbTechLayer* tech_layer = routing_layers_[layer_for_guide_dimension_];
  odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
  int track_spacing, track_init, num_tracks;
  track_grid->getAverageTrackSpacing(track_spacing, track_init, num_tracks);

  odb::Rect rect = block_->getDieArea();

  int dx = rect.dx();
  int dy = rect.dy();

  int tile_size = grid_->getPitchesInTile() * track_spacing;

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

void getViaDims(std::map<int, odb::dbTechVia*> default_vias,
                int level,
                int& width_up,
                int& prl_up,
                int& width_down,
                int& prl_down)
{
  width_up = -1;
  prl_up = -1;
  width_down = -1;
  prl_down = -1;
  if (default_vias.find(level) != default_vias.end()) {
    for (auto box : default_vias[level]->getBoxes()) {
      if (box->getTechLayer()->getRoutingLevel() == level) {
        width_up = std::min(box->getWidth(), box->getLength());
        prl_up = std::max(box->getWidth(), box->getLength());
        break;
      }
    }
  }
  if (default_vias.find(level - 1) != default_vias.end()) {
    for (auto box : default_vias[level - 1]->getBoxes()) {
      if (box->getTechLayer()->getRoutingLevel() == level) {
        width_down = std::min(box->getWidth(), box->getLength());
        prl_down = std::max(box->getWidth(), box->getLength());
        break;
      }
    }
  }
}

std::vector<std::pair<int, int>> GlobalRouter::calcLayerPitches(int max_layer)
{
  std::map<int, odb::dbTechVia*> default_vias = getDefaultVias(max_layer);
  std::vector<std::pair<int, int>> pitches(
      db_->getTech()->getRoutingLayerCount() + 1);
  for (auto const& [level, layer] : routing_layers_) {
    if (layer->getType() != odb::dbTechLayerType::ROUTING)
      continue;
    if (level > max_layer && max_layer > -1)
      break;
    pitches.push_back({-1, -1});

    int width_up, prl_up, width_down, prl_down;
    getViaDims(default_vias, level, width_up, prl_up, width_down, prl_down);
    bool up_via_valid = width_up != -1;
    bool down_via_valid = width_down != -1;
    if (!up_via_valid && !down_via_valid)
      continue;  // no default vias found
    int layer_width = layer->getWidth();
    int L2V_up = -1;
    int L2V_down = -1;
    // Priority for minSpc rule is SPACINGTABLE TWOWIDTHS > SPACINGTABLE PRL >
    // SPACING
    bool min_spc_valid = false;
    int min_spc_up = -1;
    int min_spc_down = -1;
    if (layer->hasTwoWidthsSpacingRules()) {
      min_spc_valid = true;
      if (up_via_valid)
        min_spc_up = layer->findTwSpacing(layer_width, width_up, prl_up);
      if (down_via_valid)
        min_spc_down = layer->findTwSpacing(layer_width, width_down, prl_down);
    } else if (layer->hasV55SpacingRules()) {
      min_spc_valid = true;
      if (up_via_valid)
        min_spc_up
            = layer->findV55Spacing(std::max(layer_width, width_up), prl_up);
      if (down_via_valid)
        min_spc_down = layer->findV55Spacing(std::max(layer_width, width_down),
                                             prl_down);
    } else {
      if (!layer->getV54SpacingRules().empty()) {
        min_spc_valid = true;
        int minSpc = 0;
        for (auto rule : layer->getV54SpacingRules())
          minSpc = rule->getSpacing();
        if (up_via_valid)
          min_spc_up = minSpc;
        if (down_via_valid)
          min_spc_down = minSpc;
      }
    }
    if (min_spc_valid) {
      if (up_via_valid)
        L2V_up = (level != max_routing_layer_)
                     ? (layer_width / 2) + (width_up / 2) + min_spc_up
                     : -1;
      if (down_via_valid)
        L2V_down = (level != min_routing_layer_)
                       ? (layer_width / 2) + (width_down / 2) + min_spc_down
                       : -1;
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
    if (verbose_)
      logger_->info(
          GRT,
          88,
          "Layer {:7s} Track-Pitch = {:.4f}  line-2-Via Pitch: {:.4f}",
          tech_layer->getName(),
          block_->dbuToMicrons(layer_tracks.getTrackPitch()),
          block_->dbuToMicrons(layer_tracks.getLineToViaPitch()));
  }
}

void GlobalRouter::computeCapacities(int max_layer)
{
  int h_capacity, v_capacity;

  for (auto const& [level, tech_layer] : routing_layers_) {
    if (level > max_layer && max_layer > -1) {
      break;
    }

    RoutingTracks routing_tracks = getRoutingTracksByIndex(level);
    int track_spacing = routing_tracks.getUsePitch();

    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      h_capacity = std::floor((float) grid_->getTileSize() / track_spacing);

      grid_->setHorizontalCapacity(h_capacity, level - 1);
      grid_->setVerticalCapacity(0, level - 1);
      debugPrint(logger_,
                 GRT,
                 "graph",
                 1,
                 "Layer {} has {} h-capacity",
                 tech_layer->getConstName(),
                 h_capacity);
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      v_capacity = std::floor((float) grid_->getTileSize() / track_spacing);

      grid_->setHorizontalCapacity(0, level - 1);
      grid_->setVerticalCapacity(v_capacity, level - 1);
      debugPrint(logger_,
                 GRT,
                 "graph",
                 1,
                 "Layer {} has {} v-capacity",
                 tech_layer->getConstName(),
                 v_capacity);
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

std::vector<Net*> GlobalRouter::findNets()
{
  initClockNets();

  std::vector<odb::dbNet*> db_nets;
  if (nets_to_route_.empty()) {
    db_nets.insert(
        db_nets.end(), block_->getNets().begin(), block_->getNets().end());
  } else {
    db_nets = nets_to_route_;
  }
  std::vector<Net*> clk_nets;
  for (odb::dbNet* db_net : db_nets) {
    Net* net = addNet(db_net);
    // add clock nets not connected to a leaf first
    if (net) {
      bool is_non_leaf_clock = isNonLeafClock(net->getDbNet());
      if (is_non_leaf_clock)
        clk_nets.push_back(net);
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
  std::sort(clk_nets.begin(), clk_nets.end(), nameLess);
  std::sort(non_clk_nets.begin(), non_clk_nets.end(), nameLess);

  std::vector<Net*> nets = std::move(clk_nets);
  nets.insert(nets.end(), non_clk_nets.begin(), non_clk_nets.end());

  return nets;
}

Net* GlobalRouter::addNet(odb::dbNet* db_net)
{
  if (!db_net->getSigType().isSupply() && !db_net->isSpecial()
      && db_net->getSWires().empty() && !db_net->isConnectedByAbutment()) {
    Net* net = new Net(db_net, db_net->getWire() != nullptr);
    db_net_map_[db_net] = net;
    makeItermPins(net, db_net, grid_->getGridArea());
    makeBtermPins(net, db_net, grid_->getGridArea());
    findPins(net);
    return net;
  }
  return nullptr;
}

void GlobalRouter::removeNet(odb::dbNet* db_net)
{
  Net* net = db_net_map_[db_net];
  fastroute_->removeNet(db_net);
  delete net;
  db_net_map_.erase(db_net);
  dirty_nets_.erase(db_net);
  routes_.erase(db_net);
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
  std::set<odb::dbNet*> clock_nets = sta_->findClkNets();

  if (verbose_)
    logger_->info(GRT, 19, "Found {} clock nets.", clock_nets.size());

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
  int max_routing_layer = (is_clock && max_layer_for_clock_ > 0)
                              ? max_layer_for_clock_
                              : max_routing_layer_;
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
      logger_->error(
          GRT,
          29,
          "Pin {} does not have geometries below the max routing layer ({}).",
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

    std::vector<odb::dbTechLayer*> pin_layers;
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

  if (verbose_)
    logger_->info(GRT, 4, "Blockages: {}", obstructions_cnt);
}

void GlobalRouter::findLayerExtensions(std::vector<int>& layer_extensions)
{
  layer_extensions.resize(routing_layers_.size() + 1, 0);

  int min_layer, max_layer;
  getMinMaxLayer(min_layer, max_layer);

  for (auto const& [level, obstruct_layer] : routing_layers_) {
    if (level >= min_layer && level <= max_layer) {
      int max_int = std::numeric_limits<int>::max();

      // Gets the smallest possible minimum spacing that won't cause violations
      // for ANY configuration of PARALLELRUNLENGTH (the biggest value in the
      // table)

      int spacing_extension = obstruct_layer->getSpacing(max_int, max_int);

      // Check for EOL spacing values and, if the spacing is higher than the one
      // found, use them as the macro extension instead of PARALLELRUNLENGTH

      for (auto rule : obstruct_layer->getV54SpacingRules()) {
        int spacing = rule->getSpacing();
        if (spacing > spacing_extension) {
          spacing_extension = spacing;
        }
      }

      // Check for TWOWIDTHS table values and, if the spacing is higher than the
      // one found, use them as the macro extension instead of PARALLELRUNLENGTH

      if (obstruct_layer->hasTwoWidthsSpacingRules()) {
        std::vector<std::vector<odb::uint>> spacing_table;
        obstruct_layer->getTwoWidthsSpacingTable(spacing_table);
        if (!spacing_table.empty()) {
          std::vector<odb::uint> last_row = spacing_table.back();
          odb::uint last_value = last_row.back();
          if (last_value > spacing_extension) {
            spacing_extension = last_value;
          }
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
    if (min_routing_layer_ <= layer && layer <= max_routing_layer_) {
      odb::Point lower_bound
          = odb::Point(obstruction_box->xMin(), obstruction_box->yMin());
      odb::Point upper_bound
          = odb::Point(obstruction_box->xMax(), obstruction_box->yMax());
      odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
      if (!die_area.contains(obstruction_rect)) {
        if (verbose_)
          logger_->warn(GRT, 37, "Found blockage outside die area.");
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
  // if layer is max or min, then all obs the nearest layer are added
  if (layer == max_routing_layer_
      && macro_obs_per_layer.find(layer - 1) != macro_obs_per_layer.end()) {
    extended_obs = macro_obs_per_layer.at(layer - 1);
  }
  if (layer == min_routing_layer_
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

  // sort vector by min Rect's xlo (increasing order)
  sort(upper_obs.begin(), upper_obs.end());
  sort(lower_obs.begin(), lower_obs.end());

  // Compare both vectors, find intersection between their rects
  for (const odb::Rect& cur_obs : upper_obs) {
    // start on first element to lower vector
    int pos = 0;
    while (pos < lower_obs.size() && lower_obs[pos].xMin() < cur_obs.xMax()) {
      // check if they have comun region (intersection)
      if (cur_obs.intersects(lower_obs[pos])) {
        // Get intersection rect
        odb::Rect inter_obs = cur_obs.intersect(lower_obs[pos]);
        // add rect in extended vector
        extended_obs.push_back(inter_obs);
      }
      pos++;
    }
  }

  return !extended_obs.empty();
}

// Add obstructions if they appear on upper and lower layer
void GlobalRouter::extendObstructions(
    std::unordered_map<int, std::vector<odb::Rect>>& macro_obs_per_layer,
    int bottom_layer,
    int top_layer)
{
  // if it has obs on min_layer + 1, then the min_layer needs to be block
  if (bottom_layer - 1 == min_routing_layer_) {
    bottom_layer--;
  }
  // if it has obs on max_layer - 1, then the max_layer needs to be block
  if (top_layer + 1 == max_routing_layer_) {
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

    bool isMacro = false;
    if (master->isBlock()) {
      macros_cnt++;
      isMacro = true;
    }

    if (isMacro) {
      std::unordered_map<int, std::vector<odb::Rect>> macro_obs_per_layer;
      int bottom_layer = std::numeric_limits<int>::max();
      int top_layer = std::numeric_limits<int>::min();

      for (odb::dbBox* box : master->getObstructions()) {
        int layer = box->getTechLayer()->getRoutingLevel();
        if (min_routing_layer_ <= layer && layer <= max_routing_layer_) {
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
        int layer_extension = layer_extensions[layer];
        layer_extension += macro_extension_ * grid_->getTileSize();
        for (odb::Rect& cur_obs : obs) {
          cur_obs.set_xlo(cur_obs.xMin() - layer_extension);
          cur_obs.set_ylo(cur_obs.yMin() - layer_extension);
          cur_obs.set_xhi(cur_obs.xMax() + layer_extension);
          cur_obs.set_yhi(cur_obs.yMax() + layer_extension);
          layer_obs_map[layer].push_back(cur_obs);
          applyObstructionAdjustment(cur_obs, tech->findRoutingLayer(layer));
        }
      }
    } else {
      for (odb::dbBox* box : master->getObstructions()) {
        int layer = box->getTechLayer()->getRoutingLevel();
        if (min_routing_layer_ <= layer && layer <= max_routing_layer_) {
          odb::Rect rect = box->getBox();
          transform.apply(rect);

          odb::Point lower_bound = odb::Point(rect.xMin(), rect.yMin());
          odb::Point upper_bound = odb::Point(rect.xMax(), rect.yMax());
          odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
          if (!die_area.contains(obstruction_rect)) {
            if (verbose_)
              logger_->warn(GRT,
                            38,
                            "Found blockage outside die area in instance {}.",
                            inst->getConstName());
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
          if (min_routing_layer_ <= pin_layer
              && pin_layer <= max_routing_layer_) {
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
    if (verbose_)
      logger_->error(
          GRT, 28, "Found {} pins outside die area.", pin_out_of_die_count);
  }

  if (verbose_)
    logger_->info(GRT, 3, "Macros: {}", macros_cnt);
  return obstructions_cnt;
}

void GlobalRouter::findNetsObstructions(odb::Rect& die_area)
{
  odb::dbSet<odb::dbNet> nets = block_->getNets();

  if (nets.empty()) {
    logger_->error(GRT, 94, "Design with no nets.");
  }

  for (odb::dbNet* db_net : nets) {
    odb::uint wire_cnt = 0, via_cnt = 0;
    db_net->getWireCount(wire_cnt, via_cnt);
    if (wire_cnt == 0)
      continue;

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

  if (min_routing_layer_ <= l && l <= max_routing_layer_) {
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

void GlobalRouter::getBlockage(odb::dbTechLayer* layer,
                               int x,
                               int y,
                               uint8_t& blockage_h,
                               uint8_t& blockage_v)
{
  int max_layer = std::max(max_routing_layer_, max_layer_for_clock_);
  if (layer->getRoutingLevel() <= max_layer) {
    fastroute_->getBlockage(layer, x, y, blockage_h, blockage_v);
  }
}

std::map<int, odb::dbTechVia*> GlobalRouter::getDefaultVias(
    int max_routing_layer)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbSet<odb::dbTechVia> vias = tech->getVias();
  std::map<int, odb::dbTechVia*> default_vias;

  for (odb::dbTechVia* via : vias) {
    odb::dbStringProperty* prop
        = odb::dbStringProperty::find(via, "OR_DEFAULT");

    if (prop == nullptr) {
      continue;
    } else {
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "Default via: {}.",
                 via->getConstName());
      default_vias[via->getBottomLayer()->getRoutingLevel()] = via;
    }
  }

  if (default_vias.empty()) {
    if (verbose_)
      logger_->info(GRT, 43, "No OR_DEFAULT vias defined.");
    for (int i = 1; i <= max_routing_layer; i++) {
      for (odb::dbTechVia* via : vias) {
        if (via->getBottomLayer()->getRoutingLevel() == i) {
          debugPrint(logger_,
                     utl::GRT,
                     "l2v_pitch",
                     1,
                     "Via for layers {} and {}: {}",
                     via->getBottomLayer()->getName(),
                     via->getTopLayer()->getName(),
                     via->getName());
          default_vias[i] = via;
          debugPrint(logger_,
                     utl::GRT,
                     "l2v_pitch",
                     1,
                     "Using via {} as default.",
                     via->getConstName());
          break;
        }
      }
    }
  }

  return default_vias;
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
    logger_->report("{:6d} {:6d} {:2d} -> {:6d} {:6d} {:2d}",
                    segment.init_x,
                    segment.init_y,
                    segment.init_layer,
                    segment.final_x,
                    segment.final_y,
                    segment.final_layer);
  }
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
      "Layer         Resource        Demand        Usage (%)    Max H / Max V "
      "/ Total Overflow");
  logger_->report(
      "------------------------------------------------------------------------"
      "---------------");

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
        "{:7s}      {:9}       {:7}        {:8.2f}%            {:2} / {:2} / "
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
      "------------------------------------------------------------------------"
      "---------------");
  logger_->report(
      "Total        {:9}       {:7}        {:8.2f}%            {:2} / {:2} / "
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
  for (size_t i = 0; i < lengths.size(); i++) {
    int64_t length = lengths[i];
    odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
    if (i > 0 && out.is_open()) {
      out << " " << via_count << " " << block_->dbuToMicrons(length);
    }
    if (length > 0) {
      logger_->report("\tLayer {:5s}: {:5.2f}um",
                      layer->getName(),
                      block_->dbuToMicrons(length));
    }
  }
}

void GlobalRouter::reportLayerWireLengths()
{
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
    for (size_t i = 0; i < lengths.size(); i++) {
      int64_t length = lengths[i];
      if (length > 0) {
        odb::dbTechLayer* layer = routing_layers_[i];
        logger_->report("{:5s} {:8d}um {:3d}%",
                        layer->getName(),
                        block_->dbuToMicrons(length),
                        static_cast<int>((100.0 * length) / total_length));
      }
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

  for (size_t i = 1; i < lengths.size(); i++) {
    int64_t length = lengths[i];
    odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(i);
    if (i > 0 && out.is_open()) {
      out << " " << via_count << " " << block_->dbuToMicrons(length);
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
  out << "tool "
      << "net "
      << "total_wl "
      << "#pins ";

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
  for (Pin& pin : net->getPins())
    pin_locs.push_back(PinGridLocation(
        pin.getITerm(), pin.getBTerm(), pin.getOnGridPosition()));
  return pin_locs;
}

PinGridLocation::PinGridLocation(odb::dbITerm* iterm,
                                 odb::dbBTerm* bterm,
                                 odb::Point pt)
    : iterm_(iterm), bterm_(bterm), pt_(pt)
{
}

////////////////////////////////////////////////////////////////

RoutePt::RoutePt(int x, int y, int layer) : x_(x), y_(y), layer_(layer)
{
}

bool operator<(const RoutePt& p1, const RoutePt& p2)
{
  return (p1.x_ < p2.x_) || (p1.x_ == p2.x_ && p1.y_ < p2.y_)
         || (p1.x_ == p2.x_ && p1.y_ == p2.y_ && p1.layer_ < p2.layer_);
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
  dirty_nets_.insert(net);
}

std::vector<Net*> GlobalRouter::updateDirtyRoutes(bool save_guides)
{
  std::vector<Net*> dirty_nets;
  if (!dirty_nets_.empty()) {
    fastroute_->setVerbose(false);
    if (verbose_)
      logger_->info(GRT, 9, "rerouting {} nets.", dirty_nets_.size());
    if (logger_->debugCheck(GRT, "incr", 2)) {
      debugPrint(logger_, GRT, "incr", 2, "Dirty nets:");
      for (auto net : dirty_nets_)
        debugPrint(logger_, GRT, "incr", 2, " {}", net->getConstName());
    }

    updateDirtyNets(dirty_nets);

    if (dirty_nets.empty()) {
      return dirty_nets;
    }

    const float old_critical_nets_percentage
        = fastroute_->getCriticalNetsPercentage();
    fastroute_->setCriticalNetsPercentage(0);
    fastroute_->setCongestionReportIterStep(0);

    initFastRouteIncr(dirty_nets);

    NetRouteMap new_route
        = findRouting(dirty_nets, min_routing_layer_, max_routing_layer_);
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
        // The nets that cross the congestion area are obtained and added to the
        // set
        fastroute_->getCongestionNets(congestion_nets);
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
        }
        // The dirty nets are initialized and then routed
        initFastRouteIncr(dirty_nets);
        NetRouteMap new_route
            = findRouting(dirty_nets, min_routing_layer_, max_routing_layer_);
        mergeResults(new_route);
        add_max--;
      }
      if (fastroute_->has2Doverflow()) {
        saveCongestion();
        logger_->error(GRT,
                       232,
                       "Routing congestion too high. Check the congestion "
                       "heatmap in the GUI.");
      }
    }
    fastroute_->setCriticalNetsPercentage(old_critical_nets_percentage);
    fastroute_->setCongestionReportIterStep(congestion_report_iter_step_);
    if (save_guides) {
      saveGuides();
    }
  }

  return dirty_nets;
}

void GlobalRouter::initFastRouteIncr(std::vector<Net*>& nets)
{
  initNetlist(nets);
  fastroute_->initAuxVar();
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

void GRouteDbCbk::inDbITermPreDisconnect(odb::dbITerm* iterm)
{
  // missing net pin update
  odb::dbNet* net = iterm->getNet();
  if (net != nullptr && !net->isSpecial()) {
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
  // missing net pin update
  odb::dbNet* net = bterm->getNet();
  if (net != nullptr && !net->isSpecial()) {
    grouter_->addDirtyNet(bterm->getNet());
  }
}

////////////////////////////////////////////////////////////////

GSegment::GSegment(int x0, int y0, int l0, int x1, int y1, int l1)
{
  init_x = std::min(x0, x1);
  init_y = std::min(y0, y1);
  init_layer = l0;
  final_x = std::max(x0, x1);
  final_y = std::max(y0, y1);
  final_layer = l1;
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

bool cmpById::operator()(odb::dbNet* net1, odb::dbNet* net2) const
{
  return net1->getId() < net2->getId();
}

}  // namespace grt
