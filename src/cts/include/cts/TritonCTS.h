// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {
class Resizer;
}

namespace sta {
class dbSta;
class Clock;
class dbNetwork;
class Unit;
class LibertyCell;
class Vertex;
class Graph;
class Pin;
}  // namespace sta

namespace stt {
class SteinerTreeBuilder;
struct Tree;
}  // namespace stt

namespace cts {

class ClockInst;
class TechChar;
class StaEngine;
class TreeBuilder;
class Clock;
class ClockSubNet;
class HTreeBuilder;
class CtsObserver;

class TritonCTS : public odb::dbBlockCallBackObj
{
 public:
  TritonCTS(utl::Logger* logger,
            odb::dbDatabase* db,
            sta::dbNetwork* network,
            sta::dbSta* sta,
            stt::SteinerTreeBuilder* st_builder,
            rsz::Resizer* resizer,
            est::EstimateParasitics* estimate_parasitics);
  ~TritonCTS();

  void runTritonCts();
  void reportCtsMetrics();

  enum class NdrStrategy
  {
    NONE,
    ROOT_ONLY,
    HALF,
    FULL
  };

  enum class MasterType
  {
    DUMMY,
    TREE
  };
  using MasterCount = odb::PtrMap<odb::dbMaster, int>;

  std::string getClockNets() const { return clockNets_; }
  void setFinalRootBuffer(const std::string& buffer) { rootBuffer_ = buffer; }
  std::string getRootBuffer() const { return rootBuffer_; }
  void setBufferList(const std::vector<std::string>& buffers)
  {
    bufferList_ = buffers;
  }
  std::vector<std::string> getBufferList() const { return bufferList_; }
  std::string getBufferListToString() const
  {
    std::ostringstream buffer_names;
    for (const auto& buf : bufferList_) {
      buffer_names << buf << " ";
    }
    return buffer_names.str();
  }
  void resetBufferList() { bufferList_.clear(); }
  void setDbUnits(int units) { dbUnits_ = units; }
  int getDbUnits() const { return dbUnits_; }
  void setWireSegmentUnit(unsigned wireSegmentUnit)
  {
    wireSegmentUnit_ = wireSegmentUnit;
  }
  void resetWireSegmentUnit() { wireSegmentUnit_ = 0; }
  unsigned getWireSegmentUnit() const { return wireSegmentUnit_; }
  void setPlotSolution(bool plot) { plotSolution_ = plot; }
  bool getPlotSolution() const { return plotSolution_; }

  void setObserver(std::unique_ptr<CtsObserver> observer);
  CtsObserver* getObserver() const { return observer_.get(); }

