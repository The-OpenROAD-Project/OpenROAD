/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "fastroute/GlobalRouter.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AntennaRepair.h"
#include "FastRoute.h"
#include "Grid.h"
#include "RcTreeBuilder.h"
#include "RoutingLayer.h"
#include "RoutingTracks.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "utility/Logger.h"
#include "openroad/OpenRoad.hh"
#include "sta/Clock.hh"
#include "sta/Parasitics.hh"
#include "sta/Set.hh"

namespace grt {

using utl::GRT;

void GlobalRouter::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  _logger = openroad->getLogger();
  init();
}

void GlobalRouter::init()
{
  makeComponents();
  // Initialize variables
  _adjustment = 0.0;
  _minRoutingLayer = 1;
  _maxRoutingLayer = -1;
  _unidirectionalRoute = 0;
  _fixLayer = 0;
  _overflowIterations = 50;
  _pdRevForHighFanout = -1;
  _allowOverflow = false;
  _seed = 0;
  _reportCongest = false;

  // Clock net routing variables
  _pdRev = 0;
  _alpha = 0;
  _verbose = 0;
}

void GlobalRouter::makeComponents()
{
  // Allocate memory for objects
  _allRoutingTracks = new std::vector<RoutingTracks>;
  _db = _openroad->getDb();
  _fastRoute = new FastRouteCore(_logger);
  _grid = new Grid;
  _gridOrigin = new odb::Point(0, 0);
  _nets = new std::vector<Net>;
  _sta = _openroad->getSta();
  _routingLayers = new std::vector<RoutingLayer>;
}

void GlobalRouter::deleteComponents()
{
  delete _allRoutingTracks;
  delete _fastRoute;
  delete _grid;
  delete _gridOrigin;
  delete _nets;
  delete _routingLayers;
}

void GlobalRouter::clear()
{
  _routes.clear();
  _nets->clear();
  clearFlow();
}

void GlobalRouter::clearFlow()
{
  _grid->clear();
  _fastRoute->clear();
  _allRoutingTracks->clear();
  _routingLayers->clear();
  _vCapacities.clear();
  _hCapacities.clear();
}

GlobalRouter::~GlobalRouter()
{
  deleteComponents();
}

void GlobalRouter::startFastRoute()
{
  initAdjustments();
  initPitches();
  if (_unidirectionalRoute) {
    _fixLayer = 1;
    if (_minRoutingLayer < 2)
      _minRoutingLayer = 2;
  } else {
    _fixLayer = 0;
  }

  if (_maxRoutingLayer == -1) {
    _maxRoutingLayer = computeMaxRoutingLayer();
  }

  if (_maxRoutingLayer < _selectedMetal) {
    setSelectedMetal(_maxRoutingLayer);
  }

  if (_pdRevForHighFanout != -1) {
    _fastRoute->setAlpha(_alpha);
  }

  _fastRoute->setVerbose(_verbose);
  _fastRoute->setOverflowIterations(_overflowIterations);
  _fastRoute->setPDRevForHighFanout(_pdRevForHighFanout);
  _fastRoute->setAllowOverflow(_allowOverflow);

  _logger->report("Min routing layer: {}", _minRoutingLayer);
  _logger->report("Max routing layer: {}", _maxRoutingLayer);
  _logger->report("Global adjustment: {}%", int(_adjustment * 100));
  _logger->report("Unidirectional routing: {}", _unidirectionalRoute);
  _logger->report("Grid origin: ({}, {})", _gridOrigin->x(), _gridOrigin->y());
  for (int l = 1; l <= _maxRoutingLayer; l++) {
    if (_layerPitches[l] != 0) {
      _logger->report("Layer {} pitch: {}", l, _layerPitches[l]);
    }
  }

  initCoreGrid();
  initRoutingLayers();
  initRoutingTracks();
  setCapacities();
  setSpacingsAndMinWidths();
  initNetlist();
}

void GlobalRouter::applyAdjustments()
{
  computeGridAdjustments();
  computeTrackAdjustments();
  computeObstructionsAdjustments();
  computeUserGlobalAdjustments();
  computeUserLayerAdjustments();

  for (RegionAdjustment regionAdjst : _regionAdjustments) {
    _logger->report("Adjusting region on layer {}", regionAdjst.getLayer());
    computeRegionAdjustments(regionAdjst.getRegion(),
                             regionAdjst.getLayer(),
                             regionAdjst.getAdjustment());
  }

  restorePreviousCapacities(_minLayerForClock, _maxLayerForClock);

  _fastRoute->initAuxVar();
}

void GlobalRouter::runFastRoute(bool onlySignal)
{
  startFastRoute();
  NetType type = onlySignal ? NetType::Signal : NetType::All;
  std::vector<Net*> nets;
  getNetsByType(type, nets);
  initializeNets(nets);
  applyAdjustments();
  // Store results in a temporary map, allowing to keep any previous
  // routing result (e.g., after routeClockNets)
  NetRouteMap result = findRouting(nets);

  _routes.insert(result.begin(), result.end());

  computeWirelength();

  if (_reportCongest) {
    _fastRoute->writeCongestionReport2D(_congestFile + "2D.log");
    _fastRoute->writeCongestionReport3D(_congestFile + "3D.log");
  }
}

void GlobalRouter::repairAntennas(sta::LibertyPort* diodePort)
{
  _logger->report("Repairing antennas...");

  AntennaRepair* antennaRepair = new AntennaRepair(
      this, _openroad->getAntennaChecker(), _openroad->getOpendp(), _db, _logger);

  // Copy first route result and make changes in this new vector
  NetRouteMap originalRoute(_routes);

  getPreviousCapacities(_minRoutingLayer, _maxRoutingLayer);
  addLocalConnections(originalRoute);

  odb::dbMTerm* diodeMTerm = _sta->getDbNetwork()->staToDb(diodePort);
  if (diodeMTerm == nullptr) {
    _logger->error(GRT, 69, "conversion from liberty port to dbMTerm fail.");
  }

  int violationsCnt = antennaRepair->checkAntennaViolations(
      originalRoute, _maxRoutingLayer, diodeMTerm);

  if (violationsCnt > 0) {
    clearFlow();
    antennaRepair->fixAntennas(diodeMTerm);
    antennaRepair->legalizePlacedCells();

    _logger->info(GRT, 15, "{} diodes inserted.", antennaRepair->getDiodesCount());

    startFastRoute();
    updateDirtyNets();
    std::vector<Net*> antennaNets;
    getNetsByType(NetType::Antenna, antennaNets);
    initializeNets(antennaNets);
    applyAdjustments();
    _fastRoute->setVerbose(0);
    _logger->info(GRT, 9, "#Nets to reroute: {}.", antennaNets.size());

    restorePreviousCapacities(_minRoutingLayer, _maxRoutingLayer);
    removeDirtyNetsUsage();

    NetRouteMap newRoute = findRouting(antennaNets);
    mergeResults(newRoute);
  }
}

void GlobalRouter::addDirtyNet(odb::dbNet* net)
{
  _dirtyNets.insert(net);
}

void GlobalRouter::routeClockNets()
{
  startFastRoute();
  std::vector<Net*> clockNets;
  getNetsByType(NetType::Clock, clockNets);
  initializeNets(clockNets);
  applyAdjustments();
  _logger->report("Routing clock nets...");
  _routes = findRouting(clockNets);

  _minLayerForClock = _minRoutingLayer;
  _maxLayerForClock = _maxRoutingLayer;

  getPreviousCapacities(_minLayerForClock, _maxLayerForClock);
  clearFlow();
  _logger->info(GRT, 10, "#Routed clock nets: {}", _routes.size());
}

NetRouteMap GlobalRouter::findRouting(std::vector<Net*>& nets)
{
  NetRouteMap routes = _fastRoute->run();
  addRemainingGuides(routes, nets);
  connectPadPins(routes);
  for (auto& net_route : routes) {
    GRoute& route = net_route.second;
    mergeSegments(route);
  }

  return routes;
}

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta::dbSta* dbSta = _openroad->getSta();
  dbSta->deleteParasitics();

  RcTreeBuilder builder(_openroad, this);
  for (auto& net_route : _routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    if (!route.empty()) {
      Net* net = getNet(db_net);
      builder.estimateParasitcs(db_net, net->getPins(), route);
    }
  }
}

void GlobalRouter::initCoreGrid()
{
  initGrid(_maxRoutingLayer);

  computeCapacities(_maxRoutingLayer, _layerPitches);
  computeSpacingsAndMinWidth(_maxRoutingLayer);
  initObstructions();

  _fastRoute->setLowerLeft(_grid->getLowerLeftX(), _grid->getLowerLeftY());
  _fastRoute->setTileSize(_grid->getTileWidth(), _grid->getTileHeight());
  _fastRoute->setGridsAndLayers(
      _grid->getXGrids(), _grid->getYGrids(), _grid->getNumLayers());
}

void GlobalRouter::initRoutingLayers()
{
  initRoutingLayers(*_routingLayers);

  RoutingLayer routingLayer = getRoutingLayerByIndex(1);
  _fastRoute->setLayerOrientation(routingLayer.getPreferredDirection());
}

void GlobalRouter::initRoutingTracks()
{
  initRoutingTracks(*_allRoutingTracks, _maxRoutingLayer, _layerPitches);
}

void GlobalRouter::setCapacities()
{
  for (int l = 1; l <= _grid->getNumLayers(); l++) {
    if (l < _minRoutingLayer || l > _maxRoutingLayer) {
      _fastRoute->addHCapacity(0, l);
      _fastRoute->addVCapacity(0, l);

      _hCapacities.push_back(0);
      _vCapacities.push_back(0);
    } else {
      _fastRoute->addHCapacity(_grid->getHorizontalEdgesCapacities()[l - 1], l);
      _fastRoute->addVCapacity(_grid->getVerticalEdgesCapacities()[l - 1], l);

      _hCapacities.push_back(_grid->getHorizontalEdgesCapacities()[l - 1]);
      _vCapacities.push_back(_grid->getVerticalEdgesCapacities()[l - 1]);
    }
  }

  for (int l = 1; l <= _grid->getNumLayers(); l++) {
    int newCapH = _grid->getHorizontalEdgesCapacities()[l - 1] * 100;
    _grid->updateHorizontalEdgesCapacities(l - 1, newCapH);

    int newCapV = _grid->getVerticalEdgesCapacities()[l - 1] * 100;
    _grid->updateVerticalEdgesCapacities(l - 1, newCapV);
  }
}

void GlobalRouter::getPreviousCapacities(int previousMinLayer,
                                         int previousMaxLayer)
{
  int oldCap;
  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  oldHUsages = new int**[_grid->getNumLayers()];
  for (int l = 0; l < _grid->getNumLayers(); l++) {
    oldHUsages[l] = new int*[yGrids];
    for (int i = 0; i < yGrids; i++) {
      oldHUsages[l][i] = new int[xGrids];
    }
  }

  oldVUsages = new int**[_grid->getNumLayers()];
  for (int l = 0; l < _grid->getNumLayers(); l++) {
    oldVUsages[l] = new int*[xGrids];
    for (int i = 0; i < xGrids; i++) {
      oldVUsages[l][i] = new int[yGrids];
    }
  }

  int oldTotalCap = 0;
  for (int layer = previousMinLayer; layer <= previousMaxLayer; layer++) {
    for (int y = 1; y < yGrids; y++) {
      for (int x = 1; x < xGrids; x++) {
        oldCap = _fastRoute->getEdgeCurrentResource(
            x - 1, y - 1, layer, x, y - 1, layer);
        oldTotalCap += oldCap;
        oldHUsages[layer - 1][y - 1][x - 1] = oldCap;
      }
    }

    for (int x = 1; x < xGrids; x++) {
      for (int y = 1; y < yGrids; y++) {
        oldCap = _fastRoute->getEdgeCurrentResource(
            x - 1, y - 1, layer, x - 1, y, layer);
        oldTotalCap += oldCap;
        oldVUsages[layer - 1][x - 1][y - 1] = oldCap;
      }
    }
  }
}

void GlobalRouter::restorePreviousCapacities(int previousMinLayer,
                                             int previousMaxLayer)
{
  int oldCap;
  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  int newTotalCap = 0;
  for (int layer = previousMinLayer; layer <= previousMaxLayer; layer++) {
    for (int y = 1; y < yGrids; y++) {
      for (int x = 1; x < xGrids; x++) {
        oldCap = oldHUsages[layer - 1][y - 1][x - 1];
        newTotalCap += oldCap;
        _fastRoute->addAdjustment(
            x - 1, y - 1, layer, x, y - 1, layer, oldCap, true);
      }
    }

    for (int x = 1; x < xGrids; x++) {
      for (int y = 1; y < yGrids; y++) {
        oldCap = oldVUsages[layer - 1][x - 1][y - 1];
        newTotalCap += oldCap;
        _fastRoute->addAdjustment(
            x - 1, y - 1, layer, x - 1, y, layer, oldCap, true);
      }
    }
  }
}

