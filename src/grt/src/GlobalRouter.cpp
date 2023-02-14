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

#include "FastRoute.h"
#include "Grid.h"
#include "MakeWireParasitics.h"
#include "RepairAntennas.h"
#include "RoutingTracks.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GRoute.h"
#include "gui/gui.h"
#include "heatMap.h"
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
      gui_(nullptr),
      stt_builder_(nullptr),
      antenna_checker_(nullptr),
      opendp_(nullptr),
      resizer_(nullptr),
      fastroute_(nullptr),
      grid_origin_(0, 0),
      groute_renderer_(nullptr),
      grid_(new Grid),
      routing_tracks_(new std::vector<RoutingTracks>),
      adjustment_(0.0),
      min_routing_layer_(1),
      max_routing_layer_(-1),
      layer_for_guide_dimension_(3),
      gcells_offset_(2),
      overflow_iterations_(50),
      allow_congestion_(false),
      macro_extension_(0),
      verbose_(false),
      min_layer_for_clock_(-1),
      max_layer_for_clock_(-2),
      critical_nets_percentage_(0),
      seed_(0),
      caps_perturbation_percentage_(0),
      perturbation_amount_(1),
      sta_(nullptr),
      db_(nullptr),
      block_(nullptr),
      repair_antennas_(nullptr),
      heatmap_(nullptr),
      congestion_file_name_(nullptr)
{
}

void GlobalRouter::init(utl::Logger* logger,
                        stt::SteinerTreeBuilder* stt_builder,
                        odb::dbDatabase* db,
                        sta::dbSta* sta,
                        rsz::Resizer* resizer,
                        ant::AntennaChecker* antenna_checker,
                        dpl::Opendp* opendp)
{
  logger_ = logger;
  // Broken gui api missing openroad accessor.
  gui_ = gui::Gui::get();
  stt_builder_ = stt_builder;
  db_ = db;
  stt_builder_ = stt_builder;
  antenna_checker_ = antenna_checker;
  opendp_ = opendp;
  fastroute_ = new FastRouteCore(db_, logger_, stt_builder_, gui_);
  sta_ = sta;
  resizer_ = resizer;

  heatmap_ = std::make_unique<RoutingCongestionDataSource>(logger_, db_);
  heatmap_->registerHeatMap();
}

void GlobalRouter::clear()
{
  routes_.clear();
  for (auto [ignored, net] : db_net_map_) {
    delete net;
  }
  db_net_map_.clear();
  routing_tracks_->clear();
  routing_layers_.clear();
  grid_->clear();
  fastroute_->clear();
  vertical_capacities_.clear();
  horizontal_capacities_.clear();
}

GlobalRouter::~GlobalRouter()
{
  delete routing_tracks_;
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
  initAdjustments();

  if (max_routing_layer < layer_for_guide_dimension_) {
    layer_for_guide_dimension_ = max_routing_layer;
  }

  fastroute_->setVerbose(verbose_);
  fastroute_->setOverflowIterations(overflow_iterations_);

  initRoutingLayers();
  reportLayerSettings(min_routing_layer, max_routing_layer);
  initRoutingTracks(max_routing_layer);
  initCoreGrid(max_routing_layer);
  setCapacities(min_routing_layer, max_routing_layer);

  std::vector<Net*> nets = initNetlist();
  initNets(nets);

  applyAdjustments(min_routing_layer, max_routing_layer);
  perturbCapacities();
  return nets;
}