  void setSinkClustering(bool enable) { sinkClusteringEnable_ = enable; }
  bool getSinkClustering() const { return sinkClusteringEnable_; }
  void setNumMaxLeafSinks(unsigned numSinks) { numMaxLeafSinks_ = numSinks; }
  unsigned getNumMaxLeafSinks() const { return numMaxLeafSinks_; }
  void setMaxSlew(unsigned slew) { maxSlew_ = slew; }
  unsigned getMaxSlew() const { return maxSlew_; }
  void setMaxWl(int wl) { maxWl_ = wl; }
  int getMaxWl() const { return maxWl_; }
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
  void setSkipNets(odb::dbNet* nets) { skipNets_.push_back(nets); }
  std::vector<odb::dbNet*> getSkipNets() const { return skipNets_; }
  std::string getSkipNetsToString() const
  {
    std::ostringstream skip_nets_names;
    for (const odb::dbNet* db_net : skipNets_) {
      skip_nets_names << db_net->getConstName() << " ";
    }
    return skip_nets_names.str();
  }
  void resetSkipNets() { skipNets_.clear(); }
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
  void resetTreeBuffer() { treeBuffer_.clear(); }
  std::string getTreeBuffer() const { return treeBuffer_; }
  unsigned getClusteringPower() const { return clusteringPower_; }
  void setClusteringPower(unsigned power) { clusteringPower_ = power; }
  void resetClusteringPower() { clusteringPower_ = 4; }
  double getClusteringCapacity() const { return clusteringCapacity_; }
  void setClusteringCapacity(double capacity)
  {
    clusteringCapacity_ = capacity;
  }
  void resetClusteringCapacity() { clusteringCapacity_ = 0.6; }

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
  void resetBufferDistance() { bufDistance_.reset(); }

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
  void resetVertexBufferDistance() { vertexBufDistance_.reset(); }
  bool isVertexBuffersEnabled() const { return vertexBuffersEnable_; }
  void setVertexBuffersEnabled(bool enable) { vertexBuffersEnable_ = enable; }
  bool isSimpleSegmentEnabled() const { return simpleSegmentsEnable_; }
  void setSimpleSegmentsEnabled(bool enable) { simpleSegmentsEnable_ = enable; }
  double getMaxDiameter() const { return maxDiameter_; }
  void setMaxDiameter(double distance)
  {
    maxDiameter_ = distance;
    maxDiameterSet_ = true;
  }
  void resetMaxDiameter()
  {
    maxDiameter_ = 50;
    maxDiameterSet_ = false;
  }
  bool isMaxDiameterSet() const { return maxDiameterSet_; }
  unsigned getSinkClusteringSize() const { return sinkClustersSize_; }
  void setSinkClusteringSize(unsigned size)
  {
    sinkClustersSize_ = size;
    sinkClustersSizeSet_ = true;
  }
  void resetSinkClusteringSize()
  {
    sinkClustersSize_ = 30;
    sinkClustersSizeSet_ = false;
  }
  bool isSinkClusteringSizeSet() const { return sinkClustersSizeSet_; }
  void limitSinkClusteringSizes(unsigned limit);
  unsigned getSinkClusteringLevels() const { return sinkClusteringLevels_; }
  void setSinkClusteringLevels(unsigned levels)
  {
    sinkClusteringLevels_ = levels;
  }
  void resetSinkClusteringLevels() { sinkClusteringLevels_ = 0; }

  double getMacroMaxDiameter() const { return macroMaxDiameter_; }
  void setMacroMaxDiameter(double distance)
  {
    macroMaxDiameter_ = distance;
    macroMaxDiameterSet_ = true;
  }
  void resetMacroMaxDiameter()
  {
    macroMaxDiameter_ = 50;
    macroMaxDiameterSet_ = false;
  }
  bool isMacroMaxDiameterSet() const { return macroMaxDiameterSet_; }
  unsigned getMacroSinkClusteringSize() const { return macroSinkClustersSize_; }
  void setMacroClusteringSize(unsigned size)
  {
    macroSinkClustersSize_ = size;
    macroSinkClustersSizeSet_ = true;
  }
  void resetMacroClusteringSize()
  {
    macroSinkClustersSize_ = 4;
    macroSinkClustersSizeSet_ = false;
  }
  bool isMacroSinkClusteringSizeSet() const
  {
    return macroSinkClustersSizeSet_;
  }
  unsigned getNumStaticLayers() const { return numStaticLayers_; }
  void setNumStaticLayers(unsigned num) { numStaticLayers_ = num; }
  void resetNumStaticLayers() { numStaticLayers_ = 0; }
  void setFinalSinkBuffer(const std::string& buffer) { sinkBuffer_ = buffer; }
  void setSinkBufferInputCap(double cap) { sinkBufferInputCap_ = cap; }
  double getSinkBufferInputCap() const { return sinkBufferInputCap_; }
  std::string getSinkBuffer() const { return sinkBuffer_; }
  utl::Logger* getLogger() const { return logger_; }
  stt::SteinerTreeBuilder* getSttBuilder() const { return sttBuilder_; }
  void setObstructionAware(bool obs) { obsAware_ = obs; }
  bool getObstructionAware() const { return obsAware_; }
  void enableInsertionDelay(bool insDelay) { insertionDelay_ = insDelay; }
  bool insertionDelayEnabled() const { return insertionDelay_; }
  void setBufferListInferred(bool inferred) { bufferListInferred_ = inferred; }
  bool isBufferListInferred() const { return bufferListInferred_; }
  void setSinkBufferInferred(bool inferred) { sinkBufferInferred_ = inferred; }
  bool isSinkBufferInferred() const { return sinkBufferInferred_; }
  void setRootBufferInferred(bool inferred) { rootBufferInferred_ = inferred; }
  bool isRootBufferInferred() const { return rootBufferInferred_; }
  void setSinkBufferMaxCapDerate(double derate)
  {
    sinkBufferMaxCapDerate_ = derate;
    sinkBufferMaxCapDerateSet_ = true;
  }
  void resetSinkBufferMaxCapDerate()
  {
    sinkBufferMaxCapDerate_ = sinkBufferMaxCapDerateDefault_;
    sinkBufferMaxCapDerateSet_ = false;
  }
  double getSinkBufferMaxCapDerate() const { return sinkBufferMaxCapDerate_; }
  bool isSinkBufferMaxCapDerateSet() const
  {
    return sinkBufferMaxCapDerateSet_;
  }
  void setDelayBufferDerate(float derate) { delayBufferDerate_ = derate; }
  void resetDelayBufferDerate() { delayBufferDerate_ = 1.0; }
  float getDelayBufferDerate() const { return delayBufferDerate_; }
  void enableDummyLoad(bool dummyLoad) { dummyLoad_ = dummyLoad; }
  bool dummyLoadEnabled() const { return dummyLoad_; }
  std::string getDummyLoadPrefix() const { return dummyload_prefix_; }
  void setCtsLibrary(const char* name) { ctsLibrary_ = name; }
  void resetCtsLibrary() { ctsLibrary_.clear(); }
  const char* getCtsLibrary() { return ctsLibrary_.c_str(); }
  bool isCtsLibrarySet() { return !ctsLibrary_.empty(); }