void GlobalRouter::removeDirtyNetsUsage()
{
  for (odb::dbNet* db_net : _dirtyNets) {
    GRoute& netRoute = _routes[db_net];
    int segsCnt = 0;
    for (GSegment& segment : netRoute) {
      if (!(segment.initLayer != segment.finalLayer
            || (segment.initX == segment.finalX
                && segment.initY == segment.finalY))) {
        odb::Point initOnGrid = _grid->getPositionOnGrid(
            odb::Point(segment.initX, segment.initY));
        odb::Point finalOnGrid = _grid->getPositionOnGrid(
            odb::Point(segment.finalX, segment.finalY));

        if (initOnGrid.y() == finalOnGrid.y()) {
          int minX = (initOnGrid.x() <= finalOnGrid.x()) ? initOnGrid.x()
                                                         : finalOnGrid.x();
          int maxX = (initOnGrid.x() > finalOnGrid.x()) ? initOnGrid.x()
                                                        : finalOnGrid.x();

          minX = (minX - (_grid->getTileWidth() / 2)) / _grid->getTileWidth();
          maxX = (maxX - (_grid->getTileWidth() / 2)) / _grid->getTileWidth();
          int y = (initOnGrid.y() - (_grid->getTileHeight() / 2))
                  / _grid->getTileHeight();

          for (int x = minX; x < maxX; x++) {
            int newCap
                = _fastRoute->getEdgeCurrentResource(
                      x, y, segment.initLayer, x + 1, y, segment.initLayer)
                  + 1;
            _fastRoute->addAdjustment(x,
                                      y,
                                      segment.initLayer,
                                      x + 1,
                                      y,
                                      segment.initLayer,
                                      newCap,
                                      false);
          }
        } else if (initOnGrid.x() == finalOnGrid.x()) {
          int minY = (initOnGrid.y() <= finalOnGrid.y()) ? initOnGrid.y()
                                                         : finalOnGrid.y();
          int maxY = (initOnGrid.y() > finalOnGrid.y()) ? initOnGrid.y()
                                                        : finalOnGrid.y();

          minY = (minY - (_grid->getTileHeight() / 2)) / _grid->getTileHeight();
          maxY = (maxY - (_grid->getTileHeight() / 2)) / _grid->getTileHeight();
          int x = (initOnGrid.x() - (_grid->getTileWidth() / 2))
                  / _grid->getTileWidth();

          for (int y = minY; y < maxY; y++) {
            int newCap
                = _fastRoute->getEdgeCurrentResource(
                      x, y, segment.initLayer, x, y + 1, segment.initLayer)
                  + 1;
            _fastRoute->addAdjustment(x,
                                      y,
                                      segment.initLayer,
                                      x,
                                      y + 1,
                                      segment.initLayer,
                                      newCap,
                                      false);
          }
        } else {
          _logger->error(GRT, 70, "Invalid segment for net {}.", db_net->getConstName());
        }
      }
    }
  }
}

void GlobalRouter::updateDirtyNets()
{
  for (odb::dbNet* db_net : _dirtyNets) {
    Net* net = _db_net_map[db_net];
    net->destroyPins();
    makeItermPins(net, db_net, _grid->getGridArea());
    makeBtermPins(net, db_net, _grid->getGridArea());
    findPins(net);
  }
}

void GlobalRouter::setSpacingsAndMinWidths()
{
  for (int l = 1; l <= _grid->getNumLayers(); l++) {
    _fastRoute->addMinSpacing(_grid->getSpacings()[l - 1], l);
    _fastRoute->addMinWidth(_grid->getMinWidths()[l - 1], l);
    _fastRoute->addViaSpacing(1, l);
  }
}

void GlobalRouter::findPins(Net* net)
{
  for (Pin& pin : net->getPins()) {
    odb::Point pinPosition;
    int topLayer = pin.getTopLayer();
    RoutingLayer layer = getRoutingLayerByIndex(topLayer);

    std::vector<odb::Rect> pinBoxes = pin.getBoxes().at(topLayer);
    std::vector<odb::Point> pinPositionsOnGrid;
    odb::Point posOnGrid;
    odb::Point trackPos;

    for (odb::Rect pinBox : pinBoxes) {
      posOnGrid = _grid->getPositionOnGrid(getRectMiddle(pinBox));
      pinPositionsOnGrid.push_back(posOnGrid);
    }

    int votes = -1;

    for (odb::Point pos : pinPositionsOnGrid) {
      int equals = std::count(
          pinPositionsOnGrid.begin(), pinPositionsOnGrid.end(), pos);
      if (equals > votes) {
        pinPosition = pos;
        votes = equals;
      }
    }

    if (pinOverlapsWithSingleTrack(pin, trackPos)) {
      posOnGrid = _grid->getPositionOnGrid(trackPos);
      if (!(posOnGrid == pinPosition)
          && ((layer.getPreferredDirection() == RoutingLayer::HORIZONTAL
               && posOnGrid.y() != pinPosition.y())
              || (layer.getPreferredDirection() == RoutingLayer::VERTICAL
                  && posOnGrid.x() != pinPosition.x()))) {
        pinPosition = posOnGrid;
      }
    }

    pin.setOnGridPosition(pinPosition);
  }
}

void GlobalRouter::findPins(Net* net, std::vector<RoutePt>& pinsOnGrid)
{
  findPins(net);

  for (Pin& pin : net->getPins()) {
    odb::Point pinPosition = pin.getOnGridPosition();
    int topLayer = pin.getTopLayer();
    RoutingLayer layer = getRoutingLayerByIndex(topLayer);
    // If pin is connected to PAD, create a "fake" location in routing
    // grid to avoid PAD obstructions
    if ((pin.isConnectedToPad() || pin.isPort()) && !net->isLocal()) {
      GSegment pinConnection = createFakePin(pin, pinPosition, layer);
      _padPinsConnections[net->getDbNet()].push_back(pinConnection);
    }

    int pinX = (int) ((pinPosition.x() - _grid->getLowerLeftX())
                      / _grid->getTileWidth());
    int pinY = (int) ((pinPosition.y() - _grid->getLowerLeftY())
                      / _grid->getTileHeight());

    if (!(pinX < 0 || pinX >= _grid->getXGrids() || pinY < -1
          || pinY >= _grid->getYGrids() || topLayer > _grid->getNumLayers()
          || topLayer <= 0)) {
      bool invalid = false;
      for (RoutePt& pinPos : pinsOnGrid) {
        if (pinX == pinPos.x() && pinY == pinPos.y()
            && topLayer == pinPos.layer()) {
          invalid = true;
          break;
        }
      }

      if (!invalid) {
        pinsOnGrid.push_back(RoutePt(pinX, pinY, topLayer));
      }
    }
  }
}

void GlobalRouter::initializeNets(std::vector<Net*>& nets)
{
  checkPinPlacement();
  _padPinsConnections.clear();

  int validNets = 0;

  int minDegree = std::numeric_limits<int>::max();
  int maxDegree = std::numeric_limits<int>::min();

  for (const Net& net : *_nets) {
    if (net.getNumPins() > 1) {
      validNets++;
    }
  }

  _fastRoute->setNumberNets(validNets);
  _fastRoute->setMaxNetDegree(getMaxNetDegree());

  for (Net* net : nets) {
    int pin_count = net->getNumPins();
    if (pin_count > 1) {
      if (pin_count < minDegree) {
        minDegree = pin_count;
      }

      if (pin_count > maxDegree) {
        maxDegree = pin_count;
      }

      std::vector<RoutePt> pinsOnGrid;
      findPins(net, pinsOnGrid);

      if (pinsOnGrid.size() > 1) {
        float netAlpha = _alpha;
        if (_netsAlpha.find(net->getName()) != _netsAlpha.end()) {
          netAlpha = _netsAlpha[net->getName()];
        }
        bool isClock = (net->getSignalType() == odb::dbSigType::CLOCK);

        int netID = _fastRoute->addNet(net->getDbNet(),
                                       pinsOnGrid.size(),
                                       pinsOnGrid.size(),
                                       netAlpha,
                                       isClock);
        for (RoutePt& pinPos : pinsOnGrid) {
          _fastRoute->addPin(netID, pinPos.x(), pinPos.y(), pinPos.layer());
        }
      }
    }
  }

  _logger->info(GRT, 1, "Minimum degree: {}", minDegree);
  _logger->info(GRT, 2, "Maximum degree: {}", maxDegree);

  _fastRoute->initEdges();
}

void GlobalRouter::computeGridAdjustments()
{
  odb::Point upperDieBounds
      = odb::Point(_grid->getUpperRightX(), _grid->getUpperRightY());
  int hSpace;
  int vSpace;

  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  odb::Point upperGridBounds = odb::Point(xGrids * _grid->getTileWidth(),
                                          yGrids * _grid->getTileHeight());
  int xExtra = upperDieBounds.x() - upperGridBounds.x();
  int yExtra = upperDieBounds.y() - upperGridBounds.y();

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    hSpace = 0;
    vSpace = 0;
    RoutingLayer routingLayer = getRoutingLayerByIndex(layer);

    if (layer < _minRoutingLayer
        || (layer > _maxRoutingLayer && _maxRoutingLayer > 0))
      continue;

    int newVCapacity = 0;
    int newHCapacity = 0;

    if (routingLayer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
      hSpace = _grid->getMinWidths()[layer - 1];
      newHCapacity = std::floor((_grid->getTileHeight() + yExtra) / hSpace);
    } else if (routingLayer.getPreferredDirection() == RoutingLayer::VERTICAL) {
      vSpace = _grid->getMinWidths()[layer - 1];
      newVCapacity = std::floor((_grid->getTileWidth() + xExtra) / vSpace);
    } else {
      _logger->error(GRT, 71, "Layer spacing not found.");
    }

    int numAdjustments = yGrids - 1 + xGrids - 1;
    _fastRoute->setNumAdjustments(numAdjustments);

    if (!_grid->isPerfectRegularX()) {
      for (int i = 1; i < yGrids; i++) {
        _fastRoute->addAdjustment(xGrids - 1,
                                  i - 1,
                                  layer,
                                  xGrids - 1,
                                  i,
                                  layer,
                                  newVCapacity,
                                  false);
      }
    }
    if (!_grid->isPerfectRegularY()) {
      for (int i = 1; i < xGrids; i++) {
        _fastRoute->addAdjustment(i - 1,
                                  yGrids - 1,
                                  layer,
                                  i,
                                  yGrids - 1,
                                  layer,
                                  newHCapacity,
                                  false);
      }
    }
  }
}