void GlobalRouter::applyAdjustments(int min_routing_layer,
                                    int max_routing_layer)
{
  fastroute_->initEdges();
  computeGridAdjustments(min_routing_layer, max_routing_layer);
  computeTrackAdjustments(min_routing_layer, max_routing_layer);
  computeObstructionsAdjustments();
  std::vector<int> track_space = grid_->getMinWidths();
  fastroute_->initBlockedIntervals(track_space);
  computeUserGlobalAdjustments(min_routing_layer, max_routing_layer);
  computeUserLayerAdjustments(max_routing_layer);

  computePinOffsetAdjustments();

  for (RegionAdjustment region_adjustment : region_adjustments_) {
    odb::dbTechLayer* layer = routing_layers_[region_adjustment.getLayer()];
    logger_->report("Adjusting region on layer {}", layer->getName());
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
  if (congestion_file_name_ == nullptr) {
    return;
  }
  std::ofstream out(congestion_file_name_);

  std::vector<std::pair<GSegment, TileCongestion>> congestionGridsV,
      congestionGridsH;
  fastroute_->getCongestionGrid(congestionGridsV, congestionGridsH);
  for (auto& it : congestionGridsH) {
    out << "violation type: Horizontal congestion\n";
    const int capacity = it.second.first;
    const int usage = it.second.second;
    out << "\tsrcs: \n";
    out << "\tcongestion information: capacity:" << capacity
        << " usage:" << usage << " overflow:" << usage - capacity << "\n";
    odb::Rect rect = globalRoutingToBox(it.first);
    out << "\tbbox = ";
    out << "( " << dbuToMicrons(rect.xMin()) << ", "
        << dbuToMicrons(rect.yMin()) << " ) - ";
    out << "( " << dbuToMicrons(rect.xMax()) << ", "
        << dbuToMicrons(rect.yMax()) << ") on Layer -\n";
  }

  for (auto& it : congestionGridsV) {
    out << "violation type: Vertical congestion\n";
    const int capacity = it.second.first;
    const int usage = it.second.second;
    out << "\tsrcs: \n";
    out << "\tcongestion information: capacity:" << capacity
        << " usage:" << usage << " overflow:" << usage - capacity << "\n";
    odb::Rect rect = globalRoutingToBox(it.first);
    out << "\tbbox = ";
    out << "( " << dbuToMicrons(rect.xMin()) << ", "
        << dbuToMicrons(rect.yMin()) << " ) - ";
    out << "( " << dbuToMicrons(rect.xMax()) << ", "
        << dbuToMicrons(rect.yMax()) << ") on Layer -\n";
  }
}

bool GlobalRouter::haveRoutes()
{
  if (routes_.empty()) {
    loadGuidesFromDB();
  }

  return !routes_.empty();
}

void GlobalRouter::globalRoute(bool save_guides)
{
  clear();
  block_ = db_->getChip()->getBlock();

  if (max_routing_layer_ == -1) {
    max_routing_layer_ = computeMaxRoutingLayer();
  }

  int min_layer = min_layer_for_clock_ > 0
                      ? std::min(min_routing_layer_, min_layer_for_clock_)
                      : min_routing_layer_;
  int max_layer = std::max(max_routing_layer_, max_layer_for_clock_);

  std::vector<Net*> nets = initFastRoute(min_layer, max_layer);

  if (verbose_)
    reportResources();

  routes_ = findRouting(nets, min_layer, max_layer);
  updateDbCongestion();

  saveCongestion();

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
  if (fastroute_->totalOverflow() > 0 && verbose_) {
    logger_->warn(GRT, 115, "Global routing finished with overflow.");
  }

  if (verbose_)
    reportCongestion();
  computeWirelength();
  if (verbose_)
    logger_->info(GRT, 14, "Routed nets: {}", routes_.size());
  if (save_guides)
    saveGuides();
}

void GlobalRouter::updateDbCongestion()
{
  fastroute_->updateDbCongestion();
  heatmap_->update();
}

void GlobalRouter::repairAntennas(odb::dbMTerm* diode_mterm,
                                  int iterations,
                                  float ratio_margin)
{
  if (repair_antennas_ == nullptr)
    repair_antennas_
        = new RepairAntennas(this, antenna_checker_, opendp_, db_, logger_);

  if (diode_mterm == nullptr) {
    diode_mterm = repair_antennas_->findDiodeMTerm();
    if (diode_mterm == nullptr) {
      logger_->warn(
          GRT, 246, "No diode with LEF class CORE ANTENNACELL found.");
      return;
    }
  }
  if (repair_antennas_->diffArea(diode_mterm) == 0.0)
    logger_->error(GRT,
                   244,
                   "Diode {}/{} ANTENNADIFFAREA is zero.",
                   diode_mterm->getMaster()->getConstName(),
                   diode_mterm->getConstName());

  bool violations = true;
  int itr = 0;
  while (violations && itr < iterations) {
    if (verbose_)
      logger_->info(GRT, 6, "Repairing antennas, iteration {}.", itr + 1);
    violations = repair_antennas_->checkAntennaViolations(
        routes_, max_routing_layer_, diode_mterm, ratio_margin);
    if (violations) {
      IncrementalGRoute incr_groute(this, block_);
      repair_antennas_->repairAntennas(diode_mterm);
      logger_->info(
          GRT, 15, "Inserted {} diodes.", repair_antennas_->getDiodesCount());
      int illegal_diode_placement_count
          = repair_antennas_->illegalDiodePlacementCount();
      if (illegal_diode_placement_count > 0)
        logger_->info(GRT,
                      54,
                      "Using detailed placer to place {} diodes.",
                      illegal_diode_placement_count);
      repair_antennas_->legalizePlacedCells();
      incr_groute.updateRoutes();
    }
    repair_antennas_->clearViolations();
    itr++;
  }
  saveGuides();
}

void GlobalRouter::makeNetWires()
{
  if (repair_antennas_ == nullptr)
    repair_antennas_
        = new RepairAntennas(this, antenna_checker_, opendp_, db_, logger_);
  repair_antennas_->makeNetWires(routes_, max_routing_layer_);
}

void GlobalRouter::destroyNetWires()
{
  repair_antennas_->destroyNetWires();
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

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta_->deleteParasitics();

  // Make separate parasitics for each corner, same for min/max.
  sta_->setParasiticAnalysisPts(true, false);

  MakeWireParasitics builder(logger_, resizer_, sta_, db_->getTech(), this);
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
  MakeWireParasitics builder(logger_, resizer_, sta_, db_->getTech(), this);
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
  MakeWireParasitics builder(logger_, resizer_, sta_, db_->getTech(), this);
  return builder.routeLayerLengths(db_net);
}

////////////////////////////////////////////////////////////////

void GlobalRouter::initCoreGrid(int max_routing_layer)
{
  initGrid(max_routing_layer);

  computeCapacities(max_routing_layer);
  computeSpacingsAndMinWidth(max_routing_layer);

  fastroute_->setLowerLeft(grid_->getXMin(), grid_->getYMin());
  fastroute_->setTileSize(grid_->getTileSize());
  fastroute_->setGridsAndLayers(
      grid_->getXGrids(), grid_->getYGrids(), grid_->getNumLayers());
}

void GlobalRouter::initRoutingLayers()
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
      routing_layers_[valid_layers] = tech_layer;
      valid_layers++;
    }
  }

  odb::dbTechLayer* routing_layer = routing_layers_[1];
  bool vertical
      = routing_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
  fastroute_->setLayerOrientation(vertical);
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
      fastroute_->addHCapacity(grid_->getHorizontalEdgesCapacities()[l - 1], l);
      fastroute_->addVCapacity(grid_->getVerticalEdgesCapacities()[l - 1], l);

      horizontal_capacities_.push_back(
          grid_->getHorizontalEdgesCapacities()[l - 1]);
      vertical_capacities_.push_back(
          grid_->getVerticalEdgesCapacities()[l - 1]);
    }
  }

  for (int l = 1; l <= grid_->getNumLayers(); l++) {
    int new_cap_h = grid_->getHorizontalEdgesCapacities()[l - 1] * 100;
    grid_->updateHorizontalEdgesCapacities(l - 1, new_cap_h);

    int new_cap_v = grid_->getVerticalEdgesCapacities()[l - 1] * 100;
    grid_->updateVerticalEdgesCapacities(l - 1, new_cap_v);
  }
}

void GlobalRouter::setPerturbationAmount(int perturbation)
{
  perturbation_amount_ = perturbation;
};

void GlobalRouter::updateDirtyNets()
{
  initRoutingLayers();
  for (odb::dbNet* db_net : dirty_nets_) {
    Net* net = db_net_map_[db_net];
    net->destroyPins();
    makeItermPins(net, db_net, grid_->getGridArea());
    makeBtermPins(net, db_net, grid_->getGridArea());
    findPins(net);
  }
}

