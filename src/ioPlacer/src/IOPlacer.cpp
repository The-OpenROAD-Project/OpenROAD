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

#include "ioplacer/IOPlacer.h"

#include <random>
#include <algorithm>
#include "opendb/db.h"
#include "openroad/Logger.h"
#include "openroad/OpenRoad.hh"

namespace ppl {

using ord::PPL;

void IOPlacer::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  _logger = openroad->getLogger();
  makeComponents();
}

void IOPlacer::makeComponents()
{
  _parms = new Parameters;
  _db = _openroad->getDb();
}

void IOPlacer::clear()
{
  _horLayers.clear();
  _verLayers.clear();
  _zeroSinkIOs.clear();
  _sections.clear();
  _slots.clear();
  _assignment.clear();
  _netlistIOPins.clear();
  _excludedIntervals.clear();
  _constraints.clear();
  _netlist.clear();
}

void IOPlacer::deleteComponents()
{
  delete _parms;
  delete _db;
  delete _tech;
  delete _block;
}

IOPlacer::~IOPlacer()
{
  deleteComponents();
}

void IOPlacer::initNetlistAndCore(std::set<int> horLayerIdx, std::set<int> verLayerIdx)
{
  populateIOPlacer(horLayerIdx, verLayerIdx);
}

void IOPlacer::initParms()
{
  _reportHPWL = false;
  _slotsPerSection = 200;
  _slotsIncreaseFactor = 0.01f;
  _usagePerSection = .8f;
  _usageIncreaseFactor = 0.01f;
  _forcePinSpread = true;
  _netlist = Netlist();
  _netlistIOPins = Netlist();

  if (_parms->getReportHPWL()) {
    _reportHPWL = true;
  }
  if (_parms->getForceSpread()) {
    _forcePinSpread = true;
  } else {
    _forcePinSpread = false;
  }
  if (_parms->getNumSlots() > -1) {
    _slotsPerSection = _parms->getNumSlots();
  }

  if (_parms->getSlotsFactor() > -1) {
    _slotsIncreaseFactor = _parms->getSlotsFactor();
  }
  if (_parms->getUsage() > -1) {
    _usagePerSection = _parms->getUsage();
  }
  if (_parms->getUsageFactor() > -1) {
    _usageIncreaseFactor = _parms->getUsageFactor();
  }
}