  void recordBuffer(odb::dbMaster* master, MasterType type);
  const MasterCount& getBufferCount() const { return buffer_count_; }
  const MasterCount& getDummyCount() const { return dummy_count_; }

  MasterType getType(odb::dbInst* inst) const;

  // Callbacks
  void inDbInstCreate(odb::dbInst* inst) override;

  void setRepairClockNets(bool value) { repairClockNets_ = value; }
  bool getRepairClockNets() { return repairClockNets_; }

  // NDR strategies
  void setApplyNDR(NdrStrategy strategy) { ndrStrategy_ = strategy; }
  void resetApplyNDR() { ndrStrategy_ = NdrStrategy::HALF; }
  NdrStrategy getApplyNdr() const { return ndrStrategy_; }
  const char* getApplyNdrName() const
  {
    switch (ndrStrategy_) {
      case NdrStrategy::NONE:
        return "NONE";
      case NdrStrategy::ROOT_ONLY:
        return "ROOT_ONLY";
      case NdrStrategy::HALF:
        return "HALF";
      case NdrStrategy::FULL:
        return "FULL";
    }
    return "";
  }

  TritonCTS* getParms() { return this; }
  TechChar* getCharacterization() { return techChar_.get(); }
  odb::dbBlock* getBlock() { return db_->getChip()->getBlock(); }
  int setClockNets(const char* names);
  void setBufferList(const char* buffers);
  void setRootBuffer(const char* buffers);
  std::string getRootBufferToString();
  void resetRootBuffer() { rootBuffers_.clear(); }
  void setSinkBuffer(const char* buffers);

 private:
  std::string selectRootBuffer(std::vector<std::string>& buffers);
  std::string selectSinkBuffer(std::vector<std::string>& buffers);
  std::string selectBestMaxCapBuffer(const std::vector<std::string>& buffers,
                                     float totalCap);
  TreeBuilder* addBuilder(TritonCTS* options,
                          Clock& net,
                          odb::dbNet* topInputNet,
                          TreeBuilder* parent,
                          utl::Logger* logger,
                          odb::dbDatabase* db);
  void forEachBuilder(
      const std::function<void(const TreeBuilder*)>& func) const;

  int getBufferFanoutLimit(const std::string& bufferName);
  void setupCharacterization();
  void checkCharacterization();
  void findClockRoots();
  void buildClockTrees();
  void writeDataToDb();

  // NDR functions
  std::vector<int> getAllClockTreeLevels(Clock& clockNet);
  int applyNDRToClockLevels(Clock& clockNet,
                            odb::dbTechNonDefaultRule* clockNDR,
                            const std::vector<int>& targetLevels);

  int applyNDRToClockLevelRange(Clock& clockNet,
                                odb::dbTechNonDefaultRule* clockNDR,
                                int minLevel,
                                int maxLevel);
  int applyNDRToFirstHalfLevels(Clock& clockNet,
                                odb::dbTechNonDefaultRule* clockNDR);

