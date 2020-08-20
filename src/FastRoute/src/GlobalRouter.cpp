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

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Box.h"
#include "Coordinate.h"
#include "DBWrapper.h"
#include "FastRoute.h"
#include "Grid.h"
#include "RcTreeBuilder.h"
#include "RoutingLayer.h"
#include "RoutingTracks.h"
#include "db_sta/dbSta.hh"
#include "openroad/Error.hh"
#include "openroad/OpenRoad.hh"
#include "sta/Parasitics.hh"

namespace FastRoute {

using ord::error;

void GlobalRouter::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  init();
}

void GlobalRouter::init()
{
  makeComponents();
  // Initialize variables
  _outfile = "out.guide";
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
  _nets = new std::vector<Net>;
  _grid = new Grid;
  _dbWrapper = new DBWrapper(_openroad, this, _grid);
  _fastRoute = new FT;
  _gridOrigin = new Coordinate(-1, -1);
  _routingLayers = new std::vector<RoutingLayer>;
  _allRoutingTracks = new std::vector<RoutingTracks>;
  _result = new std::vector<FastRoute::NET>;
}

void GlobalRouter::deleteComponents()
{
  delete _grid;
  delete _dbWrapper;
  delete _fastRoute;
  delete _gridOrigin;
  delete _routingLayers;
  delete _allRoutingTracks;
  delete _nets;
}

void GlobalRouter::resetResources()
{
  deleteComponents();

  _vCapacities.clear();
  _hCapacities.clear();

  makeComponents();
}

void GlobalRouter::reset()
{
  deleteComponents();

  _vCapacities.clear();
  _hCapacities.clear();
  _layersToAdjust.clear();
  _layersReductionPercentage.clear();
  regionsMinX.clear();
  regionsMinY.clear();
  regionsMaxX.clear();
  regionsMaxY.clear();
  regionsLayer.clear();
  regionsReductionPercentage.clear();
  _netsAlpha.clear();

  init();
}

GlobalRouter::~GlobalRouter()
{
  deleteComponents();
}

void GlobalRouter::startFastRoute()
{
  std::chrono::steady_clock::time_point begin
      = std::chrono::steady_clock::now();
  printHeader();
  if (_unidirectionalRoute) {
    _minRoutingLayer = 2;
    _fixLayer = 1;
  } else {
    _fixLayer = 0;
  }

  if (_maxRoutingLayer == -1) {
    _maxRoutingLayer = _dbWrapper->computeMaxRoutingLayer();
  }

  if (_maxRoutingLayer < _selectedMetal) {
    _dbWrapper->setSelectedMetal(_maxRoutingLayer);
  }

  if (_pdRevForHighFanout != -1) {
    _fastRoute->setAlpha(_alpha);
  }

  _fastRoute->setVerbose(_verbose);
  _fastRoute->setOverflowIterations(_overflowIterations);
  _fastRoute->setPDRevForHighFanout(_pdRevForHighFanout);
  _fastRoute->setAllowOverflow(_allowOverflow);

  std::cout << "[PARAMS] Min routing layer: " << _minRoutingLayer << "\n";
  std::cout << "[PARAMS] Max routing layer: " << _maxRoutingLayer << "\n";
  std::cout << "[PARAMS] Global adjustment: " << _adjustment << "\n";
  std::cout << "[PARAMS] Unidirectional routing: " << _unidirectionalRoute
            << "\n";
  std::cout << "[PARAMS] Grid origin: (" << _gridOrigin->getX() << ", "
            << _gridOrigin->getY() << ")\n";
  if (!_layerPitches.empty()) {
    std::cout << "[PARAMS] Layers pitches: \n";
    std::map<int, float>::iterator it;
    for (it = _layerPitches.begin(); it != _layerPitches.end(); it++) {
      std::cout << "Layer " << it->first << " pitch: " << it->second << "\n";
    }
  }
  std::cout << "\n";

  std::cout << "Initializing grid...\n";
  initGrid();
  std::cout << "Initializing grid... Done!\n";

  std::cout << "Initializing routing layers...\n";
  initRoutingLayers();
  std::cout << "Initializing routing layers... Done!\n";

  std::cout << "Initializing routing tracks...\n";
  initRoutingTracks();
  std::cout << "Initializing routing tracks... Done!\n";

  std::cout << "Setting capacities...\n";
  setCapacities();
  std::cout << "Setting capacities... Done!\n";

  std::cout << "Setting spacings and widths...\n";
  setSpacingsAndMinWidths();
  std::cout << "Setting spacings and widths... Done!\n";

  std::cout << "Initializing nets...\n";
  initializeNets(_reroute);
  std::cout << "Initializing nets... Done!\n";

  std::cout << "Adjusting grid...\n";
  computeGridAdjustments();
  std::cout << "Adjusting grid... Done!\n";

  std::cout << "Computing track adjustments...\n";
  computeTrackAdjustments();
  std::cout << "Computing track adjustments... Done!\n";

  std::cout << "Computing obstacles adjustments...\n";
  computeObstaclesAdjustments();
  std::cout << "Computing obstacles adjustments... Done!\n";

  std::cout << "Computing user defined adjustments...\n";
  computeUserGlobalAdjustments();
  std::cout << "Computing user defined adjustments... Done!\n";

  std::cout << "Computing user defined layers adjustments...\n";
  computeUserLayerAdjustments();
  std::cout << "Computing user defined layers adjustments... Done!\n";

  for (uint i = 0; i < regionsReductionPercentage.size(); i++) {
    if (regionsLayer[i] < 1)
      break;

    std::cout << "Adjusting specific region in layer " << regionsLayer[i]
              << "...\n";
    Coordinate lowerLeft = Coordinate(regionsMinX[i], regionsMinY[i]);
    Coordinate upperRight = Coordinate(regionsMaxX[i], regionsMaxY[i]);
    computeRegionAdjustments(
        lowerLeft, upperRight, regionsLayer[i], regionsReductionPercentage[i]);
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  if (_verbose > 0)
    std::cout << "[INFO] Elapsed time: "
              << (std::chrono::duration_cast<std::chrono::microseconds>(end
                                                                        - begin)
                      .count())
                     / 1000000.0
              << "\n";
}

void GlobalRouter::runFastRoute()
{
  std::chrono::steady_clock::time_point begin
      = std::chrono::steady_clock::now();

  std::cout << "Running FastRoute...\n\n";
  _fastRoute->initAuxVar();
  if (_enableAntennaFlow) {
    runAntennaAvoidanceFlow();
  } else if (_clockNetsRouteFlow) {
    runClockNetsRouteFlow();
  } else {
    _fastRoute->run(*_result);
    addRemainingGuides(_result);
    connectPadPins(_result);
  }
  std::cout << "Running FastRoute... Done!\n";

  for (FastRoute::NET& netRoute : *_result) {
    mergeSegments(netRoute);
  }

  computeWirelength();

  if (_reportCongest) {
    _fastRoute->writeCongestionReport2D(_congestFile + "2D.log");
    _fastRoute->writeCongestionReport3D(_congestFile + "3D.log");
  }

  if (_verbose > 0) {
    std::chrono::steady_clock::time_point end
        = std::chrono::steady_clock::now();
    double elapsed
        = (std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
               .count())
          / 1000000.0;
    std::cout << "[INFO] Elapsed time: " << elapsed << "\n";
  }
}

void GlobalRouter::runAntennaAvoidanceFlow()
{
  std::cout << "Running antenna avoidance flow...\n";
  std::vector<FastRoute::NET> *globalRoute = new std::vector<FastRoute::NET>;
  std::vector<FastRoute::NET> *newRoute = new std::vector<FastRoute::NET>;
  std::vector<FastRoute::NET> *originalRoute = new std::vector<FastRoute::NET>;

  _fastRoute->run(*globalRoute);
  addRemainingGuides(globalRoute);
  connectPadPins(globalRoute);

  for (FastRoute::NET route : *globalRoute) {
    originalRoute->push_back(route);
  }

  getPreviousCapacities(_minRoutingLayer);
  addLocalConnections(globalRoute);

  int violationsCnt
      = _dbWrapper->checkAntennaViolations(globalRoute, _maxRoutingLayer);

  // Save dirtyNets and antennaViolations before reset resources
  std::vector<odb::dbNet*> dirtyNets = _dbWrapper->getDirtyNets();
  std::map<odb::dbNet*, std::vector<VINFO>> antennaViolations =
                                            _dbWrapper->getAntennaViolations();

  resetResources();

  // Adding routes of first run here to avoid loss data in resetResources
  for (FastRoute::NET gr : *originalRoute) {
    _result->push_back(gr);
  }

  if (violationsCnt > 0) {
    _dbWrapper->setDirtyNets(dirtyNets);
    _dbWrapper->setAntennaViolations(antennaViolations);
    _dbWrapper->fixAntennas(_diodeCellName, _diodePinName);
    _dbWrapper->legalizePlacedCells();
    _reroute = true;
    startFastRoute();
    _fastRoute->setVerbose(0);
    std::cout << "[INFO] #Nets to reroute: " << _fastRoute->getNets().size()
              << "\n";

    restorePreviousCapacities(_minRoutingLayer);

    _fastRoute->initAuxVar();
    _fastRoute->run(*newRoute);
    addRemainingGuides(newRoute);
    connectPadPins(newRoute);
    mergeResults(newRoute);
  }

  for (FastRoute::NET& netRoute : *_result) {
    mergeSegments(netRoute);
  }
}

void GlobalRouter::runClockNetsRouteFlow()
{
  std::vector<FastRoute::NET> *clockNetsRoute = new std::vector<FastRoute::NET>;
  _fastRoute->setVerbose(0);
  _fastRoute->run(*clockNetsRoute);
  addRemainingGuides(clockNetsRoute);
  connectPadPins(clockNetsRoute);

  getPreviousCapacities(_minLayerForClock);

  resetResources();
  _onlyClockNets = false;
  _onlySignalNets = true;

  startFastRoute();
  restorePreviousCapacities(_minLayerForClock);

  _fastRoute->initAuxVar();
  _fastRoute->run(*_result);
  addRemainingGuides(_result);
  connectPadPins(_result);

  _result->insert(
      _result->begin(), clockNetsRoute->begin(), clockNetsRoute->end());

  for (FastRoute::NET& netRoute : *_result) {
    mergeSegments(netRoute);
  }
}

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta::dbSta* dbSta = _openroad->getSta();
  dbSta->deleteParasitics();

  RcTreeBuilder builder(_openroad, _dbWrapper, _grid);
  for (FastRoute::NET& netRoute : *_result) {
    Net* net = getNetByIdx(netRoute.idx);
    std::vector<ROUTE>& route = netRoute.route;
    builder.estimateParasitcs(net, route);
  }
}

