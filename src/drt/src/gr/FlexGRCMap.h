// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

constexpr int CMAPHISTSIZE = 8;
constexpr int CMAPSUPPLYSIZE = 8;
constexpr int CMAPDEMANDSIZE = 16;
constexpr int CMAPFRACSIZE = 1;

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frGCellPattern.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/dbTypes.h"

namespace drt {
class frTrackPattern;
class FlexGRCMap
{
 public:
  // constructors
  FlexGRCMap(frDesign* designIn, RouterConfiguration* router_cfg)
      : design_(designIn), router_cfg_(router_cfg)
  {
    auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
    numLayers_ = design_->getTech()->getLayers().size();
    xgp_ = &(gCellPatterns.at(0));
    ygp_ = &(gCellPatterns.at(1));
  }
  FlexGRCMap(FlexGRCMap* in, RouterConfiguration* router_cfg)
      : design_(in->design_),
        xgp_(in->xgp_),
        ygp_(in->ygp_),
        numLayers_(in->numLayers_),
        bits_(in->bits_),
        zMap_(in->zMap_),
        layerTrackPitches_(in->layerTrackPitches_),
        layerLine2ViaPitches_(in->layerLine2ViaPitches_),
        layerPitches_(in->layerPitches_),
        router_cfg_(router_cfg)
  {
  }
  // getters
  frDesign* getDesign() const { return design_; }

  frLayerNum getNumLayers() { return numLayers_; }

  std::map<frLayerNum, odb::dbTechLayerDir> getZMap() { return zMap_; }

  unsigned getSupply(unsigned x,
                     unsigned y,
                     unsigned z,
                     frDirEnum dir,
                     bool isRaw = false) const
  {
    unsigned supply = 0;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          supply = getBits(idx, 24, CMAPSUPPLYSIZE);
          break;
        case frDirEnum::N:
          supply = getBits(idx, 16, CMAPSUPPLYSIZE);
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
    if (isRaw) {
      return supply << CMAPFRACSIZE;
    }
    return supply;
  }

  unsigned getRawSupply(unsigned x, unsigned y, unsigned z, frDirEnum dir) const
  {
    return getSupply(x, y, z, dir, true);
  }

