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

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

TechChar::TechChar(CtsOptions* options,
                   odb::dbDatabase* db,
                   sta::dbSta* sta,
                   rsz::Resizer* resizer,
                   sta::dbNetwork* db_network,
                   Logger* logger)
    : options_(options),
      db_(db),
      resizer_(resizer),
      openSta_(sta),
      openStaChar_(nullptr),
      db_network_(db_network),
      logger_(logger),
      resPerDBU_(0.0),
      capPerDBU_(0.0)
{
}

void TechChar::compileLut(const std::vector<TechChar::ResultData>& lutSols)
{
  logger_->info(CTS, 84, "Compiling LUT.");
  initLengthUnits();

  minSegmentLength_ = toInternalLengthUnit(minSegmentLength_);
  maxSegmentLength_ = toInternalLengthUnit(maxSegmentLength_);

  reportCharacterizationBounds();  // min and max values already set
  checkCharacterizationBounds();

  unsigned noSlewDegradationCount = 0;
  actualMinInputCap_ = std::numeric_limits<unsigned>::max();
  // For the results in each wire segment...
  for (ResultData lutLine : lutSols) {
    actualMinInputCap_
        = std::min(static_cast<unsigned>(lutLine.totalcap), actualMinInputCap_);
    // Checks the output slew of the wiresegment.
    if (lutLine.isPureWire && lutLine.pinSlew <= lutLine.inSlew) {
      ++noSlewDegradationCount;
      ++lutLine.pinSlew;
    }

    const unsigned length = toInternalLengthUnit(lutLine.wirelength);

    WireSegment& segment = createWireSegment(length,
                                             (unsigned) lutLine.load,
                                             (unsigned) lutLine.pinSlew,
                                             lutLine.totalPower,
                                             (unsigned) lutLine.pinArrival,
                                             (unsigned) lutLine.totalcap,
                                             (unsigned) lutLine.inSlew);

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
        const std::string topologyS = lutLine.topology[topologyIndex];
        // Each buffered topology always has a wire segment followed by a
        // buffer.
        if (masterNames_.find(topologyS) == masterNames_.end()) {
          // Is a number (i.e. a wire segment).
          segment.addBuffer(std::stod(topologyS));
        } else {
          segment.addBufferMaster(topologyS);
        }
      }
    }
  }

  if (noSlewDegradationCount > 0) {
    logger_->warn(CTS,
                  43,
                  "{} wires are pure wire and no slew degradation.\n"
                  "TritonCTS forced slew degradation on these wires.",
                  noSlewDegradationCount);
  }

  logger_->info(
      CTS, 46, "    Number of wire segments: {}.", wireSegments_.size());
  logger_->info(CTS,
                47,
                "    Number of keys in characterization LUT: {}.",
                keyToWireSegments_.size());

  logger_->info(CTS, 48, "    Actual min input cap: {}.", actualMinInputCap_);
}

void TechChar::initLengthUnits()
{
  lengthUnitRatio_ = options_->getWireSegmentUnit() / lengthUnit_;
}

inline void TechChar::reportCharacterizationBounds() const
{
  logger_->report(
      "Min. len    Max. len    Min. cap    Max. cap    Min. slew   Max. slew");

  logger_->report("{:<12}{:<12}{:<12}{:<12}{:<12}{:<12}",
                  minSegmentLength_,
                  maxSegmentLength_,
                  minCapacitance_,
                  maxCapacitance_,
                  minSlew_,
                  maxSlew_);
}

