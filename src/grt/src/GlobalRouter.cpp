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
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AntennaRepair.h"
#include "FastRoute.h"
#include "Grid.h"
#include "MakeWireParasitics.h"
#include "RoutingLayer.h"
#include "RoutingTracks.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GRoute.h"
#include "gui/gui.h"
#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "ord/OpenRoad.hh"
#include "sta/Clock.hh"
#include "sta/Parasitics.hh"
#include "sta/Set.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace grt {

using utl::GRT;

void GlobalRouter::init(ord::OpenRoad* openroad)
{
  openroad_ = openroad;
  logger_ = openroad->getLogger();
  // Broken gui api missing openroad accessor.
  gui_ = gui::Gui::get();
  init();
}

void GlobalRouter::init()
{
  makeComponents();
  // Initialize variables
  adjustment_ = 0.0;
  min_routing_layer_ = 1;
  max_routing_layer_ = -1;
  overflow_iterations_ = 50;
  allow_congestion_ = false;
  macro_extension_ = 0;
  verbose_ = 0;
  alpha_ = 0.3;
  seed_ = 0;
  caps_perturbation_percentage_ = 0;
  perturbation_amount_ = 1;
}

void GlobalRouter::makeComponents()
{
  // Allocate memory for objects
  routing_tracks_ = new std::vector<RoutingTracks>;
  db_ = openroad_->getDb();
  fastroute_ = new FastRouteCore(db_, logger_);
  grid_ = new Grid;
  grid_origin_ = new odb::Point(0, 0);
  nets_ = new std::vector<Net>;
  sta_ = openroad_->getSta();
  routing_layers_ = new std::vector<RoutingLayer>;
}

void GlobalRouter::deleteComponents()
{
  delete routing_tracks_;
  delete fastroute_;
  delete grid_;
  delete grid_origin_;
  delete nets_;
  delete routing_layers_;
}

void GlobalRouter::clear()
{
  routes_.clear();
  nets_->clear();
  clearObjects();
}

void GlobalRouter::clearObjects()
{
  grid_->clear();
  fastroute_->clear();
  routing_tracks_->clear();
  routing_layers_->clear();
  vertical_capacities_.clear();
  horizontal_capacities_.clear();
}

GlobalRouter::~GlobalRouter()
{
  deleteComponents();
}

std::vector<Net*> GlobalRouter::startFastRoute(int min_routing_layer,
                                               int max_routing_layer,
                                               NetType type)
{
  initAdjustments();

  if (max_routing_layer < selected_metal_) {
    setSelectedMetal(max_routing_layer);
  }

  fastroute_->setVerbose(verbose_);
  fastroute_->setOverflowIterations(overflow_iterations_);
  fastroute_->setAllowOverflow(allow_congestion_);

  block_ = db_->getChip()->getBlock();
  reportLayerSettings(min_routing_layer, max_routing_layer);

  initRoutingLayers();
  initRoutingTracks(max_routing_layer);
  initCoreGrid(max_routing_layer);
  setCapacities(min_routing_layer, max_routing_layer);
  initNetlist();

  std::vector<Net*> nets;
  getNetsByType(type, nets);
  initializeNets(nets);
  applyAdjustments(min_routing_layer, max_routing_layer);
  perturbCapacities();

  return nets;
}

void GlobalRouter::applyAdjustments(int min_routing_layer, int max_routing_layer)
{
  computeGridAdjustments(min_routing_layer, max_routing_layer);
  computeTrackAdjustments(min_routing_layer, max_routing_layer);
  computeObstructionsAdjustments();
  computeUserGlobalAdjustments(min_routing_layer, max_routing_layer);
  computeUserLayerAdjustments(max_routing_layer);

  odb::dbTech* tech = db_->getTech();
  for (RegionAdjustment region_adjustment : region_adjustments_) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(region_adjustment.getLayer());
    logger_->report("Adjusting region on layer {}", layer->getName());
    computeRegionAdjustments(region_adjustment.getRegion(),
                             region_adjustment.getLayer(),
                             region_adjustment.getAdjustment());
  }

  fastroute_->initAuxVar();
}

void GlobalRouter::globalRouteClocksSeparately()
{
  // route clock nets
  std::vector<Net*> clock_nets
      = startFastRoute(min_layer_for_clock_, max_layer_for_clock_, NetType::Clock);
  reportResources();

  logger_->report("Routing clock nets...");
  routes_ = findRouting(clock_nets, min_layer_for_clock_, max_layer_for_clock_);
  Capacities clk_capacities
      = saveCapacities(min_layer_for_clock_, max_layer_for_clock_);
  clearObjects();
  logger_->info(GRT, 10, "Routed clock nets: {}", routes_.size());

  if (max_routing_layer_ == -1) {
    max_routing_layer_ = computeMaxRoutingLayer();
  }
  // route signal nets
  std::vector<Net*> signalNets
      = startFastRoute(min_routing_layer_, max_routing_layer_, NetType::Signal);
  restoreCapacities(clk_capacities, min_layer_for_clock_, max_layer_for_clock_);
  reportResources();

  // Store results in a temporary map, allowing to keep previous
  // routing result from clock nets
  NetRouteMap result
      = findRouting(signalNets, min_routing_layer_, max_routing_layer_);
  routes_.insert(result.begin(), result.end());
}

void GlobalRouter::globalRoute()
{
  if (max_routing_layer_ == -1) {
    max_routing_layer_ = computeMaxRoutingLayer();
  }

  std::vector<Net*> nets
      = startFastRoute(min_routing_layer_, max_routing_layer_, NetType::All);
  reportResources();

  routes_ = findRouting(nets, min_routing_layer_, max_routing_layer_);
}

void GlobalRouter::run()
{
  clear();

  globalRoute();

  reportCongestion();
  computeWirelength();
}

void GlobalRouter::repairAntennas(sta::LibertyPort* diode_port, int iterations)
{
  AntennaRepair antenna_repair = AntennaRepair(this,
                                              openroad_->getAntennaChecker(),
                                              openroad_->getOpendp(),
                                              db_,
                                              logger_);

  odb::dbMTerm* diode_mterm = sta_->getDbNetwork()->staToDb(diode_port);

  int violations_cnt = -1;
  int itr = 0;
  while (violations_cnt != 0 && itr < iterations) {
    logger_->info(GRT, 6, "Repairing antennas, iteration {}.", itr+1);
    // Copy first route result and make changes in this new vector
    NetRouteMap originalRoute(routes_);

    Capacities capacities = saveCapacities(min_routing_layer_, max_routing_layer_);
    addLocalConnections(originalRoute);

    violations_cnt = antenna_repair.checkAntennaViolations(
        originalRoute, max_routing_layer_, diode_mterm);

    if (violations_cnt > 0) {
      clearObjects();
      antenna_repair.repairAntennas(diode_mterm);
      antenna_repair.legalizePlacedCells();

      logger_->info(GRT, 15, "{} diodes inserted.", antenna_repair.getDiodesCount());

      updateDirtyNets();
      std::vector<Net*> antenna_nets
          = startFastRoute(min_routing_layer_, max_routing_layer_, NetType::Antenna);

      fastroute_->setVerbose(0);
      logger_->info(GRT, 9, "Nets to reroute: {}.", antenna_nets.size());

      restoreCapacities(capacities, min_routing_layer_, max_routing_layer_);
      removeDirtyNetsRouting();

      NetRouteMap new_route
          = findRouting(antenna_nets, min_routing_layer_, max_routing_layer_);
      mergeResults(new_route);
    }

    antenna_repair.clearViolations();
    dirty_nets_.clear();
    itr++;
  }
}

void GlobalRouter::addDirtyNet(odb::dbNet* net)
{
  dirty_nets_.insert(net);
}

NetRouteMap GlobalRouter::findRouting(std::vector<Net*>& nets,
                                      int min_routing_layer,
                                      int max_routing_layer)
{
  NetRouteMap routes = fastroute_->run();
  addRemainingGuides(routes, nets, min_routing_layer, max_routing_layer);
  connectPadPins(routes);
  for (auto& net_route : routes) {
    std::vector<Pin>& pins = db_net_map_[net_route.first]->getPins();
    GRoute& route = net_route.second;
    mergeSegments(pins, route);
  }

  return routes;
}

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta::dbSta* db_sta = openroad_->getSta();
  db_sta->deleteParasitics();

  MakeWireParasitics builder(openroad_, this);
  for (auto& net_route : routes_) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    if (!route.empty()) {
      Net* net = getNet(db_net);
      builder.estimateParasitcs(db_net, net->getPins(), route);
    }
  }
}

void GlobalRouter::initCoreGrid(int max_routing_layer)
{
  initGrid(max_routing_layer);

  computeCapacities(max_routing_layer);
  computeSpacingsAndMinWidth(max_routing_layer);
  initObstructions();

  fastroute_->setLowerLeft(grid_->getLowerLeftX(), grid_->getLowerLeftY());
  fastroute_->setTileSize(grid_->getTileWidth(), grid_->getTileHeight());
  fastroute_->setGridsAndLayers(
      grid_->getXGrids(), grid_->getYGrids(), grid_->getNumLayers());
}

void GlobalRouter::initRoutingLayers()
{
  initRoutingLayers(*routing_layers_);

  RoutingLayer routing_layer = getRoutingLayerByIndex(1);
  fastroute_->setLayerOrientation(routing_layer.getPreferredDirection());
}

void GlobalRouter::initRoutingTracks(int max_routing_layer)
{
  initRoutingTracks(*routing_tracks_, max_routing_layer);
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

      horizontal_capacities_.push_back(grid_->getHorizontalEdgesCapacities()[l - 1]);
      vertical_capacities_.push_back(grid_->getVerticalEdgesCapacities()[l - 1]);
    }
  }

  for (int l = 1; l <= grid_->getNumLayers(); l++) {
    int new_cap_h = grid_->getHorizontalEdgesCapacities()[l - 1] * 100;
    grid_->updateHorizontalEdgesCapacities(l - 1, new_cap_h);

    int new_cap_v = grid_->getVerticalEdgesCapacities()[l - 1] * 100;
    grid_->updateVerticalEdgesCapacities(l - 1, new_cap_v);
  }
}

Capacities GlobalRouter::saveCapacities(int previous_min_layer,
                                        int previous_max_layer)
{
  int old_cap;
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  auto gcell_grid = block_->getGCellGrid();

  Capacities capacities;

  CapacitiesVec& h_caps = capacities.getHorCapacities();
  CapacitiesVec& v_caps = capacities.getVerCapacities();

  h_caps.resize(grid_->getNumLayers());
  for (int l = 0; l < grid_->getNumLayers(); l++) {
    h_caps[l].resize(y_grids);
    for (int i = 0; i < y_grids; i++) {
      h_caps[l][i].resize(x_grids);
    }
  }

  v_caps.resize(grid_->getNumLayers());
  for (int l = 0; l < grid_->getNumLayers(); l++) {
    v_caps[l].resize(x_grids);
    for (int i = 0; i < x_grids; i++) {
      v_caps[l][i].resize(y_grids);
    }
  }

  for (int layer = previous_min_layer; layer <= previous_max_layer; layer++) {
    auto tech_layer = db_->getTech()->findRoutingLayer(layer);
    for (int y = 1; y < y_grids; y++) {
      for (int x = 1; x < x_grids; x++) {
        old_cap = getEdgeResource(x - 1, y - 1, x, y - 1, tech_layer, gcell_grid);
        h_caps[layer - 1][y - 1][x - 1] = old_cap;
      }
    }

    for (int x = 1; x < x_grids; x++) {
      for (int y = 1; y < y_grids; y++) {
        old_cap = getEdgeResource(x - 1, y - 1, x - 1, y, tech_layer, gcell_grid);
        v_caps[layer - 1][x - 1][y - 1] = old_cap;
      }
    }
  }

  return capacities;
}

