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

#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "CtsOptions.h"
#include "db_sta/dbNetwork.hh"
#include "ord/OpenRoad.hh"
#include "sta/Corner.hh"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

using utl::Logger;

class WireSegment
{
  uint8_t length_;
  uint8_t load_;
  uint8_t outputSlew_;

  double power_;
  unsigned delay_;
  uint8_t inputCap_;
  uint8_t inputSlew_;

  std::vector<double> bufferLocations_;
  std::vector<std::string> bufferMasters_;

 public:
  WireSegment(uint8_t length,
              uint8_t load,
              uint8_t outputSlew,
              double power,
              unsigned delay,
              uint8_t inputCap,
              uint8_t inputSlew)
      : length_(length),
        load_(load),
        outputSlew_(outputSlew),
        power_(power),
        delay_(delay),
        inputCap_(inputCap),
        inputSlew_(inputSlew)
  {
  }

  void addBuffer(double location) { bufferLocations_.push_back(location); }

  void addBufferMaster(std::string name) { bufferMasters_.push_back(name); }

  double getPower() const { return power_; }
  unsigned getDelay() const { return delay_; }
  uint8_t getInputCap() const { return inputCap_; }
  uint8_t getInputSlew() const { return inputSlew_; }
  uint8_t getLength() const { return length_; }
  uint8_t getLoad() const { return load_; }
  uint8_t getOutputSlew() const { return outputSlew_; }
  bool isBuffered() const { return bufferLocations_.size() > 0; }
  unsigned getNumBuffers() const { return bufferLocations_.size(); }
  const std::vector<double>& getBufferLocations() { return bufferLocations_; }
  const std::vector<std::string>& getBufferMasters() { return bufferMasters_; }

  double getBufferLocation(unsigned idx) const
  {
    if (idx >= bufferLocations_.size()) {
      return -1.0;
    }
    return bufferLocations_[idx];
  }

  std::string getBufferMaster(unsigned idx) const
  {
    assert(idx >= 0 || idx < bufferMasters_.size());
    return bufferMasters_[idx];
  }
};

//-----------------------------------------------------------------------------

class TechChar
{
 public:
  TechChar(CtsOptions* options,
           ord::OpenRoad* openroad,
           odb::dbDatabase* db,
           sta::dbSta* sta,
           rsz::Resizer* resizer,
           sta::dbNetwork* db_network,
           Logger* logger);

  typedef uint32_t Key;
  static constexpr unsigned NUM_BITS_PER_FIELD = 10;
  static constexpr unsigned MAX_NORMALIZED_VAL = (1 << NUM_BITS_PER_FIELD) - 1;
  unsigned LENGTH_UNIT_MICRON = 10;

  // SolutionData represents the various different structures of the
  // characterization segment. Ports, insts, nets...
  struct SolutionData
  {
    std::vector<odb::dbNet*> netVector;
    std::vector<unsigned int> nodesWithoutBufVector;
    odb::dbBPin* inPort;
    odb::dbBPin* outPort;
    std::vector<odb::dbInst*> instVector;
    std::vector<std::string> topologyDescriptor;
    bool isPureWire = true;
  };

  // ResultData represents the resulting metrics for a specific characterization
  // segment. The topology object helps on reconstructing that segment.
  struct ResultData
  {
    float load;
    float inSlew;
    float wirelength;
    float pinSlew;
    float pinArrival;
    float totalcap;
    float totalPower;
    bool isPureWire;
    std::vector<std::string> topology;
  };

  // ResultData represents the resulting metrics for a specific characterization
  // segment. The topology object helps on reconstructing that segment.
  struct CharKey
  {
    float load;
    float wirelength;
    float pinSlew;
    float totalcap;

    bool operator<(const CharKey& o) const
    {
      return std::tie(load, wirelength, pinSlew, totalcap)
             < std::tie(o.load, o.wirelength, o.pinSlew, o.totalcap);
    }
  };

  void create();
  void compileLut(const std::vector<ResultData>& lutSols);
  void printCharacterization() const;
  void printSolution() const;

  void report() const;
  void reportSegment(unsigned key) const;
  void reportSegments(uint8_t length, uint8_t load, uint8_t outputSlew) const;

  void forEachWireSegment(
      const std::function<void(unsigned, const WireSegment&)> func) const;

  void forEachWireSegment(
      uint8_t length,
      uint8_t load,
      uint8_t outputSlew,
      const std::function<void(unsigned, const WireSegment&)> func) const;

