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

#include "TechChar.h"
#include "utl/Logger.h"

#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Units.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "rsz/Resizer.hh"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace cts {

using utl::CTS;

void TechChar::compileLut(std::vector<TechChar::ResultData> lutSols)
{
  _logger->info(CTS, 84, "Compiling LUT");
  initLengthUnits();

  _minSegmentLength = toInternalLengthUnit(_minSegmentLength);
  _maxSegmentLength = toInternalLengthUnit(_maxSegmentLength);

  reportCharacterizationBounds();  // min and max values already set
  checkCharacterizationBounds();

  unsigned noSlewDegradationCount = 0;
  _actualMinInputCap = std::numeric_limits<unsigned>::max();
  // For the results in each wire segment...
  for (ResultData lutLine : lutSols) {
    _actualMinInputCap
        = std::min(static_cast<unsigned>(lutLine.totalcap), _actualMinInputCap);
    // Checks the output slew of the wiresegment.
    if (lutLine.isPureWire && lutLine.pinSlew <= lutLine.inSlew) {
      ++noSlewDegradationCount;
      ++lutLine.pinSlew;
    }

    unsigned length = toInternalLengthUnit(lutLine.wirelength);

    WireSegment& segment = createWireSegment((unsigned) (length),
                                             (unsigned) (lutLine.load),
                                             (unsigned) (lutLine.pinSlew),
                                             lutLine.totalPower,
                                             (unsigned) (lutLine.pinArrival),
                                             (unsigned) (lutLine.totalcap),
                                             (unsigned) (lutLine.inSlew));

    if (!(lutLine.isPureWire)) {
      // Goes through the topology of the wiresegment and defines the buffer
      // locations and masters.
      int maxIndex = 0;
      if (lutLine.topology.size() % 2 == 0) {
        maxIndex = lutLine.topology.size();
      } else {
        maxIndex = lutLine.topology.size() - 1;
      }
      for (int topologyIndex = 0; topologyIndex < maxIndex; topologyIndex++) {
        std::string topologyS = lutLine.topology[topologyIndex];
        // Each buffered topology always has a wire segment followed by a
        // buffer.
        if (_masterNames.find(topologyS) == _masterNames.end()) {
          // Is a number (i.e. a wire segment).
          segment.addBuffer(std::stod(topologyS));
        } else {
          segment.addBufferMaster(topologyS);
        }
      }
    }
  }

  if (noSlewDegradationCount > 0) {
    _logger->warn(CTS, 43, "{} wires are pure wire and no slew degration.\n"
                  "TritonCTS forced slew degradation on these wires.",
                  noSlewDegradationCount);
  }

  _logger->info(CTS, 46, "    Number of wire segments: {}",
                _wireSegments.size());
  _logger->info(CTS, 47, "    Number of keys in characterization LUT: {}",
                _keyToWireSegments.size());

  _logger->info(CTS, 48, "    Actual min input cap: {}",
                _actualMinInputCap);
}

void TechChar::initLengthUnits()
{
  _charLengthUnit = _options->getWireSegmentUnit();
  _lengthUnit = LENGTH_UNIT_MICRON;
  _lengthUnitRatio = _charLengthUnit / _lengthUnit;
}

inline void TechChar::reportCharacterizationBounds() const
{
  _logger->report("Min. len    Max. len    Min. cap    Max. cap    Min. slew   Max. slew");

  _logger->report("{:<12}{:<12}{:<12}{:<12}{:<12}{:<12}",
                  _minSegmentLength, _maxSegmentLength, 
                  _minCapacitance, _maxCapacitance, _minSlew, _maxSlew);
}

inline void TechChar::checkCharacterizationBounds() const
{
  if (_minSegmentLength > MAX_NORMALIZED_VAL
      || _maxSegmentLength > MAX_NORMALIZED_VAL
      || _minCapacitance > MAX_NORMALIZED_VAL
      || _maxCapacitance > MAX_NORMALIZED_VAL || _minSlew > MAX_NORMALIZED_VAL
      || _maxSlew > MAX_NORMALIZED_VAL) {
    _logger->error(CTS, 65, "Normalized values in the LUT should be in the range [1, {}"
                   "\n Check the table above to see the normalization ranges and check "
                   "your characterization configuration.", std::to_string(MAX_NORMALIZED_VAL));
  }
}

inline WireSegment& TechChar::createWireSegment(uint8_t length,
                                                uint8_t load,
                                                uint8_t outputSlew,
                                                double power,
                                                unsigned delay,
                                                uint8_t inputCap,
                                                uint8_t inputSlew)
{
  _wireSegments.emplace_back(
      length, load, outputSlew, power, delay, inputCap, inputSlew);

  unsigned segmentIdx = _wireSegments.size() - 1;
  unsigned key = computeKey(length, load, outputSlew);

  if (_keyToWireSegments.find(key) == _keyToWireSegments.end()) {
    _keyToWireSegments[key] = std::deque<unsigned>();
  }

  _keyToWireSegments[key].push_back(segmentIdx);

  return _wireSegments.back();
}

void TechChar::forEachWireSegment(
    const std::function<void(unsigned, const WireSegment&)> func) const
{
  for (unsigned idx = 0; idx < _wireSegments.size(); ++idx) {
    func(idx, _wireSegments[idx]);
  }
};

void TechChar::forEachWireSegment(
    uint8_t length,
    uint8_t load,
    uint8_t outputSlew,
    const std::function<void(unsigned, const WireSegment&)> func) const
{
  unsigned key = computeKey(length, load, outputSlew);

  if (_keyToWireSegments.find(key) == _keyToWireSegments.end()) {
    return;
  }

  const std::deque<unsigned>& wireSegmentsIdx = _keyToWireSegments.at(key);
  for (unsigned idx : wireSegmentsIdx) {
    func(idx, _wireSegments[idx]);
  }
};

void TechChar::report() const
{
  _logger->report("\n");
  _logger->report("*********************************************************************");
  _logger->report("*                     Report Characterization                       *");
  _logger->report("*********************************************************************");
  _logger->report("     Idx  Len  Load      Out slew    Power   Delay"
                  "   In cap  In slew Buf     Buf Locs");

  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    std::string buffLocs;
    for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
      buffLocs = buffLocs + std::to_string(segment.getBufferLocation(idx)) + " ";
    }

    _logger->report("     {:<5}{:<5}{:<10}{:<12}{:<8}{:<8}{:<8}{:<8}{:<10}{}",
                    idx, segment.getLength(),segment.getLoad(),
                    segment.getOutputSlew(), segment.getPower(), segment.getDelay(),
                    segment.getInputCap(), segment.getInputSlew(),
                    segment.isBuffered(), buffLocs);
  });

  _logger->report("*************************************************************");
}

