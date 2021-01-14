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

#ifndef _FLEX_GR_CMAP_H_
#define _FLEX_GR_CMAP_H_

#define CMAPHISTSIZE 8
#define CMAPSUPPLYSIZE 8
#define CMAPDEMANDSIZE 16
#define CMAPFRACSIZE 1

#include "frBaseTypes.h"
#include "frDesign.h"

namespace fr {
  class frTrackPattern;
  class FlexGRCMap {
  public:
    // constructors
    FlexGRCMap (frDesign *designIn): design(designIn), bits() {
      auto &gCellPatterns = design->getTopBlock()->getGCellPatterns();
      xgp = &(gCellPatterns.at(0));
      ygp = &(gCellPatterns.at(1));
    }
    FlexGRCMap (FlexGRCMap* in): design(in->design), xgp(in->xgp), ygp(in->ygp),
                                 numLayers(in->numLayers), bits(in->bits), 
                                 zMap(in->zMap), layerTrackPitches(in->layerTrackPitches),
                                 layerLine2ViaPitches(in->layerLine2ViaPitches),
                                 layerPitches(in->layerPitches) {}
    // getters
    frDesign* getDesign() const {
      return design;
    }

    frLayerNum getNumLayers() {
      return numLayers;
    }

    std::map<frLayerNum, frPrefRoutingDirEnum> getZMap() {
      return zMap;
    }

    unsigned getSupply(unsigned x, unsigned y, unsigned z, frDirEnum dir, bool isRaw = false) const {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
      if (isRaw) {
        return supply << CMAPFRACSIZE;
      } else {
        return supply;
      }
    }

    unsigned getRawSupply(unsigned x, unsigned y, unsigned z, frDirEnum dir) const {
      return getSupply(x, y, z, dir, true);
    }

