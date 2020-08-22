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

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "AntennaRepair.h"
#include "Box.h"
#include "Coordinate.h"
#include "FastRoute.h"
#include "Grid.h"
#include "RcTreeBuilder.h"
#include "RoutingLayer.h"
#include "RoutingTracks.h"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "openroad/Error.hh"
#include "openroad/OpenRoad.hh"
#include "sta/Parasitics.hh"
#include "sta/Clock.hh"
#include "sta/Set.hh"

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
  _allRoutingTracks = new std::vector<RoutingTracks>;
  _db = _openroad->getDb();
  _fastRoute = new FT;
  _grid = new Grid;
  _gridOrigin = new Coordinate(-1, -1);
  _nets = new std::vector<Net>;
  _openSta = _openroad->getSta();
  _routingLayers = new std::vector<RoutingLayer>;
}

void GlobalRouter::deleteComponents()
{
  delete _allRoutingTracks;
  delete _db;
  delete _fastRoute;
  delete _grid;
  delete _gridOrigin;
  delete _nets;
  delete _openSta;
  delete _routes;
  delete _routingLayers;
}

void GlobalRouter::clear()
{
  // Clear classes
  _grid->clear();
  _fastRoute->clear();

  // Clear vector
  _allRoutingTracks->clear();
  _nets->clear();
  _db_net_map.clear();
  delete _routes;
  _routes = nullptr;
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
  printHeader();
  if (_unidirectionalRoute) {
    _minRoutingLayer = 2;
    _fixLayer = 1;
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
  initCoreGrid();
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
}

void GlobalRouter::runFastRoute()
{
  std::cout << "Running FastRoute...\n\n";
  _fastRoute->initAuxVar();
  if (_enableAntennaFlow) {
    runAntennaAvoidanceFlow();
  } else if (_clockNetsRouteFlow) {
    runClockNetsRouteFlow();
  } else {
    _routes = _fastRoute->run();
    addRemainingGuides(_routes);
    connectPadPins(_routes);
  }
  std::cout << "Running FastRoute... Done!\n";

  mergeSegments();
  computeWirelength();

  if (_reportCongest) {
    _fastRoute->writeCongestionReport2D(_congestFile + "2D.log");
    _fastRoute->writeCongestionReport3D(_congestFile + "3D.log");
  }
}

void GlobalRouter::runAntennaAvoidanceFlow()
{
  std::cout << "Running antenna avoidance flow...\n";

  AntennaRepair* antennaRepair = 
    new AntennaRepair(this, _openroad->getAntennaChecker(),
                       _openroad->getOpendp(), _db);

  NetRouteMap *globalRoute = _fastRoute->run();
  addRemainingGuides(globalRoute);
  connectPadPins(globalRoute);

  // Save routing.
  NetRouteMap* originalRoute = new NetRouteMap(*globalRoute);

  getPreviousCapacities(_minRoutingLayer);
  addLocalConnections(globalRoute);

  int violationsCnt
      = antennaRepair->checkAntennaViolations(globalRoute, _maxRoutingLayer);

  clear();

  // Restore routing.
  _routes = originalRoute;

  if (violationsCnt > 0) {
    antennaRepair->fixAntennas(_diodeCellName, _diodePinName);
    antennaRepair->legalizePlacedCells();
    _reroute = true;
    startFastRoute();
    _fastRoute->setVerbose(0);
    std::cout << "[INFO] #Nets to reroute: " << _fastRoute->getNets().size()
              << "\n";

    restorePreviousCapacities(_minRoutingLayer);

    _fastRoute->initAuxVar();
    NetRouteMap* newRoute = _fastRoute->run();
    addRemainingGuides(newRoute);
    connectPadPins(newRoute);
    mergeResults(newRoute);
  }

  for (auto &net_route : *_routes) {
    GRoute &route = net_route.second;
    mergeSegments(route);
  }
}

void GlobalRouter::runClockNetsRouteFlow()
{
  _fastRoute->setVerbose(0);
  NetRouteMap* clockNetsRoute = _fastRoute->run();
  addRemainingGuides(clockNetsRoute);
  connectPadPins(clockNetsRoute);

  getPreviousCapacities(_minLayerForClock);

  clear();
  _onlyClockNets = false;
  _onlySignalNets = true;

  startFastRoute();
  restorePreviousCapacities(_minLayerForClock);

  _fastRoute->initAuxVar();
  _routes = _fastRoute->run();
  addRemainingGuides(_routes);
  connectPadPins(_routes);
  for (auto &net_route : *_routes) {
    GRoute &route = net_route.second;
    mergeSegments(route);
  }

  mergeResults(clockNetsRoute);
}

void GlobalRouter::estimateRC()
{
  // Remove any existing parasitics.
  sta::dbSta* dbSta = _openroad->getSta();
  dbSta->deleteParasitics();

  RcTreeBuilder builder(_openroad, this);
  for (auto &net_route : *_routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    if (!route.empty()) {
      Net* net = getNet(db_net);
      builder.estimateParasitcs(db_net, net->getPins(), route);
    }
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

void GlobalRouter::initCoreGrid()
{
  initGrid(_maxRoutingLayer);

  computeCapacities(_maxRoutingLayer, _layerPitches);
  computeSpacingsAndMinWidth(_maxRoutingLayer);
  initObstacles();

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
  initRoutingTracks(
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
  initNetlist(reroute);

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
          std::vector<PIN> pins;
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
              GSegment pinConnection = createFakePin(pin, pinPosition, layer);
              _padPinsConnections[&net].push_back(pinConnection);
            }

            PIN grPin;
            grPin.x = pinPosition.getX();
            grPin.y = pinPosition.getY();
            grPin.layer = topLayer;
            pins.push_back(grPin);
          }

          PIN grPins[pins.size()];
          int count = 0;

          for (PIN pin : pins) {
            grPins[count] = pin;
            count++;
          }

          float netAlpha = _alpha;
          if (_netsAlpha.find(net.getName()) != _netsAlpha.end()) {
            netAlpha = _netsAlpha[net.getName()];
          }

          bool isClock = (net.getSignalType() == odb::dbSigType::CLOCK);
          _fastRoute->addNet(net.getDbNet(), pins.size(), 1,
			     grPins, netAlpha, isClock);
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
  _congestFile = congestFile;
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

  std::cout << "[INFO] Num routed nets: " << _routes->size() << "\n";
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
    GRoute &route = (*_routes)[db_net];
    if (!route.empty()) {
      guideFile << db_net->getConstName() << "\n";
      guideFile << "(\n";
      std::vector<Box> guideBox;
      finalLayer = -1;
      for (GSegment &segment : route) {
	if (segment.initLayer != finalLayer && finalLayer != -1) {
	  mergeBox(guideBox);
	  for (Box guide : guideBox) {
	    guideFile << guide.getLowerBound().getX() + offsetX << " "
		      << guide.getLowerBound().getY() + offsetY << " "
		      << guide.getUpperBound().getX() + offsetX << " "
		      << guide.getUpperBound().getY() + offsetY << " "
		      << phLayerF.getName() << "\n";
	  }
	  guideBox.clear();
	  finalLayer = segment.initLayer;
	}
	if (segment.initLayer == segment.finalLayer) {
	  if (segment.initLayer < _minRoutingLayer && segment.initX != segment.finalX
	      && segment.initY != segment.finalY) {
	    error("Routing with guides in blocked metal for net %s\n",
		  db_net->getConstName());
	  }
	  Box box = globalRoutingToBox(segment);
	  guideBox.push_back(box);
	  if (segment.finalLayer < _minRoutingLayer && !_unidirectionalRoute) {
	    phLayerF = getRoutingLayerByIndex(
					      (segment.finalLayer + (_minRoutingLayer - segment.finalLayer)));
	  } else {
	    phLayerF = getRoutingLayerByIndex(segment.finalLayer);
	  }
	  finalLayer = segment.finalLayer;
	} else {
	  if (abs(segment.finalLayer - segment.initLayer) > 1) {
	    error("Connection between non-adjacent layers in net %s\n",
		  db_net->getConstName());
	  } else {
	    RoutingLayer phLayerI;
	    if (segment.initLayer < _minRoutingLayer && !_unidirectionalRoute) {
	      phLayerI = getRoutingLayerByIndex(segment.initLayer + _minRoutingLayer
						- segment.initLayer);
	    } else {
	      phLayerI = getRoutingLayerByIndex(segment.initLayer);
	    }
	    if (segment.finalLayer < _minRoutingLayer && !_unidirectionalRoute) {
	      phLayerF = getRoutingLayerByIndex(
						segment.finalLayer + _minRoutingLayer - segment.finalLayer);
	    } else {
	      phLayerF = getRoutingLayerByIndex(segment.finalLayer);
	    }
	    finalLayer = segment.finalLayer;
	    Box box;
	    box = globalRoutingToBox(segment);
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

	    box = globalRoutingToBox(segment);
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

void GlobalRouter::addRemainingGuides(NetRouteMap *routes)
{
  auto net_pins = _fastRoute->getNets();
  for (auto &net_route : *routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    Net* net = getNet(db_net);
    // Skip nets with 1 pin or less
    if (net->getNumPins() > 1) {
      std::vector<PIN>& pins = net_pins[db_net];
      // Try to add local guides for net with no output of FR core
      if (route.empty()) {
        int lastLayer = -1;
        for (uint p = 0; p < pins.size(); p++) {
          if (p > 0) {
            // If the net is not local, FR core result is invalid
            if (pins[p].x != pins[p - 1].x || pins[p].y != pins[p - 1].y) {
              error("Net %s not properly covered\n",
		    db_net->getConstName());
            }
          }

          if (pins[p].layer > lastLayer)
            lastLayer = pins[p].layer;
        }

        for (int l = _minRoutingLayer - _fixLayer; l <= lastLayer; l++) {
          GSegment segment;
          segment.initLayer = l;
          segment.initX = pins[0].x;
          segment.initY = pins[0].y;
          segment.finalLayer = l + 1;
          segment.finalX = pins[0].x;
          segment.finalY = pins[0].y;
          route.push_back(segment);
        }
      } else {  // For nets with routing, add guides for pin acess at upper
                // layers
        for (PIN pin : pins) {
          if (pin.layer > 1) {
            // for each pin placed at upper layers, get all segments that
            // potentially covers it
            GRoute coverSegs;

            int wireViaLayer = std::numeric_limits<int>::max();
            for (uint i = 0; i < route.size(); i++) {
              if ((pin.x == route[i].initX && pin.y == route[i].initY)
                  || (pin.x == route[i].finalX
                      && pin.y == route[i].finalY)) {
                if (!(route[i].initX == route[i].finalX
                      && route[i].initY == route[i].finalY)) {
                  coverSegs.push_back(route[i]);
                  if (route[i].initLayer < wireViaLayer) {
                    wireViaLayer = route[i].initLayer;
                  }
                }
              }
            }

            bool bottomLayerPin = false;
            for (PIN pin2 : pins) {
              if (pin.x == pin2.x && pin.y == pin2.y
                  && pin.layer > pin2.layer) {
                bottomLayerPin = true;
              }
            }

            if (!bottomLayerPin) {
              for (uint i = 0; i < route.size(); i++) {
                if ((pin.x == route[i].initX && pin.y == route[i].initY)
                    || (pin.x == route[i].finalX
                        && pin.y == route[i].finalY)) {
                  // remove all vias to this pin that doesn't connects two wires
                  if (route[i].initX == route[i].finalX
                      && route[i].initY == route[i].finalY
                      && (route[i].initLayer < wireViaLayer
                          || route[i].finalLayer < wireViaLayer)) {
                    route.erase(route.begin() + i);
                    i = 0;
                  }
                }
              }
            }

            int closestLayer = -1;
            int minorDiff = std::numeric_limits<int>::max();

            for (GSegment &seg : coverSegs) {
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
                GSegment segment;
                segment.initLayer = l;
                segment.initX = pin.x;
                segment.initY = pin.y;
                segment.finalLayer = l - 1;
                segment.finalX = pin.x;
                segment.finalY = pin.y;
                route.push_back(segment);
              }
            } else if (closestLayer < pin.layer) {
              for (int l = closestLayer; l < pin.layer; l++) {
                GSegment segment;
                segment.initLayer = l;
                segment.initX = pin.x;
                segment.initY = pin.y;
                segment.finalLayer = l + 1;
                segment.finalX = pin.x;
                segment.finalY = pin.y;
                route.push_back(segment);
              }
            }
          }
        }
      }
    }
  }

  // Add local guides for nets with no routing.
  for (Net& net : *_nets) {
    odb::dbNet* db_net = net.getDbNet();
    if (checkSignalType(net)
        && net.getNumPins() > 1
	&& (routes->find(db_net) == routes->end()
	    || (*routes)[db_net].empty())) {
      GRoute &route = (*routes)[db_net];
      for (PIN &pin : net_pins[db_net]) {
        GSegment segment;
        segment.initLayer = pin.layer;
        segment.initX = pin.x;
        segment.initY = pin.y;
        segment.finalLayer = pin.layer;
        segment.finalX = pin.x;
        segment.finalY = pin.y;
        route.push_back(segment);
      }
    }
  }
}

void GlobalRouter::connectPadPins(NetRouteMap *routes)
{
  for (auto &net_route : *routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    Net* net = getNet(db_net);
    if (_padPinsConnections.find(net) != _padPinsConnections.end()
        || net->getNumPins() > 1) {
      for (GSegment &segment : _padPinsConnections[net]) {
        route.push_back(segment);
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

Box GlobalRouter::globalRoutingToBox(const GSegment& route)
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

  for (Pin* port : getAllPorts()) {
    if (port->getNumLayers() == 0) {
      error("Pin %s does not have layer assignment\n", port->getName().c_str());
    }
    DBU layer = port->getLayers()[0];  // port have only one layer

    if (mapLayerToPositions[layer].empty()) {
      mapLayerToPositions[layer].push_back(port->getPosition());
    } else {
      for (Coordinate pos : mapLayerToPositions[layer]) {
        if (pos == port->getPosition()) {
          std::cout << "[WARNING] At least 2 pins in position (" << pos.getX()
                    << ", " << pos.getY() << "), layer " << layer + 1 << "\n";
          invalid = true;
        }
      }
      mapLayerToPositions[layer].push_back(port->getPosition());
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

void GlobalRouter::computeWirelength()
{
  DBU totalWirelength = 0;
  for (auto &net_route : *_routes) {
    GRoute &route = net_route.second;
    for (GSegment &segment : route) {
      DBU segmentWl = std::abs(segment.finalX - segment.initX)
                    + std::abs(segment.finalY - segment.initY);
      totalWirelength += segmentWl;

      if (segmentWl > 0) {
        totalWirelength += (_grid->getTileWidth() + _grid->getTileHeight()) / 2;
      }
    }
  }
  std::cout << std::fixed << "[INFO] Total wirelength: "
            << (float) totalWirelength / _grid->getDatabaseUnit() << " um\n";
}

void GlobalRouter::mergeSegments()
{
  for (auto &net_route : *_routes) {
    GRoute &route = net_route.second;
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
    std::map<Point, int> segsAtPoint;
    for (const GSegment& seg : segments) {
      segsAtPoint[{seg.initX, seg.initY, seg.initLayer}] += 1;
      segsAtPoint[{seg.finalX, seg.finalY, seg.finalLayer}] += 1;
    }

    uint i = 0;
    while (i < segments.size() - 1) {
      GSegment &segment0 = segments[i];
      GSegment &segment1 = segments[i + 1];

      // both segments are not vias
      if (segment0.initLayer == segment0.finalLayer
	  && segment1.initLayer == segment1.finalLayer &&
	  // segments are on the same layer
	  segment0.initLayer == segment1.initLayer) {
	// if segment 0 connects to the end of segment 1
	GSegment &newSeg = segments[i];
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

void GlobalRouter::addLocalConnections(NetRouteMap *routes)
{
  int topLayer;
  std::vector<Box> pinBoxes;
  Coordinate pinPosition;
  Coordinate realPinPosition;
  GSegment horSegment;
  GSegment verSegment;

  for (auto &net_route : *routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    Net* net = getNet(db_net);

    for (Pin &pin : net->getPins()) {
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

      route.push_back(horSegment);
      route.push_back(verSegment);
    }
  }
}

void GlobalRouter::mergeResults(NetRouteMap *routes)
{
  for (auto &net_route : *routes) {
    odb::dbNet* db_net = net_route.first;
    GRoute &route = net_route.second;
    (*_routes)[db_net] = route;
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

GSegment GlobalRouter::createFakePin(Pin pin,
				     Coordinate& pinPosition,
				     RoutingLayer layer)
{
  int topLayer = layer.getIndex();
  GSegment pinConnection;
  pinConnection.initLayer = topLayer;
  pinConnection.finalLayer = topLayer;

  if (layer.getPreferredDirection() == RoutingLayer::HORIZONTAL) {
    pinConnection.finalX = pinPosition.getX();
    pinConnection.initY = pinPosition.getY();
    pinConnection.finalY = pinPosition.getY();

    DBU newXPosition;
    if (pin.getOrientation() == PinOrientation::west) {
      newXPosition
          = pinPosition.getX() + (_gcellsOffset * _grid->getTileWidth());
      pinConnection.initX = newXPosition;
      pinPosition.setX(newXPosition);
    } else if (pin.getOrientation() == PinOrientation::east) {
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
    if (pin.getOrientation() == PinOrientation::south) {
      newYPosition
          = pinPosition.getY() + (_gcellsOffset * _grid->getTileHeight());
      pinConnection.initY = newYPosition;
      pinPosition.setY(newYPosition);
    } else if (pin.getOrientation() == PinOrientation::north) {
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

std::vector<Pin*> GlobalRouter::getAllPorts() {
  std::vector<Pin*> ports; 
  for (Net &net : *_nets) {
    for (Pin &pin : net.getPins()) {
      if (pin.isPort()) {
        ports.push_back(&pin);
      }
    }
  }
  return ports;
}

// db functions

void GlobalRouter::initGrid(int maxLayer)
{
  _block = _db->getChip()->getBlock();

  odb::dbTech* tech = _db->getTech();

  odb::dbTechLayer* selectedLayer = tech->findRoutingLayer(selectedMetal);

  if (selectedLayer == nullptr) {
    error("Layer %d not found\n", selectedMetal);
  }

  odb::dbTrackGrid* selectedTrack = _block->findTrackGrid(selectedLayer);

  if (selectedTrack == nullptr) {
    error("Track for layer %d not found\n", selectedMetal);
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
    error("Layer %d does not have valid direction\n", selectedMetal);
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
  std::map<int, std::vector<Box>> genericMap;

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
                genericMap,
                tech->getLefUnits());
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
      error("Layer %d does not have valid direction\n", l);
    }

    RoutingLayer routingLayer = RoutingLayer(index, name, preferredDirection);
    routingLayers.push_back(routingLayer);
  }
}

void GlobalRouter::initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                                  int maxLayer,
                                  std::map<int, float> layerPitches)
{
  odb::dbTech* tech = _db->getTech();

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    if (layer > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(layer);

    if (techLayer == nullptr) {
      error("Layer %d not found\n", selectedMetal);
    }

    odb::dbTrackGrid* selectedTrack = _block->findTrackGrid(techLayer);

    if (selectedTrack == nullptr) {
      error("Track for layer %d not found\n", selectedMetal);
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
      if (layerPitches.find(layer) != layerPitches.end()) {
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
      if (layerPitches.find(layer) != layerPitches.end()) {
        line2ViaPitch = (int) (tech->getLefUnits() * layerPitches[layer]);
      } else {
        line2ViaPitch = -1;
      }
      location = initTrackX;
      numTracks = numTracksX;
      orientation = RoutingLayer::VERTICAL;
    } else {
      error("Layer %d does not have valid direction\n",
            selectedMetal);
    }

    RoutingTracks routingTracks = RoutingTracks(
        layer, trackPitch, line2ViaPitch, location, numTracks, orientation);
    allRoutingTracks.push_back(routingTracks);
  }
}

void GlobalRouter::computeCapacities(int maxLayer,
                                  std::map<int, float> layerPitches)
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
      error("Track for layer %d not found\n", l);
    }

    track->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
    track->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

    if (techLayer->getDirection().getValue()
        == odb::dbTechLayerDir::HORIZONTAL) {
      trackSpacing = trackStepY;

      if (layerPitches.find(l) != layerPitches.end()) {
        int layerPitch = (int) (tech->getLefUnits() * layerPitches[l]);
        trackSpacing = std::max(layerPitch, trackStepY);
      }

      hCapacity = std::floor((float) _grid->getTileWidth() / trackSpacing);

      _grid->addHorizontalCapacity(hCapacity, l - 1);
      _grid->addVerticalCapacity(0, l - 1);
    } else if (techLayer->getDirection().getValue()
               == odb::dbTechLayerDir::VERTICAL) {
      trackSpacing = trackStepX;

      if (layerPitches.find(l) != layerPitches.end()) {
        int layerPitch = (int) (tech->getLefUnits() * layerPitches[l]);
        trackSpacing = std::max(layerPitch, trackStepX);
      }

      vCapacity = std::floor((float) _grid->getTileWidth() / trackSpacing);

      _grid->addHorizontalCapacity(0, l - 1);
      _grid->addVerticalCapacity(vCapacity, l - 1);
    } else {
      error("Layer %d does not have valid direction\n", l);
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
      error("Track for layer %d not found\n", l);
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
      error("Layer %d does not have valid direction\n", l);
    }

    _grid->addSpacing(minSpacing, l - 1);
    _grid->addMinWidth(minWidth, l - 1);
  }
}

void GlobalRouter::initNetlist(bool reroute)
{
  initClockNets();

  odb::dbTech* tech = _db->getTech();

  if (reroute) {
    if (_dirtyNets.empty()) {
      error("Not found any dirty net to reroute");
    }

    addNets(_dirtyNets);
  } else {
    std::vector<odb::dbNet*> nets;

    for (odb::dbNet* net : _block->getNets()) {
      nets.push_back(net);
    }

    if (nets.empty()) {
      error("Design without nets");
    }

    addNets(nets);
  }
}

void GlobalRouter::addNets(std::vector<odb::dbNet*> nets)
{
  Box dieArea(_grid->getLowerLeftX(),
              _grid->getLowerLeftY(),
              _grid->getUpperRightX(),
              _grid->getUpperRightY(),
              -1);

  // Prevent _nets from growing because pointers to nets become invalid.
  reserveNets(nets.size());
  for (odb::dbNet* db_net : nets) {
    if (db_net->getSigType().getValue() != odb::dbSigType::POWER
        && db_net->getSigType().getValue() != odb::dbSigType::GROUND
        && !db_net->isSpecial() && db_net->getSWires().empty()) {
      Net* net = addNet(db_net);
      _db_net_map[db_net] = net;
      makeItermPins(net, db_net, dieArea);
      makeBtermPins(net, db_net, dieArea);
    }
  }
}

Net* GlobalRouter::getNet(odb::dbNet* db_net)
{
  return _db_net_map[db_net];
}

void GlobalRouter::initClockNets()
{
  std::set<odb::dbNet*> _clockNets;

  _openSta->findClkNets(_clockNets);

  std::cout << "[INFO] Found " << _clockNets.size() << " clock nets\n";

  for (odb::dbNet* net : _clockNets) {
    net->setSigType(odb::dbSigType::CLOCK);
  }
}

void GlobalRouter::makeItermPins(Net* net, odb::dbNet* db_net, Box& dieArea)
{
  odb::dbTech* tech = _db->getTech();
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    int pX, pY;
    std::vector<int> pinLayers;
    std::map<int, std::vector<Box>> pinBoxes;

    odb::dbMTerm* mTerm = iterm->getMTerm();
    odb::dbMaster* master = mTerm->getMaster();

    if (master->getType() == odb::dbMasterType::COVER
        || master->getType() == odb::dbMasterType::COVER_BUMP) {
      std::cout << "[WARNING] Net connected with instance of class COVER added "
                   "for routing\n";
    }

    bool connectedToPad = master->getType().isPad();
    bool connectedToMacro = master->isBlock();

    Coordinate pinPos;

    odb::dbInst* inst = iterm->getInst();
    inst->getOrigin(pX, pY);
    odb::Point origin = odb::Point(pX, pY);
    odb::dbTransform transform(inst->getOrient(), origin);

    odb::dbBox* instBox = inst->getBBox();
    Coordinate instMiddle = Coordinate(
        (instBox->xMin() + (instBox->xMax() - instBox->xMin()) / 2.0),
        (instBox->yMin() + (instBox->yMax() - instBox->yMin()) / 2.0));

    for (odb::dbMPin* mterm : mTerm->getMPins()) {
      Coordinate lowerBound;
      Coordinate upperBound;
      Box pinBox;
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
        lowerBound = Coordinate(rect.xMin(), rect.yMin());
        upperBound = Coordinate(rect.xMax(), rect.yMax());
        pinBox = Box(lowerBound, upperBound, pinLayer);
        if (!dieArea.inside(pinBox)) {
          std::cout << "[WARNING] Pin " << getITermName(iterm)
                    << " is outside die area\n";
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
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToPin = pinPosition.getX() - instMiddle.getX();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToPin = pinPosition.getY() - instMiddle.getY();
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

void GlobalRouter::makeBtermPins(Net* net, odb::dbNet* db_net, Box& dieArea)
{
  odb::dbTech* tech = _db->getTech();
  for (odb::dbBTerm* bterm : db_net->getBTerms()) {
    int posX, posY;
    std::string pinName;

    bterm->getFirstPinLocation(posX, posY);
    odb::dbITerm* iterm = bterm->getITerm();
    bool connectedToPad = false;
    bool connectedToMacro = false;
    Coordinate instMiddle = Coordinate(-1, -1);

    if (iterm != nullptr) {
      odb::dbMTerm* mterm = iterm->getMTerm();
      odb::dbMaster* master = mterm->getMaster();
      connectedToPad = master->getType().isPad();
      connectedToMacro = master->isBlock();

      odb::dbInst* inst = iterm->getInst();
      odb::dbBox* instBox = inst->getBBox();
      instMiddle = Coordinate(
          (instBox->xMin() + (instBox->xMax() - instBox->xMin()) / 2.0),
          (instBox->yMin() + (instBox->yMax() - instBox->yMin()) / 2.0));
    }

    std::vector<int> pinLayers;
    std::map<int, std::vector<Box>> pinBoxes;

    pinName = bterm->getConstName();
    Coordinate pinPos;

    for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
      Coordinate lowerBound;
      Coordinate upperBound;
      Box pinBox;
      int pinLayer;
      int lastLayer = -1;

      odb::dbBox* currBTermBox = bterm_pin->getBox();
      odb::dbTechLayer* techLayer = currBTermBox->getTechLayer();
      if (techLayer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
        continue;
      }

      pinLayer = techLayer->getRoutingLevel();
      lowerBound = Coordinate(currBTermBox->xMin(), currBTermBox->yMin());
      upperBound = Coordinate(currBTermBox->xMax(), currBTermBox->yMax());
      pinBox = Box(lowerBound, upperBound, pinLayer);
      if (!dieArea.inside(pinBox)) {
        std::cout << "[WARNING] Pin " << pinName << " is outside die area\n";
      }
      pinBoxes[pinLayer].push_back(pinBox);

      if (pinLayer > lastLayer) {
        pinPos = lowerBound;
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

    if (connectedToPad) {
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToPin = pinPosition.getX() - instMiddle.getX();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::east);
        } else {
          pin.setOrientation(PinOrientation::west);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToPin = pinPosition.getY() - instMiddle.getY();
        if (instToPin < 0) {
          pin.setOrientation(PinOrientation::north);
        } else {
          pin.setOrientation(PinOrientation::south);
        }
      }
    } else {
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToDie = pinPosition.getX() - dieArea.getMiddle().getX();
        if (instToDie < 0) {
          pin.setOrientation(PinOrientation::west);
        } else {
          pin.setOrientation(PinOrientation::east);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToDie = pinPosition.getY() - dieArea.getMiddle().getY();
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

void GlobalRouter::initObstacles()
{
  Box dieArea(_grid->getLowerLeftX(),
              _grid->getLowerLeftY(),
              _grid->getUpperRightX(),
              _grid->getUpperRightY(),
              -1);

  // Get routing obstructions
  odb::dbTech* tech = _db->getTech();

  std::map<int, uint> layerExtensions;

  for (odb::dbTechLayer* obstructLayer : tech->getLayers()) {
    if (obstructLayer->getType().getValue() != odb::dbTechLayerType::ROUTING) {
      continue;
    }

    int maxInt = std::numeric_limits<int>::max();

    // Gets the smallest possible minimum spacing that won't cause violations
    // for ANY configuration of PARALLELRUNLENGTH (the biggest value in the
    // table)

    uint macroExtension = obstructLayer->getSpacing(maxInt, maxInt);

    odb::dbSet<odb::dbTechLayerSpacingRule> eolRules;

    // Check for EOL spacing values and, if the spacing is higher than the one
    // found, use them as the macro extension instead of PARALLELRUNLENGTH

    if (obstructLayer->getV54SpacingRules(eolRules)) {
      for (odb::dbTechLayerSpacingRule* currentRule : eolRules) {
        uint currentSpacing = currentRule->getSpacing();
        if (currentSpacing > macroExtension) {
          macroExtension = currentSpacing;
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
        if (lastValue > macroExtension) {
          macroExtension = lastValue;
        }
      }
    }

    // Save the extension to use when defining Macros

    layerExtensions[obstructLayer->getRoutingLevel()] = macroExtension;
  }

  int obstructionsCnt = 0;

  for (odb::dbObstruction* currObstruct : _block->getObstructions()) {
    odb::dbBox* obstructBox = currObstruct->getBBox();

    int layer = obstructBox->getTechLayer()->getRoutingLevel();

    Coordinate lowerBound
        = Coordinate(obstructBox->xMin(), obstructBox->yMin());
    Coordinate upperBound
        = Coordinate(obstructBox->xMax(), obstructBox->yMax());
    Box obstacleBox = Box(lowerBound, upperBound, layer);
    if (!dieArea.inside(obstacleBox)) {
      std::cout << "[WARNING] Found obstacle outside die area\n";
    }
    _grid->addObstacle(layer, obstacleBox);
    obstructionsCnt++;
  }

  std::cout << "[INFO] #DB Obstructions: " << obstructionsCnt << "\n";

  // Get instance obstructions
  int macrosCnt = 0;
  int obstaclesCnt = 0;
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

      uint macroExtension = 0;

      if (isMacro) {
        macroExtension = layerExtensions[currBox->getTechLayer()->getRoutingLevel()];
      }

      Coordinate lowerBound = Coordinate(rect.xMin() - macroExtension,
                                         rect.yMin() - macroExtension);
      Coordinate upperBound = Coordinate(rect.xMax() + macroExtension,
                                         rect.yMax() + macroExtension);
      Box obstacleBox = Box(lowerBound, upperBound, layer);
      if (!dieArea.inside(obstacleBox)) {
        std::cout << "[WARNING] Found obstacle outside die area in instance "
                  << currInst->getConstName() << "\n";
      }
      _grid->addObstacle(layer, obstacleBox);
      obstaclesCnt++;
    }

    for (odb::dbMTerm* mTerm : master->getMTerms()) {
      for (odb::dbMPin* mterm : mTerm->getMPins()) {
        Coordinate lowerBound;
        Coordinate upperBound;
        Box pinBox;
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
          lowerBound = Coordinate(rect.xMin(), rect.yMin());
          upperBound = Coordinate(rect.xMax(), rect.yMax());
          pinBox = Box(lowerBound, upperBound, pinLayer);
          if (!dieArea.inside(pinBox)) {
            std::cout << "[WARNING] Found pin outside die area in instance "
                      << currInst->getConstName() << "\n";
          }
          _grid->addObstacle(pinLayer, pinBox);
        }
      }
    }
  }

  std::cout << "[INFO] #DB Obstacles: " << obstaclesCnt << "\n";
  std::cout << "[INFO] #DB Macros: " << macrosCnt << "\n";

  // Get nets obstructions (routing wires and pdn wires)
  odb::dbSet<odb::dbNet> nets = _block->getNets();

  if (nets.empty()) {
    error("Design without nets\n");
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

            Coordinate lowerBound
                = Coordinate(wireRect.xMin(), wireRect.yMin());
            Coordinate upperBound
                = Coordinate(wireRect.xMax(), wireRect.yMax());
            Box obstacleBox = Box(lowerBound, upperBound, l);
            if (!dieArea.inside(obstacleBox)) {
              std::cout << "[WARNING] Net " << db_net->getConstName()
                        << " has wires outside die area\n";
            }
            _grid->addObstacle(l, obstacleBox);
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

            Coordinate lowerBound
                = Coordinate(wireRect.xMin(), wireRect.yMin());
            Coordinate upperBound
                = Coordinate(wireRect.xMax(), wireRect.yMax());
            Box obstacleBox = Box(lowerBound, upperBound, l);
            if (!dieArea.inside(obstacleBox)) {
              std::cout << "[WARNING] Net " << db_net->getConstName()
                        << " has wires outside die area\n";
            }
            _grid->addObstacle(l, obstacleBox);
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
      std::cout << "[ERROR] Layer" << selectedMetal
                << " not found\n";
      std::exit(1);
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
    error("Tech without vias\n");
  }

  std::vector<odb::dbTechVia*> defaultVias;

  for (odb::dbTechVia* currVia : vias) {
    odb::dbStringProperty* prop
        = odb::dbStringProperty::find(currVia, "OR_DEFAULT");

    if (prop == nullptr) {
      continue;
    } else {
      std::cout << "[INFO] Default via: " << currVia->getConstName() << "\n";
      defaultVias.push_back(currVia);
    }
  }

  if (defaultVias.empty()) {
    std::cout << "[WARNING]No OR_DEFAULT vias defined\n";
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
      std::cout << "[INFO] Default via: " << currVia->getConstName() << "\n";
      defaultVias[currVia->getBottomLayer()->getRoutingLevel()] = currVia;
    }
  }

  if (defaultVias.empty()) {
    std::cout << "[WARNING]No OR_DEFAULT vias defined\n";
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

const char *
getNetName(odb::dbNet* db_net)
{
  return db_net->getConstName();
}

// Useful for debugging.
void print(GRoute &route)
{
  for (GSegment &segment : route) {
    printf("%6d %6d %2d -> %6d %6d %2d\n",
	   segment.initX,
	   segment.initY,
	   segment.initLayer,
	   segment.finalX,
	   segment.finalY,
	   segment.finalLayer);
  }
}

}  // namespace FastRoute