void GlobalRouter::computeTrackAdjustments()
{
  odb::Point upperDieBounds
      = odb::Point(_grid->getUpperRightX(), _grid->getUpperRightY());
  for (RoutingLayer layer : *_routingLayers) {
    int trackLocation;
    int numInitAdjustments = 0;
    int numFinalAdjustments = 0;
    int trackSpace;
    int numTracks = 0;

    if (layer.getIndex() < _minRoutingLayer
        || (layer.getIndex() > _maxRoutingLayer && _maxRoutingLayer > 0))
      continue;

    if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
      RoutingTracks routingTracks = getRoutingTracksByIndex(layer.getIndex());
      trackLocation = routingTracks.getLocation();
      trackSpace = std::max(routingTracks.getTrackPitch(),
                            routingTracks.getLine2ViaPitch());
      numTracks = routingTracks.getNumTracks();

      if (numTracks > 0) {
        int finalTrackLocation = trackLocation + (trackSpace * (numTracks - 1));
        int remainingFinalSpace = upperDieBounds.y() - finalTrackLocation;
        int extraSpace = upperDieBounds.y()
                         - (_grid->getTileHeight() * _grid->getYGrids());
        if (_grid->isPerfectRegularY()) {
          numFinalAdjustments
              = std::ceil((float) remainingFinalSpace / _grid->getTileHeight());
        } else {
          if (remainingFinalSpace != 0) {
            int finalSpace = remainingFinalSpace - extraSpace;
            if (finalSpace <= 0)
              numFinalAdjustments = 1;
            else
              numFinalAdjustments
                  = std::ceil((float) finalSpace / _grid->getTileHeight());
          } else
            numFinalAdjustments = 0;
        }

        numFinalAdjustments *= _grid->getXGrids();
        numInitAdjustments
            = std::ceil((float) trackLocation / _grid->getTileHeight());
        numInitAdjustments *= _grid->getXGrids();
        _fastRoute->setNumAdjustments(numInitAdjustments + numFinalAdjustments);

        int y = 0;
        while (trackLocation >= _grid->getTileHeight()) {
          for (int x = 1; x < _grid->getXGrids(); x++) {
            _fastRoute->addAdjustment(
                x - 1, y, layer.getIndex(), x, y, layer.getIndex(), 0);
          }
          y++;
          trackLocation -= _grid->getTileHeight();
        }
        if (trackLocation > 0) {
          int remainingTile = _grid->getTileHeight() - trackLocation;
          int newCapacity = std::floor((float) remainingTile / trackSpace);
          for (int x = 1; x < _grid->getXGrids(); x++) {
            _fastRoute->addAdjustment(x - 1,
                                      y,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      newCapacity);
          }
        }

        y = _grid->getYGrids() - 1;
        while (remainingFinalSpace >= _grid->getTileHeight() + extraSpace) {
          for (int x = 1; x < _grid->getXGrids(); x++) {
            _fastRoute->addAdjustment(
                x - 1, y, layer.getIndex(), x, y, layer.getIndex(), 0);
          }
          y--;
          remainingFinalSpace -= (_grid->getTileHeight() + extraSpace);
          extraSpace = 0;
        }
        if (remainingFinalSpace > 0) {
          int remainingTile
              = (_grid->getTileHeight() + extraSpace) - remainingFinalSpace;
          int newCapacity = std::floor((float) remainingTile / trackSpace);
          for (int x = 1; x < _grid->getXGrids(); x++) {
            _fastRoute->addAdjustment(x - 1,
                                      y,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      newCapacity);
          }
        }
      }
    } else {
      RoutingTracks routingTracks = getRoutingTracksByIndex(layer.getIndex());
      trackLocation = routingTracks.getLocation();
      trackSpace = std::max(routingTracks.getTrackPitch(),
                            routingTracks.getLine2ViaPitch());
      numTracks = routingTracks.getNumTracks();

      if (numTracks > 0) {
        int finalTrackLocation = trackLocation + (trackSpace * (numTracks - 1));
        int remainingFinalSpace = upperDieBounds.x() - finalTrackLocation;
        int extraSpace
            = upperDieBounds.x() - (_grid->getTileWidth() * _grid->getXGrids());
        if (_grid->isPerfectRegularX()) {
          numFinalAdjustments
              = std::ceil((float) remainingFinalSpace / _grid->getTileWidth());
        } else {
          if (remainingFinalSpace != 0) {
            int finalSpace = remainingFinalSpace - extraSpace;
            if (finalSpace <= 0)
              numFinalAdjustments = 1;
            else
              numFinalAdjustments
                  = std::ceil((float) finalSpace / _grid->getTileWidth());
          } else
            numFinalAdjustments = 0;
        }

        numFinalAdjustments *= _grid->getYGrids();
        numInitAdjustments
            = std::ceil((float) trackLocation / _grid->getTileWidth());
        numInitAdjustments *= _grid->getYGrids();
        _fastRoute->setNumAdjustments(numInitAdjustments + numFinalAdjustments);

        int x = 0;
        while (trackLocation >= _grid->getTileWidth()) {
          for (int y = 1; y < _grid->getYGrids(); y++) {
            _fastRoute->addAdjustment(
                x, y - 1, layer.getIndex(), x, y, layer.getIndex(), 0);
          }
          x++;
          trackLocation -= _grid->getTileWidth();
        }
        if (trackLocation > 0) {
          int remainingTile = _grid->getTileWidth() - trackLocation;
          int newCapacity = std::floor((float) remainingTile / trackSpace);
          for (int y = 1; y < _grid->getYGrids(); y++) {
            _fastRoute->addAdjustment(x,
                                      y - 1,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      newCapacity);
          }
        }

        x = _grid->getXGrids() - 1;
        while (remainingFinalSpace >= _grid->getTileWidth() + extraSpace) {
          for (int y = 1; y < _grid->getYGrids(); y++) {
            _fastRoute->addAdjustment(
                x, y - 1, layer.getIndex(), x, y, layer.getIndex(), 0);
          }
          x--;
          remainingFinalSpace -= (_grid->getTileWidth() + extraSpace);
          extraSpace = 0;
        }
        if (remainingFinalSpace > 0) {
          int remainingTile
              = (_grid->getTileWidth() + extraSpace) - remainingFinalSpace;
          int newCapacity = std::floor((float) remainingTile / trackSpace);
          for (int y = 1; y < _grid->getYGrids(); y++) {
            _fastRoute->addAdjustment(x,
                                      y - 1,
                                      layer.getIndex(),
                                      x,
                                      y,
                                      layer.getIndex(),
                                      newCapacity);
          }
        }
      }
    }
  }
}

void GlobalRouter::computeUserGlobalAdjustments()
{
  if (_adjustment == 0.0)
    return;

  for (int l = _minRoutingLayer; l <= _maxRoutingLayer; l++) {
    if (_adjustments[l] == 0) {
      _adjustments[l] = _adjustment;
    }
  }
}

void GlobalRouter::computeUserLayerAdjustments()
{
  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  for (int layer = 1; layer <= _maxRoutingLayer; layer++) {
    float adjustment = _adjustments[layer];
    if (adjustment != 0) {
      _logger->info(GRT, 13+_maxRoutingLayer+layer, "Reducing resources of layer {} by {}%.", layer, int(adjustment * 100));
      if (_hCapacities[layer - 1] != 0) {
        int newCap = _grid->getHorizontalEdgesCapacities()[layer - 1]
                     * (1 - adjustment);
        _grid->updateHorizontalEdgesCapacities(layer - 1, newCap);

        for (int y = 1; y < yGrids; y++) {
          for (int x = 1; x < xGrids; x++) {
            int edgeCap = _fastRoute->getEdgeCapacity(
                x - 1, y - 1, layer, x, y - 1, layer);
            int newHCapacity = std::floor((float) edgeCap * (1 - adjustment));
            _fastRoute->addAdjustment(
                x - 1, y - 1, layer, x, y - 1, layer, newHCapacity);
          }
        }
      }

      if (_vCapacities[layer - 1] != 0) {
        int newCap
            = _grid->getVerticalEdgesCapacities()[layer - 1] * (1 - adjustment);
        _grid->updateVerticalEdgesCapacities(layer - 1, newCap);

        for (int x = 1; x < xGrids; x++) {
          for (int y = 1; y < yGrids; y++) {
            int edgeCap = _fastRoute->getEdgeCapacity(
                x - 1, y - 1, layer, x - 1, y, layer);
            int newVCapacity = std::floor((float) edgeCap * (1 - adjustment));
            _fastRoute->addAdjustment(
                x - 1, y - 1, layer, x - 1, y, layer, newVCapacity);
          }
        }
      }
    }
  }
}

void GlobalRouter::computeRegionAdjustments(const odb::Rect& region,
                                            int layer,
                                            float reductionPercentage)
{
  odb::Rect firstTileBox;
  odb::Rect lastTileBox;
  std::pair<Grid::TILE, Grid::TILE> tilesToAdjust;

  odb::Rect dieBox = _grid->getGridArea();

  if ((dieBox.xMin() > region.ll().x() && dieBox.yMin() > region.ll().y())
      || (dieBox.xMax() < region.ur().x() && dieBox.yMax() < region.ur().y())) {
    _logger->error(GRT, 72, "Informed region is outside die area.");
  }

  RoutingLayer routingLayer = getRoutingLayerByIndex(layer);
  bool direction = routingLayer.getPreferredDirection();

  tilesToAdjust = _grid->getBlockedTiles(region, firstTileBox, lastTileBox);
  Grid::TILE& firstTile = tilesToAdjust.first;
  Grid::TILE& lastTile = tilesToAdjust.second;

  RoutingTracks routingTracks = getRoutingTracksByIndex(layer);
  int trackSpace = std::max(routingTracks.getTrackPitch(),
                            routingTracks.getLine2ViaPitch());

  int firstTileReduce = _grid->computeTileReduce(
      region, firstTileBox, trackSpace, true, direction);

  int lastTileReduce = _grid->computeTileReduce(
      region, lastTileBox, trackSpace, false, direction);

  // If preferred direction is horizontal, only first and the last line will
  // have specific adjustments
  if (direction == RoutingLayer::HORIZONTAL) {
    // Setting capacities of edges completely contains the adjust region
    // according the percentage of reduction
    for (int x = firstTile._x; x < lastTile._x; x++) {
      for (int y = firstTile._y; y <= lastTile._y; y++) {
        int edgeCap = _fastRoute->getEdgeCapacity(x, y, layer, x + 1, y, layer);

        if (y == firstTile._y) {
          edgeCap -= firstTileReduce;
          if (edgeCap < 0)
            edgeCap = 0;
          _fastRoute->addAdjustment(x, y, layer, x + 1, y, layer, edgeCap);
        } else if (y == lastTile._y) {
          edgeCap -= lastTileReduce;
          if (edgeCap < 0)
            edgeCap = 0;
          _fastRoute->addAdjustment(x, y, layer, x + 1, y, layer, edgeCap);
        } else {
          edgeCap -= edgeCap * reductionPercentage;
          _fastRoute->addAdjustment(x, y, layer, x + 1, y, layer, 0);
        }
      }
    }
  } else {
    // If preferred direction is vertical, only first and last columns will have
    // specific adjustments
    for (int x = firstTile._x; x <= lastTile._x; x++) {
      // Setting capacities of edges completely contains the adjust region
      // according the percentage of reduction
      for (int y = firstTile._y; y < lastTile._y; y++) {
        int edgeCap = _fastRoute->getEdgeCapacity(x, y, layer, x, y + 1, layer);

        if (x == firstTile._x) {
          edgeCap -= firstTileReduce;
          if (edgeCap < 0)
            edgeCap = 0;
          _fastRoute->addAdjustment(x, y, layer, x, y + 1, layer, edgeCap);
        } else if (x == lastTile._x) {
          edgeCap -= lastTileReduce;
          if (edgeCap < 0)
            edgeCap = 0;
          _fastRoute->addAdjustment(x, y, layer, x, y + 1, layer, edgeCap);
        } else {
          edgeCap -= edgeCap * reductionPercentage;
          _fastRoute->addAdjustment(x, y, layer, x, y + 1, layer, 0);
        }
      }
    }
  }
}