void GlobalRouter::enableAntennaAvoidance(char* diodeCellName,
                                             char* diodePinName)
{
  _enableAntennaFlow = true;
  std::string cellName(diodeCellName);
  std::string pinName(diodePinName);
  _diodeCellName = cellName;
  _diodePinName = pinName;
}

void GlobalRouter::initGrid()
{
  _dbWrapper->initGrid(_maxRoutingLayer);

  _dbWrapper->computeCapacities(_maxRoutingLayer, _layerPitches);
  _dbWrapper->computeSpacingsAndMinWidth(_maxRoutingLayer);
  _dbWrapper->initObstacles();

  _fastRoute->setLowerLeft(_grid->getLowerLeftX(), _grid->getLowerLeftY());
  _fastRoute->setTileSize(_grid->getTileWidth(), _grid->getTileHeight());
  _fastRoute->setGridsAndLayers(
      _grid->getXGrids(), _grid->getYGrids(), _grid->getNumLayers());
}

void GlobalRouter::initRoutingLayers()
{
  _dbWrapper->initRoutingLayers(*_routingLayers);

  RoutingLayer routingLayer = getRoutingLayerByIndex(1);
  _fastRoute->setLayerOrientation(routingLayer.getPreferredDirection());
}

void GlobalRouter::initRoutingTracks()
{
  _dbWrapper->initRoutingTracks(
      *_allRoutingTracks, _maxRoutingLayer, _layerPitches);
}

