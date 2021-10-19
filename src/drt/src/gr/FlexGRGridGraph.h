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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FLEX_GR_GRID_GRAPH_H_
#define _FLEX_GR_GRID_GRAPH_H_

#define GRGRIDGRAPHHISTCOSTSIZE 8
#define GRSUPPLYSIZE 8
#define GRDEMANDSIZE 16
#define GRFRACSIZE 1

#include <iostream>
#include <map>

#include "db/grObj/grPin.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "gr/FlexGRWavefront.h"

namespace fr {
class FlexGRWorker;
class FlexGRGridGraph
{
 public:
  // constructors
  FlexGRGridGraph(frDesign* designIn, FlexGRWorker* workerIn)
      : design_(designIn),
        grWorker_(workerIn),
        xgp_(nullptr),
        ygp_(nullptr),
        bits_(),
        prevDirs_(),
        srcs_(),
        dsts_(),
        xCoords_(),
        yCoords_(),
        zCoords_(),
        zHeights_(),
        zDirs_(),
        ggCongCost_(0),
        ggHistCost_(0),
        wavefront_(),
        is2DRouting_(false)
  {
  }
  // getters
  frTechObject* getTech() const { return design_->getTech(); }

  frDesign* getDesign() const { return design_; }

  FlexGRWorker* getGRWorker() const { return grWorker_; }

  bool is2D() { return is2DRouting_; }

  frCoord getEdgeLength(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    frCoord sol = 0;
    correct(x, y, z, dir);
    // if (isValid(x, y, z, dir)) {
    switch (dir) {
      case frDirEnum::E:
        sol = xCoords_[x + 1] - xCoords_[x];
        break;
      case frDirEnum::N:
        sol = yCoords_[y + 1] - yCoords_[y];
        break;
      case frDirEnum::U:
        sol = zHeights_[z + 1] - zHeights_[z];
        break;
      default:;
    }
    // }
    return sol;
  }

  void getDim(frMIdx& xDim, frMIdx& yDim, frMIdx& zDim) const
  {
    xDim = xCoords_.size();
    yDim = yCoords_.size();
    zDim = zCoords_.size();
  }

  frPoint& getPoint(frMIdx x, frMIdx y, frPoint& in) const
  {
    in.set(xCoords_[x], yCoords_[y]);
    return in;
  }

  frLayerNum getLayerNum(frMIdx z) const { return zCoords_[z]; }

