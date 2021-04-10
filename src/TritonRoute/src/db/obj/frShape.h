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

#ifndef _FR_SHAPE_H_
#define _FR_SHAPE_H_

#include "db/infra/frSegStyle.h"
#include "db/obj/frFig.h"

namespace fr {
class frNet;
class frPin;
class drPathSeg;
class taPathSeg;
class drPatchWire;
class frShape : public frPinFig
{
 public:
  // setters
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
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

  virtual void setIter(frListIter<std::unique_ptr<frShape>>& in) = 0;
  virtual frListIter<std::unique_ptr<frShape>> getIter() const = 0;

 protected:
  // constructors
  frShape() : frPinFig() {}
};

class frRect : public frShape
{
 public:
  // constructors
  frRect() : frShape(), box_(), layer_(0), owner_(nullptr) {}
  frRect(const frRect& in)
      : frShape(in), box_(in.box_), layer_(in.layer_), owner_(in.owner_)
  {
  }
  // setters
  void setBBox(const frBox& boxIn) { box_.set(boxIn); }
  // getters
  // others
  bool isHor() const
  {
    frCoord xSpan = box_.right() - box_.left();
    frCoord ySpan = box_.top() - box_.bottom();
    return (xSpan >= ySpan) ? true : false;
  }
  frCoord width() const
  {
    frCoord xSpan = box_.right() - box_.left();
    frCoord ySpan = box_.top() - box_.bottom();
    return (xSpan > ySpan) ? ySpan : xSpan;
  }
  frCoord length() const
  {
    frCoord xSpan = box_.right() - box_.left();
    frCoord ySpan = box_.top() - box_.bottom();
    return (xSpan < ySpan) ? ySpan : xSpan;
  }
  frBlockObjectEnum typeId() const override { return frcRect; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == frcPin);
  }

  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == frcNet);
  }

  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner_); }

  void addToNet(frNet* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from frFig
   * getBBox
   * move, in .cpp
   * overlaps in .cpp
   */
  void getBBox(frBox& boxIn) const override { boxIn.set(box_); }
  void move(const frTransform& xform) override { box_.transform(xform); }
  bool overlaps(const frBox& box) const override
  {
    frBox rectBox;
    getBBox(rectBox);
    return rectBox.overlaps(box);
  }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }

 protected:
  frBox box_;
  frLayerNum layer_;
  frBlockObject* owner_;  // general back pointer 0
  frListIter<std::unique_ptr<frShape>> iter_;
};

class frPatchWire : public frShape
{
 public:
  // constructors
  frPatchWire() : frShape(), offsetBox_(), origin_(), layer_(0), owner_(nullptr)
  {
  }
  frPatchWire(const frPatchWire& in)
      : frShape(),
        offsetBox_(in.offsetBox_),
        origin_(in.origin_),
        layer_(in.layer_),
        owner_(in.owner_)
  {
  }
  frPatchWire(const drPatchWire& in);
  // setters
  void setOffsetBox(const frBox& in) { offsetBox_.set(in); }
  void setOrigin(const frPoint& in) { origin_.set(in); }
  // getters
  // others
  frBlockObjectEnum typeId() const override { return frcPatchWire; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == frcPin);
  }

  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == frcNet);
  }

  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner_); }

  void addToNet(frNet* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from frFig
   * getBBox
   * move, in .cpp
   * overlaps in .cpp
   */
  void getBBox(frBox& boxIn) const override
  {
    frTransform xform(origin_);
    boxIn.set(offsetBox_);
    boxIn.transform(xform);
  }
  void getOffsetBox(frBox& boxIn) const { boxIn.set(offsetBox_); }
  void getOrigin(frPoint& in) const { in.set(origin_); }
  void move(const frTransform& xform) override {}
  bool overlaps(const frBox& box) const override
  {
    frBox rectBox;
    getBBox(rectBox);
    return rectBox.overlaps(box);
  }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }

 protected:
  // frBox          box_;
  frBox offsetBox_;
  frPoint origin_;
  frLayerNum layer_;
  frBlockObject* owner_;  // general back pointer 0
  frListIter<std::unique_ptr<frShape>> iter_;
};

