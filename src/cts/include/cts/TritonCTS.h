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

#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
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
  TritonCTS() = default;
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
  TechChar* getCharacterization() { return techChar_; }
  int setClockNets(const char* names);
  void setBufferList(const char* buffers);
  void inferBufferList(std::vector<std::string>& buffers);
  bool isClockCellCandidate(sta::LibertyCell* cell);
  void setRootBuffer(const char* buffers);
  std::string selectRootBuffer(std::vector<std::string>& buffers);
  void setSinkBuffer(const char* buffers);
  std::string selectSinkBuffer(std::vector<std::string>& buffers);
  std::string selectBestMaxCapBuffer(const std::vector<std::string>& buffers,
                                     float totalCap);

 private:
  void addBuilder(TreeBuilder* builder);
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
  HTreeBuilder* initClockTreeForMacrosAndRegs(
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
  HTreeBuilder* addClockSinks(
      Clock& clockNet,
      odb::dbNet* physicalNet,
      const std::vector<std::pair<odb::dbInst*, odb::dbMTerm*>>& sinks,
      HTreeBuilder* parentBuilder,
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
  void adjustLatencies(TreeBuilder* macroBuilder, TreeBuilder* registerBuilder);
  void computeTopBufferDelay(TreeBuilder* builder);
  odb::dbInst* insertDelayBuffer(odb::dbInst* driver,
                                 const std::string& clockName,
                                 int locX,
                                 int locY);

  sta::dbSta* openSta_;
  sta::dbNetwork* network_;
  Logger* logger_;
  CtsOptions* options_;
  TechChar* techChar_;
  rsz::Resizer* resizer_;
  std::vector<TreeBuilder*>* builders_;
  std::set<odb::dbNet*> staClockNets_;
  std::set<odb::dbNet*> visitedClockNets_;
  std::map<odb::dbInst*, ClockInst*> inst2clkbuf_;
  std::map<ClockInst*, ClockSubNet*> driver2subnet_;

  // db vars
  odb::dbDatabase* db_;
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