  bool hasEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return getBit(idx, 0);
        case frDirEnum::N:
          return getBit(idx, 1);
        case frDirEnum::U:
          return getBit(idx, 2);
        default:
          return false;
      }
    } else {
      return false;
    }
  }

  bool hasBlock(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return getBit(idx, 3);
        case frDirEnum::N:
          return getBit(idx, 4);
        case frDirEnum::U:
          return getBit(idx, 5);
        default:
          return false;
      }
    } else {
      return false;
    }
  }

  bool hasHistoryCost(frMIdx x, frMIdx y, frMIdx z) const
  {
    auto idx = getIdx(x, y, z);
    return (getBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE));
  }

  bool hasCongCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    correct(x, y, z, dir);
    return (getRawDemand(x, y, z, dir) > getRawSupply(x, y, z, dir));
  }

  unsigned getHistoryCost(frMIdx x, frMIdx y, frMIdx z) const
  {
    unsigned histCost = 0;
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      histCost = getBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE);
    }
    return histCost;
  }

  // E == H; N == V; currently no U / D
  unsigned getSupply(frMIdx x,
                     frMIdx y,
                     frMIdx z,
                     frDirEnum dir,
                     bool isRaw = false) const
  {
    unsigned supply = 0;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          supply = getBits(idx, 24, GRSUPPLYSIZE);
          break;
        case frDirEnum::N:
          supply = getBits(idx, 16, GRSUPPLYSIZE);
          break;
        default:
          std::cout << "Error: unexpected dir " << int(dir)
                    << " in FlexGRGridGraph::getSupply\n";
      }
    }
    if (isRaw) {
      return supply << GRFRACSIZE;
    } else {
      return supply;
    }
  }

  unsigned getRawSupply(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    return getSupply(x, y, z, dir, true);
  }

  // E == H; N == V; currently no U / D
  unsigned getDemand(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return (getBits(idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE));
        case frDirEnum::N:
          return (getBits(idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE));
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::getDemand\n";
          return 0;
      }
    } else {
      return 0;
    }
  }

  // E == H; N == V; currently no U / D
  unsigned getRawDemand(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return (getBits(idx, 48, GRDEMANDSIZE));
        case frDirEnum::N:
          return (getBits(idx, 32, GRDEMANDSIZE));
        default:
          std::cout << "Error: unexpected dir " << int(dir)
                    << " in FlexGRGridGraph::getRawDemand\n";
          return 0;
      }
    } else {
      return 0;
    }
  }

  double getCongCost(int demand, int supply);

  // unsafe access, no check
  bool isAstarVisited(frMIdx x, frMIdx y, frMIdx z) const
  {
    return (getPrevAstarNodeDir(x, y, z) == frDirEnum::UNKNOWN);
  }

  // unsafe access, no check
  frDirEnum getPrevAstarNodeDir(frMIdx x, frMIdx y, frMIdx z) const
  {
    auto baseIdx = 3 * getIdx(x, y, z);
    return (frDirEnum)(((unsigned short) (prevDirs_[baseIdx]) << 2)
                       + ((unsigned short) (prevDirs_[baseIdx + 1]) << 1)
                       + ((unsigned short) (prevDirs_[baseIdx + 2]) << 0));
  }

  // unsafe access, no check
  bool isSrc(frMIdx x, frMIdx y, frMIdx z) const
  {
    return srcs_[getIdx(x, y, z)];
  }

  // unsafe access, no check
  bool isDst(frMIdx x, frMIdx y, frMIdx z) const
  {
    return dsts_[getIdx(x, y, z)];
  }

  // setters
  bool addEdge(frMIdx x,
               frMIdx y,
               frMIdx z,
               frDirEnum dir /*, const frBox &box*/)
  {
    bool sol = false;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          setBit(idx, 0);
          sol = true;
          break;
        case frDirEnum::N:
          setBit(idx, 1);
          sol = true;
          break;
        case frDirEnum::U:
          setBit(idx, 2);
          sol = true;
          break;
        default:;
      }
    } else {
      // cout <<"not valid edge";
    }
    // }
    return sol;
  }

  bool removeEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    bool sol = false;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          resetBit(idx, 0);
          sol = true;
          break;
        case frDirEnum::N:
          resetBit(idx, 1);
          sol = true;
          break;
        case frDirEnum::U:
          resetBit(idx, 2);
          sol = true;
          break;
        default:;
      }
    }
    return sol;
  }

  void setBlock(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return setBit(idx, 3);
        case frDirEnum::N:
          return setBit(idx, 4);
        case frDirEnum::U:
          return setBit(idx, 5);
        default:;
      }
    }
  }

  void resetBlock(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return resetBit(idx, 3);
        case frDirEnum::N:
          return resetBit(idx, 4);
        case frDirEnum::U:
          return resetBit(idx, 5);
        default:;
      }
    }
  }

  void setHistoryCost(frMIdx x, frMIdx y, frMIdx z, unsigned histCostIn)
  {
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      setBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE, histCostIn);
    }
  }

  void addHistoryCost(frMIdx x, frMIdx y, frMIdx z, unsigned in = 1)
  {
    auto idx = getIdx(x, y, z);
    addToBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE, in);
  }

  void decayHistoryCost(frMIdx x, frMIdx y, frMIdx z)
  {
    auto idx = getIdx(x, y, z);
    subToBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE, 1);
  }

  void decayHistoryCost(frMIdx x, frMIdx y, frMIdx z, double d)
  {
    auto idx = getIdx(x, y, z);
    int currCost = (getBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE));
    currCost *= d;
    currCost = std::max(0, currCost);
    setBits(idx, 8, GRGRIDGRAPHHISTCOSTSIZE, currCost);
  }

  // E == H; N == V; currently no U / D
  void setSupply(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned supplyIn)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return setBits(idx, 24, GRSUPPLYSIZE, supplyIn);
        case frDirEnum::N:
          return setBits(idx, 16, GRSUPPLYSIZE, supplyIn);
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::setSupply\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void setDemand(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, unsigned demandIn)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return setBits(
              idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
        case frDirEnum::N:
          return setBits(
              idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::setDemand\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void setRawDemand(frMIdx x,
                    frMIdx y,
                    frMIdx z,
                    frDirEnum dir,
                    unsigned rawDemandIn)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          return setBits(idx, 48, GRDEMANDSIZE, rawDemandIn);
        case frDirEnum::N:
          return setBits(idx, 32, GRDEMANDSIZE, rawDemandIn);
        default:
          std::cout
              << "Error: unexpected dir in FlexGRGridGraph::setRawDemand\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void addDemand(frMIdx x,
                 frMIdx y,
                 frMIdx z,
                 frDirEnum dir,
                 unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          addToBits(idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
          break;
        case frDirEnum::N:
          addToBits(idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
          break;
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::addDemand\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void addRawDemand(frMIdx x,
                    frMIdx y,
                    frMIdx z,
                    frDirEnum dir,
                    unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          addToBits(idx, 48, GRDEMANDSIZE, delta);
          break;
        case frDirEnum::N:
          addToBits(idx, 32, GRDEMANDSIZE, delta);
          break;
        default:
          std::cout
              << "Error: unexpected dir in FlexGRGridGraph::addRawDemand\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void subDemand(frMIdx x,
                 frMIdx y,
                 frMIdx z,
                 frDirEnum dir,
                 unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          subToBits(idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
          break;
        case frDirEnum::N:
          subToBits(idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, delta);
          break;
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::subDemand\n";
      }
    }
  }

  // E == H; N == V; currently no U / D
  void subRawDemand(frMIdx x,
                    frMIdx y,
                    frMIdx z,
                    frDirEnum dir,
                    unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          subToBits(idx, 48, GRDEMANDSIZE, delta);
          break;
        case frDirEnum::N:
          subToBits(idx, 32, GRDEMANDSIZE, delta);
          break;
        default:
          std::cout
              << "Error: unexpected dir in FlexGRGridGraph::subRawDemand\n";
      }
    }
  }

  // unsafe access, no idx check
  void setPrevAstarNodeDir(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    auto baseIdx = 3 * getIdx(x, y, z);
    prevDirs_[baseIdx] = ((unsigned short) dir >> 2) & 1;
    prevDirs_[baseIdx + 1] = ((unsigned short) dir >> 1) & 1;
    prevDirs_[baseIdx + 2] = ((unsigned short) dir) & 1;
  }
  // unsafe access, no idx check
  void setSrc(frMIdx x, frMIdx y, frMIdx z) { srcs_[getIdx(x, y, z)] = 1; }
  void setSrc(const FlexMazeIdx& mi)
  {
    srcs_[getIdx(mi.x(), mi.y(), mi.z())] = 1;
  }
  // unsafe access, no idx check
  void setDst(frMIdx x, frMIdx y, frMIdx z) { dsts_[getIdx(x, y, z)] = 1; }
  void setDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = 1;
  }
  void resetDst(frMIdx x, frMIdx y, frMIdx z) { dsts_[getIdx(x, y, z)] = 0; }
  void resetDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = 0;
  }

  void setCost(frUInt4 ggCongCostIn, frUInt4 ggHistCostIn)
  {
    ggCongCost_ = ggCongCostIn;
    ggHistCost_ = ggHistCostIn;
  }
  void set2D(bool in) { is2DRouting_ = in; }

  // functions
  void init();
  void print();
  void resetStatus();
  void resetPrevNodeDir();
  void resetSrc();
  void resetDst();
  bool search(std::vector<FlexMazeIdx>& connComps,
              grNode* nextPinNode,
              std::vector<FlexMazeIdx>& path,
              FlexMazeIdx& ccMazeIdx1,
              FlexMazeIdx& ccMazeIdx2,
              const frPoint& centerPt);

  void cleanup()
  {
    bits_.clear();
    bits_.shrink_to_fit();
    prevDirs_.clear();
    prevDirs_.shrink_to_fit();
    srcs_.clear();
    srcs_.shrink_to_fit();
    dsts_.clear();
    dsts_.shrink_to_fit();
    xCoords_.clear();
    xCoords_.shrink_to_fit();
    yCoords_.clear();
    yCoords_.shrink_to_fit();
    zCoords_.clear();
    zCoords_.shrink_to_fit();
    zHeights_.clear();
    zHeights_.shrink_to_fit();
    zDirs_.clear();
    zDirs_.shrink_to_fit();
  }

 protected:
  frDesign* design_;
  FlexGRWorker* grWorker_;
  frGCellPattern* xgp_;
  frGCellPattern* ygp_;

  // [0] hasEEdge; [1] hasNEdge; [2] hasUEdge
  // [3] blockE;   [4] blockN;   [5] blockU
  // [6] empty;    [7] empty
  // [15-8]  history cost
  // [31-24] supply H; [23-16] supply V // last bit is fractional
  // [63-48] demand H; [47-32] demand V // last bit is fractional
  std::vector<unsigned long long> bits_;
  std::vector<bool> prevDirs_;
  std::vector<bool> srcs_;
  std::vector<bool> dsts_;
  std::vector<frCoord> xCoords_;
  std::vector<frCoord> yCoords_;
  std::vector<frLayerNum> zCoords_;
  std::vector<frCoord> zHeights_;
  std::vector<bool> zDirs_;  // is horz dir
  unsigned ggCongCost_;
  unsigned ggHistCost_;

  FlexGRWavefront wavefront_;

  // flags
  bool is2DRouting_;

  // internal getters
  bool getBit(frMIdx idx, frMIdx pos) const { return (bits_[idx] >> pos) & 1; }
  unsigned getBits(frMIdx idx, frMIdx pos, unsigned length) const
  {
    auto tmp = bits_[idx] & (((1ull << length) - 1) << pos);  // mask
    return tmp >> pos;
  }
  frMIdx getMazeXIdx(frCoord in) const
  {
    auto it = std::lower_bound(xCoords_.begin(), xCoords_.end(), in);
    return it - xCoords_.begin();
  }
  frMIdx getMazeYIdx(frCoord in) const
  {
    auto it = std::lower_bound(yCoords_.begin(), yCoords_.end(), in);
    return it - yCoords_.begin();
  }
  frMIdx getMazeZIdx(frLayerNum in) const
  {
    auto it = std::lower_bound(zCoords_.begin(), zCoords_.end(), in);
    return it - zCoords_.begin();
  }
  FlexMazeIdx& getMazeIdx(const frPoint& p,
                          frLayerNum layerNum,
                          FlexMazeIdx& mIdx) const
  {
    mIdx.set(getMazeXIdx(p.x()), getMazeYIdx(p.y()), getMazeZIdx(layerNum));
    return mIdx;
  }
  frCoord getZHeight(frMIdx in) const { return zHeights_[in]; }
  bool getZDir(frMIdx in) const { return zDirs_[in]; }
  frMIdx getIdx(frMIdx xIdx, frMIdx yIdx, frMIdx zIdx) const
  {
    return (getZDir(zIdx)) ? (xIdx + yIdx * xCoords_.size()
                              + zIdx * xCoords_.size() * yCoords_.size())
                           : (yIdx + xIdx * yCoords_.size()
                              + zIdx * xCoords_.size() * yCoords_.size());
  }

  // internal setters
  void setBit(frMIdx idx, frMIdx pos) { bits_[idx] |= 1 << pos; }
  void resetBit(frMIdx idx, frMIdx pos) { bits_[idx] &= ~(1 << pos); }
  void addToBits(frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val)
  {
    auto tmp = getBits(idx, pos, length) + val;
    tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
    setBits(idx, pos, length, tmp);
  }
  void subToBits(frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val)
  {
    int tmp = (int) getBits(idx, pos, length) - (int) val;
    tmp = (tmp < 0) ? 0 : tmp;
    setBits(idx, pos, length, tmp);
  }
  void setBits(frMIdx idx, frMIdx pos, frUInt4 length, frUInt4 val)
  {
    bits_[idx] &= ~(((1ull << length) - 1) << pos);  // clear related bits to 0
    bits_[idx] |= ((unsigned long long) val & ((1ull << length) - 1))
                  << pos;  // only get last length bits of val
  }

  // internal utility
  void correct(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
    switch (dir) {
      case frDirEnum::W:
        x--;
        dir = frDirEnum::E;
        break;
      case frDirEnum::S:
        y--;
        dir = frDirEnum::N;
        break;
      case frDirEnum::D:
        z--;
        dir = frDirEnum::U;
        break;
      default:;
    }
    return;
  }
  void correctU(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
    switch (dir) {
      case frDirEnum::D:
        z--;
        dir = frDirEnum::U;
        break;
      default:;
    }
    return;
  }

  void reverse(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
    switch (dir) {
      case frDirEnum::E:
        x++;
        dir = frDirEnum::W;
        break;
      case frDirEnum::S:
        y--;
        dir = frDirEnum::N;
        break;
      case frDirEnum::W:
        x--;
        dir = frDirEnum::E;
        break;
      case frDirEnum::N:
        y++;
        dir = frDirEnum::S;
        break;
      case frDirEnum::U:
        z++;
        dir = frDirEnum::D;
        break;
      case frDirEnum::D:
        z--;
        dir = frDirEnum::U;
        break;
      default:;
    }
    return;
  }
  void getPrevGrid(frMIdx& gridX,
                   frMIdx& gridY,
                   frMIdx& gridZ,
                   const frDirEnum dir) const;
  void getNextGrid(frMIdx& gridX,
                   frMIdx& gridY,
                   frMIdx& gridZ,
                   const frDirEnum dir);
  bool isValid(frMIdx x, frMIdx y, frMIdx z) const
  {
    if (x < 0 || y < 0 || z < 0 || x >= (frMIdx) xCoords_.size()
        || y >= (frMIdx) yCoords_.size() || z >= (frMIdx) zCoords_.size()) {
      return false;
    } else {
      return true;
    }
  }

  // internal init utility
  void initCoords();
  void initGrids();
  void initEdges();
  // maze related
  frCost getEstCost(const FlexMazeIdx& src,
                    const FlexMazeIdx& dstMazeIdx1,
                    const FlexMazeIdx& dstMazeIdx2,
                    const frDirEnum& dir);
  frCost getNextPathCost(const FlexGRWavefrontGrid& currGrid,
                         const frDirEnum& dir);
  frDirEnum getLastDir(const std::bitset<WAVEFRONTBITSIZE>& buffer);
  void traceBackPath(const FlexGRWavefrontGrid& currGrid,
                     std::vector<FlexMazeIdx>& path,
                     std::vector<FlexMazeIdx>& root,
                     FlexMazeIdx& ccMazeIdx1,
                     FlexMazeIdx& ccMazeIdx2);
  void expandWavefront(FlexGRWavefrontGrid& currGrid,
                       const FlexMazeIdx& dstMazeIdx1,
                       const FlexMazeIdx& dstMazeIdx2,
                       const frPoint& centerPt);
  bool isExpandable(const FlexGRWavefrontGrid& currGrid, frDirEnum dir);
  FlexMazeIdx getTailIdx(const FlexMazeIdx& currIdx,
                         const FlexGRWavefrontGrid& currGrid);
  void expand(FlexGRWavefrontGrid& currGrid,
              const frDirEnum& dir,
              const FlexMazeIdx& dstMazeIdx1,
              const FlexMazeIdx& dstMazeIdx2,
              const frPoint& centerPt);

  friend class FlexGRWorker;
};
}  // namespace fr

#endif