void GlobalRouter::restoreCapacities(Capacities capacities,
                                     int previous_min_layer,
                                     int previous_max_layer)
{
  int old_cap;
  // Check if current edge capacity is larger than the old edge capacity
  // before applying adjustments.
  // After inserting diodes, edges can have less capacity than before,
  // and apply adjustment without a check leads to warns and wrong adjustments.
  int cap;
  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  const CapacitiesVec& h_caps = capacities.getHorCapacities();
  const CapacitiesVec& v_caps = capacities.getVerCapacities();

  for (int layer = previous_min_layer; layer <= previous_max_layer; layer++) {
    for (int y = 1; y < y_grids; y++) {
      for (int x = 1; x < x_grids; x++) {
        old_cap = h_caps[layer - 1][y - 1][x - 1];
        cap = fastroute_->getEdgeCapacity(x - 1, y - 1, layer, x, y - 1, layer);
        if (old_cap <= cap) {
          fastroute_->addAdjustment(
              x - 1, y - 1, layer, x, y - 1, layer, old_cap, true);
        }
      }
    }

    for (int x = 1; x < x_grids; x++) {
      for (int y = 1; y < y_grids; y++) {
        old_cap = v_caps[layer - 1][x - 1][y - 1];
        cap = fastroute_->getEdgeCapacity(x - 1, y - 1, layer, x - 1, y, layer);
        if (old_cap <= cap) {
          fastroute_->addAdjustment(
              x - 1, y - 1, layer, x - 1, y, layer, old_cap, true);
        }
      }
    }
  }
}

int GlobalRouter::getEdgeResource(int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  odb::dbTechLayer* tech_layer,
                                  odb::dbGCellGrid* gcell_grid)
{
  int resource = 0;

  if (y1 == y2) {
    resource = gcell_grid->getHorizontalCapacity(tech_layer, x1, y1)
               - gcell_grid->getHorizontalUsage(tech_layer, x1, y1);
  } else if (x1 == x2) {
    resource = gcell_grid->getVerticalCapacity(tech_layer, x1, y1)
               - gcell_grid->getVerticalUsage(tech_layer, x1, y1);
  }

  return resource;
}

void GlobalRouter::removeDirtyNetsRouting()
{
  for (odb::dbNet* db_net : dirty_nets_) {
    GRoute& net_route = routes_[db_net];
    for (GSegment& segment : net_route) {
      if (!(segment.init_layer != segment.final_layer || (segment.isVia()))) {
        odb::Point init_on_grid = grid_->getPositionOnGrid(
            odb::Point(segment.init_x, segment.init_y));
        odb::Point final_on_grid = grid_->getPositionOnGrid(
            odb::Point(segment.final_x, segment.final_y));

        if (init_on_grid.y() == final_on_grid.y()) {
          int min_x = (init_on_grid.x() <= final_on_grid.x()) ? init_on_grid.x()
                                                         : final_on_grid.x();
          int max_x = (init_on_grid.x() > final_on_grid.x()) ? init_on_grid.x()
                                                        : final_on_grid.x();

          min_x = (min_x - (grid_->getTileWidth() / 2)) / grid_->getTileWidth();
          max_x = (max_x - (grid_->getTileWidth() / 2)) / grid_->getTileWidth();
          int y = (init_on_grid.y() - (grid_->getTileHeight() / 2))
                  / grid_->getTileHeight();

          for (int x = min_x; x < max_x; x++) {
            int new_cap
                = fastroute_->getEdgeCurrentResource(
                      x, y, segment.init_layer, x + 1, y, segment.init_layer)
                  + 1;
            fastroute_->addAdjustment(x,
                                      y,
                                      segment.init_layer,
                                      x + 1,
                                      y,
                                      segment.init_layer,
                                      new_cap,
                                      false);
          }
        } else if (init_on_grid.x() == final_on_grid.x()) {
          int min_y = (init_on_grid.y() <= final_on_grid.y()) ? init_on_grid.y()
                                                         : final_on_grid.y();
          int max_y = (init_on_grid.y() > final_on_grid.y()) ? init_on_grid.y()
                                                        : final_on_grid.y();

          min_y = (min_y - (grid_->getTileHeight() / 2)) / grid_->getTileHeight();
          max_y = (max_y - (grid_->getTileHeight() / 2)) / grid_->getTileHeight();
          int x = (init_on_grid.x() - (grid_->getTileWidth() / 2))
                  / grid_->getTileWidth();

          for (int y = min_y; y < max_y; y++) {
            int new_cap
                = fastroute_->getEdgeCurrentResource(
                      x, y, segment.init_layer, x, y + 1, segment.init_layer)
                  + 1;
            fastroute_->addAdjustment(x,
                                      y,
                                      segment.init_layer,
                                      x,
                                      y + 1,
                                      segment.init_layer,
                                      new_cap,
                                      false);
          }
        } else {
          logger_->error(GRT, 70, "Invalid segment for net {}.", db_net->getConstName());
        }
      }
    }
  }
}

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

void GlobalRouter::findPins(Net* net)
{
  for (Pin& pin : net->getPins()) {
    odb::Point pin_position;
    int top_layer = pin.getTopLayer();
    RoutingLayer layer = getRoutingLayerByIndex(top_layer);

    std::vector<odb::Rect> pin_boxes = pin.getBoxes().at(top_layer);
    std::vector<odb::Point> pin_positions_on_grid;
    odb::Point pos_on_grid;

    for (odb::Rect pin_box : pin_boxes) {
      pos_on_grid = grid_->getPositionOnGrid(getRectMiddle(pin_box));
      pin_positions_on_grid.push_back(pos_on_grid);
    }

    int votes = -1;

    for (odb::Point pos : pin_positions_on_grid) {
      int equals = std::count(
          pin_positions_on_grid.begin(), pin_positions_on_grid.end(), pos);
      if (equals > votes) {
        pin_position = pos;
        votes = equals;
      }
    }

    if (pinOverlapsWithSingleTrack(pin, pos_on_grid)) {
      pos_on_grid = grid_->getPositionOnGrid(pos_on_grid);
      if (!(pos_on_grid == pin_position)
          && ((layer.getPreferredDirection() == RoutingLayer::HORIZONTAL
               && pos_on_grid.y() != pin_position.y())
              || (layer.getPreferredDirection() == RoutingLayer::VERTICAL
                  && pos_on_grid.x() != pin_position.x()))) {
        pin_position = pos_on_grid;
      }
    }

    pin.setOnGridPosition(pin_position);
  }
}

void GlobalRouter::findPins(Net* net, std::vector<RoutePt>& pins_on_grid, int& root_idx)
{
  findPins(net);

  root_idx = 0;
  for (Pin& pin : net->getPins()) {
    odb::Point pin_position = pin.getOnGridPosition();
    int top_layer = pin.getTopLayer();
    RoutingLayer layer = getRoutingLayerByIndex(top_layer);
    // If pin is connected to PAD, create a "fake" location in routing
    // grid to avoid PAD obstructions
    if ((pin.isConnectedToPad() || pin.isPort()) && !net->isLocal()) {
      GSegment pin_connection = createFakePin(pin, pin_position, layer);
      pad_pins_connections_[net->getDbNet()].push_back(pin_connection);
    }

    int pinX = (int) ((pin_position.x() - grid_->getLowerLeftX())
                      / grid_->getTileWidth());
    int pinY = (int) ((pin_position.y() - grid_->getLowerLeftY())
                      / grid_->getTileHeight());

    if (!(pinX < 0 || pinX >= grid_->getXGrids() || pinY < -1
          || pinY >= grid_->getYGrids() || top_layer > grid_->getNumLayers()
          || top_layer <= 0)) {
      bool invalid = false;
      for (RoutePt& pin_pos : pins_on_grid) {
        if (pinX == pin_pos.x() && pinY == pin_pos.y()
            && top_layer == pin_pos.layer()) {
          invalid = true;
          break;
        }
      }

      if (!invalid) {
        pins_on_grid.push_back(RoutePt(pinX, pinY, top_layer));
        if (pin.isDriver()) {
          root_idx = pins_on_grid.size()-1;
        }
      }
    }
  }
}

void GlobalRouter::initializeNets(std::vector<Net*>& nets)
{
  checkPinPlacement();
  pad_pins_connections_.clear();

  int valid_nets = 0;

  int min_degree = std::numeric_limits<int>::max();
  int max_degree = std::numeric_limits<int>::min();

  for (const Net& net : *nets_) {
    if (net.getNumPins() > 1) {
      valid_nets++;
    }
  }

  fastroute_->setNumberNets(valid_nets);
  fastroute_->setMaxNetDegree(getMaxNetDegree());

  if (seed_ != 0) {
    std::mt19937 g;
    g.seed(seed_);

    if (nets.size() > 1) {
      utl::shuffle(nets.begin(), nets.end(), g);
    }
  }

  for (Net* net : nets) {
    int pin_count = net->getNumPins();
    if (pin_count > 1 && !net->isLocal()) {
      if (pin_count < min_degree) {
        min_degree = pin_count;
      }

      if (pin_count > max_degree) {
        max_degree = pin_count;
      }

      std::vector<RoutePt> pins_on_grid;
      int root_idx;
      findPins(net, pins_on_grid, root_idx);

      // check if net is local in the global routing grid position
      // the (x,y) pin positions here may be different from the original
      // (x,y) pin positions because of findFakePinPosition function
      bool on_grid_local = true;
      RoutePt position = pins_on_grid[0];
      for (RoutePt& pin_pos : pins_on_grid) {
        if (pin_pos.x() != position.x() ||
            pin_pos.y() != position.y()) {
          on_grid_local = false;
          break;
        }
      }

      if (pins_on_grid.size() > 1 && !on_grid_local) {
        float net_alpha = alpha_;
        if (net_alpha_map_.find(net->getName()) != net_alpha_map_.end()) {
          net_alpha = net_alpha_map_[net->getName()];
        }
        bool is_clock = (net->getSignalType() == odb::dbSigType::CLOCK) &&
                         !clockHasLeafITerm(net->getDbNet());

        int num_layers = grid_->getNumLayers();
        std::vector<int> edge_cost_per_layer(num_layers + 1, 1);
        int edge_cost_for_net = computeTrackConsumption(net, edge_cost_per_layer);

        int min_layer = (is_clock && min_layer_for_clock_ > 0) ?
                         min_layer_for_clock_ : min_routing_layer_;
        int max_layer = (is_clock && max_layer_for_clock_ > 0) ?
                        max_layer_for_clock_ : max_routing_layer_;

        int netID = fastroute_->addNet(net->getDbNet(),
                                       pins_on_grid.size(),
                                       net_alpha,
                                       is_clock,
                                       root_idx,
                                       edge_cost_for_net,
                                       min_layer-1,
                                       max_layer-1,
                                       edge_cost_per_layer);
        for (RoutePt& pin_pos : pins_on_grid) {
          fastroute_->addPin(netID, pin_pos.x(), pin_pos.y(), pin_pos.layer()-1);
        }
      }
    }
  }

  logger_->info(GRT, 1, "Minimum degree: {}", min_degree);
  logger_->info(GRT, 2, "Maximum degree: {}", max_degree);

  fastroute_->initEdges();
}

int GlobalRouter::computeTrackConsumption(const Net* net,
                                          std::vector<int>& edge_costs_per_layer)
{
  int track_consumption = 1;
  odb::dbNet* db_net = net->getDbNet();
  odb::dbTechNonDefaultRule* ndr = db_net->getNonDefaultRule();
  if (ndr != nullptr) {
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
      edge_costs_per_layer[layerIdx - 1] = consumption;

      track_consumption = std::max(track_consumption, consumption);
    }
  }

  return track_consumption;
}