void TechChar::reportSegments(uint8_t length,
                              uint8_t load,
                              uint8_t outputSlew) const
{
  _logger->report("\n");
  _logger->report("*********************************************************************");
  _logger->report("*                     Report Characterization                       *");
  _logger->report("*********************************************************************");

  _logger->report(" Reporting wire segments with length: {} load: {} out slew: {}",
    length, load, outputSlew);

  _logger->report("     Idx  Len  Load      Out slew    Power   Delay"
                  "   In cap  In slew Buf     Buf Locs");

  forEachWireSegment(
      length, load, outputSlew, [&](unsigned idx, const WireSegment& segment) {
        std::string buffLocs;
        for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
          buffLocs = buffLocs + std::to_string(segment.getBufferLocation(idx)) + " ";
        }
        _logger->report("     {:<5}{:<5}{:<10}{:<12}{:<8}{:<8}{:<8}{:<8}{:<10}{}",
                    idx, segment.getLength(),segment.getLoad(),
                    segment.getOutputSlew(), segment.getPower(), segment.getDelay(),
                    segment.getInputCap(), segment.getInputSlew(),
                    segment.isBuffered(), buffLocs);
      });
}

void TechChar::write(const std::string& filename) const
{
  std::ofstream file(filename.c_str());

  if (!file.is_open()) {
    _logger->error(CTS, 59, "Could not open characterization file.");
  }

  file << _minSegmentLength << " " << _maxSegmentLength << " "
       << _minCapacitance << " " << _maxCapacitance << " " << _minSlew << " "
       << _maxSlew << " " << _options->getWireSegmentUnit() << "\n";

  file.precision(15);
  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    file << idx << " " << (unsigned) segment.getLength() << " "
         << (unsigned) segment.getLoad() << " "
         << (unsigned) segment.getOutputSlew() << " " << segment.getPower()
         << " " << segment.getDelay() << " " << (unsigned) segment.getInputCap()
         << " " << (unsigned) segment.getInputSlew() << " ";

    file << !segment.isBuffered() << " ";

    for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
      file << segment.getBufferLocation(idx) << " ";
    }

    file << "\n";
  });

  file.close();
}

void TechChar::writeSol(const std::string& filename) const
{
  std::ofstream file(filename.c_str());

  if (!file.is_open()) {
    _logger->error(CTS, 60, " Could not open characterization file.");
  }

  file.precision(15);
  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    file << idx << " ";

    if (segment.isBuffered()) {
      for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
        float wirelengthValue = segment.getBufferLocation(idx)
                                * ((float) (segment.getLength())
                                   * (float) (_options->getWireSegmentUnit()));
        file << (unsigned long) (wirelengthValue);
        file << "," << segment.getBufferMaster(idx);
        if (!(idx + 1 >= segment.getNumBuffers())) {
          file << ",";
        }
      }
    } else {
      float wirelengthValue = (float) (segment.getLength())
                              * (float) (_options->getWireSegmentUnit());
      file << (unsigned long) (wirelengthValue);
    }

    file << "\n";
  });

  file.close();
}

void TechChar::createFakeEntries(unsigned length, unsigned fakeLength)
{
  // This condition would just duplicate wires that already exist
  if (length == fakeLength) {
    return;
  }

  _logger->warn(CTS, 45, "Creating fake entries in the LUT.");
  for (unsigned load = 1; load <= getMaxCapacitance(); ++load) {
    for (unsigned outSlew = 1; outSlew <= getMaxSlew(); ++outSlew) {
      forEachWireSegment(
          length, load, outSlew, [&](unsigned key, const WireSegment& seg) {
            unsigned power = seg.getPower();
            unsigned delay = seg.getDelay();
            unsigned inputCap = seg.getInputCap();
            unsigned inputSlew = seg.getInputSlew();

            WireSegment& fakeSeg = createWireSegment(
                fakeLength, load, outSlew, power, delay, inputCap, inputSlew);

            for (unsigned buf = 0; buf < seg.getNumBuffers(); ++buf) {
              fakeSeg.addBuffer(seg.getBufferLocation(buf));
              fakeSeg.addBufferMaster(seg.getBufferMaster(buf));
            }
          });
    }
  }
}

void TechChar::reportSegment(unsigned key) const
{
  const WireSegment& seg = getWireSegment(key);

  _logger->report("    Key: {} outSlew: {} load: {} length: {} isBuffered: {}",
                  key, seg.getOutputSlew(), seg.getLoad(),
                  seg.getLength(), seg.isBuffered());
}

void TechChar::getMaxSlewMaxCapFromAxis(sta::TableAxis* axis, float& maxSlew, bool& maxSlewExist,
                                     float& maxCap, bool& maxCapExist, bool midValue)
{
  if (axis) {
    switch (axis->variable()) {
      case sta::TableAxisVariable::total_output_net_capacitance:
      {
        unsigned idx = axis->size() - 1;
        if (midValue && idx>1) {
          idx = axis->size()/2 - 1;
        }
        maxCap = axis->axisValue(idx);
        maxCapExist = true;
        break;
      }
      case sta::TableAxisVariable::input_net_transition:
      case sta::TableAxisVariable::input_transition_time:
      {
        unsigned idx = axis->size() - 1;
        if (midValue && idx>1) {
          idx = axis->size()/2 - 1;
        }
        maxSlew = axis->axisValue(idx);
        maxSlewExist = true;
        break;
      }
      default:
        break;
    }
  }
}
void TechChar::getBufferMaxSlewMaxCap(sta::LibertyLibrary* staLib, sta::LibertyCell* buffer,
                                      float &maxSlew, bool &maxSlewExist,
                                      float &maxCap, bool &maxCapExist, bool midValue)
{
  sta::LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  sta::TimingArcSetSeq *arc_sets = buffer->timingArcSets(input, output);
  if (arc_sets) {
    for (sta::TimingArcSet *arc_set : *arc_sets) {
      sta::TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
        sta::TimingArc *arc = arc_iter.next();
        sta::GateTableModel *model = dynamic_cast<sta::GateTableModel*>(arc->model());
        if(model && model->delayModel()) {
          auto delayModel = model->delayModel();
          sta::TableAxis *axis1 = delayModel->axis1();
          sta::TableAxis *axis2 = delayModel->axis2();
          sta::TableAxis *axis3 = delayModel->axis3();
          if(axis1) getMaxSlewMaxCapFromAxis(axis1, maxSlew, maxSlewExist, maxCap, maxCapExist, midValue);
          if(axis2) getMaxSlewMaxCapFromAxis(axis2, maxSlew, maxSlewExist, maxCap, maxCapExist, midValue);
          if(axis3) getMaxSlewMaxCapFromAxis(axis3, maxSlew, maxSlewExist, maxCap, maxCapExist, midValue);
        }
      }
    }
  }
}

