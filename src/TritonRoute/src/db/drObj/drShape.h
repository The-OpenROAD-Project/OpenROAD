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

#ifndef _DR_SHAPE_H_
#define _DR_SHAPE_H_

#include "db/drObj/drFig.h"
#include "db/infra/frSegStyle.h"
#include "dr/FlexMazeTypes.h"


namespace fr {
  class drNet;
  class drPin;
  class frPathSeg;
  class frPatchWire;
  class drShape: public drPinFig {
  public:
    // setters
    virtual void setLayerNum (frLayerNum tmpLayerNum) = 0;
    // getters
    virtual frLayerNum getLayerNum() const = 0;
    // others

    /* drom drPinFig
     * hasPin
     * getPin
     * addToPin
     * removedromPin
     */

    /* drom drConnFig
     * hasNet
     * getNet
     * addToNet
     * removedromNet
     */

    /* drom drFig
     * getBBox
     * move
     * overlaps
     */
  protected:
    // constructors
    drShape(): drPinFig() {}
    drShape(const drShape &in): drPinFig(in) {}
  };

  class drPathSeg: public drShape {
  public:
    // constructors
    drPathSeg(): drShape(), begin(), end(), layer(0), style(), owner(nullptr), 
                 beginMazeIdx(), endMazeIdx(), patchSeg(false) {}
    drPathSeg(const drPathSeg &in): drShape(in), begin(in.begin), end(in.end), layer(in.layer), style(in.style), owner(in.owner), 
                                    beginMazeIdx(in.beginMazeIdx), endMazeIdx(in.endMazeIdx), patchSeg(in.patchSeg) {}
    drPathSeg(const frPathSeg &in);
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
      return drcPathSeg;
    }

    /* from drShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
    /* from drPinFig
     * hasPin
     * getPin
     * addToPin
     * removedromPin
     */
    bool hasPin() const override {
      return (owner) && (owner->typeId() == drcPin);
    }
    
    drPin* getPin() const override {
      return reinterpret_cast<drPin*>(owner);
    }
    
    void addToPin(drPin* in) override {
      owner = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner = nullptr;
    }

    /* from drConnFig
     * hasNet
     * getNet
     * addToNet
     * removedromNet
     */
    bool hasNet() const override {
      return (owner) && (owner->typeId() == drcNet);
    }
    
    drNet* getNet() const override {
      return reinterpret_cast<drNet*>(owner);
    }
    
    void addToNet(drNet* in) override {
      owner = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner = nullptr;
    }

    /* from drFig
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

    bool hasMazeIdx() const {
      return (!beginMazeIdx.empty());
    }
    void getMazeIdx(FlexMazeIdx &bi, FlexMazeIdx &ei) const {
      bi.set(beginMazeIdx);
      ei.set(endMazeIdx);
    }
    void setMazeIdx(FlexMazeIdx &bi, FlexMazeIdx &ei) {
      beginMazeIdx.set(bi);
      endMazeIdx.set(ei);
    }
    void setPatchSeg(bool in) {
      patchSeg = in;
    }
    bool isPatchSeg() const {
      return patchSeg;
    }
  protected:
    frPoint        begin; // begin always smaller than end, assumed
    frPoint        end;
    frLayerNum     layer;
    frSegStyle     style;
    drBlockObject* owner;
    FlexMazeIdx    beginMazeIdx;
    FlexMazeIdx    endMazeIdx;
    bool           patchSeg; 
  };

  class drPatchWire: public drShape {
  public:
    // constructors
    drPatchWire(): drShape(), offsetBox(), origin(), layer(0), owner(nullptr) {};
    drPatchWire(const drPatchWire& in): drShape(in), offsetBox(in.offsetBox), origin(in.origin), layer(in.layer), owner(in.owner) {};
    drPatchWire(const frPatchWire& in);
    // others
    frBlockObjectEnum typeId() const override {
      return drcPatchWire;
    }

    /* from drShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }


    /* from drPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (owner) && (owner->typeId() == drcPin);
    }
    
    drPin* getPin() const override {
      return reinterpret_cast<drPin*>(owner);
    }
    
    void addToPin(drPin* in) override {
      owner = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner = nullptr;
    }

    /* from drConnfig
     * hasNet
     * getNet
     * addToNet
     * removedFromNet
     */
    bool hasNet() const override {
      return (owner) && (owner->typeId() == drcNet);
    }
    
    drNet* getNet() const override {
      return reinterpret_cast<drNet*>(owner);
    }
    
    void addToNet(drNet* in) override {
      owner = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner = nullptr;
    }

    /* from drFig
     * getBBox
     * setBBox
     */
    void getBBox (frBox &boxIn) const override {
      frTransform xform(origin);
      boxIn.set(offsetBox);
      boxIn.transform(xform);
    }

    void getOffsetBox (frBox &boxIn) const {
      boxIn = offsetBox;
    }
    void setOffsetBox (const frBox &boxIn) {
      offsetBox.set(boxIn);
    }

    void getOrigin(frPoint &in) const {
      in.set(origin);
    }
    void setOrigin(const frPoint &in) {
      origin.set(in);
    }

  protected:
    frBox           offsetBox;
    frPoint         origin;
    frLayerNum      layer;
    drBlockObject*  owner;
  };
}

#endif