inline void TechChar::checkCharacterizationBounds() const
{
  if (minSegmentLength_ > MAX_NORMALIZED_VAL
      || maxSegmentLength_ > MAX_NORMALIZED_VAL
      || minCapacitance_ > MAX_NORMALIZED_VAL
      || maxCapacitance_ > MAX_NORMALIZED_VAL || minSlew_ > MAX_NORMALIZED_VAL
      || maxSlew_ > MAX_NORMALIZED_VAL) {
    logger_->error(
        CTS,
        65,
        "Normalized values in the LUT should be in the range [1, {}\n"
        "    Check the table above to see the normalization ranges and your "
        "    characterization configuration.",
        std::to_string(MAX_NORMALIZED_VAL));
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
  wireSegments_.emplace_back(
      length, load, outputSlew, power, delay, inputCap, inputSlew);

  const unsigned segmentIdx = wireSegments_.size() - 1;
  const unsigned key = computeKey(length, load, outputSlew);

  if (keyToWireSegments_.find(key) == keyToWireSegments_.end()) {
    keyToWireSegments_[key] = std::deque<unsigned>();
  }

  keyToWireSegments_[key].push_back(segmentIdx);

  return wireSegments_.back();
}

void TechChar::forEachWireSegment(
    const std::function<void(unsigned, const WireSegment&)>& func) const
{
  for (unsigned idx = 0; idx < wireSegments_.size(); ++idx) {
    func(idx, wireSegments_[idx]);
  }
};

void TechChar::forEachWireSegment(
    uint8_t length,
    uint8_t load,
    uint8_t outputSlew,
    const std::function<void(unsigned, const WireSegment&)>& func) const
{
  const unsigned key = computeKey(length, load, outputSlew);

  if (keyToWireSegments_.find(key) != keyToWireSegments_.end()) {
    const std::deque<unsigned>& wireSegmentsIdx = keyToWireSegments_.at(key);
    for (unsigned idx : wireSegmentsIdx) {
      func(idx, wireSegments_[idx]);
    }
  }
}

void TechChar::report() const
{
  logger_->report("\n");
  logger_->report(
      "*********************************************************************");
  logger_->report(
      "*                     Report Characterization                       *");
  logger_->report(
      "*********************************************************************");
  logger_->report(
      "     Idx  Len  Load      Out slew    Power   Delay"
      "   In cap  In slew Buf     Buf Locs");

  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    std::string buffLocs;
    for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
      buffLocs += std::to_string(segment.getBufferLocation(idx)) + " ";
    }

    logger_->report("     {:<5}{:<5}{:<10}{:<12}{:<8}{:<8}{:<8}{:<8}{:<10}{}",
                    idx,
                    segment.getLength(),
                    segment.getLoad(),
                    segment.getOutputSlew(),
                    segment.getPower(),
                    segment.getDelay(),
                    segment.getInputCap(),
                    segment.getInputSlew(),
                    segment.isBuffered(),
                    buffLocs);
  });

  logger_->report(
      "*************************************************************");
}

void TechChar::reportSegments(uint8_t length,
                              uint8_t load,
                              uint8_t outputSlew) const
{
  logger_->report("\n");
  logger_->report(
      "*********************************************************************");
  logger_->report(
      "*                     Report Characterization                       *");
  logger_->report(
      "*********************************************************************");

  logger_->report(
      " Reporting wire segments with length: {} load: {} out slew: {}",
      length,
      load,
      outputSlew);

  logger_->report(
      "     Idx  Len  Load      Out slew    Power   Delay"
      "   In cap  In slew Buf     Buf Locs");

  forEachWireSegment(
      length, load, outputSlew, [&](unsigned idx, const WireSegment& segment) {
        std::string buffLocs;
        for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
          buffLocs += std::to_string(segment.getBufferLocation(idx)) + " ";
        }
        logger_->report(
            "     {:<5}{:<5}{:<10}{:<12}{:<8}{:<8}{:<8}{:<8}{:<10}{}",
            idx,
            segment.getLength(),
            segment.getLoad(),
            segment.getOutputSlew(),
            segment.getPower(),
            segment.getDelay(),
            segment.getInputCap(),
            segment.getInputSlew(),
            segment.isBuffered(),
            buffLocs);
      });
}

void TechChar::printCharacterization() const
{
  logger_->report("minSegmentLength = {}", minSegmentLength_);
  logger_->report("maxSegmentLength = {}", maxSegmentLength_);
  logger_->report("minCapacitance = {}", minCapacitance_);
  logger_->report("maxCapacitance = {}", maxCapacitance_);
  logger_->report("minSlew = {}", minSlew_);
  logger_->report("maxSlew = {}", maxSlew_);
  logger_->report("wireSegmentUnit = {}", options_->getWireSegmentUnit());

  logger_->report(
      "\nidx length load outSlew power delay inCap inSlew pureWire bufLoc");
  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    std::string buffer_locations;
    for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
      buffer_locations += std::to_string(segment.getBufferLocation(idx)) + " ";
    }

    logger_->report("{:6} {:2} {:2} {:2} {:.2e} {:4} {:2} {:2} {} {}",
                    idx,
                    (unsigned) segment.getLength(),
                    (unsigned) segment.getLoad(),
                    (unsigned) segment.getOutputSlew(),
                    segment.getPower(),
                    segment.getDelay(),
                    (unsigned) segment.getInputCap(),
                    (unsigned) segment.getInputSlew(),
                    !segment.isBuffered(),
                    buffer_locations);
  });
}

void TechChar::printSolution() const
{
  logger_->report("idx <length,buffer>*");
  forEachWireSegment([&](unsigned idx, const WireSegment& segment) {
    std::string report = std::to_string(idx) + " ";

    if (segment.isBuffered()) {
      for (unsigned idx = 0; idx < segment.getNumBuffers(); ++idx) {
        const float wirelengthValue
            = segment.getBufferLocation(idx)
              * ((float) (segment.getLength())
                 * (float) (options_->getWireSegmentUnit()));

        report += std::to_string(lround(wirelengthValue));
        report += "," + segment.getBufferMaster(idx);
        if (!(idx + 1 >= segment.getNumBuffers())) {
          report += ",";
        }
      }
    } else {
      const float wirelengthValue = (float) (segment.getLength())
                                    * (float) (options_->getWireSegmentUnit());
      report += std::to_string(lround(wirelengthValue));
    }

    logger_->report("{}", report);
  });
}

