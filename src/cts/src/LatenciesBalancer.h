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

/////////////////////////////////////////////////////////////////////////
// Class: LevelBalancer
// Purpose: Balance buffer levels accross nets of same clock
// Nets driven by drivers other than clock source itself are driven by
// clock gates (CGC). Each of these nets is build independently by CTS
// Since the clock is same, large skew would be introduced between sinks
// of different clock nets.
//
// INPUT to Level Balancer
//
//                |----|>----[]  Level = 1
//                |----|>----[]
//                |----|>----[]
//   [root]-------|                   |---|>----[]   Level = 3
//                |----|>----D--------|
//                          (CGC)     |---|>-----[]
//
// OUTPUT of Level Balancer
//
//                |----|>-|>|>---[]  Level = 3
//                |----|>-|>|>---[]
//                |----|>-|>|>---[]
//   [root]-------|                   |---|>----[] Level = 3
//                |----|>----D--------|
//                          (CGC)     |---|>-----[]
//
//

struct GraphNode
{
  GraphNode(int id,
       std::string name,
       int parentId)
    : id(id),
      name(name),
      parentId(parentId)
    {
    }

  int id;
  std::string name;
  int parentId;
  std::vector<int> childrenIds;
};

class LatanciesBalancer
{
 public:
 LatanciesBalancer(TreeBuilder* root,
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
  void findAllBuilders(TreeBuilder* builder);
  void expandBuilderGraph(TreeBuilder* builder);
  int getNodeIdByName(std::string name);
  odb::dbNet* getFirstInputNet(odb::dbInst* inst) const;
  float getVertexClkArrival(sta::Vertex* sinkVertex,
                                     odb::dbNet* topNet,
                                     odb::dbITerm* iterm);
  void computeAveSinkArrivals(TreeBuilder* builder);
  void computeSinkArrivalRecur(odb::dbNet* topClokcNet,
                                        odb::dbITerm* iterm,
                                        float& sumArrivals,
                                        unsigned& numSinks);
  bool propagateClock(odb::dbITerm* input);
  bool isSink(odb::dbITerm* iterm);

  TreeBuilder* root_ = nullptr;
  const CtsOptions* options_ = nullptr;
  Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  sta::dbSta* openSta_ = nullptr;
  sta::Graph* timing_graph_ = nullptr;
  double wireSegmentUnit_;
  std::vector<GraphNode> graph_;
};

}  // namespace cts