void TechChar::getClockLayerResCap(double &cap, double &res)
{
  /* Clock layer should be set with set_wire_rc -clock */
  rsz::Resizer *sizer = ord::OpenRoad::openRoad()->getResizer();
  sta::Corner *corner = _openSta->cmdCorner();

  cap = sizer->wireClkCapacitance(corner)*1e-6; //convert from per meter to per micron
  res = sizer->wireClkResistance(corner)*1e-6; //convert from per meter to per micron

}
// Characterization Methods

void TechChar::initCharacterization()
{
  // Sets up most of the attributes that the characterization uses.
  // Gets the chip, openSta and networks.
  ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
  _dbNetworkChar = openRoad->getDbNetwork();
  if (_dbNetworkChar == nullptr) {
    _logger->error(CTS, 68, "Network not found. Check your lef/def/verilog file.");
  }
  _db = openRoad->getDb();
  if (_db == nullptr) {
    _logger->error(CTS, 69, "Database not found. Check your lef/def/verilog file.");
  }
  odb::dbChip* chip = _db->getChip();
  if (chip == nullptr) {
    _logger->error(CTS, 70, "Chip not found. Check your lef/def/verilog file.");
  }
  odb::dbBlock* block = chip->getBlock();
  if (block == nullptr) {
    _logger->error(CTS, 71, "Block not found. Check your lef/def/verilog file.");
  }
  _openSta = openRoad->getSta();
  sta::Network* networkChar = _openSta->network();
  if (networkChar == nullptr) {
    _logger->error(CTS, 72, "Network not found. Check your lef/def/verilog file.");
  }
  float dbUnitsPerMicron = static_cast<float>(block->getDbUnitsPerMicron());

  // Change resPerSqr and capPerSqr to DBU units.
  double newCapPerSqr = _options->getCapPerSqr() * 1e-12 / dbUnitsPerMicron;
  double newResPerSqr = _options->getResPerSqr() / dbUnitsPerMicron;
  if(newCapPerSqr == 0.0 || newResPerSqr == 0.0) {
    getClockLayerResCap(newCapPerSqr, newResPerSqr);
    newCapPerSqr = (newCapPerSqr / dbUnitsPerMicron);  // picofarad/meter to farad/DBU
    newResPerSqr = (newResPerSqr / dbUnitsPerMicron);  // ohm/meter to ohm/DBU
  }
  if(newCapPerSqr == 0.0 || newResPerSqr == 0.0) {
    std::cout << "    [WARNING] Per unit resistance or capacitance not set or zero." << std::endl;
    std::cout << "              Use set_wire_rc before running clock_tree_synthesis." << std::endl;
  } else {
    _options->setCapPerSqr(newCapPerSqr);  // picofarad/micron to farad/DBU
    _options->setResPerSqr(newResPerSqr);  // ohm/micron to ohm/DBU
  }
  // Change intervals if needed
  if (_options->getSlewInter() != 0) {
    _charSlewInter = _options->getSlewInter();
  }
  if (_options->getCapInter() != 0) {
    _charCapInter = _options->getCapInter();
  }

  // Gets the buffer masters and its in/out pins.
  std::vector<std::string> masterVector = _options->getBufferList();
  if (masterVector.size() < 1) {
    _logger->error(CTS, 73, "Buffer not found. Check your -buf_list input.");
  }
  odb::dbMaster* testBuf = nullptr;
  for (std::string masterString : masterVector) {
    testBuf = _db->findMaster(masterString.c_str());
    if (testBuf == nullptr) {
      _logger->error(CTS, 74, "Buffer {} not found. Check your -buf_list input.", masterString);
    }
    _masterNames.insert(masterString);
  }

  std::string bufMasterName = masterVector[0];
  _charBuf = _db->findMaster(bufMasterName.c_str());

  odb::dbMaster *sinkMaster = _db->findMaster(_options->getSinkBuffer().c_str());

  for (odb::dbMTerm* masterTerminal : _charBuf->getMTerms()) {
    if (masterTerminal->getIoType() == odb::dbIoType::INPUT
        && masterTerminal->getSigType() == odb::dbSigType::SIGNAL) {
      _charBufIn = masterTerminal->getName();
    } else if (masterTerminal->getIoType() == odb::dbIoType::OUTPUT
               && masterTerminal->getSigType() == odb::dbSigType::SIGNAL) {
      _charBufOut = masterTerminal->getName();
    }
  }
  // Creates the new characterization block. (Wiresegments are created here
  // instead of the main block)
  std::string characterizationBlockName = "CharacterizationBlock";
  _charBlock = odb::dbBlock::create(block, characterizationBlockName.c_str());

  // Gets the capacitance and resistance per DBU. User input.
  _resPerDBU = _options->getResPerSqr();
  _capPerDBU = _options->getCapPerSqr();

  // Defines the different wirelengths to test and the characterization unit.
  unsigned wirelengthIterations = _options->getCharWirelengthIterations();
  unsigned maxWirelength = (_charBuf->getHeight() * 10)
                           * wirelengthIterations;  // Hard-coded limit
  if (_options->getWireSegmentUnit() == 0) {
    unsigned charaunit = _charBuf->getHeight() * 10;
    _options->setWireSegmentUnit(static_cast<unsigned>(charaunit));
  } else {
    // Updates the units to DBU.
    int dbUnitsPerMicron = block->getDbUnitsPerMicron();
    unsigned segmentDistance = _options->getWireSegmentUnit();
    _options->setWireSegmentUnit(segmentDistance * dbUnitsPerMicron);
  }

  // Required to make sure that the fake entry for minLengthSinkRegion
  // exists (see HTreeBuilder::run())
  if (_options->isFakeLutEntriesEnabled()) {
    maxWirelength = std::max(maxWirelength,
                             2 * _options->getWireSegmentUnit());
  }

  for (unsigned wirelengthInter = _options->getWireSegmentUnit();
       (wirelengthInter <= maxWirelength)
       && (wirelengthInter
           <= wirelengthIterations * _options->getWireSegmentUnit());
       wirelengthInter += _options->getWireSegmentUnit()) {
    _wirelengthsToTest.push_back(wirelengthInter);
  }

  if (_wirelengthsToTest.size() < 1) {
    _logger->error(CTS, 75,  "Error generating the wirelengths to test. Check your -wire_unit "
        "parameter or technology files.");
  }

  setLenghthUnit(static_cast<unsigned>(((_charBuf->getHeight() * 10) / 2)
                                       / dbUnitsPerMicron));

  // Gets the max slew and max cap if they weren't added as parameters.
  float maxSlew = 0.0;
  float maxCap = 0.0;
  if (_options->getMaxCharSlew() == 0 || _options->getMaxCharCap() == 0) {
    sta::Cell* masterCell = _dbNetworkChar->dbToSta(_charBuf);
    sta::Cell* sinkCell = _dbNetworkChar->dbToSta(sinkMaster);
    sta::LibertyCell* libertyCell = networkChar->libertyCell(masterCell);
    sta::LibertyCell* libertySinkCell = networkChar->libertyCell(sinkCell);
    bool maxSlewExist = false;
    bool maxCapExist = false;

    if (!libertyCell) {
      _logger->error(CTS, 96, "No Liberty cell found for {}.", bufMasterName);
    } else {
      sta::LibertyLibrary* staLib = libertyCell->libertyLibrary();
      getBufferMaxSlewMaxCap(staLib, libertyCell, maxSlew, maxSlewExist, maxCap, maxCapExist);
      if (!maxSlewExist || !maxCapExist) { //In case buffer does not have tables
        _logger->warn(CTS, 67,
              "Could not get maxSlew/maxCap values from buffer {}. Using library values", bufMasterName);
        staLib->defaultMaxSlew(maxSlew, maxSlewExist);
        staLib->defaultMaxCapacitance(maxCap, maxCapExist);
      }
    }
    if (!maxSlewExist || !maxCapExist) {
      _logger->error(CTS, 77, "Liberty Library does not have Max Slew or Max Cap values.");
    } else {
      _charMaxSlew = maxSlew;
      _charMaxCap = maxCap;
    }
    if (!libertySinkCell) {
      _logger->error(CTS, 76, "No Liberty cell found for {}.", _options->getSinkBuffer());
    } else {
      sta::LibertyLibrary* staLib = libertySinkCell->libertyLibrary();
      sta::LibertyPort *input, *output;
      libertySinkCell->bufferPorts(input, output);
      _options->setSinkBufferInputCap(input->capacitance());
      maxCapExist = false;
      maxSlewExist = false;
      getBufferMaxSlewMaxCap(staLib, libertySinkCell, maxSlew, maxSlewExist, maxCap, maxCapExist, true);
      if (!maxCapExist) { //In case buffer does not have tables
        _logger->warn(CTS, 66,
              "Could not get maxSlew/maxCap values from buffer {}", _options->getSinkBuffer());
        _options->setSinkBufferMaxCap(_charMaxCap);
      } else {
        _options->setSinkBufferMaxCap(maxCap);
      }
    }

  } else {
    _charMaxSlew = _options->getMaxCharSlew();
    _charMaxCap = _options->getMaxCharCap();
  }
  // Creates the different slews and loads to test.
  unsigned slewIterations = _options->getCharSlewIterations();
  unsigned loadIterations = _options->getCharLoadIterations();
  for (float slewInter = _charSlewInter;
       (slewInter <= _charMaxSlew)
       && (slewInter <= slewIterations * _charSlewInter);
       slewInter += _charSlewInter) {
    _slewsToTest.push_back(slewInter);
  }
  for (float capInter = _charCapInter;
       ((capInter <= _charMaxCap)
        && (capInter <= loadIterations * _charCapInter));
       capInter += _charCapInter) {
    _loadsToTest.push_back(capInter);
  }

  if ((_loadsToTest.size() < 1) || (_slewsToTest.size() < 1)) {
    _logger->error(CTS, 78,  "Error generating the wirelengths to test. "
        "Check your -max_cap / -max_slew / -cap_inter / -slew_inter parameter "
        "or technology files.");
  }
}

