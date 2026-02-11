// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

constexpr int GRGRIDGRAPHHISTCOSTSIZE = 8;
constexpr int GRSUPPLYSIZE = 8;
constexpr int GRDEMANDSIZE = 16;
constexpr int GRFRACSIZE = 1;

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "db/grObj/grNode.h"
#include "db/grObj/grPin.h"
#include "db/obj/frGCellPattern.h"
#include "db/tech/frTechObject.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "gr/FlexGRWavefront.h"

namespace drt {
class FlexGRWorker;
class FlexGRGridGraph
{
 public:
  // constructors
  FlexGRGridGraph(frDesign* designIn,
                  FlexGRWorker* workerIn,
                  RouterConfiguration* router_cfg)
      : design_(designIn), grWorker_(workerIn), router_cfg_(router_cfg)
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

  odb::Point& getPoint(frMIdx x, frMIdx y, odb::Point& in) const
  {
    in = {xCoords_[x], yCoords_[y]};
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
    }
    return supply;
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
    return (frDirEnum) (((uint16_t) (prevDirs_[baseIdx]) << 2)
                        + ((uint16_t) (prevDirs_[baseIdx + 1]) << 1)
                        + ((uint16_t) (prevDirs_[baseIdx + 2]) << 0));
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
               frDirEnum dir /*, const Rect &box*/)
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
          setBit(idx, 3);
          break;
        case frDirEnum::N:
          setBit(idx, 4);
          break;
        case frDirEnum::U:
          setBit(idx, 5);
          break;
        default:
          break;
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
          resetBit(idx, 3);
          break;
        case frDirEnum::N:
          resetBit(idx, 4);
          break;
        case frDirEnum::U:
          resetBit(idx, 5);
          break;
        default:
          break;
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
          setBits(idx, 24, GRSUPPLYSIZE, supplyIn);
          break;
        case frDirEnum::N:
          setBits(idx, 16, GRSUPPLYSIZE, supplyIn);
          break;
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::setSupply\n";
          break;
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
          setBits(idx, 48 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
          break;
        case frDirEnum::N:
          setBits(idx, 32 + GRFRACSIZE, GRDEMANDSIZE - GRFRACSIZE, demandIn);
          break;
        default:
          std::cout << "Error: unexpected dir in FlexGRGridGraph::setDemand\n";
          break;
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
          setBits(idx, 48, GRDEMANDSIZE, rawDemandIn);
          break;
        case frDirEnum::N:
          setBits(idx, 32, GRDEMANDSIZE, rawDemandIn);
          break;
        default:
          std::cout
              << "Error: unexpected dir in FlexGRGridGraph::setRawDemand\n";
          break;
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
    prevDirs_[baseIdx] = ((uint16_t) dir >> 2) & 1;
    prevDirs_[baseIdx + 1] = ((uint16_t) dir >> 1) & 1;
    prevDirs_[baseIdx + 2] = ((uint16_t) dir) & 1;
  }
  // unsafe access, no idx check
  void setSrc(frMIdx x, frMIdx y, frMIdx z) { srcs_[getIdx(x, y, z)] = true; }
  void setSrc(const FlexMazeIdx& mi)
  {
    srcs_[getIdx(mi.x(), mi.y(), mi.z())] = true;
  }
  // unsafe access, no idx check
  void setDst(frMIdx x, frMIdx y, frMIdx z) { dsts_[getIdx(x, y, z)] = true; }
  void setDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = true;
  }
  void resetDst(frMIdx x, frMIdx y, frMIdx z)
  {
    dsts_[getIdx(x, y, z)] = false;
  }
  void resetDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = false;
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
              const odb::Point& centerPt);

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

 private:
  frDesign* design_{nullptr};
  FlexGRWorker* grWorker_{nullptr};
  frGCellPattern* xgp_{nullptr};
  frGCellPattern* ygp_{nullptr};
  RouterConfiguration* router_cfg_;

  // [0] hasEEdge; [1] hasNEdge; [2] hasUEdge
  // [3] blockE;   [4] blockN;   [5] blockU
  // [6] empty;    [7] empty
  // [15-8]  history cost
  // [31-24] supply H; [23-16] supply V // last bit is fractional
  // [63-48] demand H; [47-32] demand V // last bit is fractional
  std::vector<uint64_t> bits_;
  std::vector<bool> prevDirs_;
  std::vector<bool> srcs_;
  std::vector<bool> dsts_;
  std::vector<frCoord> xCoords_;
  std::vector<frCoord> yCoords_;
  std::vector<frLayerNum> zCoords_;
  std::vector<frCoord> zHeights_;
  std::vector<bool> zDirs_;  // is horz dir
  unsigned ggCongCost_{0};
  unsigned ggHistCost_{0};

  FlexGRWavefront wavefront_;

  // flags
  bool is2DRouting_{false};

  // internal getters
  bool getBit(frMIdx idx, frMIdx pos) const { return (bits_[idx] >> pos) & 1; }
  unsigned getBits(frMIdx idx, frMIdx pos, unsigned length) const
  {
    auto tmp = bits_[idx] & (((1ull << length) - 1) << pos);  // mask
    return tmp >> pos;
  }
  frMIdx getMazeXIdx(frCoord in) const
  {
    auto it = std::ranges::lower_bound(xCoords_, in);
    return it - xCoords_.begin();
  }
  frMIdx getMazeYIdx(frCoord in) const
  {
    auto it = std::ranges::lower_bound(yCoords_, in);
    return it - yCoords_.begin();
  }
  frMIdx getMazeZIdx(frLayerNum in) const
  {
    auto it = std::ranges::lower_bound(zCoords_, in);
    return it - zCoords_.begin();
  }
  FlexMazeIdx& getMazeIdx(const odb::Point& p,
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
    bits_[idx] |= ((uint64_t) val & ((1ull << length) - 1))
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
  }
  void getPrevGrid(frMIdx& gridX,
                   frMIdx& gridY,
                   frMIdx& gridZ,
                   frDirEnum dir) const;
  void getNextGrid(frMIdx& gridX, frMIdx& gridY, frMIdx& gridZ, frDirEnum dir);
  bool isValid(frMIdx x, frMIdx y, frMIdx z) const
  {
    if (x < 0 || y < 0 || z < 0 || x >= (frMIdx) xCoords_.size()
        || y >= (frMIdx) yCoords_.size() || z >= (frMIdx) zCoords_.size()) {
      return false;
    }
    return true;
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
  frDirEnum getLastDir(const std::bitset<GRWAVEFRONTBITSIZE>& buffer);
  void traceBackPath(const FlexGRWavefrontGrid& currGrid,
                     std::vector<FlexMazeIdx>& path,
                     std::vector<FlexMazeIdx>& root,
                     FlexMazeIdx& ccMazeIdx1,
                     FlexMazeIdx& ccMazeIdx2);
  void expandWavefront(FlexGRWavefrontGrid& currGrid,
                       const FlexMazeIdx& dstMazeIdx1,
                       const FlexMazeIdx& dstMazeIdx2,
                       const odb::Point& centerPt);
  bool isExpandable(const FlexGRWavefrontGrid& currGrid, frDirEnum dir);
  FlexMazeIdx getTailIdx(const FlexMazeIdx& currIdx,
                         const FlexGRWavefrontGrid& currGrid);
  void expand(FlexGRWavefrontGrid& currGrid,
              const frDirEnum& dir,
              const FlexMazeIdx& dstMazeIdx1,
              const FlexMazeIdx& dstMazeIdx2,
              const odb::Point& centerPt);

  friend class FlexGRWorker;
};
}  // namespace drt
