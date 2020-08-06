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

#include "DBWrapper.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Box.h"
#include "Coordinate.h"
#include "Pin.h"
#include "openroad/Error.hh"

namespace FastRoute {

using ord::error;

DBWrapper::DBWrapper(odb::dbDatabase* db, Netlist* netlist, Grid* grid)
    : _db(db), _netlist(netlist), _grid(grid)
{
}

void DBWrapper::initGrid(int maxLayer)
{
  // WORKAROUND: Initializing _chip here while we don't have a
  // "populateFastRoute" function"
  _chip = _db->getChip();

  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  odb::dbTechLayer* selectedLayer = tech->findRoutingLayer(selectedMetal);

  if (!selectedLayer) {
    error("Layer %d not found\n", selectedMetal);
  }

  odb::dbTrackGrid* selectedTrack = block->findTrackGrid(selectedLayer);

  if (!selectedTrack) {
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
  block->getDieArea(rect);

  long lowerLeftX = rect.xMin();
  long lowerLeftY = rect.yMin();

  long upperRightX = rect.xMax();
  long upperRightY = rect.yMax();

  long tileWidth = _grid->getPitchesInTile() * trackSpacing;
  long tileHeight = _grid->getPitchesInTile() * trackSpacing;

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

  *_grid = Grid(lowerLeftX,
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

void DBWrapper::initRoutingLayers(std::vector<RoutingLayer>& routingLayers)
{
  odb::dbTech* tech = _db->getTech();

  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

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

void DBWrapper::initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                                  int maxLayer,
                                  std::map<int, float> layerPitches)
{
  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    if (layer > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techayer = tech->findRoutingLayer(layer);

    if (!techayer) {
      error("Layer %d not found\n", selectedMetal);
    }

    odb::dbTrackGrid* selectedTrack = block->findTrackGrid(techayer);

    if (!selectedTrack) {
      error("Track for layer %d not found\n", selectedMetal);
    }

    int trackStepX, trackStepY;
    int initTrackX, numTracksX;
    int initTrackY, numTracksY;
    int trackPitch, line2ViaPitch, location, numTracks;
    bool orientation;

    selectedTrack->getGridPatternX(0, initTrackX, numTracksX, trackStepX);
    selectedTrack->getGridPatternY(0, initTrackY, numTracksY, trackStepY);

    if (techayer->getDirection().getValue()
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
    } else if (techayer->getDirection().getValue()
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
      error("Layer %d does not have valid direction! Exiting...\n",
            selectedMetal);
    }

    RoutingTracks routingTracks = RoutingTracks(
        layer, trackPitch, line2ViaPitch, location, numTracks, orientation);
    allRoutingTracks.push_back(routingTracks);
  }
}

void DBWrapper::computeCapacities(int maxLayer,
                                  std::map<int, float> layerPitches)
{
  int trackSpacing;
  int hCapacity, vCapacity;
  int trackStepX, trackStepY;

  int initTrackX, numTracksX;
  int initTrackY, numTracksY;

  odb::dbTech* tech = _db->getTech();

  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(l);

    odb::dbTrackGrid* track = block->findTrackGrid(techLayer);

    if (!track) {
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

void DBWrapper::computeSpacingsAndMinWidth(int maxLayer)
{
  int minSpacing = 0;
  int minWidth;
  int trackStepX, trackStepY;
  int initTrackX, numTracksX;
  int initTrackY, numTracksY;

  odb::dbTech* tech = _db->getTech();

  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  for (int l = 1; l <= tech->getRoutingLayerCount(); l++) {
    if (l > maxLayer && maxLayer > -1) {
      break;
    }

    odb::dbTechLayer* techLayer = tech->findRoutingLayer(l);

    odb::dbTrackGrid* track = block->findTrackGrid(techLayer);

    if (!track) {
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

void DBWrapper::initNetlist(bool reroute)
{
  Box dieArea(_grid->getLowerLeftX(),
              _grid->getLowerLeftY(),
              _grid->getUpperRightX(),
              _grid->getUpperRightY(),
              -1);

  odb::dbBlock* block = _chip->getBlock();
  odb::dbTech* tech = _db->getTech();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  std::vector<odb::dbNet*> nets;

  for (odb::dbNet* net : block->getNets()) {
    nets.push_back(net);
  }

  if (nets.size() == 0) {
    error("Design without nets");
  }

  if (reroute) {
    nets = dirtyNets;
  }

  // Sort nets so guide file net order is consistent.
  std::vector<odb::dbNet*> sorted_nets;
  for (odb::dbNet* net : nets)
    sorted_nets.push_back(net);
  std::sort(sorted_nets.begin(),
            sorted_nets.end(),
            [](odb::dbNet* net1, odb::dbNet* net2) {
              return strcmp(net1->getConstName(), net2->getConstName()) < 0;
            });
  // Prevent _netlist->_nets from growing because pointers to nets become
  // invalid.
  _netlist->reserveNets(nets.size());
  for (odb::dbNet* db_net : sorted_nets) {
    if (db_net->getSigType().getValue() != odb::dbSigType::POWER
        && db_net->getSigType().getValue() != odb::dbSigType::GROUND
        && !db_net->isSpecial() && db_net->getSWires().size() == 0) {
      Net* net = _netlist->addNet(db_net);
      makeItermPins(net, db_net, dieArea);
      makeBtermPins(net, db_net, dieArea);
    }
  }
}

void DBWrapper::makeItermPins(Net* net, odb::dbNet* db_net, Box& dieArea)
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

    Pin::Type type(Pin::Type::OTHER);
    if (mTerm->getIoType() == odb::dbIoType::INPUT) {
      type = Pin::SINK;
    } else if (mTerm->getIoType() == odb::dbIoType::OUTPUT) {
      type = Pin::SOURCE;
    }

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
            Orientation::INVALID,
            pinBoxes,
            (connectedToPad || connectedToMacro),
            type);

    if (connectedToPad || connectedToMacro) {
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToPin = pinPosition.getX() - instMiddle.getX();
        if (instToPin < 0) {
          pin.setOrientation(Orientation::ORIENT_EAST);
        } else {
          pin.setOrientation(Orientation::ORIENT_WEST);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToPin = pinPosition.getY() - instMiddle.getY();
        if (instToPin < 0) {
          pin.setOrientation(Orientation::ORIENT_NORTH);
        } else {
          pin.setOrientation(Orientation::ORIENT_SOUTH);
        }
      }
    }

    net->addPin(pin);
  }
}

void DBWrapper::makeBtermPins(Net* net, odb::dbNet* db_net, Box& dieArea)
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

    Pin::Type type(Pin::Type::OTHER);
    if (bterm->getIoType() == odb::dbIoType::INPUT) {
      type = Pin::SOURCE;
    } else if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
      type = Pin::SINK;
    }

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
            Orientation::INVALID,
            pinBoxes,
            (connectedToPad || connectedToMacro),
            type);

    if (connectedToPad) {
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToPin = pinPosition.getX() - instMiddle.getX();
        if (instToPin < 0) {
          pin.setOrientation(Orientation::ORIENT_EAST);
        } else {
          pin.setOrientation(Orientation::ORIENT_WEST);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToPin = pinPosition.getY() - instMiddle.getY();
        if (instToPin < 0) {
          pin.setOrientation(Orientation::ORIENT_NORTH);
        } else {
          pin.setOrientation(Orientation::ORIENT_SOUTH);
        }
      }
    } else {
      Coordinate pinPosition = pin.getPosition();
      odb::dbTechLayer* techLayer = tech->findRoutingLayer(pin.getTopLayer());

      if (techLayer->getDirection().getValue()
          == odb::dbTechLayerDir::HORIZONTAL) {
        DBU instToDie = pinPosition.getX() - dieArea.getMiddle().getX();
        if (instToDie < 0) {
          pin.setOrientation(Orientation::ORIENT_WEST);
        } else {
          pin.setOrientation(Orientation::ORIENT_EAST);
        }
      } else if (techLayer->getDirection().getValue()
                 == odb::dbTechLayerDir::VERTICAL) {
        DBU instToDie = pinPosition.getY() - dieArea.getMiddle().getY();
        if (instToDie < 0) {
          pin.setOrientation(Orientation::ORIENT_SOUTH);
        } else {
          pin.setOrientation(Orientation::ORIENT_NORTH);
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

void DBWrapper::initObstacles()
{
  Box dieArea(_grid->getLowerLeftX(),
              _grid->getLowerLeftY(),
              _grid->getUpperRightX(),
              _grid->getUpperRightY(),
              -1);

  // Get routing obstructions
  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  std::map<std::string, uint> layerExtensions;

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

    layerExtensions[obstructLayer->getName()] = macroExtension;
  }

  int obstructionsCnt = 0;

  for (odb::dbObstruction* currObstruct : block->getObstructions()) {
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
  for (odb::dbInst* currInst : block->getInsts()) {
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
        macroExtension = layerExtensions[currBox->getTechLayer()->getName()];
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
  odb::dbSet<odb::dbNet> nets = block->getNets();

  if (nets.size() == 0) {
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

int DBWrapper::computeMaxRoutingLayer()
{
  _chip = _db->getChip();

  int maxRoutingLayer = -1;

  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  for (int layer = 1; layer <= tech->getRoutingLayerCount(); layer++) {
    odb::dbTechLayer* techLayer = tech->findRoutingLayer(layer);
    if (!techLayer) {
      std::cout << "[ERROR] Layer" << selectedMetal
                << " not found! Exiting...\n";
      std::exit(1);
    }
    odb::dbTrackGrid* selectedTrack = block->findTrackGrid(techLayer);
    if (!selectedTrack) {
      break;
    }
    maxRoutingLayer = layer;
  }

  return maxRoutingLayer;
}

void DBWrapper::getCutLayerRes(unsigned belowLayerId, float& r)
{
  odb::dbBlock* block = _chip->getBlock();
  odb::dbTech* tech = _db->getTech();
  odb::dbTechLayer* cut = tech->findRoutingLayer(belowLayerId)->getUpperLayer();
  r = cut->getResistance();  // assumes single cut
}

void DBWrapper::getLayerRC(unsigned layerId, float& r, float& c)
{
  odb::dbBlock* block = _chip->getBlock();
  odb::dbTech* tech = _db->getTech();
  odb::dbTechLayer* techLayer = tech->findRoutingLayer(layerId);

  float layerWidth
      = (float) techLayer->getWidth() / block->getDbUnitsPerMicron();
  float resOhmPerMicron = techLayer->getResistance() / layerWidth;
  float capPfPerMicron = layerWidth * techLayer->getCapacitance()
                         + 2 * techLayer->getEdgeCapacitance();

  r = 1E+6 * resOhmPerMicron;         // Meters
  c = 1E+6 * 1E-12 * capPfPerMicron;  // F/m
}

float DBWrapper::dbuToMeters(unsigned dbu)
{
  odb::dbBlock* block = _chip->getBlock();
  return (float) dbu / (block->getDbUnitsPerMicron() * 1E+6);
}

std::set<int> DBWrapper::findTransitionLayers(int maxRoutingLayer)
{
  std::set<int> transitionLayers;
  odb::dbTech* tech = _db->getTech();
  odb::dbSet<odb::dbTechVia> vias = tech->getVias();

  if (vias.size() == 0) {
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

  if (defaultVias.size() == 0) {
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

std::map<int, odb::dbTechVia*> DBWrapper::getDefaultVias(int maxRoutingLayer)
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

  if (defaultVias.size() == 0) {
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

void DBWrapper::commitGlobalSegmentsToDB(std::vector<FastRoute::NET> routing,
                                         int maxRoutingLayer)
{
  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  std::map<int, odb::dbTechVia*> defaultVias = getDefaultVias(maxRoutingLayer);

  for (FastRoute::NET netRoute : routing) {
    std::string netName = netRoute.name;
    odb::dbWire* wire = odb::dbWire::create(dbNets[netName]);
    odb::dbWireEncoder wireEncoder;
    wireEncoder.begin(wire);
    odb::dbWireType wireType = odb::dbWireType::ROUTED;

    for (FastRoute::ROUTE seg : netRoute.route) {
      if (std::abs(seg.initLayer - seg.finalLayer) > 1) {
        error("Global route segment not valid\n");
      }
      int x1 = seg.initX;
      int y1 = seg.initY;
      int x2 = seg.finalX;
      int y2 = seg.finalY;
      int l1 = seg.initLayer;
      int l2 = seg.finalLayer;

      odb::dbTechLayer* currLayer = tech->findRoutingLayer(l1);

      if (l1 == l2) {  // Add wire
        if (x1 == x2 && y1 == y2)
          continue;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addPoint(x2, y2);
      } else {  // Add via
        int bottomLayer = (l1 < l2) ? l1 : l2;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addTechVia(defaultVias[bottomLayer]);
      }
    }
    wireEncoder.end();
  }
}

int DBWrapper::checkAntennaViolations(std::vector<FastRoute::NET> routing,
                                      int maxRoutingLayer)
{
  if (!_chip) {
    _chip = _db->getChip();
  }
  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  _arc = new antenna_checker::AntennaChecker;
  _arc->setDb(_db);
  _arc->load_antenna_rules();

  std::map<int, odb::dbTechVia*> defaultVias = getDefaultVias(maxRoutingLayer);

  odb::dbSet<odb::dbNet> nets = block->getNets();

  for (odb::dbNet* currNet : nets) {
    std::string netName = currNet->getConstName();

    if (currNet->getSigType().getValue() == odb::dbSigType::POWER
        || currNet->getSigType().getValue() == odb::dbSigType::GROUND
        || currNet->isSpecial() || currNet->getSWires().size() > 0) {
      continue;
    }

    dbNets[netName] = currNet;
  }

  for (FastRoute::NET netRoute : routing) {
    std::string netName = netRoute.name;

    odb::dbWire* wire = odb::dbWire::create(dbNets[netName]);
    odb::dbWireEncoder wireEncoder;
    wireEncoder.begin(wire);
    odb::dbWireType wireType = odb::dbWireType::ROUTED;

    for (FastRoute::ROUTE seg : netRoute.route) {
      if (std::abs(seg.initLayer - seg.finalLayer) > 1) {
        error("Global route segment not valid\n");
      }
      int x1 = seg.initX;
      int y1 = seg.initY;
      int x2 = seg.finalX;
      int y2 = seg.finalY;
      int l1 = seg.initLayer;
      int l2 = seg.finalLayer;

      odb::dbTechLayer* currLayer = tech->findRoutingLayer(l1);

      if (l1 == l2) {  // Add wire
        if (x1 == x2 && y1 == y2)
          continue;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addPoint(x2, y2);
      } else {  // Add via
        int bottomLayer = (l1 < l2) ? l1 : l2;
        wireEncoder.newPath(currLayer, wireType);
        wireEncoder.addPoint(x1, y1);
        wireEncoder.addTechVia(defaultVias[bottomLayer]);
      }
    }
    wireEncoder.end();

    odb::orderWires(dbNets[netName], false, false);

    std::vector<VINFO> netViol
        = _arc->get_net_antenna_violations(dbNets[netName]);
    if (netViol.size() > 0) {
      antennaViolations[dbNets[netName]->getConstName()] = netViol;
      dirtyNets.push_back(dbNets[netName]);
    }
    if (wire != nullptr) {
      odb::dbWire::destroy(wire);
    }
  }

  std::cout << "[INFO] #Antenna violations: " << antennaViolations.size()
            << "\n";
  return antennaViolations.size();
}

void DBWrapper::insertDiode(odb::dbNet* net,
                            std::string antennaCellName,
                            std::string antennaPinName,
                            odb::dbInst* sinkInst,
                            odb::dbITerm* sinkITerm,
                            std::string antennaInstName,
                            int siteWidth,
                            r_tree& fixedInsts)
{
  bool legallyPlaced = false;
  bool placeAtLeft = true;
  int leftOffset = 0;
  int rightOffset = 0;
  int offset;

  odb::dbBlock* block = _chip->getBlock();
  std::string netName = net->getConstName();

  odb::dbMaster* antennaMaster = _db->findMaster(antennaCellName.c_str());
  odb::dbSet<odb::dbMTerm> antennaMTerms = antennaMaster->getMTerms();

  int instLocX, instLocY, instWidth;
  odb::dbBox* sinkBBox = sinkInst->getBBox();
  instLocX = sinkBBox->xMin();
  instLocY = sinkBBox->yMin();
  instWidth = sinkBBox->xMax() - sinkBBox->xMin();
  odb::dbOrientType instOrient = sinkInst->getOrient();

  odb::dbInst* antennaInst
      = odb::dbInst::create(block, antennaMaster, antennaInstName.c_str());
  odb::dbITerm* antennaITerm = antennaInst->findITerm(antennaPinName.c_str());
  odb::dbBox* antennaBBox = antennaInst->getBBox();
  int antennaWidth = antennaBBox->xMax() - antennaBBox->xMin();

  // Use R-tree to check if diode will not overlap or cause 1-site spacing with
  // other cells
  std::vector<value> overlapInsts;
  while (!legallyPlaced) {
    if (placeAtLeft) {
      offset = -(antennaWidth + leftOffset * siteWidth);
      leftOffset++;
      placeAtLeft = false;
    } else {
      offset = instWidth + rightOffset * siteWidth;
      rightOffset++;
      placeAtLeft = true;
    }

    antennaInst->setOrient(instOrient);
    antennaInst->setLocation(instLocX + offset, instLocY);

    odb::dbBox* instBox = antennaInst->getBBox();
    box box(point(instBox->xMin() - (2 * siteWidth) + 1, instBox->yMin() + 1),
            point(instBox->xMax() + (2 * siteWidth) - 1, instBox->yMax() - 1));
    fixedInsts.query(bgi::intersects(box), std::back_inserter(overlapInsts));

    if (overlapInsts.size() == 0) {
      legallyPlaced = true;
    }
    overlapInsts.clear();
  }

  antennaInst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  sinkInst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  odb::dbITerm::connect(antennaITerm, net);

  // Add diode to the R-tree of fixed instances
  int fixedInstId = fixedInsts.size();
  odb::dbBox* instBox = antennaInst->getBBox();
  box b(point(instBox->xMin(), instBox->yMin()),
        point(instBox->xMax(), instBox->yMax()));
  value v(b, fixedInstId);
  fixedInsts.insert(v);
  fixedInstId++;
}

void DBWrapper::getFixedInstances(r_tree& fixedInsts)
{
  odb::dbTech* tech = _db->getTech();
  if (!tech) {
    error("obd::dbTech not initialized\n");
  }

  odb::dbBlock* block = _chip->getBlock();
  if (!block) {
    error("odb::dbBlock not found\n");
  }

  int fixedInstId = 0;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getPlacementStatus() == odb::dbPlacementStatus::FIRM) {
      odb::dbBox* instBox = inst->getBBox();
      box b(point(instBox->xMin(), instBox->yMin()),
            point(instBox->xMax(), instBox->yMax()));
      value v(b, fixedInstId);
      fixedInsts.insert(v);
      fixedInstId++;
    }
  }
}

void DBWrapper::fixAntennas(std::string antennaCellName,
                            std::string antennaPinName)
{
  odb::dbBlock* block = _chip->getBlock();
  int siteWidth = -1;
  int cnt = 0;
  r_tree fixedInsts;
  getFixedInstances(fixedInsts);

  auto rows = block->getRows();
  if (!rows.empty()) {
    for (odb::dbRow* db_row : rows) {
      odb::dbSite* site = db_row->getSite();
      int site_width = site->getWidth();
      if (siteWidth == -1) {
        siteWidth = site_width;
      }

      if (siteWidth != site_width) {
        std::cout << "[WARNING] Design has rows with different site width\n";
      }
    }
  }

  for (auto const& violation : antennaViolations) {
    odb::dbNet* net = dbNets[violation.first];
    for (int i = 0; i < violation.second.size(); i++) {
      for (odb::dbITerm* sinkITerm : violation.second[i].iterms) {
        odb::dbInst* sinkInst = sinkITerm->getInst();
        std::string antennaInstName = "ANTENNA_" + std::to_string(cnt);
        insertDiode(net,
                    antennaCellName,
                    antennaPinName,
                    sinkInst,
                    sinkITerm,
                    antennaInstName,
                    siteWidth,
                    fixedInsts);
        cnt++;
      }
    }
  }
}

void DBWrapper::legalizePlacedCells()
{
  _opendp = new opendp::Opendp();
  _opendp->init(_db);
  _opendp->setPaddingGlobal(2, 2);
  _opendp->detailedPlacement(0);
  _opendp->checkPlacement(false);
  delete _opendp;
}

}  // namespace FastRoute
