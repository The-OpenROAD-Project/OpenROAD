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

#ifndef _FR_ACCESS_H_
#define _FR_ACCESS_H_

#include "db/infra/frPoint.h"
#include "db/obj/frBlockObject.h"
#include <iostream>

namespace fr {
  class frViaDef;
  class frPin;
  class frPinAccess;
  class frAccessPoint: public frBlockObject {
  public:
    // constructors
    frAccessPoint(const frPoint& point, frLayerNum layerNum): frBlockObject(),
                    point(point), layerNum(layerNum), accesses(std::vector<bool>(6, false)),
                     viaDefs(), typeL(frAccessPointEnum::frcOnGridAP), 
                     typeH(frAccessPointEnum::frcOnGridAP), aps(nullptr) {}
    // getters
    void getPoint(frPoint &in) const {
      in.set(point);
    }
    const frPoint& getPoint() const {
      return point;
    }
    frLayerNum getLayerNum() const {
      return layerNum;
    }
    bool hasAccess() const {
      return (accesses[0] || accesses[1] || accesses[2] || accesses[3] || accesses[4] || accesses[5]); 
    }
    bool hasAccess(const frDirEnum &dir) const {
      switch (dir) {
        case (frDirEnum::E):
          return accesses[0];
          break;
        case (frDirEnum::S):
          return accesses[1];
          break;
        case (frDirEnum::W):
          return accesses[2];
          break;
        case (frDirEnum::N):
          return accesses[3];
          break;
        case (frDirEnum::U):
          return accesses[4];
          break;
        case (frDirEnum::D):
          return accesses[5];
          break;
        default:
          return false;
      }
    }
    const std::vector<bool>& getAccess() const {
      return accesses;
    }
    bool hasViaDef(int numCut = 1, int idx = 0) const {
      // first check numCuts
      int numCutIdx = numCut - 1;
      if (numCutIdx >= 0 && numCutIdx < (int)viaDefs.size()) {
        ;
      } else {
        return false;
      }
      // then check idx
      if (idx >= 0 && idx < (int)(viaDefs[numCutIdx].size())) {
        return true;
      } else {
        return false;
      }
    }
    // e.g., getViaDefs()     --> get all one-cut viadefs
    // e.g., getViaDefs(1)    --> get all one-cut viadefs
    // e.g., getViaDefs(2)    --> get all two-cut viadefs
    const std::vector<frViaDef*>& getViaDefs(int numCut = 1) const {
      return viaDefs[numCut - 1];
    }
    std::vector<frViaDef*>& getViaDefs(int numCut = 1) {
      return viaDefs[numCut - 1];
    }
    // e.g., getViaDef()     --> get best one-cut viadef
    // e.g., getViaDef(1)    --> get best one-cut viadef
    // e.g., getViaDef(2)    --> get best two-cut viadef
    // e.g., getViaDef(1, 1) --> get 1st alternative one-cut viadef
    frViaDef* getViaDef(int numCut = 1, int idx = 0) const {
      return viaDefs[numCut - 1][idx];
    }
    frPinAccess* getPinAccess() const {
      return aps;
    }
    frCost getCost() const {
      return (int)typeL + 4 * (int)typeH;
    }
    frAccessPointEnum getType(bool isL) const {
      if (isL) {
        return typeL;
      } else {
        return typeH;
      }
    }
    // setters
    void setPoint(const frPoint &in) {
      point.set(in);
    }
    void setAccess(const frDirEnum &dir, bool isValid = true) {
      switch (dir) {
        case (frDirEnum::E):
          accesses[0] = isValid;
          break;
        case (frDirEnum::S):
          accesses[1] = isValid;
          break;
        case (frDirEnum::W):
          accesses[2] = isValid;
          break;
        case (frDirEnum::N):
          accesses[3] = isValid;
          break;
        case (frDirEnum::U):
          accesses[4] = isValid;
          break;
        case (frDirEnum::D):
          accesses[5] = isValid;
          break;
        default:
          std::cout << "Error: unexpected direction in setValidAccess\n";
      }
    }
    void addViaDef(frViaDef *in);
    void addToPinAccess(frPinAccess* in) {
      aps = in;
    }
    void setType(frAccessPointEnum in, bool isL = true) {
      if (isL) {
        typeL = in;
      } else {
        typeH = in;
      }
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcAccessPoint;
    }
  private:
    frPoint                              point;
    frLayerNum                           layerNum;
    std::vector<bool>                    accesses; // 0 = E, 1 = S, 2 = W, 3 = N, 4 = U, 5 = D
    std::vector<std::vector<frViaDef*> > viaDefs;  // cut number -> up-via access map 
    frAccessPointEnum                    typeL;
    frAccessPointEnum                    typeH;
    frPinAccess*                         aps;
  };

  class frPinAccess: public frBlockObject {
  public:
    frPinAccess(): frBlockObject(), aps(), pin(nullptr) {}
    // getters
    const std::vector<std::unique_ptr<frAccessPoint> >& getAccessPoints() const {
      return aps;
    }
    frPin* getPin() const {
      return pin;
    }
    frAccessPoint* getAccessPoint(int idx) const {
      return aps[idx].get();
    }
    int getNumAccessPoints() const {
      return aps.size();
    }
    // setters
    void addAccessPoint(std::unique_ptr<frAccessPoint> in) {
      aps.push_back(std::move(in));
    }
    void addToPin(frPin* in) {
      pin = in;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return frcPinAccess;
    }
  private:
    std::vector<std::unique_ptr<frAccessPoint> > aps;
    frPin* pin;
  };
}

#endif
