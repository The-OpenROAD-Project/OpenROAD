// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <bitset>
#include <memory>
#include <queue>

#include "db/infra/frBox.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "global.h"

namespace drt {
class FlexWavefrontGrid
{
 public:
  FlexWavefrontGrid(int xIn,
                    int yIn,
                    int zIn,
                    frCoord vLengthXIn,
                    frCoord vLengthYIn,
                    bool prevViaUpIn,
                    frCoord tLengthIn,
                    frCoord distIn,
                    frCost pathCostIn,
                    frCost costIn,
                    const std::bitset<WAVEFRONTBITSIZE>& backTraceBufferIn = {})
      : xIdx_(xIn),
        yIdx_(yIn),
        zIdx_(zIn),
        pathCost_(pathCostIn),
        cost_(costIn),
        vLengthX_(vLengthXIn),
        vLengthY_(vLengthYIn),
        dist_(distIn),
        prevViaUp_(prevViaUpIn),
        tLength_(tLengthIn),
        backTraceBuffer_(backTraceBufferIn)
  {
  }
  bool operator<(const FlexWavefrontGrid& b) const
  {
    if (cost_ != b.cost_) {
      return cost_ > b.cost_;  // prefer smaller cost
    }
    if (dist_ != b.dist_) {
      return dist_ > b.dist_;  // prefer routing close to pin gravity center
                               // (centerPt)
    }
    if (zIdx_ != b.zIdx_) {
      return zIdx_ < b.zIdx_;  // prefer upper layer
    }
    return pathCost_ < b.pathCost_;  // prefer larger pathcost, DFS-style
  }
  // getters
  frMIdx x() const { return xIdx_; }
  frMIdx y() const { return yIdx_; }
  frMIdx z() const { return zIdx_; }
  frCost getPathCost() const { return pathCost_; }
  frCost getCost() const { return cost_; }
  const std::bitset<WAVEFRONTBITSIZE>& getBackTraceBuffer() const
  {
    return backTraceBuffer_;
  }
  frCoord getLength() const { return vLengthX_; }
  void getVLength(frCoord& vLengthXIn, frCoord& vLengthYIn) const
  {
    vLengthXIn = vLengthX_;
    vLengthYIn = vLengthY_;
  }
  bool isPrevViaUp() const { return prevViaUp_; }
  frCoord getTLength() const { return tLength_; }
  frUInt4 getId() const { return id_; }
  frUInt4 getParentId() const { return parent_id_; }
  // setters

  void resetLength()
  {
    vLengthX_ = 0;
    vLengthY_ = 0;
  }
  void setPrevViaUp(bool in) { prevViaUp_ = in; }
  frDirEnum getLastDir() const
  {
    auto currDirVal = backTraceBuffer_.to_ulong() & 0b111u;
    return static_cast<frDirEnum>(currDirVal);
  }
  bool isBufferFull() const
  {
    std::bitset<WAVEFRONTBITSIZE> mask = WAVEFRONTBUFFERHIGHMASK;
    return (mask & backTraceBuffer_).any();
  }
  frDirEnum shiftAddBuffer(const frDirEnum& dir)
  {
    auto retBS = static_cast<frDirEnum>(
        (backTraceBuffer_ >> (WAVEFRONTBITSIZE - DIRBITSIZE)).to_ulong());
    backTraceBuffer_ <<= DIRBITSIZE;
    std::bitset<WAVEFRONTBITSIZE> newBS = (unsigned) dir;
    backTraceBuffer_ |= newBS;
    return retBS;
  }
  void setSrcTaperBox(const frBox3D* b) { srcTaperBox_ = b; }
  const frBox3D* getSrcTaperBox() const { return srcTaperBox_; }
  void setId(const frUInt4 in) { id_ = in; }
  void setParentId(const frUInt4 in) { parent_id_ = in; }

 private:
  frMIdx xIdx_, yIdx_, zIdx_;
  frCost pathCost_;  // path cost
  frCost cost_;      // path + est cost
  frCoord vLengthX_;
  frCoord vLengthY_;
  frCoord dist_;  // to maze center
  bool prevViaUp_;
  frCoord tLength_;  // length since last turn
  std::bitset<WAVEFRONTBITSIZE> backTraceBuffer_;
  const frBox3D* srcTaperBox_ = nullptr;
  frUInt4 id_{0};
  frUInt4 parent_id_{0};
};

class myPriorityQueue : public std::priority_queue<FlexWavefrontGrid>
{
 public:
  void cleanup() { this->c.clear(); }
  void fit()
  {
    this->c.clear();
    this->c.shrink_to_fit();
  }
};

class FlexWavefront
{
 public:
  bool empty() const { return wavefrontPQ_.empty(); }
  const FlexWavefrontGrid& top() const { return wavefrontPQ_.top(); }
  void pop() { wavefrontPQ_.pop(); }
  void push(const FlexWavefrontGrid& in) { wavefrontPQ_.push(in); }
  unsigned int size() const { return wavefrontPQ_.size(); }
  void cleanup() { wavefrontPQ_.cleanup(); }
  void fit() { wavefrontPQ_.fit(); }

 private:
  myPriorityQueue wavefrontPQ_;
};
}  // namespace drt