bool GlobalRouter::pinAccessPointPositions(
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

  if (access_points.empty())
    return false;

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

  has_access_points = pinAccessPointPositions(pin, ap_positions);

  std::vector<odb::Point> positions_on_grid;

  if (has_access_points) {
    for (const auto& ap_position : ap_positions) {
      pos_on_grid = ap_position.second;
      positions_on_grid.push_back(pos_on_grid);
    }
  } else {
    // if odb doesn't have any PAs, run the grt version considering the
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

void GlobalRouter::findPins(Net* net,
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
      bool invalid = false;
      for (RoutePt& pin_pos : pins_on_grid) {
        if (pinX == pin_pos.x() && pinY == pin_pos.y()
            && conn_layer == pin_pos.layer()) {
          invalid = true;
          break;
        }
      }

      if (!invalid) {
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

void GlobalRouter::computeNetSlacks()
{
  // Find the slack for all nets
  std::unordered_map<Net*, float> net_slack_map;
  std::vector<float> slacks;
  for (auto net_itr : db_net_map_) {
    Net* net = net_itr.second;
    float slack = getNetSlack(net);
    net_slack_map[net] = slack;
    slacks.push_back(slack);
  }
  std::stable_sort(slacks.begin(), slacks.end());

  // Find the slack threshold based on the percentage of critical nets
  // defined by the user
  int threshold_index
      = std::ceil(slacks.size() * critical_nets_percentage_ / 100);
  float slack_th = slacks[threshold_index];

  // Ensure the slack threshold is negative
  if (slack_th >= 0) {
    for (float slack : slacks) {
      if (slack >= 0)
        break;
      slack_th = slack;
    }
  }

  if (slack_th >= 0) {
    return;
  }

  // Add the slack values smaller than the threshold to the nets
  for (auto [net, slack] : net_slack_map) {
    if (slack <= slack_th) {
      net->setSlack(slack);
    }
  }
}

void GlobalRouter::initNets(std::vector<Net*>& nets)
{
  checkPinPlacement();
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

  if (critical_nets_percentage_ != 0) {
    computeNetSlacks();
  }

  for (Net* net : nets) {
    int pin_count = net->getNumPins();
    if (pin_count > 1 && !net->isLocal() && !net->hasWires()) {
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
    logger_->info(GRT, 1, "Minimum degree: {}", min_degree);
    logger_->info(GRT, 2, "Maximum degree: {}", max_degree);
  }
}

bool GlobalRouter::makeFastrouteNet(Net* net)
{
  std::vector<RoutePt> pins_on_grid;
  int root_idx;
  findPins(net, pins_on_grid, root_idx);

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
    getNetLayerRange(net, min_layer, max_layer);

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
    int min_pin_layer = std::numeric_limits<int>::max();
    for (RoutePt& pin_pos : pins_on_grid) {
      fr_net->addPin(pin_pos.x(), pin_pos.y(), pin_pos.layer() - 1);
      min_pin_layer = std::min(min_pin_layer, pin_pos.layer()) - 1;
    }
    fr_net->setMinLayer(std::max(min_pin_layer, min_layer - 1));
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

void GlobalRouter::getNetLayerRange(Net* net, int& min_layer, int& max_layer)
{
  int port_min_layer = std::numeric_limits<int>::max();
  for (const Pin& pin : net->getPins()) {
    if (pin.isPort() || pin.isConnectedToPadOrMacro()) {
      port_min_layer = std::min(port_min_layer, pin.getConnectionLayer());
    }
  }

  bool is_non_leaf_clock = isNonLeafClock(net->getDbNet());
  min_layer = (is_non_leaf_clock && min_layer_for_clock_ > 0)
                  ? min_layer_for_clock_
                  : min_routing_layer_;
  min_layer = std::min(min_layer, port_min_layer);
  max_layer = (is_non_leaf_clock && max_layer_for_clock_ > 0)
                  ? max_layer_for_clock_
                  : max_routing_layer_;
}

void GlobalRouter::computeTrackConsumption(
    const Net* net,
    int& track_consumption,
    std::vector<int>*& edge_costs_per_layer)
{
  edge_costs_per_layer = nullptr;
  track_consumption = 1;
  odb::dbNet* db_net = net->getDbNet();
  odb::dbTechNonDefaultRule* ndr = db_net->getNonDefaultRule();
  if (ndr) {
    int num_layers = grid_->getNumLayers();
    edge_costs_per_layer = new std::vector<int>(num_layers + 1, 1);
    std::vector<odb::dbTechLayerRule*> layer_rules;
    ndr->getLayerRules(layer_rules);

    for (odb::dbTechLayerRule* layer_rule : layer_rules) {
      int layerIdx = layer_rule->getLayer()->getRoutingLevel();
      RoutingTracks routing_tracks = getRoutingTracksByIndex(layerIdx);
      int default_width = layer_rule->getLayer()->getWidth();
      int default_pitch = routing_tracks.getTrackPitch();

      int ndr_spacing = layer_rule->getSpacing();
      int ndr_width = layer_rule->getWidth();
      int ndr_pitch = 2
                      * (std::ceil(ndr_width / 2 + ndr_spacing
                                   + default_width / 2 - default_pitch));

      int consumption = std::ceil((float) ndr_pitch / default_pitch);
      (*edge_costs_per_layer)[layerIdx - 1] = consumption;

      track_consumption = std::max(track_consumption, consumption);
    }
  }
}

void GlobalRouter::computeGridAdjustments(int min_routing_layer,
                                          int max_routing_layer)
{
  odb::Point upper_die_bounds(grid_->getXMax(), grid_->getYMax());
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
      h_space = grid_->getMinWidths()[level - 1];
      new_h_capacity = std::floor((grid_->getTileSize() + y_extra) / h_space);
    } else if (routing_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      v_space = grid_->getMinWidths()[level - 1];
      new_v_capacity = std::floor((grid_->getTileSize() + x_extra) / v_space);
    } else {
      logger_->error(GRT, 71, "Layer spacing not found.");
    }

    int num_adjustments = y_grids - 1 + x_grids - 1;
    fastroute_->setNumAdjustments(num_adjustments);

    if (!grid_->isPerfectRegularX()) {
      for (int i = 1; i < y_grids; i++) {
        fastroute_->addAdjustment(
            x_grids - 1, i - 1, x_grids - 1, i, level, new_v_capacity, false);
      }
    }
    if (!grid_->isPerfectRegularY()) {
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
      if (yh > 0) {
        odb::Rect init_track_obs(0, 0, grid_->getXMax(), yh);
        applyObstructionAdjustment(init_track_obs, layer);
      }

      /* top most obstruction */
      const int yl = final_track_location + track_space;
      if (yl < grid_->getYMax()) {
        odb::Rect final_track_obs(0, yl, grid_->getXMax(), grid_->getYMax());
        applyObstructionAdjustment(final_track_obs, layer);
      }
    } else {
      /* left most obstruction */
      const int xh = track_location - track_space;
      if (xh > 0) {
        const odb::Rect init_track_obs(0, 0, xh, grid_->getYMax());
        applyObstructionAdjustment(init_track_obs, layer);
      }

      /* right most obstruction */
      const int xl = final_track_location + track_space;
      if (xl < grid_->getXMax()) {
        const odb::Rect final_track_obs(
            xl, 0, grid_->getXMax(), grid_->getYMax());
        applyObstructionAdjustment(final_track_obs, layer);
      }
    }
  }
}

void GlobalRouter::computePinOffsetAdjustments()
{
  for (auto const& map_obj : pad_pins_connections_) {
    for (auto segment : map_obj.second) {
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
    if (adjustments_[l] == 0) {
      adjustments_[l] = adjustment_;
    }
  }
}

void GlobalRouter::computeUserLayerAdjustments(int max_routing_layer)
{
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  for (int layer = 1; layer <= max_routing_layer; layer++) {
    float adjustment = adjustments_[layer];
    if (adjustment != 0) {
      if (horizontal_capacities_[layer - 1] != 0) {
        int newCap = grid_->getHorizontalEdgesCapacities()[layer - 1]
                     * (1 - adjustment);
        grid_->updateHorizontalEdgesCapacities(layer - 1, newCap);

        for (int y = 1; y < y_grids; y++) {
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
        int newCap
            = grid_->getVerticalEdgesCapacities()[layer - 1] * (1 - adjustment);
        grid_->updateVerticalEdgesCapacities(layer - 1, newCap);

        for (int x = 1; x < x_grids; x++) {
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

  // If preferred direction is horizontal, only first and the last line will
  // have specific adjustments
  if (!vertical) {
    // Setting capacities of edges completely contains the adjust region
    // according the percentage of reduction
    fastroute_->applyHorizontalAdjustments(
        first_tile, last_tile, layer, first_tile_reduce, last_tile_reduce);
  } else {
    // If preferred direction is vertical, only first and last columns will have
    // specific adjustments
    fastroute_->applyVerticalAdjustments(
        first_tile, last_tile, layer, first_tile_reduce, last_tile_reduce);
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

  int track_space = grid_->getMinWidths()[layer - 1];

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
  initAdjustments();
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
  critical_nets_percentage_ = critical_nets_percentage;
}

void GlobalRouter::addLayerAdjustment(int layer, float reduction_percentage)
{
  initAdjustments();
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
  odb::dbTechLayer* max_tech_layer = tech->findRoutingLayer(max_routing_layer_);
  if (layer > max_routing_layer_ && max_routing_layer_ > 0) {
    if (verbose_)
      logger_->warn(GRT,
                    30,
                    "Specified layer {} for adjustment is greater than max "
                    "routing layer {} and will be ignored.",
                    tech_layer->getName(),
                    max_tech_layer->getName());
  } else {
    adjustments_[layer] = reduction_percentage;
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
        int newCap
            = grid_->getHorizontalEdgesCapacities()[layer - 1] + perturbation;
        newCap = newCap < 0 ? 0 : newCap;
        grid_->updateHorizontalEdgesCapacities(layer - 1, newCap);
        int edge_cap
            = fastroute_->getEdgeCapacity(x - 1, y - 1, x, y - 1, layer);
        int new_h_capacity = (edge_cap + perturbation);
        new_h_capacity = new_h_capacity < 0 ? 0 : new_h_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, x, y - 1, layer, new_h_capacity, subtract);
      } else if (vertical_capacities_[layer - 1] != 0) {
        int newCap
            = grid_->getVerticalEdgesCapacities()[layer - 1] + perturbation;
        newCap = newCap < 0 ? 0 : newCap;
        grid_->updateVerticalEdgesCapacities(layer - 1, newCap);
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
  if (max_routing_layer_ == -1 || routing_layers_.empty()) {
    max_routing_layer_ = computeMaxRoutingLayer();
    int min_layer = min_layer_for_clock_ > 0
                        ? std::min(min_routing_layer_, min_layer_for_clock_)
                        : min_routing_layer_;
    int max_layer = std::max(max_routing_layer_, max_layer_for_clock_);

    initRoutingLayers();
    initRoutingTracks(max_layer);
    initCoreGrid(max_layer);
    initAdjustments();
    setCapacities(min_layer, max_layer);
    applyAdjustments(min_layer, max_layer);
  }
  std::vector<Net*> nets = initNetlist();
  initNets(nets);
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
                      net->getName());
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

      if (seg1.isVia() && seg1.init_layer < seg2.init_layer
          && (seg1_init == seg2_init || seg1_init == seg2_final)) {
        seg1.final_layer = seg2.init_layer;
      } else if (seg2.isVia() && seg2.init_layer < seg1.init_layer
                 && (seg2_init == seg1_init || seg2_init == seg1_final)) {
        seg2.init_layer = seg1.final_layer;
      }
    }
  }
}

void GlobalRouter::updateEdgesUsage()
{
  for (const auto& [net, groute] : routes_) {
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
  if (db_gcell)
    db_gcell->resetGrid();
  else
    db_gcell = odb::dbGCellGrid::create(block);

  db_gcell->addGridPatternX(
      grid_->getXMin(), grid_->getXGrids(), grid_->getTileSize());
  db_gcell->addGridPatternY(
      grid_->getYMin(), grid_->getYGrids(), grid_->getTileSize());
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
      for (int x = 0; x < grid_->getXGrids() - 1; x++) {
        const unsigned short blockageH = capH - h_edges_3D[k][y][x].cap;
        const unsigned short blockageV = capV - v_edges_3D[k][y][x].cap;
        const unsigned short usageH = h_edges_3D[k][y][x].usage + blockageH;
        const unsigned short usageV = v_edges_3D[k][y][x].usage + blockageV;
        db_gcell->setCapacity(layer, x, y, capH, capV, 0);
        db_gcell->setUsage(layer, x, y, usageH, usageV, 0);
        db_gcell->setBlockage(layer, x, y, blockageH, blockageV, 0);
      }
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
    db_net->clearGuides();
    auto iter = routes_.find(db_net);
    if (iter == routes_.end()) {
      continue;
    }
    Net* net = db_net_map_[db_net];
    GRoute& route = iter->second;

    if (!route.empty()) {
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

          if (net->isLocal()) {
            int layer_idx1 = segment.init_layer;
            int layer_idx2 = segment.final_layer;
            odb::dbTechLayer* layer1 = routing_layers_[layer_idx1];
            odb::dbTechLayer* layer2 = routing_layers_[layer_idx2];
            odb::dbGuide::create(db_net, layer1, box);
            odb::dbGuide::create(db_net, layer2, box);
          } else {
            int layer_idx = std::min(segment.init_layer, segment.final_layer);
            odb::dbTechLayer* layer = routing_layers_[layer_idx];
            odb::dbGuide::create(db_net, layer, box);
            if (isCoveringPin(net, segment)) {
              odb::dbTechLayer* layer = routing_layers_[segment.final_layer];
              odb::dbGuide::create(db_net, layer, box);
            }
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
  for (auto pin : net->getPins()) {
    if (pin.getConnectionLayer() == segment.final_layer
        && pin.getOnGridPosition()
               == odb::Point(segment.final_x, segment.final_y)
        && (pin.isPort() || pin.isConnectedToPadOrMacro())) {
      return true;
    }
  }

  return false;
}

RoutingTracks GlobalRouter::getRoutingTracksByIndex(int layer)
{
  for (RoutingTracks routing_tracks : *routing_tracks_) {
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
        logger_->error(
            GRT, 76, "Net {} not properly covered.", db_net->getConstName());
      }
    }

    if (pins[p].getConnectionLayer() > last_layer)
      last_layer = pins[p].getConnectionLayer();
  }

  if (last_layer == max_routing_layer) {
    last_layer--;
  }

  for (int l = 1; l <= last_layer; l++) {
    odb::Point pin_pos = findFakePinPosition(pins[0], db_net);
    GSegment segment = GSegment(
        pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
    route.push_back(segment);
  }
}

void GlobalRouter::addGuidesForPinAccess(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  for (Pin& pin : pins) {
    if (pin.getConnectionLayer() > 1) {
      // for each pin placed at upper layers, get all segments that
      // potentially covers it
      GRoute cover_segs;

      odb::Point pin_pos = findFakePinPosition(pin, db_net);

      int wire_via_layer = std::numeric_limits<int>::max();
      for (size_t i = 0; i < route.size(); i++) {
        if (((pin_pos.x() == route[i].init_x && pin_pos.y() == route[i].init_y)
             || (pin_pos.x() == route[i].final_x
                 && pin_pos.y() == route[i].final_y))
            && (!(route[i].init_x == route[i].final_x
                  && route[i].init_y == route[i].final_y))) {
          cover_segs.push_back(route[i]);
          if (route[i].init_layer < wire_via_layer) {
            wire_via_layer = route[i].init_layer;
          }
        }
      }

      bool bottom_layer_pin = false;
      for (Pin& pin2 : pins) {
        odb::Point pin2_pos = pin2.getOnGridPosition();
        if (pin_pos.x() == pin2_pos.x() && pin_pos.y() == pin2_pos.y()
            && pin.getConnectionLayer() > pin2.getConnectionLayer()) {
          bottom_layer_pin = true;
        }
      }

      if (!bottom_layer_pin) {
        for (size_t i = 0; i < route.size(); i++) {
          if (((pin_pos.x() == route[i].init_x
                && pin_pos.y() == route[i].init_y)
               || (pin_pos.x() == route[i].final_x
                   && pin_pos.y() == route[i].final_y))
              && (route[i].init_x == route[i].final_x
                  && route[i].init_y == route[i].final_y
                  && (route[i].init_layer < wire_via_layer
                      || route[i].final_layer < wire_via_layer))) {
            // remove all vias to this pin that doesn't connects two wires
            route.erase(route.begin() + i);
            i--;
          }
        }
      }

      int closest_layer = -1;
      int minor_diff = std::numeric_limits<int>::max();

      for (GSegment& seg : cover_segs) {
        if (seg.init_layer != seg.final_layer) {
          logger_->error(GRT, 77, "Segment has invalid layer assignment.");
        }

        int diff_layers = std::abs(pin.getConnectionLayer() - seg.init_layer);
        if (diff_layers < minor_diff && seg.init_layer > closest_layer) {
          minor_diff = seg.init_layer;
          closest_layer = seg.init_layer;
        }
      }

      if (closest_layer > pin.getConnectionLayer()) {
        for (int l = closest_layer; l > pin.getConnectionLayer(); l--) {
          GSegment segment = GSegment(
              pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l - 1);
          route.push_back(segment);
        }
      } else if (closest_layer < pin.getConnectionLayer()) {
        for (int l = closest_layer; l < pin.getConnectionLayer(); l++) {
          GSegment segment = GSegment(
              pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
          route.push_back(segment);
        }
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
    if (net->getNumPins() > 1 && !net->hasWires()) {
      odb::dbNet* db_net = net->getDbNet();
      GRoute& route = routes[db_net];
      if (route.empty()) {
        addGuidesForLocalNets(
            db_net, route, min_routing_layer, max_routing_layer);
      } else {
        addGuidesForPinAccess(db_net, route);
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
        || net->getNumPins() > 1) {
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
  guide_box = final_box;
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
  if (!route.empty()) {
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

    size_t i = 0;
    while (i < segments.size() - 1) {
      GSegment& segment0 = segments[i];
      GSegment& segment1 = segments[i + 1];

      // both segments are not vias
      if (segment0.init_layer == segment0.final_layer
          && segment1.init_layer == segment1.final_layer &&
          // segments are on the same layer
          segment0.init_layer == segment1.init_layer) {
        // if segment 0 connects to the end of segment 1
        GSegment& new_seg = segments[i];
        if (segmentsConnect(segment0, segment1, new_seg, segs_at_point)) {
          segments[i] = new_seg;
          for (int idx = i + 1; idx < segments.size() - 1; idx++) {
            segments[idx] = segments[idx + 1];
          }
          segments.pop_back();
        } else {
          i++;
        }
      } else {
        i++;
      }
    }
  }
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

  int pin_conn_init_x = pin_connection.init_x;
  int pin_conn_init_y = pin_connection.init_y;

  int pin_conn_final_x = pin_connection.final_x;
  int pin_conn_final_y = pin_connection.final_y;

  for (Pin& net_pin : net->getPins()) {
    if (net_pin.getName() != pin.getName()) {
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

void GlobalRouter::initAdjustments()
{
  if (adjustments_.empty()) {
    adjustments_.resize(db_->getTech()->getRoutingLayerCount() + 1, 0);
  }
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
  int track_spacing = trackSpacing();

  odb::Rect rect = block_->getDieArea();

  int upper_rightX = rect.xMax();
  int upper_rightY = rect.yMax();

  int tile_size = grid_->getPitchesInTile() * track_spacing;

  int x_grids = upper_rightX / tile_size;
  int y_grids = upper_rightY / tile_size;

  bool perfect_regular_x = (x_grids * tile_size) == upper_rightX;
  bool perfect_regular_y = (y_grids * tile_size) == upper_rightY;

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

// Assumes initRoutingLayers and initRoutingTracks have been called
// to check layers and tracks.
int GlobalRouter::trackSpacing()
{
  odb::dbTechLayer* tech_layer = routing_layers_[layer_for_guide_dimension_];
  odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);

  if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    int track_step_y = -1;
    int init_track_y, num_tracks_y;
    track_grid->getGridPatternY(0, init_track_y, num_tracks_y, track_step_y);
    return track_step_y;
  } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    int track_step_x = -1;
    int init_track_x, num_tracks_x;
    track_grid->getGridPatternX(0, init_track_x, num_tracks_x, track_step_x);
    return track_step_x;
  }
  logger_->error(GRT, 82, "Cannot find track spacing.");
  return 0;
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
                 dbuToMicrons(layer_width));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_up : viaWidth = {:.4f} , prl = {:.4f} , minSpc = {:.4f} "
                 ", L2V = {:.4f} ",
                 dbuToMicrons(width_up),
                 dbuToMicrons(prl_up),
                 dbuToMicrons(min_spc_up),
                 dbuToMicrons(L2V_up));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_down : viaWidth = {:.4f} , prl = {:.4f} , minSpc = "
                 "{:.4f} , L2V = {:.4f} ",
                 dbuToMicrons(width_down),
                 dbuToMicrons(prl_down),
                 dbuToMicrons(min_spc_down),
                 dbuToMicrons(L2V_down));
    }
    pitches[level] = {L2V_up, L2V_down};
  }
  return pitches;
}

// For multiple track patterns we need to compute an average
// track pattern for gcell construction.
void GlobalRouter::averageTrackPattern(odb::dbTrackGrid* grid,
                                       bool is_x,
                                       int& track_init,
                                       int& num_tracks,
                                       int& track_step)
{
  std::vector<int> coordinates;
  if (is_x) {
    grid->getGridX(coordinates);
  } else {
    grid->getGridY(coordinates);
  }
  const int span = coordinates.back() - coordinates.front();
  track_init = coordinates.front();
  track_step = span / coordinates.size();
  num_tracks = coordinates.size();
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
    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      if (track_grid->getNumGridPatternsY() == 1) {
        track_grid->getGridPatternY(0, track_init, num_tracks, track_step);
      } else if (track_grid->getNumGridPatternsY() > 1) {
        averageTrackPattern(
            track_grid, false, track_init, num_tracks, track_step);
      } else {
        logger_->error(GRT,
                       124,
                       "Horizontal tracks for layer {} not found.",
                       tech_layer->getName());
        return;  // error throws
      }
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      if (track_grid->getNumGridPatternsX() == 1) {
        track_grid->getGridPatternX(0, track_init, num_tracks, track_step);
      } else if (track_grid->getNumGridPatternsX() > 1) {
        averageTrackPattern(
            track_grid, true, track_init, num_tracks, track_step);
      } else {
        logger_->error(GRT,
                       147,
                       "Vertical tracks for layer {} not found.",
                       tech_layer->getName());
        return;  // error throws
      }
    } else {
      logger_->error(
          GRT, 148, "Layer {} has invalid direction.", tech_layer->getName());
      return;  // error throws
    }

    RoutingTracks layer_tracks = RoutingTracks(level,
                                               track_step,
                                               l2vPitches[level].first,
                                               l2vPitches[level].second,
                                               track_init,
                                               num_tracks);
    routing_tracks_->push_back(layer_tracks);
    if (verbose_)
      logger_->info(
          GRT,
          88,
          "Layer {:7s} Track-Pitch = {:.4f}  line-2-Via Pitch: {:.4f}",
          tech_layer->getName(),
          dbuToMicrons(layer_tracks.getTrackPitch()),
          dbuToMicrons(layer_tracks.getLineToViaPitch()));
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

      grid_->addHorizontalCapacity(h_capacity, level - 1);
      grid_->addVerticalCapacity(0, level - 1);
      debugPrint(logger_,
                 GRT,
                 "graph",
                 1,
                 "Layer {} has {} h-capacity",
                 tech_layer->getConstName(),
                 h_capacity);
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      v_capacity = std::floor((float) grid_->getTileSize() / track_spacing);

      grid_->addHorizontalCapacity(0, level - 1);
      grid_->addVerticalCapacity(v_capacity, level - 1);
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

void GlobalRouter::computeSpacingsAndMinWidth(int max_layer)
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

    int min_spacing = 0;
    int min_width = 0;
    int track_step_x = -1;
    int track_step_y = -1;
    int init_track_x, num_tracks_x;
    int init_track_y, num_tracks_y;
    if (track->getNumGridPatternsX() > 0) {
      track->getGridPatternX(0, init_track_x, num_tracks_x, track_step_x);
    }
    if (track->getNumGridPatternsY() > 0) {
      track->getGridPatternY(0, init_track_y, num_tracks_y, track_step_y);
    }

    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      min_width = track_step_y;
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      min_width = track_step_x;
    }

    grid_->addSpacing(min_spacing, level - 1);
    grid_->addMinWidth(min_width, level - 1);
  }
}

static bool nameLess(const Net* a, const Net* b)
{
  return a->getName() < b->getName();
}

std::vector<Net*> GlobalRouter::initNetlist()
{
  initClockNets();

  std::vector<Net*> clk_nets;
  for (odb::dbNet* db_net : block_->getNets()) {
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

  std::vector<Net*> nets = clk_nets;
  nets.insert(nets.end(), non_clk_nets.begin(), non_clk_nets.end());

  return nets;
}

Net* GlobalRouter::addNet(odb::dbNet* db_net)
{
  if (!db_net->getSigType().isSupply() && !db_net->isSpecial()
      && db_net->getSWires().empty()) {
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
    odb::dbTransform transform;
    inst->getTransform(transform);

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

      for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
        odb::dbTechLayer* tech_layer = bpin_box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        const odb::Rect rect = bpin_box->getBox();
        if (!die_area.contains(rect) && verbose_) {
          logger_->warn(GRT, 36, "Pin {} is outside die area.", pin_name);
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
  odb::Rect die_area(
      grid_->getXMin(), grid_->getYMin(), grid_->getXMax(), grid_->getYMax());
  std::vector<int> layer_extensions;

  findLayerExtensions(layer_extensions);
  int obstructions_cnt = findObstructions(die_area);
  obstructions_cnt += findInstancesObstructions(die_area, layer_extensions);
  findNetsObstructions(die_area);

  if (verbose_)
    logger_->info(GRT, 4, "Blockages: {}", obstructions_cnt);
}

void GlobalRouter::findLayerExtensions(std::vector<int>& layer_extensions)
{
  layer_extensions.resize(routing_layers_.size() + 1, 0);

  for (auto const& [level, obstruct_layer] : routing_layers_) {
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
      std::vector<std::vector<uint>> spacing_table;
      obstruct_layer->getTwoWidthsSpacingTable(spacing_table);
      if (!spacing_table.empty()) {
        std::vector<uint> last_row = spacing_table.back();
        uint last_value = last_row.back();
        if (last_value > spacing_extension) {
          spacing_extension = last_value;
        }
      }
    }

    // Save the extension to use when defining Macros

    layer_extensions[level] = spacing_extension;
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
      applyObstructionAdjustment(obstruction_rect,
                                 obstruction_box->getTechLayer());
      obstructions_cnt++;
    }
  }

  return obstructions_cnt;
}

bool GlobalRouter::layerIsBlocked(
    int layer,
    odb::dbTechLayerDir& direction,
    const std::unordered_map<int, odb::Rect>& macro_obs_per_layer,
    odb::Rect& extended_obs)
{
  bool blocked_above = layer == (max_routing_layer_);
  bool blocked_below = layer == (min_routing_layer_);

  if (macro_obs_per_layer.find(layer + 1) != macro_obs_per_layer.end()) {
    const odb::Rect& layer_obs = macro_obs_per_layer.at(layer);
    const odb::Rect& upper_obs = macro_obs_per_layer.at(layer + 1);
    if (direction == odb::dbTechLayerDir::VERTICAL) {
      blocked_above = blocked_above ||  // upper layer out of range
                      upper_obs.xMin() < layer_obs.xMin()
                      ||                                    // west edge blocked
                      layer_obs.xMax() < upper_obs.xMax();  // east edge blocked
    } else {
      blocked_above
          = blocked_above ||                        // upper layer out of range
            upper_obs.yMin() < layer_obs.yMin() ||  // south edge blocked
            layer_obs.yMax() < upper_obs.yMax();    // north edge blocked
    }

    if (blocked_above) {
      extended_obs.set_xlo(std::min(extended_obs.xMin(), upper_obs.xMin()));
      extended_obs.set_ylo(std::min(extended_obs.yMin(), upper_obs.yMin()));
      extended_obs.set_xhi(std::max(extended_obs.xMax(), upper_obs.xMax()));
      extended_obs.set_yhi(std::max(extended_obs.yMax(), upper_obs.yMax()));
    }
  }

  if (macro_obs_per_layer.find(layer - 1) != macro_obs_per_layer.end()) {
    const odb::Rect& layer_obs = macro_obs_per_layer.at(layer);
    const odb::Rect& lower_obs = macro_obs_per_layer.at(layer - 1);
    if (direction == odb::dbTechLayerDir::VERTICAL) {
      blocked_below = blocked_below ||  // lower layer out of range
                      lower_obs.xMin() < layer_obs.xMin()
                      ||                                    // west edge blocked
                      layer_obs.xMax() < lower_obs.xMax();  // east edge blocked
    } else {
      blocked_below
          = blocked_below ||                        // lower layer out of range
            lower_obs.yMin() < layer_obs.yMin() ||  // south edge blocked
            layer_obs.yMax() < lower_obs.yMax();    // north edge blocked
    }

    if (blocked_below) {
      extended_obs.set_xlo(std::min(extended_obs.xMin(), lower_obs.xMin()));
      extended_obs.set_ylo(std::min(extended_obs.yMin(), lower_obs.yMin()));
      extended_obs.set_xhi(std::max(extended_obs.xMax(), lower_obs.xMax()));
      extended_obs.set_yhi(std::max(extended_obs.yMax(), lower_obs.yMax()));
    }
  }

  return blocked_above && blocked_below;
}

void GlobalRouter::extendObstructions(
    std::unordered_map<int, odb::Rect>& macro_obs_per_layer,
    int bottom_layer,
    int top_layer)
{
  odb::dbTech* tech = db_->getTech();
  for (int layer = bottom_layer; layer <= top_layer; layer++) {
    odb::Rect& obs = macro_obs_per_layer[layer];
    odb::Rect extended_obs = obs;
    odb::dbTechLayerDir direction
        = tech->findRoutingLayer(layer)->getDirection();
    if (layerIsBlocked(layer, direction, macro_obs_per_layer, extended_obs)) {
      if (direction == odb::dbTechLayerDir::VERTICAL) {
        // extend west and east edges
        obs.set_xlo(std::min(obs.xMin(), extended_obs.xMin()));
        obs.set_xhi(std::max(obs.xMax(), extended_obs.xMax()));
      } else {
        // extend south and north edges
        obs.set_ylo(std::min(obs.yMin(), extended_obs.yMin()));
        obs.set_yhi(std::max(obs.yMax(), extended_obs.yMax()));
      }
    }
  }
}

int GlobalRouter::findInstancesObstructions(
    odb::Rect& die_area,
    const std::vector<int>& layer_extensions)
{
  int macros_cnt = 0;
  int obstructions_cnt = 0;
  int pin_out_of_die_count = 0;
  odb::dbTech* tech = db_->getTech();
  for (odb::dbInst* inst : block_->getInsts()) {
    int pX, pY;

    odb::dbMaster* master = inst->getMaster();

    inst->getOrigin(pX, pY);
    odb::Point origin = odb::Point(pX, pY);

    odb::dbTransform transform(inst->getOrient(), origin);

    bool isMacro = false;
    if (master->isBlock()) {
      macros_cnt++;
      isMacro = true;
    }

    if (isMacro) {
      std::unordered_map<int, odb::Rect> macro_obs_per_layer;
      int bottom_layer = std::numeric_limits<int>::max();
      int top_layer = std::numeric_limits<int>::min();

      for (odb::dbBox* box : master->getObstructions()) {
        int layer = box->getTechLayer()->getRoutingLevel();
        if (min_routing_layer_ <= layer && layer <= max_routing_layer_) {
          odb::Rect rect = box->getBox();
          transform.apply(rect);

          if (macro_obs_per_layer.find(layer) == macro_obs_per_layer.end()) {
            macro_obs_per_layer[layer] = rect;
          } else {
            macro_obs_per_layer[layer].merge(rect);
          }
          obstructions_cnt++;

          bottom_layer = std::min(bottom_layer, layer);
          top_layer = std::max(top_layer, layer);
        }
      }

      extendObstructions(macro_obs_per_layer, bottom_layer, top_layer);

      for (auto& [layer, obs] : macro_obs_per_layer) {
        int layer_extension = layer_extensions[layer];
        layer_extension += macro_extension_ * grid_->getTileSize();
        obs.set_xlo(obs.xMin() - layer_extension);
        obs.set_ylo(obs.yMin() - layer_extension);
        obs.set_xhi(obs.xMax() + layer_extension);
        obs.set_yhi(obs.yMax() + layer_extension);
        applyObstructionAdjustment(obs, tech->findRoutingLayer(layer));
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
          applyObstructionAdjustment(obstruction_rect, box->getTechLayer());
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
            if (!die_area.contains(pin_box)) {
              logger_->error(GRT,
                             39,
                             "Found pin {} outside die area in instance {}.",
                             mterm->getConstName(),
                             inst->getConstName());
              pin_out_of_die_count++;
            }
            applyObstructionAdjustment(pin_box, box->getTechLayer());
          }
        }
      }
    }
  }

  if (pin_out_of_die_count > 0) {
    if (verbose_)
      logger_->warn(
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
    uint wire_cnt = 0, via_cnt = 0;
    db_net->getWireCount(wire_cnt, via_cnt);
    if (wire_cnt == 0)
      continue;

    if (db_net->getSigType().isSupply()) {
      for (odb::dbSWire* swire : db_net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            continue;
          } else {
            odb::Rect wire_rect = s->getBox();
            int l = s->getTechLayer()->getRoutingLevel();

            if (min_routing_layer_ <= l && l <= max_routing_layer_) {
              odb::Point lower_bound
                  = odb::Point(wire_rect.xMin(), wire_rect.yMin());
              odb::Point upper_bound
                  = odb::Point(wire_rect.xMax(), wire_rect.yMax());
              odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
              if (!die_area.contains(obstruction_rect)) {
                if (verbose_)
                  logger_->warn(GRT,
                                40,
                                "Net {} has wires outside die area.",
                                db_net->getConstName());
              }
              applyObstructionAdjustment(obstruction_rect, s->getTechLayer());
            }
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
          if (pshape.shape.isVia()) {
            continue;
          } else {
            odb::Rect wire_rect = pshape.shape.getBox();
            int l = pshape.shape.getTechLayer()->getRoutingLevel();

            if (min_routing_layer_ <= l && l <= max_routing_layer_) {
              odb::Point lower_bound
                  = odb::Point(wire_rect.xMin(), wire_rect.yMin());
              odb::Point upper_bound
                  = odb::Point(wire_rect.xMax(), wire_rect.yMax());
              odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
              if (!die_area.contains(obstruction_rect)) {
                if (verbose_)
                  logger_->warn(GRT,
                                41,
                                "Net {} has wires outside die area.",
                                db_net->getConstName());
              }
              applyObstructionAdjustment(obstruction_rect,
                                         pshape.shape.getTechLayer());
            }
          }
        }
      }
    }
  }
}

int GlobalRouter::computeMaxRoutingLayer()
{
  int max_routing_layer = -1;

  odb::dbTech* tech = db_->getTech();

  int valid_layers = 1;
  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(valid_layers);
    if (tech_layer->getRoutingLevel() != 0) {
      odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
      if (track_grid == nullptr) {
        break;
      }
      max_routing_layer = valid_layers;
      valid_layers++;
    }
  }

  return max_routing_layer;
}

double GlobalRouter::dbuToMicrons(int64_t dbu)
{
  return (double) dbu / (block_->getDbUnitsPerMicron());
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
      out << " " << via_count << " " << dbuToMicrons(length);
    }
    if (length > 0) {
      logger_->report(
          "\tLayer {:5s}: {:5.2f}um", layer->getName(), dbuToMicrons(length));
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
                        static_cast<int64_t>(dbuToMicrons(length)),
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
                  dbuToMicrons(wl));

    if (out.is_open()) {
      out << "grt: " << net->getName() << " " << dbuToMicrons(wl) << " "
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
                  dbuToMicrons(wl));

    if (out.is_open()) {
      out << "drt: " << net->getName() << " " << dbuToMicrons(wl) << " "
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
      out << " " << via_count << " " << dbuToMicrons(length);
    }
    if (length > 0) {
      logger_->report(
          "\tLayer {:5s}: {:5.2f}um", layer->getName(), dbuToMicrons(length));
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

void GlobalRouter::initDebugFastRoute()
{
  fastroute_->setDebugOn(true);
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

class GrouteRenderer : public gui::Renderer
{
 public:
  GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech);
  void highlight(odb::dbNet* net, bool show_pin_locations);
  void clear();
  virtual void drawLayer(odb::dbTechLayer* layer,
                         gui::Painter& painter) override;

 private:
  GlobalRouter* groute_;
  odb::dbTech* tech_;
  std::set<odb::dbNet*> nets_;
  std::unordered_map<odb::dbNet*, bool> show_pin_locations_;
};

// Highlight guide in the gui.
void GlobalRouter::highlightRoute(odb::dbNet* net, bool show_pin_locations)
{
  if (gui::Gui::enabled()) {
    if (groute_renderer_ == nullptr) {
      groute_renderer_ = new GrouteRenderer(this, db_->getTech());
      gui_->registerRenderer(groute_renderer_);
    }
    groute_renderer_->highlight(net, show_pin_locations);
  }
}

void GlobalRouter::clearRouteGui()
{
  if (groute_renderer_)
    groute_renderer_->clear();
}

void GrouteRenderer::clear()
{
  nets_.clear();
  show_pin_locations_.clear();
  redraw();
}

GrouteRenderer::GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech)
    : groute_(groute), tech_(tech)
{
}

void GrouteRenderer::highlight(odb::dbNet* net, bool show_pin_locations)
{
  nets_.insert(net);
  show_pin_locations_[net] = show_pin_locations;
  redraw();
}

void GrouteRenderer::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  painter.setPen(layer);
  painter.setBrush(layer);

  for (odb::dbNet* net : nets_) {
    Net* gr_net = groute_->getNet(net);
    if (show_pin_locations_[net]) {
      // draw on grid pin locations
      for (const Pin& pin : gr_net->getPins()) {
        if (pin.getConnectionLayer() == layer->getRoutingLevel()) {
          painter.drawCircle(pin.getOnGridPosition().x(),
                             pin.getOnGridPosition().y(),
                             (int) (groute_->getTileSize() / 1.5));
        }
      }
    }

    // draw guides
    NetRouteMap& routes = groute_->getRoutes();
    GRoute& groute = routes[const_cast<odb::dbNet*>(net)];
    for (GSegment& seg : groute) {
      int layer1 = seg.init_layer;
      int layer2 = seg.final_layer;
      if (layer1 != layer2) {
        continue;
      }
      odb::dbTechLayer* seg_layer = tech_->findRoutingLayer(layer1);
      if (seg_layer != layer) {
        continue;
      }
      // Draw rect because drawLine does not have a way to set the pen
      // thickness.
      odb::Rect rect = groute_->globalRoutingToBox(seg);
      painter.drawRect(rect);
    }
  }
}

////////////////////////////////////////////////////////////////

IncrementalGRoute::IncrementalGRoute(GlobalRouter* groute, odb::dbBlock* block)
    : groute_(groute), db_cbk_(groute)
{
  db_cbk_.addOwner(block);
}

void IncrementalGRoute::updateRoutes()
{
  groute_->updateDirtyRoutes();
}

IncrementalGRoute::~IncrementalGRoute()
{
  // Updating DB congestion is slow and only used for gui so
  // don't bother updating it.
  // groute_->updateDbCongestion();
  db_cbk_.removeOwner();
}

void GlobalRouter::addDirtyNet(odb::dbNet* net)
{
  dirty_nets_.insert(net);
}

void GlobalRouter::updateDirtyRoutes()
{
  if (!dirty_nets_.empty()) {
    fastroute_->setVerbose(false);
    if (verbose_)
      logger_->info(GRT, 9, "rerouting {} nets.", dirty_nets_.size());
    if (logger_->debugCheck(GRT, "incr", 2)) {
      debugPrint(logger_, GRT, "incr", 2, "Dirty nets:");
      for (auto net : dirty_nets_)
        debugPrint(logger_, GRT, "incr", 2, " {}", net->getConstName());
    }

    updateDirtyNets();
    std::vector<Net*> dirty_nets;
    dirty_nets.reserve(dirty_nets_.size());
    for (odb::dbNet* db_net : dirty_nets_) {
      dirty_nets.push_back(db_net_map_[db_net]);
    }
    initFastRouteIncr(dirty_nets);

    NetRouteMap new_route
        = findRouting(dirty_nets, min_routing_layer_, max_routing_layer_);
    mergeResults(new_route);
    dirty_nets_.clear();

    if (fastroute_->has2Doverflow() && !allow_congestion_) {
      logger_->error(GRT,
                     232,
                     "Routing congestion too high. Check the congestion "
                     "heatmap in the GUI.");
    }
  }
}

void GlobalRouter::initFastRouteIncr(std::vector<Net*>& nets)
{
  initNets(nets);
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
    if (db_net != nullptr && !db_net->isSpecial())
      grouter_->addDirtyNet(iterm->getNet());
  }
}

void GRouteDbCbk::inDbNetCreate(odb::dbNet* net)
{
  grouter_->addNet(net);
}

void GRouteDbCbk::inDbNetDestroy(odb::dbNet* net)
{
  grouter_->removeNet(net);
}

void GRouteDbCbk::inDbITermPreDisconnect(odb::dbITerm* iterm)
{
  // missing net pin update
  grouter_->addDirtyNet(iterm->getNet());
}

void GRouteDbCbk::inDbITermPostConnect(odb::dbITerm* iterm)
{
  // missing net pin update
  grouter_->addDirtyNet(iterm->getNet());
}

void GRouteDbCbk::inDbBTermPostConnect(odb::dbBTerm* bterm)
{
  // missing net pin update
  grouter_->addDirtyNet(bterm->getNet());
}

void GRouteDbCbk::inDbBTermPreDisconnect(odb::dbBTerm* bterm)
{
  // missing net pin update
  grouter_->addDirtyNet(bterm->getNet());
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
  std::size_t h1 = std::hash<int>()(seg.init_x * seg.init_y * seg.init_layer);
  std::size_t h2
      = std::hash<int>()(seg.final_x * seg.final_y * seg.final_layer);

  return h1 ^ h2;
}

bool cmpById::operator()(odb::dbNet* net1, odb::dbNet* net2) const
{
  return net1->getId() < net2->getId();
}

}  // namespace grt
