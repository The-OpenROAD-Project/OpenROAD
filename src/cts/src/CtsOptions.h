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

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "CtsObserver.h"
#include "Util.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace stt {
class SteinerTreeBuilder;
}
namespace cts {

class CtsOptions
{
 public:
  CtsOptions(utl::Logger* logger, stt::SteinerTreeBuilder* sttBuildder)
      : logger_(logger), sttBuilder_(sttBuildder)
  {
  }

  void setClockNets(const std::string& clockNets) { clockNets_ = clockNets; }
  std::string getClockNets() const { return clockNets_; }
  void setRootBuffer(const std::string& buffer) { rootBuffer_ = buffer; }
  std::string getRootBuffer() const { return rootBuffer_; }
  void setBufferList(const std::vector<std::string>& buffers)
  {
    bufferList_ = buffers;
  }
  std::vector<std::string> getBufferList() const { return bufferList_; }
  void setDbUnits(int units) { dbUnits_ = units; }
  int getDbUnits() const { return dbUnits_; }
  void setWireSegmentUnit(unsigned wireSegmentUnit)
  {
    wireSegmentUnit_ = wireSegmentUnit;
  }
  unsigned getWireSegmentUnit() const { return wireSegmentUnit_; }
  void setPlotSolution(bool plot) { plotSolution_ = plot; }
  bool getPlotSolution() const { return plotSolution_; }

  void setObserver(std::unique_ptr<CtsObserver> observer)
  {
    observer_ = std::move(observer);
  }
  CtsObserver* getObserver() const { return observer_.get(); }

  void setSinkClustering(bool enable) { sinkClusteringEnable_ = enable; }
  bool getSinkClustering() const { return sinkClusteringEnable_; }
  void setSinkClusteringUseMaxCap(bool useMaxCap)
  {
    sinkClusteringUseMaxCap_ = useMaxCap;
  }
  bool getSinkClusteringUseMaxCap() const { return sinkClusteringUseMaxCap_; }
  void setNumMaxLeafSinks(unsigned numSinks) { numMaxLeafSinks_ = numSinks; }
  unsigned getNumMaxLeafSinks() const { return numMaxLeafSinks_; }
  void setMaxSlew(unsigned slew) { maxSlew_ = slew; }
  unsigned getMaxSlew() const { return maxSlew_; }
  void setMaxCharSlew(double slew) { maxCharSlew_ = slew; }
  double getMaxCharSlew() const { return maxCharSlew_; }
  void setMaxCharCap(double cap) { maxCharCap_ = cap; }
  double getMaxCharCap() const { return maxCharCap_; }
  void setCharWirelengthIterations(unsigned wirelengthIterations)
  {
    charWirelengthIterations_ = wirelengthIterations;
  }
  unsigned getCharWirelengthIterations() const
  {
    return charWirelengthIterations_;
  }
  void setCapSteps(int steps) { capSteps_ = steps; }
  int getCapSteps() const { return capSteps_; }
  void setSlewSteps(int steps) { slewSteps_ = steps; }
  int getSlewSteps() const { return slewSteps_; }
  void setClockTreeMaxDepth(unsigned depth) { clockTreeMaxDepth_ = depth; }
  unsigned getClockTreeMaxDepth() const { return clockTreeMaxDepth_; }
  void setEnableFakeLutEntries(bool enable) { enableFakeLutEntries_ = enable; }
  unsigned isFakeLutEntriesEnabled() const { return enableFakeLutEntries_; }
  void setForceBuffersOnLeafLevel(bool force)
  {
    forceBuffersOnLeafLevel_ = force;
  }
  bool forceBuffersOnLeafLevel() const { return forceBuffersOnLeafLevel_; }
  void setBufDistRatio(double ratio) { bufDistRatio_ = ratio; }
  double getBufDistRatio() { return bufDistRatio_; }
  void setClockNetsObjs(const std::vector<odb::dbNet*>& nets)
  {
    clockNetsObjs_ = nets;
  }
  std::vector<odb::dbNet*> getClockNetsObjs() const { return clockNetsObjs_; }
  void setMetricsFile(const std::string& metricFile)
  {
    metricFile_ = metricFile;
  }
  std::string getMetricsFile() const { return metricFile_; }
  void setNumClockRoots(unsigned roots) { clockRoots_ = roots; }
  int getNumClockRoots() const { return clockRoots_; }
  void setNumClockSubnets(int nets) { clockSubnets_ = nets; }
  int getNumClockSubnets() const { return clockSubnets_; }
  void setNumBuffersInserted(int buffers) { buffersInserted_ = buffers; }
  int getNumBuffersInserted() const { return buffersInserted_; }
  void setNumSinks(int sinks) { sinks_ = sinks; }
  int getNumSinks() const { return sinks_; }
  void setTreeBuffer(const std::string& buffer) { treeBuffer_ = buffer; }
  std::string getTreeBuffer() const { return treeBuffer_; }
  unsigned getClusteringPower() const { return clusteringPower_; }
  void setClusteringPower(unsigned power) { clusteringPower_ = power; }
  double getClusteringCapacity() const { return clusteringCapacity_; }
  void setClusteringCapacity(double capacity)
  {
    clusteringCapacity_ = capacity;
  }