void TechChar::createFakeEntries(unsigned length, unsigned fakeLength)
{
  // This condition would just duplicate wires that already exist
  if (length == fakeLength) {
    return;
  }

  logger_->warn(CTS, 45, "Creating fake entries in the LUT.");
  for (unsigned load = 1; load <= getMaxCapacitance(); ++load) {
    for (unsigned outSlew = 1; outSlew <= getMaxSlew(); ++outSlew) {
      forEachWireSegment(
          length, load, outSlew, [&](unsigned key, const WireSegment& seg) {
            const unsigned power = seg.getPower();
            const unsigned delay = seg.getDelay();
            const unsigned inputCap = seg.getInputCap();
            const unsigned inputSlew = seg.getInputSlew();

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

  logger_->report(
      "    Key: {} inSlew: {} inCap: {} outSlew: {} load: {} length: {} delay: "
      "{}",
      key,
      seg.getInputSlew(),
      seg.getInputCap(),
      seg.getOutputSlew(),
      seg.getLoad(),
      seg.getLength(),
      seg.getDelay());

  for (unsigned idx = 0; idx < seg.getNumBuffers(); ++idx) {
    logger_->report("      location: {} buffer: {}",
                    seg.getBufferLocation(idx),
                    seg.getBufferMaster(idx));
  }
}

void TechChar::initClockLayerResCap(float dbUnitsPerMicron)
{
  // Clock RC should be set with set_wire_rc -clock
  sta::Corner* corner = openSta_->cmdCorner();

  // convert from per meter to per dbu
  capPerDBU_ = resizer_->wireClkCapacitance(corner) * 1e-6 / dbUnitsPerMicron;
  resPerDBU_ = resizer_->wireClkResistance(corner) * 1e-6 / dbUnitsPerMicron;

  if (resPerDBU_ == 0.0 || capPerDBU_ == 0.0) {
    logger_->warn(CTS,
                  104,
                  "Clock wire resistance/capacitance values are zero.\nUse "
                  "set_wire_rc to set them.");
  }
}

// Characterization Methods

void TechChar::initCharacterization()
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();
  const float dbUnitsPerMicron = block->getDbUnitsPerMicron();

  initClockLayerResCap(dbUnitsPerMicron);

  // Gets the buffer masters and its in/out pins.
  std::vector<std::string> masterVector = options_->getBufferList();
  if (masterVector.size() < 1) {
    logger_->error(CTS, 73, "Buffer not found. Check your -buf_list input.");
  }
  for (const std::string& masterString : masterVector) {
    odb::dbMaster* testBuf = db_->findMaster(masterString.c_str());
    if (testBuf == nullptr) {
      logger_->error(CTS,
                     74,
                     "Buffer {} not found. Check your -buf_list input.",
                     masterString);
    }
    auto* libertyCell = db_network_->findLibertyCell(masterString.c_str());
    if (libertyCell == nullptr) {
      logger_->error(CTS, 106, "No Liberty found for buffer {}.", masterString);
    }
    masterNames_.insert(masterString);
  }

  float maxBuffCap = 0.0;
  std::string bufMasterName;
  for (int i = 0; i < masterVector.size(); i++) {
    float maxCap = 0.0;
    bool maxCapExist = false;
    odb::dbMaster* buf = db_->findMaster(masterVector[i].c_str());
    sta::Cell* masterCell = db_network_->dbToSta(buf);
    sta::LibertyCell* libertyCell = db_network_->libertyCell(masterCell);
    sta::LibertyPort *input, *output;
    libertyCell->bufferPorts(input, output);
    output->capacitanceLimit(sta::MinMax::max(), maxCap, maxCapExist);
    sta::LibertyLibrary* lib = libertyCell->libertyLibrary();
    if (!maxCapExist)
      lib->defaultMaxCapacitance(maxCap, maxCapExist);
    if (!maxCapExist)
      logger_->error(
          CTS, 111, "No max capacitance found for cell {}.", masterVector[i]);
    if (maxCap >= maxBuffCap) {
      maxBuffCap = maxCap;
      charBuf_ = buf;
      bufMasterName = masterVector[i];
    }
  }

  if (bufMasterName == "") {
    logger_->error(
        CTS,
        113,
        "Characterization buffer is not defined.\n"
        "    Check that -buf_list has supported buffers from platform.");
  } else {
    logger_->info(CTS, 49, "Characterization buffer is: {}.", bufMasterName);
  }

  odb::dbMaster* sinkMaster
      = db_->findMaster(options_->getSinkBuffer().c_str());

  for (odb::dbMTerm* masterTerminal : charBuf_->getMTerms()) {
    if (masterTerminal->getIoType() == odb::dbIoType::INPUT
        && (masterTerminal->getSigType() == odb::dbSigType::SIGNAL
            || masterTerminal->getSigType() == odb::dbSigType::CLOCK)) {
      charBufIn_ = masterTerminal;
    } else if (masterTerminal->getIoType() == odb::dbIoType::OUTPUT
               && masterTerminal->getSigType() == odb::dbSigType::SIGNAL) {
      charBufOut_ = masterTerminal;
    }
  }

  if (!charBufIn_) {
    logger_->error(
        CTS, 534, "Could not find buffer input port for {}.", bufMasterName);
  }

  if (!charBufOut_) {
    logger_->error(
        CTS, 541, "Could not find buffer output port for {}.", bufMasterName);
  }
  // Creates the new characterization block. (Wiresegments are created here
  // instead of the main block)
  const char* characterizationBlockName = "CharacterizationBlock";
  if (auto char_block = block->findChild(characterizationBlockName)) {
    odb::dbBlock::destroy(char_block);
  }
  charBlock_ = odb::dbBlock::create(block, characterizationBlockName);

  // Defines the different wirelengths to test and the characterization unit.
  const unsigned wirelengthIterations = options_->getCharWirelengthIterations();
  unsigned maxWirelength = (charBuf_->getHeight() * 10)
                           * wirelengthIterations;  // Hard-coded limit
  if (options_->getWireSegmentUnit() == 0) {
    const unsigned charaunit = charBuf_->getHeight() * 10;
    options_->setWireSegmentUnit(charaunit);
  } else {
    // Updates the units to DBU.
    const unsigned segmentDistance = options_->getWireSegmentUnit();
    options_->setWireSegmentUnit(segmentDistance * dbUnitsPerMicron);
  }

  // Required to make sure that the fake entry for minLengthSinkRegion
  // exists (see HTreeBuilder::run())
  if (options_->isFakeLutEntriesEnabled()) {
    maxWirelength = std::max(maxWirelength, 2 * options_->getWireSegmentUnit());
  }

  for (unsigned wirelengthInter = options_->getWireSegmentUnit();
       (wirelengthInter <= maxWirelength)
       && (wirelengthInter
           <= wirelengthIterations * options_->getWireSegmentUnit());
       wirelengthInter += options_->getWireSegmentUnit()) {
    wirelengthsToTest_.push_back(wirelengthInter);
  }

  if (wirelengthsToTest_.size() < 1) {
    logger_->error(
        CTS,
        75,
        "Error generating the wirelengths to test.\n"
        "    Check the -wire_unit parameter or the technology files.");
  }

  setLengthUnit(charBuf_->getHeight() * 10 / 2);

  // Gets the max slew and max cap if they weren't added as parameters.
  float maxSlew = 0.0;
  float maxCap = 0.0;
  if (options_->getMaxCharSlew() == 0 || options_->getMaxCharCap() == 0) {
    sta::Cell* masterCell = db_network_->dbToSta(charBuf_);
    sta::Cell* sinkCell = db_network_->dbToSta(sinkMaster);
    sta::LibertyCell* libertyCell = db_network_->libertyCell(masterCell);
    sta::LibertyCell* libertySinkCell = db_network_->libertyCell(sinkCell);
    bool maxSlewExist = false;
    bool maxCapExist = false;

    if (!libertyCell) {
      logger_->error(CTS, 96, "No Liberty cell found for {}.", bufMasterName);
    } else {
      sta::LibertyPort *input, *output;
      libertyCell->bufferPorts(input, output);
      sta::LibertyLibrary* lib = libertyCell->libertyLibrary();

      output->slewLimit(sta::MinMax::max(), maxSlew, maxSlewExist);
      if (!maxSlewExist)
        lib->defaultMaxSlew(maxSlew, maxSlewExist);
      if (!maxSlewExist)
        logger_->error(
            CTS, 107, "No max slew found for cell {}.", bufMasterName);

      output->capacitanceLimit(sta::MinMax::max(), maxCap, maxCapExist);
      if (!maxCapExist)
        lib->defaultMaxCapacitance(maxCap, maxCapExist);
      if (!maxCapExist)
        logger_->error(
            CTS, 108, "No max capacitance found for cell {}.", bufMasterName);
      options_->setMaxCharSlew(maxSlew);
      options_->setMaxCharCap(maxCap);
    }
    if (!libertySinkCell) {
      logger_->error(
          CTS, 76, "No Liberty cell found for {}.", options_->getSinkBuffer());
    } else {
      sta::LibertyPort *input, *output;
      libertySinkCell->bufferPorts(input, output);
      options_->setSinkBufferInputCap(input->capacitance());
    }
  } else {
    maxSlew = options_->getMaxCharSlew();
    maxCap = options_->getMaxCharCap();
  }

  const unsigned slewSteps = options_->getSlewSteps();
  const unsigned loadSteps = options_->getCapSteps();

  charSlewStepSize_ = maxSlew / slewSteps;
  charCapStepSize_ = maxCap / loadSteps;

  // Creates the different slews and loads to test.
  for (int step = 1; step <= slewSteps; ++step) {
    slewsToTest_.push_back(step * charSlewStepSize_);
  }
  for (int step = 1; step <= loadSteps; ++step) {
    loadsToTest_.push_back(step * charCapStepSize_);
  }

  if ((loadsToTest_.size() < 1) || (slewsToTest_.size() < 1)) {
    logger_->error(
        CTS,
        78,
        "Error generating the wirelengths to test.\n"
        "    Check the parameters -max_cap/-max_slew/-cap_inter/-slew_inter\n"
        "          or the technology files.");
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
      = setupWirelength / options_->getWireSegmentUnit();
  const unsigned numberOfTopologies = 1 << numberOfNodes;
  std::vector<SolutionData> topologiesVector;
  odb::dbNet* net = nullptr;

  // For each possible topology...
  for (unsigned solutionCounterInt = 0; solutionCounterInt < numberOfTopologies;
       solutionCounterInt++) {
    // Creates a bitset that represents the buffer locations.
    const std::bitset<5> solutionCounter(solutionCounterInt);
    unsigned short int wireCounter = 0;
    SolutionData topology;
    // Creates the starting net.
    const std::string netName = "net_" + std::to_string(setupWirelength) + "_"
                                + solutionCounter.to_string() + "_"
                                + std::to_string(wireCounter);
    net = odb::dbNet::create(charBlock_, netName.c_str());
    odb::dbWire::create(net);
    net->setSigType(odb::dbSigType::SIGNAL);
    // Creates the input port.
    const std::string inPortName
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
        const std::string bufName = "buf_" + std::to_string(setupWirelength)
                                    + "_" + solutionCounter.to_string() + "_"
                                    + std::to_string(wireCounter);
        odb::dbInst* bufInstance
            = odb::dbInst::create(charBlock_, charBuf_, bufName.c_str());
        odb::dbITerm* bufInstanceInPin = bufInstance->getITerm(charBufIn_);
        odb::dbITerm* bufInstanceOutPin = bufInstance->getITerm(charBufOut_);
        bufInstanceInPin->connect(net);
        // Updates the topology with the old net and number of nodes that didn't
        // have buffers until now.
        topology.netVector.push_back(net);
        topology.nodesWithoutBufVector.push_back(nodesWithoutBuf);
        // Creates a new net.
        wireCounter++;
        const std::string netName = "net_" + std::to_string(setupWirelength)
                                    + "_" + solutionCounter.to_string() + "_"
                                    + std::to_string(wireCounter);
        net = odb::dbNet::create(charBlock_, netName.c_str());
        odb::dbWire::create(net);
        bufInstanceOutPin->connect(net);
        net->setSigType(odb::dbSigType::SIGNAL);
        // Updates the topology wih the new instance and the current topology
        // (as a vector of strings).
        topology.instVector.push_back(bufInstance);
        topology.topologyDescriptor.push_back(
            std::to_string(nodesWithoutBuf * options_->getWireSegmentUnit()));
        topology.topologyDescriptor.push_back(charBuf_->getName());
        nodesWithoutBuf = 0;
        isPureWire = false;
      }
    }
    // Finishing up the topology with the output port.
    const std::string outPortName = "out_" + std::to_string(setupWirelength)
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
          std::to_string(nodesWithoutBuf * options_->getWireSegmentUnit()));
    }
    // Go to the next topology.
    topologiesVector.push_back(topology);
  }
  return topologiesVector;
}