std::vector<TechChar::SolutionData> TechChar::createPatterns(
    unsigned setupWirelength)
{
  // Sets the number of nodes (wirelength/characterization unit) that a buffer
  // can be placed and...
  //...the number of topologies (combinations of buffers, considering only 1
  // drive) that can exist.
  const unsigned numberOfNodes
      = setupWirelength / _options->getWireSegmentUnit();
  unsigned numberOfTopologies = 1 << numberOfNodes;
  std::vector<SolutionData> topologiesVector;
  odb::dbNet* net = nullptr;
  odb::dbWire* wire = nullptr;

  // For each possible topology...
  for (unsigned solutionCounterInt = 0; solutionCounterInt < numberOfTopologies;
       solutionCounterInt++) {
    // Creates a bitset that represents the buffer locations.
    std::bitset<5> solutionCounter(solutionCounterInt);
    unsigned short int wireCounter = 0;
    SolutionData topology;
    // Creates the starting net.
    std::string netName = "net_" + std::to_string(setupWirelength) + "_"
                          + solutionCounter.to_string() + "_"
                          + std::to_string(wireCounter);
    net = odb::dbNet::create(_charBlock, netName.c_str());
    wire = odb::dbWire::create(net);
    net->setSigType(odb::dbSigType::SIGNAL);
    // Creates the input port.
    std::string inPortName
        = "in_" + std::to_string(setupWirelength) + solutionCounter.to_string();
    odb::dbBTerm* inPort = odb::dbBTerm::create(
        net, inPortName.c_str());  // sig type is signal by default
    inPort->setIoType(odb::dbIoType::INPUT);
    odb::dbBPin* inPortPin = odb::dbBPin::create(inPort);
    // Updates the topology with the new port.
    topology.inPort = inPortPin;
    // Iterating through possible buffers...
    unsigned nodesWithoutBuf = 0;
    bool isPureWire = true;
    for (unsigned nodeIndex = 0; nodeIndex < numberOfNodes; nodeIndex++) {
      if (solutionCounter[nodeIndex] == 0) {
        // Not a buffer, only a wire segment.
        nodesWithoutBuf++;
      } else {
        // Buffer, need to create the instance and a new net.
        nodesWithoutBuf++;
        // Creates a new buffer instance.
        std::string bufName = "buf_" + std::to_string(setupWirelength)
                              + solutionCounter.to_string() + "_"
                              + std::to_string(wireCounter);
        odb::dbInst* bufInstance
            = odb::dbInst::create(_charBlock, _charBuf, bufName.c_str());
        odb::dbITerm* bufInstanceInPin
            = bufInstance->findITerm(_charBufIn.c_str());
        odb::dbITerm* bufInstanceOutPin
            = bufInstance->findITerm(_charBufOut.c_str());
        odb::dbITerm::connect(bufInstanceInPin, net);
        // Updates the topology with the old net and number of nodes that didn't
        // have buffers until now.
        topology.netVector.push_back(net);
        topology.nodesWithoutBufVector.push_back(nodesWithoutBuf);
        // Creates a new net.
        wireCounter++;
        std::string netName = "net_" + std::to_string(setupWirelength)
                              + solutionCounter.to_string() + "_"
                              + std::to_string(wireCounter);
        net = odb::dbNet::create(_charBlock, netName.c_str());
        wire = odb::dbWire::create(net);
        odb::dbITerm::connect(bufInstanceOutPin, net);
        net->setSigType(odb::dbSigType::SIGNAL);
        // Updates the topology wih the new instance and the current topology
        // (as a vector of strings).
        topology.instVector.push_back(bufInstance);
        topology.topologyDescriptor.push_back(
            std::to_string(nodesWithoutBuf * _options->getWireSegmentUnit()));
        topology.topologyDescriptor.push_back(_charBuf->getName());
        nodesWithoutBuf = 0;
        isPureWire = false;
      }
    }
    // Finishing up the topology with the output port.
    std::string outPortName = "out_" + std::to_string(setupWirelength)
                              + solutionCounter.to_string();
    odb::dbBTerm* outPort = odb::dbBTerm::create(
        net, outPortName.c_str());  // sig type is signal by default
    outPort->setIoType(odb::dbIoType::OUTPUT);
    odb::dbBPin* outPortPin = odb::dbBPin::create(outPort);
    // Updates the topology with the output port, old new, possible instances
    // and other attributes.
    topology.outPort = outPortPin;
    if (isPureWire) {
      topology.instVector.push_back(nullptr);
    }
    topology.isPureWire = isPureWire;
    topology.netVector.push_back(net);
    topology.nodesWithoutBufVector.push_back(nodesWithoutBuf);
    if (nodesWithoutBuf != 0) {
      topology.topologyDescriptor.push_back(
          std::to_string(nodesWithoutBuf * _options->getWireSegmentUnit()));
    }
    // Go to the next topology.
    topologiesVector.push_back(topology);
  }
  return topologiesVector;
}