    unsigned getSupply2D(unsigned x, unsigned y, frDirEnum dir, bool isRaw = false) const {
      unsigned supply = 0;
      for (unsigned z = 0; z < zMap.size(); z++) {
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
            case frDirEnum::U:
              ;
              break;
            default:
              ;
          }
        }
      }
      if (isRaw) {
        return supply << CMAPFRACSIZE;
      } else {
        return supply;
      }
    }

    unsigned getRawSupply2D(unsigned x, unsigned y, frDirEnum dir) const {
      return getSupply2D(x, y, dir, true);
    }

    unsigned getDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, bool isRaw = false) const {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
      if (isRaw) {
        return demand;
      } else {
        return demand >> CMAPFRACSIZE;
      }
    }

    unsigned getRawDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir) const {
      return getDemand(x, y, z, dir, true);
    }

    unsigned getDemand2D(unsigned x, unsigned y, frDirEnum dir, bool isRaw = false) const {
      unsigned demand = 0;
      for (unsigned z = 0; z < zMap.size(); z++) {
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
            case frDirEnum::U:
              ;
              break;
            default:
              ;
          }
        }
      }
      if (isRaw) {
        return demand;
      } else {
        return demand >> CMAPFRACSIZE;
      }
    }

    unsigned getRawDemand2D(unsigned x, unsigned y, frDirEnum dir) const {
      return getDemand2D(x, y, dir, true);
    }

    unsigned getHistoryCost(unsigned x, unsigned y, unsigned z) const {
      unsigned hist = 0;
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        hist = getBits(idx, 8, CMAPHISTSIZE);
      }
      return hist;
    }

    bool hasBlock(unsigned x, unsigned y, unsigned z, frDirEnum dir) const {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
      return sol;
    }

    bool hasOverflow(unsigned x, unsigned y, unsigned z, frDirEnum dir) const {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
      return sol;
    }

    // setters
    void setSupply(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned supply) {
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
          default:
            ;
        }
      }
    }

    void addDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        switch (dir) {
          case frDirEnum::E:
            addToBits(idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
            break;
          case frDirEnum::N:
            addToBits(idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
            break;
          case frDirEnum::U:
            break;
          default:
            ;
        }
      }
    }

    void addRawDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1) {
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
          default:
            ;
        }
      }
    }

    void subDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        switch (dir) {
          case frDirEnum::E:
            subToBits(idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
            break;
          case frDirEnum::N:
            subToBits(idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, delta);
            break;
          case frDirEnum::U:
            break;
          default:
            ;
        }
      }
    }

    void subRawDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned delta = 1) {
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
          default:
            ;
        }
      }
    }

    void setDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned demand) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        switch (dir) {
          case frDirEnum::E:
            setBits(idx, 48 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, demand);
            break;
          case frDirEnum::N:
            setBits(idx, 32 + CMAPFRACSIZE, CMAPDEMANDSIZE - CMAPFRACSIZE, demand);
            break;
          case frDirEnum::U:
            break;
          default:
            ;
        }
      }
    }

    void setRawDemand(unsigned x, unsigned y, unsigned z, frDirEnum dir, unsigned demand) {
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
          default:
            ;
        }
      }
    }

    void setHistoryCost(unsigned x, unsigned y, unsigned z, unsigned hist) {
      if (isValid(x, y, z)) {
        auto idx = getIdx(x, y, z);
        setBits(idx, 8, CMAPHISTSIZE, hist);
      }
    }

    void setBlock(unsigned x, unsigned y, unsigned z, frDirEnum dir, bool block) {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
    }

    void setOverflow(unsigned x, unsigned y, unsigned z, frDirEnum dir, bool overflow) {
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
          case frDirEnum::U:
            ;
            break;
          default:
            ;
        }
      }
    }

    void setLayerTrackPitches(const std::vector<frCoord> &in) {
      layerTrackPitches = in;
    }
    void setLayerLine2ViaPitches(const std::vector<frCoord> &in) {
      layerLine2ViaPitches = in;
    }
    void setLayerPitches(const std::vector<frCoord> &in) {
      layerPitches = in;
    }

    // other functions
    void init();
    void initFrom3D(FlexGRCMap *cmap3D);
    
    void print(bool isAll = false);
    void print2D(bool isAll = false);
    void printLayers();

    void cleanup() {
      bits.clear();
      bits.shrink_to_fit();
    }
  protected:
    frDesign* design;
    const frGCellPattern *xgp;
    const frGCellPattern *ygp;
    frLayerNum numLayers;

    // E == horizontal, N == vertical
    // [63-48] demand E; [47-32] demand N // last bit is fractional
    // [31-24] supply E; [23-16] supply N
    // [15-8] cong history
    // [3] block E [2] block N [1] overflow E; [0] overflow N
    std::vector<unsigned long long> bits;
    std::map<frLayerNum, frPrefRoutingDirEnum> zMap;
    std::vector<frCoord> layerTrackPitches;
    std::vector<frCoord> layerLine2ViaPitches;
    std::vector<frCoord> layerPitches;

    // internal getters
    bool getBit(unsigned idx, unsigned pos) const {
      return (bits[idx] >> pos) & 1;
    }
    unsigned getBits(unsigned idx, unsigned pos, unsigned length) const {
      auto tmp = bits[idx] & (((1ull << length) - 1) << pos);
      return tmp >> pos;
    }
    unsigned getIdx(unsigned xIdx, unsigned yIdx, unsigned zIdx) const {
      return (xIdx + yIdx * xgp->getCount() + zIdx * xgp->getCount() * ygp->getCount());
    }

    // internal setters
    void setBit(unsigned idx, unsigned pos) {
      bits[idx] |= 1 << pos;
    }
    void resetBit(unsigned idx, unsigned pos) {
      bits[idx] &= ~(1 << pos);
    }
    void addToBits(unsigned idx, unsigned pos, unsigned length, unsigned val) {
      auto tmp = getBits(idx, pos, length) + val;
      tmp = (tmp > (1u << length)) ? (1u << length) : tmp;
      setBits(idx, pos, length, tmp);
    }
    void subToBits(unsigned idx, unsigned pos, unsigned length, unsigned val) {
      auto tmp = (int)getBits(idx, pos, length) - (int)val;
      tmp = (tmp < 0) ? 0 : tmp;
      setBits(idx, pos, length, tmp);
    }
    void setBits(unsigned idx, unsigned pos, unsigned length, unsigned val) {
      bits[idx] &= ~(((1ull << length) - 1) << pos); // clear related bits to 0
      bits[idx] |= ((unsigned long long)val & ((1ull << length) - 1)) << pos; // only get last length bits of val
    }

    // internal utility
    void correct(unsigned &x, unsigned &y, unsigned &z, frDirEnum &dir) const {
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
        default:
          ;
      }
      return;
    }

    void reverse(unsigned &x, unsigned &y, unsigned &z, frDirEnum &dir) const {
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
        default:
          ;
      }
      return;
    }

    bool isValid(unsigned x, unsigned y, unsigned z) const {
      if (x < 0 || y < 0 || z < 0 ||
          x >= xgp->getCount() || y >= ygp->getCount() || z >= design->getTech()->getLayers().size()) {
        return false;
      } else {
        return true;
      }
    }

    bool isValid(unsigned x, unsigned y, unsigned z, frDirEnum dir) const {
      auto sol = isValid(x, y, z);
      reverse(x, y, z, dir);
      return (sol && isValid(x, y, z));
    }


    // utility
    unsigned getNumTracks(const std::vector<std::unique_ptr<frTrackPattern> > &tps, bool isHorz, frCoord low, frCoord high, frCoord line2ViaPitch = 0);
    unsigned getNumBlkTracks(bool isHorz, frLayerNum lNum, const std::set<frCoord> &trackLocs, const std::vector<rq_box_value_t<frBlockObject*>> &results, const frCoord bloatDist);
    void getTrackLocs(const std::vector<std::unique_ptr<frTrackPattern> > &tps, bool isHorz, frCoord low, frCoord high, std::set<frCoord> &trackLocs);
    unsigned getNumPins(const std::vector<rq_box_value_t<frBlockObject*>> &results);
    frCoord calcBloatDist(frBlockObject *obj, const frLayerNum lNum, const frBox &box, bool isOBS);
  };
}

#endif
