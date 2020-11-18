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

#ifndef _TA_SHAPE_H_
#define _TA_SHAPE_H_

#include "db/taObj/taFig.h"
#include "db/infra/frSegStyle.h"


namespace fr {
  class frNet;
  class taPin;
  class frPathSeg;
  class taShape: public taPinFig {
  public:
    // setters
    virtual void setLayerNum (frLayerNum tmpLayerNum) = 0;
    // getters
    virtual frLayerNum getLayerNum() const = 0;
    // others

    /* from frPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */

    /* from frFig
     * getBBox
     * move
     * overlaps
     */
  protected:
    // constructors
    taShape(): taPinFig() {}
  };

  class taPathSeg: public taShape {
  public:
    // constructors
    taPathSeg(): taShape(), begin(), end(), layer(0), style(), owner(nullptr) {}
    taPathSeg(const taPathSeg &in): begin(in.begin), end(in.end), layer(in.layer), style(in.style), owner(in.owner) {}
    taPathSeg(const frPathSeg &in);
    // getters
    void getPoints(frPoint &beginIn, frPoint &endIn) const {
      beginIn.set(begin);
      endIn.set(end);
    }
    void getStyle(frSegStyle &styleIn) const {
      styleIn.setBeginStyle(style.getBeginStyle(), style.getBeginExt());
      styleIn.setEndStyle(style.getEndStyle(), style.getEndExt());
      styleIn.setWidth(style.getWidth());
    }
    // setters
    void setPoints(const frPoint &beginIn, const frPoint &endIn) {
      begin.set(beginIn);
      end.set(endIn);
    }
    void setStyle(const frSegStyle &styleIn) {
      style.setBeginStyle(styleIn.getBeginStyle(), styleIn.getBeginExt());
      style.setEndStyle(styleIn.getEndStyle(), styleIn.getEndExt());
      style.setWidth(styleIn.getWidth());
    }
    // others
    frBlockObjectEnum typeId() const override {
      return tacPathSeg;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
    /* from frPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (owner) && (owner->typeId() == tacPin);
    }
    
    taPin* getPin() const override {
      return reinterpret_cast<taPin*>(owner);
    }
    
    void addToPin(taPin* in) override {
      owner = reinterpret_cast<frBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner = nullptr;
    }

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (owner) && (owner->typeId() == frcNet);
    }
    
    frNet* getNet() const override {
      return reinterpret_cast<frNet*>(owner);
    }
    
    void addToNet(frNet* in) override {
      owner = reinterpret_cast<frBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner = nullptr;
    }

    /* from frFig
     * getBBox
     * move, in .cpp
     * overlaps, in .cpp
     */
    // needs to be updated
    void getBBox (frBox &boxIn) const override {
      bool isHorizontal = true;
      if (begin.x() == end.x()) {
        isHorizontal = false;
      }
      auto width    = style.getWidth();
      auto beginExt = style.getBeginExt();
      auto endExt   = style.getEndExt();
      if (isHorizontal) {
        boxIn.set(begin.x() - beginExt, begin.y() - width / 2,
                  end.x()   + endExt,   end.y()   + width / 2);
      } else {
        boxIn.set(begin.x() - width / 2, begin.y() - beginExt,
                  end.x()   + width / 2, end.y()   + endExt);
      }
    }
    void move(const frTransform &xform) override {
      begin.transform(xform);
      end.transform(xform);
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }
  protected:
    frPoint        begin; // begin always smaller than end, assumed
    frPoint        end;
    frLayerNum     layer;
    frSegStyle     style;
    frBlockObject* owner;
  };
}

#endif