void TechChar::createStaInstance()
{
  // Creates a new OpenSTA instance that is used only for the characterization.
  ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
  // Creates the new instance based on the charcterization block.
  if (_openStaChar != nullptr) {
    _openStaChar->clear();
  }
  _openStaChar = sta::makeBlockSta(openRoad, _charBlock);
  // Sets the current OpenSTA instance as the new one just created.
  sta::Sta::setSta(_openStaChar);
  _openStaChar->clear();
  // Gets the new components from the new OpenSTA.
  _dbNetworkChar = openRoad->getDbNetwork();
  // Set some attributes for the new instance.
  _openStaChar->units()->timeUnit()->setScale(1);
  _openStaChar->units()->capacitanceUnit()->setScale(1);
  _openStaChar->units()->resistanceUnit()->setScale(1);
  _openStaChar->units()->powerUnit()->setScale(1);
  // Gets the corner and other analysis attributes from the new instance.
  _charCorner = _openStaChar->corners()->findCorner(0);
  sta::PathAPIndex path_ap_index
      = _charCorner->findPathAnalysisPt(sta::MinMax::max())->index();
  sta::Corners* corners = _openStaChar->search()->corners();
  _charPathAnalysis = corners->findPathAnalysisPt(path_ap_index);
}

void TechChar::setParasitics(
    std::vector<TechChar::SolutionData> topologiesVector,
    unsigned setupWirelength)
{
  // For each topology...
  for (SolutionData solution : topologiesVector) {
    // For each net in the topolgy -> set the parasitics.
    for (unsigned netIndex = 0; netIndex < solution.netVector.size();
         ++netIndex) {
      // Gets the ITerms (instance pins) and BTerms (other high-level pins) from
      // the current net.
      odb::dbNet* net = solution.netVector[netIndex];
      unsigned nodesWithoutBuf = solution.nodesWithoutBufVector[netIndex];
      odb::dbBTerm* inBTerm = solution.inPort->getBTerm();
      odb::dbBTerm* outBTerm = solution.outPort->getBTerm();
      odb::dbSet<odb::dbBTerm> netBTerms = net->getBTerms();
      odb::dbSet<odb::dbITerm> netITerms = net->getITerms();
      sta::Pin* firstPin = nullptr;
      sta::Pin* lastPin = nullptr;
      // Gets the sta::Pin from the beginning and end of the net.
      if (netBTerms.size() > 1) {  // Parasitics for a purewire segment.
                                   // First and last pin are already available.
        firstPin = _dbNetworkChar->dbToSta(inBTerm);
        lastPin = _dbNetworkChar->dbToSta(outBTerm);
      } else if (netBTerms.size()
                 == 1) {  // Parasitics for the end/start of a net.
                          // One Port and one instance pin.
        odb::dbBTerm* netBTerm = net->get1stBTerm();
        odb::dbITerm* netITerm = net->get1stITerm();
        if (netBTerm == inBTerm) {
          firstPin = _dbNetworkChar->dbToSta(netBTerm);
          lastPin = _dbNetworkChar->dbToSta(netITerm);
        } else {
          firstPin = _dbNetworkChar->dbToSta(netITerm);
          lastPin = _dbNetworkChar->dbToSta(netBTerm);
        }
      } else {  // Parasitics for a net that is between two buffers. Need to
                // iterate over the net ITerms.
        for (odb::dbITerm* iterm : netITerms) {
          if (iterm != nullptr) {
            if (iterm->getIoType() == odb::dbIoType::INPUT) {
              lastPin = _dbNetworkChar->dbToSta(iterm);
            }

            if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
              firstPin = _dbNetworkChar->dbToSta(iterm);
            }

            if (firstPin != nullptr && lastPin != nullptr) {
              break;
            }
          }
        }
      }
      // Sets the Pi and Elmore information.
      unsigned charUnit = _options->getWireSegmentUnit();
      _openStaChar->makePiElmore(firstPin,
                                 sta::RiseFall::rise(),
                                 sta::MinMaxAll::all(),
                                 (nodesWithoutBuf * charUnit * _capPerDBU) / 2,
                                 nodesWithoutBuf * charUnit * _resPerDBU,
                                 (nodesWithoutBuf * charUnit * _capPerDBU) / 2);
      _openStaChar->makePiElmore(firstPin,
                                 sta::RiseFall::fall(),
                                 sta::MinMaxAll::all(),
                                 (nodesWithoutBuf * charUnit * _capPerDBU) / 2,
                                 nodesWithoutBuf * charUnit * _resPerDBU,
                                 (nodesWithoutBuf * charUnit * _capPerDBU) / 2);
      _openStaChar->setElmore(firstPin,
                              lastPin,
                              sta::RiseFall::rise(),
                              sta::MinMaxAll::all(),
                              nodesWithoutBuf * charUnit * _capPerDBU);
      _openStaChar->setElmore(firstPin,
                              lastPin,
                              sta::RiseFall::fall(),
                              sta::MinMaxAll::all(),
                              nodesWithoutBuf * charUnit * _capPerDBU);
    }
  }
}

