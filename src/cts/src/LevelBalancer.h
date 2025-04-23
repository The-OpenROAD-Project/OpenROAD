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

class LevelBalancer
{
 public:
  LevelBalancer(TreeBuilder* root,
                const CtsOptions* options,
                Logger* logger,
                double scalingUnit)
      : root_(root),
        options_(options),
        logger_(logger),
        levelBufCount_(0),
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

 private:
  using CellLevelMap
      = std::map<odb::dbInst*, std::pair<unsigned, TreeBuilder*>>;

  TreeBuilder* root_;
  const CtsOptions* options_;
  Logger* logger_;
  CellLevelMap cgcLevelMap_;
  unsigned levelBufCount_;
  double wireSegmentUnit_;
};

}  // namespace cts