  WireSegment& createWireSegment(uint8_t length,
                                 uint8_t load,
                                 uint8_t outputSlew,
                                 double power,
                                 unsigned delay,
                                 uint8_t inputCap,
                                 uint8_t inputSlew);

  const WireSegment& getWireSegment(unsigned idx) const
  {
    return wireSegments_[idx];
  }

  unsigned getMinSegmentLength() const { return minSegmentLength_; }
  unsigned getMaxSegmentLength() const { return maxSegmentLength_; }
  unsigned getMaxCapacitance() const { return maxCapacitance_; }
  unsigned getMaxSlew() const { return maxSlew_; }
  void setActualMinInputCap(unsigned cap) { actualMinInputCap_ = cap; }
  unsigned getActualMinInputCap() const { return actualMinInputCap_; }
  void setLenghthUnit(unsigned length) { LENGTH_UNIT_MICRON = length; }
  unsigned getLengthUnit() const { return lengthUnit_; }

  void createFakeEntries(unsigned length, unsigned fakeLength);

  unsigned computeKey(uint8_t length, uint8_t load, uint8_t outputSlew) const
  {
    return length | (load << NUM_BITS_PER_FIELD)
           | (outputSlew << 2 * NUM_BITS_PER_FIELD);
  }

  float getCharMaxCap() const { return charMaxCap_; }
  double getCapPerDBU() const { return capPerDBU_; }
  float getCharMaxSlew() const { return charMaxSlew_; }
  utl::Logger* getLogger() { return options_->getLogger(); }

 protected:
  void initLengthUnits();
  void reportCharacterizationBounds() const;
  void checkCharacterizationBounds() const;

  unsigned toInternalLengthUnit(unsigned length)
  {
    return length * lengthUnitRatio_;
  }

  unsigned lengthUnit_ = 0;
  unsigned charLengthUnit_ = 0;
  unsigned lengthUnitRatio_ = 0;

  unsigned minSegmentLength_ = 0;
  unsigned maxSegmentLength_ = 0;
  unsigned minCapacitance_ = 0;
  unsigned maxCapacitance_ = 0;
  unsigned minSlew_ = 0;
  unsigned maxSlew_ = 0;

  unsigned actualMinInputCap_ = 0;
  unsigned actualMinInputSlew_ = 0;

  std::deque<WireSegment> wireSegments_;
  std::unordered_map<Key, std::deque<unsigned>> keyToWireSegments_;

  // Characterization attributes

  void initCharacterization();
  std::vector<SolutionData> createPatterns(unsigned setupWirelength);
  void createStaInstance();
  void setParasitics(const std::vector<SolutionData>& topologiesVector,
                     unsigned setupWirelength);
  ResultData computeTopologyResults(const SolutionData& solution,
                                    sta::Vertex* outPinVert,
                                    float load,
                                    float inSlew,
                                    unsigned setupWirelength);
  void updateBufferTopologies(SolutionData& solution);
  std::vector<ResultData> characterizationPostProcess();
  unsigned normalizeCharResults(float value,
                                float iter,
                                unsigned* min,
                                unsigned* max);
  void getClockLayerResCap(float dbUnitsPerMicron);

  CtsOptions* options_;
  ord::OpenRoad* openroad_;
  odb::dbDatabase* db_;
  rsz::Resizer* resizer_;
  sta::dbSta* openSta_;
  sta::dbSta* openStaChar_;
  sta::dbNetwork* db_network_;
  Logger* logger_;
  sta::PathAnalysisPt* charPathAnalysis_ = nullptr;
  sta::Corner* charCorner_ = nullptr;
  odb::dbBlock* charBlock_ = nullptr;
  odb::dbMaster* charBuf_ = nullptr;
  std::string charBufIn_ = "";
  std::string charBufOut_ = "";
  double resPerDBU_;  // ohms/dbu
  double capPerDBU_;  // farads/dbu
  float charMaxSlew_ = 0.0;
  float charMaxCap_ = 0.0;
  float charSlewInter_ = 5.0e-12;  // Hard-coded interval
  float charCapInter_ = 5.0e-15;
  std::set<std::string> masterNames_;
  std::vector<float> wirelengthsToTest_;
  std::vector<float> loadsToTest_;
  std::vector<float> slewsToTest_;

  std::map<CharKey, std::vector<ResultData>> solutionMap_;
};

}  // namespace cts