void GlobalRouter::computeGridAdjustments(int min_routing_layer,
                                          int max_routing_layer)
{
  odb::Point upper_die_bounds
      = odb::Point(grid_->getUpperRightX(), grid_->getUpperRightY());
  int h_space;
  int v_space;

  int x_grids = grid_->getXGrids();
  int y_grids = grid_->getYGrids();

  odb::Point upper_grid_bounds = odb::Point(x_grids * grid_->getTileWidth(),
                                          y_grids * grid_->getTileHeight());
  int x_extra = upper_die_bounds.x() - upper_grid_bounds.x();
  int y_extra = upper_die_bounds.y() - upper_grid_bounds.y();

  for (int layer = 1; layer <= grid_->getNumLayers(); layer++) {
    h_space = 0;
    v_space = 0;
    RoutingLayer routing_layer = getRoutingLayerByIndex(layer);

    if (layer < min_routing_layer
        || (layer > max_routing_layer && max_routing_layer > 0))
      continue;

    int new_v_capacity = 0;
    int new_h_capacity = 0;

    if (routing_layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
      h_space = grid_->getMinWidths()[layer - 1];
      new_h_capacity = std::floor((grid_->getTileHeight() + y_extra) / h_space);
    } else if (routing_layer.getPreferredDirection() == RoutingLayer::VERTICAL) {
      v_space = grid_->getMinWidths()[layer - 1];
      new_v_capacity = std::floor((grid_->getTileWidth() + x_extra) / v_space);
    } else {
      logger_->error(GRT, 71, "Layer spacing not found.");
    }

    int num_adjustments = y_grids - 1 + x_grids - 1;
    fastroute_->setNumAdjustments(num_adjustments);

    if (!grid_->isPerfectRegularX()) {
      for (int i = 1; i < y_grids; i++) {
        fastroute_->addAdjustment(x_grids - 1,
                                  i - 1,
                                  layer,
                                  x_grids - 1,
                                  i,
                                  layer,
                                  new_v_capacity,
                                  false);
      }
    }
    if (!grid_->isPerfectRegularY()) {
      for (int i = 1; i < x_grids; i++) {
        fastroute_->addAdjustment(i - 1,
                                  y_grids - 1,
                                  layer,
                                  i,
                                  y_grids - 1,
                                  layer,
                                  new_h_capacity,
                                  false);
      }
    }
  }
}

