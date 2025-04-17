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

#include "odb/db.h"

namespace utl {
class Logger;
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
}  // namespace sta

namespace stt {
class SteinerTreeBuilder;
}

namespace cts {

using utl::Logger;

class ClockInst;
class CtsOptions;
class TechChar;
class StaEngine;
class TreeBuilder;
class Clock;
class ClockSubNet;
class HTreeBuilder;

class TritonCTS
{
 public:
  TritonCTS();
  ~TritonCTS();

  void init(utl::Logger* logger,
            odb::dbDatabase* db,
            sta::dbNetwork* network,
            sta::dbSta* sta,
            stt::SteinerTreeBuilder* st_builder,
            rsz::Resizer* resizer);
  void runTritonCts();
  void reportCtsMetrics();
  CtsOptions* getParms() { return options_; }
  TechChar* getCharacterization() { return techChar_.get(); }
  int setClockNets(const char* names);
  void setBufferList(const char* buffers);
  void setRootBuffer(const char* buffers);
  void setSinkBuffer(const char* buffers);

 private:
  bool isClockCellCandidate(sta::LibertyCell* cell);
  std::string selectRootBuffer(std::vector<std::string>& buffers);
  std::string selectSinkBuffer(std::vector<std::string>& buffers);
  std::string selectBestMaxCapBuffer(const std::vector<std::string>& buffers,
                                     float totalCap);
  void inferBufferList(std::vector<std::string>& buffers);
  TreeBuilder* addBuilder(CtsOptions* options,
                          Clock& net,
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

  // db functions
  bool masterExists(const std::string& master) const;
  void populateTritonCTS();
  void writeClockNetsToDb(TreeBuilder* builder,
                          std::set<odb::dbNet*>& clkLeafNets);
  void writeClockNDRsToDb(const std::set<odb::dbNet*>& clkLeafNets);
  void incrementNumClocks() { ++numberOfClocks_; }
  void clearNumClocks() { numberOfClocks_ = 0; }
  unsigned getNumClocks() const { return numberOfClocks_; }
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
                             const std::unordered_set<odb::dbInst*>& dummies);
  std::pair<int, int> branchBufferCount(ClockInst* inst,
                                        int bufCounter,
                                        Clock& clockNet);
  odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
  odb::dbITerm* getSingleOutput(odb::dbInst* inst, odb::dbITerm* input) const;
  void findClockRoots(sta::Clock* clk, std::set<odb::dbNet*>& clockNets);
  float getInputPinCap(odb::dbITerm* iterm);
  bool isSink(odb::dbITerm* iterm);
  ClockInst* getClockFromInst(odb::dbInst* inst);
  bool hasInsertionDelay(odb::dbInst* inst, odb::dbMTerm* mterm);
  double computeInsertionDelay(const std::string& name,
                               odb::dbInst* inst,
                               odb::dbMTerm* mterm);
  void writeDummyLoadsToDb(Clock& clockNet,
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
  void balanceMacroRegisterLatencies();
  float getVertexClkArrival(sta::Vertex* sinkVertex,
                            odb::dbNet* topNet,
                            odb::dbITerm* iterm);
  void computeAveSinkArrivals(TreeBuilder* builder, sta::Graph* graph);
  void computeSinkArrivalRecur(odb::dbNet* topClokcNet,
                               odb::dbITerm* iterm,
                               float& sumArrivals,
                               unsigned& numSinks,
                               sta::Graph* graph);
  bool propagateClock(odb::dbITerm* input);
  void adjustLatencies(TreeBuilder* macroBuilder, TreeBuilder* registerBuilder);
  void computeTopBufferDelay(TreeBuilder* builder);
  odb::dbInst* insertDelayBuffer(odb::dbInst* driver,
                                 const std::string& clockName,
                                 int locX,
                                 int locY);

  sta::dbSta* openSta_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  Logger* logger_ = nullptr;
  CtsOptions* options_ = nullptr;
  std::unique_ptr<TechChar> techChar_;
  rsz::Resizer* resizer_ = nullptr;
  std::vector<std::unique_ptr<TreeBuilder>> builders_;
  std::set<odb::dbNet*> staClockNets_;
  std::set<odb::dbNet*> visitedClockNets_;
  std::map<odb::dbInst*, ClockInst*> inst2clkbuf_;
  std::map<ClockInst*, ClockSubNet*> driver2subnet_;

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
};

}  // namespace cts