  // db functions
  bool masterExists(const std::string& master) const;
  void populateTritonCTS();
  void destroyClockModNet(sta::Pin* pin_driver);
  void writeClockNetsToDb(TreeBuilder* builder,
                          odb::PtrSet<odb::dbNet>& clkLeafNets);
  void writeClockNDRsToDb(TreeBuilder* builder);
  void incrementNumClocks() { ++numberOfClocks_; }
  void clearNumClocks() { numberOfClocks_ = 0; }
  unsigned getNumClocks() const { return numberOfClocks_; }
  void cloneClockGaters(odb::dbNet* clkNet,
                        std::set<odb::Point>& occupiedPositions,
                        std::unordered_set<odb::dbNet*>& visitedNets);
  void findLongEdges(
      stt::Tree& clkSteiner,
      odb::Point driverPt,
      std::map<odb::Point, std::vector<odb::dbITerm*>>& point2pin,
      std::set<odb::Point>& occupiedPositions);
  void resolveLocationCollision(odb::dbInst* clone,
                                odb::Point location,
                                std::set<odb::Point>& occupiedPositions);
  void initOneClockTree(odb::dbNet* driverNet,
                        odb::dbNet* clkInputNet,
                        const std::string& sdcClockName,
                        TreeBuilder* parent);
  TreeBuilder* initClock(odb::dbNet* firstNet,
                         odb::dbNet* clkInputNet,
                         const std::string& sdcClock,
                         TreeBuilder* parentBuilder);
  void disconnectAllSinksFromNet(odb::dbNet* net);
  void disconnectAllPinsFromNet(odb::dbNet* net);
  void checkUpstreamConnections(odb::dbNet* net);
  void createClockBuffers(Clock& clockNet, odb::dbModule* parent);
  TreeBuilder* initClockTreeForMacrosAndRegs(
      odb::dbNet*& firstNet,
      odb::dbNet* clkInputNet,
      const std::unordered_set<odb::dbMaster*>& buffer_masters,
      Clock& ClockNet,
      TreeBuilder* parentBuilder);
  bool separateMacroRegSinks(
      odb::dbNet*& net,
      Clock& clockNet,
      const std::unordered_set<odb::dbMaster*>& buffer_masters,
      std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& registerSinks,
      std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& macroSinks);
  TreeBuilder* addClockSinks(
      Clock& clockNet,
      odb::dbNet* topInputNet,
      odb::dbNet* physicalNet,
      const std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& sinks,
      TreeBuilder* parentBuilder,
      const std::string& macrosOrRegs);
  Clock forkRegisterClockNetwork(
      Clock& clockNet,
      const std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& registerSinks,
      odb::dbNet*& firstNet,
      odb::dbNet*& secondNet,
      std::string& topBufferName);
  void computeITermPosition(odb::dbITerm* term, int& x, int& y) const;
  void countSinksPostDbWrite(TreeBuilder* builder,
                             odb::dbNet* net,
                             unsigned& sinks_cnt,
                             unsigned& leafSinks,
                             unsigned currWireLength,
                             double& sinkWireLength,
                             int& minDepth,
                             int& maxDepth,
                             int depth,
                             bool fullTree,
                             const std::unordered_set<odb::dbITerm*>& sinks,
                             const std::unordered_set<odb::dbInst*>& dummies,
                             std::unordered_set<odb::dbNet*>& visitedNets);
  std::pair<int, int> branchBufferCount(ClockInst* inst,
                                        int bufCounter,
                                        Clock& clockNet);
  odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
  odb::dbITerm* getSingleOutput(odb::dbInst* inst, odb::dbITerm* input) const;
  void findClockRoots(sta::Clock* clk, odb::PtrSet<odb::dbNet>& clockNets);
  float getInputPinCap(odb::dbITerm* iterm);
  bool isSink(odb::dbITerm* iterm);
  ClockInst* getClockFromInst(odb::dbInst* inst);
  bool hasInsertionDelay(odb::dbInst* inst, odb::dbMTerm* mterm);
  double computeInsertionDelay(const std::string& name,
                               odb::dbInst* inst,
                               odb::dbMTerm* mterm);
  int writeDummyLoadsToDb(Clock& clockNet,
                          std::unordered_set<odb::dbInst*>& dummies);
  bool computeIdealOutputCaps(Clock& clockNet);
  void findCandidateDummyCells(std::vector<sta::LibertyCell*>& dummyCandidates);
  odb::dbInst* insertDummyCell(
      Clock& clockNet,
      ClockInst* inst,
      const std::vector<sta::LibertyCell*>& dummyCandidates);
  ClockInst& placeDummyCell(Clock& clockNet,
                            const ClockInst* inst,
                            const sta::LibertyCell* dummyCell,
                            odb::dbInst*& dummyInst);
  void connectDummyCell(const ClockInst* inst,
                        odb::dbInst* dummyInst,
                        ClockSubNet& subNet,
                        ClockInst& dummyClock);
  void printClockNetwork(const Clock& clockNet) const;
  void setAllClocksPropagated();
  void repairClockNets();
  void balanceMacroRegisterLatencies();