void IOPlacer::randomPlacement(const RandomMode mode)
{
  const double seed = _parms->getRandSeed();

  slotVector_t validSlots;
  for (Slot_t &slot : _slots) {
    if (!slot.blocked) {
      validSlots.push_back(slot);
    }
  }

  int numIOs = _netlist.numIOPins();
  int numSlots = validSlots.size();
  double shift = numSlots / double(numIOs);
  int mid1 = numSlots * 1 / 8 - numIOs / 8;
  int mid2 = numSlots * 3 / 8 - numIOs / 8;
  int mid3 = numSlots * 5 / 8 - numIOs / 8;
  int mid4 = numSlots * 7 / 8 - numIOs / 8;
  int idx = 0;
  int slotsPerEdge = numIOs / 4;
  int lastSlots = (numIOs - slotsPerEdge * 3);
  std::vector<int> vSlots(numSlots);
  std::vector<int> vIOs(numIOs);

  std::vector<InstancePin> instPins;
  _netlist.forEachSinkOfIO(
      idx, [&](InstancePin& instPin) { instPins.push_back(instPin); });
  if (_sections.size() < 1) {
    Section_t s = {Point(0, 0)};
    _sections.push_back(s);
  }

  // MF @ 2020/03/09: Set the seed for std::random_shuffle
  srand(seed);
  //---

  switch (mode) {
    case RandomMode::Full:
      _logger->report("RandomMode Full");

      for (size_t i = 0; i < vSlots.size(); ++i) {
        vSlots[i] = i;
      }

      // MF @ 2020/03/09: std::shuffle produces different results
      // between gccs 4.8.x and 8.5.x
      // std::random_shuffle is deterministic across versions
      std::random_shuffle(vSlots.begin(), vSlots.end());
      // ---

      _netlist.forEachIOPin([&](int idx, IOPin& ioPin) {
        int b = vSlots[0];
        ioPin.setPos(validSlots.at(b).pos);
        ioPin.setLayer(validSlots.at(b).layer);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vSlots.erase(vSlots.begin());
      });
      break;
    case RandomMode::Even:
      _logger->report("RandomMode Even");

      for (size_t i = 0; i < vIOs.size(); ++i) {
        vIOs[i] = i;
      }

      // MF @ 2020/03/09: std::shuffle produces different results
      // between gccs 4.8.x and 8.5.x
      // std::random_shuffle is deterministic across versions
      std::random_shuffle(vIOs.begin(), vIOs.end());
      // ---

      _netlist.forEachIOPin([&](int idx, IOPin& ioPin) {
        int b = vIOs[0];
        ioPin.setPos(validSlots.at(floor(b * shift)).pos);
        ioPin.setLayer(validSlots.at(floor(b * shift)).layer);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vIOs.erase(vIOs.begin());
      });
      break;
    case RandomMode::Group:
      _logger->report("RandomMode Group");
      for (size_t i = mid1; i < mid1 + slotsPerEdge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid2; i < mid2 + slotsPerEdge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid3; i < mid3 + slotsPerEdge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid4; i < mid4 + lastSlots; i++) {
        vIOs[idx++] = i;
      }

      // MF @ 2020/03/09: std::shuffle produces different results
      // between gccs 4.8.x and 8.5.x
      // std::random_shuffle is deterministic across versions
      std::random_shuffle(vIOs.begin(), vIOs.end());
      // ---

      _netlist.forEachIOPin([&](int idx, IOPin& ioPin) {
        int b = vIOs[0];
        ioPin.setPos(validSlots.at(b).pos);
        ioPin.setLayer(validSlots.at(b).layer);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vIOs.erase(vIOs.begin());
      });
      break;
    default:
      _logger->error(PPL, 1, "Random mode not found");
      break;
  }
}

void IOPlacer::initIOLists()
{
  _netlist.forEachIOPin([&](int idx, IOPin& ioPin) {
    std::vector<InstancePin> instPinsVector;
    if (_netlist.numSinksOfIO(idx) != 0) {
      _netlist.forEachSinkOfIO(idx, [&](InstancePin& instPin) {
        instPinsVector.push_back(instPin);
      });
      _netlistIOPins.addIONet(ioPin, instPinsVector);
    } else {
      _zeroSinkIOs.push_back(ioPin);
    }
  });
}

bool IOPlacer::checkBlocked(Edge edge, int pos)
{
  for (Interval blockedInterval : _excludedIntervals) {
    if (blockedInterval.getEdge() == edge &&
        pos >= blockedInterval.getBegin() &&
        pos <= blockedInterval.getEnd()) {
      return true;
    }
  }

  return false;
}