void GlobalRouter::setCapacities()
{
  for (int l = 1; l <= _grid->getNumLayers(); l++) {
    if (l < _minRoutingLayer || l > _maxRoutingLayer
        || (_onlyClockNets && l < _minLayerForClock)) {
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

void GlobalRouter::getPreviousCapacities(int previousMinLayer)
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
  for (int layer = previousMinLayer; layer <= _grid->getNumLayers(); layer++) {
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

  std::cout << "Old total capacity: " << oldTotalCap << "\n";
}

void GlobalRouter::restorePreviousCapacities(int previousMinLayer)
{
  int oldCap;
  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  int newTotalCap = 0;
  for (int layer = previousMinLayer; layer <= _grid->getNumLayers(); layer++) {
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

  std::cout << "New total capacity: " << newTotalCap << "\n";
}

void GlobalRouter::setSpacingsAndMinWidths()
{
  for (int l = 1; l <= _grid->getNumLayers(); l++) {
    _fastRoute->addMinSpacing(_grid->getSpacings()[l - 1], l);
    _fastRoute->addMinWidth(_grid->getMinWidths()[l - 1], l);
    _fastRoute->addViaSpacing(1, l);
  }
}

void GlobalRouter::initializeNets(bool reroute)
{
  _dbWrapper->initNetlist(reroute);

  std::cout << "Checking pin placement...\n";
  checkPinPlacement();
  std::cout << "Checking pin placement... Done!\n";

  int validNets = 0;

  int minDegree = std::numeric_limits<int>::max();
  int maxDegree = std::numeric_limits<int>::min();

  for (const Net& net : *_nets) {
    if (net.getNumPins() > 1
        && checkSignalType(net)
        && net.getNumPins() < std::numeric_limits<short>::max()) {
      validNets++;
    }
  }

  _fastRoute->setNumberNets(validNets);
  _fastRoute->setMaxNetDegree(getMaxNetDegree());

  int idx = 0;
  for (Net& net : *_nets) {
    int pin_count = net.getNumPins();
    if (pin_count > 1) {
      if (pin_count < minDegree) {
        minDegree = pin_count;
      }

      if (pin_count > maxDegree) {
        maxDegree = pin_count;
      }

      if (checkSignalType(net)) {
        if (pin_count >= std::numeric_limits<short>::max()) {
          std::cout << "[WARNING] FastRoute cannot handle net " << net.getName()
                    << " due to large number of pins\n";
          std::cout << "[WARNING] Net " << net.getName() << " has "
                    << net.getNumPins() << " pins\n";
        } else {
          std::vector<FastRoute::PIN> pins;
          for (Pin& pin : net.getPins()) {
            Coordinate pinPosition;
            int topLayer = pin.getTopLayer();
            RoutingLayer layer = getRoutingLayerByIndex(topLayer);

            std::vector<Box> pinBoxes = pin.getBoxes().at(topLayer);
            std::vector<Coordinate> pinPositionsOnGrid;
            Coordinate posOnGrid;
            Coordinate trackPos;

            for (Box pinBox : pinBoxes) {
              posOnGrid = _grid->getPositionOnGrid(pinBox.getMiddle());
              pinPositionsOnGrid.push_back(posOnGrid);
            }

            int votes = -1;

            for (Coordinate pos : pinPositionsOnGrid) {
              int equals = std::count(
                  pinPositionsOnGrid.begin(), pinPositionsOnGrid.end(), pos);
              if (equals > votes) {
                pinPosition = pos;
                votes = equals;
              }
            }

            if (pinOverlapsWithSingleTrack(pin, trackPos)) {
              posOnGrid = _grid->getPositionOnGrid(trackPos);

              if (!(posOnGrid == pinPosition)) {
                if ((layer.getPreferredDirection() == RoutingLayer::HORIZONTAL
                     && posOnGrid.getY() != pinPosition.getY())
                    || (layer.getPreferredDirection() == RoutingLayer::VERTICAL
                        && posOnGrid.getX() != pinPosition.getX())) {
                  pinPosition = posOnGrid;
                }
              }
            }

            pin.setOnGridPosition(pinPosition);

            // If pin is connected to PAD, create a "fake" location in routing
            // grid to avoid PAD obstacles
            if (pin.isConnectedToPad() || pin.isPort()) {
              FastRoute::ROUTE pinConnection
                  = createFakePin(pin, pinPosition, layer);
              _padPinsConnections[&net].push_back(pinConnection);
            }

            FastRoute::PIN grPin;
            grPin.x = pinPosition.getX();
            grPin.y = pinPosition.getY();
            grPin.layer = topLayer;
            pins.push_back(grPin);
          }

          FastRoute::PIN grPins[pins.size()];
          int count = 0;

          for (FastRoute::PIN pin : pins) {
            grPins[count] = pin;
            count++;
          }

          float netAlpha = _alpha;
          if (_netsAlpha.find(net.getName()) != _netsAlpha.end()) {
            netAlpha = _netsAlpha[net.getName()];
          }

          // name is copied by FR
          char* net_name = const_cast<char*>(net.getConstName());
          bool isClock
              = (net.getSignalType() == odb::dbSigType::CLOCK) ? true : false;
          _fastRoute->addNet(
              net_name, idx, pins.size(), 1, grPins, netAlpha, isClock);
        }
      }
    }
    idx++;
  }

  std::cout << "[INFO] Minimum degree: " << minDegree << "\n";
  std::cout << "[INFO] Maximum degree: " << maxDegree << "\n";

  _fastRoute->initEdges();
}

void GlobalRouter::computeGridAdjustments()
{
  Coordinate upperDieBounds
      = Coordinate(_grid->getUpperRightX(), _grid->getUpperRightY());
  DBU hSpace;
  DBU vSpace;

  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  Coordinate upperGridBounds = Coordinate(xGrids * _grid->getTileWidth(),
                                          yGrids * _grid->getTileHeight());
  DBU xExtra = upperDieBounds.getX() - upperGridBounds.getX();
  DBU yExtra = upperDieBounds.getY() - upperGridBounds.getY();

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    hSpace = 0;
    vSpace = 0;
    RoutingLayer routingLayer = getRoutingLayerByIndex(layer);

    if (layer < _minRoutingLayer
        || (layer > _maxRoutingLayer && _maxRoutingLayer > 0)
        || (_onlyClockNets && layer < _minLayerForClock))
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
      error("Layer spacing not found\n");
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
  Coordinate upperDieBounds
      = Coordinate(_grid->getUpperRightX(), _grid->getUpperRightY());
  for (RoutingLayer layer : *_routingLayers) {
    DBU trackLocation;
    int numInitAdjustments = 0;
    int numFinalAdjustments = 0;
    DBU trackSpace;
    int numTracks = 0;

    if (layer.getIndex() < _minRoutingLayer
        || (layer.getIndex() > _maxRoutingLayer && _maxRoutingLayer > 0)
        || (_onlyClockNets && layer.getIndex() < _minLayerForClock))
      continue;

    if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
      RoutingTracks routingTracks = getRoutingTracksByIndex(layer.getIndex());
      trackLocation = routingTracks.getLocation();
      trackSpace = std::max(routingTracks.getTrackPitch(),
                            routingTracks.getLine2ViaPitch());
      numTracks = routingTracks.getNumTracks();

      if (numTracks > 0) {
        DBU finalTrackLocation = trackLocation + (trackSpace * (numTracks - 1));
        DBU remainingFinalSpace = upperDieBounds.getY() - finalTrackLocation;
        DBU extraSpace = upperDieBounds.getY()
                         - (_grid->getTileHeight() * _grid->getYGrids());
        if (_grid->isPerfectRegularY()) {
          numFinalAdjustments
              = std::ceil((float) remainingFinalSpace / _grid->getTileHeight());
        } else {
          if (remainingFinalSpace != 0) {
            DBU finalSpace = remainingFinalSpace - extraSpace;
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
          DBU remainingTile = _grid->getTileHeight() - trackLocation;
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
          DBU remainingTile
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
        DBU finalTrackLocation = trackLocation + (trackSpace * (numTracks - 1));
        DBU remainingFinalSpace = upperDieBounds.getX() - finalTrackLocation;
        DBU extraSpace = upperDieBounds.getX()
                         - (_grid->getTileWidth() * _grid->getXGrids());
        if (_grid->isPerfectRegularX()) {
          numFinalAdjustments
              = std::ceil((float) remainingFinalSpace / _grid->getTileWidth());
        } else {
          if (remainingFinalSpace != 0) {
            DBU finalSpace = remainingFinalSpace - extraSpace;
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
          DBU remainingTile = _grid->getTileWidth() - trackLocation;
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
          DBU remainingTile
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

  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  int numAdjustments = _grid->getNumLayers() * yGrids * xGrids;

  numAdjustments *= 2;
  _fastRoute->setNumAdjustments(numAdjustments);

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    if (_hCapacities[layer - 1] != 0) {
      int newCap = _grid->getHorizontalEdgesCapacities()[layer - 1]
                   * (1 - _adjustment);
      _grid->updateHorizontalEdgesCapacities(layer - 1, newCap);

      for (int y = 1; y < yGrids; y++) {
        for (int x = 1; x < xGrids; x++) {
          int edgeCap = _fastRoute->getEdgeCapacity(
              x - 1, y - 1, layer, x, y - 1, layer);
          int newHCapacity = std::floor((float) edgeCap * (1 - _adjustment));
          _fastRoute->addAdjustment(
              x - 1, y - 1, layer, x, y - 1, layer, newHCapacity);
        }
      }
    }

    if (_vCapacities[layer - 1] != 0) {
      int newCap
          = _grid->getVerticalEdgesCapacities()[layer - 1] * (1 - _adjustment);
      _grid->updateVerticalEdgesCapacities(layer - 1, newCap);

      for (int x = 1; x < xGrids; x++) {
        for (int y = 1; y < yGrids; y++) {
          int edgeCap = _fastRoute->getEdgeCapacity(
              x - 1, y - 1, layer, x - 1, y, layer);
          int newVCapacity = std::floor((float) edgeCap * (1 - _adjustment));
          _fastRoute->addAdjustment(
              x - 1, y - 1, layer, x - 1, y, layer, newVCapacity);
        }
      }
    }
  }
}

void GlobalRouter::computeUserLayerAdjustments()
{
  int xGrids = _grid->getXGrids();
  int yGrids = _grid->getYGrids();

  int numAdjustments = _layersToAdjust.size() * yGrids * xGrids;

  numAdjustments *= 2;
  _fastRoute->setNumAdjustments(numAdjustments);

  for (uint idx = 0; idx < _layersToAdjust.size(); idx++) {
    int layer = _layersToAdjust[idx];
    float adjustment = _layersReductionPercentage[idx];
    std::cout << "[INFO] Reducing resources of layer " << layer << " by "
              << adjustment * 100 << "%\n";
    if (_hCapacities[layer - 1] != 0) {
      int newCap
          = _grid->getHorizontalEdgesCapacities()[layer - 1] * (1 - adjustment);
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

void GlobalRouter::computeRegionAdjustments(const Coordinate& lowerBound,
                                               const Coordinate& upperBound,
                                               int layer,
                                               float reductionPercentage)
{
  Box firstTileBox;
  Box lastTileBox;
  std::pair<Grid::TILE, Grid::TILE> tilesToAdjust;

  Box dieBox = Box(_grid->getLowerLeftX(),
                   _grid->getLowerLeftY(),
                   _grid->getUpperRightX(),
                   _grid->getUpperRightY(),
                   -1);

  if ((dieBox.getLowerBound().getX() > lowerBound.getX()
       && dieBox.getLowerBound().getY() > lowerBound.getY())
      || (dieBox.getUpperBound().getX() < upperBound.getX()
          && dieBox.getUpperBound().getY() < upperBound.getY())) {
    error("Informed region is outside die area\n");
  }

  RoutingLayer routingLayer = getRoutingLayerByIndex(layer);
  bool direction = routingLayer.getPreferredDirection();
  Box regionToAdjust = Box(lowerBound, upperBound, -1);

  tilesToAdjust
      = _grid->getBlockedTiles(regionToAdjust, firstTileBox, lastTileBox);
  Grid::TILE& firstTile = tilesToAdjust.first;
  Grid::TILE& lastTile = tilesToAdjust.second;

  RoutingTracks routingTracks = getRoutingTracksByIndex(layer);
  DBU trackSpace = std::max(routingTracks.getTrackPitch(),
                            routingTracks.getLine2ViaPitch());

  int firstTileReduce = _grid->computeTileReduce(
      regionToAdjust, firstTileBox, trackSpace, true, direction);

  int lastTileReduce = _grid->computeTileReduce(
      regionToAdjust, lastTileBox, trackSpace, false, direction);

  // If preferred direction is horizontal, only first and the last line will
  // have specific adjustments
  if (direction == RoutingLayer::HORIZONTAL) {
    // Setting capacities of edges completely inside the adjust region according
    // the percentage of reduction
    for (int x = firstTile._x; x <= lastTile._x; x++) {
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
      // Setting capacities of edges completely inside the adjust region
      // according the percentage of reduction
      for (int y = firstTile._y; y <= lastTile._y; y++) {
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

void GlobalRouter::computeObstaclesAdjustments()
{
  std::map<int, std::vector<Box>> obstacles = _grid->getAllObstacles();

  for (int layer = 1; layer <= _grid->getNumLayers(); layer++) {
    std::vector<Box> layerObstacles = obstacles[layer];
    if (!layerObstacles.empty()) {
      RoutingLayer routingLayer = getRoutingLayerByIndex(layer);

      std::pair<Grid::TILE, Grid::TILE> blockedTiles;

      bool direction = routingLayer.getPreferredDirection();

      std::cout << "[INFO] Processing " << layerObstacles.size()
                << " obstacles in layer " << layer << "\n";

      int trackSpace = _grid->getMinWidths()[layer - 1];

      for (Box& obs : layerObstacles) {
        Box firstTileBox;
        Box lastTileBox;

        blockedTiles = _grid->getBlockedTiles(obs, firstTileBox, lastTileBox);

        Grid::TILE& firstTile = blockedTiles.first;
        Grid::TILE& lastTile = blockedTiles.second;

        int firstTileReduce = _grid->computeTileReduce(
            obs, firstTileBox, trackSpace, true, direction);

        int lastTileReduce = _grid->computeTileReduce(
            obs, lastTileBox, trackSpace, false, direction);

        if (direction == RoutingLayer::HORIZONTAL) {
          for (int x = firstTile._x - 1; x < lastTile._x; x++) {
            // Setting capacities of completely blocked edges to zero
            for (int y = firstTile._y - 1; y < lastTile._y; y++) {
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
          for (int x = firstTile._x - 1; x < lastTile._x; x++) {
            // Setting capacities of completely blocked edges to zero
            for (int y = firstTile._y - 1; y < lastTile._y; y++) {
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

void GlobalRouter::setOutputFile(const std::string& outfile)
{
  _outfile = outfile;
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
  if (layer > _maxRoutingLayer && _maxRoutingLayer > 0) {
    std::cout << "[ERROR] Specified layer " << layer
              << " for adjustment is greater than max routing layer "
              << _maxRoutingLayer << " and will be ignored" << std::endl;
  } else {
    _layersToAdjust.push_back(layer);
    _layersReductionPercentage.push_back(reductionPercentage);
  }
}

void GlobalRouter::addRegionAdjustment(int minX,
                                          int minY,
                                          int maxX,
                                          int maxY,
                                          int layer,
                                          float reductionPercentage)
{
  regionsMinX.push_back(minX);
  regionsMinY.push_back(minY);
  regionsMaxX.push_back(maxX);
  regionsMaxY.push_back(maxY);
  regionsLayer.push_back(layer);
  regionsReductionPercentage.push_back(reductionPercentage);
}

void GlobalRouter::setLayerPitch(int layer, float pitch)
{
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
  *_gridOrigin = Coordinate(x, y);
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
  std::string cgtFile(congestFile);
  _congestFile = cgtFile;
}

void GlobalRouter::setClockNetsRouteFlow(bool clockFlow)
{
  _clockNetsRouteFlow = clockFlow;
  _onlyClockNets = clockFlow;
  _onlySignalNets = false;
}

void GlobalRouter::setMinLayerForClock(int minLayer)
{
  _minLayerForClock = minLayer;
}

void GlobalRouter::writeGuides()
{
  std::cout << "Writing guides...\n";
  std::ofstream guideFile;
  guideFile.open(_outfile);
  if (!guideFile.is_open()) {
    guideFile.close();
    error("Guides file could not be open\n");
  }
  RoutingLayer phLayerF;

  int offsetX = _gridOrigin->getX();
  int offsetY = _gridOrigin->getY();

  std::cout << "[INFO] Num routed nets: " << _result->size() << "\n";
  int finalLayer;

  for (FastRoute::NET& netRoute : *_result) {
    if (!netRoute.route.empty()) {
      guideFile << netRoute.name << "\n";
      guideFile << "(\n";
      std::vector<Box> guideBox;
      finalLayer = -1;
      for (FastRoute::ROUTE route : netRoute.route) {
	if (route.initLayer != finalLayer && finalLayer != -1) {
	  mergeBox(guideBox);
	  for (Box guide : guideBox) {
	    guideFile << guide.getLowerBound().getX() + offsetX << " "
		      << guide.getLowerBound().getY() + offsetY << " "
		      << guide.getUpperBound().getX() + offsetX << " "
		      << guide.getUpperBound().getY() + offsetY << " "
		      << phLayerF.getName() << "\n";
	  }
	  guideBox.clear();
	  finalLayer = route.initLayer;
	}
	if (route.initLayer == route.finalLayer) {
	  if (route.initLayer < _minRoutingLayer && route.initX != route.finalX
	      && route.initY != route.finalY) {
	    error("Routing with guides in blocked metal for net %s\n",
		  netRoute.name.c_str());
	  }
	  Box box;
	  box = globalRoutingToBox(route);
	  guideBox.push_back(box);
	  if (route.finalLayer < _minRoutingLayer && !_unidirectionalRoute) {
	    phLayerF = getRoutingLayerByIndex(
					      (route.finalLayer + (_minRoutingLayer - route.finalLayer)));
	  } else {
	    phLayerF = getRoutingLayerByIndex(route.finalLayer);
	  }
	  finalLayer = route.finalLayer;
	} else {
	  if (abs(route.finalLayer - route.initLayer) > 1) {
	    error("Connection between non-adjacent layers in net %s\n",
		  netRoute.name.c_str());
	  } else {
	    RoutingLayer phLayerI;
	    if (route.initLayer < _minRoutingLayer && !_unidirectionalRoute) {
	      phLayerI = getRoutingLayerByIndex(route.initLayer + _minRoutingLayer
						- route.initLayer);
	    } else {
	      phLayerI = getRoutingLayerByIndex(route.initLayer);
	    }
	    if (route.finalLayer < _minRoutingLayer && !_unidirectionalRoute) {
	      phLayerF = getRoutingLayerByIndex(
						route.finalLayer + _minRoutingLayer - route.finalLayer);
	    } else {
	      phLayerF = getRoutingLayerByIndex(route.finalLayer);
	    }
	    finalLayer = route.finalLayer;
	    Box box;
	    box = globalRoutingToBox(route);
	    guideBox.push_back(box);
	    mergeBox(guideBox);
	    for (Box guide : guideBox) {
	      guideFile << guide.getLowerBound().getX() + offsetX << " "
			<< guide.getLowerBound().getY() + offsetY << " "
			<< guide.getUpperBound().getX() + offsetX << " "
			<< guide.getUpperBound().getY() + offsetY << " "
			<< phLayerI.getName() << "\n";
	    }
	    guideBox.clear();

	    box = globalRoutingToBox(route);
	    guideBox.push_back(box);
	  }
	}
      }
      mergeBox(guideBox);
      for (Box guide : guideBox) {
	guideFile << guide.getLowerBound().getX() + offsetX << " "
		  << guide.getLowerBound().getY() + offsetY << " "
		  << guide.getUpperBound().getX() + offsetX << " "
		  << guide.getUpperBound().getY() + offsetY << " "
		  << phLayerF.getName() << "\n";
      }
      guideFile << ")\n";
    }
  }

  guideFile.close();
  std::cout << "Writing guides... Done!\n";
}

void GlobalRouter::printGrid()
{
  std::cout << "**** Global Routing Grid ****\n";
  std::cout << "******** Lower left: (" << _grid->getLowerLeftX() << ", "
            << _grid->getLowerLeftY() << ") ********\n";
  std::cout << "******** Tile size: " << _grid->getTileWidth() << " ********\n";
  std::cout << "******** xGrids, yGrids: " << _grid->getXGrids() << ", "
            << _grid->getYGrids() << " ********\n";
  std::cout << "******** Perfect regular X/Y: " << _grid->isPerfectRegularX()
            << "/" << _grid->isPerfectRegularY() << " ********\n";
  std::cout << "******** Num layers: " << _grid->getNumLayers()
            << " ********\n";
  std::cout << "******** Num nets: " << getNetCount()
            << " ********\n";
  std::cout << "******** Tile size: " << _grid->getPitchesInTile() << "\n";
}

void GlobalRouter::printHeader()
{
  std::cout << "\n";
  std::cout << " *****************\n";
  std::cout << " *   FastRoute   *\n";
  std::cout << " *****************\n";
  std::cout << "\n";
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

void GlobalRouter::addRemainingGuides(
    std::vector<FastRoute::NET>* globalRoute)
{
  auto net_pins = _fastRoute->getNets();
  std::unordered_set<Net*> routed_nets;

  for (FastRoute::NET& netRoute : *globalRoute) {
    Net* net = getNetByIdx(netRoute.idx);
    routed_nets.insert(net);
    // Skip nets with 1 pin or less
    if (net->getNumPins() > 1) {
      std::vector<FastRoute::PIN>& pins = net_pins[netRoute.idx];
      // Try to add local guides for net with no output of FR core
      if (netRoute.route.empty()) {
        int lastLayer = -1;
        for (uint p = 0; p < pins.size(); p++) {
          if (p > 0) {
            // If the net is not local, FR core result is invalid
            if (pins[p].x != pins[p - 1].x || pins[p].y != pins[p - 1].y) {
              error("Net %s not properly covered\n", netRoute.name.c_str());
            }
          }

          if (pins[p].layer > lastLayer)
            lastLayer = pins[p].layer;
        }

        for (int l = _minRoutingLayer - _fixLayer; l <= lastLayer; l++) {
          FastRoute::ROUTE route;
          route.initLayer = l;
          route.initX = pins[0].x;
          route.initY = pins[0].y;
          route.finalLayer = l + 1;
          route.finalX = pins[0].x;
          route.finalY = pins[0].y;
          netRoute.route.push_back(route);
        }
      } else {  // For nets with routing, add guides for pin acess at upper
                // layers
        for (FastRoute::PIN pin : pins) {
          if (pin.layer > 1) {
            // for each pin placed at upper layers, get all segments that
            // potentially covers it
            std::vector<FastRoute::ROUTE>& segments = netRoute.route;
            std::vector<FastRoute::ROUTE> coverSegs;

            int wireViaLayer = std::numeric_limits<int>::max();
            for (uint i = 0; i < segments.size(); i++) {
              if ((pin.x == segments[i].initX && pin.y == segments[i].initY)
                  || (pin.x == segments[i].finalX
                      && pin.y == segments[i].finalY)) {
                if (!(segments[i].initX == segments[i].finalX
                      && segments[i].initY == segments[i].finalY)) {
                  coverSegs.push_back(segments[i]);
                  if (segments[i].initLayer < wireViaLayer) {
                    wireViaLayer = segments[i].initLayer;
                  }
                }
              }
            }

            bool bottomLayerPin = false;
            for (FastRoute::PIN pin2 : pins) {
              if (pin.x == pin2.x && pin.y == pin2.y
                  && pin.layer > pin2.layer) {
                bottomLayerPin = true;
              }
            }

            if (!bottomLayerPin) {
              for (uint i = 0; i < segments.size(); i++) {
                if ((pin.x == segments[i].initX && pin.y == segments[i].initY)
                    || (pin.x == segments[i].finalX
                        && pin.y == segments[i].finalY)) {
                  // remove all vias to this pin that doesn't connects two wires
                  if (segments[i].initX == segments[i].finalX
                      && segments[i].initY == segments[i].finalY
                      && (segments[i].initLayer < wireViaLayer
                          || segments[i].finalLayer < wireViaLayer)) {
                    segments.erase(segments.begin() + i);
                    i = 0;
                  }
                }
              }
            }

            int closestLayer = -1;
            int minorDiff = std::numeric_limits<int>::max();

            for (FastRoute::ROUTE seg : coverSegs) {
              if (seg.initLayer != seg.finalLayer) {
                error("Segment has invalid layer assignment\n");
              }

              int diffLayers = std::abs(pin.layer - seg.initLayer);
              if (diffLayers < minorDiff && seg.initLayer > closestLayer) {
                minorDiff = seg.initLayer;
                closestLayer = seg.initLayer;
              }
            }

            if (closestLayer > pin.layer) {
              for (int l = closestLayer; l > pin.layer; l--) {
                FastRoute::ROUTE route;
                route.initLayer = l;
                route.initX = pin.x;
                route.initY = pin.y;
                route.finalLayer = l - 1;
                route.finalX = pin.x;
                route.finalY = pin.y;
                netRoute.route.push_back(route);
              }
            } else if (closestLayer < pin.layer) {
              for (int l = closestLayer; l < pin.layer; l++) {
                FastRoute::ROUTE route;
                route.initLayer = l;
                route.initX = pin.x;
                route.initY = pin.y;
                route.finalLayer = l + 1;
                route.finalX = pin.x;
                route.finalY = pin.y;
                netRoute.route.push_back(route);
              }
            }
          }
        }
      }
    }
  }

  // Add local guides for nets with no routing.
  for (Net& net : *_nets) {
    if (checkSignalType(net)
        && net.getNumPins() > 1 && routed_nets.find(&net) == routed_nets.end()) {
      int net_idx = getNetIdx(&net);
      std::vector<FastRoute::PIN>& pins = net_pins[net_idx];

      FastRoute::NET localNet;
      localNet.idx = net_idx;
      localNet.name = net.getConstName();
      for (FastRoute::PIN pin : pins) {
        FastRoute::ROUTE route;
        route.initLayer = pin.layer;
        route.initX = pin.x;
        route.initY = pin.y;
        route.finalLayer = pin.layer;
        route.finalX = pin.x;
        route.finalY = pin.y;
        localNet.route.push_back(route);
      }
      globalRoute->push_back(localNet);
    }
  }
}

void GlobalRouter::connectPadPins(std::vector<FastRoute::NET>* globalRoute)
{
  for (FastRoute::NET& netRoute : *globalRoute) {
    Net* net = getNetByIdx(netRoute.idx);
    if (_padPinsConnections.find(net) != _padPinsConnections.end()
        || net->getNumPins() > 1) {
      for (FastRoute::ROUTE route : _padPinsConnections[net]) {
        netRoute.route.push_back(route);
      }
    }
  }
}

void GlobalRouter::mergeBox(std::vector<Box>& guideBox)
{
  std::vector<Box> finalBox;
  if (guideBox.size() < 1) {
    error("Guides vector is empty\n");
  }
  finalBox.push_back(guideBox[0]);
  for (uint i = 1; i < guideBox.size(); i++) {
    Box box = guideBox[i];
    Box& lastBox = finalBox.back();
    if (lastBox.overlap(box)) {
      DBU lowerX = std::min(lastBox.getLowerBound().getX(),
                            box.getLowerBound().getX());
      DBU lowerY = std::min(lastBox.getLowerBound().getY(),
                            box.getLowerBound().getY());
      DBU upperX = std::max(lastBox.getUpperBound().getX(),
                            box.getUpperBound().getX());
      DBU upperY = std::max(lastBox.getUpperBound().getY(),
                            box.getUpperBound().getY());
      lastBox = Box(lowerX, lowerY, upperX, upperY, -1);
    } else
      finalBox.push_back(box);
  }
  guideBox.clear();
  guideBox = finalBox;
}

Box GlobalRouter::globalRoutingToBox(const FastRoute::ROUTE& route)
{
  Box dieBounds = Box(_grid->getLowerLeftX(),
                      _grid->getLowerLeftY(),
                      _grid->getUpperRightX(),
                      _grid->getUpperRightY(),
                      -1);
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

  DBU llX = initX - (_grid->getTileWidth() / 2);
  DBU llY = initY - (_grid->getTileHeight() / 2);

  DBU urX = finalX + (_grid->getTileWidth() / 2);
  DBU urY = finalY + (_grid->getTileHeight() / 2);

  if ((dieBounds.getUpperBound().getX() - urX) / _grid->getTileWidth() < 1) {
    urX = dieBounds.getUpperBound().getX();
  }
  if ((dieBounds.getUpperBound().getY() - urY) / _grid->getTileHeight() < 1) {
    urY = dieBounds.getUpperBound().getY();
  }

  Coordinate lowerLeft = Coordinate(llX, llY);
  Coordinate upperRight = Coordinate(urX, urY);

  Box routeBds = Box(lowerLeft, upperRight, -1);
  return routeBds;
}

void GlobalRouter::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<Coordinate>> mapLayerToPositions;

  for (Pin port : getAllPorts()) {
    if (port.getNumLayers() == 0) {
      error("Pin %s does not have layer assignment\n", port.getName().c_str());
    }
    DBU layer = port.getLayers()[0];  // port have only one layer

    if (mapLayerToPositions[layer].empty()) {
      mapLayerToPositions[layer].push_back(port.getPosition());
    } else {
      for (Coordinate pos : mapLayerToPositions[layer]) {
        if (pos == port.getPosition()) {
          std::cout << "[WARNING] At least 2 pins in position (" << pos.getX()
                    << ", " << pos.getY() << "), layer " << layer + 1 << "\n";
          invalid = true;
        }
      }
      mapLayerToPositions[layer].push_back(port.getPosition());
    }
  }

  if (invalid) {
    error("Invalid pin placement\n");
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
    std::cout << "V cap " << cnt << ": " << vCap << "\n";
    route.verticalEdgesCapacities.push_back(vCap);
    cnt++;
  }

  cnt = 0;
  for (int hCap : _grid->getHorizontalEdgesCapacities()) {
    std::cout << "H cap " << cnt << ": " << hCap << "\n";
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

std::vector<GlobalRouter::EST_> GlobalRouter::getEst()
{
  std::vector<EST_> netsEst;

  for (FastRoute::NET netRoute : *_result) {
    EST_ netEst;
    int validTiles = 0;
    for (FastRoute::ROUTE route : netRoute.route) {
      if (route.initX != route.finalX || route.initY != route.finalY
          || route.initLayer != route.finalLayer) {
        validTiles++;
      }
    }

    if (validTiles == 0) {
      netEst.netName = netRoute.name;
      netEst.netId = netRoute.idx;
      netEst.numSegments = validTiles;
    } else {
      netEst.netName = netRoute.name;
      netEst.netId = netRoute.idx;
      netEst.numSegments = netRoute.route.size();
      for (FastRoute::ROUTE route : netRoute.route) {
        netEst.initX.push_back(route.initX);
        netEst.initY.push_back(route.initY);
        netEst.initLayer.push_back(route.initLayer);
        netEst.finalX.push_back(route.finalX);
        netEst.finalY.push_back(route.finalY);
        netEst.finalLayer.push_back(route.finalLayer);
      }

      netsEst.push_back(netEst);
    }
  }

  return netsEst;
}

void GlobalRouter::computeWirelength()
{
  DBU totalWirelength = 0;
  for (FastRoute::NET& netRoute : *_result) {
    for (ROUTE route : netRoute.route) {
      DBU routeWl = std::abs(route.finalX - route.initX)
                    + std::abs(route.finalY - route.initY);
      totalWirelength += routeWl;

      if (routeWl > 0) {
        totalWirelength += (_grid->getTileWidth() + _grid->getTileHeight()) / 2;
      }
    }
  }
  std::cout << std::fixed << "[INFO] Total wirelength: "
            << (float) totalWirelength / _grid->getDatabaseUnit() << " um\n";
}

void GlobalRouter::mergeSegments(FastRoute::NET& net)
{
  if (!net.route.empty()) {
    // vector copy - bad bad -cherry
    std::vector<ROUTE> segments = net.route;
    std::map<Point, int> segsAtPoint;
    for (const ROUTE& seg : segments) {
      segsAtPoint[{seg.initX, seg.initY, seg.initLayer}] += 1;
      segsAtPoint[{seg.finalX, seg.finalY, seg.finalLayer}] += 1;
    }

    uint i = 0;
    while (i < segments.size() - 1) {
      ROUTE segment0 = segments[i];
      ROUTE segment1 = segments[i + 1];

      // both segments are not vias
      if (segment0.initLayer == segment0.finalLayer
	  && segment1.initLayer == segment1.finalLayer &&
	  // segments are on the same layer
	  segment0.initLayer == segment1.initLayer) {
	// if segment 0 connects to the end of segment 1
	ROUTE newSeg = segments[i];
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

    net.route = segments;
  }
}

bool GlobalRouter::segmentsConnect(const ROUTE& seg0,
                                      const ROUTE& seg1,
                                      ROUTE& newSeg,
                                      const std::map<Point, int>& segsAtPoint)
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
      merge = segsAtPoint.at({initX0, initY0, seg0.initLayer}) == 2;
    } else if (finalY0 == initY1) {
      merge = segsAtPoint.at({initX1, initY1, seg1.initLayer}) == 2;
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
      merge = segsAtPoint.at({initX0, initY0, seg0.initLayer}) == 2;
    } else if (finalX0 == initX1) {
      merge = segsAtPoint.at({initX1, initY1, seg1.initLayer}) == 2;
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

void GlobalRouter::addLocalConnections(
    std::vector<FastRoute::NET>* globalRoute)
{
  int topLayer;
  std::vector<Box> pinBoxes;
  Coordinate pinPosition;
  Coordinate realPinPosition;
  FastRoute::ROUTE horSegment;
  FastRoute::ROUTE verSegment;

  for (FastRoute::NET& netRoute : *globalRoute) {
      Net* net = getNetByIdx(netRoute.idx);

    for (Pin pin : net->getPins()) {
      topLayer = pin.getTopLayer();
      pinBoxes = pin.getBoxes().at(topLayer);
      pinPosition = pin.getOnGridPosition();

      realPinPosition = pinBoxes[0].getMiddle();
      horSegment.initX = realPinPosition.getX();
      horSegment.initY = realPinPosition.getY();
      horSegment.initLayer = topLayer;
      horSegment.finalX = pinPosition.getX();
      horSegment.finalY = realPinPosition.getY();
      horSegment.finalLayer = topLayer;

      verSegment.initX = pinPosition.getX();
      verSegment.initY = realPinPosition.getY();
      verSegment.initLayer = topLayer;
      verSegment.finalX = pinPosition.getX();
      verSegment.finalY = pinPosition.getY();
      verSegment.finalLayer = topLayer;

      netRoute.route.push_back(horSegment);
      netRoute.route.push_back(verSegment);
    }
  }
}

void GlobalRouter::mergeResults(const std::vector<FastRoute::NET>* newRoute)
{
  for (FastRoute::NET netRoute : *newRoute) {
    for (int i = 0; i < _result->size(); i++) {
      if (netRoute.name == _result->at(i).name) {
        _result->at(i) = netRoute;
        break;
      }
    }
  }
}

bool GlobalRouter::pinOverlapsWithSingleTrack(const Pin& pin,
                                                 Coordinate& trackPosition)
{
  DBU minX = std::numeric_limits<DBU>::max();
  DBU minY = std::numeric_limits<DBU>::max();
  DBU maxX = std::numeric_limits<DBU>::min();
  DBU maxY = std::numeric_limits<DBU>::min();

  DBU min, max;

  int topLayer = pin.getTopLayer();
  std::vector<Box> pinBoxes = pin.getBoxes().at(topLayer);

  RoutingLayer layer = getRoutingLayerByIndex(topLayer);
  RoutingTracks tracks = getRoutingTracksByIndex(topLayer);

  for (Box pinBox : pinBoxes) {
    if (pinBox.getLowerBound().getX() <= minX)
      minX = pinBox.getLowerBound().getX();

    if (pinBox.getLowerBound().getY() <= minY)
      minY = pinBox.getLowerBound().getY();

    if (pinBox.getUpperBound().getX() >= maxX)
      maxX = pinBox.getUpperBound().getX();

    if (pinBox.getUpperBound().getY() >= maxY)
      maxY = pinBox.getUpperBound().getY();
  }

  Coordinate middle
      = Coordinate((minX + (maxX - minX) / 2.0), (minY + (maxY - minY) / 2.0));
  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    min = minY;
    max = maxY;

    if ((float) (max - min) / tracks.getTrackPitch() <= 3) {
      DBU nearestTrack = std::floor((float) (max - tracks.getLocation())
                                    / tracks.getTrackPitch())
                             * tracks.getTrackPitch()
                         + tracks.getLocation();
      DBU nearestTrack2 = std::floor((float) (max - tracks.getLocation())
                                         / tracks.getTrackPitch()
                                     - 1)
                              * tracks.getTrackPitch()
                          + tracks.getLocation();

      if ((nearestTrack >= min && nearestTrack <= max)
          && (nearestTrack2 >= min && nearestTrack2 <= max)) {
        return false;
      }

      if (nearestTrack >= min && nearestTrack <= max) {
        trackPosition = Coordinate(middle.getX(), nearestTrack);
        return true;
      } else if (nearestTrack2 >= min && nearestTrack2 <= max) {
        trackPosition = Coordinate(middle.getX(), nearestTrack2);
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
      DBU nearestTrack = std::floor((float) (max - tracks.getLocation())
                                    / tracks.getTrackPitch())
                             * tracks.getTrackPitch()
                         + tracks.getLocation();
      DBU nearestTrack2 = std::floor((float) (max - tracks.getLocation())
                                         / tracks.getTrackPitch()
                                     - 1)
                              * tracks.getTrackPitch()
                          + tracks.getLocation();

      if ((nearestTrack >= min && nearestTrack <= max)
          && (nearestTrack2 >= min && nearestTrack2 <= max)) {
        return false;
      }

      if (nearestTrack >= min && nearestTrack <= max) {
        trackPosition = Coordinate(nearestTrack, middle.getY());
        return true;
      } else if (nearestTrack2 >= min && nearestTrack2 <= max) {
        trackPosition = Coordinate(nearestTrack2, middle.getY());
        return true;
      } else {
        return false;
      }
    }
  }

  return false;
}

FastRoute::ROUTE GlobalRouter::createFakePin(Pin pin,
                                                Coordinate& pinPosition,
                                                RoutingLayer layer)
{
  int topLayer = layer.getIndex();
  FastRoute::ROUTE pinConnection;
  pinConnection.initLayer = topLayer;
  pinConnection.finalLayer = topLayer;

  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    pinConnection.finalX = pinPosition.getX();
    pinConnection.initY = pinPosition.getY();
    pinConnection.finalY = pinPosition.getY();

    DBU newXPosition;
    if (pin.getOrientation() == Orientation::ORIENT_WEST) {
      newXPosition
          = pinPosition.getX() + (_gcellsOffset * _grid->getTileWidth());
      pinConnection.initX = newXPosition;
      pinPosition.setX(newXPosition);
    } else if (pin.getOrientation() == Orientation::ORIENT_EAST) {
      newXPosition
          = pinPosition.getX() - (_gcellsOffset * _grid->getTileWidth());
      pinConnection.initX = newXPosition;
      pinPosition.setX(newXPosition);
    } else {
      std::cout << "[WARNING] Pin " << pin.getName()
                << " has invalid orientation\n";
    }
  } else {
    pinConnection.initX = pinPosition.getX();
    pinConnection.finalX = pinPosition.getX();
    pinConnection.finalY = pinPosition.getY();

    DBU newYPosition;
    if (pin.getOrientation() == Orientation::ORIENT_SOUTH) {
      newYPosition
          = pinPosition.getY() + (_gcellsOffset * _grid->getTileHeight());
      pinConnection.initY = newYPosition;
      pinPosition.setY(newYPosition);
    } else if (pin.getOrientation() == Orientation::ORIENT_NORTH) {
      newYPosition
          = pinPosition.getY() - (_gcellsOffset * _grid->getTileHeight());
      pinConnection.initY = newYPosition;
      pinPosition.setY(newYPosition);
    } else {
      std::cout << "[WARNING] Pin " << pin.getName()
                << " has invalid orientation\n";
    }
  }

  return pinConnection;
}

bool GlobalRouter::checkSignalType(const Net &net) {
  bool isClock = net.getSignalType() == odb::dbSigType::CLOCK;
  return ((!_onlyClockNets && !_onlySignalNets) ||
          (_onlyClockNets && isClock) ||
          (_onlySignalNets && !isClock));
}

int GlobalRouter::getNetCount() const {
  return _nets->size();
}

Net* GlobalRouter::addNet(odb::dbNet* db_net) {
  _nets->push_back(Net(db_net));
  Net* net = &_nets->back();
  return net;
}
  
void GlobalRouter::reserveNets(size_t net_count) {
  _nets->reserve(net_count);
}

int GlobalRouter::getNetIdx(Net* net) {
  return net - &(*_nets)[0];
}

Net* GlobalRouter::getNetByIdx(int idx) {
  return &(*_nets)[idx];
}

int GlobalRouter::getMaxNetDegree() {
  int maxDegree = -1;
  for (Net &net : *_nets) {
    int netDegree = net.getNumPins();
    if (netDegree > maxDegree) {
      maxDegree = netDegree;
    }
  }
  return maxDegree;
}

std::vector<Pin> GlobalRouter::getAllPorts() {
  std::vector<Pin> ports; 
  for (Net &net : *_nets) {
    for (Pin pin : net.getPins()) {
      if (pin.isPort()) {
        ports.push_back(pin);
      }
    }
  }
  return ports;
}

}  // namespace FastRoute
