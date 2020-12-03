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

#ifndef _DR_ACCESS_PATTERN_H
#define _DR_ACCESS_PATTERN_H

#include "db/drObj/drBlockObject.h"
#include "dr/FlexMazeTypes.h"

namespace fr {
  class drPin;
  class frViaDef;
  class drAccessPattern: public drBlockObject {
  public:
    drAccessPattern(): beginPoint(), beginLayerNum(0), beginArea(0), pin(nullptr), validAccess(std::vector<bool>(6, true)),
                       vU(nullptr), vD(nullptr), vUIdx(0), vDIdx(0), onTrackX(true), onTrackY(true), pinCost(0) {}
    // getters
    void getPoint(frPoint &bpIn) const {
      bpIn.set(beginPoint);
    }
    frLayerNum getBeginLayerNum() const {
      return beginLayerNum;
    }
    frCoord getBeginArea() const {
      return beginArea;
    }
    drPin* getPin() const {
      return pin;
    }
    bool hasMazeIdx() const {
      return (!mazeIdx.empty());
    }
    void getMazeIdx(FlexMazeIdx &in) const {
      in.set(mazeIdx);
    }
    bool hasValidAccess(const frDirEnum &dir) {
      switch (dir) {
        case (frDirEnum::E):
          return validAccess[0];
          break;
        case (frDirEnum::S):
          return validAccess[1];
          break;
        case (frDirEnum::W):
          return validAccess[2];
          break;
        case (frDirEnum::N):
          return validAccess[3];
          break;
        case (frDirEnum::U):
          return validAccess[4];
          break;
        case (frDirEnum::D):
          return validAccess[5];
          break;
        default:
          return false;
      }
    }
    bool hasAccessViaDef(const frDirEnum &dir = frDirEnum::U) {
      if (dir ==  frDirEnum::U) {
        return !(vU == nullptr);
      } else {
        return !(vD == nullptr);
      }
    }
    frViaDef* getAccessViaDef(const frDirEnum &dir = frDirEnum::U) {
      if (dir ==  frDirEnum::U) {
        return (*vU)[vUIdx];
      } else {
        return (*vD)[vDIdx];
      }
    }
    bool nextAccessViaDef(const frDirEnum &dir = frDirEnum::U) {
      bool sol = true;
      if (dir ==  frDirEnum::U) {
        if ((*vU).size() == 1) {
          sol = false;
        } else {
          ++vUIdx;
          if (vUIdx >= (int)(*vU).size()) {
            vUIdx -= (int)(*vU).size();
          }
        }
      } else {
        if ((*vD).size() == 1) {
          sol = false;
        } else {
          ++vDIdx;
          if (vDIdx >= (int)(*vD).size()) {
            vDIdx -= (int)(*vD).size();
          }
        }
      }
      return sol;
    }
    bool prevAccessViaDef(const frDirEnum &dir = frDirEnum::U) {
      bool sol = true;
      if (dir ==  frDirEnum::U) {
        if ((*vU).size() == 1) {
          sol = false;
        } else {
          --vUIdx;
          if (vUIdx < 0) {
            vUIdx += (int)(*vU).size();
          }
        }
      } else {
        if ((*vD).size() == 1) {
          sol = false;
        } else {
          --vDIdx;
          if (vDIdx < 0) {
            vDIdx += (int)(*vD).size();
          }
        }
      }
      return sol;
    }
    bool isOnTrack(bool isX) const {
      return (isX) ? onTrackX : onTrackY;
    }
    frUInt4 getPinCost() const {
      return pinCost;
    }
    // setters
    void setPoint(const frPoint &bpIn) {
      beginPoint.set(bpIn);
    }
    void setBeginLayerNum(frLayerNum in) {
      beginLayerNum = in;
    }
    void setBeginArea(frCoord in) {
      beginArea = in;
    }
    void setMazeIdx(const FlexMazeIdx &in) {
      mazeIdx.set(in);
    }
    void setPin(drPin* in) {
      pin = in;
    }
    void setValidAccess(const std::vector<bool> &in) {
      validAccess = in;
    }

    void setAccessViaDef(const frDirEnum dir, std::vector<frViaDef*>* viaDef) {
      if (dir == frDirEnum::U) {
        vU = viaDef;
      } else {
        vD = viaDef;
      }
    }
    void setOnTrack(bool in, bool isX) {
      if (isX) {
        onTrackX = in;
      } else {
        onTrackY = in;
      }
    }
    void setPinCost(frUInt4 in) {
      pinCost = in;
    }

    // others
    frBlockObjectEnum typeId() const override {
      return drcAccessPattern;
    }

  protected:
    frPoint                 beginPoint;
    frLayerNum              beginLayerNum;
    frCoord                 beginArea;
    FlexMazeIdx             mazeIdx;
    drPin*                  pin;
    std::vector<bool>       validAccess;
    std::vector<frViaDef*>* vU;
    std::vector<frViaDef*>* vD;
    int                     vUIdx;
    int                     vDIdx;
    bool                    onTrackX; // initialized in initMazeIdx_ap
    bool                    onTrackY; // initialized in initMazeIdx_ap
    frUInt4                 pinCost; // is preferred ap
  };
}




#endif