void IOPlacer::findSlots(const std::set<int>& layers, Edge edge)
{
  Point lb = _core.getBoundary().ll();
  Point ub = _core.getBoundary().ur();

  int lbX = lb.x();
  int lbY = lb.y();
  int ubX = ub.x();
  int ubY = ub.y();

  bool vertical = (edge == Edge::Top || edge == Edge::Bottom);
  int min = vertical ? lbX : lbY;
  int max = vertical ? ubX : ubY;

  int offset = _parms->getBoundariesOffset() * _core.getDatabaseUnit();

  int i = 0;
  for (int layer : layers) {
    int currX, currY, start_idx, end_idx;

    int minDstPins = vertical ? _core.getMinDstPinsX()[i] * _parms->getMinDistance()
                              : _core.getMinDstPinsY()[i] * _parms->getMinDistance();
    int initTracks = vertical ? _core.getInitTracksX()[i]
                              : _core.getInitTracksY()[i];
    int numTracks = vertical ? _core.getNumTracksX()[i]
                             : _core.getNumTracksY()[i];

    float thicknessMultiplier = vertical ? _parms->getVerticalThicknessMultiplier()
                                         : _parms->getHorizontalThicknessMultiplier();

    int halfWidth = vertical ? int(ceil(_core.getMinWidthX()[i] / 2.0))
                             : int(ceil(_core.getMinWidthY()[i] / 2.0));

    halfWidth *= thicknessMultiplier;

    int num_tracks_offset
      = std::ceil(offset / (std::max(_core.getMinDstPinsX()[i] * _parms->getMinDistance(),
                                     _core.getMinDstPinsY()[i] * _parms->getMinDistance())));

    start_idx
        = std::max(0.0,
                   ceil((min + halfWidth - initTracks) / minDstPins))
          + num_tracks_offset;
    end_idx
        = std::min((numTracks - 1),
                   (int)floor((max - halfWidth - initTracks) / minDstPins))
          - num_tracks_offset;
    if (vertical) {
      currX = initTracks + start_idx * minDstPins;
      currY = (edge == Edge::Bottom) ? lbY : ubY;
    } else {
      currY = initTracks + start_idx * minDstPins;
      currX = (edge == Edge::Left) ? lbX : ubX;
    }

    std::vector<Point> slots;
    for (int i = start_idx; i <= end_idx; ++i) {
      Point pos(currX, currY);
      slots.push_back(pos);
      if (vertical) {
        currX += minDstPins;
      } else {
        currY += minDstPins;
      }
    }

    if (edge == Edge::Top || edge == Edge::Left) {
      std::reverse(slots.begin(), slots.end());
    }

    for (Point pos : slots) {
      currX = pos.getX();
      currY = pos.getY();
      bool blocked = vertical ? checkBlocked(edge, currX) : checkBlocked(edge, currY);
      _slots.push_back({blocked, false, Point(currX, currY), layer});
    }
    i++;
  }
}

void IOPlacer::defineSlots()
{
  Point lb = _core.getBoundary().ll();
  Point ub = _core.getBoundary().ur();
  int lbX = lb.x();
  int lbY = lb.y();
  int ubX = ub.x();
  int ubY = ub.y();

  int offset = _parms->getBoundariesOffset();
  offset *= _core.getDatabaseUnit();

  /*******************************************
   *  Order of the edges when creating slots  *
   ********************************************
   *                 <----                    *
   *                                          *
   *                 3st edge     upperBound  *
   *           *------------------x           *
   *           |                  |           *
   *   |       |                  |      ^    *
   *   |  4th  |                  | 2nd  |    *
   *   |  edge |                  | edge |    *
   *   V       |                  |      |    *
   *           |                  |           *
   *           x------------------*           *
   *   lowerBound    1st edge                 *
   *                 ---->                    *
   *******************************************/

  // For wider pins (when set_hor|ver_thick multiplier is used), a valid
  // slot is one that does not cause a part of the pin to lie outside
  // the die area (OffGrid violation):
  // (offset + k_start * pitch) - halfWidth >= lower_bound , where k_start is a
  // non-negative integer => start_idx is k_start (offset + k_end * pitch) +
  // halfWidth <= upper_bound, where k_end is a non-negative integer => end_idx
  // is k_end
  //     ^^^^^^^^ position of tracks(slots)

  findSlots(_verLayers, Edge::Bottom);

  findSlots(_horLayers, Edge::Right);

  findSlots(_verLayers, Edge::Top);

  findSlots(_horLayers, Edge::Left);
}

