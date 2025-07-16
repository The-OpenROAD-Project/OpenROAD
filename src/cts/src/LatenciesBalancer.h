// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "odb/db.h"

namespace utl {
class Logger;
}  // namespace utl

namespace sta {
class dbSta;
class dbNetwork;
class LibertyCell;
class Vertex;
class Graph;
}  // namespace sta

namespace cts {

using utl::Logger;

struct GraphNode
{
  GraphNode(int id, std::string name, odb::dbITerm* inputTerm)
      : id(id), name(std::move(name)), inputTerm(inputTerm)
  {
  }

  int id;
  std::string name;
  std::vector<int> childrenIds;
  float delay = 0.0;
  int nBuffInsert = -1;
  odb::dbITerm* inputTerm = nullptr;
};

class LatenciesBalancer
{
 public:
  LatenciesBalancer(TreeBuilder* root,
                    const CtsOptions* options,
                    Logger* logger,
                    odb::dbDatabase* db,
                    sta::dbNetwork* network,
                    sta::dbSta* sta,
                    double scalingUnit)
      : root_(root),
        options_(options),
        logger_(logger),
        db_(db),
        network_(network),
        openSta_(sta),
        wireSegmentUnit_(scalingUnit)
  {
  }

  void run();

 private:
  void initSta();
  void findLeafBuilders(TreeBuilder* builder);
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

  // DFS search throw the tree graph to insert delay buffers. At each node,
  // evaluate the delay of the its children, if the children need delay buffers
  // and need different ammount of delay buffers, isert this difference, to the
  // child that need more buffers.
  void balanceLatencies(int nodeId);
  odb::dbITerm* insertDelayBuffers(
      int numBuffers,
      int srcX,
      int srcY,
      const std::vector<odb::dbITerm*>& sinksInput);
  odb::dbInst* createDelayBuffer(odb::dbNet* driverNet,
                                 const std::string& clockName,
                                 int locX,
                                 int locY);
  bool propagateClock(odb::dbITerm* input);
  bool isSink(odb::dbITerm* iterm);

  void showGraph();

  TreeBuilder* root_ = nullptr;
  const CtsOptions* options_ = nullptr;
  Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  sta::dbSta* openSta_ = nullptr;
  sta::Graph* timingGraph_ = nullptr;
  double wireSegmentUnit_;
  float worseDelay_;
  int delayBufIndex_;
  std::vector<GraphNode> graph_;
  std::map<std::string, TreeBuilder*> inst2builder_;
};

}  // namespace cts