void TechChar::setSdc(std::vector<TechChar::SolutionData> topologiesVector,
                      unsigned setupWirelength)
{
  // Creates a clock to set input and output delay.
  sta::Sdc* sdcChar = _openStaChar->sdc();
  sta::FloatSeq* characterizationClockWave = new sta::FloatSeq;
  characterizationClockWave->push_back(0);
  characterizationClockWave->push_back(1);
  const std::string characterizationClockName
      = "characlock" + std::to_string(setupWirelength);
  _openStaChar->makeClock(characterizationClockName.c_str(),
                          nullptr,
                          false,
                          1.0,
                          characterizationClockWave,
                          nullptr);
  sta::Clock* characterizationClock
      = sdcChar->findClock(characterizationClockName.c_str());
  // For each topology...
  for (SolutionData solution : topologiesVector) {
    // Gets the input and output ports.
    odb::dbBTerm* inBTerm = solution.inPort->getBTerm();
    odb::dbBTerm* outBTerm = solution.outPort->getBTerm();
    sta::Pin* inPin = _dbNetworkChar->dbToSta(inBTerm);
    sta::Pin* outPin = _dbNetworkChar->dbToSta(outBTerm);
    // Set the input delay and output delay on each one.
    _openStaChar->setInputDelay(inPin,
                                sta::RiseFallBoth::riseFall(),
                                characterizationClock,
                                sta::RiseFall::rise(),
                                nullptr,
                                false,
                                false,
                                sta::MinMaxAll::max(),
                                true,
                                0.0);
    _openStaChar->setOutputDelay(outPin,
                                 sta::RiseFallBoth::riseFall(),
                                 characterizationClock,
                                 sta::RiseFall::rise(),
                                 nullptr,
                                 false,
                                 false,
                                 sta::MinMaxAll::max(),
                                 true,
                                 0.0);
  }
}

TechChar::ResultData TechChar::computeTopologyResults(
    TechChar::SolutionData solution,
    sta::Vertex* outPinVert,
    float load,
    unsigned setupWirelength)
{
  ResultData results;
  // Computations for power, requires the PowerResults class from OpenSTA.
  float totalPower = 0;
  if (!solution.isPureWire) {
    // If it isn't a pure wire solution, get the sum of the total power of each
    // buffer.
    for (odb::dbInst* bufferInst : solution.instVector) {
      sta::Instance* bufferInstSta = _dbNetworkChar->dbToSta(bufferInst);
      sta::PowerResult instResults;
      instResults.clear();
      _openStaChar->power(bufferInstSta, _charCorner, instResults);
      totalPower = totalPower + instResults.total();
    }
  }
  results.totalPower = totalPower;
  // Computations for input capacitance.
  float incap = 0;
  if (solution.isPureWire) {
    // For pure-wire, sum of the current load with the capacitance of the net.
    incap = load + (setupWirelength * _capPerDBU);
  } else {
    // For buffered solutions, add the cap of the input of the first buffer
    // with the capacitance of the left-most net.
    float length = std::stod(solution.topologyDescriptor[0]);
    sta::LibertyCell* firstInstLiberty = _dbNetworkChar->libertyCell(
        _dbNetworkChar->dbToSta(solution.instVector[0]));
    sta::LibertyPort* firstPinLiberty
        = firstInstLiberty->findLibertyPort(_charBufIn.c_str());
    float firstPinCapRise = firstPinLiberty->capacitance(sta::RiseFall::rise(),
                                                         sta::MinMax::max());
    float firstPinCapFall = firstPinLiberty->capacitance(sta::RiseFall::fall(),
                                                         sta::MinMax::max());
    float firstPinCap = 0;
    if (firstPinCapRise > firstPinCapFall) {
      firstPinCap = firstPinCapRise;
    } else {
      firstPinCap = firstPinCapFall;
    }
    incap = firstPinCap + (length * _capPerDBU);
  }
  float totalcap = std::floor((incap + (_charCapInter / 2)) / _charCapInter)
                   * _charCapInter;
  results.totalcap = totalcap;
  // Computations for delay.
  float pinArrival = _openStaChar->vertexArrival(
      outPinVert, sta::RiseFall::fall(), _charPathAnalysis);
  results.pinArrival = pinArrival;
  // Computations for output slew.
  float pinRise = _openStaChar->vertexSlew(
      outPinVert, sta::RiseFall::rise(), sta::MinMax::max());
  float pinFall = _openStaChar->vertexSlew(
      outPinVert, sta::RiseFall::fall(), sta::MinMax::max());
  float pinSlew = std::floor((((pinRise + pinFall) / 2) + (_charSlewInter / 2))
                             / _charSlewInter)
                  * _charSlewInter;
  results.pinSlew = pinSlew;

  return results;
}