void IOPlacer::createSections()
{
  Point lb = _core.getBoundary().ll();
  Point ub = _core.getBoundary().ur();

  slotVector_t& slots = _slots;
  _sections.clear();
  int numSlots = slots.size();
  int beginSlot = 0;
  int endSlot = 0;

  int slotsPerEdge = numSlots/_slotsPerSection;
  if (slotsPerEdge < 4) {
    _slotsPerSection = numSlots / 4;
    _logger->warn(PPL, 7, "Redefining the number of slots per section to have at least one section per edge");
  }

  while (endSlot < numSlots) {
    int blockedSlots = 0;
    endSlot = beginSlot + _slotsPerSection - 1;
    if (endSlot > numSlots) {
      endSlot = numSlots;
    }
    for (int i = beginSlot; i < endSlot; ++i) {
      if (slots[i].blocked) {
        blockedSlots++;
      }
    }
    int midPoint = (endSlot - beginSlot) / 2;
    Section_t nSec = {slots.at(beginSlot + midPoint).pos};
    if (_usagePerSection > 1.f) {
      _logger->warn(PPL, 10, "section usage exeeded max");
      _usagePerSection = 1.;
      _logger->report("Forcing slots per section to increase");
      if (_slotsIncreaseFactor != 0.0f) {
        _slotsPerSection *= (1 + _slotsIncreaseFactor);
      } else if (_usageIncreaseFactor != 0.0f) {
        _slotsPerSection *= (1 + _usageIncreaseFactor);
      } else {
        _slotsPerSection *= 1.1;
      }
    }
    nSec.numSlots = endSlot - beginSlot - blockedSlots;
    if (nSec.numSlots < 0) {
      _logger->error(PPL, 15, "Negative number of slots");
    }
    nSec.beginSlot = beginSlot;
    nSec.endSlot = endSlot;
    nSec.maxSlots = nSec.numSlots * _usagePerSection;
    nSec.curSlots = 0;
    if (nSec.pos.x() == lb.x()) {
      nSec.edge = Edge::Left;
    } else if (nSec.pos.x() == ub.x()) {
      nSec.edge = Edge::Right;
    } else if (nSec.pos.y() == lb.y()) {
      nSec.edge = Edge::Bottom;
    } else if (nSec.pos.y() == ub.y()) {
      nSec.edge = Edge::Top;
    }
    _sections.push_back(nSec);
    beginSlot = ++endSlot;
  }
}

bool IOPlacer::assignPinsSections()
{
  Netlist& net = _netlistIOPins;
  sectionVector_t& sections = _sections;
  createSections();
  int totalPinsAssigned = 0;
  net.forEachIOPin([&](int idx, IOPin& ioPin) {
    bool pinAssigned = false;
    std::vector<int> dst(sections.size());
    std::vector<InstancePin> instPinsVector;
    for (int i = 0; i < sections.size(); i++) {
      dst[i] = net.computeIONetHPWL(idx, sections[i].pos, sections[i].edge, _constraints);
    }
    net.forEachSinkOfIO(
        idx, [&](InstancePin& instPin) { instPinsVector.push_back(instPin); });
    for (auto i : sort_indexes(dst)) {
      if (sections[i].curSlots < sections[i].maxSlots) {
        sections[i].net.addIONet(ioPin, instPinsVector);
        sections[i].curSlots++;
        pinAssigned = true;
        totalPinsAssigned++;
        break;
      }
      // Try to add pin just to first
      if (!_forcePinSpread)
        break;
    }
    if (!pinAssigned) {
      return;  // "break" forEachIOPin
    }
  });
  // if forEachIOPin ends or returns/breaks goes here
  if (totalPinsAssigned == net.numIOPins()) {
    _logger->report("Successfully assigned I/O pins");
    return true;
  } else {
    _logger->report("Unsuccessfully assigned I/O pins");
    return false;
  }
}

void IOPlacer::printConfig()
{
  _logger->info(PPL, 1, " * Num of slots          {}", _slots.size());
  _logger->info(PPL, 2, " * Num of I/O            {}", _netlist.numIOPins());
  _logger->info(PPL, 3, " * Num of I/O w/sink     {}", _netlistIOPins.numIOPins());
  _logger->info(PPL, 4, " * Num of I/O w/o sink   {}", _zeroSinkIOs.size());
  _logger->info(PPL, 5, " * Slots Per Section     {}", _slotsPerSection);
  _logger->info(PPL, 6, " * Slots Increase Factor {}", _slotsIncreaseFactor);
  _logger->info(PPL, 7, " * Usage Per Section     {}", _usagePerSection);
  _logger->info(PPL, 8, " * Usage Increase Factor {}", _usageIncreaseFactor);
  _logger->info(PPL, 9, " * Force Pin Spread      {}", _forcePinSpread);
}

