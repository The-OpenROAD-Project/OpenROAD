/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FLEX_WF_H
#define _FLEX_WF_H

#include <queue>
#include <bitset>
#include <vector>
#include "frBaseTypes.h"
#include "dr/FlexMazeTypes.h"
#include <memory>
#include "global.h"
//#include <boost/pool/pool_alloc.hpp>

namespace fr {
  class FlexWavefrontGrid {
  public:
    FlexWavefrontGrid(): xIdx_(-1), yIdx_(-1), zIdx_(-1), pathCost_(0), cost_(0), layerPathArea_(0), 
                         vLengthX_(std::numeric_limits<frCoord>::max()), 
                         vLengthY_(std::numeric_limits<frCoord>::max()), 
                         dist_(0), prevViaUp_(false), 
                         tLength_(std::numeric_limits<frCoord>::max()), backTraceBuffer_() {}
    FlexWavefrontGrid(int xIn, int yIn, int zIn, frCoord layerPathAreaIn, 
                      frCoord vLengthXIn, frCoord vLengthYIn,
                      bool prevViaUpIn, frCoord tLengthIn,
                      frCoord distIn, frCost pathCostIn, frCost costIn/*, frDirEnum preTurnDirIn*/): 
                      xIdx_(xIn), yIdx_(yIn), zIdx_(zIn), pathCost_(pathCostIn), cost_(costIn), 
                      layerPathArea_(layerPathAreaIn), vLengthX_(vLengthXIn), vLengthY_(vLengthYIn),
                      dist_(distIn), prevViaUp_(prevViaUpIn), tLength_(tLengthIn), backTraceBuffer_() {}
    FlexWavefrontGrid(int xIn, int yIn, int zIn, frCoord layerPathAreaIn, 
                      frCoord vLengthXIn, frCoord vLengthYIn,
                      bool prevViaUpIn, frCoord tLengthIn,
                      frCoord distIn, frCost pathCostIn, frCost costIn, 
                      std::bitset<WAVEFRONTBITSIZE> backTraceBufferIn): 
                      xIdx_(xIn), yIdx_(yIn), zIdx_(zIn), pathCost_(pathCostIn), cost_(costIn), 
                      layerPathArea_(layerPathAreaIn), vLengthX_(vLengthXIn), vLengthY_(vLengthYIn),
                      dist_(distIn), prevViaUp_(prevViaUpIn), tLength_(tLengthIn), backTraceBuffer_(backTraceBufferIn) {}
    bool operator<(const FlexWavefrontGrid &b) const {
      if (cost_ != b.cost_) {
        return cost_ > b.cost_; // prefer smaller cost
      } else {
        if (dist_ != b.dist_) {
          return dist_ > b.dist_; // prefer routing close to pin gravity center (centerPt)
        } else {
          if (zIdx_ != b.zIdx_) {
            return zIdx_ < b.zIdx_; // prefer upper layer
          } else {
            return pathCost_ < b.pathCost_; //prefer larger pathcost, DFS-style
          }
        }
      }
    }
    // getters
    frMIdx x() const {
      return xIdx_;
    }
    frMIdx y() const {
      return yIdx_;
    }
    frMIdx z() const {
      return zIdx_;
    }
    frCost getPathCost() const {
      return pathCost_;
    }
    frCost getCost() const {
      return cost_;
    }
    std::bitset<WAVEFRONTBITSIZE> getBackTraceBuffer() const {
      return backTraceBuffer_;
    }
    frCoord getLayerPathArea() const {
      return layerPathArea_;
    }
    frCoord getLength() const {
      return vLengthX_;
    }
    void getVLength(frCoord &vLengthXIn, frCoord &vLengthYIn) const {
      vLengthXIn = vLengthX_;
      vLengthYIn = vLengthY_;
    }
    bool isPrevViaUp() const {
      return prevViaUp_;
    }
    frCoord getTLength() const {
      return tLength_;
    }
    // setters
    void addLayerPathArea(frCoord in) {
      layerPathArea_ += in;
    }
    void resetLayerPathArea() {
      layerPathArea_ = 0;
    }

    void resetLength() {
      vLengthX_ = 0;
      vLengthY_ = 0;
    }
    void setPrevViaUp(bool in) {
      prevViaUp_ = in;
    }
    frDirEnum getLastDir() const {
      auto currDirVal = backTraceBuffer_.to_ulong() & 0b111u;
      return static_cast<frDirEnum>(currDirVal);
    }
    bool isBufferFull() const {
      std::bitset<WAVEFRONTBITSIZE> mask = WAVEFRONTBUFFERHIGHMASK;
      return (mask & backTraceBuffer_).any();
    }
    frDirEnum shiftAddBuffer(const frDirEnum &dir) {
      auto retBS = static_cast<frDirEnum>((backTraceBuffer_ >> (WAVEFRONTBITSIZE - DIRBITSIZE)).to_ulong());
      backTraceBuffer_ <<= DIRBITSIZE;
      std::bitset<WAVEFRONTBITSIZE> newBS = (unsigned)dir;
      backTraceBuffer_ |= newBS;
      return retBS;
    }
    void setSrcTaperBox(frBox3D* b){
        srcTaperBox = b;
    }
    frBox3D* getSrcTaperBox() const{
        return srcTaperBox;
    }
  protected:
    frMIdx xIdx_, yIdx_, zIdx_;
    frCost pathCost_; // path cost
    frCost cost_; // path + est cost
    frCoord layerPathArea_;
    frCoord vLengthX_;
    frCoord vLengthY_;
    frCoord dist_; // to maze center
    bool    prevViaUp_;
    frCoord tLength_; // length since last turn
    std::bitset<WAVEFRONTBITSIZE> backTraceBuffer_;
    frBox3D* srcTaperBox = nullptr;
  };

  class myPriorityQueue: public std::priority_queue<FlexWavefrontGrid> {
  public:
    void cleanup() {
      this->c.clear();
    }
    void fit() {
      this->c.clear();
      this->c.shrink_to_fit();
    }
    void init(int val) {
      this->c.reserve(val);
    }
  };

  class FlexWavefront {
  public:
    bool empty() const {
      return wavefrontPQ_.empty();
    }
    const FlexWavefrontGrid& top() const {
      return wavefrontPQ_.top();
    }
    void pop() {
      wavefrontPQ_.pop();
    }
    void push(const FlexWavefrontGrid &in) {
      wavefrontPQ_.push(in);
    }
    unsigned int size() const {
      return wavefrontPQ_.size();
    }
    void cleanup() {
      wavefrontPQ_.cleanup();
    }
    void fit() {
      wavefrontPQ_.fit();
    }
  protected:
    myPriorityQueue wavefrontPQ_;
  };
}


#endif
