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
#include "opendb/db.h"
#include "openroad/Error.hh"
#include "openroad/OpenRoad.hh"

namespace ioPlacer {

using ord::error;

void IOPlacer::init(ord::OpenRoad* openroad)
{
  _openroad = openroad;
  makeComponents();

  _reportHPWL = false;
  _slotsPerSection = 200;
  _slotsIncreaseFactor = 0.01f;
  _usagePerSection = .8f;
  _usageIncreaseFactor = 0.01f;
  _forcePinSpread = true;
}

void IOPlacer::makeComponents()
{
  _parms = new Parameters;
  _db = _openroad->getDb();
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

void IOPlacer::initNetlistAndCore()
{
  populateIOPlacer();

  if (_parms->getBlockagesFile().size() != 0) {
    _blockagesFile = _parms->getBlockagesFile();
  }
}

void IOPlacer::initParms()
{
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
  if (_parms->getRandomMode() > -1) {
    _randomMode = (RandomMode) _parms->getRandomMode();
  }
  if (_forcePinSpread && (_randomMode > 0)) {
    std::cout << "WARNING: force pin spread option has no effect"
              << " when using random pin placement\n";
  }
}

void IOPlacer::randomPlacement(const RandomMode mode)
{
  const double seed = _parms->getRandSeed();

  int numIOs = _netlist.numIOPins();
  int numSlots = _slots.size();
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
    Section_t s = {Coordinate(0, 0)};
    _sections.push_back(s);
  }

  // MF @ 2020/03/09: Set the seed for std::random_shuffle
  srand(seed);
  //---

  switch (mode) {
    case RandomMode::Full:
      std::cout << "RandomMode Full\n";

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
        ioPin.setPos(_slots.at(b).pos);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vSlots.erase(vSlots.begin());
      });
      break;
    case RandomMode::Even:
      std::cout << "RandomMode Even\n";

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
        ioPin.setPos(_slots.at(floor(b * shift)).pos);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vIOs.erase(vIOs.begin());
      });
      break;
    case RandomMode::Group:
      std::cout << "RandomMode Group\n";
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
        ioPin.setPos(_slots.at(b).pos);
        _assignment.push_back(ioPin);
        _sections[0].net.addIONet(ioPin, instPins);
        vIOs.erase(vIOs.begin());
      });
      break;
    default:
      error("Random mode not found\n");
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

inline bool IOPlacer::checkBlocked(int currX, int currY)
{
  int blockedBeginX;
  int blockedBeginY;
  int blockedEndX;
  int blockedEndY;
  for (std::pair<Coordinate, Coordinate> blockage : _blockagesArea) {
    blockedBeginX = std::get<0>(blockage).getX();
    blockedBeginY = std::get<0>(blockage).getY();
    blockedEndX = std::get<0>(blockage).getX();
    blockedEndY = std::get<0>(blockage).getY();
    if (currX >= blockedBeginX)
      if (currY >= blockedBeginY)
        if (currX <= blockedEndX)
          if (currY <= blockedEndY)
            return true;
  }
  return false;
}