void IOPlacer::setupSections()
{
  bool allAssigned;
  int i = 0;

  do {
    _logger->info(PPL, 10, "Tentative {} to setup sections", i++);
    printConfig();

    allAssigned = assignPinsSections();

    _usagePerSection *= (1 + _usageIncreaseFactor);
    _slotsPerSection *= (1 + _slotsIncreaseFactor);
    if (_sections.size() > MAX_SECTIONS_RECOMMENDED) {
      _logger->warn(PPL, 15, "Number of sections is {}"
           " while the maximum recommended value is {}"
           " this may negatively affect performance",
           _sections.size(), MAX_SECTIONS_RECOMMENDED);
    }
    if (_slotsPerSection > MAX_SLOTS_RECOMMENDED) {
      _logger->warn(PPL, 15, "Number of slots per sections is {}"
           " while the maximum recommended value is {}"
           " this may negatively affect performance",
           _slotsPerSection, MAX_SLOTS_RECOMMENDED);
    }
  } while (!allAssigned);
}

void IOPlacer::updateOrientation(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();
  int lowerXBound = _core.getBoundary().ll().x();
  int lowerYBound = _core.getBoundary().ll().y();
  int upperXBound = _core.getBoundary().ur().x();
  int upperYBound = _core.getBoundary().ur().y();

  if (x == lowerXBound) {
    if (y == upperYBound) {
      pin.setOrientation(Orientation::South);
      return;
    } else {
      pin.setOrientation(Orientation::East);
      return;
    }
  }
  if (x == upperXBound) {
    if (y == lowerYBound) {
      pin.setOrientation(Orientation::North);
      return;
    } else {
      pin.setOrientation(Orientation::West);
      return;
    }
  }
  if (y == lowerYBound) {
    pin.setOrientation(Orientation::North);
    return;
  }
  if (y == upperYBound) {
    pin.setOrientation(Orientation::South);
    return;
  }
}

void IOPlacer::updatePinArea(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();
  const int l = pin.getLayer();
  int lowerXBound = _core.getBoundary().ll().x();
  int lowerYBound = _core.getBoundary().ll().y();
  int upperXBound = _core.getBoundary().ur().x();
  int upperYBound = _core.getBoundary().ur().y();

  int index;

  int i = 0;
  for (int layer : _horLayers) {
    if (layer == pin.getLayer())
      index = i;
    i++;
  }

  i = 0;
  for (int layer : _verLayers) {
    if (layer == pin.getLayer())
      index = i;
    i++;
  }

  if (pin.getOrientation() == Orientation::North
      || pin.getOrientation() == Orientation::South) {
    float thicknessMultiplier = _parms->getVerticalThicknessMultiplier();
    int halfWidth = int(ceil(_core.getMinWidthX()[index] / 2.0)) * thicknessMultiplier;
    int height = int(std::max(2.0 * halfWidth,
                              ceil(_core.getMinAreaX()[index] / (2.0 * halfWidth))));

    int ext = 0;
    if (_parms->getVerticalLength() != -1) {
      height = _parms->getVerticalLength() * _core.getDatabaseUnit();
    }

    if (_parms->getVerticalLengthExtend() != -1) {
      ext = _parms->getVerticalLengthExtend() * _core.getDatabaseUnit();
    }

    if (pin.getOrientation() == Orientation::North) {
      pin.setLowerBound(pin.getX() - halfWidth, pin.getY() - ext);
      pin.setUpperBound(pin.getX() + halfWidth, pin.getY() + height);
    } else {
      pin.setLowerBound(pin.getX() - halfWidth, pin.getY() + ext);
      pin.setUpperBound(pin.getX() + halfWidth, pin.getY() - height);
    }
  }

  if (pin.getOrientation() == Orientation::West
      || pin.getOrientation() == Orientation::East) {
    float thicknessMultiplier = _parms->getHorizontalThicknessMultiplier();
    int halfWidth = int(ceil(_core.getMinWidthY()[index] / 2.0)) * thicknessMultiplier;
    int height = int(std::max(2.0 * halfWidth,
                              ceil(_core.getMinAreaY()[index] / (2.0 * halfWidth))));

    int ext = 0;
    if (_parms->getHorizontalLengthExtend() != -1) {
      ext = _parms->getHorizontalLengthExtend() * _core.getDatabaseUnit();
    }
    if (_parms->getHorizontalLength() != -1) {
      height = _parms->getHorizontalLength() * _core.getDatabaseUnit();
    }

    if (pin.getOrientation() == Orientation::East) {
      pin.setLowerBound(pin.getX() - ext, pin.getY() - halfWidth);
      pin.setUpperBound(pin.getX() + height, pin.getY() + halfWidth);
    } else {
      pin.setLowerBound(pin.getX() - height, pin.getY() - halfWidth);
      pin.setUpperBound(pin.getX() + ext, pin.getY() + halfWidth);
    }
  }
}