void TechChar::createStaInstance()
{
  // Creates a new OpenSTA instance that is used only for the characterization.
  // Creates the new instance based on the charcterization block.
  openStaChar_ = openSta_->makeBlockSta(charBlock_);
  // Gets the corner and other analysis attributes from the new instance.
  charCorner_ = openStaChar_->cmdCorner();
  sta::PathAPIndex path_ap_index
      = charCorner_->findPathAnalysisPt(sta::MinMax::max())->index();
  sta::Corners* corners = openStaChar_->search()->corners();
  charPathAnalysis_ = corners->findPathAnalysisPt(path_ap_index);
}

void TechChar::setParasitics(
    const std::vector<TechChar::SolutionData>& topologiesVector,
    unsigned setupWirelength)
{
  // For each topology...
  for (const SolutionData& solution : topologiesVector) {
    // For each net in the topolgy -> set the parasitics.
    for (unsigned netIndex = 0; netIndex < solution.netVector.size();
         ++netIndex) {
      // Gets the ITerms (instance pins) and BTerms (other high-level pins) from
      // the current net.
      odb::dbNet* net = solution.netVector[netIndex];
      const unsigned nodesWithoutBuf = solution.nodesWithoutBufVector[netIndex];
      odb::dbBTerm* inBTerm = solution.inPort->getBTerm();
      odb::dbBTerm* outBTerm = solution.outPort->getBTerm();
      odb::dbSet<odb::dbBTerm> netBTerms = net->getBTerms();
      odb::dbSet<odb::dbITerm> netITerms = net->getITerms();
      sta::Pin* firstPin = nullptr;
      sta::Pin* lastPin = nullptr;
      // Gets the sta::Pin from the beginning and end of the net.
      if (netBTerms.size() > 1) {  // Parasitics for a purewire segment.
                                   // First and last pin are already available.
        firstPin = db_network_->dbToSta(inBTerm);
        lastPin = db_network_->dbToSta(outBTerm);
      } else if (netBTerms.size()
                 == 1) {  // Parasitics for the end/start of a net.
                          // One Port and one instance pin.
        odb::dbBTerm* netBTerm = net->get1stBTerm();
        odb::dbITerm* netITerm = net->get1stITerm();
        if (netBTerm == inBTerm) {
          firstPin = db_network_->dbToSta(netBTerm);
          lastPin = db_network_->dbToSta(netITerm);
        } else {
          firstPin = db_network_->dbToSta(netITerm);
          lastPin = db_network_->dbToSta(netBTerm);
        }
      } else {  // Parasitics for a net that is between two buffers. Need to
                // iterate over the net ITerms.
        for (odb::dbITerm* iterm : netITerms) {
          if (iterm != nullptr) {
            if (iterm->getIoType() == odb::dbIoType::INPUT) {
              lastPin = db_network_->dbToSta(iterm);
            }

            if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
              firstPin = db_network_->dbToSta(iterm);
            }

            if (firstPin != nullptr && lastPin != nullptr) {
              break;
            }
          }
        }
      }
      // Sets the Pi and Elmore information.
      const unsigned charUnit = options_->getWireSegmentUnit();
      const double wire_cap = nodesWithoutBuf * charUnit * capPerDBU_;
      const double wire_res = nodesWithoutBuf * charUnit * resPerDBU_;
      openStaChar_->makePiElmore(firstPin,
                                 sta::RiseFall::rise(),
                                 sta::MinMaxAll::all(),
                                 wire_cap / 2,
                                 wire_res,
                                 wire_cap / 2);
      openStaChar_->setElmore(firstPin,
                              lastPin,
                              sta::RiseFall::rise(),
                              sta::MinMaxAll::all(),
                              wire_res * wire_cap);
    }
  }
}

