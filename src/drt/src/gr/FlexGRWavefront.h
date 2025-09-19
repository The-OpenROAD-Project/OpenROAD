// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <bitset>
#include <memory>
#include <queue>
#include <vector>

#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "global.h"

namespace drt {
class FlexGRWavefrontGrid
{
 public:
  FlexGRWavefrontGrid() = default;
  FlexGRWavefrontGrid(int xIn,
                      int yIn,
                      int zIn,
                      frCoord distIn,
                      frCost pathCostIn,
                      frCost costIn)
      : xIdx_(xIn),
        yIdx_(yIn),
        zIdx_(zIn),
        pathCost_(pathCostIn),
        cost_(costIn),
        dist_(distIn)
  {
  }
  FlexGRWavefrontGrid(int xIn,
                      int yIn,
                      int zIn,
                      frCoord distIn,
                      frCost pathCostIn,
                      frCost costIn,
                      std::bitset<GRWAVEFRONTBITSIZE> backTraceBufferIn)
      : xIdx_(xIn),
        yIdx_(yIn),
        zIdx_(zIn),
        pathCost_(pathCostIn),
        cost_(costIn),
        dist_(distIn),
        backTraceBuffer_(backTraceBufferIn)
  {
  }
  bool operator<(const FlexGRWavefrontGrid& b) const
  {
    if (this->cost_ != b.cost_) {
      return this->cost_ > b.cost_;  // prefer smaller cost
    }
    if (this->dist_ != b.dist_) {
      return this->dist_ > b.dist_;  // prefer routing close to pin gravity
                                     // center (centerPt)
    }
    if (this->zIdx_ != b.zIdx_) {
      return this->zIdx_ < b.zIdx_;  // prefer upper layer
    }
    return this->pathCost_ < b.pathCost_;  // prefer larger pathcost, DFS-style
  }
  // getters
  frMIdx x() const { return xIdx_; }
  frMIdx y() const { return yIdx_; }
  frMIdx z() const { return zIdx_; }
  frCost getPathCost() const { return pathCost_; }
  frCost getCost() const { return cost_; }
  std::bitset<GRWAVEFRONTBITSIZE> getBackTraceBuffer() const
  {
    return backTraceBuffer_;
  }

  // setters
  frDirEnum getLastDir() const
  {
    auto currDirVal = backTraceBuffer_.to_ulong() & 0b111u;
    return static_cast<frDirEnum>(currDirVal);
  }

  bool isBufferFull() const
  {
    std::bitset<GRWAVEFRONTBITSIZE> mask = GRWAVEFRONTBUFFERHIGHMASK;
    return (mask & backTraceBuffer_).any();
  }

  frDirEnum shiftAddBuffer(const frDirEnum& dir)
  {
    auto retBS = static_cast<frDirEnum>(
        (backTraceBuffer_ >> (GRWAVEFRONTBITSIZE - DIRBITSIZE)).to_ulong());
    backTraceBuffer_ <<= DIRBITSIZE;
    std::bitset<GRWAVEFRONTBITSIZE> newBS = (unsigned) dir;
    backTraceBuffer_ |= newBS;
    return retBS;
  }

 private:
  frMIdx xIdx_{-1}, yIdx_{-1}, zIdx_{-1};
  frCost pathCost_{0};  // path cost
  frCost cost_{0};      // path + est cost
  frCoord dist_{0};     // to maze center
  std::bitset<GRWAVEFRONTBITSIZE> backTraceBuffer_;
};

class FlexGRWavefront
{
 public:
  bool empty() const { return wavefrontPQ_.empty(); }
  const FlexGRWavefrontGrid& top() const { return wavefrontPQ_.top(); }
  void pop() { wavefrontPQ_.pop(); }
  void push(const FlexGRWavefrontGrid& in) { wavefrontPQ_.push(in); }
  unsigned int size() const { return wavefrontPQ_.size(); }
  void cleanup() { wavefrontPQ_ = std::priority_queue<FlexGRWavefrontGrid>(); }

 private:
  std::priority_queue<FlexGRWavefrontGrid> wavefrontPQ_;
};
}  // namespace drt
