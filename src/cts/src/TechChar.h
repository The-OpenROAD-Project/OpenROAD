// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CtsOptions.h"
#include "boost/functional/hash.hpp"
#include "boost/unordered/unordered_map.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace cts {

struct PairHash
{
  std::size_t operator()(const std::pair<size_t, size_t>& iPair) const
  {
    return boost::hash_value(iPair);
  }
};

struct PairEqual
{
  bool operator()(const std::pair<size_t, size_t>& p1,
                  const std::pair<size_t, size_t>& p2) const
  {
    return ((p1.first == p2.first) && (p1.second == p2.second));
  }
};

class WireSegment
{
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

  void addBufferMaster(const std::string& name)
  {
    bufferMasters_.push_back(name);
  }

  void setWl2FirstBuffer(int wl) { wl2FirstBuffer_ = wl; }
  void setLastWl(int wl) { lastWl_ = wl; }

  double getPower() const { return power_; }
  unsigned getDelay() const { return delay_; }
  uint8_t getInputCap() const { return inputCap_; }
  uint8_t getInputSlew() const { return inputSlew_; }
  uint8_t getLength() const { return length_; }
  uint8_t getLoad() const { return load_; }
  uint8_t getOutputSlew() const { return outputSlew_; }
  int getWl2FirstBuffer() const { return wl2FirstBuffer_; }
  int getLastWl() const { return lastWl_; }
  bool isBuffered() const { return !bufferLocations_.empty(); }
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

  const std::string& getBufferMaster(unsigned idx) const
  {
    assert(idx >= 0 || idx < bufferMasters_.size());
    return bufferMasters_[idx];
  }

 private:
  uint8_t length_;
  uint8_t load_;
  uint8_t outputSlew_;

  double power_;
  unsigned delay_;
  uint8_t inputCap_;
  uint8_t inputSlew_;

  std::vector<double> bufferLocations_;
  std::vector<std::string> bufferMasters_;
  int wl2FirstBuffer_ = 0;
  int lastWl_ = 0;
};

//-----------------------------------------------------------------------------

class TechChar
{
 public:
  TechChar(CtsOptions* options,
           odb::dbDatabase* db,
           sta::dbSta* sta,
           rsz::Resizer* resizer,
           est::EstimateParasitics* estimate_parasitics,
           sta::dbNetwork* db_network,
           utl::Logger* logger);

  void create();

  void report() const;
  void reportSegment(unsigned key) const;
  void reportSegments(uint8_t length, uint8_t load, uint8_t outputSlew) const;

  void forEachWireSegment(
      const std::function<void(unsigned, const WireSegment&)>& func) const;

  void forEachWireSegment(
      uint8_t length,
      uint8_t load,
      uint8_t outputSlew,
      const std::function<void(unsigned, const WireSegment&)>& func) const;

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
  unsigned getLengthUnit() const { return lengthUnit_; }

  void createFakeEntries(unsigned length, unsigned fakeLength);

  double getCapPerDBU() const { return capPerDBU_; }
  utl::Logger* getLogger() { return options_->getLogger(); }

 private:
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

  using Key = uint32_t;

  void printCharacterization() const;
  void printSolution() const;

  WireSegment& createWireSegment(uint8_t length,
                                 uint8_t load,
                                 uint8_t outputSlew,
                                 double power,
                                 unsigned delay,
                                 uint8_t inputCap,
                                 uint8_t inputSlew);

  void compileLut(const std::vector<ResultData>& lutSols);
  void setLengthUnit(unsigned length) { lengthUnit_ = length; }
  unsigned computeKey(uint8_t length, uint8_t load, uint8_t outputSlew) const
  {
    return length | (load << NUM_BITS_PER_FIELD)
           | (outputSlew << 2 * NUM_BITS_PER_FIELD);
  }

  void initLengthUnits();
  void reportCharacterizationBounds() const;
  void checkCharacterizationBounds() const;

  unsigned toInternalLengthUnit(unsigned length)
  {
    return length * lengthUnitRatio_;
  }

  // Characterization attributes

  void initCharacterization();
  void finalizeRootSinkBuffers();
  void trimSortBufferList(std::vector<std::string>& buffers);
  float getMaxCapLimit(const std::string& buf);
  void collectSlewsLoadsFromTableAxis(sta::LibertyCell* libCell,
                                      sta::LibertyPort* input,
                                      sta::LibertyPort* output,
                                      std::vector<float>& axisSlews,
                                      std::vector<float>& axisLoads);
  void sortAndUniquify(std::vector<float>& values, const std::string& name);
  void reduceOrExpand(std::vector<float>& values, unsigned limit);
  std::vector<float>::iterator smallestDiffIter(std::vector<float>& values);
  std::vector<float>::iterator largestDiffIter(std::vector<float>& values);
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
  void updateBufferTopologiesOld(TechChar::SolutionData& solution);
  size_t cellNameToID(const std::string& masterName);
  std::vector<size_t> getCurrConfig(const SolutionData& solution);
  std::vector<size_t> getNextConfig(const std::vector<size_t>& currConfig);
  odb::dbMaster* getMasterFromConfig(std::vector<size_t> nextConfig,
                                     unsigned nodeIndex);
  void swapTopologyBuffer(SolutionData& solution,
                          unsigned nodeIndex,
                          const std::string& newMasterName);
  std::vector<ResultData> characterizationPostProcess();
  unsigned normalizeCharResults(float value,
                                float iter,
                                unsigned* min,
                                unsigned* max);
  void initClockLayerResCap(float dbUnitsPerMicron);
  unsigned getBufferingCombo(size_t numBuffers, size_t numNodes);
  bool isTopologyMonotonic(const std::vector<size_t>& row);

  static constexpr unsigned NUM_BITS_PER_FIELD = 10;
  static constexpr unsigned MAX_NORMALIZED_VAL = (1 << NUM_BITS_PER_FIELD) - 1;

  unsigned lengthUnit_ = 0;
  unsigned lengthUnitRatio_ = 0;

  unsigned minSegmentLength_ = 0;
  unsigned maxSegmentLength_ = 0;
  unsigned minCapacitance_ = 0;
  unsigned maxCapacitance_ = 0;
  unsigned minSlew_ = 0;
  unsigned maxSlew_ = 0;

  unsigned actualMinInputCap_ = 0;

  std::deque<WireSegment> wireSegments_;
  std::unordered_map<Key, std::deque<unsigned>> keyToWireSegments_;

  CtsOptions* options_;
  odb::dbDatabase* db_;
  rsz::Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
  sta::dbSta* openSta_;
  std::unique_ptr<sta::dbSta> openStaChar_;
  sta::dbNetwork* db_network_;
  utl::Logger* logger_;
  sta::Scene* charCorner_ = nullptr;
  odb::dbBlock* charBlock_ = nullptr;
  odb::dbMaster* charBuf_ = nullptr;
  odb::dbMTerm* charBufIn_ = nullptr;
  odb::dbMTerm* charBufOut_ = nullptr;
  double resPerDBU_;  // ohms/dbu
  double capPerDBU_;  // farads/dbu
  float charSlewStepSize_ = 0.0;
  float charCapStepSize_ = 0.0;
  std::vector<std::string> masterNames_;
  std::vector<float> wirelengthsToTest_;
  std::vector<float> loadsToTest_;
  std::vector<float> slewsToTest_;

  std::map<CharKey, std::vector<ResultData>> solutionMap_;
  // keep track of acceptable buffering combinations in topology
  boost::unordered_map<std::pair<size_t, size_t>, unsigned, PairHash, PairEqual>
      bufferingComboTable_;
};

}  // namespace cts
