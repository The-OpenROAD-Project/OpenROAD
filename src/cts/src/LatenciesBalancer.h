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
                double scalingUnit)
      : root_(root),
        options_(options),
        logger_(logger),
        db_(db),
        wireSegmentUnit_(scalingUnit)
  {
  }

  void run();
  void addBufferLevels(TreeBuilder* builder,
                       const std::vector<ClockInst*>& cluster,
                       ClockSubNet* driverNet,
                       unsigned bufLevels,
                       const std::string& nameSuffix);
  void fixTreeLevels(TreeBuilder* builder,
                     unsigned parentDepth,
                     unsigned maxTreeDepth);
  unsigned computeMaxTreeDepth(TreeBuilder* parent);
  void findAllBuilders(TreeBuilder* builder);
  void expandBuilderGraph(TreeBuilder* builder);
  int getNodeIdByName(std::string name);
  odb::dbNet* getFirstInputNet(odb::dbInst* inst) const;

 private:

  TreeBuilder* root_;
  const CtsOptions* options_;
  Logger* logger_;
  odb::dbDatabase* db_ = nullptr;
  double wireSegmentUnit_;
  std::vector<GraphNode> graph_;
};

}  // namespace cts