TechChar::ResultData TechChar::computeTopologyResults(
    const TechChar::SolutionData& solution,
    sta::Vertex* outPinVert,
    float load,
    float inSlew,
    unsigned setupWirelength)
{
  ResultData results;
  results.wirelength = setupWirelength;
  results.topology = solution.topologyDescriptor;
  results.isPureWire = solution.isPureWire;
  results.load = load;
  results.inSlew = inSlew;
  // Computations for power, requires the PowerResults class from OpenSTA.
  float totalPower = 0;
  if (!solution.isPureWire) {
    // If it isn't a pure wire solution, get the sum of the total power of each
    // buffer.
    for (odb::dbInst* bufferInst : solution.instVector) {
      sta::Instance* bufferInstSta = db_network_->dbToSta(bufferInst);
      sta::PowerResult instResults
          = openStaChar_->power(bufferInstSta, charCorner_);
      totalPower = totalPower + instResults.total();
    }
  }
  results.totalPower = totalPower;
  // Computations for input capacitance.
  float incap = 0;
  if (solution.isPureWire) {
    // For pure-wire, sum of the current load with the capacitance of the net.
    incap = load + (setupWirelength * capPerDBU_);
  } else {
    // For buffered solutions, add the cap of the input of the first buffer
    // with the capacitance of the left-most net.
    float length = std::stod(solution.topologyDescriptor[0]);
    sta::LibertyCell* firstInstLiberty = db_network_->libertyCell(
        db_network_->dbToSta(solution.instVector[0]));
    sta::LibertyPort* firstPinLiberty
        = firstInstLiberty->findLibertyPort(charBufIn_->getName().c_str());
    float firstPinCap = firstPinLiberty->capacitance();
    incap = firstPinCap + length * capPerDBU_;
  }
  const float totalcap
      = std::round(incap / charCapStepSize_) * charCapStepSize_;
  results.totalcap = totalcap;
  // Computations for delay.
  const float pinArrival = openStaChar_->vertexArrival(
      outPinVert, sta::RiseFall::fall(), charPathAnalysis_);
  results.pinArrival = pinArrival;
  // Computations for output slew.
  const float pinRise = openStaChar_->vertexSlew(
      outPinVert, sta::RiseFall::rise(), sta::MinMax::max());
  const float pinFall = openStaChar_->vertexSlew(
      outPinVert, sta::RiseFall::fall(), sta::MinMax::max());
  const float pinSlew = std::round((pinRise + pinFall) / 2 / charSlewStepSize_)
                        * charSlewStepSize_;
  results.pinSlew = pinSlew;

  return results;
}