  void setMaxFanout(unsigned maxFanout) { maxFanout_ = maxFanout; }
  unsigned getMaxFanout() const { return maxFanout_; }

  // BufferDistance is in DBU
  int32_t getBufferDistance() const
  {
    if (bufDistance_) {
      return *bufDistance_;
    }

    if (dbUnits_ == -1) {
      logger_->error(
          utl::CTS, 542, "Must provide a dbUnit conversion though setDbUnits.");
    }

    return 100 /*um*/ * dbUnits_;
  }
  void setBufferDistance(int32_t distance_dbu) { bufDistance_ = distance_dbu; }

  // VertexBufferDistance is in DBU
  int32_t getVertexBufferDistance() const
  {
    if (vertexBufDistance_) {
      return *vertexBufDistance_;
    }

    if (dbUnits_ == -1) {
      logger_->error(
          utl::CTS, 543, "Must provide a dbUnit conversion though setDbUnits.");
    }

    return 240 /*um*/ * dbUnits_;
  }
  void setVertexBufferDistance(int32_t distance_dbu)
  {
    vertexBufDistance_ = distance_dbu;
  }
  bool isVertexBuffersEnabled() const { return vertexBuffersEnable_; }
  void setVertexBuffersEnabled(bool enable) { vertexBuffersEnable_ = enable; }
  bool isSimpleSegmentEnabled() const { return simpleSegmentsEnable_; }
  void setSimpleSegmentsEnabled(bool enable) { simpleSegmentsEnable_ = enable; }
  double getMaxDiameter() const { return maxDiameter_; }
  void setMaxDiameter(double distance)
  {
    maxDiameter_ = distance;
    sinkClusteringUseMaxCap_ = false;
    maxDiameterSet_ = true;
  }
  bool isMaxDiameterSet() const { return maxDiameterSet_; }
  unsigned getSinkClusteringSize() const { return sinkClustersSize_; }
  void setSinkClusteringSize(unsigned size)
  {
    sinkClustersSize_ = size;
    sinkClusteringUseMaxCap_ = false;
    sinkClustersSizeSet_ = true;
  }
  bool isSinkClusteringSizeSet() const { return sinkClustersSizeSet_; }
  unsigned getSinkClusteringLevels() const { return sinkClusteringLevels_; }
  void setSinkClusteringLevels(unsigned levels)
  {
    sinkClusteringLevels_ = levels;
  }
  unsigned getNumStaticLayers() const { return numStaticLayers_; }
  void setBalanceLevels(bool balance) { balanceLevels_ = balance; }
  bool getBalanceLevels() const { return balanceLevels_; }
  void setNumStaticLayers(unsigned num) { numStaticLayers_ = num; }
  void setSinkBuffer(const std::string& buffer) { sinkBuffer_ = buffer; }
  void setSinkBufferInputCap(double cap) { sinkBufferInputCap_ = cap; }
  double getSinkBufferInputCap() const { return sinkBufferInputCap_; }
  std::string getSinkBuffer() const { return sinkBuffer_; }
  utl::Logger* getLogger() const { return logger_; }
  stt::SteinerTreeBuilder* getSttBuilder() const { return sttBuilder_; }
  void setObstructionAware(bool obs) { obsAware_ = obs; }
  bool getObstructionAware() const { return obsAware_; }
  void setApplyNDR(bool ndr) { applyNDR_ = ndr; }
  bool applyNDR() const { return applyNDR_; }
  void enableInsertionDelay(bool insDelay) { insertionDelay_ = insDelay; }
  bool insertionDelayEnabled() const { return insertionDelay_; }
  void setBufferListInferred(bool inferred) { bufferListInferred_ = inferred; }
  bool isBufferListInferred() const { return bufferListInferred_; }
  void setSinkBufferInferred(bool inferred) { sinkBufferInferred_ = inferred; }
  bool isSinkBufferInferred() const { return sinkBufferInferred_; }
  void setRootBufferInferred(bool inferred) { rootBufferInferred_ = inferred; }
  bool isRootBufferInferred() const { return rootBufferInferred_; }
  void setSinkBufferMaxCapDerate(float derate)
  {
    sinkBufferMaxCapDerate_ = derate;
    sinkBufferMaxCapDerateSet_ = true;
  }
  float getSinkBufferMaxCapDerate() const { return sinkBufferMaxCapDerate_; }
  bool isSinkBufferMaxCapDerateSet() const
  {
    return sinkBufferMaxCapDerateSet_;
  }
  void setDelayBufferDerate(float derate) { delayBufferDerate_ = derate; }
  float getDelayBufferDerate() const { return delayBufferDerate_; }
  void enableDummyLoad(bool dummyLoad) { dummyLoad_ = dummyLoad; }
  bool dummyLoadEnabled() const { return dummyLoad_; }
  void setCtsLibrary(const char* name) { ctsLibrary_ = name; }
  const char* getCtsLibrary() { return ctsLibrary_.c_str(); }
  bool isCtsLibrarySet() { return !ctsLibrary_.empty(); }

