// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "utl/Logger.h"

namespace sta {
class dbSta;
class dbNetwork;
class LibertyCell;
class Vertex;
class Graph;
}  // namespace sta

namespace cts {

struct GraphNode
{
  GraphNode(int id, std::string name, odb::dbITerm* inputTerm)
      : id(id), name(std::move(name)), inputTerm(inputTerm)
  {
  }

  int id;
  std::string name;
  std::vector<int> childrenIds;
  double arrival = 0.0;
  double dlyNeeded = -1.0;
  int nBuffInsert = -1;
  odb::dbITerm* inputTerm = nullptr;
};

struct DPResult {
  std::vector<int> elements;  // dp_elements
  std::vector<int64_t> values;    // dp
};

class LatencyBalancer
{
 public:
  LatencyBalancer(TreeBuilder* root,
                  const CtsOptions* options,
                  utl::Logger* logger,
                  odb::dbDatabase* db,
                  sta::dbNetwork* network,
                  sta::dbSta* sta,
                  TechChar* techChar,
                  double capPerDBU,
                  double resPerDBU)
      : root_(root),
        options_(options),
        logger_(logger),
        db_(db),
        network_(network),
        openSta_(sta),
        techChar_(techChar),
        capPerDBU_(capPerDBU),
        resPerDBU_(resPerDBU),
        worseDelay_(std::numeric_limits<float>::min())
  {
  }

  int run();

 private:
  void initSta();
  void findLeafBuilders(TreeBuilder* builder);
  void computeBuffersDelay(std::vector<int>& buffersDelay,
                           double extra_out_cap);
  int64_t computeWireLumpedDelay(const std::string& load, double wl, double& wireCap);
  void buildGraph(odb::dbNet* clkInputNet);
  odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
  float getVertexClkArrival(sta::Vertex* sinkVertex,
                            odb::dbNet* topNet,
                            odb::dbITerm* iterm);
  float computeAveSinkArrivals(TreeBuilder* builder);
  void computeSinkArrivalRecur(odb::dbNet* topClokcNet,
                               odb::dbITerm* iterm,
                               float& sumArrivals,
                               unsigned& numSinks);

  DPResult solveDP(int64_t target, const std::vector<int>& bufDelays, int64_t wireDly, const std::vector<odb::dbITerm*>& sinks, const std::vector<std::string>& dlyBuffers, double extraOutCap, double loadPinsHwpl);
  static int backtrackCount(const DPResult& dp, const std::vector<int>& bufDelays, int64_t target);
  static std::vector<std::string> backtrackNames(const DPResult& dp, const std::vector<int>& bufDelays, const std::vector<std::string>& dlyBuffers, int64_t target);
  std::vector<std::string> computeNumberOfDelayBuffers(
      double delayNeeded,
      int srcX,
      int srcY,
      const std::vector<odb::dbITerm*>& sinks);
  // DFS search throw the tree graph to insert delay buffers. At each node,
  // evaluate the delay of the its children, if the children need delay buffers
  // and need different ammount of delay buffers, isert this difference, to the
  // child that need more buffers.
  void balanceLatencies(int nodeId);
  odb::dbITerm* insertDelayBuffers(
      int srcX,
      int srcY,
      const std::vector<std::string>& buffersMaster,
      const std::vector<odb::dbITerm*>& sinksInput);
  bool propagateClock(odb::dbITerm* input);
  bool isSink(odb::dbITerm* iterm);

  void showGraph();

  TreeBuilder* root_ = nullptr;
  const CtsOptions* options_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  sta::dbSta* openSta_ = nullptr;
  sta::Graph* timingGraph_ = nullptr;
  TechChar* techChar_ = nullptr;
  double wireSegmentUnit_;
  float bufferDelay_;
  double capPerDBU_;
  double resPerDBU_;
  float worseDelay_;
  int delayBufIndex_{0};
  std::vector<int> buffersDelay_;
  std::vector<GraphNode> graph_;
  std::map<std::string, TreeBuilder*> inst2builder_;
};

}  // namespace cts
