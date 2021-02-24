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
    taPathSeg(): taShape(), begin_(), end_(), layer_(0), style_(), owner_(nullptr) {}
    taPathSeg(const taPathSeg &in): begin_(in.begin_), end_(in.end_), layer_(in.layer_), style_(in.style_), owner_(in.owner_) {}
    taPathSeg(const frPathSeg &in);
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
    // others
    frBlockObjectEnum typeId() const override {
      return tacPathSeg;
    }

    /* from frShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer_ = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer_;
    }
    
    /* from frPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (owner_) && (owner_->typeId() == tacPin);
    }
    
    taPin* getPin() const override {
      return reinterpret_cast<taPin*>(owner_);
    }
    
    void addToPin(taPin* in) override {
      owner_ = reinterpret_cast<frBlockObject*>(in);
    }
    
    void removeFromPin() override {
      owner_ = nullptr;
    }

    /* from frConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (owner_) && (owner_->typeId() == frcNet);
    }
    
    void addToNet(frNet* in) override {
      owner_ = reinterpret_cast<frBlockObject*>(in);
    }
    
    void removeFromNet() override {
      owner_ = nullptr;
    }

    /* from frFig
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
    void move(const frTransform &xform) override {
      begin_.transform(xform);
      end_.transform(xform);
    }
    bool overlaps(const frBox &box) const override {
      return false;
    }
  protected:
    frPoint        begin_; // begin always smaller than end, assumed
    frPoint        end_;
    frLayerNum     layer_;
    frSegStyle     style_;
    frBlockObject* owner_;
  };
}

#endif