 private:
  std::string clockNets_ = "";
  std::string rootBuffer_ = "";
  std::string sinkBuffer_ = "";
  std::string treeBuffer_ = "";
  std::string metricFile_ = "";
  int dbUnits_ = -1;
  unsigned wireSegmentUnit_ = 0;
  bool plotSolution_ = false;
  bool sinkClusteringEnable_ = true;
  bool sinkClusteringUseMaxCap_ = true;
  bool simpleSegmentsEnable_ = false;
  bool vertexBuffersEnable_ = false;
  std::unique_ptr<CtsObserver> observer_;
  std::optional<int> vertexBufDistance_;
  std::optional<int> bufDistance_;
  double clusteringCapacity_ = 0.6;
  unsigned clusteringPower_ = 4;
  unsigned numMaxLeafSinks_ = 15;
  unsigned maxFanout_ = 0;
  unsigned maxSlew_ = 4;
  double maxCharSlew_ = 0;
  double maxCharCap_ = 0;
  double sinkBufferInputCap_ = 0;
  int capSteps_ = 20;
  int slewSteps_ = 7;
  unsigned charWirelengthIterations_ = 4;
  unsigned clockTreeMaxDepth_ = 100;
  bool enableFakeLutEntries_ = true;
  bool forceBuffersOnLeafLevel_ = true;
  double bufDistRatio_ = 0.1;
  int clockRoots_ = 0;
  int clockSubnets_ = 0;
  int buffersInserted_ = 0;
  int sinks_ = 0;
  double maxDiameter_ = 50;
  bool maxDiameterSet_ = false;
  unsigned sinkClustersSize_ = 20;
  bool sinkClustersSizeSet_ = false;
  bool balanceLevels_ = false;
  unsigned sinkClusteringLevels_ = 0;
  unsigned numStaticLayers_ = 0;
  std::vector<std::string> bufferList_;
  std::vector<odb::dbNet*> clockNetsObjs_;
  utl::Logger* logger_ = nullptr;
  stt::SteinerTreeBuilder* sttBuilder_ = nullptr;
  bool obsAware_ = false;
  bool applyNDR_ = false;
  bool insertionDelay_ = true;
  bool bufferListInferred_ = false;
  bool sinkBufferInferred_ = false;
  bool rootBufferInferred_ = false;
  bool sinkBufferMaxCapDerateSet_ = false;
  float sinkBufferMaxCapDerateDefault_ = 0.01;
  float sinkBufferMaxCapDerate_ = sinkBufferMaxCapDerateDefault_;
  bool dummyLoad_ = true;
  float delayBufferDerate_ = 1.0;  // no derate
  std::string ctsLibrary_;
};

}  // namespace cts