int IOPlacer::returnIONetsHPWL(Netlist& netlist)
{
  int pinIndex = 0;
  int hpwl = 0;
  netlist.forEachIOPin([&](int idx, IOPin& ioPin) {
    hpwl += netlist.computeIONetHPWL(idx, ioPin.getPosition());
    pinIndex++;
  });

  return hpwl;
}

int IOPlacer::returnIONetsHPWL()
{
  return returnIONetsHPWL(_netlist);
}

void IOPlacer::excludeInterval(Edge edge, int begin, int end)
{
  Interval excludedInterv
      = Interval(edge, begin, end);

  _excludedIntervals.push_back(excludedInterv);
}

void IOPlacer::addDirectionConstraint(Direction direction, Edge edge,
                                       int begin, int end) {
  Interval interval(edge, begin, end);
  Constraint constraint("INVALID", direction, interval);
  _constraints.push_back(constraint);
}

void IOPlacer::addNameConstraint(std::string name, Edge edge,
                                       int begin, int end) {
  Interval interval(edge, begin, end);
  Constraint constraint(name, Direction::Invalid, interval);
  _constraints.push_back(constraint);
}

Edge IOPlacer::getEdge(std::string edge) {
  if (edge == "top") {
    return Edge::Top;
  } else if (edge == "bottom") {
    return Edge::Bottom;
  } else if (edge == "left") {
    return Edge::Left;
  } else {
    return Edge::Right;
  }

  return Edge::Invalid;
}

Direction IOPlacer::getDirection(std::string direction) {
  if (direction == "input") {
    return Direction::Input;
  } else if (direction == "output") {
    return Direction::Output;
  } else if (direction == "inout") {
    return Direction::Inout;
  } else {
    return Direction::Feedthru;
  }

  return Direction::Invalid;
}

