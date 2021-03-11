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
    drPathSeg(): drShape(), begin_(), end_(), layer_(0), style_(), owner_(nullptr), 
                 beginMazeIdx_(), endMazeIdx_(), patchSeg_(false), isTapered_(false) {}
    drPathSeg(const drPathSeg &in): drShape(in), begin_(in.begin_), end_(in.end_), layer_(in.layer_), style_(in.style_), owner_(in.owner_), 
                                    beginMazeIdx_(in.beginMazeIdx_), endMazeIdx_(in.endMazeIdx_), patchSeg_(in.patchSeg_), isTapered_(in.isTapered_) {
    }
    drPathSeg(const frPathSeg &in);
    // getters
    void getPoints(frPoint &beginIn, frPoint &endIn) const {
      beginIn.set(begin_);
      endIn.set(end_);
    }
    
    void getStyle(frSegStyle &styleIn) const {
      styleIn.setBeginStyle(style_.getBeginStyle(), style_.getBeginExt());
      styleIn.setEndStyle(style_.getEndStyle(), style_.getEndExt());
      styleIn.setWidth(style_.getWidth());
    }
    // setters
    void setPoints(const frPoint &beginIn, const frPoint &endIn) {
      begin_.set(beginIn);
      end_.set(endIn);
    }
    void setStyle(const frSegStyle &styleIn) {
      style_.setBeginStyle(styleIn.getBeginStyle(), styleIn.getBeginExt());
      style_.setEndStyle(styleIn.getEndStyle(), styleIn.getEndExt());
      style_.setWidth(styleIn.getWidth());
    }
    frCoord getBeginX() const{
        return begin_.x();
    }
    frCoord getBeginY() const{
        return begin_.y();
    }
    frCoord getEndX() const{
        return end_.x();
    }
    frCoord getEndY() const{
        return end_.y();
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
      layer_ = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer_;
    }
    
    /* from drPinFig
     * hasPin
     * getPin
     * addToPin
     * removedromPin
     */
    bool hasPin() const override {
      return (owner_) && (owner_->typeId() == drcPin);
    }
    
    drPin* getPin() const override {
      return reinterpret_cast<drPin*>(owner_);
    }
    
    void addToPin(drPin* in) override {
      owner_ = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner_ = nullptr;
    }

    /* from drConnFig
     * hasNet
     * getNet
     * addToNet
     * removedromNet
     */
    bool hasNet() const override {
      return (owner_) && (owner_->typeId() == drcNet);
    }
    
    drNet* getNet() const override {
      return reinterpret_cast<drNet*>(owner_);
    }
    
    void addToNet(drNet* in) override {
      owner_ = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner_ = nullptr;
    }

    /* from drFig
     * getBBox
     * move, in .cpp
     * overlaps, in .cpp
     */
    // needs to be updated
    void getBBox (frBox &boxIn) const override {
      bool isHorizontal = true;
      if (begin_.x() == end_.x()) {
        isHorizontal = false;
      }
      auto width    = style_.getWidth();
      auto beginExt = style_.getBeginExt();
      auto endExt   = style_.getEndExt();
      if (isHorizontal) {
        boxIn.set(begin_.x() - beginExt, begin_.y() - width / 2,
                  end_.x()   + endExt,   end_.y()   + width / 2);
      } else {
        boxIn.set(begin_.x() - width / 2, begin_.y() - beginExt,
                  end_.x()   + width / 2, end_.y()   + endExt);
      }
    }

    bool hasMazeIdx() const {
      return (!beginMazeIdx_.empty());
    }
    void getMazeIdx(FlexMazeIdx &bi, FlexMazeIdx &ei) const {
      bi.set(beginMazeIdx_);
      ei.set(endMazeIdx_);
    }
    void setMazeIdx(FlexMazeIdx &bi, FlexMazeIdx &ei) {
      beginMazeIdx_.set(bi);
      endMazeIdx_.set(ei);
    }
    void setPatchSeg(bool in) {
      patchSeg_ = in;
    }
    bool isPatchSeg() const {
      return patchSeg_;
    }
    bool isTapered() const {
      return isTapered_;
    }
    void setTapered(bool t){
        isTapered_ = t;
    }
  protected:
    frPoint        begin_; // begin always smaller than end, assumed
    frPoint        end_;
    frLayerNum     layer_;
    frSegStyle     style_;
    drBlockObject* owner_;
    FlexMazeIdx    beginMazeIdx_;
    FlexMazeIdx    endMazeIdx_;
    bool           patchSeg_;
    bool           isTapered_;
  };

  class drPatchWire: public drShape {
  public:
    // constructors
    drPatchWire(): drShape(), offsetBox_(), origin_(), layer_(0), owner_(nullptr) {};
    drPatchWire(const drPatchWire& in): drShape(in), offsetBox_(in.offsetBox_), origin_(in.origin_), layer_(in.layer_), owner_(in.owner_) {};
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
      layer_ = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer_;
    }


    /* from drPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (owner_) && (owner_->typeId() == drcPin);
    }
    
    drPin* getPin() const override {
      return reinterpret_cast<drPin*>(owner_);
    }
    
    void addToPin(drPin* in) override {
      owner_ = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner_ = nullptr;
    }

    /* from drConnfig
     * hasNet
     * getNet
     * addToNet
     * removedFromNet
     */
    bool hasNet() const override {
      return (owner_) && (owner_->typeId() == drcNet);
    }
    
    drNet* getNet() const override {
      return reinterpret_cast<drNet*>(owner_);
    }
    
    void addToNet(drNet* in) override {
      owner_ = reinterpret_cast<drBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner_ = nullptr;
    }

    /* from drFig
     * getBBox
     * setBBox
     */
    void getBBox (frBox &boxIn) const override {
      frTransform xform(origin_);
      boxIn.set(offsetBox_);
      boxIn.transform(xform);
    }

    void getOffsetBox (frBox &boxIn) const {
      boxIn = offsetBox_;
    }
    void setOffsetBox (const frBox &boxIn) {
      offsetBox_.set(boxIn);
    }

    void getOrigin(frPoint &in) const {
      in.set(origin_);
    }
    void setOrigin(const frPoint &in) {
      origin_.set(in);
    }

  protected:
    frBox           offsetBox_;
    frPoint         origin_;
    frLayerNum      layer_;
    drBlockObject*  owner_;
  };
}

#endif
