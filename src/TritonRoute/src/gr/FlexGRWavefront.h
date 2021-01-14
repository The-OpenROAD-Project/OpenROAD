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

#ifndef _FLEX_GR_WF_H_
#define _FLEX_GR_WF_H_

#include <queue>
#include <bitset>
#include <vector>
#include "frBaseTypes.h"
#include "dr/FlexMazeTypes.h"
#include <memory>
#include "global.h"

namespace fr {
  class FlexGRWavefrontGrid {
  public:
    FlexGRWavefrontGrid(): xIdx(-1), yIdx(-1), zIdx(-1), pathCost(0), cost(0),
                         dist(0), backTraceBuffer() {}
    FlexGRWavefrontGrid(int xIn, int yIn, int zIn, 
                      frCoord distIn, frCost pathCostIn, frCost costIn): 
                      xIdx(xIn), yIdx(yIn), zIdx(zIn), pathCost(pathCostIn), cost(costIn), 
                      dist(distIn), backTraceBuffer() {}
    FlexGRWavefrontGrid(int xIn, int yIn, int zIn, 
                      frCoord distIn, frCost pathCostIn, frCost costIn,
                      std::bitset<WAVEFRONTBITSIZE> backTraceBufferIn): 
                      xIdx(xIn), yIdx(yIn), zIdx(zIn), pathCost(pathCostIn), cost(costIn), 
                      dist(distIn), backTraceBuffer(backTraceBufferIn) {}
    bool operator<(const FlexGRWavefrontGrid &b) const {
      if (this->cost != b.cost) {
        return this->cost > b.cost; // prefer smaller cost
      } else {
        if (this->dist != b.dist) {
          return this->dist > b.dist; // prefer routing close to pin gravity center (centerPt)
        } else {
          if (this->zIdx != b.zIdx) {
            return this->zIdx < b.zIdx; // prefer upper layer
          } else {
            return this->pathCost < b.pathCost; //prefer larger pathcost, DFS-style
            //return this->pathCost < b.pathCost; //prefer smaller pathcost, BFS-style, test1 shows better results
          }
        }
      }
    }
    // getters
    frMIdx x() const {
      return xIdx;
    }
    frMIdx y() const {
      return yIdx;
    }
    frMIdx z() const {
      return zIdx;
    }
    frCost getPathCost() const {
      return pathCost;
    }
    frCost getCost() const {
      return cost;
    }
    std::bitset<WAVEFRONTBITSIZE> getBackTraceBuffer() const {
      return backTraceBuffer;
    }

    // setters
    frDirEnum getLastDir() const {
      auto currDirVal = backTraceBuffer.to_ulong() & 0b111u;
      return static_cast<frDirEnum>(currDirVal);
    }
    
    bool isBufferFull() const {
      std::bitset<WAVEFRONTBITSIZE> mask = WAVEFRONTBUFFERHIGHMASK;
      return (mask & backTraceBuffer).any();
    }

    frDirEnum shiftAddBuffer(const frDirEnum &dir) {
      auto retBS = static_cast<frDirEnum>((backTraceBuffer >> (WAVEFRONTBITSIZE - DIRBITSIZE)).to_ulong());
      backTraceBuffer <<= DIRBITSIZE;
      std::bitset<WAVEFRONTBITSIZE> newBS = (unsigned)dir;
      backTraceBuffer |= newBS;
      return retBS;
    }
  protected:
    frMIdx xIdx, yIdx, zIdx;
    frCost pathCost; // path cost
    frCost cost; // path + est cost
    frCoord dist; // to maze center
    std::bitset<GRWAVEFRONTBITSIZE> backTraceBuffer;
  };

  class FlexGRWavefront {
  public:
    bool empty() const {
      return wavefrontPQ.empty();
    }
    const FlexGRWavefrontGrid& top() const {
      return wavefrontPQ.top();
    }
    void pop() {
      wavefrontPQ.pop();
    }
    void push(const FlexGRWavefrontGrid &in) {
      wavefrontPQ.push(in);
    }
    unsigned int size() const {
      return wavefrontPQ.size();
    }
    void cleanup() {
      wavefrontPQ = std::priority_queue<FlexGRWavefrontGrid>();
    }
  protected:
    std::priority_queue<FlexGRWavefrontGrid> wavefrontPQ;
  };
}

#endif