  sta::dbSta* openSta_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::unique_ptr<TechChar> techChar_;
  rsz::Resizer* resizer_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  std::vector<std::unique_ptr<TreeBuilder>> builders_;
  odb::PtrSet<odb::dbNet> staClockNets_;
  odb::PtrSet<odb::dbNet> visitedClockNets_;
  odb::PtrMap<odb::dbInst, ClockInst*> inst2clkbuf_;
  std::map<ClockInst*, ClockSubNet*> driver2subnet_;
  odb::PtrMap<odb::dbNet, TreeBuilder*> net2builder_;

  // db vars
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  unsigned numberOfClocks_ = 0;
  unsigned numClkNets_ = 0;
  unsigned numFixedNets_ = 0;
  unsigned dummyLoadIndex_ = 0;

  // root buffer and sink bufer candidates
  std::vector<std::string> rootBuffers_;
  std::vector<std::string> sinkBuffers_;

  // register tree root buffer indices
  unsigned regTreeRootBufIndex_ = 0;
  // index for delay buffer added for latency adjustment
  unsigned delayBufIndex_ = 0;

  std::string clockNets_;
  std::string rootBuffer_;
  std::string sinkBuffer_;
  std::string treeBuffer_;
  std::string metricFile_;
  int dbUnits_ = -1;
  unsigned wireSegmentUnit_ = 0;
  bool plotSolution_ = false;
  bool sinkClusteringEnable_ = true;
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
  int maxWl_ = 0;
  double maxCharSlew_ = 0;
  double maxCharCap_ = 0;
  int capSteps_ = 20;
  int slewSteps_ = 7;
  unsigned charWirelengthIterations_ = 4;
  double sinkBufferInputCap_ = 0;
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
  unsigned sinkClustersSize_ = 30;
  bool sinkClustersSizeSet_ = false;
  double macroMaxDiameter_ = 50;
  bool macroMaxDiameterSet_ = false;
  unsigned macroSinkClustersSize_ = 4;
  bool macroSinkClustersSizeSet_ = true;
  unsigned sinkClusteringLevels_ = 0;
  unsigned numStaticLayers_ = 0;
  std::vector<std::string> bufferList_;
  std::vector<odb::dbNet*> clockNetsObjs_;
  std::vector<odb::dbNet*> skipNets_;
  stt::SteinerTreeBuilder* sttBuilder_ = nullptr;
  bool obsAware_ = true;
  bool insertionDelay_ = true;
  bool bufferListInferred_ = false;
  bool sinkBufferInferred_ = false;
  bool rootBufferInferred_ = false;
  bool sinkBufferMaxCapDerateSet_ = false;
  double sinkBufferMaxCapDerateDefault_ = 0.01;
  double sinkBufferMaxCapDerate_ = sinkBufferMaxCapDerateDefault_;
  bool dummyLoad_ = true;
  float delayBufferDerate_ = 1.0;  // no derate
  std::string ctsLibrary_;
  MasterCount buffer_count_;
  std::string dummyload_prefix_ = "clkload";
  MasterCount dummy_count_;
  bool repairClockNets_ = false;
  NdrStrategy ndrStrategy_ = NdrStrategy::HALF;
};

}  // namespace cts