class frPolygon : public frShape
{
 public:
  // constructors
  frPolygon() : frShape(), points_(), layer_(0), owner_(nullptr) {}
  frPolygon(const frPolygon& in)
      : frShape(), points_(in.points_), layer_(in.layer_), owner_(in.owner_)
  {
  }
  // setters
  void setPoints(const std::vector<frPoint>& pointsIn) { points_ = pointsIn; }
  // getters
  const std::vector<frPoint>& getPoints() const { return points_; }
  // others
  frBlockObjectEnum typeId() const override { return frcPolygon; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == frcPin);
  }

  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == frcNet);
  }

  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner_); }

  void addToNet(frNet* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from frFig
   * getBBox
   * move, in .cpp
   * overlaps, in .cpp
   */
  void getBBox(frBox& boxIn) const override
  {
    frCoord llx = 0;
    frCoord lly = 0;
    frCoord urx = 0;
    frCoord ury = 0;
    if (points_.size()) {
      llx = points_.begin()->x();
      urx = points_.begin()->x();
      lly = points_.begin()->y();
      ury = points_.begin()->y();
    }
    for (auto& point : points_) {
      llx = (llx < point.x()) ? llx : point.x();
      lly = (lly < point.y()) ? lly : point.y();
      urx = (urx > point.x()) ? urx : point.x();
      ury = (ury > point.y()) ? ury : point.y();
    }
    boxIn.set(llx, lly, urx, ury);
  }
  void move(const frTransform& xform) override
  {
    for (auto& point : points_) {
      point.transform(xform);
    }
  }
  bool overlaps(const frBox& box) const override { return false; }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }

 protected:
  std::vector<frPoint> points_;
  frLayerNum layer_;
  frBlockObject* owner_;
  frListIter<std::unique_ptr<frShape>> iter_;
};

class frPathSeg : public frShape
{
 public:
  // constructors
  frPathSeg()
      : frShape(),
        begin_(),
        end_(),
        layer_(0),
        style_(),
        owner_(nullptr),
        tapered_(false)
  {
  }
  frPathSeg(const frPathSeg& in)
      : begin_(in.begin_),
        end_(in.end_),
        layer_(in.layer_),
        style_(in.style_),
        owner_(in.owner_),
        tapered_(in.tapered_)
  {
  }
  frPathSeg(const drPathSeg& in);
  frPathSeg(const taPathSeg& in);
  // getters
  void getPoints(frPoint& beginIn, frPoint& endIn) const
  {
    beginIn.set(begin_);
    endIn.set(end_);
  }
  void getStyle(frSegStyle& styleIn) const
  {
    styleIn.setBeginStyle(style_.getBeginStyle(), style_.getBeginExt());
    styleIn.setEndStyle(style_.getEndStyle(), style_.getEndExt());
    styleIn.setWidth(style_.getWidth());
  }
  frEndStyle getBeginStyle() const { return style_.getBeginStyle(); }
  frEndStyle getEndStyle() const { return style_.getEndStyle(); }
  frUInt4 getEndExt() const { return style_.getEndExt(); }
  frUInt4 getBeginExt() const { return style_.getBeginExt(); }
  bool isVertical() const { return begin_.x() == end_.x(); }
  frCoord high() const { return isVertical() ? end_.y() : end_.x(); }
  frCoord low() const { return isVertical() ? begin_.y() : begin_.x(); }
  // setters
  void setPoints(const frPoint& beginIn, const frPoint& endIn)
  {
    begin_.set(beginIn);
    end_.set(endIn);
  }
  void setStyle(const frSegStyle& styleIn)
  {
    style_.setBeginStyle(styleIn.getBeginStyle(), styleIn.getBeginExt());
    style_.setEndStyle(styleIn.getEndStyle(), styleIn.getEndExt());
    style_.setWidth(styleIn.getWidth());
  }
  void setBeginStyle(frEndStyle bs, frUInt4 ext = 0)
  {
    style_.setBeginStyle(bs, ext);
  }
  void setEndStyle(frEndStyle es, frUInt4 ext = 0)
  {
    style_.setEndStyle(es, ext);
  }
  // others
  frBlockObjectEnum typeId() const override { return frcPathSeg; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == frcPin);
  }

  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == frcNet);
  }

  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner_); }

  void addToNet(frNet* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from frFig
   * getBBox
   * move, in .cpp
   * overlaps, in .cpp
   */
  // needs to be updated
  void getBBox(frBox& boxIn) const override
  {
    bool isHorizontal = true;
    if (begin_.x() == end_.x()) {
      isHorizontal = false;
    }
    auto width = style_.getWidth();
    auto beginExt = style_.getBeginExt();
    auto endExt = style_.getEndExt();
    if (isHorizontal) {
      boxIn.set(begin_.x() - beginExt,
                begin_.y() - width / 2,
                end_.x() + endExt,
                end_.y() + width / 2);
    } else {
      boxIn.set(begin_.x() - width / 2,
                begin_.y() - beginExt,
                end_.x() + width / 2,
                end_.y() + endExt);
    }
  }
  void move(const frTransform& xform) override
  {
    begin_.transform(xform);
    end_.transform(xform);
  }
  bool overlaps(const frBox& box) const override { return false; }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }
  void setTapered(bool t) { tapered_ = t; }
  bool isTapered() const { return tapered_; }

 protected:
  frPoint begin_;  // begin always smaller than end, assumed
  frPoint end_;
  frLayerNum layer_;
  frSegStyle style_;
  frBlockObject* owner_;
  bool tapered_;
  frListIter<std::unique_ptr<frShape>> iter_;
};
}  // namespace fr

#endif