void IOPlacer::run(bool randomMode)
{
  initParms();

  _logger->report("Running IO placement");

  initNetlistAndCore(_horLayers, _verLayers);

  std::vector<HungarianMatching> hgVec;
  int initHPWL = 0;
  int totalHPWL = 0;
  int deltaHPWL = 0;

  initIOLists();
  defineSlots();

  printConfig();

  if (_reportHPWL) {
    initHPWL = returnIONetsHPWL(_netlist);
  }

  if (!_cellsPlaced || randomMode) {
    _logger->report("Random pin placement");
    randomPlacement(RandomMode::Even);
  } else {
    setupSections();

    for (int idx = 0; idx < _sections.size(); idx++) {
      if (_sections[idx].net.numIOPins() > 0) {
        HungarianMatching hg(_sections[idx], _slots, _logger);
        hgVec.push_back(hg);
      }
    }

    for (int idx = 0; idx < hgVec.size(); idx++) {
      hgVec[idx].findAssignment(_constraints);
    }

    for (int idx = 0; idx < hgVec.size(); idx++) {
      hgVec[idx].getFinalAssignment(_assignment);
    }

    int i = 0;
    while (_zeroSinkIOs.size() > 0 && i < _slots.size()) {
      if (!_slots[i].used && !_slots[i].blocked) {
        _slots[i].used = true;
        _zeroSinkIOs[0].setPos(_slots[i].pos);
        _zeroSinkIOs[0].setLayer(_slots[i].layer);
        _assignment.push_back(_zeroSinkIOs[0]);
        _zeroSinkIOs.erase(_zeroSinkIOs.begin());
      }
      i++;
    }
  }
  for (int i = 0; i < _assignment.size(); ++i) {
    updateOrientation(_assignment[i]);
    updatePinArea(_assignment[i]);
  }

  if (_assignment.size() != (int) _netlist.numIOPins()) {
    _logger->error(PPL, 20, "Assigned {} pins out of {} IO pins", _assignment.size(), _netlist.numIOPins());
  }

  if (_reportHPWL) {
    for (int idx = 0; idx < _sections.size(); idx++) {
      totalHPWL += returnIONetsHPWL(_sections[idx].net);
    }
    deltaHPWL = initHPWL - totalHPWL;
    _logger->info(PPL, 11, "***HPWL before ioPlacer: {}", initHPWL);
    _logger->info(PPL, 11, "***HPWL after  ioPlacer: {}", totalHPWL);
    _logger->info(PPL, 11, "***HPWL delta  ioPlacer: {}", deltaHPWL);
  }

  commitIOPlacementToDB(_assignment);
  _logger->report("IO placement done.");
}

// db functions
void IOPlacer::populateIOPlacer(std::set<int> horLayerIdx, std::set<int> verLayerIdx)
{
  _tech = _db->getTech();
  _block = _db->getChip()->getBlock();
  initNetlist();
  initCore(horLayerIdx, verLayerIdx);
}

void IOPlacer::initCore(std::set<int> horLayerIdxs, std::set<int> verLayerIdxs)
{
  int databaseUnit = _tech->getLefUnits();

  Rect boundary;
  _block->getDieArea(boundary);

  std::vector<int> minSpacingsX;
  std::vector<int> minSpacingsY;
  std::vector<int> initTracksX;
  std::vector<int> initTracksY;
  std::vector<int> minAreasX;
  std::vector<int> minAreasY;
  std::vector<int> minWidthsX;
  std::vector<int> minWidthsY;
  std::vector<int> numTracksX;
  std::vector<int> numTracksY;

  for (int horLayerIdx : horLayerIdxs) {
    int minSpacingY = 0;
    int initTrackY = 0;
    int minAreaY = 0;
    int minWidthY = 0;
    int numTrackY = 0;

    odb::dbTechLayer* horLayer = _tech->findRoutingLayer(horLayerIdx);
    odb::dbTrackGrid* horTrackGrid = _block->findTrackGrid(horLayer);
    horTrackGrid->getGridPatternY(0, initTrackY, numTrackY, minSpacingY);

    minAreaY = horLayer->getArea() * databaseUnit * databaseUnit;
    minWidthY = horLayer->getWidth();

    minSpacingsY.push_back(minSpacingY);
    initTracksY.push_back(initTrackY);
    minAreasY.push_back(minAreaY);
    minWidthsY.push_back(minWidthY);
    numTracksY.push_back(numTrackY);
  }

  for (int verLayerIdx : verLayerIdxs) {
    int minSpacingX = 0;
    int initTrackX = 0;
    int minAreaX = 0;
    int minWidthX = 0;
    int numTrackX = 0;

    odb::dbTechLayer* verLayer = _tech->findRoutingLayer(verLayerIdx); 
    odb::dbTrackGrid* verTrackGrid = _block->findTrackGrid(verLayer);
    verTrackGrid->getGridPatternX(0, initTrackX, numTrackX, minSpacingX);

    minAreaX = verLayer->getArea() * databaseUnit * databaseUnit;
    minWidthX = verLayer->getWidth();

    minSpacingsX.push_back(minSpacingX);
    initTracksX.push_back(initTrackX);
    minAreasX.push_back(minAreaX);
    minWidthsX.push_back(minWidthX);
    numTracksX.push_back(numTrackX);
  }

  _core = Core(boundary,
                minSpacingsX,
                minSpacingsY,
                initTracksX,
                initTracksY,
                numTracksX,
                numTracksY,
                minAreasX,
                minAreasY,
                minWidthsX,
                minWidthsY,
                databaseUnit);
}