  unsigned getSupply2D(unsigned x,
                       unsigned y,
                       frDirEnum dir,
                       bool isRaw = false) const
  {
    unsigned supply = 0;
    for (unsigned z = 0; z < zMap_.size(); z++) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        switch (dir) {
          case frDirEnum::E:
            supply += getBits(idx, 24, CMAPSUPPLYSIZE);
            break;
          case frDirEnum::N:
            supply += getBits(idx, 16, CMAPSUPPLYSIZE);
            break;
          case frDirEnum::U:;
            break;
          default:;
        }
      }
    }
    if (isRaw) {
      return supply << CMAPFRACSIZE;
    }
    return supply;
  }

  unsigned getRawSupply2D(unsigned x, unsigned y, frDirEnum dir) const
  {
    return getSupply2D(x, y, dir, true);
  }

  unsigned getDemand(unsigned x,
                     unsigned y,
                     unsigned z,
                     frDirEnum dir,
                     bool isRaw = false) const
  {
    unsigned demand = 0;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          demand = getBits(idx, 48, CMAPDEMANDSIZE);
          break;
        case frDirEnum::N:
          demand = getBits(idx, 32, CMAPDEMANDSIZE);
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
    if (isRaw) {
      return demand;
    }
    return demand >> CMAPFRACSIZE;
  }

  unsigned getRawDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir) const
  {
    return getDemand(x, y, z, dir, true);
  }

  unsigned getDemand2D(unsigned x,
                       unsigned y,
                       frDirEnum dir,
                       bool isRaw = false) const
  {
    unsigned demand = 0;
    for (unsigned z = 0; z < zMap_.size(); z++) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        switch (dir) {
          case frDirEnum::E:
            demand += getBits(idx, 48, CMAPDEMANDSIZE);
            break;
          case frDirEnum::N:
            demand += getBits(idx, 32, CMAPDEMANDSIZE);
            break;
          case frDirEnum::U:;
            break;
          default:;
        }
      }
    }
    if (isRaw) {
      return demand;
    }
    return demand >> CMAPFRACSIZE;
  }

  unsigned getRawDemand2D(unsigned x, unsigned y, frDirEnum dir) const
  {
    return getDemand2D(x, y, dir, true);
  }

  unsigned getHistoryCost(unsigned x, unsigned y, unsigned z) const
  {
    unsigned hist = 0;
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      hist = getBits(idx, 8, CMAPHISTSIZE);
    }
    return hist;
  }

  bool hasBlock(unsigned x, unsigned y, unsigned z, frDirEnum dir) const
  {
    bool sol = false;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          sol = getBit(idx, 3);
          break;
        case frDirEnum::N:
          sol = getBit(idx, 2);
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
    return sol;
  }

  bool hasOverflow(unsigned x, unsigned y, unsigned z, frDirEnum dir) const
  {
    bool sol = false;
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          sol = getBit(idx, 1);
          break;
        case frDirEnum::N:
          sol = getBit(idx, 0);
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
    return sol;
  }

  // setters
  void setSupply(unsigned x,
                 unsigned y,
                 unsigned z,
                 frDirEnum dir,
                 unsigned supply)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          setBits(idx, 24, CMAPSUPPLYSIZE, supply);
          break;
        case frDirEnum::N:
          setBits(idx, 16, CMAPSUPPLYSIZE, supply);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void addDemand(unsigned x,
                 unsigned y,
                 unsigned z,
                 frDirEnum dir,
                 unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          addToBits(
              idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
          break;
        case frDirEnum::N:
          addToBits(
              idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void addRawDemand(unsigned x,
                    unsigned y,
                    unsigned z,
                    frDirEnum dir,
                    unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          addToBits(idx, 48, CMAPDEMANDSIZE, delta);
          break;
        case frDirEnum::N:
          addToBits(idx, 32, CMAPDEMANDSIZE, delta);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void subDemand(unsigned x,
                 unsigned y,
                 unsigned z,
                 frDirEnum dir,
                 unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          subToBits(
              idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
          break;
        case frDirEnum::N:
          subToBits(
              idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void subRawDemand(unsigned x,
                    unsigned y,
                    unsigned z,
                    frDirEnum dir,
                    unsigned delta = 1)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          subToBits(idx, 48, CMAPDEMANDSIZE, delta);
          break;
        case frDirEnum::N:
          subToBits(idx, 32, CMAPDEMANDSIZE, delta);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void setDemand(unsigned x,
                 unsigned y,
                 unsigned z,
                 frDirEnum dir,
                 unsigned demand)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          setBits(
              idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, demand);
          break;
        case frDirEnum::N:
          setBits(
              idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, demand);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void setRawDemand(unsigned x,
                    unsigned y,
                    unsigned z,
                    frDirEnum dir,
                    unsigned demand)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          setBits(idx, 48, CMAPDEMANDSIZE, demand);
          break;
        case frDirEnum::N:
          setBits(idx, 32, CMAPDEMANDSIZE, demand);
          break;
        case frDirEnum::U:
          break;
        default:;
      }
    }
  }

  void setHistoryCost(unsigned x, unsigned y, unsigned z, unsigned hist)
  {
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      setBits(idx, 8, CMAPHISTSIZE, hist);
    }
  }

  void setBlock(unsigned x, unsigned y, unsigned z, frDirEnum dir, bool block)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          if (block) {
            setBit(idx, 3);
          } else {
            resetBit(idx, 3);
          }
          break;
        case frDirEnum::N:
          if (block) {
            setBit(idx, 2);
          } else {
            resetBit(idx, 2);
          }
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
  }

  void setOverflow(unsigned x,
                   unsigned y,
                   unsigned z,
                   frDirEnum dir,
                   bool overflow)
  {
    correct(x, y, z, dir);
    if (isValid(x, y, z)) {
      auto idx = getIdx(x, y, z);
      switch (dir) {
        case frDirEnum::E:
          if (overflow) {
            setBit(idx, 1);
          } else {
            resetBit(idx, 1);
          }
          break;
        case frDirEnum::N:
          if (overflow) {
            setBit(idx, 0);
          } else {
            resetBit(idx, 0);
          }
          break;
        case frDirEnum::U:;
          break;
        default:;
      }
    }
  }

  void setLayerTrackPitches(const std::vector<frCoord>& in)
  {
    layerTrackPitches_ = in;
  }
  void setLayerLine2ViaPitches(const std::vector<frCoord>& in)
  {
    layerLine2ViaPitches_ = in;
  }
  void setLayerPitches(const std::vector<frCoord>& in) { layerPitches_ = in; }

  // other functions
  void init();
  void initFrom3D(FlexGRCMap* cmap3D);

  void print(bool isAll = false);
  void print2D(bool isAll = false);
  void printLayers();

  void cleanup()
  {
    bits_.clear();
    bits_.shrink_to_fit();
  }

 private:
  frDesign* design_;
  const frGCellPattern* xgp_;
  const frGCellPattern* ygp_;
  frLayerNum numLayers_;

  // E == horizontal, N == vertical
  // [63-48] demand E; [47-32] demand N // last bit is fractional
  // [31-24] supply E; [23-16] supply N
  // [15-8] cong history
  // [3] block E [2] block N [1] overflow E; [0] overflow N
  std::vector<uint64_t> bits_;
  std::map<frLayerNum, odb::dbTechLayerDir> zMap_;
  std::vector<frCoord> layerTrackPitches_;
  std::vector<frCoord> layerLine2ViaPitches_;
  std::vector<frCoord> layerPitches_;

  RouterConfiguration* router_cfg_;

  // internal getters
  bool getBit(unsigned idx, unsigned pos) const
  {
    return (bits_[idx] >> pos) & 1;
  }
  unsigned getBits(unsigned idx, unsigned pos, unsigned length) const
  {
    auto tmp = bits_[idx] & (((1ull << length) - 1) << pos);
    return tmp >> pos;
  }
  unsigned getIdx(unsigned xIdx, unsigned yIdx, unsigned zIdx) const
  {
    return (xIdx + yIdx * xgp_->getCount()
            + zIdx * xgp_->getCount() * ygp_->getCount());
  }

  // internal setters
  void setBit(unsigned idx, unsigned pos) { bits_[idx] |= 1 << pos; }
  void resetBit(unsigned idx, unsigned pos) { bits_[idx] &= ~(1 << pos); }
  void addToBits(unsigned idx, unsigned pos, unsigned length, unsigned val)
  {
    auto tmp = getBits(idx, pos, length) + val;
    tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
    setBits(idx, pos, length, tmp);
  }
  void subToBits(unsigned idx, unsigned pos, unsigned length, unsigned val)
  {
    auto tmp = (int) getBits(idx, pos, length) - (int) val;
    tmp = (tmp < 0) ? 0 : tmp;
    setBits(idx, pos, length, tmp);
  }
  void setBits(unsigned idx, unsigned pos, unsigned length, unsigned val)
  {
    bits_[idx] &= ~(((1ull << length) - 1) << pos);  // clear related bits to 0
    bits_[idx] |= ((uint64_t) val & ((1ull << length) - 1))
                  << pos;  // only get last length bits of val
  }

  // internal utility
  void correct(unsigned& x, unsigned& y, unsigned& z, frDirEnum& dir) const
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

  void reverse(unsigned& x, unsigned& y, unsigned& z, frDirEnum& dir) const
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

  bool isValid(unsigned x, unsigned y, unsigned z) const
  {
    if (x < 0 || y < 0 || z < 0 || x >= xgp_->getCount()
        || y >= ygp_->getCount()
        || z >= design_->getTech()->getLayers().size()) {
      return false;
    }
    return true;
  }

  bool isValid(unsigned x, unsigned y, unsigned z, frDirEnum dir) const
  {
    auto sol = isValid(x, y, z);
    reverse(x, y, z, dir);
    return (sol && isValid(x, y, z));
  }

  // utility
  unsigned getNumTracks(const std::vector<std::unique_ptr<frTrackPattern>>& tps,
                        bool isHorz,
                        frCoord low,
                        frCoord high,
                        frCoord line2ViaPitch = 0);
  unsigned getNumBlkTracks(
      bool isHorz,
      frLayerNum lNum,
      const std::set<frCoord>& trackLocs,
      const std::vector<rq_box_value_t<frBlockObject*>>& results,
      frCoord bloatDist);
  void getTrackLocs(const std::vector<std::unique_ptr<frTrackPattern>>& tps,
                    bool isHorz,
                    frCoord low,
                    frCoord high,
                    std::set<frCoord>& trackLocs);
  unsigned getNumPins(
      const std::vector<rq_box_value_t<frBlockObject*>>& results);
  frCoord calcBloatDist(frBlockObject* obj,
                        frLayerNum lNum,
                        const odb::Rect& box,
                        bool isOBS);
};
}  // namespace drt