void IOPlacer::defineSlots()
{
  Coordinate lb = _core.getLowerBound();
  Coordinate ub = _core.getUpperBound();
  int lbX = lb.getX();
  int lbY = lb.getY();
  int ubX = ub.getX();
  int ubY = ub.getY();

  int minDstPinsX = _core.getMinDstPinsX() * _parms->getMinDistance();
  int minDstPinsY = _core.getMinDstPinsY() * _parms->getMinDistance();
  int initTracksX = _core.getInitTracksX();
  int initTracksY = _core.getInitTracksY();
  int numTracksX = _core.getNumTracksX();
  int numTracksY = _core.getNumTracksY();
  int offset = _parms->getBoundariesOffset();
  offset *= _core.getDatabaseUnit();

  int num_tracks_offset
      = std::ceil(offset / (std::max(minDstPinsX, minDstPinsY)));

  int totalNumSlots = 0;
  totalNumSlots += (ubX - lbX) * 2 / minDstPinsX;
  totalNumSlots += (ubY - lbY) * 2 / minDstPinsY;

  /*******************************************
   * How the for bellow follows core boundary *
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

  int start_idx, end_idx;
  int currX, currY;
  float thicknessMultiplierV = _parms->getVerticalThicknessMultiplier();
  float thicknessMultiplierH = _parms->getHorizontalThicknessMultiplier();
  int halfWidthX = int(ceil(_core.getMinWidthX() / 2.0)) * thicknessMultiplierV;
  int halfWidthY = int(ceil(_core.getMinWidthY() / 2.0)) * thicknessMultiplierH;

  std::vector<Coordinate> slotsEdge1;

  // For wider pins (when set_hor|ver_thick multiplier is used), a valid
  // slot is one that does not cause a part of the pin to lie outside
  // the die area (OffGrid violation):
  // (offset + k_start * pitch) - halfWidth >= lower_bound , where k_start is a
  // non-negative integer => start_idx is k_start (offset + k_end * pitch) +
  // halfWidth <= upper_bound, where k_end is a non-negative integer => end_idx
  // is k_end
  //     ^^^^^^^^ position of tracks(slots)

  start_idx
      = std::max(0.0,
                 ceil((lbX + halfWidthX - initTracksX) / minDstPinsX))
        + num_tracks_offset;
  end_idx
      = std::min((numTracksX - 1),
                 (int)floor((ubX - halfWidthX - initTracksX) / minDstPinsX))
        - num_tracks_offset;
  currX = initTracksX + start_idx * minDstPinsX;
  currY = lbY;
  for (int i = start_idx; i <= end_idx; ++i) {
    Coordinate pos(currX, currY);
    slotsEdge1.push_back(pos);
    currX += minDstPinsX;
  }

  std::vector<Coordinate> slotsEdge2;
  start_idx
      = std::max(0.0,
                 ceil((lbY + halfWidthY - initTracksY) / minDstPinsY))
        + num_tracks_offset;
  end_idx
      = std::min((numTracksY - 1),
                 (int)floor((ubY - halfWidthY - initTracksY) / minDstPinsY))
        - num_tracks_offset;
  currY = initTracksY + start_idx * minDstPinsY;
  currX = ubX;
  for (int i = start_idx; i <= end_idx; ++i) {
    Coordinate pos(currX, currY);
    slotsEdge2.push_back(pos);
    currY += minDstPinsY;
  }

  std::vector<Coordinate> slotsEdge3;
  start_idx
      = std::max(0.0,
                 ceil((lbX + halfWidthX - initTracksX) / minDstPinsX))
        + num_tracks_offset;
  end_idx
      = std::min((numTracksX - 1),
                 (int)floor((ubX - halfWidthX - initTracksX) / minDstPinsX))
        - num_tracks_offset;
  currX = initTracksX + start_idx * minDstPinsX;
  currY = ubY;
  for (int i = start_idx; i <= end_idx; ++i) {
    Coordinate pos(currX, currY);
    slotsEdge3.push_back(pos);
    currX += minDstPinsX;
  }
  std::reverse(slotsEdge3.begin(), slotsEdge3.end());

  std::vector<Coordinate> slotsEdge4;
  start_idx
      = std::max(0.0,
                 ceil((lbY + halfWidthY - initTracksY) / minDstPinsY))
        + num_tracks_offset;
  end_idx
      = std::min((numTracksY - 1),
                 (int)floor((ubY - halfWidthY - initTracksY) / minDstPinsY))
        - num_tracks_offset;
  currY = initTracksY + start_idx * minDstPinsY;
  currX = lbX;
  for (int i = start_idx; i <= end_idx; ++i) {
    Coordinate pos(currX, currY);
    slotsEdge4.push_back(pos);
    currY += minDstPinsY;
  }
  std::reverse(slotsEdge4.begin(), slotsEdge4.end());

  int i = 0;
  for (Coordinate pos : slotsEdge1) {
    currX = pos.getX();
    currY = pos.getY();
    bool blocked = checkBlocked(currX, currY);
    _slots.push_back({blocked, false, Coordinate(currX, currY)});
    i++;
  }

  for (Coordinate pos : slotsEdge2) {
    currX = pos.getX();
    currY = pos.getY();
    bool blocked = checkBlocked(currX, currY);
    _slots.push_back({blocked, false, Coordinate(currX, currY)});
    i++;
  }

  for (Coordinate pos : slotsEdge3) {
    currX = pos.getX();
    currY = pos.getY();
    bool blocked = checkBlocked(currX, currY);
    _slots.push_back({blocked, false, Coordinate(currX, currY)});
    i++;
  }

  for (Coordinate pos : slotsEdge4) {
    currX = pos.getX();
    currY = pos.getY();
    bool blocked = checkBlocked(currX, currY);
    _slots.push_back({blocked, false, Coordinate(currX, currY)});
    i++;
  }
}

void IOPlacer::createSections()
{
  slotVector_t& slots = _slots;
  _sections.clear();
  int numSlots = slots.size();
  int beginSlot = 0;
  int endSlot = 0;
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
      std::cout << "WARNING: section usage exeeded max\n";
      _usagePerSection = 1.;
      std::cout << "Forcing slots per section to increase\n";
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
      error("Negative number of slots\n");
    }
    nSec.beginSlot = beginSlot;
    nSec.endSlot = endSlot;
    nSec.maxSlots = nSec.numSlots * _usagePerSection;
    nSec.curSlots = 0;
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
      dst[i] = net.computeIONetHPWL(idx, sections[i].pos);
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
    std::cout << " > Successfully assigned I/O pins\n";
    return true;
  } else {
    std::cout << " > Unsuccessfully assigned I/O pins\n";
    return false;
  }
}

void IOPlacer::printConfig()
{
  std::cout << " * Num of slots          " << _slots.size() << "\n";
  std::cout << " * Num of I/O            " << _netlist.numIOPins() << "\n";
  std::cout << " * Num of I/O w/sink     " << _netlistIOPins.numIOPins()
            << "\n";
  std::cout << " * Num of I/O w/o sink   " << _zeroSinkIOs.size() << "\n";
  std::cout << " * Slots Per Section     " << _slotsPerSection << "\n";
  std::cout << " * Slots Increase Factor " << _slotsIncreaseFactor << "\n";
  std::cout << " * Usage Per Section     " << _usagePerSection << "\n";
  std::cout << " * Usage Increase Factor " << _usageIncreaseFactor << "\n";
  std::cout << " * Force Pin Spread      " << _forcePinSpread << "\n\n";
}

void IOPlacer::setupSections()
{
  bool allAssigned;
  int i = 0;
  if (!_forcePinSpread && _usageIncreaseFactor == 0.0f
      && _slotsIncreaseFactor == 0.0f) {
    std::cout << "WARNING: if _forcePinSpread = false than either "
                 "_usageIncreaseFactor or _slotsIncreaseFactor "
                 "must be != 0\n";
  }
  do {
    std::cout << "Tentative " << i++ << " to setup sections\n";
    printConfig();

    allAssigned = assignPinsSections();

    _usagePerSection *= (1 + _usageIncreaseFactor);
    _slotsPerSection *= (1 + _slotsIncreaseFactor);
    if (_sections.size() > MAX_SECTIONS_RECOMMENDED) {
      std::cout << "WARNING: number of sections is " << _sections.size()
                << " while the maximum recommended value is "
                << MAX_SECTIONS_RECOMMENDED
                << " this may negatively affect performance\n";
    }
    if (_slotsPerSection > MAX_SLOTS_RECOMMENDED) {
      std::cout << "WARNING: number of slots per sections is "
                << _slotsPerSection
                << " while the maximum recommended value is "
                << MAX_SLOTS_RECOMMENDED
                << " this may negatively affect performance\n";
    }
  } while (!allAssigned);
}

inline void IOPlacer::updateOrientation(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();
  int lowerXBound = _core.getLowerBound().getX();
  int lowerYBound = _core.getLowerBound().getY();
  int upperXBound = _core.getUpperBound().getX();
  int upperYBound = _core.getUpperBound().getY();

  if (x == lowerXBound) {
    if (y == upperYBound) {
      pin.setOrientation(Orientation::ORIENT_SOUTH);
      return;
    } else {
      pin.setOrientation(Orientation::ORIENT_EAST);
      return;
    }
  }
  if (x == upperXBound) {
    if (y == lowerYBound) {
      pin.setOrientation(Orientation::ORIENT_NORTH);
      return;
    } else {
      pin.setOrientation(Orientation::ORIENT_WEST);
      return;
    }
  }
  if (y == lowerYBound) {
    pin.setOrientation(Orientation::ORIENT_NORTH);
    return;
  }
  if (y == upperYBound) {
    pin.setOrientation(Orientation::ORIENT_SOUTH);
    return;
  }
}

inline void IOPlacer::updatePinArea(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();
  int lowerXBound = _core.getLowerBound().getX();
  int lowerYBound = _core.getLowerBound().getY();
  int upperXBound = _core.getUpperBound().getX();
  int upperYBound = _core.getUpperBound().getY();

  if (pin.getOrientation() == Orientation::ORIENT_NORTH
      || pin.getOrientation() == Orientation::ORIENT_SOUTH) {
    float thicknessMultiplier = _parms->getVerticalThicknessMultiplier();
    int halfWidth = int(ceil(_core.getMinWidthX() / 2.0)) * thicknessMultiplier;
    int height = int(std::max(2.0 * halfWidth,
                              ceil(_core.getMinAreaX() / (2.0 * halfWidth))));

    int ext = 0;
    if (_parms->getVerticalLength() != -1) {
      height = _parms->getVerticalLength() * _core.getDatabaseUnit();
    }

    if (_parms->getVerticalLengthExtend() != -1) {
      ext = _parms->getVerticalLengthExtend() * _core.getDatabaseUnit();
    }

    if (pin.getOrientation() == Orientation::ORIENT_NORTH) {
      pin.setLowerBound(pin.getX() - halfWidth, pin.getY() - ext);
      pin.setUpperBound(pin.getX() + halfWidth, pin.getY() + height);
    } else {
      pin.setLowerBound(pin.getX() - halfWidth, pin.getY() + ext);
      pin.setUpperBound(pin.getX() + halfWidth, pin.getY() - height);
    }
  }

  if (pin.getOrientation() == Orientation::ORIENT_WEST
      || pin.getOrientation() == Orientation::ORIENT_EAST) {
    float thicknessMultiplier = _parms->getHorizontalThicknessMultiplier();
    int halfWidth = int(ceil(_core.getMinWidthY() / 2.0)) * thicknessMultiplier;
    int height = int(std::max(2.0 * halfWidth,
                              ceil(_core.getMinAreaY() / (2.0 * halfWidth))));

    int ext = 0;
    if (_parms->getHorizontalLengthExtend() != -1) {
      ext = _parms->getHorizontalLengthExtend() * _core.getDatabaseUnit();
    }
    if (_parms->getHorizontalLength() != -1) {
      height = _parms->getHorizontalLength() * _core.getDatabaseUnit();
    }

    if (pin.getOrientation() == Orientation::ORIENT_EAST) {
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

void IOPlacer::addBlockedArea(int llx, int lly,
                              int urx, int ury)
{
  Coordinate lowerLeft = Coordinate(llx, lly);
  Coordinate upperRight = Coordinate(urx, ury);
  std::pair<Coordinate, Coordinate> blkArea
      = std::make_pair(lowerLeft, upperRight);

  _blockagesArea.push_back(blkArea);
}

void IOPlacer::run()
{
  initParms();

  std::cout << " > Running IO placement\n";

  if (_parms->getNumThreads() > 0) {
    // omp_set_dynamic(0);
    // omp_set_num_threads(_parms->getNumThreads());
    std::cout << " * User defines number of threads\n";
  }
  // std::cout << " * IOPlacer is using " << omp_get_max_threads()
  //           << " threads.\n";

  initNetlistAndCore();

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

  if (!_cellsPlaced || (_randomMode > 0)) {
    std::cout << "WARNING: running random pin placement\n";
    randomPlacement(_randomMode);
  } else {
    setupSections();

    for (int idx = 0; idx < _sections.size(); idx++) {
      if (_sections[idx].net.numIOPins() > 0) {
        HungarianMatching hg(_sections[idx], _slots);
        hgVec.push_back(hg);
      }
    }

    for (int idx = 0; idx < hgVec.size(); idx++) {
      hgVec[idx].run();
    }

    for (int idx = 0; idx < hgVec.size(); idx++) {
      hgVec[idx].getFinalAssignment(_assignment);
    }

    int i = 0;
    while (_zeroSinkIOs.size() > 0 && i < _slots.size()) {
      if (!_slots[i].used && !_slots[i].blocked) {
        _slots[i].used = true;
        _zeroSinkIOs[0].setPos(_slots[i].pos);
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
    error("Assigned %d pins out of %d IO pins\n", _assignment.size(), _netlist.numIOPins());
  }

  if (_reportHPWL) {
    for (int idx = 0; idx < _sections.size(); idx++) {
      totalHPWL += returnIONetsHPWL(_sections[idx].net);
    }
    deltaHPWL = initHPWL - totalHPWL;
    std::cout << "***HPWL before ioPlacer: " << initHPWL << "\n";
    std::cout << "***HPWL after  ioPlacer: " << totalHPWL << "\n";
    std::cout << "***HPWL delta  ioPlacer: " << deltaHPWL << "\n";
  }

  commitIOPlacementToDB(_assignment);
  std::cout << " > IO placement done.\n";
}

// db functions
void IOPlacer::populateIOPlacer()
{
  _tech = _db->getTech();
  _block = _db->getChip()->getBlock();
  initNetlist();
  initCore();
}

void IOPlacer::initCore()
{
  int databaseUnit = _tech->getLefUnits();

  odb::Rect rect;
  _block->getDieArea(rect);

  Coordinate lowerBound(rect.xMin(), rect.yMin());
  Coordinate upperBound(rect.xMax(), rect.yMax());

  int horLayerIdx = _parms->getHorizontalMetalLayer();
  int verLayerIdx = _parms->getVerticalMetalLayer();

  odb::dbTechLayer* horLayer = _tech->findRoutingLayer(horLayerIdx);
  odb::dbTechLayer* verLayer = _tech->findRoutingLayer(verLayerIdx);

  odb::dbTrackGrid* horTrackGrid = _block->findTrackGrid(horLayer);
  odb::dbTrackGrid* verTrackGrid = _block->findTrackGrid(verLayer);

  int minSpacingX = 0;
  int minSpacingY = 0;
  int initTrackX = 0;
  int initTrackY = 0;
  int minAreaX = 0;
  int minAreaY = 0;
  int minWidthX = 0;
  int minWidthY = 0;

  int numTracksX = 0;
  int numTracksY = 0;
  verTrackGrid->getGridPatternX(0, initTrackX, numTracksX, minSpacingX);
  horTrackGrid->getGridPatternY(0, initTrackY, numTracksY, minSpacingY);

  minAreaX = verLayer->getArea() * databaseUnit * databaseUnit;
  minWidthX = verLayer->getWidth();
  minAreaY = horLayer->getArea() * databaseUnit * databaseUnit;
  minWidthY = horLayer->getWidth();

  _core = Core(lowerBound,
                upperBound,
                minSpacingX,
                minSpacingY,
                initTrackX,
                initTrackY,
                numTracksX,
                numTracksY,
                minAreaX,
                minAreaY,
                minWidthX,
                minWidthY,
                databaseUnit);
  if (_verbose) {
    std::cout << "lowerBound: " << lowerBound.getX() << " " << lowerBound.getY()
              << "\n";
    std::cout << "upperBound: " << upperBound.getX() << " " << upperBound.getY()
              << "\n";
    std::cout << "minSpacingX: " << minSpacingX << "\n";
    std::cout << "minSpacingY: " << minSpacingY << "\n";
    std::cout << "initTrackX: " << initTrackX << "\n";
    std::cout << "initTrackY: " << initTrackY << "\n";
    std::cout << "numTracksX: " << numTracksX << "\n";
    std::cout << "numTracksY: " << numTracksY << "\n";
    std::cout << "minAreaX: " << minAreaX << "\n";
    std::cout << "minAreaY: " << minAreaY << "\n";
    std::cout << "minWidthX: " << minWidthX << "\n";
    std::cout << "minWidthY: " << minWidthY << "\n";
    std::cout << "databaseUnit: " << databaseUnit << "\n";
  }
}

void IOPlacer::initNetlist()
{
  odb::dbSet<odb::dbBTerm> bterms = _block->getBTerms();

  odb::dbSet<odb::dbBTerm>::iterator btIter;

  for (btIter = bterms.begin(); btIter != bterms.end(); ++btIter) {
    odb::dbBTerm* curBTerm = *btIter;
    odb::dbNet* net = curBTerm->getNet();
    if (!net) {
      std::cout << "[WARNING] Pin " << curBTerm->getConstName()
                << " without net!\n";
    }

    Direction dir = DIR_INOUT;
    switch (curBTerm->getIoType()) {
      case odb::dbIoType::INPUT:
        dir = DIR_IN;
        break;
      case odb::dbIoType::OUTPUT:
        dir = DIR_OUT;
        break;
      default:
        dir = DIR_INOUT;
    }

    int xPos = 0;
    int yPos = 0;
    curBTerm->getFirstPinLocation(xPos, yPos);

    Coordinate bounds(0, 0);
    IOPin ioPin(curBTerm->getConstName(),
                Coordinate(xPos, yPos),
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
          InstancePin(inst->getConstName(), Coordinate(instX, instY)));
    }

    _netlist.addIONet(ioPin, instPins);
  }
}

void IOPlacer::commitIOPlacementToDB(std::vector<IOPin>& assignment)
{
  int horLayerIdx = _parms->getHorizontalMetalLayer();
  int verLayerIdx = _parms->getVerticalMetalLayer();

  odb::dbTechLayer* horLayer = _tech->findRoutingLayer(horLayerIdx);

  odb::dbTechLayer* verLayer = _tech->findRoutingLayer(verLayerIdx);

  for (IOPin& pin : assignment) {
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

    Coordinate lowerBound = pin.getLowerBound();
    Coordinate upperBound = pin.getUpperBound();

    odb::dbBPin* bpin = odb::dbBPin::create(bterm);

    int size = upperBound.getX() - lowerBound.getX();

    int xMin = lowerBound.getX();
    int yMin = lowerBound.getY();
    int xMax = upperBound.getX();
    int yMax = upperBound.getY();
    odb::dbTechLayer* layer = verLayer;
    if (pin.getOrientation() == Orientation::ORIENT_EAST
        || pin.getOrientation() == Orientation::ORIENT_WEST) {
      layer = horLayer;
    }

    odb::dbBox::create(bpin, layer, xMin, yMin, xMax, yMax);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  };
}

}  // namespace ioPlacer