void IOPlacer::initNetlist()
{
  odb::dbSet<odb::dbBTerm> bterms = _block->getBTerms();

  odb::dbSet<odb::dbBTerm>::iterator btIter;

  for (btIter = bterms.begin(); btIter != bterms.end(); ++btIter) {
    odb::dbBTerm* curBTerm = *btIter;
    odb::dbNet* net = curBTerm->getNet();
    if (!net) {
      _logger->warn(PPL, 5, "Pin {} without net!",
           curBTerm->getConstName());
    }

    Direction dir = Direction::Inout;
    switch (curBTerm->getIoType()) {
      case odb::dbIoType::INPUT:
        dir = Direction::Input;
        break;
      case odb::dbIoType::OUTPUT:
        dir = Direction::Output;
        break;
      default:
        dir = Direction::Inout;
    }

    int xPos = 0;
    int yPos = 0;
    curBTerm->getFirstPinLocation(xPos, yPos);

    Point bounds(0, 0);
    IOPin ioPin(curBTerm->getConstName(),
                Point(xPos, yPos),
                dir,
                bounds,
                bounds,
                net->getConstName(),
                "FIXED");

    std::vector<InstancePin> instPins;
    odb::dbSet<odb::dbITerm> iterms = net->getITerms();
    odb::dbSet<odb::dbITerm>::iterator iIter;
    for (iIter = iterms.begin(); iIter != iterms.end(); ++iIter) {
      odb::dbITerm* curITerm = *iIter;
      odb::dbInst* inst = curITerm->getInst();
      int instX = 0, instY = 0;
      inst->getLocation(instX, instY);

      instPins.push_back(
          InstancePin(inst->getConstName(), Point(instX, instY)));
    }

    _netlist.addIONet(ioPin, instPins);
  }
}

void IOPlacer::commitIOPlacementToDB(std::vector<IOPin>& assignment)
{
  for (IOPin& pin : assignment) {
    odb::dbTechLayer* layer = _tech->findRoutingLayer(pin.getLayer());

    odb::dbBTerm* bterm = _block->findBTerm(pin.getName().c_str());
    odb::dbSet<odb::dbBPin> bpins = bterm->getBPins();
    odb::dbSet<odb::dbBPin>::iterator bpinIter;
    std::vector<odb::dbBPin*> allBPins;
    for (bpinIter = bpins.begin(); bpinIter != bpins.end(); ++bpinIter) {
      odb::dbBPin* curBPin = *bpinIter;
      allBPins.push_back(curBPin);
    }

    for (odb::dbBPin* bpin : allBPins) {
      odb::dbBPin::destroy(bpin);
    }

    Point lowerBound = pin.getLowerBound();
    Point upperBound = pin.getUpperBound();

    odb::dbBPin* bpin = odb::dbBPin::create(bterm);

    int size = upperBound.x() - lowerBound.x();

    int xMin = lowerBound.x();
    int yMin = lowerBound.y();
    int xMax = upperBound.x();
    int yMax = upperBound.y();

    odb::dbBox::create(bpin, layer, xMin, yMin, xMax, yMax);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}

}  // namespace ppl