TechChar::SolutionData TechChar::updateBufferTopologies(
    TechChar::SolutionData solution)
{
  unsigned index = 0;
  // Change the buffer topology by increasing the size of the buffers.
  // After testing all the sizes for the current buffer, increment the size of
  // the next one (works like a carry mechanism). Ex for 4 different buffers:
  // 103-> 110 -> 111 -> 112 -> 113 -> 120 ...
  bool done = false;
  while (!done) {
    // Gets the iterator to the beggining of the _masterNames set.
    std::set<std::string>::iterator masterItr
        = _masterNames.find(solution.instVector[index]->getMaster()->getName());
    // Gets the iterator to the end of the _masterNames set.
    std::set<std::string>::iterator masterFinalItr = _masterNames.end();
    masterFinalItr--;
    if (masterItr == masterFinalItr) {
      // If the iterator can't increment past the final iterator...
      // change the current buf master to the _charBuf and try to go to next
      // instance.
      odb::dbInst* inst = solution.instVector[index];
      inst->swapMaster(_charBuf);
      unsigned topologyCounter = 0;
      for (unsigned topologyIndex = 0;
           topologyIndex < solution.topologyDescriptor.size();
           topologyIndex++) {
        // Iterates through the topologyDescriptor to set the new information
        //(string representing the current buffer)
        std::string topologyS = solution.topologyDescriptor[topologyIndex];
        if (!(_masterNames.find(topologyS) == _masterNames.end())) {
          if (topologyCounter == index) {
            std::set<std::string>::iterator firstMaster = _masterNames.begin();
            solution.topologyDescriptor[topologyIndex] = *firstMaster;
            break;
          }
          topologyCounter++;
        }
      }
      index++;
    } else {
      // Increment the iterator and change the current buffer to the new size.
      masterItr++;
      std::string masterString = *masterItr;
      odb::dbMaster* newBufMaster = _db->findMaster(masterString.c_str());
      odb::dbInst* inst = solution.instVector[index];
      inst->swapMaster(newBufMaster);
      unsigned topologyCounter = 0;
      for (unsigned topologyIndex = 0;
           topologyIndex < solution.topologyDescriptor.size();
           topologyIndex++) {
        std::string topologyS = solution.topologyDescriptor[topologyIndex];
        if (!(_masterNames.find(topologyS) == _masterNames.end())) {
          if (topologyCounter == index) {
            solution.topologyDescriptor[topologyIndex] = masterString;
            break;
          }
          topologyCounter++;
        }
      }
      done = true;
    }
    if (index >= solution.instVector.size()) {
      // If the next instance doesn't exist, all the topologies were tested ->
      // exit the function.
      done = true;
    }
  }
  return solution;
}

std::vector<TechChar::ResultData> TechChar::characterizationPostProcess()
{
  // Post-process of the characterization results.
  std::vector<ResultData> selectedSolutions;
  // Select only a subset of the total results. If, for a combination of input
  // cap, wirelength, load and output slew, more than 3 results exists -> select
  // only 3 of them.
  for (auto& keyResults : _solutionMap) {
    std::vector<ResultData> resultVector = keyResults.second;
    for (ResultData selectedResults : resultVector) {
      selectedSolutions.push_back(selectedResults);
    }
  }

  // Creates variables to set the max and min values. These are normalized.
  unsigned minResultWirelength = std::numeric_limits<unsigned>::max();
  unsigned maxResultWirelength = 0;
  unsigned minResultCapacitance = std::numeric_limits<unsigned>::max();
  unsigned maxResultCapacitance = 0;
  unsigned minResultSlew = std::numeric_limits<unsigned>::max();
  unsigned maxResultSlew = 0;
  std::vector<ResultData> convertedSolutions;
  for (ResultData solution : selectedSolutions) {
    if (solution.pinSlew <= _charMaxSlew) {
      ResultData convertedResult;
      // Processing and normalizing of output slew.
      convertedResult.pinSlew = normalizeCharResults(
          solution.pinSlew, _charSlewInter, &minResultSlew, &maxResultSlew);
      // Processing and normalizing of input slew.
      convertedResult.inSlew = normalizeCharResults(
          solution.inSlew, _charSlewInter, &minResultSlew, &maxResultSlew);
      // Processing and normalizing of input cap.
      convertedResult.totalcap = normalizeCharResults(solution.totalcap,
                                                      _charCapInter,
                                                      &minResultCapacitance,
                                                      &maxResultCapacitance);
      // Processing and normalizing of load.
      convertedResult.load = normalizeCharResults(solution.load,
                                                  _charCapInter,
                                                  &minResultCapacitance,
                                                  &maxResultCapacitance);
      // Processing and normalizing of the wirelength.
      convertedResult.wirelength
          = normalizeCharResults(solution.wirelength,
                                 _options->getWireSegmentUnit(),
                                 &minResultWirelength,
                                 &maxResultWirelength);
      // Processing and normalizing of delay.
      convertedResult.pinArrival = static_cast<unsigned>(
          std::ceil(solution.pinArrival / (_charSlewInter / 5)));
      // Add missing information.
      convertedResult.totalPower = solution.totalPower;
      convertedResult.isPureWire = solution.isPureWire;
      std::vector<std::string> topologyResult;
      for (int topologyIndex = 0; topologyIndex < solution.topology.size();
           topologyIndex++) {
        std::string topologyS = solution.topology[topologyIndex];
        // Normalizes the strings that represents the topology too.
        if (_masterNames.find(topologyS) == _masterNames.end()) {
          // Is a number (i.e. a wire segment).
          topologyResult.push_back(std::to_string(
              std::stod(topologyS) / static_cast<float>(solution.wirelength)));
        } else {
          topologyResult.push_back(topologyS);
        }
      }
      convertedResult.topology = topologyResult;
      // Send the results to a vector. This will be used to create the
      // wiresegments for CTS.
      convertedSolutions.push_back(convertedResult);
    }
  }
  // Sets the min and max values and returns the result vector.
  _minSlew = minResultSlew;
  _maxSlew = maxResultSlew;
  _minCapacitance = minResultCapacitance;
  _maxCapacitance = maxResultCapacitance;
  _minSegmentLength = minResultWirelength;
  _maxSegmentLength = maxResultWirelength;
  return convertedSolutions;
}

unsigned TechChar::normalizeCharResults(float value,
                                        float iter,
                                        unsigned* min,
                                        unsigned* max)
{
  unsigned normVal = static_cast<unsigned>(std::ceil(value / iter));
  if (normVal == 0) {
    normVal = 1;
  }
  *min = std::min(*min, normVal);
  *max = std::max(*max, normVal);
  return normVal;
}