void TechChar::updateBufferTopologies(TechChar::SolutionData& solution)
{
  unsigned index = 0;
  // Change the buffer topology by increasing the size of the buffers.
  // After testing all the sizes for the current buffer, increment the size of
  // the next one (works like a carry mechanism). Ex for 4 different buffers:
  // 103-> 110 -> 111 -> 112 -> 113 -> 120 ...
  bool done = false;
  while (!done) {
    // Gets the iterator to the beggining of the masterNames_ set.
    std::set<std::string>::iterator masterItr
        = masterNames_.find(solution.instVector[index]->getMaster()->getName());
    // Gets the iterator to the end of the masterNames_ set.
    std::set<std::string>::iterator masterFinalItr = masterNames_.end();
    masterFinalItr--;
    if (masterItr == masterFinalItr) {
      // If the iterator can't increment past the final iterator...
      // change the current buf master to the charBuf_ and try to go to next
      // instance.
      odb::dbInst* inst = solution.instVector[index];
      inst->swapMaster(charBuf_);
      unsigned topologyCounter = 0;
      for (unsigned topologyIndex = 0;
           topologyIndex < solution.topologyDescriptor.size();
           topologyIndex++) {
        // Iterates through the topologyDescriptor to set the new information
        //(string representing the current buffer)
        std::string topologyS = solution.topologyDescriptor[topologyIndex];
        if (!(masterNames_.find(topologyS) == masterNames_.end())) {
          if (topologyCounter == index) {
            std::set<std::string>::iterator firstMaster = masterNames_.begin();
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
      const std::string masterString = *masterItr;
      odb::dbMaster* newBufMaster = db_->findMaster(masterString.c_str());
      odb::dbInst* inst = solution.instVector[index];
      inst->swapMaster(newBufMaster);
      unsigned topologyCounter = 0;
      for (unsigned topologyIndex = 0;
           topologyIndex < solution.topologyDescriptor.size();
           topologyIndex++) {
        const std::string topologyS
            = solution.topologyDescriptor[topologyIndex];
        if (!(masterNames_.find(topologyS) == masterNames_.end())) {
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
}

std::vector<TechChar::ResultData> TechChar::characterizationPostProcess()
{
  // Post-process of the characterization results.
  std::vector<ResultData> selectedSolutions;
  // Select only a subset of the total results. If, for a combination of input
  // cap, wirelength, load and output slew, more than 3 results exists -> select
  // only 3 of them.
  for (auto& keyResults : solutionMap_) {
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
    if (solution.pinSlew <= options_->getMaxCharSlew()) {
      ResultData convertedResult;
      // Processing and normalizing of output slew.
      convertedResult.pinSlew = normalizeCharResults(
          solution.pinSlew, charSlewStepSize_, &minResultSlew, &maxResultSlew);
      // Processing and normalizing of input slew.
      convertedResult.inSlew = normalizeCharResults(
          solution.inSlew, charSlewStepSize_, &minResultSlew, &maxResultSlew);
      // Processing and normalizing of input cap.
      convertedResult.totalcap = normalizeCharResults(solution.totalcap,
                                                      charCapStepSize_,
                                                      &minResultCapacitance,
                                                      &maxResultCapacitance);
      // Processing and normalizing of load.
      convertedResult.load = normalizeCharResults(solution.load,
                                                  charCapStepSize_,
                                                  &minResultCapacitance,
                                                  &maxResultCapacitance);
      // Processing and normalizing of the wirelength.
      convertedResult.wirelength
          = normalizeCharResults(solution.wirelength,
                                 options_->getWireSegmentUnit(),
                                 &minResultWirelength,
                                 &maxResultWirelength);
      // Processing and normalizing of delay.
      convertedResult.pinArrival
          = std::ceil(solution.pinArrival / (charSlewStepSize_ / 5));
      // Add missing information.
      convertedResult.totalPower = solution.totalPower;
      convertedResult.isPureWire = solution.isPureWire;
      std::vector<std::string> topologyResult;
      for (int topologyIndex = 0; topologyIndex < solution.topology.size();
           topologyIndex++) {
        std::string topologyS = solution.topology[topologyIndex];
        // Normalizes the strings that represents the topology too.
        if (masterNames_.find(topologyS) == masterNames_.end()) {
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
  minSlew_ = minResultSlew;
  maxSlew_ = maxResultSlew;
  minCapacitance_ = minResultCapacitance;
  maxCapacitance_ = maxResultCapacitance;
  minSegmentLength_ = minResultWirelength;
  maxSegmentLength_ = maxResultWirelength;
  return convertedSolutions;
}

unsigned TechChar::normalizeCharResults(float value,
                                        float iter,
                                        unsigned* min,
                                        unsigned* max)
{
  unsigned normVal = std::ceil(value / iter);
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
  for (unsigned setupWirelength : wirelengthsToTest_) {
    // Creates the topologies for the current wirelength.
    std::vector<SolutionData> topologiesVector
        = createPatterns(setupWirelength);
    // Creates an OpenSTA instance.
    createStaInstance();
    // Setup of the parasitics for each net.
    setParasitics(topologiesVector, setupWirelength);
    // For each topology...
    sta::Graph* graph = openStaChar_->ensureGraph();
    for (SolutionData solution : topologiesVector) {
      // Gets the input and output port (as terms, pins and vertices).
      odb::dbBTerm* inBTerm = solution.inPort->getBTerm();
      odb::dbBTerm* outBTerm = solution.outPort->getBTerm();
      odb::dbNet* lastNet = solution.netVector.back();
      sta::Pin* inPin = db_network_->dbToSta(inBTerm);
      sta::Pin* outPin = db_network_->dbToSta(outBTerm);
      sta::Vertex* outPinVert = graph->pinLoadVertex(outPin);
      sta::Vertex* inPinVert = graph->pinDrvrVertex(inPin);

      // Gets the first pin of the last net. Needed to set a new parasitic
      // (load) value.
      sta::Pin* firstPinLastNet = nullptr;
      if (lastNet->getBTerms().size() > 1) {
        // Parasitics for purewire segment.
        // First and last pin are already available.
        firstPinLastNet = inPin;
      } else {
        // Parasitics for the end/start of a net. One Port and one
        // instance pin.
        odb::dbITerm* netITerm = lastNet->get1stITerm();
        firstPinLastNet = db_network_->dbToSta(netITerm);
      }

      float c1, c2, r1;
      bool piExists = false;
      // Gets the parasitics that are currently used for the last net.
      openStaChar_->findPiElmore(firstPinLastNet,
                                 sta::RiseFall::rise(),
                                 sta::MinMax::max(),
                                 c2,
                                 r1,
                                 c1,
                                 piExists);
      // For each possible buffer combination (different sizes).
      unsigned buffersUpdate
          = std::pow(masterNames_.size(), solution.instVector.size());
      do {
        // For each possible load.
        for (float load : loadsToTest_) {
          // Sets the new parasitic of the last net (load added to last pin).
          openStaChar_->makePiElmore(firstPinLastNet,
                                     sta::RiseFall::rise(),
                                     sta::MinMaxAll::all(),
                                     c2,
                                     r1,
                                     c1 + load);
          openStaChar_->setElmore(firstPinLastNet,
                                  outPin,
                                  sta::RiseFall::rise(),
                                  sta::MinMaxAll::all(),
                                  r1 * (c1 + c2 + load));
          // For each possible input slew.
          for (float inputslew : slewsToTest_) {
            // Sets the slew on the input vertex.
            // Here the new pattern is created (combination of load, buffers and
            // slew values).
            openStaChar_->setAnnotatedSlew(inPinVert,
                                           charCorner_,
                                           sta::MinMaxAll::all(),
                                           sta::RiseFallBoth::riseFall(),
                                           inputslew);
            // Updates timing for the new pattern.
            openStaChar_->updateTiming(true);

            // Gets the results (delay, slew, power...) for the pattern.
            ResultData results = computeTopologyResults(
                solution, outPinVert, load, inputslew, setupWirelength);

            // Appends the results to a map, grouping each result by
            // wirelength, load, output slew and input cap.
            CharKey solutionKey;
            solutionKey.wirelength = results.wirelength;
            solutionKey.pinSlew = results.pinSlew;
            solutionKey.load = results.load;
            solutionKey.totalcap = results.totalcap;
            if (solutionMap_.find(solutionKey) != solutionMap_.end()) {
              solutionMap_[solutionKey].push_back(results);
            } else {
              std::vector<ResultData> resultGroup;
              resultGroup.push_back(results);
              solutionMap_[solutionKey] = resultGroup;
            }
            topologiesCreated++;
            if (topologiesCreated % 50000 == 0) {
              logger_->info(CTS,
                            38,
                            "Number of created patterns = {}.",
                            topologiesCreated);
            }
          }
        }
        // If the solution is not a pure-wire, update the buffer topologies.
        if (!solution.isPureWire) {
          updateBufferTopologies(solution);
        }
        // For pure-wire solution buffersUpdate == 1, so it only runs once.
        buffersUpdate--;
      } while (buffersUpdate != 0);
    }
    openStaChar_.reset(nullptr);
  }
  logger_->info(CTS, 39, "Number of created patterns = {}.", topologiesCreated);
  // Post-processing of the results.
  const std::vector<ResultData> convertedSolutions
      = characterizationPostProcess();
  compileLut(convertedSolutions);
  if (logger_->debugCheck(CTS, "characterization", 3)) {
    printCharacterization();
    printSolution();
  }
  odb::dbBlock::destroy(charBlock_);
}

}  // namespace cts