void GlobalRouter::computeTrackAdjustments(int min_routing_layer,
                                           int max_routing_layer)
{
  odb::Point upper_die_bounds
      = odb::Point(grid_->getUpperRightX(), grid_->getUpperRightY());
  for (RoutingLayer layer : *routing_layers_) {
    int track_location;
    int num_init_adjustments = 0;
    int num_final_adjustments = 0;
    int track_space;
    int num_tracks = 0;

    if (layer.getIndex() < min_routing_layer
        || (layer.getIndex() > max_routing_layer && max_routing_layer > 0))
      continue;

    if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
      RoutingTracks routing_tracks = getRoutingTracksByIndex(layer.getIndex());
      track_location = routing_tracks.getLocation();
      track_space = routing_tracks.getUsePitch();
      num_tracks = routing_tracks.getNumTracks();

      if (num_tracks > 0) {
        int final_track_location = track_location + (track_space * (num_tracks - 1));
        int remaining_final_space = upper_die_bounds.y() - final_track_location;
        int extra_space = upper_die_bounds.y()
                         - (grid_->getTileHeight() * grid_->getYGrids());
        if (grid_->isPerfectRegularY()) {
          num_final_adjustments
              = std::ceil((float) remaining_final_space / grid_->getTileHeight());
        } else {
          if (remaining_final_space != 0) {
            int final_space = remaining_final_space - extra_space;
            if (final_space <= 0)
              num_final_adjustments = 1;
            else
              num_final_adjustments
                  = std::ceil((float) final_space / grid_->getTileHeight());
          } else
            num_final_adjustments = 0;
        }

        num_final_adjustments *= grid_->getXGrids();
        num_init_adjustments
            = std::ceil((float) track_location / grid_->getTileHeight());
        num_init_adjustments *= grid_->getXGrids();
        fastroute_->setNumAdjustments(num_init_adjustments + num_final_adjustments);

        int y = 0;
        while (track_location >= grid_->getTileHeight()) {
          for (int x = 1; x < grid_->getXGrids(); x++) {
            fastroute_->addAdjustment(
                x - 1, y, layer.getIndex(), x, y, layer.getIndex(), 0, true);
          }
          y++;
          track_location -= grid_->getTileHeight();
        }
        if (track_location > 0) {
          int remaining_tile = grid_->getTileHeight() - track_location;
          int new_capacity = std::floor((float) remaining_tile / track_space);
          for (int x = 1; x < grid_->getXGrids(); x++) {
            fastroute_->addAdjustment(x - 1,
                                      y,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      new_capacity, true);
          }
        }

        y = grid_->getYGrids() - 1;
        while (remaining_final_space >= grid_->getTileHeight() + extra_space) {
          for (int x = 1; x < grid_->getXGrids(); x++) {
            fastroute_->addAdjustment(
                x - 1, y, layer.getIndex(), x, y, layer.getIndex(), 0, true);
          }
          y--;
          remaining_final_space -= (grid_->getTileHeight() + extra_space);
          extra_space = 0;
        }
        if (remaining_final_space > 0) {
          int remaining_tile
              = (grid_->getTileHeight() + extra_space) - remaining_final_space;
          int new_capacity = std::floor((float) remaining_tile / track_space);
          for (int x = 1; x < grid_->getXGrids(); x++) {
            fastroute_->addAdjustment(x - 1,
                                      y,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      new_capacity, true);
          }
        }
      }
    } else {
      RoutingTracks routing_tracks = getRoutingTracksByIndex(layer.getIndex());
      track_location = routing_tracks.getLocation();
      track_space = routing_tracks.getUsePitch();
      num_tracks = routing_tracks.getNumTracks();

      if (num_tracks > 0) {
        int final_track_location = track_location + (track_space * (num_tracks - 1));
        int remaining_final_space = upper_die_bounds.x() - final_track_location;
        int extra_space
            = upper_die_bounds.x() - (grid_->getTileWidth() * grid_->getXGrids());
        if (grid_->isPerfectRegularX()) {
          num_final_adjustments
              = std::ceil((float) remaining_final_space / grid_->getTileWidth());
        } else {
          if (remaining_final_space != 0) {
            int final_space = remaining_final_space - extra_space;
            if (final_space <= 0)
              num_final_adjustments = 1;
            else
              num_final_adjustments
                  = std::ceil((float) final_space / grid_->getTileWidth());
          } else
            num_final_adjustments = 0;
        }

        num_final_adjustments *= grid_->getYGrids();
        num_init_adjustments
            = std::ceil((float) track_location / grid_->getTileWidth());
        num_init_adjustments *= grid_->getYGrids();
        fastroute_->setNumAdjustments(num_init_adjustments + num_final_adjustments);

        int x = 0;
        while (track_location >= grid_->getTileWidth()) {
          for (int y = 1; y < grid_->getYGrids(); y++) {
            fastroute_->addAdjustment(
                x, y - 1, layer.getIndex(), x, y, layer.getIndex(), 0, true);
          }
          x++;
          track_location -= grid_->getTileWidth();
        }
        if (track_location > 0) {
          int remaining_tile = grid_->getTileWidth() - track_location;
          int new_capacity = std::floor((float) remaining_tile / track_space);
          for (int y = 1; y < grid_->getYGrids(); y++) {
            fastroute_->addAdjustment(x,
                                      y - 1,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      new_capacity,
                                      true);
          }
        }

        x = grid_->getXGrids() - 1;
        while (remaining_final_space >= grid_->getTileWidth() + extra_space) {
          for (int y = 1; y < grid_->getYGrids(); y++) {
            fastroute_->addAdjustment(
                x, y - 1, layer.getIndex(), x, y, layer.getIndex(), 0, true);
          }
          x--;
          remaining_final_space -= (grid_->getTileWidth() + extra_space);
          extra_space = 0;
        }
        if (remaining_final_space > 0) {
          int remaining_tile
              = (grid_->getTileWidth() + extra_space) - remaining_final_space;
          int new_capacity = std::floor((float) remaining_tile / track_space);
          for (int y = 1; y < grid_->getYGrids(); y++) {
            fastroute_->addAdjustment(x,
                                      y - 1,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      new_capacity,
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

  odb::dbTech* tech = db_->getTech();
  for (int layer = 1; layer <= max_routing_layer; layer++) {
    float adjustment = adjustments_[layer];
    if (adjustment != 0) {
      if (horizontal_capacities_[layer - 1] != 0) {
        int newCap = grid_->getHorizontalEdgesCapacities()[layer - 1]
                     * (1 - adjustment);
        grid_->updateHorizontalEdgesCapacities(layer - 1, newCap);

        for (int y = 1; y < y_grids; y++) {
          for (int x = 1; x < x_grids; x++) {
            int edge_cap = fastroute_->getEdgeCapacity(
                x - 1, y - 1, layer, x, y - 1, layer);
            int new_h_capacity = std::floor((float) edge_cap * (1 - adjustment));
            fastroute_->addAdjustment(
                x - 1, y - 1, layer, x, y - 1, layer, new_h_capacity, true);
          }
        }
      }

      if (vertical_capacities_[layer - 1] != 0) {
        int newCap
            = grid_->getVerticalEdgesCapacities()[layer - 1] * (1 - adjustment);
        grid_->updateVerticalEdgesCapacities(layer - 1, newCap);

        for (int x = 1; x < x_grids; x++) {
          for (int y = 1; y < y_grids; y++) {
            int edge_cap = fastroute_->getEdgeCapacity(
                x - 1, y - 1, layer, x - 1, y, layer);
            int new_v_capacity = std::floor((float) edge_cap * (1 - adjustment));
            fastroute_->addAdjustment(
                x - 1, y - 1, layer, x - 1, y, layer, new_v_capacity, true);
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
  odb::Rect first_tile_box;
  odb::Rect last_tile_box;
  std::pair<Grid::TILE, Grid::TILE> tiles_to_adjust;

  odb::Rect die_box = grid_->getGridArea();

  if ((die_box.xMin() > region.ll().x() && die_box.yMin() > region.ll().y())
      || (die_box.xMax() < region.ur().x() && die_box.yMax() < region.ur().y())) {
    logger_->error(GRT, 72, "Informed region is outside die area.");
  }

  RoutingLayer routing_layer = getRoutingLayerByIndex(layer);
  bool direction = routing_layer.getPreferredDirection();

  tiles_to_adjust = grid_->getBlockedTiles(region, first_tile_box, last_tile_box);
  Grid::TILE& first_tile = tiles_to_adjust.first;
  Grid::TILE& last_tile = tiles_to_adjust.second;

  RoutingTracks routing_tracks = getRoutingTracksByIndex(layer);
  int track_space = routing_tracks.getUsePitch();

  int first_tile_reduce = grid_->computeTileReduce(
      region, first_tile_box, track_space, true, direction);

  int last_tile_reduce = grid_->computeTileReduce(
      region, last_tile_box, track_space, false, direction);

  // If preferred direction is horizontal, only first and the last line will
  // have specific adjustments
  if (direction == RoutingLayer::HORIZONTAL) {
    // Setting capacities of edges completely contains the adjust region
    // according the percentage of reduction
    for (int x = first_tile._x; x < last_tile._x; x++) {
      for (int y = first_tile._y; y <= last_tile._y; y++) {
        int edge_cap = fastroute_->getEdgeCapacity(x, y, layer, x + 1, y, layer);

        if (y == first_tile._y) {
          edge_cap -= first_tile_reduce;
          if (edge_cap < 0)
            edge_cap = 0;
          fastroute_->addAdjustment(x, y, layer, x + 1, y, layer, edge_cap, true);
        } else if (y == last_tile._y) {
          edge_cap -= last_tile_reduce;
          if (edge_cap < 0)
            edge_cap = 0;
          fastroute_->addAdjustment(x, y, layer, x + 1, y, layer, edge_cap, true);
        } else {
          edge_cap -= edge_cap * reduction_percentage;
          fastroute_->addAdjustment(x, y, layer, x + 1, y, layer, 0, true);
        }
      }
    }
  } else {
    // If preferred direction is vertical, only first and last columns will have
    // specific adjustments
    for (int x = first_tile._x; x <= last_tile._x; x++) {
      // Setting capacities of edges completely contains the adjust region
      // according the percentage of reduction
      for (int y = first_tile._y; y < last_tile._y; y++) {
        int edge_cap = fastroute_->getEdgeCapacity(x, y, layer, x, y + 1, layer);

        if (x == first_tile._x) {
          edge_cap -= first_tile_reduce;
          if (edge_cap < 0)
            edge_cap = 0;
          fastroute_->addAdjustment(x, y, layer, x, y + 1, layer, edge_cap, true);
        } else if (x == last_tile._x) {
          edge_cap -= last_tile_reduce;
          if (edge_cap < 0)
            edge_cap = 0;
          fastroute_->addAdjustment(x, y, layer, x, y + 1, layer, edge_cap, true);
        } else {
          edge_cap -= edge_cap * reduction_percentage;
          fastroute_->addAdjustment(x, y, layer, x, y + 1, layer, 0, true);
        }
      }
    }
  }
}

void GlobalRouter::computeObstructionsAdjustments()
{
  std::map<int, std::vector<odb::Rect>> obstructions
      = grid_->getAllObstructions();

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer;
  for (int layer = 1; layer <= grid_->getNumLayers(); layer++) {
    std::vector<odb::Rect> layer_obstructions = obstructions[layer];
    if (!layer_obstructions.empty()) {
      RoutingLayer routing_layer = getRoutingLayerByIndex(layer);

      std::pair<Grid::TILE, Grid::TILE> blocked_tiles;

      bool direction = routing_layer.getPreferredDirection();

      tech_layer = tech->findRoutingLayer(layer);
      logger_->info(GRT, 17, "Processing {} blockages on layer {}.",
                    layer_obstructions.size(),
                    tech_layer->getName());

      int track_space = grid_->getMinWidths()[layer - 1];

      for (odb::Rect& obs : layer_obstructions) {
        if (obs.xMax() <= grid_->getLowerLeftX()
            || obs.xMin() >= grid_->getUpperRightX()
            || obs.yMax() <= grid_->getLowerLeftY()
            || obs.yMin() >= grid_->getUpperRightY()) {
          logger_->info(GRT, 209, "Ignoring an obstruction on layer {} outside the die area.",
              tech_layer->getName());
          continue;
        }

        odb::Rect first_tile_box;
        odb::Rect last_tile_box;

        blocked_tiles = grid_->getBlockedTiles(obs, first_tile_box, last_tile_box);

        Grid::TILE& first_tile = blocked_tiles.first;
        Grid::TILE& last_tile = blocked_tiles.second;

        int first_tile_reduce = grid_->computeTileReduce(
            obs, first_tile_box, track_space, true, direction);

        int last_tile_reduce = grid_->computeTileReduce(
            obs, last_tile_box, track_space, false, direction);

        if (direction == RoutingLayer::HORIZONTAL) {
          for (int x = first_tile._x; x < last_tile._x; x++) {
            for (int y = first_tile._y; y <= last_tile._y; y++) {
              if (y == first_tile._y) {
                int edge_cap
                    = fastroute_->getEdgeCapacity(x, y, layer, x + 1, y, layer);
                edge_cap -= first_tile_reduce;
                if (edge_cap < 0)
                  edge_cap = 0;
                fastroute_->addAdjustment(
                    x, y, layer, x + 1, y, layer, edge_cap, true);
              } else if (y == last_tile._y) {
                int edge_cap
                    = fastroute_->getEdgeCapacity(x, y, layer, x + 1, y, layer);
                edge_cap -= last_tile_reduce;
                if (edge_cap < 0)
                  edge_cap = 0;
                fastroute_->addAdjustment(
                    x, y, layer, x + 1, y, layer, edge_cap, true);
              } else {
                fastroute_->addAdjustment(x, y, layer, x + 1, y, layer, 0, true);
              }
            }
          }
        } else {
          for (int x = first_tile._x; x <= last_tile._x; x++) {
            for (int y = first_tile._y; y < last_tile._y; y++) {
              if (x == first_tile._x) {
                int edge_cap
                    = fastroute_->getEdgeCapacity(x, y, layer, x, y + 1, layer);
                edge_cap -= first_tile_reduce;
                if (edge_cap < 0)
                  edge_cap = 0;
                fastroute_->addAdjustment(
                    x, y, layer, x, y + 1, layer, edge_cap, true);
              } else if (x == last_tile._x) {
                int edge_cap
                    = fastroute_->getEdgeCapacity(x, y, layer, x, y + 1, layer);
                edge_cap -= last_tile_reduce;
                if (edge_cap < 0)
                  edge_cap = 0;
                fastroute_->addAdjustment(
                    x, y, layer, x, y + 1, layer, edge_cap, true);
              } else {
                fastroute_->addAdjustment(x, y, layer, x, y + 1, layer, 0, true);
              }
            }
          }
        }
      }
    }
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

void GlobalRouter::setAlpha(const float alpha)
{
  alpha_ = alpha;
}

void GlobalRouter::addLayerAdjustment(int layer, float reduction_percentage)
{
  initAdjustments();
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
  odb::dbTechLayer* max_tech_layer = tech->findRoutingLayer(max_routing_layer_);
  if (layer > max_routing_layer_ && max_routing_layer_ > 0) {
    logger_->warn(GRT, 30, "Specified layer {} for adjustment is greater than max routing layer {} and will be ignored.",
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
  region_adjustments_.push_back(
      RegionAdjustment(min_x, min_y, max_x, max_y, layer, reduction_percentage));
}

void GlobalRouter::addAlphaForNet(char* netName, float alpha)
{
  std::string name(netName);
  net_alpha_map_[name] = alpha;
}

void GlobalRouter::setVerbose(const int v)
{
  verbose_ = v;
}

void GlobalRouter::setOverflowIterations(int iterations)
{
  overflow_iterations_ = iterations;
}

void GlobalRouter::setGridOrigin(long x, long y)
{
  *grid_origin_ = odb::Point(x, y);
}

void GlobalRouter::setAllowCongestion(bool allow_congestion)
{
  allow_congestion_ = allow_congestion;
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

  int num_2d_grids = x_grids*y_grids;
  int num_perturbations = (caps_perturbation_percentage_/100)*num_2d_grids;

  std::mt19937 g;
  g.seed(seed_);

  for (int layer = 1; layer <= max_routing_layer_; layer++) {
    std::uniform_int_distribution<int> uni_x(1, x_grids-1);
    std::uniform_int_distribution<int> uni_y(1, y_grids-1);
    std::bernoulli_distribution add_or_subtract;

    for (int i = 0; i < num_perturbations; i++) {
      int x = uni_x(g);
      int y = uni_y(g);
      bool subtract = add_or_subtract(g);
      int perturbation = subtract ? -perturbation_amount_ : perturbation_amount_;
      if (horizontal_capacities_[layer - 1] != 0) {
        int newCap = grid_->getHorizontalEdgesCapacities()[layer - 1] + perturbation;
        newCap = newCap < 0 ? 0 : newCap;
        grid_->updateHorizontalEdgesCapacities(layer - 1, newCap);
        int edge_cap = fastroute_->getEdgeCapacity(
            x - 1, y - 1, layer, x, y - 1, layer);
        int new_h_capacity = (edge_cap + perturbation);
        new_h_capacity = new_h_capacity < 0 ? 0 : new_h_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, layer, x, y - 1, layer, new_h_capacity, subtract);
      } else if (vertical_capacities_[layer - 1] != 0) {
        int newCap = grid_->getVerticalEdgesCapacities()[layer - 1] + perturbation;
        newCap = newCap < 0 ? 0 : newCap;
        grid_->updateVerticalEdgesCapacities(layer - 1, newCap);
        int edge_cap = fastroute_->getEdgeCapacity(
            x - 1, y - 1, layer, x - 1, y, layer);
        int new_v_capacity = (edge_cap + perturbation);
        new_v_capacity = new_v_capacity < 0 ? 0 : new_v_capacity;
        fastroute_->addAdjustment(
            x - 1, y - 1, layer, x - 1, y, layer, new_v_capacity, subtract);
      }
    }
  }
}

void GlobalRouter::writeGuides(const char* file_name)
{
  std::ofstream guide_file;
  guide_file.open(file_name);
  if (!guide_file.is_open()) {
    guide_file.close();
    logger_->error(GRT, 73, "Guides file could not be opened.");
  }
  RoutingLayer ph_layer_final;

  int offset_x = grid_origin_->x();
  int offset_y = grid_origin_->y();

  logger_->info(GRT, 14, "Routed nets: {}", routes_.size());
  int final_layer;

  // Sort nets so guide file net order is consistent.
  std::vector<odb::dbNet*> sorted_nets;
  for (odb::dbNet* net : block_->getNets())
    sorted_nets.push_back(net);
  std::sort(sorted_nets.begin(),
            sorted_nets.end(),
            [](odb::dbNet* net1, odb::dbNet* net2) {
              return strcmp(net1->getConstName(), net2->getConstName()) < 0;
            });

  for (odb::dbNet* db_net : sorted_nets) {
    GRoute& route = routes_[db_net];
    if (!route.empty()) {
      guide_file << db_net->getConstName() << "\n";
      guide_file << "(\n";
      std::vector<odb::Rect> guide_box;
      final_layer = -1;
      for (GSegment& segment : route) {
        if (segment.init_layer != final_layer && final_layer != -1) {
          mergeBox(guide_box);
          for (odb::Rect& guide : guide_box) {
            guide_file << guide.xMin() + offset_x << " " << guide.yMin() + offset_y
                      << " " << guide.xMax() + offset_x << " "
                      << guide.yMax() + offset_y << " " << ph_layer_final.getName()
                      << "\n";
          }
          guide_box.clear();
          final_layer = segment.init_layer;
        }
        if (segment.init_layer == segment.final_layer) {
          if (segment.init_layer < min_routing_layer_
              && segment.init_x != segment.final_x
              && segment.init_y != segment.final_y) {
            logger_->error(GRT, 74, "Routing with guides in blocked metal for net {}.",
                           db_net->getConstName());
          }

          guide_box.push_back(globalRoutingToBox(segment));
          ph_layer_final = getRoutingLayerByIndex(segment.final_layer);
          final_layer = segment.final_layer;
        } else {
          if (abs(segment.final_layer - segment.init_layer) > 1) {
            logger_->error(GRT, 75, "Connection between non-adjacent layers in net {}.",
                           db_net->getConstName());
          } else {
            RoutingLayer ph_layer_init;
            ph_layer_init = getRoutingLayerByIndex(segment.init_layer);
            ph_layer_final = getRoutingLayerByIndex(segment.final_layer);

            final_layer = segment.final_layer;
            odb::Rect box;
            guide_box.push_back(globalRoutingToBox(segment));
            mergeBox(guide_box);
            for (odb::Rect& guide : guide_box) {
              guide_file << guide.xMin() + offset_x << " "
                        << guide.yMin() + offset_y << " "
                        << guide.xMax() + offset_x << " "
                        << guide.yMax() + offset_y << " " << ph_layer_init.getName()
                        << "\n";
            }
            guide_box.clear();

            guide_box.push_back(globalRoutingToBox(segment));
          }
        }
      }
      mergeBox(guide_box);
      for (odb::Rect& guide : guide_box) {
        guide_file << guide.xMin() + offset_x << " " << guide.yMin() + offset_y
                  << " " << guide.xMax() + offset_x << " "
                  << guide.yMax() + offset_y << " " << ph_layer_final.getName()
                  << "\n";
      }
      guide_file << ")\n";
    }
  }

  guide_file.close();
}

RoutingLayer GlobalRouter::getRoutingLayerByIndex(int index)
{
  if (routing_layers_->empty()) {
    logger_->error(GRT, 42, "Routing layers were not initialized.");
  }

  RoutingLayer layer = routing_layers_->front();

  for (RoutingLayer routing_layer : *routing_layers_) {
    if (routing_layer.getIndex() == index) {
      layer = routing_layer;
      break;
    }
  }

  if (layer.getIndex() != index) {
    logger_->error(GRT, 220, "Routing layer of index {} not found.", index);
  }

  return layer;
}

RoutingTracks GlobalRouter::getRoutingTracksByIndex(int layer)
{
  RoutingTracks tracks;

  for (RoutingTracks routing_tracks : *routing_tracks_) {
    if (routing_tracks.getLayerIndex() == layer) {
      tracks = routing_tracks;
    }
  }

  return tracks;
}

void GlobalRouter::addGuidesForLocalNets(odb::dbNet* db_net,
                                         GRoute& route,
                                         int min_routing_layer,
                                         int max_routing_layer)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  int last_layer = -1;
  for (uint p = 0; p < pins.size(); p++) {
    if (p > 0) {
      odb::Point pin_pos0 = findFakePinPosition(pins[p - 1], db_net);
      odb::Point pin_pos1 = findFakePinPosition(pins[p], db_net);
      // If the net is not local, FR core result is invalid
      if (pin_pos1.x() != pin_pos0.x() || pin_pos1.y() != pin_pos0.y()) {
        logger_->error(GRT, 76, "Net {} not properly covered.", db_net->getConstName());
      }
    }

    if (pins[p].getTopLayer() > last_layer)
      last_layer = pins[p].getTopLayer();
  }

  if (last_layer == max_routing_layer) {
    last_layer--;
  }

  for (int l = 1; l <= last_layer; l++) {
    odb::Point pin_pos = findFakePinPosition(pins[0], db_net);
    GSegment segment
        = GSegment(pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l + 1);
    route.push_back(segment);
  }
}

void GlobalRouter::addGuidesForPinAccess(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = db_net_map_[db_net]->getPins();
  for (Pin& pin : pins) {
    if (pin.getTopLayer() > 1) {
      // for each pin placed at upper layers, get all segments that
      // potentially covers it
      GRoute cover_segs;

      odb::Point pin_pos = findFakePinPosition(pin, db_net);

      int wire_via_layer = std::numeric_limits<int>::max();
      for (uint i = 0; i < route.size(); i++) {
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
            && pin.getTopLayer() > pin2.getTopLayer()) {
          bottom_layer_pin = true;
        }
      }

      if (!bottom_layer_pin) {
        for (uint i = 0; i < route.size(); i++) {
          if (((pin_pos.x() == route[i].init_x && pin_pos.y() == route[i].init_y)
               || (pin_pos.x() == route[i].final_x
                   && pin_pos.y() == route[i].final_y))
              && (route[i].init_x == route[i].final_x
                  && route[i].init_y == route[i].final_y
                  && (route[i].init_layer < wire_via_layer
                      || route[i].final_layer < wire_via_layer))) {
            // remove all vias to this pin that doesn't connects two wires
            route.erase(route.begin() + i);
            i = 0;
          }
        }
      }

      int closest_layer = -1;
      int minor_diff = std::numeric_limits<int>::max();

      for (GSegment& seg : cover_segs) {
        if (seg.init_layer != seg.final_layer) {
          logger_->error(GRT, 77, "Segment has invalid layer assignment.");
        }

        int diff_layers = std::abs(pin.getTopLayer() - seg.init_layer);
        if (diff_layers < minor_diff && seg.init_layer > closest_layer) {
          minor_diff = seg.init_layer;
          closest_layer = seg.init_layer;
        }
      }

      if (closest_layer > pin.getTopLayer()) {
        for (int l = closest_layer; l > pin.getTopLayer(); l--) {
          GSegment segment = GSegment(
              pin_pos.x(), pin_pos.y(), l, pin_pos.x(), pin_pos.y(), l - 1);
          route.push_back(segment);
        }
      } else if (closest_layer < pin.getTopLayer()) {
        for (int l = closest_layer; l < pin.getTopLayer(); l++) {
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
    if (net->getNumPins() > 1) {
      odb::dbNet* db_net = net->getDbNet();
      GRoute& route = routes[db_net];
      if (route.empty()) {
        addGuidesForLocalNets(db_net, route, min_routing_layer, max_routing_layer);
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

void GlobalRouter::mergeBox(std::vector<odb::Rect>& guide_box)
{
  std::vector<odb::Rect> final_box;
  if (guide_box.size() < 1) {
    logger_->error(GRT, 78, "Guides vector is empty.");
  }
  final_box.push_back(guide_box[0]);
  for (uint i = 1; i < guide_box.size(); i++) {
    odb::Rect box = guide_box[i];
    odb::Rect& lastBox = final_box.back();
    if (lastBox.overlaps(box)) {
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
  long init_x, init_y;
  long final_x, final_y;

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

  int llX = init_x - (grid_->getTileWidth() / 2);
  int llY = init_y - (grid_->getTileHeight() / 2);

  int urX = final_x + (grid_->getTileWidth() / 2);
  int urY = final_y + (grid_->getTileHeight() / 2);

  if ((die_bounds.xMax() - urX) / grid_->getTileWidth() < 1) {
    urX = die_bounds.xMax();
  }
  if ((die_bounds.yMax() - urY) / grid_->getTileHeight() < 1) {
    urY = die_bounds.yMax();
  }

  odb::Point lower_left = odb::Point(llX, llY);
  odb::Point upper_right = odb::Point(urX, urY);

  odb::Rect route_bds = odb::Rect(lower_left, upper_right);
  return route_bds;
}

void GlobalRouter::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<odb::Point>> layer_positions_map;

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* tech_layer;
  for (Pin* port : getAllPorts()) {
    if (port->getNumLayers() == 0) {
      logger_->error(GRT, 79, "Pin {} does not have layer assignment.",
                     port->getName().c_str());
    }
    int layer = port->getLayers()[0];  // port have only one layer

    tech_layer = tech->findRoutingLayer(layer + 1);
    if (layer_positions_map[layer].empty()) {
      layer_positions_map[layer].push_back(port->getPosition());
    } else {
      for (odb::Point& pos : layer_positions_map[layer]) {
        if (pos == port->getPosition()) {
          logger_->warn(GRT, 31, "At least 2 pins in position ({}, {}), layer {}.",
                        pos.x(),
                        pos.y(),
                        tech_layer->getName());
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

void GlobalRouter::computeWirelength()
{
  long total_wirelength = 0;
  for (auto& net_route : routes_) {
    GRoute& route = net_route.second;
    for (GSegment& segment : route) {
      int segmentWl = std::abs(segment.final_x - segment.init_x)
                      + std::abs(segment.final_y - segment.init_y);
      total_wirelength += segmentWl;

      if (segmentWl > 0) {
        total_wirelength += (grid_->getTileWidth() + grid_->getTileHeight()) / 2;
      }
    }
  }
  logger_->info(GRT, 18, "Total wirelength: {} um",
                total_wirelength / block_->getDefUnits());
}

// This needs to be rewritten to shift down undeleted elements instead
// of using erase.
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
                              pin.getTopLayer());
      segs_at_point[pinPt] += 1;
    }

    uint i = 0;
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
  long init_x0 = std::min(seg0.init_x, seg0.final_x);
  long init_y0 = std::min(seg0.init_y, seg0.final_y);
  long final_x0 = std::max(seg0.final_x, seg0.init_x);
  long final_y0 = std::max(seg0.final_y, seg0.init_y);

  long init_x1 = std::min(seg1.init_x, seg1.final_x);
  long init_y1 = std::min(seg1.init_y, seg1.final_y);
  long final_x1 = std::max(seg1.final_x, seg1.init_x);
  long final_y1 = std::max(seg1.final_y, seg1.init_y);

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

void GlobalRouter::addLocalConnections(NetRouteMap& routes)
{
  int top_layer;
  std::vector<odb::Rect> pin_boxes;
  odb::Point pin_position;
  odb::Point real_pin_position;
  GSegment hor_segment;
  GSegment ver_segment;

  for (auto& net_route : routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    Net* net = getNet(db_net);

    for (Pin& pin : net->getPins()) {
      top_layer = pin.getTopLayer();
      pin_boxes = pin.getBoxes().at(top_layer);
      pin_position = pin.getOnGridPosition();
      real_pin_position = getRectMiddle(pin_boxes[0]);

      hor_segment = GSegment(real_pin_position.x(),
                            real_pin_position.y(),
                            top_layer,
                            pin_position.x(),
                            real_pin_position.y(),
                            top_layer);
      ver_segment = GSegment(pin_position.x(),
                            real_pin_position.y(),
                            top_layer,
                            pin_position.x(),
                            pin_position.y(),
                            top_layer);

      route.push_back(hor_segment);
      route.push_back(ver_segment);
    }
  }
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

  int top_layer = pin.getTopLayer();
  std::vector<odb::Rect> pin_boxes = pin.getBoxes().at(top_layer);

  RoutingLayer layer = getRoutingLayerByIndex(top_layer);
  RoutingTracks tracks = getRoutingTracksByIndex(top_layer);

  odb::Rect pin_rect;
  pin_rect.mergeInit();
  for (odb::Rect pin_box : pin_boxes) {
    pin_rect.merge(pin_box);
  }

  bool horizontal = layer.getPreferredDirection() == RoutingLayer::HORIZONTAL;
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

    if (nearest_track >= min && nearest_track <= max) {
      track_position = horizontal ? odb::Point(track_position.x(), nearest_track)
                                 : odb::Point(nearest_track, track_position.y());
      return true;
    } else if (nearest_track2 >= min && nearest_track2 <= max) {
      track_position = horizontal ? odb::Point(track_position.x(), nearest_track2)
                                 : odb::Point(nearest_track2, track_position.y());
      return true;
    } else {
      return false;
    }
  }

  return false;
}

GSegment GlobalRouter::createFakePin(Pin pin,
                                     odb::Point& pin_position,
                                     RoutingLayer layer)
{
  int top_layer = layer.getIndex();
  GSegment pin_connection;
  pin_connection.init_layer = top_layer;
  pin_connection.final_layer = top_layer;

  pin_connection.init_x = pin_position.x();
  pin_connection.final_x = pin_position.x();
  pin_connection.init_y = pin_position.y();
  pin_connection.final_y = pin_position.y();

  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    int new_x_position;
    if (pin.getOrientation() == PinOrientation::west) {
      new_x_position = pin_position.x() + (gcells_offset_ * grid_->getTileWidth());
      if (new_x_position <= grid_->getUpperRightX()) {
        pin_connection.init_x = new_x_position;
        pin_position.setX(new_x_position);
      }
    } else if (pin.getOrientation() == PinOrientation::east) {
      new_x_position = pin_position.x() - (gcells_offset_ * grid_->getTileWidth());
      if (new_x_position >= grid_->getLowerLeftX()) {
        pin_connection.init_x = new_x_position;
        pin_position.setX(new_x_position);
      }
    } else {
      logger_->warn(GRT, 32, "Pin {} has invalid orientation.", pin.getName());
    }
  } else {
    int new_y_position;
    if (pin.getOrientation() == PinOrientation::south) {
      new_y_position = pin_position.y() + (gcells_offset_ * grid_->getTileHeight());
      if (new_y_position <= grid_->getUpperRightY()) {
        pin_connection.init_y = new_y_position;
        pin_position.setY(new_y_position);
      }
    } else if (pin.getOrientation() == PinOrientation::north) {
      new_y_position = pin_position.y() - (gcells_offset_ * grid_->getTileHeight());
      if (new_y_position >= grid_->getLowerLeftY()) {
        pin_connection.init_y = new_y_position;
        pin_position.setY(new_y_position);
      }
    } else {
      logger_->warn(GRT, 33, "Pin {} has invalid orientation.", pin.getName());
    }
  }

  return pin_connection;
}

odb::Point GlobalRouter::findFakePinPosition(Pin& pin, odb::dbNet* db_net)
{
  odb::Point fake_position = pin.getOnGridPosition();
  Net* net = db_net_map_[db_net];
  if ((pin.isConnectedToPad() || pin.isPort()) && !net->isLocal()) {
    RoutingLayer layer = getRoutingLayerByIndex(pin.getTopLayer());
    createFakePin(pin, fake_position, layer);
  }

  return fake_position;
}

void GlobalRouter::initAdjustments()
{
  if (adjustments_.empty()) {
    adjustments_.resize(db_->getTech()->getRoutingLayerCount() + 1, 0);
  }
}

int GlobalRouter::getNetCount() const
{
  return nets_->size();
}

Net* GlobalRouter::addNet(odb::dbNet* db_net)
{
  nets_->push_back(Net(db_net));
  Net* net = &nets_->back();
  return net;
}

void GlobalRouter::reserveNets(size_t net_count)
{
  nets_->reserve(net_count);
}

int GlobalRouter::getMaxNetDegree()
{
  int max_degree = -1;
  for (Net& net : *nets_) {
    int netDegree = net.getNumPins();
    if (netDegree > max_degree) {
      max_degree = netDegree;
    }
  }
  return max_degree;
}

std::vector<Pin*> GlobalRouter::getAllPorts()
{
  std::vector<Pin*> ports;
  for (Net& net : *nets_) {
    for (Pin& pin : net.getPins()) {
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

// db functions

void GlobalRouter::initGrid(int max_layer)
{
  odb::dbTech* tech = db_->getTech();

  odb::dbTechLayer* tech_layer = tech->findRoutingLayer(selected_metal_);

  if (tech_layer == nullptr) {
    logger_->error(GRT, 81, "Layer {} not found.", selected_metal_);
  }

  odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);

  if (track_grid == nullptr) {
    logger_->error(GRT, 82, "Track for layer {} not found.", tech_layer->getName());
  }

  int track_step_x, track_step_y;
  int init_track_x, num_tracks_x;
  int init_track_y, num_tracks_y;
  int track_spacing;

  track_grid->getGridPatternX(0, init_track_x, num_tracks_x, track_step_x);
  track_grid->getGridPatternY(0, init_track_y, num_tracks_y, track_step_y);

  if (tech_layer->getDirection().getValue()
      == odb::dbTechLayerDir::HORIZONTAL) {
    track_spacing = track_step_y;
  } else if (tech_layer->getDirection().getValue()
             == odb::dbTechLayerDir::VERTICAL) {
    track_spacing = track_step_x;
  } else {
    logger_->error(GRT, 83, "Layer {} does not have valid direction.",
                   tech_layer->getName());
  }

  odb::Rect rect;
  block_->getDieArea(rect);

  int lower_leftX = rect.xMin();
  int lower_leftY = rect.yMin();

  int upper_rightX = rect.xMax();
  int upper_rightY = rect.yMax();

  int tile_width = grid_->getPitchesInTile() * track_spacing;
  int tile_height = grid_->getPitchesInTile() * track_spacing;

  int x_grids = std::floor((float) upper_rightX / tile_width);
  int y_grids = std::floor((float) upper_rightY / tile_height);

  bool perfect_regular_x = false;
  bool perfect_regular_y = false;

  int num_layers = tech->getRoutingLayerCount();
  if (max_layer > -1) {
    num_layers = max_layer;
  }

  if ((x_grids * tile_width) == upper_rightX)
    perfect_regular_x = true;

  if ((y_grids * tile_height) == upper_rightY)
    perfect_regular_y = true;

  std::vector<int> generic_vector(num_layers);
  std::map<int, std::vector<odb::Rect>> generic_map;

  grid_->init(lower_leftX,
              lower_leftY,
              rect.xMax(),
              rect.yMax(),
              tile_width,
              tile_height,
              x_grids,
              y_grids,
              perfect_regular_x,
              perfect_regular_y,
              num_layers,
              generic_vector,
              generic_vector,
              generic_vector,
              generic_vector,
              generic_map);
}

void GlobalRouter::initRoutingLayers(std::vector<RoutingLayer>& routing_layers)
{
  odb::dbTech* tech = db_->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l);
    int index = l;
    std::string name = tech_layer->getConstName();
    bool preferred_direction;
    if (tech_layer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      preferred_direction = RoutingLayer::HORIZONTAL;
    } else if (tech_layer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      preferred_direction = RoutingLayer::VERTICAL;
    } else {
      logger_->error(GRT, 84, "Layer {} does not have valid direction.",
                     tech_layer->getName());
    }

    RoutingLayer routing_layer = RoutingLayer(index, name, preferred_direction);
    routing_layers.push_back(routing_layer);
  }
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
  odb::dbTech* tech = db_->getTech();
  for (auto layer : tech->getLayers()) {
    if (layer->getType() != odb::dbTechLayerType::ROUTING)
      continue;
    int level = layer->getRoutingLevel();
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
        min_spc_up = layer->findV55Spacing(std::max(layer_width, width_up), prl_up);
      if (down_via_valid)
        min_spc_down
            = layer->findV55Spacing(std::max(layer_width, width_down), prl_down);
    } else {
      odb::dbSet<odb::dbTechLayerSpacingRule> rules;
      layer->getV54SpacingRules(rules);
      if (rules.size() > 0) {
        min_spc_valid = true;
        int minSpc;
        for (auto rule : rules)
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
                 static_cast<float>(dbuToMicrons(layer_width)));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_up : viaWidth = {:.4f} , prl = {:.4f} , minSpc = {:.4f} "
                 ", L2V = {:.4f} ",
                 static_cast<float>(dbuToMicrons(width_up)),
                 static_cast<float>(dbuToMicrons(prl_up)),
                 static_cast<float>(dbuToMicrons(min_spc_up)),
                 static_cast<float>(dbuToMicrons(L2V_up)));
      debugPrint(logger_,
                 utl::GRT,
                 "l2v_pitch",
                 1,
                 "L2V_down : viaWidth = {:.4f} , prl = {:.4f} , minSpc = "
                 "{:.4f} , L2V = {:.4f} ",
                 static_cast<float>(dbuToMicrons(width_down)),
                 static_cast<float>(dbuToMicrons(prl_down)),
                 static_cast<float>(dbuToMicrons(min_spc_down)),
                 static_cast<float>(dbuToMicrons(L2V_down)));
    }
    pitches[level] = {L2V_up, L2V_down};
  }
  return pitches;
}

void GlobalRouter::initRoutingTracks(
    std::vector<RoutingTracks>& routing_tracks,
    int max_layer)
{
  odb::dbTech* tech = db_->getTech();
  auto l2vPitches = calcLayerPitches(max_layer);
  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    if (layer > max_layer && max_layer > -1) {
      break;
    }

    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);

    if (tech_layer == nullptr) {
      logger_->error(GRT, 85, "Routing layer {} not found.", layer);
    }

    odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);

    if (track_grid == nullptr) {
      logger_->error(GRT, 86, "Track for layer {} not found.", tech_layer->getName());
    }

    int track_step_x, track_step_y;
    int init_track_x, num_tracks_x;
    int init_track_y, num_tracks_y;
    int track_pitch, line_2__via_pitch_down, line_2__via_pitch_up, location, num_tracks;
    bool orientation;

    track_grid->getGridPatternX(0, init_track_x, num_tracks_x, track_step_x);
    track_grid->getGridPatternY(0, init_track_y, num_tracks_y, track_step_y);

    if (tech_layer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      track_pitch = track_step_y;
      line_2__via_pitch_up = l2vPitches[layer].first;
      line_2__via_pitch_down = l2vPitches[layer].second;
      location = init_track_y;
      num_tracks = num_tracks_y;
      orientation = RoutingLayer::HORIZONTAL;
    } else if (tech_layer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      track_pitch = track_step_x;
      line_2__via_pitch_up = l2vPitches[layer].first;
      line_2__via_pitch_down = l2vPitches[layer].second;
      location = init_track_x;
      num_tracks = num_tracks_x;
      orientation = RoutingLayer::VERTICAL;
    } else {
      logger_->error(GRT, 87, "Layer {} does not have valid direction.",
                     tech_layer->getName());
    }

    RoutingTracks layer_tracks = RoutingTracks(layer,
                                                track_pitch,
                                                line_2__via_pitch_up,
                                                line_2__via_pitch_down,
                                                location,
                                                num_tracks,
                                                orientation);
    routing_tracks.push_back(layer_tracks);
    logger_->info(GRT, 88, "Layer {:7s} Track-Pitch = {:.4f}  line-2-Via Pitch: {:.4f}",
        tech_layer->getName(),
        static_cast<float>(dbuToMicrons(layer_tracks.getTrackPitch())),
        static_cast<float>(dbuToMicrons(layer_tracks.getLineToViaPitch())));
  }
}

void GlobalRouter::computeCapacities(int max_layer)
{
  int h_capacity, v_capacity;

  odb::dbTech* tech = db_->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > max_layer && max_layer > -1) {
      break;
    }

    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l);

    RoutingTracks routing_tracks = getRoutingTracksByIndex(l);
    int track_spacing = routing_tracks.getUsePitch();

    if (tech_layer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      h_capacity = std::floor((float) grid_->getTileWidth() / track_spacing);

      grid_->addHorizontalCapacity(h_capacity, l - 1);
      grid_->addVerticalCapacity(0, l - 1);
      debugPrint(logger_,
                 GRT,
                 "graph",
                 1,
                 "Layer {} has {} h-capacity",
                 tech_layer->getConstName(),
                 h_capacity);
    } else if (tech_layer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      v_capacity = std::floor((float) grid_->getTileWidth() / track_spacing);

      grid_->addHorizontalCapacity(0, l - 1);
      grid_->addVerticalCapacity(v_capacity, l - 1);
      debugPrint(logger_,
                 GRT,
                 "graph",
                 1,
                 "Layer {} has {} v-capacity",
                 tech_layer->getConstName(),
                 v_capacity);
    } else {
      logger_->error(GRT, 89, "Layer {} does not have valid direction.",
                     tech_layer->getName());
    }
  }
}

void GlobalRouter::computeSpacingsAndMinWidth(int max_layer)
{
  int min_spacing = 0;
  int min_width;
  int track_step_x, track_step_y;
  int init_track_x, num_tracks_x;
  int init_track_y, num_tracks_y;

  odb::dbTech* tech = db_->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > max_layer && max_layer > -1) {
      break;
    }

    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l);

    odb::dbTrackGrid* track = block_->findTrackGrid(tech_layer);

    if (track == nullptr) {
      logger_->error(GRT, 90, "Track for layer {} not found.", tech_layer->getName());
    }

    track->getGridPatternX(0, init_track_x, num_tracks_x, track_step_x);
    track->getGridPatternY(0, init_track_y, num_tracks_y, track_step_y);

    if (tech_layer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      min_width = track_step_y;
    } else if (tech_layer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      min_width = track_step_x;
    } else {
      logger_->error(GRT, 91, "Layer {} does not have valid direction.",
                     tech_layer->getName());
    }

    grid_->addSpacing(min_spacing, l - 1);
    grid_->addMinWidth(min_width, l - 1);
  }
}

void GlobalRouter::initNetlist()
{
  if (nets_->empty()) {
    initClockNets();
    std::set<odb::dbNet*, cmpById> db_nets;

    for (odb::dbNet* net : block_->getNets()) {
      db_nets.insert(net);
    }

    if (db_nets.empty()) {
      logger_->error(GRT, 92, "Design without nets.");
    }

    addNets(db_nets);
  }
}

void GlobalRouter::addNets(std::set<odb::dbNet*, cmpById>& db_nets)
{
  // Prevent nets_ from growing because pointers to nets become invalid.
  reserveNets(db_nets.size());
  for (odb::dbNet* db_net : db_nets) {
    if (db_net->getSigType().getValue() != odb::dbSigType::POWER
        && db_net->getSigType().getValue() != odb::dbSigType::GROUND
        && !db_net->isSpecial() && db_net->getSWires().empty()) {
      Net* net = addNet(db_net);
      db_net_map_[db_net] = net;
      makeItermPins(net, db_net, grid_->getGridArea());
      makeBtermPins(net, db_net, grid_->getGridArea());
      findPins(net);
    }
  }
}

Net* GlobalRouter::getNet(odb::dbNet* db_net)
{
  return db_net_map_[db_net];
}

void GlobalRouter::getNetsByType(NetType type, std::vector<Net*>& nets)
{
  if (type == NetType::Clock || type == NetType::Signal) {
    bool get_clock = type == NetType::Clock;
    for (Net net : *nets_) {
      if ((get_clock && net.getSignalType() == odb::dbSigType::CLOCK
           && !clockHasLeafITerm(net.getDbNet()))
          || (!get_clock
              && (net.getSignalType() != odb::dbSigType::CLOCK
                  || clockHasLeafITerm(net.getDbNet())))) {
        nets.push_back(db_net_map_[net.getDbNet()]);
      }
    }
  } else if (type == NetType::Antenna) {
    for (odb::dbNet* db_net : dirty_nets_) {
      nets.push_back(db_net_map_[db_net]);
    }
  } else {
    for (Net net : *nets_) {
      nets.push_back(db_net_map_[net.getDbNet()]);
    }
  }
}

void GlobalRouter::initClockNets()
{
  std::set<odb::dbNet*> clock_nets = sta_->findClkNets();

  logger_->info(GRT, 19, "Found {} clock nets.", clock_nets.size());

  for (odb::dbNet* net : clock_nets) {
    net->setSigType(odb::dbSigType::CLOCK);
  }
}

bool GlobalRouter::isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network)
{
  const sta::Pin* pin = network->dbToSta(iterm);
  sta::LibertyPort* lib_port = network->libertyPort(pin);
  if (lib_port == nullptr)
    return false;
  return lib_port->isRegClk();
}

bool GlobalRouter::clockHasLeafITerm(odb::dbNet* db_net)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (db_net->getSigType() == odb::dbSigType::CLOCK) {
    for (odb::dbITerm* iterm : db_net->getITerms()) {
      if (isClkTerm(iterm, network)) {
        return true;
      }
    }
  }

  return false;
}

void GlobalRouter::makeItermPins(Net* net,
                                 odb::dbNet* db_net,
                                 const odb::Rect& die_area)
{
  odb::dbTech* tech = db_->getTech();
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    int pX, pY;
    std::vector<int> pin_layers;
    std::map<int, std::vector<odb::Rect>> pin_boxes;

    odb::dbMTerm* mterm = iterm->getMTerm();
    odb::dbMaster* master = mterm->getMaster();

    if (master->getType() == odb::dbMasterType::COVER
        || master->getType() == odb::dbMasterType::COVER_BUMP) {
      logger_->warn(GRT, 34, "Net connected with instance of class COVER added for routing.");
    }

    bool connected_to_pad = master->getType().isPad();
    bool connected_to_macro = master->isBlock();

    odb::Point pin_pos;

    odb::dbInst* inst = iterm->getInst();
    inst->getOrigin(pX, pY);
    odb::Point origin = odb::Point(pX, pY);
    odb::dbTransform transform(inst->getOrient(), origin);

    odb::dbBox* inst_box = inst->getBBox();
    odb::Point inst_middle = odb::Point(
        (inst_box->xMin() + (inst_box->xMax() - inst_box->xMin()) / 2.0),
        (inst_box->yMin() + (inst_box->yMax() - inst_box->yMin()) / 2.0));

    for (odb::dbMPin* mterm : mterm->getMPins()) {
      odb::Point lower_bound;
      odb::Point upper_bound;
      odb::Rect pin_box;
      int pin_layer;
      int last_layer = -1;

      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::Rect rect;
        box->getBox(rect);
        transform.apply(rect);

        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        pin_layer = tech_layer->getRoutingLevel();
        lower_bound = odb::Point(rect.xMin(), rect.yMin());
        upper_bound = odb::Point(rect.xMax(), rect.yMax());
        pin_box = odb::Rect(lower_bound, upper_bound);
        if (!die_area.contains(pin_box)) {
          logger_->warn(GRT, 35, "Pin {} is outside die area.", getITermName(iterm));
        }
        pin_boxes[pin_layer].push_back(pin_box);
        if (pin_layer > last_layer) {
          pin_pos = lower_bound;
        }
      }
    }

    for (auto& layer_boxes : pin_boxes) {
      pin_layers.push_back(layer_boxes.first);
    }

    Pin pin(iterm,
            pin_pos,
            pin_layers,
            PinOrientation::invalid,
            pin_boxes,
            (connected_to_pad || connected_to_macro));

    if (connected_to_pad || connected_to_macro) {
      odb::Point pin_position = pin.getPosition();
      odb::dbTechLayer* tech_layer = tech->findRoutingLayer(pin.getTopLayer());

      if (tech_layer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int inst_to_pin = pin_position.x() - inst_middle.x();
        if (inst_to_pin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (tech_layer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int inst_to_pin = pin_position.y() - inst_middle.y();
        if (inst_to_pin < 0) {
          pin.setOrientation(PinOrientation::north);
        } else {
          pin.setOrientation(PinOrientation::south);
        }
      }
    }

    net->addPin(pin);
  }
}

void GlobalRouter::makeBtermPins(Net* net,
                                 odb::dbNet* db_net,
                                 const odb::Rect& die_area)
{
  odb::dbTech* tech = db_->getTech();
  for (odb::dbBTerm* bterm : db_net->getBTerms()) {
    int posX, posY;
    std::string pin_name;

    bterm->getFirstPinLocation(posX, posY);
    odb::dbITerm* iterm = bterm->getITerm();
    bool connected_to_pad = false;
    bool connected_to_macro = false;
    odb::Point inst_middle = odb::Point(-1, -1);

    if (iterm != nullptr) {
      odb::dbMTerm* mterm = iterm->getMTerm();
      odb::dbMaster* master = mterm->getMaster();
      connected_to_pad = master->getType().isPad();
      connected_to_macro = master->isBlock();

      odb::dbInst* inst = iterm->getInst();
      odb::dbBox* inst_box = inst->getBBox();
      inst_middle = odb::Point(
          (inst_box->xMin() + (inst_box->xMax() - inst_box->xMin()) / 2.0),
          (inst_box->yMin() + (inst_box->yMax() - inst_box->yMin()) / 2.0));
    }

    std::vector<int> pin_layers;
    std::map<int, std::vector<odb::Rect>> pin_boxes;

    pin_name = bterm->getConstName();
    odb::Point pin_pos;

    for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
      odb::Point lower_bound;
      odb::Point upper_bound;
      odb::Rect pin_box;
      int pin_layer;
      int last_layer = -1;

      for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
        odb::dbTechLayer* tech_layer = bpin_box->getTechLayer();
        if (tech_layer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        pin_layer = tech_layer->getRoutingLevel();
        lower_bound = odb::Point(bpin_box->xMin(), bpin_box->yMin());
        upper_bound = odb::Point(bpin_box->xMax(), bpin_box->yMax());
        pin_box = odb::Rect(lower_bound, upper_bound);
        if (!die_area.contains(pin_box)) {
          logger_->warn(GRT, 36, "Pin {} is outside die area.", pin_name);
        }
        pin_boxes[pin_layer].push_back(pin_box);

        if (pin_layer > last_layer) {
          pin_pos = lower_bound;
        }
      }
    }

    for (auto& layer_boxes : pin_boxes) {
      pin_layers.push_back(layer_boxes.first);
    }

    Pin pin(bterm,
            pin_pos,
            pin_layers,
            PinOrientation::invalid,
            pin_boxes,
            (connected_to_pad || connected_to_macro));

    if (pin.getLayers().empty()) {
      logger_->error(GRT, 93, "Pin {} does not have layer assignment.",
                     bterm->getConstName());
    }

    if (connected_to_pad) {
      odb::Point pin_position = pin.getPosition();
      odb::dbTechLayer* tech_layer = tech->findRoutingLayer(pin.getTopLayer());

      if (tech_layer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int inst_to_pin = pin_position.x() - inst_middle.x();
        if (inst_to_pin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (tech_layer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int inst_to_pin = pin_position.y() - inst_middle.y();
        if (inst_to_pin < 0) {
          pin.setOrientation(PinOrientation::north);
        } else {
          pin.setOrientation(PinOrientation::south);
        }
      }
    } else {
      odb::Point pin_position = pin.getPosition();
      odb::dbTechLayer* tech_layer = tech->findRoutingLayer(pin.getTopLayer());

      if (tech_layer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int inst_to_die = pin_position.x() - getRectMiddle(die_area).x();
        if (inst_to_die < 0) {
          pin.setOrientation(PinOrientation::west);
        } else {
          pin.setOrientation(PinOrientation::east);
        }
      } else if (tech_layer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int inst_to_die = pin_position.y() - getRectMiddle(die_area).y();
        if (inst_to_die < 0) {
          pin.setOrientation(PinOrientation::south);
        } else {
          pin.setOrientation(PinOrientation::north);
        }
      }
    }
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

void GlobalRouter::initObstructions()
{
  odb::Rect die_area(grid_->getLowerLeftX(),
                    grid_->getLowerLeftY(),
                    grid_->getUpperRightX(),
                    grid_->getUpperRightY());
  std::vector<int> layer_extensions;

  findLayerExtensions(layer_extensions);
  int obstructions_cnt = findObstructions(die_area);
  obstructions_cnt += findInstancesObstructions(die_area, layer_extensions);
  findNetsObstructions(die_area);

  logger_->info(GRT, 4, "Blockages: {}", obstructions_cnt);
}

void GlobalRouter::findLayerExtensions(std::vector<int>& layer_extensions)
{
  odb::dbTech* tech = db_->getTech();
  layer_extensions.resize(tech->getRoutingLayerCount() + 1, 0);

  for (odb::dbTechLayer* obstruct_layer : tech->getLayers()) {
    if (obstruct_layer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
      continue;
    }

    int max_int = std::numeric_limits<int>::max();

    // Gets the smallest possible minimum spacing that won't cause violations
    // for ANY configuration of PARALLELRUNLENGTH (the biggest value in the
    // table)

    int spacing_extension = obstruct_layer->getSpacing(max_int, max_int);

    odb::dbSet<odb::dbTechLayerSpacingRule> eol_rules;

    // Check for EOL spacing values and, if the spacing is higher than the one
    // found, use them as the macro extension instead of PARALLELRUNLENGTH

    if (obstruct_layer->getV54SpacingRules(eol_rules)) {
      for (odb::dbTechLayerSpacingRule* rule : eol_rules) {
        int spacing = rule->getSpacing();
        if (spacing > spacing_extension) {
          spacing_extension = spacing;
        }
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

    layer_extensions[obstruct_layer->getRoutingLevel()] = spacing_extension;
  }
}

int GlobalRouter::findObstructions(odb::Rect& die_area)
{
  int obstructions_cnt = 0;
  for (odb::dbObstruction* obstruction : block_->getObstructions()) {
    odb::dbBox* obstruction_box = obstruction->getBBox();

    int layer = obstruction_box->getTechLayer()->getRoutingLevel();

    odb::Point lower_bound
        = odb::Point(obstruction_box->xMin(), obstruction_box->yMin());
    odb::Point upper_bound
        = odb::Point(obstruction_box->xMax(), obstruction_box->yMax());
    odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
    if (!die_area.contains(obstruction_rect)) {
      logger_->warn(GRT, 37, "Found blockage outside die area.");
    }
    grid_->addObstruction(layer, obstruction_rect);
    obstructions_cnt++;
  }

  return obstructions_cnt;
}

int GlobalRouter::findInstancesObstructions(
    odb::Rect& die_area,
    const std::vector<int>& layer_extensions)
{
  int macros_cnt = 0;
  int obstructions_cnt = 0;
  int pin_out_of_die_count = 0;
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

    for (odb::dbBox* box : master->getObstructions()) {
      int layer = box->getTechLayer()->getRoutingLevel();

      odb::Rect rect;
      box->getBox(rect);
      transform.apply(rect);

      int layer_extension = 0;

      if (isMacro) {
        layer_extension
            = layer_extensions[box->getTechLayer()->getRoutingLevel()];
        layer_extension += macro_extension_ * grid_->getTileWidth();
      }

      odb::Point lower_bound = odb::Point(rect.xMin() - layer_extension,
                                         rect.yMin() - layer_extension);
      odb::Point upper_bound = odb::Point(rect.xMax() + layer_extension,
                                         rect.yMax() + layer_extension);
      odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
      if (!die_area.contains(obstruction_rect)) {
        logger_->warn(GRT, 38, "Found blockage outside die area in instance {}.",
                      inst->getConstName());
      }
      grid_->addObstruction(layer, obstruction_rect);
      obstructions_cnt++;
    }

    for (odb::dbMTerm* mterm : master->getMTerms()) {
      for (odb::dbMPin* mterm : mterm->getMPins()) {
        odb::Point lower_bound;
        odb::Point upper_bound;
        odb::Rect pin_box;
        int pin_layer;

        for (odb::dbBox* box : mterm->getGeometry()) {
          odb::Rect rect;
          box->getBox(rect);
          transform.apply(rect);

          odb::dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer->getType().getValue()
              != odb::dbTechLayerType::ROUTING) {
            continue;
          }

          pin_layer = tech_layer->getRoutingLevel();
          lower_bound = odb::Point(rect.xMin(), rect.yMin());
          upper_bound = odb::Point(rect.xMax(), rect.yMax());
          pin_box = odb::Rect(lower_bound, upper_bound);
          if (!die_area.contains(pin_box)) {
              logger_->warn(GRT, 39, "Found pin outside die area in instance {}.",
                            inst->getConstName());
            pin_out_of_die_count++;
          }
          grid_->addObstruction(pin_layer, pin_box);
        }
      }
    }
  }

  if (pin_out_of_die_count > 0) {
    logger_->warn(GRT, 28, "Found {} pins outside die area.", pin_out_of_die_count);
  }

  logger_->info(GRT, 3, "Macros: {}", macros_cnt);
  return obstructions_cnt;
}

void GlobalRouter::findNetsObstructions(odb::Rect& die_area)
{
  odb::dbSet<odb::dbNet> nets = block_->getNets();

  if (nets.empty()) {
    logger_->error(GRT, 94, "Design without nets.");
  }

  for (odb::dbNet* db_net : nets) {
    uint wire_cnt = 0, via_cnt = 0;
    db_net->getWireCount(wire_cnt, via_cnt);
    if (wire_cnt < 1)
      continue;

    if (db_net->getSigType() == odb::dbSigType::POWER
        || db_net->getSigType() == odb::dbSigType::GROUND) {
      for (odb::dbSWire* swire : db_net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            continue;
          } else {
            odb::Rect wire_rect;
            s->getBox(wire_rect);
            int l = s->getTechLayer()->getRoutingLevel();

            odb::Point lower_bound
                = odb::Point(wire_rect.xMin(), wire_rect.yMin());
            odb::Point upper_bound
                = odb::Point(wire_rect.xMax(), wire_rect.yMax());
            odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
            if (!die_area.contains(obstruction_rect)) {
              logger_->warn(GRT, 40, "Net {} has wires outside die area.",
                            db_net->getConstName());
            }
            grid_->addObstruction(l, obstruction_rect);
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
            odb::Rect wire_rect;
            pshape.shape.getBox(wire_rect);
            int l = pshape.shape.getTechLayer()->getRoutingLevel();

            odb::Point lower_bound
                = odb::Point(wire_rect.xMin(), wire_rect.yMin());
            odb::Point upper_bound
                = odb::Point(wire_rect.xMax(), wire_rect.yMax());
            odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
            if (!die_area.contains(obstruction_rect)) {
              logger_->warn(GRT, 41, "Net {} has wires outside die area.",
                            db_net->getConstName());
            }
            grid_->addObstruction(l, obstruction_rect);
          }
        }
      }
    }
  }
}

int GlobalRouter::computeMaxRoutingLayer()
{
  block_ = db_->getChip()->getBlock();

  int max_routing_layer = -1;

  odb::dbTech* tech = db_->getTech();

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(layer);
    if (tech_layer == nullptr) {
      logger_->error(GRT, 95, "Layer {} not found.", layer);
    }
    odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
    if (track_grid == nullptr) {
      break;
    }
    max_routing_layer = layer;
  }

  return max_routing_layer;
}

double GlobalRouter::dbuToMicrons(int64_t dbu)
{
  return (double) dbu / (block_->getDbUnitsPerMicron());
}

std::map<int, odb::dbTechVia*> GlobalRouter::getDefaultVias(int max_routing_layer)
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
      logger_->info(GRT, 8, "Default via: {}.", via->getConstName());
      default_vias[via->getBottomLayer()->getRoutingLevel()] = via;
    }
  }

  if (default_vias.empty()) {
    logger_->warn(GRT, 43, "No OR_DEFAULT vias defined.");
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
          logger_->info(GRT, 224, "Chose via {} as default.", via->getConstName());
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

void GlobalRouter::reportLayerSettings(int min_routing_layer, int max_routing_layer)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* min_layer = tech->findRoutingLayer(min_routing_layer);
  odb::dbTechLayer* max_layer = tech->findRoutingLayer(max_routing_layer);
  logger_->info(GRT, 20, "Min routing layer: {}", min_layer->getName());
  logger_->info(GRT, 21, "Max routing layer: {}", max_layer->getName());
  logger_->info(GRT, 22, "Global adjustment: {}%", int(adjustment_ * 100));
  logger_->info(GRT, 23, "Grid origin: ({}, {})", grid_origin_->x(), grid_origin_->y());
}

void GlobalRouter::reportResources()
{
  fastroute_->computeCongestionInformation();
  odb::dbTech* tech = db_->getTech();
  std::vector<int> original_resources = fastroute_->getOriginalResources();
  std::vector<int> derated_resources = fastroute_->getTotalCapacityPerLayer();

  logger_->report("");
  logger_->info(GRT, 53, "Routing resources analysis:");
  logger_->report("          Routing      Original      Derated      Resource");
  logger_->report(
      "Layer     Direction    Resources     Resources    Reduction (%)");
  logger_->report(
      "---------------------------------------------------------------");

  for (int l = 0; l < original_resources.size(); l++) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(l + 1);
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
  odb::dbTech* tech = db_->getTech();
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

  for (int l = 0; l < resources.size(); l++) {
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

    odb::dbTechLayer* layer = tech->findRoutingLayer(l + 1);
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
  float total_usage = (total_resource == 0) ? 0 : 
                      (float) total_demand / (float) total_resource * 100;
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

void GlobalRouter::reportLayerWireLengths()
{
  std::vector<int64_t> lengths;
  lengths.resize(db_->getTech()->getRoutingLayerCount() + 1);
  int64_t total_length = 0;
  for (auto& net_route : routes_) {
    GRoute& route = net_route.second;
    for (GSegment& seg : route) {
      int layer1 = seg.init_layer;
      int layer2 = seg.final_layer;
      if (layer1 == layer2) {
        int seg_length
            = abs(seg.init_x - seg.final_x) + abs(seg.init_y - seg.final_y);
        lengths[layer1] += seg_length;
        total_length += seg_length;
      }
    }
  }
  if (total_length > 0) {
    odb::dbTech* tech = db_->getTech();
    for (int i = 0; i < lengths.size(); i++) {
      int64_t length = lengths[i];
      if (length > 0) {
        odb::dbTechLayer* layer = tech->findRoutingLayer(i);
        logger_->report("{:5s} {:8d}um {:3d}%",
                        layer->getName(),
                        static_cast<int64_t>(dbuToMicrons(length)),
                        static_cast<int>((100.0 * length) / total_length));
      }
    }
  }
}

////////////////////////////////////////////////////////////////

RoutePt::RoutePt(int x, int y, int layer) : _x(x), _y(y), _layer(layer)
{
}

bool operator<(const RoutePt& p1, const RoutePt& p2)
{
  return (p1._x < p2._x) || (p1._x == p2._x && p1._y < p2._y)
         || (p1._x == p2._x && p1._y == p2._y && p1._layer < p2._layer);
}

class GrouteRenderer : public gui::Renderer
{
 public:
  GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech);
  void highlight(const odb::dbNet* net);
  virtual void drawObjects(gui::Painter& /* painter */) override;

 private:
  GlobalRouter* groute_;
  odb::dbTech* tech_;
  const odb::dbNet* net_;
};

// Highlight guide in the gui.
void GlobalRouter::highlightRoute(const odb::dbNet* net)
{
  if (gui_) {
    if (groute_renderer_ == nullptr) {
      groute_renderer_ = new GrouteRenderer(this, db_->getTech());
      gui_->registerRenderer(groute_renderer_);
    }
    groute_renderer_->highlight(net);
  }
}

GrouteRenderer::GrouteRenderer(GlobalRouter* groute, odb::dbTech* tech)
    : groute_(groute), tech_(tech), net_(nullptr)
{
}

void GrouteRenderer::highlight(const odb::dbNet* net)
{
  net_ = net;
}

void GrouteRenderer::drawObjects(gui::Painter& painter)
{
  if (net_) {
    NetRouteMap& routes = groute_->getRoutes();
    GRoute& groute = routes[const_cast<odb::dbNet*>(net_)];
    for (GSegment& seg : groute) {
      int layer1 = seg.init_layer;
      int layer2 = seg.final_layer;
      if (layer1 == layer2) {
        odb::dbTechLayer* layer = tech_->findRoutingLayer(layer1);
        // Draw rect because drawLine does not have a way to set the pen
        // thickness.
        odb::Rect rect;
        // gui clips visiblity of rect when zoomed out so layer width doesn't
        // work very well.
        int thickness = layer->getWidth() * 20;
        if (seg.init_x == seg.final_x)
          // vertical
          rect = odb::Rect(
              seg.init_x, seg.init_y, seg.init_x + thickness, seg.final_y);
        else
          // horizontal
          rect = odb::Rect(
              seg.init_x, seg.init_y, seg.final_x, seg.init_y + thickness);
        painter.setPen(layer);
        painter.setBrush(layer);
        painter.drawRect(rect);
      }
    }
  }
}

}  // namespace grt