void TechChar::create()
{
  // Setup of the attributes required to run the characterization.
  initCharacterization();
  long unsigned int topologiesCreated = 0;
  for (unsigned setupWirelength : _wirelengthsToTest) {
    // Creates the topologies for the current wirelength.
    std::vector<SolutionData> topologiesVector
        = createPatterns(setupWirelength);
    // Creates the new openSTA instance and setup its components with
    // updateTiming(true);
    createStaInstance();
    _openStaChar->updateTiming(true);
    // Setup of the parasitics for each net and the input/output delay.
    setParasitics(topologiesVector, setupWirelength);
    setSdc(topologiesVector, setupWirelength);
    // For each topology...
    for (SolutionData solution : topologiesVector) {
      // Gets the input and output port (as terms, pins and vertices).
      odb::dbBTerm* inBTerm = solution.inPort->getBTerm();
      odb::dbBTerm* outBTerm = solution.outPort->getBTerm();
      odb::dbNet* lastNet = solution.netVector.back();
      sta::Pin* inPin = _dbNetworkChar->dbToSta(inBTerm);
      sta::Pin* outPin = _dbNetworkChar->dbToSta(outBTerm);
      sta::Port* inPort = _dbNetworkChar->port(inPin);
      sta::Port* outPort = _dbNetworkChar->port(outPin);
      sta::Vertex* outPinVert
          = _openStaChar->graph()->vertex(outBTerm->staVertexId());
      sta::Vertex* inPinVert
          = _openStaChar->graph()->vertex(inBTerm->staVertexId());

      // Gets the first pin of the last net. Needed to set a new parasitic
      // (load) value.
      sta::Pin* firstPinLastNet = nullptr;
      if (lastNet->getBTerms().size()
          > 1) {  // Parasitics for purewire segment.
                  // First and last pin are already available.
        firstPinLastNet = inPin;
      } else {  // Parasitics for the end/start of a net. One Port and one
                // instance pin.
        odb::dbITerm* netITerm = lastNet->get1stITerm();
        firstPinLastNet = _dbNetworkChar->dbToSta(netITerm);
      }

      float c1 = 0;
      float c2 = 0;
      float r1 = 0;
      bool piExists = false;
      // Gets the parasitics that are currently used for the last net.
      _openStaChar->findPiElmore(firstPinLastNet,
                                 sta::RiseFall::rise(),
                                 sta::MinMax::max(),
                                 c1,
                                 r1,
                                 c2,
                                 piExists);
      // For each possible buffer combination (different sizes).
      unsigned buffersUpdate
          = std::pow(_masterNames.size(), solution.instVector.size());
      do {
        // For each possible load.
        for (float load : _loadsToTest) {
          // sta2->setPortExtPinCap(outPort, sta::RiseFallBoth::riseFall(),
          //                       sta::MinMaxAll::all(), load );

          // Sets the new parasitic of the last net (load added to last pin).
          _openStaChar->makePiElmore(firstPinLastNet,
                                     sta::RiseFall::rise(),
                                     sta::MinMaxAll::all(),
                                     c1,
                                     r1,
                                     c2 + load);
          _openStaChar->makePiElmore(firstPinLastNet,
                                     sta::RiseFall::fall(),
                                     sta::MinMaxAll::all(),
                                     c1,
                                     r1,
                                     c2 + load);
          _openStaChar->setElmore(firstPinLastNet,
                                  outPin,
                                  sta::RiseFall::rise(),
                                  sta::MinMaxAll::all(),
                                  c1 + c2 + load);
          _openStaChar->setElmore(firstPinLastNet,
                                  outPin,
                                  sta::RiseFall::fall(),
                                  sta::MinMaxAll::all(),
                                  c1 + c2 + load);
          // For each possible input slew.
          for (float inputslew : _slewsToTest) {
            // sta2->setInputSlew(inPort, sta::RiseFallBoth::riseFall(),
            //                   sta::MinMaxAll::all(), inputslew);

            // Sets the slew on the input vertex.
            // Here the new pattern is created (combination of load, buffers and
            // slew values).
            _openStaChar->setAnnotatedSlew(inPinVert,
                                           _charCorner,
                                           sta::MinMaxAll::all(),
                                           sta::RiseFallBoth::riseFall(),
                                           inputslew);
            // Updates timing for the new pattern.
            _openStaChar->updateTiming(true);

            // Gets the results (delay, slew, power...) for the pattern.
            ResultData results = computeTopologyResults(
                solution, outPinVert, load, setupWirelength);

            // Appends the results to a map, grouping each result by
            // wirelength, load, output slew and input cap.
            results.wirelength = setupWirelength;
            results.load = load;
            results.inSlew = inputslew;
            results.topology = solution.topologyDescriptor;
            results.isPureWire = solution.isPureWire;
            CharKey solutionKey;
            solutionKey.wirelength = results.wirelength;
            solutionKey.pinSlew = results.pinSlew;
            solutionKey.load = results.load;
            solutionKey.totalcap = results.totalcap;
            if (_solutionMap.find(solutionKey) != _solutionMap.end()) {
              _solutionMap[solutionKey].push_back(results);
            } else {
              std::vector<ResultData> resultGroup;
              resultGroup.push_back(results);
              _solutionMap[solutionKey] = resultGroup;
            }
            topologiesCreated++;
            if (topologiesCreated % 50000 == 0) {
              _logger->info(CTS, 38, "Number of created patterns = {}.", topologiesCreated);
            }
          }
        }
        // If the solution is not a pure-wire, update the buffer topologies.
        if (!solution.isPureWire) {
          solution = updateBufferTopologies(solution);
        }
        // For pure-wire solution buffersUpdate == 1, so it only runs once.
        buffersUpdate--;
      } while (buffersUpdate != 0);
    }
  }
  _logger->info(CTS, 39, "Number of created patterns = {}.", topologiesCreated);
  // Returns the OpenSTA instance to the old one.
  sta::Sta::setSta(_openSta);
  // Post-processing of the results.
  std::vector<ResultData> convertedSolutions = characterizationPostProcess();
  // Changes the segment units back to micron and creates the wire segments.
  float dbUnitsPerMicron
      = static_cast<float>(_charBlock->getDbUnitsPerMicron());
  float segmentDistance = static_cast<float>(_options->getWireSegmentUnit());
  _options->setWireSegmentUnit(
      static_cast<unsigned>(segmentDistance / dbUnitsPerMicron));
  compileLut(convertedSolutions);
  // Saves the characterization file if needed.
  if (_options->getOutputPath().length() > 0) {
    std::string lutFile = _options->getOutputPath() + "lut.txt";
    std::string solFile = _options->getOutputPath() + "sol_list.txt";
    write(lutFile);
    writeSol(solFile);
  }
  if (_openStaChar != nullptr) {
    _openStaChar->clear();
    delete _openStaChar;
    _openStaChar = nullptr;
  }
}

}  // namespace cts
