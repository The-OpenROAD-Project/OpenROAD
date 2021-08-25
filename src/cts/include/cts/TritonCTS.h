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

#include <string>
#include <functional>
#include <set>
#include <map>

namespace ord {
class OpenRoad;
} // namespace ord

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbBlock;
class dbInst;
class dbNet;
class dbITerm;
} // namespace odb

namespace sta {
class dbSta;
class Clock;
class dbNetwork;
}  // namespace sta

namespace cts {

using utl::Logger;

class Clock;
class ClockInst;
class CtsOptions;
class TechChar;
class StaEngine;
class TreeBuilder;

class TritonCTS
{
 public:
  TritonCTS() = default;
  ~TritonCTS();

  void init(ord::OpenRoad* openroad);
  void runTritonCts();
  void reportCtsMetrics();
  CtsOptions* getParms() { return _options; }
  TechChar* getCharacterization() { return _techChar; }
  void addBuilder(TreeBuilder* builder);
  void forEachBuilder(const std::function<void(const TreeBuilder*)> func) const;
  int setClockNets(const char* names);
  void setBufferList(const char* buffers);

private:
  void setupCharacterization();
  void checkCharacterization();
  void findClockRoots();
  void buildClockTrees();
  void runPostCtsOpt();
  void writeDataToDb();

  // db functions
  bool masterExists(const std::string& master) const;
  void populateTritonCTS();
  void writeClockNetsToDb(Clock& clockNet);
  void incrementNumClocks() { _numberOfClocks = _numberOfClocks + 1; }
  void clearNumClocks() { _numberOfClocks = 0; }
  unsigned getNumClocks() const { return _numberOfClocks; }
  void parseClockNames(std::vector<std::string>& clockNetNames) const;
  void initOneClockTree(odb::dbNet* driverNet, std::string sdcClockName, TreeBuilder* parent);
  TreeBuilder* initClock(odb::dbNet* net, std::string sdcClock, TreeBuilder* parentBuilder);
  void disconnectAllSinksFromNet(odb::dbNet* net);
  void disconnectAllPinsFromNet(odb::dbNet* net);
  void checkUpstreamConnections(odb::dbNet* net);
  void createClockBuffers(Clock& clk);
  void removeNonClockNets();
  void computeITermPosition(odb::dbITerm* term, int& x, int& y) const;
void countSinksPostDbWrite(TreeBuilder* builder, odb::dbNet* net,
                           unsigned &sinks, unsigned &leafSinks,
                           unsigned currWireLength, double &sinkWireLength,
                           int& minDepth, int& maxDepth, int depth,
                           bool fullTree);
  std::pair<int, int> branchBufferCount(ClockInst* inst,
                                        int bufCounter,
                                        Clock& clockNet);
  odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
  odb::dbITerm* getSingleOutput(odb::dbInst* inst, odb::dbITerm* input) const;
  void findClockRoots(sta::Clock* clk,
                      std::set<odb::dbNet*> &clockNets);
  float getInputPinCap(odb::dbITerm* iterm);
  bool isSink(odb::dbITerm* iterm);
  ClockInst* getClockFromInst(odb::dbInst* inst);

  ord::OpenRoad* _openroad;
  sta::dbSta* _openSta;
  sta::dbNetwork* _network;
  Logger* _logger;
  CtsOptions* _options;
  TechChar* _techChar;
  std::vector<TreeBuilder*>* _builders;
  std::set <odb::dbNet*> staClockNets;
  std::set <odb::dbNet*> visitedClockNets;
  std::map<odb::dbInst*, ClockInst*> inst2clkbuf;

  // db vars
  odb::dbDatabase* _db;
  odb::dbBlock* _block = nullptr;
  unsigned _numberOfClocks = 0;
  unsigned _numClkNets = 0;
  unsigned _numFixedNets = 0;
};

}  // namespace cts