void GlobalRouter::computeObstructionsAdjustments()
{
  std::map<int, std::vector<odb::Rect>> obstructions = _grid->getAllObstructions();

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    std::vector<odb::Rect> layerObstructions = obstructions[layer];
    if (!layerObstructions.empty()) {
      RoutingLayer routingLayer = getRoutingLayerByIndex(layer);

      std::pair<Grid::TILE, Grid::TILE> blockedTiles;

      bool direction = routingLayer.getPreferredDirection();

      _logger->info(GRT, 17+layer, "Processing {} blockages on layer {}.", layerObstructions.size(), layer);

      int trackSpace = _grid->getMinWidths()[layer - 1];

      for (odb::Rect& obs : layerObstructions) {
        odb::Rect firstTileBox;
        odb::Rect lastTileBox;

        blockedTiles = _grid->getBlockedTiles(obs, firstTileBox, lastTileBox);

        Grid::TILE& firstTile = blockedTiles.first;
        Grid::TILE& lastTile = blockedTiles.second;

        int firstTileReduce = _grid->computeTileReduce(
            obs, firstTileBox, trackSpace, true, direction);

        int lastTileReduce = _grid->computeTileReduce(
            obs, lastTileBox, trackSpace, false, direction);

        if (direction == RoutingLayer::HORIZONTAL) {
          for (int x = firstTile._x; x < lastTile._x; x++) {
            for (int y = firstTile._y; y <= lastTile._y; y++) {
              if (y == firstTile._y) {
                int edgeCap
                    = _fastRoute->getEdgeCapacity(x, y, layer, x + 1, y, layer);
                edgeCap -= firstTileReduce;
                if (edgeCap < 0)
                  edgeCap = 0;
                _fastRoute->addAdjustment(
                    x, y, layer, x + 1, y, layer, edgeCap);
              } else if (y == lastTile._y) {
                int edgeCap
                    = _fastRoute->getEdgeCapacity(x, y, layer, x + 1, y, layer);
                edgeCap -= lastTileReduce;
                if (edgeCap < 0)
                  edgeCap = 0;
                _fastRoute->addAdjustment(
                    x, y, layer, x + 1, y, layer, edgeCap);
              } else {
                _fastRoute->addAdjustment(x, y, layer, x + 1, y, layer, 0);
              }
            }
          }
        } else {
          for (int x = firstTile._x; x <= lastTile._x; x++) {
            for (int y = firstTile._y; y < lastTile._y; y++) {
              if (x == firstTile._x) {
                int edgeCap
                    = _fastRoute->getEdgeCapacity(x, y, layer, x, y + 1, layer);
                edgeCap -= firstTileReduce;
                if (edgeCap < 0)
                  edgeCap = 0;
                _fastRoute->addAdjustment(
                    x, y, layer, x, y + 1, layer, edgeCap);
              } else if (x == lastTile._x) {
                int edgeCap
                    = _fastRoute->getEdgeCapacity(x, y, layer, x, y + 1, layer);
                edgeCap -= lastTileReduce;
                if (edgeCap < 0)
                  edgeCap = 0;
                _fastRoute->addAdjustment(
                    x, y, layer, x, y + 1, layer, edgeCap);
              } else {
                _fastRoute->addAdjustment(x, y, layer, x, y + 1, layer, 0);
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
  _adjustment = adjustment;
}

void GlobalRouter::setMinRoutingLayer(const int minLayer)
{
  _minRoutingLayer = minLayer;
}

void GlobalRouter::setMaxRoutingLayer(const int maxLayer)
{
  _maxRoutingLayer = maxLayer;
}

void GlobalRouter::setUnidirectionalRoute(const bool unidirRoute)
{
  _unidirectionalRoute = unidirRoute;
}

void GlobalRouter::setAlpha(const float alpha)
{
  _alpha = alpha;
}

void GlobalRouter::setPitchesInTile(const int pitchesInTile)
{
  _grid->setPitchesInTile(pitchesInTile);
}

void GlobalRouter::setSeed(unsigned seed)
{
  _seed = seed;
}

void GlobalRouter::addLayerAdjustment(int layer, float reductionPercentage)
{
  initAdjustments();
  if (layer > _maxRoutingLayer && _maxRoutingLayer > 0) {
    _logger->warn(GRT, 30, "Specified layer {} for adjustment is greater than max routing layer {} and will be ignored.", layer, _maxRoutingLayer);
  } else {
    _adjustments[layer] = reductionPercentage;
  }
}

void GlobalRouter::addRegionAdjustment(int minX,
                                       int minY,
                                       int maxX,
                                       int maxY,
                                       int layer,
                                       float reductionPercentage)
{
  _regionAdjustments.push_back(
      RegionAdjustment(minX, minY, maxX, maxY, layer, reductionPercentage));
}

void GlobalRouter::setLayerPitch(int layer, float pitch)
{
  initPitches();
  _layerPitches[layer] = pitch;
}

void GlobalRouter::addAlphaForNet(char* netName, float alpha)
{
  std::string name(netName);
  _netsAlpha[name] = alpha;
}

void GlobalRouter::setVerbose(const int v)
{
  _verbose = v;
}

void GlobalRouter::setOverflowIterations(int iterations)
{
  _overflowIterations = iterations;
}

void GlobalRouter::setGridOrigin(long x, long y)
{
  *_gridOrigin = odb::Point(x, y);
}

void GlobalRouter::setPDRevForHighFanout(int pdRevForHighFanout)
{
  _pdRevForHighFanout = pdRevForHighFanout;
}

void GlobalRouter::setAllowOverflow(bool allowOverflow)
{
  _allowOverflow = allowOverflow;
}

void GlobalRouter::setReportCongestion(char* congestFile)
{
  _reportCongest = true;
  _congestFile = congestFile;
}

void GlobalRouter::setMacroExtension(int macroExtension)
{
  _macroExtension = macroExtension;
}

void GlobalRouter::writeGuides(const char* fileName)
{
  std::ofstream guideFile;
  guideFile.open(fileName);
  if (!guideFile.is_open()) {
    guideFile.close();
    _logger->error(GRT, 73, "Guides file could not be opened.");
  }
  RoutingLayer phLayerF;

  int offsetX = _gridOrigin->x();
  int offsetY = _gridOrigin->y();

  _logger->info(GRT, 14, "Num routed nets: {}", _routes.size());
  int finalLayer;

  // Sort nets so guide file net order is consistent.
  std::vector<odb::dbNet*> sorted_nets;
  for (odb::dbNet* net : _block->getNets())
    sorted_nets.push_back(net);
  std::sort(sorted_nets.begin(),
            sorted_nets.end(),
            [](odb::dbNet* net1, odb::dbNet* net2) {
              return strcmp(net1->getConstName(), net2->getConstName()) < 0;
            });

  for (odb::dbNet* db_net : sorted_nets) {
    GRoute& route = _routes[db_net];
    if (!route.empty()) {
      guideFile << db_net->getConstName() << "\n";
      guideFile << "(\n";
      std::vector<odb::Rect> guideBox;
      finalLayer = -1;
      for (GSegment& segment : route) {
        if (segment.initLayer != finalLayer && finalLayer != -1) {
          mergeBox(guideBox);
          for (odb::Rect& guide : guideBox) {
            guideFile << guide.xMin() + offsetX << " " << guide.yMin() + offsetY
                      << " " << guide.xMax() + offsetX << " "
                      << guide.yMax() + offsetY << " " << phLayerF.getName()
                      << "\n";
          }
          guideBox.clear();
          finalLayer = segment.initLayer;
        }
        if (segment.initLayer == segment.finalLayer) {
          if (segment.initLayer < _minRoutingLayer
              && segment.initX != segment.finalX
              && segment.initY != segment.finalY) {
            _logger->error(GRT, 74, "Routing with guides in blocked metal for net {}.",
                  db_net->getConstName());
          }

          guideBox.push_back(globalRoutingToBox(segment));
          if (segment.finalLayer < _minRoutingLayer && !_unidirectionalRoute) {
            phLayerF = getRoutingLayerByIndex(
                (segment.finalLayer + (_minRoutingLayer - segment.finalLayer)));
          } else {
            phLayerF = getRoutingLayerByIndex(segment.finalLayer);
          }
          finalLayer = segment.finalLayer;
        } else {
          if (abs(segment.finalLayer - segment.initLayer) > 1) {
            _logger->error(GRT, 75, "Connection between non-adjacent layers in net {}.",
                  db_net->getConstName());
          } else {
            RoutingLayer phLayerI;
            if (segment.initLayer < _minRoutingLayer && !_unidirectionalRoute) {
              phLayerI = getRoutingLayerByIndex(
                  segment.initLayer + _minRoutingLayer - segment.initLayer);
            } else {
              phLayerI = getRoutingLayerByIndex(segment.initLayer);
            }
            if (segment.finalLayer < _minRoutingLayer
                && !_unidirectionalRoute) {
              phLayerF = getRoutingLayerByIndex(
                  segment.finalLayer + _minRoutingLayer - segment.finalLayer);
            } else {
              phLayerF = getRoutingLayerByIndex(segment.finalLayer);
            }
            finalLayer = segment.finalLayer;
            odb::Rect box;
            guideBox.push_back(globalRoutingToBox(segment));
            mergeBox(guideBox);
            for (odb::Rect& guide : guideBox) {
              guideFile << guide.xMin() + offsetX << " "
                        << guide.yMin() + offsetY << " "
                        << guide.xMax() + offsetX << " "
                        << guide.yMax() + offsetY << " " << phLayerI.getName()
                        << "\n";
            }
            guideBox.clear();

            guideBox.push_back(globalRoutingToBox(segment));
          }
        }
      }
      mergeBox(guideBox);
      for (odb::Rect& guide : guideBox) {
        guideFile << guide.xMin() + offsetX << " " << guide.yMin() + offsetY
                  << " " << guide.xMax() + offsetX << " "
                  << guide.yMax() + offsetY << " " << phLayerF.getName()
                  << "\n";
      }
      guideFile << ")\n";
    }
  }

  guideFile.close();
}

RoutingLayer GlobalRouter::getRoutingLayerByIndex(int index)
{
  RoutingLayer selectedRoutingLayer;

  for (RoutingLayer routingLayer : *_routingLayers) {
    if (routingLayer.getIndex() == index) {
      selectedRoutingLayer = routingLayer;
      break;
    }
  }

  return selectedRoutingLayer;
}

RoutingTracks GlobalRouter::getRoutingTracksByIndex(int layer)
{
  RoutingTracks selectedRoutingTracks;

  for (RoutingTracks routingTracks : *_allRoutingTracks) {
    if (routingTracks.getLayerIndex() == layer) {
      selectedRoutingTracks = routingTracks;
    }
  }

  return selectedRoutingTracks;
}

void GlobalRouter::addGuidesForLocalNets(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = _db_net_map[db_net]->getPins();
  int lastLayer = -1;
  for (uint p = 0; p < pins.size(); p++) {
    if (p > 0) {
      odb::Point pinPos0 = findFakePinPosition(pins[p - 1], db_net);
      odb::Point pinPos1 = findFakePinPosition(pins[p], db_net);
      // If the net is not local, FR core result is invalid
      if (pinPos1.x() != pinPos0.x() || pinPos1.y() != pinPos0.y()) {
        _logger->error(GRT, 76, "Net {} not properly covered.", db_net->getConstName());
      }
    }

    if (pins[p].getTopLayer() > lastLayer)
      lastLayer = pins[p].getTopLayer();
  }

  if (lastLayer == _maxRoutingLayer) {
    lastLayer--;
  }

  for (int l = _minRoutingLayer - _fixLayer; l <= lastLayer; l++) {
    odb::Point pinPos = findFakePinPosition(pins[0], db_net);
    GSegment segment
        = GSegment(pinPos.x(), pinPos.y(), l, pinPos.x(), pinPos.y(), l + 1);
    route.push_back(segment);
  }
}

void GlobalRouter::addGuidesForPinAccess(odb::dbNet* db_net, GRoute& route)
{
  std::vector<Pin>& pins = _db_net_map[db_net]->getPins();
  for (Pin& pin : pins) {
    if (pin.getTopLayer() > 1) {
      // for each pin placed at upper layers, get all segments that
      // potentially covers it
      GRoute coverSegs;

      odb::Point pinPos = findFakePinPosition(pin, db_net);

      int wireViaLayer = std::numeric_limits<int>::max();
      for (uint i = 0; i < route.size(); i++) {
        if (((pinPos.x() == route[i].initX && pinPos.y() == route[i].initY)
             || (pinPos.x() == route[i].finalX
                 && pinPos.y() == route[i].finalY))
            && (!(route[i].initX == route[i].finalX
                  && route[i].initY == route[i].finalY))) {
          coverSegs.push_back(route[i]);
          if (route[i].initLayer < wireViaLayer) {
            wireViaLayer = route[i].initLayer;
          }
        }
      }

      bool bottomLayerPin = false;
      for (Pin& pin2 : pins) {
        odb::Point pin2Pos = pin2.getOnGridPosition();
        if (pinPos.x() == pin2Pos.x() && pinPos.y() == pin2Pos.y()
            && pin.getTopLayer() > pin2.getTopLayer()) {
          bottomLayerPin = true;
        }
      }

      if (!bottomLayerPin) {
        for (uint i = 0; i < route.size(); i++) {
          if (((pinPos.x() == route[i].initX && pinPos.y() == route[i].initY)
               || (pinPos.x() == route[i].finalX
                   && pinPos.y() == route[i].finalY))
              && (route[i].initX == route[i].finalX
                  && route[i].initY == route[i].finalY
                  && (route[i].initLayer < wireViaLayer
                      || route[i].finalLayer < wireViaLayer))) {
            // remove all vias to this pin that doesn't connects two wires
            route.erase(route.begin() + i);
            i = 0;
          }
        }
      }

      int closestLayer = -1;
      int minorDiff = std::numeric_limits<int>::max();

      for (GSegment& seg : coverSegs) {
        if (seg.initLayer != seg.finalLayer) {
          _logger->error(GRT, 77, "Segment has invalid layer assignment.");
        }

        int diffLayers = std::abs(pin.getTopLayer() - seg.initLayer);
        if (diffLayers < minorDiff && seg.initLayer > closestLayer) {
          minorDiff = seg.initLayer;
          closestLayer = seg.initLayer;
        }
      }

      if (closestLayer > pin.getTopLayer()) {
        for (int l = closestLayer; l > pin.getTopLayer(); l--) {
          GSegment segment = GSegment(
              pinPos.x(), pinPos.y(), l, pinPos.x(), pinPos.y(), l - 1);
          route.push_back(segment);
        }
      } else if (closestLayer < pin.getTopLayer()) {
        for (int l = closestLayer; l < pin.getTopLayer(); l++) {
          GSegment segment = GSegment(
              pinPos.x(), pinPos.y(), l, pinPos.x(), pinPos.y(), l + 1);
          route.push_back(segment);
        }
      }
    }
  }
}

void GlobalRouter::addRemainingGuides(NetRouteMap& routes,
                                      std::vector<Net*>& nets)
{
  for (Net* net : nets) {
    if (net->getNumPins() > 1) {
      odb::dbNet* db_net = net->getDbNet();
      GRoute& route = routes[db_net];
      if (route.empty()) {
        addGuidesForLocalNets(db_net, route);
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
    if (_padPinsConnections.find(db_net) != _padPinsConnections.end()
        || net->getNumPins() > 1) {
      for (GSegment& segment : _padPinsConnections[db_net]) {
        route.push_back(segment);
      }
    }
  }
}

void GlobalRouter::mergeBox(std::vector<odb::Rect>& guideBox)
{
  std::vector<odb::Rect> finalBox;
  if (guideBox.size() < 1) {
    _logger->error(GRT, 78, "Guides vector is empty.");
  }
  finalBox.push_back(guideBox[0]);
  for (uint i = 1; i < guideBox.size(); i++) {
    odb::Rect box = guideBox[i];
    odb::Rect& lastBox = finalBox.back();
    if (lastBox.overlaps(box)) {
      int lowerX = std::min(lastBox.xMin(), box.xMin());
      int lowerY = std::min(lastBox.yMin(), box.yMin());
      int upperX = std::max(lastBox.xMax(), box.xMax());
      int upperY = std::max(lastBox.yMax(), box.yMax());
      lastBox = odb::Rect(lowerX, lowerY, upperX, upperY);
    } else
      finalBox.push_back(box);
  }
  guideBox.clear();
  guideBox = finalBox;
}

odb::Rect GlobalRouter::globalRoutingToBox(const GSegment& route)
{
  odb::Rect dieBounds = _grid->getGridArea();
  long initX, initY;
  long finalX, finalY;

  if (route.initX < route.finalX) {
    initX = route.initX;
    finalX = route.finalX;
  } else {
    initX = route.finalX;
    finalX = route.initX;
  }

  if (route.initY < route.finalY) {
    initY = route.initY;
    finalY = route.finalY;
  } else {
    initY = route.finalY;
    finalY = route.initY;
  }

  int llX = initX - (_grid->getTileWidth() / 2);
  int llY = initY - (_grid->getTileHeight() / 2);

  int urX = finalX + (_grid->getTileWidth() / 2);
  int urY = finalY + (_grid->getTileHeight() / 2);

  if ((dieBounds.xMax() - urX) / _grid->getTileWidth() < 1) {
    urX = dieBounds.xMax();
  }
  if ((dieBounds.yMax() - urY) / _grid->getTileHeight() < 1) {
    urY = dieBounds.yMax();
  }

  odb::Point lowerLeft = odb::Point(llX, llY);
  odb::Point upperRight = odb::Point(urX, urY);

  odb::Rect routeBds = odb::Rect(lowerLeft, upperRight);
  return routeBds;
}

void GlobalRouter::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<odb::Point>> mapLayerToPositions;

  for (Pin* port : getAllPorts()) {
    if (port->getNumLayers() == 0) {
      _logger->error(GRT, 79, "Pin {} does not have layer assignment.", port->getName().c_str());
    }
    int layer = port->getLayers()[0];  // port have only one layer

    if (mapLayerToPositions[layer].empty()) {
      mapLayerToPositions[layer].push_back(port->getPosition());
    } else {
      for (odb::Point& pos : mapLayerToPositions[layer]) {
        if (pos == port->getPosition()) {
          _logger->warn(GRT, 31, "At least 2 pins in position ({}, {}), layer {}.", pos.x(), pos.y(), layer + 1);
          invalid = true;
        }
      }
      mapLayerToPositions[layer].push_back(port->getPosition());
    }
  }

  if (invalid) {
    _logger->error(GRT, 80, "Invalid pin placement.");
  }
}

GlobalRouter::ROUTE_ GlobalRouter::getRoute()
{
  ROUTE_ route;

  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    for (int y = 1; y < yGrids; y++) {
      for (int x = 1; x < xGrids; x++) {
        int edgeCap
            = _fastRoute->getEdgeCapacity(x - 1, y - 1, layer, x, y - 1, layer);
        if (edgeCap != _grid->getHorizontalEdgesCapacities()[layer - 1]) {
          _numAdjusts++;
        }
      }
    }

    for (int x = 1; x < xGrids; x++) {
      for (int y = 1; y < yGrids; y++) {
        int edgeCap
            = _fastRoute->getEdgeCapacity(x - 1, y - 1, layer, x - 1, y, layer);
        if (edgeCap != _grid->getVerticalEdgesCapacities()[layer - 1]) {
          _numAdjusts++;
        }
      }
    }
  }

  route.gridCountX = _grid->getXGrids();
  route.gridCountY = _grid->getYGrids();
  route.numLayers = _grid->getNumLayers();

  int cnt = 0;
  for (int vCap : _grid->getVerticalEdgesCapacities()) {
    route.verticalEdgesCapacities.push_back(vCap);
    cnt++;
  }

  cnt = 0;
  for (int hCap : _grid->getHorizontalEdgesCapacities()) {
    route.horizontalEdgesCapacities.push_back(hCap);
    cnt++;
  }

  for (uint i = 0; i < _grid->getMinWidths().size(); i++) {
    route.minWireWidths.push_back(100);
  }

  for (int spacing : _grid->getSpacings()) {
    route.minWireSpacings.push_back(spacing);
  }

  for (int i = 0; i < _grid->getNumLayers(); i++) {
    route.viaSpacings.push_back(1);
  }

  route.gridOriginX = _grid->getLowerLeftX();
  route.gridOriginY = _grid->getLowerLeftY();
  route.tileWidth = _grid->getTileWidth();
  route.tileHeight = _grid->getTileHeight();
  route.blockPorosity = 0;
  route.numAdjustments = _numAdjusts;

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    for (int y = 1; y < yGrids; y++) {
      for (int x = 1; x < xGrids; x++) {
        int edgeCap
            = _fastRoute->getEdgeCapacity(x - 1, y - 1, layer, x, y - 1, layer)
              * 100;
        if (edgeCap != _grid->getHorizontalEdgesCapacities()[layer - 1]) {
          ADJUSTMENT_ adj;
          adj.firstX = x - 1;
          adj.firstY = y - 1;
          adj.firstLayer = layer;
          adj.finalX = x;
          adj.finalY = y - 1;
          adj.finalLayer = layer;
          adj.edgeCapacity = edgeCap;

          route.adjustments.push_back(adj);
        }
      }
    }

    for (int x = 1; x < xGrids; x++) {
      for (int y = 1; y < yGrids; y++) {
        int edgeCap
            = _fastRoute->getEdgeCapacity(x - 1, y - 1, layer, x - 1, y, layer)
              * 100;
        if (edgeCap != _grid->getVerticalEdgesCapacities()[layer - 1]) {
          ADJUSTMENT_ adj;
          adj.firstX = x - 1;
          adj.firstY = y - 1;
          adj.firstLayer = layer;
          adj.finalX = x - 1;
          adj.finalY = y;
          adj.finalLayer = layer;
          adj.edgeCapacity = edgeCap;

          route.adjustments.push_back(adj);
        }
      }
    }
  }

  return route;
}

void GlobalRouter::computeWirelength()
{
  long totalWirelength = 0;
  for (auto& net_route : _routes) {
    GRoute& route = net_route.second;
    for (GSegment& segment : route) {
      int segmentWl = std::abs(segment.finalX - segment.initX)
                      + std::abs(segment.finalY - segment.initY);
      totalWirelength += segmentWl;

      if (segmentWl > 0) {
        totalWirelength += (_grid->getTileWidth() + _grid->getTileHeight()) / 2;
      }
    }
  }
  _logger->info(GRT, 18, "Total wirelength: {} um", totalWirelength / _block->getDefUnits());
}

void GlobalRouter::mergeSegments()
{
  for (auto& net_route : _routes) {
    GRoute& route = net_route.second;
    mergeSegments(route);
  }
}

// This needs to be rewritten to shift down undeleted elements instead
// of using erase.
void GlobalRouter::mergeSegments(GRoute& route)
{
  if (!route.empty()) {
    // vector copy - bad bad -cherry
    GRoute segments = route;
    std::map<RoutePt, int> segsAtPoint;
    for (const GSegment& seg : segments) {
      RoutePt pt0 = RoutePt(seg.initX, seg.initY, seg.initLayer);
      RoutePt pt1 = RoutePt(seg.finalX, seg.finalY, seg.finalLayer);
      segsAtPoint[pt0] += 1;
      segsAtPoint[pt1] += 1;
    }

    uint i = 0;
    while (i < segments.size() - 1) {
      GSegment& segment0 = segments[i];
      GSegment& segment1 = segments[i + 1];

      // both segments are not vias
      if (segment0.initLayer == segment0.finalLayer
          && segment1.initLayer == segment1.finalLayer &&
          // segments are on the same layer
          segment0.initLayer == segment1.initLayer) {
        // if segment 0 connects to the end of segment 1
        GSegment& newSeg = segments[i];
        if (segmentsConnect(segment0, segment1, newSeg, segsAtPoint)) {
          segments[i] = newSeg;
          // N^2 again -cherry
          segments.erase(segments.begin() + i + 1);
        } else {
          i++;
        }
      } else {
        i++;
      }
    }
    route = segments;
  }
}

bool GlobalRouter::segmentsConnect(const GSegment& seg0,
                                   const GSegment& seg1,
                                   GSegment& newSeg,
                                   const std::map<RoutePt, int>& segsAtPoint)
{
  long initX0 = std::min(seg0.initX, seg0.finalX);
  long initY0 = std::min(seg0.initY, seg0.finalY);
  long finalX0 = std::max(seg0.finalX, seg0.initX);
  long finalY0 = std::max(seg0.finalY, seg0.initY);

  long initX1 = std::min(seg1.initX, seg1.finalX);
  long initY1 = std::min(seg1.initY, seg1.finalY);
  long finalX1 = std::max(seg1.finalX, seg1.initX);
  long finalY1 = std::max(seg1.finalY, seg1.initY);

  // vertical segments aligned
  if (initX0 == finalX0 && initX1 == finalX1 && initX0 == initX1) {
    bool merge = false;
    if (initY0 == finalY1) {
      RoutePt pt = RoutePt(initX0, initY0, seg0.initLayer);
      merge = segsAtPoint.at(pt) == 2;
    } else if (finalY0 == initY1) {
      RoutePt pt = RoutePt(initX1, initY1, seg1.initLayer);
      merge = segsAtPoint.at(pt) == 2;
    }
    if (merge) {
      newSeg.initX = std::min(initX0, initX1);
      newSeg.initY = std::min(initY0, initY1);
      newSeg.finalX = std::max(finalX0, finalX1);
      newSeg.finalY = std::max(finalY0, finalY1);
      return true;
    }
    // horizontal segments aligned
  } else if (initY0 == finalY0 && initY1 == finalY1 && initY0 == initY1) {
    bool merge = false;
    if (initX0 == finalX1) {
      RoutePt pt = RoutePt(initX0, initY0, seg0.initLayer);
      merge = segsAtPoint.at(pt) == 2;
    } else if (finalX0 == initX1) {
      RoutePt pt = RoutePt(initX1, initY1, seg1.initLayer);
      merge = segsAtPoint.at(pt) == 2;
    }
    if (merge) {
      newSeg.initX = std::min(initX0, initX1);
      newSeg.initY = std::min(initY0, initY1);
      newSeg.finalX = std::max(finalX0, finalX1);
      newSeg.finalY = std::max(finalY0, finalY1);
      return true;
    }
  }

  return false;
}

void GlobalRouter::addLocalConnections(NetRouteMap& routes)
{
  int topLayer;
  std::vector<odb::Rect> pinBoxes;
  odb::Point pinPosition;
  odb::Point realPinPosition;
  GSegment horSegment;
  GSegment verSegment;

  for (auto& net_route : routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    Net* net = getNet(db_net);

    for (Pin& pin : net->getPins()) {
      topLayer = pin.getTopLayer();
      pinBoxes = pin.getBoxes().at(topLayer);
      pinPosition = pin.getOnGridPosition();
      realPinPosition = getRectMiddle(pinBoxes[0]);

      horSegment = GSegment(realPinPosition.x(),
                            realPinPosition.y(),
                            topLayer,
                            pinPosition.x(),
                            realPinPosition.y(),
                            topLayer);
      verSegment = GSegment(pinPosition.x(),
                            realPinPosition.y(),
                            topLayer,
                            pinPosition.x(),
                            pinPosition.y(),
                            topLayer);

      route.push_back(horSegment);
      route.push_back(verSegment);
    }
  }
}

void GlobalRouter::mergeResults(NetRouteMap& routes)
{
  for (auto& net_route : routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute& route = net_route.second;
    _routes[db_net] = route;
  }
}

bool GlobalRouter::pinOverlapsWithSingleTrack(const Pin& pin,
                                              odb::Point& trackPosition)
{
  int minX = std::numeric_limits<int>::max();
  int minY = std::numeric_limits<int>::max();
  int maxX = std::numeric_limits<int>::min();
  int maxY = std::numeric_limits<int>::min();

  int min, max;

  int topLayer = pin.getTopLayer();
  std::vector<odb::Rect> pinBoxes = pin.getBoxes().at(topLayer);

  RoutingLayer layer = getRoutingLayerByIndex(topLayer);
  RoutingTracks tracks = getRoutingTracksByIndex(topLayer);

  for (odb::Rect pinBox : pinBoxes) {
    if (pinBox.xMin() <= minX)
      minX = pinBox.xMin();

    if (pinBox.yMin() <= minY)
      minY = pinBox.yMin();

    if (pinBox.xMax() >= maxX)
      maxX = pinBox.xMax();

    if (pinBox.yMax() >= maxY)
      maxY = pinBox.yMax();
  }

  odb::Point middle
      = odb::Point((minX + (maxX - minX) / 2.0), (minY + (maxY - minY) / 2.0));
  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    min = minY;
    max = maxY;

    if ((float) (max - min) / tracks.getTrackPitch() <= 3) {
      int nearestTrack = std::floor((float) (max - tracks.getLocation())
                                    / tracks.getTrackPitch())
                             * tracks.getTrackPitch()
                         + tracks.getLocation();
      int nearestTrack2 = std::floor((float) (max - tracks.getLocation())
                                         / tracks.getTrackPitch()
                                     - 1)
                              * tracks.getTrackPitch()
                          + tracks.getLocation();

      if ((nearestTrack >= min && nearestTrack <= max)
          && (nearestTrack2 >= min && nearestTrack2 <= max)) {
        return false;
      }

      if (nearestTrack >= min && nearestTrack <= max) {
        trackPosition = odb::Point(middle.x(), nearestTrack);
        return true;
      } else if (nearestTrack2 >= min && nearestTrack2 <= max) {
        trackPosition = odb::Point(middle.x(), nearestTrack2);
        return true;
      } else {
        return false;
      }
    }
  } else {
    min = minX;
    max = maxX;

    if ((float) (max - min) / tracks.getTrackPitch() <= 3) {
      // begging for subexpression factoring -cherry
      int nearestTrack = std::floor((float) (max - tracks.getLocation())
                                    / tracks.getTrackPitch())
                             * tracks.getTrackPitch()
                         + tracks.getLocation();
      int nearestTrack2 = std::floor((float) (max - tracks.getLocation())
                                         / tracks.getTrackPitch()
                                     - 1)
                              * tracks.getTrackPitch()
                          + tracks.getLocation();

      if ((nearestTrack >= min && nearestTrack <= max)
          && (nearestTrack2 >= min && nearestTrack2 <= max)) {
        return false;
      }

      if (nearestTrack >= min && nearestTrack <= max) {
        trackPosition = odb::Point(nearestTrack, middle.y());
        return true;
      } else if (nearestTrack2 >= min && nearestTrack2 <= max) {
        trackPosition = odb::Point(nearestTrack2, middle.y());
        return true;
      } else {
        return false;
      }
    }
  }

  return false;
}

GSegment GlobalRouter::createFakePin(Pin pin,
                                     odb::Point& pinPosition,
                                     RoutingLayer layer)
{
  int topLayer = layer.getIndex();
  GSegment pinConnection;
  pinConnection.initLayer = topLayer;
  pinConnection.finalLayer = topLayer;

  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    pinConnection.finalX = pinPosition.x();
    pinConnection.initY = pinPosition.y();
    pinConnection.finalY = pinPosition.y();

    int newXPosition;
    if (pin.getOrientation() == PinOrientation::west) {
      newXPosition = pinPosition.x() + (_gcellsOffset * _grid->getTileWidth());
      if (newXPosition <= _grid->getUpperRightX()) {
        pinConnection.initX = newXPosition;
        pinPosition.setX(newXPosition);
      }
    } else if (pin.getOrientation() == PinOrientation::east) {
      newXPosition = pinPosition.x() - (_gcellsOffset * _grid->getTileWidth());
      if (newXPosition >= _grid->getLowerLeftX()) {
        pinConnection.initX = newXPosition;
        pinPosition.setX(newXPosition);
      }
    } else {
      _logger->warn(GRT, 32, "Pin {} has invalid orientation.", pin.getName());
    }
  } else {
    pinConnection.initX = pinPosition.x();
    pinConnection.finalX = pinPosition.x();
    pinConnection.finalY = pinPosition.y();

    int newYPosition;
    if (pin.getOrientation() == PinOrientation::south) {
      newYPosition = pinPosition.y() + (_gcellsOffset * _grid->getTileHeight());
      if (newYPosition <= _grid->getUpperRightY()) {
        pinConnection.initY = newYPosition;
        pinPosition.setY(newYPosition);
      }
    } else if (pin.getOrientation() == PinOrientation::north) {
      newYPosition = pinPosition.y() - (_gcellsOffset * _grid->getTileHeight());
      if (newYPosition >= _grid->getLowerLeftY()) {
        pinConnection.initY = newYPosition;
        pinPosition.setY(newYPosition);
      }
    } else {
      _logger->warn(GRT, 33, "Pin {} has invalid orientation.", pin.getName());
    }
  }

  return pinConnection;
}

odb::Point GlobalRouter::findFakePinPosition(Pin& pin, odb::dbNet* db_net)
{
  odb::Point fakePos = pin.getOnGridPosition();
  Net* net = _db_net_map[db_net];
  if ((pin.isConnectedToPad() || pin.isPort()) && !net->isLocal()) {
    RoutingLayer layer = getRoutingLayerByIndex(pin.getTopLayer());
    createFakePin(pin, fakePos, layer);
  }

  return fakePos;
}

void GlobalRouter::initAdjustments()
{
  if (_adjustments.empty()) {
    _adjustments.resize(_db->getTech()->getRoutingLayerCount() + 1, 0);
  }
}

void GlobalRouter::initPitches()
{
  if (_layerPitches.empty()) {
    _layerPitches.resize(_db->getTech()->getRoutingLayerCount() + 1, 0);
  }
}

int GlobalRouter::getNetCount() const
{
  return _nets->size();
}

Net* GlobalRouter::addNet(odb::dbNet* db_net)
{
  _nets->push_back(Net(db_net));
  Net* net = &_nets->back();
  return net;
}

void GlobalRouter::reserveNets(size_t net_count)
{
  _nets->reserve(net_count);
}

int GlobalRouter::getMaxNetDegree()
{
  int maxDegree = -1;
  for (Net& net : *_nets) {
    int netDegree = net.getNumPins();
    if (netDegree > maxDegree) {
      maxDegree = netDegree;
    }
  }
  return maxDegree;
}

std::vector<Pin*> GlobalRouter::getAllPorts()
{
  std::vector<Pin*> ports;
  for (Net& net : *_nets) {
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

void GlobalRouter::initGrid(int maxLayer)
{
  _block = _db->getChip()->getBlock();

  odb::dbTech* tech = _db->getTech();

  odb::dbTechLayer* selectedLayer = tech->findRoutingLayer(selectedMetal);

  if (selectedLayer == nullptr) {
    _logger->error(GRT, 81, "Layer {} not found.", selectedMetal);
  }

  odb::dbTrackGrid* selectedTrack = _block->findTrackGrid(selectedLayer);

  if (selectedTrack == nullptr) {
    _logger->error(GRT, 82, "Track for layer {} not found.", selectedMetal);
  }

  int trackStepX, trackStepY;
  int initTrackX, numTracksX;
  int initTrackY, numTracksY;
  int trackSpacing;

  selectedTrack->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
  selectedTrack->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

  if (selectedLayer->getDirection().getValue()
      == odb::dbTechLayerDir::HORIZONTAL) {
    trackSpacing = trackStepY;
  } else if (selectedLayer->getDirection().getValue()
             == odb::dbTechLayerDir::VERTICAL) {
    trackSpacing = trackStepX;
  } else {
    _logger->error(GRT, 83, "Layer {} does not have valid direction.", selectedMetal);
  }

  odb::Rect rect;
  _block->getDieArea(rect);

  int lowerLeftX = rect.xMin();
  int lowerLeftY = rect.yMin();

  int upperRightX = rect.xMax();
  int upperRightY = rect.yMax();

  int tileWidth = _grid->getPitchesInTile() * trackSpacing;
  int tileHeight = _grid->getPitchesInTile() * trackSpacing;

  int xGrids = std::floor((float) upperRightX / tileWidth);
  int yGrids = std::floor((float) upperRightY / tileHeight);

  bool perfectRegularX = false;
  bool perfectRegularY = false;

  int numLayers = tech->getRoutingLayerCount();
  if (maxLayer > -1) {
    numLayers = maxLayer;
  }

  if ((xGrids * tileWidth) == upperRightX)
    perfectRegularX = true;

  if ((yGrids * tileHeight) == upperRightY)
    perfectRegularY = true;

  std::vector<int> genericVector(numLayers);
  std::map<int, std::vector<odb::Rect>> genericMap;

  _grid->init(lowerLeftX,
              lowerLeftY,
              rect.xMax(),
              rect.yMax(),
              tileWidth,
              tileHeight,
              xGrids,
              yGrids,
              perfectRegularX,
              perfectRegularY,
              numLayers,
              genericVector,
              genericVector,
              genericVector,
              genericVector,
              genericMap);
}

void GlobalRouter::initRoutingLayers(std::vector<RoutingLayer>& routingLayers)
{
  odb::dbTech* tech = _db->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    odb::dbTechLayer* techLayer = tech->findRoutingLayer(l);
    int index = l;
    std::string name = techLayer->getConstName();
    bool preferredDirection;
    if (techLayer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      preferredDirection = RoutingLayer::HORIZONTAL;
    } else if (techLayer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      preferredDirection = RoutingLayer::VERTICAL;
    } else {
      _logger->error(GRT, 84, "Layer {} does not have valid direction.", l);
    }

    RoutingLayer routingLayer = RoutingLayer(index, name, preferredDirection);
    routingLayers.push_back(routingLayer);
  }
}

void GlobalRouter::initRoutingTracks(
    std::vector<RoutingTracks>& allRoutingTracks,
    int maxLayer,
    std::vector<float> layerPitches)
{
  odb::dbTech* tech = _db->getTech();

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    if (layer > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(layer);

    if (techLayer == nullptr) {
      _logger->error(GRT, 85, "Layer {} not found.", selectedMetal);
    }

    odb::dbTrackGrid* selectedTrack = _block->findTrackGrid(techLayer);

    if (selectedTrack == nullptr) {
      _logger->error(GRT, 86, "Track for layer {} not found.", selectedMetal);
    }

    int trackStepX, trackStepY;
    int initTrackX, numTracksX;
    int initTrackY, numTracksY;
    int trackPitch, line2ViaPitch, location, numTracks;
    bool orientation;

    selectedTrack->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
    selectedTrack->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

    if (techLayer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      trackPitch = trackStepY;
      if (layerPitches[layer] != 0) {
        line2ViaPitch = (int) (tech->getLefUnits() * layerPitches[layer]);
      } else {
        line2ViaPitch = -1;
      }
      location = initTrackY;
      numTracks = numTracksY;
      orientation = RoutingLayer::HORIZONTAL;
    } else if (techLayer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      trackPitch = trackStepX;
      if (layerPitches[layer] != 0) {
        line2ViaPitch = (int) (tech->getLefUnits() * layerPitches[layer]);
      } else {
        line2ViaPitch = -1;
      }
      location = initTrackX;
      numTracks = numTracksX;
      orientation = RoutingLayer::VERTICAL;
    } else {
      _logger->error(GRT, 87, "Layer {} does not have valid direction.", selectedMetal);
    }

    RoutingTracks routingTracks = RoutingTracks(
        layer, trackPitch, line2ViaPitch, location, numTracks, orientation);
    allRoutingTracks.push_back(routingTracks);
  }
}

void GlobalRouter::computeCapacities(int maxLayer,
                                     std::vector<float> layerPitches)
{
  int trackSpacing;
  int hCapacity, vCapacity;
  int trackStepX, trackStepY;

  int initTrackX, numTracksX;
  int initTrackY, numTracksY;

  odb::dbTech* tech = _db->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(l);

    odb::dbTrackGrid* track = _block->findTrackGrid(techLayer);

    if (track == nullptr) {
      _logger->error(GRT, 88, "Track for layer {} not found.", l);
    }

    track->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
    track->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

    if (techLayer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      trackSpacing = trackStepY;

      if (layerPitches[l] != 0) {
        int layerPitch = (int) (tech->getLefUnits() * layerPitches[l]);
        trackSpacing = std::max(layerPitch, trackStepY);
      }

      hCapacity = std::floor((float) _grid->getTileWidth() / trackSpacing);

      _grid->addHorizontalCapacity(hCapacity, l - 1);
      _grid->addVerticalCapacity(0, l - 1);
    } else if (techLayer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      trackSpacing = trackStepX;

      if (layerPitches[l] != 0) {
        int layerPitch = (int) (tech->getLefUnits() * layerPitches[l]);
        trackSpacing = std::max(layerPitch, trackStepX);
      }

      vCapacity = std::floor((float) _grid->getTileWidth() / trackSpacing);

      _grid->addHorizontalCapacity(0, l - 1);
      _grid->addVerticalCapacity(vCapacity, l - 1);
    } else {
      _logger->error(GRT, 89, "Layer {} does not have valid direction.", l);
    }
  }
}

void GlobalRouter::computeSpacingsAndMinWidth(int maxLayer)
{
  int minSpacing = 0;
  int minWidth;
  int trackStepX, trackStepY;
  int initTrackX, numTracksX;
  int initTrackY, numTracksY;

  odb::dbTech* tech = _db->getTech();

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(l);

    odb::dbTrackGrid* track = _block->findTrackGrid(techLayer);

    if (track == nullptr) {
      _logger->error(GRT, 90, "Track for layer {} not found.", l);
    }

    track->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
    track->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

    if (techLayer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      minWidth = trackStepY;
    } else if (techLayer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      minWidth = trackStepX;
    } else {
      _logger->error(GRT, 91, "Layer {} does not have valid direction.", l);
    }

    _grid->addSpacing(minSpacing, l - 1);
    _grid->addMinWidth(minWidth, l - 1);
  }
}

void GlobalRouter::initNetlist()
{
  if (_nets->empty()) {
    initClockNets();
    std::set<odb::dbNet*> db_nets;

    for (odb::dbNet* net : _block->getNets()) {
      db_nets.insert(net);
    }

    if (db_nets.empty()) {
      _logger->error(GRT, 92, "Design without nets.");
    }

    addNets(db_nets);
  }
}

void GlobalRouter::addNets(std::set<odb::dbNet*>& db_nets)
{
  // Prevent _nets from growing because pointers to nets become invalid.
  reserveNets(db_nets.size());
  for (odb::dbNet* db_net : db_nets) {
    if (db_net->getSigType().getValue() != odb::dbSigType::POWER
        && db_net->getSigType().getValue() != odb::dbSigType::GROUND
        && !db_net->isSpecial() && db_net->getSWires().empty()) {
      Net* net = addNet(db_net);
      _db_net_map[db_net] = net;
      makeItermPins(net, db_net, _grid->getGridArea());
      makeBtermPins(net, db_net, _grid->getGridArea());
      findPins(net);
    }
  }
}

Net* GlobalRouter::getNet(odb::dbNet* db_net)
{
  return _db_net_map[db_net];
}

void GlobalRouter::getNetsByType(NetType type, std::vector<Net*>& nets)
{
  if (type == NetType::Clock || type == NetType::Signal) {
    bool getClock = type == NetType::Clock;
    for (Net net : *_nets) {
      if ((getClock && net.getSignalType() == odb::dbSigType::CLOCK
           && !clockHasLeafITerm(net.getDbNet()))
          || (!getClock
              && (net.getSignalType() != odb::dbSigType::CLOCK
                  || clockHasLeafITerm(net.getDbNet())))) {
        nets.push_back(_db_net_map[net.getDbNet()]);
      }
    }
  } else if (type == NetType::Antenna) {
    for (odb::dbNet* db_net : _dirtyNets) {
      nets.push_back(_db_net_map[db_net]);
    }
  } else {
    for (Net net : *_nets) {
      nets.push_back(_db_net_map[net.getDbNet()]);
    }
  }
}

void GlobalRouter::initClockNets()
{
  std::set<odb::dbNet*> clockNets = _sta->findClkNets();

  _logger->info(GRT, 17, "Found {} clock nets.", clockNets.size());

  for (odb::dbNet* net : clockNets) {
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
  sta::dbNetwork* network = _sta->getDbNetwork();
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
                                 const odb::Rect& dieArea)
{
  odb::dbTech* tech = _db->getTech();
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    int pX, pY;
    std::vector<int> pinLayers;
    std::map<int, std::vector<odb::Rect>> pinBoxes;

    odb::dbMTerm* mTerm = iterm->getMTerm();
    odb::dbMaster* master = mTerm->getMaster();

    if (master->getType() == odb::dbMasterType::COVER
        || master->getType() == odb::dbMasterType::COVER_BUMP) {
      _logger->warn(GRT, 34, "Net connected with instance of class COVER added for routing.");
    }

    bool connectedToPad = master->getType().isPad();
    bool connectedToMacro = master->isBlock();

    odb::Point pinPos;

    odb::dbInst* inst = iterm->getInst();
    inst->getOrigin(pX, pY);
    odb::Point origin = odb::Point(pX, pY);
    odb::dbTransform transform(inst->getOrient(), origin);

    odb::dbBox* instBox = inst->getBBox();
    odb::Point instMiddle = odb::Point(
        (instBox->xMin() + (instBox->xMax() - instBox->xMin()) / 2.0),
        (instBox->yMin() + (instBox->yMax() - instBox->yMin()) / 2.0));

    for (odb::dbMPin* mterm : mTerm->getMPins()) {
      odb::Point lowerBound;
      odb::Point upperBound;
      odb::Rect pinBox;
      int pinLayer;
      int lastLayer = -1;

      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::Rect rect;
        box->getBox(rect);
        transform.apply(rect);

        odb::dbTechLayer* techLayer = box->getTechLayer();
        if (techLayer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        pinLayer = techLayer->getRoutingLevel();
        lowerBound = odb::Point(rect.xMin(), rect.yMin());
        upperBound = odb::Point(rect.xMax(), rect.yMax());
        pinBox = odb::Rect(lowerBound, upperBound);
        if (!dieArea.contains(pinBox)) {
          _logger->warn(GRT, 35, "Pin {} is outside die area.", getITermName(iterm));
        }
        pinBoxes[pinLayer].push_back(pinBox);
        if (pinLayer > lastLayer) {
          pinPos = lowerBound;
        }
      }
    }

    for (auto& layer_boxes : pinBoxes) {
      pinLayers.push_back(layer_boxes.first);
    }

    Pin pin(iterm,
            pinPos,
            pinLayers,
            PinOrientation::invalid,
            pinBoxes,
            (connectedToPad || connectedToMacro));

    if (connectedToPad || connectedToMacro) {
      odb::Point pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int instToPin = pinPosition.x() - instMiddle.x();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int instToPin = pinPosition.y() - instMiddle.y();
        if (instToPin < 0) {
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
                                 const odb::Rect& dieArea)
{
  odb::dbTech* tech = _db->getTech();
  for (odb::dbBTerm* bterm : db_net->getBTerms()) {
    int posX, posY;
    std::string pinName;

    bterm->getFirstPinLocation(posX, posY);
    odb::dbITerm* iterm = bterm->getITerm();
    bool connectedToPad = false;
    bool connectedToMacro = false;
    odb::Point instMiddle = odb::Point(-1, -1);

    if (iterm != nullptr) {
      odb::dbMTerm* mterm = iterm->getMTerm();
      odb::dbMaster* master = mterm->getMaster();
      connectedToPad = master->getType().isPad();
      connectedToMacro = master->isBlock();

      odb::dbInst* inst = iterm->getInst();
      odb::dbBox* instBox = inst->getBBox();
      instMiddle = odb::Point(
          (instBox->xMin() + (instBox->xMax() - instBox->xMin()) / 2.0),
          (instBox->yMin() + (instBox->yMax() - instBox->yMin()) / 2.0));
    }

    std::vector<int> pinLayers;
    std::map<int, std::vector<odb::Rect>> pinBoxes;

    pinName = bterm->getConstName();
    odb::Point pinPos;

    for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
      odb::Point lowerBound;
      odb::Point upperBound;
      odb::Rect pinBox;
      int pinLayer;
      int lastLayer = -1;

      for (odb::dbBox* currBPinBox : bterm_pin->getBoxes()) {
        odb::dbTechLayer* techLayer = currBPinBox->getTechLayer();
        if (techLayer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        pinLayer = techLayer->getRoutingLevel();
        lowerBound = odb::Point(currBPinBox->xMin(), currBPinBox->yMin());
        upperBound = odb::Point(currBPinBox->xMax(), currBPinBox->yMax());
        pinBox = odb::Rect(lowerBound, upperBound);
        if (!dieArea.contains(pinBox)) {
          _logger->warn(GRT, 36, "Pin {} is outside die area.", pinName);
        }
        pinBoxes[pinLayer].push_back(pinBox);

        if (pinLayer > lastLayer) {
          pinPos = lowerBound;
        }
      }
    }

    for (auto& layer_boxes : pinBoxes) {
      pinLayers.push_back(layer_boxes.first);
    }

    Pin pin(bterm,
            pinPos,
            pinLayers,
            PinOrientation::invalid,
            pinBoxes,
            (connectedToPad || connectedToMacro));

    if (pin.getLayers().empty()) {
      _logger->error(GRT, 93, "Pin {} does not have layer assignment.", bterm->getConstName());
    }

    if (connectedToPad) {
      odb::Point pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int instToPin = pinPosition.x() - instMiddle.x();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int instToPin = pinPosition.y() - instMiddle.y();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::north);
        } else {
          pin.setOrientation(PinOrientation::south);
        }
      }
    } else {
      odb::Point pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        int instToDie = pinPosition.x() - getRectMiddle(dieArea).x();
        if (instToDie < 0) {
          pin.setOrientation(PinOrientation::west);
        } else {
          pin.setOrientation(PinOrientation::east);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        int instToDie = pinPosition.y() - getRectMiddle(dieArea).y();
        if (instToDie < 0) {
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
  odb::dbMTerm* mTerm = iterm->getMTerm();
  odb::dbMaster* master = mTerm->getMaster();
  std::string pin_name = iterm->getInst()->getConstName();
  pin_name += "/";
  pin_name += mTerm->getConstName();
  return pin_name;
}

void GlobalRouter::initObstructions()
{
  odb::Rect dieArea(_grid->getLowerLeftX(),
                    _grid->getLowerLeftY(),
                    _grid->getUpperRightX(),
                    _grid->getUpperRightY());
  std::vector<int> layerExtensions;

  findLayerExtensions(layerExtensions);
  int obstructionsCnt = findObstructions(dieArea);
  obstructionsCnt += findInstancesObstructions(dieArea, layerExtensions);
  findNetsObstructions(dieArea);

  _logger->info(GRT, 4, "Blockages: {}", obstructionsCnt);
}

void GlobalRouter::findLayerExtensions(std::vector<int>& layerExtensions)
{
  odb::dbTech* tech = _db->getTech();
  layerExtensions.resize(tech->getRoutingLayerCount() + 1, 0);

  for (odb::dbTechLayer* obstructLayer : tech->getLayers()) {
    if (obstructLayer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
      continue;
    }

    int maxInt = std::numeric_limits<int>::max();

    // Gets the smallest possible minimum spacing that won't cause violations
    // for ANY configuration of PARALLELRUNLENGTH (the biggest value in the
    // table)

    int spacingExtension = obstructLayer->getSpacing(maxInt, maxInt);

    odb::dbSet<odb::dbTechLayerSpacingRule> eolRules;

    // Check for EOL spacing values and, if the spacing is higher than the one
    // found, use them as the macro extension instead of PARALLELRUNLENGTH

    if (obstructLayer->getV54SpacingRules(eolRules)) {
      for (odb::dbTechLayerSpacingRule* currentRule : eolRules) {
        int currentSpacing = currentRule->getSpacing();
        if (currentSpacing > spacingExtension) {
          spacingExtension = currentSpacing;
        }
      }
    }

    // Check for TWOWIDTHS table values and, if the spacing is higher than the
    // one found, use them as the macro extension instead of PARALLELRUNLENGTH

    if (obstructLayer->hasTwoWidthsSpacingRules()) {
      std::vector<std::vector<uint>> spacingTable;
      obstructLayer->getTwoWidthsSpacingTable(spacingTable);
      if (!spacingTable.empty()) {
        std::vector<uint> lastRow = spacingTable.back();
        uint lastValue = lastRow.back();
        if (lastValue > spacingExtension) {
          spacingExtension = lastValue;
        }
      }
    }

    // Save the extension to use when defining Macros

    layerExtensions[obstructLayer->getRoutingLevel()] = spacingExtension;
  }
}

int GlobalRouter::findObstructions(odb::Rect& dieArea)
{
  int obstructionsCnt = 0;
  for (odb::dbObstruction* currObstruct : _block->getObstructions()) {
    odb::dbBox* obstructBox = currObstruct->getBBox();

    int layer = obstructBox->getTechLayer()->getRoutingLevel();

    odb::Point lowerBound
        = odb::Point(obstructBox->xMin(), obstructBox->yMin());
    odb::Point upperBound
        = odb::Point(obstructBox->xMax(), obstructBox->yMax());
    odb::Rect obstructionBox = odb::Rect(lowerBound, upperBound);
    if (!dieArea.contains(obstructionBox)) {
      _logger->warn(GRT, 37, "Found blockage outside die area.");
    }
    _grid->addObstruction(layer, obstructionBox);
    obstructionsCnt++;
  }

  return obstructionsCnt;
}

int GlobalRouter::findInstancesObstructions(
    odb::Rect& dieArea,
    const std::vector<int>& layerExtensions)
{
  int macrosCnt = 0;
  int obstructionsCnt = 0;
  for (odb::dbInst* currInst : _block->getInsts()) {
    int pX, pY;

    odb::dbMaster* master = currInst->getMaster();

    currInst->getOrigin(pX, pY);
    odb::Point origin = odb::Point(pX, pY);

    odb::dbTransform transform(currInst->getOrient(), origin);

    bool isMacro = false;
    if (master->isBlock()) {
      macrosCnt++;
      isMacro = true;
    }

    for (odb::dbBox* currBox : master->getObstructions()) {
      int layer = currBox->getTechLayer()->getRoutingLevel();

      odb::Rect rect;
      currBox->getBox(rect);
      transform.apply(rect);

      int layerExtension = 0;

      if (isMacro) {
        layerExtension
            = layerExtensions[currBox->getTechLayer()->getRoutingLevel()];
        layerExtension += _macroExtension * _grid->getTileWidth();
      }

      odb::Point lowerBound = odb::Point(rect.xMin() - layerExtension,
                                         rect.yMin() - layerExtension);
      odb::Point upperBound = odb::Point(rect.xMax() + layerExtension,
                                         rect.yMax() + layerExtension);
      odb::Rect obstructionBox = odb::Rect(lowerBound, upperBound);
      if (!dieArea.contains(obstructionBox)) {
        _logger->warn(GRT, 38, "Found blockage outside die area in instance {}.", currInst->getConstName());
      }
      _grid->addObstruction(layer, obstructionBox);
      obstructionsCnt++;
    }

    for (odb::dbMTerm* mTerm : master->getMTerms()) {
      for (odb::dbMPin* mterm : mTerm->getMPins()) {
        odb::Point lowerBound;
        odb::Point upperBound;
        odb::Rect pinBox;
        int pinLayer;

        for (odb::dbBox* box : mterm->getGeometry()) {
          odb::Rect rect;
          box->getBox(rect);
          transform.apply(rect);

          odb::dbTechLayer* techLayer = box->getTechLayer();
          if (techLayer->getType().getValue()
              != odb::dbTechLayerType::ROUTING) {
            continue;
          }

          pinLayer = techLayer->getRoutingLevel();
          lowerBound = odb::Point(rect.xMin(), rect.yMin());
          upperBound = odb::Point(rect.xMax(), rect.yMax());
          pinBox = odb::Rect(lowerBound, upperBound);
          if (!dieArea.contains(pinBox)) {
            _logger->warn(GRT, 39, "Found pin outside die area in instance {}.", currInst->getConstName());
          }
          _grid->addObstruction(pinLayer, pinBox);
        }
      }
    }
  }

  _logger->info(GRT, 3, "Macros: {}", macrosCnt);
  return obstructionsCnt;
}

void GlobalRouter::findNetsObstructions(odb::Rect& dieArea)
{
  odb::dbSet<odb::dbNet> nets = _block->getNets();

  if (nets.empty()) {
    _logger->error(GRT, 94, "Design without nets.");
  }

  for (odb::dbNet* db_net : nets) {
    uint wireCnt = 0, viaCnt = 0;
    db_net->getWireCount(wireCnt, viaCnt);
    if (wireCnt < 1)
      continue;

    if (db_net->getSigType() == odb::dbSigType::POWER
        || db_net->getSigType() == odb::dbSigType::GROUND) {
      for (odb::dbSWire* swire : db_net->getSWires()) {
        for (odb::dbSBox* s : swire->getWires()) {
          if (s->isVia()) {
            continue;
          } else {
            odb::Rect wireRect;
            s->getBox(wireRect);
            int l = s->getTechLayer()->getRoutingLevel();

            odb::Point lowerBound
                = odb::Point(wireRect.xMin(), wireRect.yMin());
            odb::Point upperBound
                = odb::Point(wireRect.xMax(), wireRect.yMax());
            odb::Rect obstructionBox = odb::Rect(lowerBound, upperBound);
            if (!dieArea.contains(obstructionBox)) {
              _logger->warn(GRT, 40, "Net {} has wires outside die area.", db_net->getConstName());
            }
            _grid->addObstruction(l, obstructionBox);
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
            odb::Rect wireRect;
            pshape.shape.getBox(wireRect);
            int l = pshape.shape.getTechLayer()->getRoutingLevel();

            odb::Point lowerBound
                = odb::Point(wireRect.xMin(), wireRect.yMin());
            odb::Point upperBound
                = odb::Point(wireRect.xMax(), wireRect.yMax());
            odb::Rect obstructionBox = odb::Rect(lowerBound, upperBound);
            if (!dieArea.contains(obstructionBox)) {
              _logger->warn(GRT, 41, "Net {} has wires outside die area.", db_net->getConstName());
            }
            _grid->addObstruction(l, obstructionBox);
          }
        }
      }
    }
  }
}

int GlobalRouter::computeMaxRoutingLayer()
{
  _block = _db->getChip()->getBlock();

  int maxRoutingLayer = -1;

  odb::dbTech* tech = _db->getTech();

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    odb::dbTechLayer* techLayer = tech->findRoutingLayer(layer);
    if (techLayer == nullptr) {
      _logger->error(GRT, 95, "Layer {} not found.", selectedMetal);
    }
    odb::dbTrackGrid* selectedTrack = _block->findTrackGrid(techLayer);
    if (selectedTrack == nullptr) {
      break;
    }
    maxRoutingLayer = layer;
  }

  return maxRoutingLayer;
}

void GlobalRouter::getCutLayerRes(unsigned belowLayerId, float& r)
{
  odb::dbTech* tech = _db->getTech();
  odb::dbTechLayer* cut = tech->findRoutingLayer(belowLayerId)->getUpperLayer();
  r = cut->getResistance();  // assumes single cut
}

void GlobalRouter::getLayerRC(unsigned layerId, float& r, float& c)
{
  odb::dbTech* tech = _db->getTech();
  odb::dbTechLayer* techLayer = tech->findRoutingLayer(layerId);

  float layerWidth
      = (float) techLayer->getWidth() / _block->getDbUnitsPerMicron();
  float resOhmPerMicron = techLayer->getResistance() / layerWidth;
  float capPfPerMicron = layerWidth * techLayer->getCapacitance()
                         + 2 * techLayer->getEdgeCapacitance();

  r = 1E+6 * resOhmPerMicron;         // Meters
  c = 1E+6 * 1E-12 * capPfPerMicron;  // F/m
}

float GlobalRouter::dbuToMeters(unsigned dbu)
{
  return (float) dbu / (_block->getDbUnitsPerMicron() * 1E+6);
}

std::set<int> GlobalRouter::findTransitionLayers(int maxRoutingLayer)
{
  std::set<int> transitionLayers;
  odb::dbTech* tech = _db->getTech();
  odb::dbSet<odb::dbTechVia> vias = tech->getVias();

  if (vias.empty()) {
    _logger->error(GRT, 96, "Tech without vias.");
  }

  std::vector<odb::dbTechVia*> defaultVias;

  for (odb::dbTechVia* currVia : vias) {
    odb::dbStringProperty* prop
        = odb::dbStringProperty::find(currVia, "OR_DEFAULT");

    if (prop == nullptr) {
      continue;
    } else {
      _logger->info(GRT, 7, "Default via: {}.", currVia->getConstName());
      defaultVias.push_back(currVia);
    }
  }

  if (defaultVias.empty()) {
    _logger->warn(GRT, 42, "No OR_DEFAULT vias defined.");
    for (odb::dbTechVia* currVia : vias) {
      defaultVias.push_back(currVia);
    }
  }

  for (odb::dbTechVia* currVia : defaultVias) {
    int bottomSize = -1;
    int tmpLen;

    odb::dbTechLayer* bottomLayer;
    odb::dbSet<odb::dbBox> viaBoxes = currVia->getBoxes();
    odb::dbSet<odb::dbBox>::iterator boxIter;

    for (boxIter = viaBoxes.begin(); boxIter != viaBoxes.end(); boxIter++) {
      odb::dbBox* currBox = *boxIter;
      odb::dbTechLayer* layer = currBox->getTechLayer();

      if (layer->getDirection().getValue() == odb::dbTechLayerDir::HORIZONTAL) {
        tmpLen = currBox->yMax() - currBox->yMin();
      } else if (layer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        tmpLen = currBox->xMax() - currBox->xMin();
      } else {
        continue;
      }

      if (layer->getConstName() == currVia->getBottomLayer()->getConstName()) {
        bottomLayer = layer;
        if (tmpLen >= bottomSize) {
          bottomSize = tmpLen;
        }
      }
    }

    if (bottomLayer->getRoutingLevel() >= maxRoutingLayer
        || bottomLayer->getRoutingLevel() <= 4)
      continue;

    if (bottomSize > bottomLayer->getWidth()) {
      transitionLayers.insert(bottomLayer->getRoutingLevel());
    }
  }

  return transitionLayers;
}

std::map<int, odb::dbTechVia*> GlobalRouter::getDefaultVias(int maxRoutingLayer)
{
  odb::dbTech* tech = _db->getTech();
  odb::dbSet<odb::dbTechVia> vias = tech->getVias();
  std::map<int, odb::dbTechVia*> defaultVias;

  for (odb::dbTechVia* currVia : vias) {
    odb::dbStringProperty* prop
        = odb::dbStringProperty::find(currVia, "OR_DEFAULT");

    if (prop == nullptr) {
      continue;
    } else {
      _logger->info(GRT, 8, "Default via: {}.", currVia->getConstName());
      defaultVias[currVia->getBottomLayer()->getRoutingLevel()] = currVia;
    }
  }

  if (defaultVias.empty()) {
    _logger->warn(GRT, 43, "No OR_DEFAULT vias defined.");
    for (int i = 1; i <= maxRoutingLayer; i++) {
      for (odb::dbTechVia* currVia : vias) {
        if (currVia->getBottomLayer()->getRoutingLevel() == i) {
          defaultVias[i] = currVia;
          break;
        }
      }
    }
  }

  return defaultVias;
}

RegionAdjustment::RegionAdjustment(int minX,
                                   int minY,
                                   int maxX,
                                   int maxY,
                                   int l,
                                   float adjst)
{
  region = odb::Rect(minX, minY, maxX, maxY);
  layer = l;
  adjustment = adjst;
}

const char* getNetName(odb::dbNet* db_net)
{
  return db_net->getConstName();
}

// Useful for debugging.
void GlobalRouter::print(GRoute& route)
{
  for (GSegment& segment : route) {
    _logger->report("{:6d} {:6d} {:2d} -> {:6d} {:6d} {:2d}",
           segment.initX,
           segment.initY,
           segment.initLayer,
           segment.finalX,
           segment.finalY,
           segment.finalLayer);
  }
}

}  // namespace grt
