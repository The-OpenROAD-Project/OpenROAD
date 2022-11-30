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
class drShape : public drPinFig
{
 public:
  // setters
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
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
  drShape() : drPinFig() {}
  drShape(const drShape& in) : drPinFig(in) {}

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drPinFig>(*this);
  }

  friend class boost::serialization::access;
};

class drPathSeg : public drShape
{
 public:
  // constructors
  drPathSeg()
      : drShape(),
        begin_(),
        end_(),
        layer_(0),
        style_(),
        owner_(nullptr),
        beginMazeIdx_(),
        endMazeIdx_(),
        patchSeg_(false),
        isTapered_(false)
  {
  }
  drPathSeg(const drPathSeg& in)
      : drShape(in),
        begin_(in.begin_),
        end_(in.end_),
        layer_(in.layer_),
        style_(in.style_),
        owner_(in.owner_),
        beginMazeIdx_(in.beginMazeIdx_),
        endMazeIdx_(in.endMazeIdx_),
        patchSeg_(in.patchSeg_),
        isTapered_(in.isTapered_)
  {
  }
  drPathSeg(const frPathSeg& in);
  // getters
  std::pair<Point, Point> getPoints() const { return {begin_, end_}; }

  frCoord length() const
  {
    // assuming it is always orthogonal
    return end_.x() - begin_.x() + end_.y() - begin_.y();
  }
  const Point& getBeginPoint() const { return begin_; }

  const Point& getEndPoint() const { return end_; }

  bool isVertical() const { return begin_.x() == end_.x(); }

  frCoord low() const
  {
    if (isVertical())
      return begin_.y();
    return begin_.x();
  }
  frCoord high() const
  {
    if (isVertical())
      return end_.y();
    return end_.x();
  }
  frSegStyle getStyle() const { return style_; }
  // setters
  void setPoints(const Point& beginIn, const Point& endIn)
  {
    begin_ = beginIn;
    end_ = endIn;
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
  frCoord getBeginX() const { return begin_.x(); }
  frCoord getBeginY() const { return begin_.y(); }
  frCoord getEndX() const { return end_.x(); }
  frCoord getEndY() const { return end_.y(); }
  // others
  frBlockObjectEnum typeId() const override { return drcPathSeg; }

  /* from drShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from drPinFig
   * hasPin
   * getPin
   * addToPin
   * removedromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == drcPin);
  }

  drPin* getPin() const override { return reinterpret_cast<drPin*>(owner_); }

  void addToPin(drPin* in) override
  {
    owner_ = reinterpret_cast<drBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from drConnFig
   * hasNet
   * getNet
   * addToNet
   * removedromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == drcNet);
  }

  drNet* getNet() const override { return reinterpret_cast<drNet*>(owner_); }

  void addToNet(drNet* in) override
  {
    owner_ = reinterpret_cast<drBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from drFig
   * getBBox
   * move, in .cpp
   * overlaps, in .cpp
   */
  // needs to be updated
  Rect getBBox() const override
  {
    bool isHorizontal = true;
    if (begin_.x() == end_.x()) {
      isHorizontal = false;
    }
    auto width = style_.getWidth();
    auto beginExt = style_.getBeginExt();
    auto endExt = style_.getEndExt();
    if (isHorizontal) {
      return Rect(begin_.x() - beginExt,
                  begin_.y() - width / 2,
                  end_.x() + endExt,
                  end_.y() + width / 2);
    } else {
      return Rect(begin_.x() - width / 2,
                  begin_.y() - beginExt,
                  end_.x() + width / 2,
                  end_.y() + endExt);
    }
  }

  bool hasMazeIdx() const { return (!beginMazeIdx_.empty()); }
  std::pair<FlexMazeIdx, FlexMazeIdx> getMazeIdx() const
  {
    return {beginMazeIdx_, endMazeIdx_};
  }
  void setMazeIdx(FlexMazeIdx& bi, FlexMazeIdx& ei)
  {
    beginMazeIdx_.set(bi);
    endMazeIdx_.set(ei);
  }
  void setPatchSeg(bool in) { patchSeg_ = in; }
  bool isPatchSeg() const { return patchSeg_; }
  bool isTapered() const { return isTapered_; }
  void setTapered(bool t) { isTapered_ = t; }

 protected:
  Point begin_;  // begin always smaller than end, assumed
  Point end_;
  frLayerNum layer_;
  frSegStyle style_;
  drBlockObject* owner_;
  FlexMazeIdx beginMazeIdx_;
  FlexMazeIdx endMazeIdx_;
  bool patchSeg_;
  bool isTapered_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drShape>(*this);
    (ar) & begin_;
    (ar) & end_;
    (ar) & layer_;
    (ar) & style_;
    (ar) & owner_;
    (ar) & beginMazeIdx_;
    (ar) & endMazeIdx_;
    (ar) & patchSeg_;
    (ar) & isTapered_;
  }

  friend class boost::serialization::access;
};

class drPatchWire : public drShape
{
 public:
  // constructors
  drPatchWire()
      : drShape(), offsetBox_(), origin_(), layer_(0), owner_(nullptr){};
  drPatchWire(const drPatchWire& in)
      : drShape(in),
        offsetBox_(in.offsetBox_),
        origin_(in.origin_),
        layer_(in.layer_),
        owner_(in.owner_){};
  drPatchWire(const frPatchWire& in);
  // others
  frBlockObjectEnum typeId() const override { return drcPatchWire; }

  /* from drShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from drPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner_) && (owner_->typeId() == drcPin);
  }

  drPin* getPin() const override { return reinterpret_cast<drPin*>(owner_); }

  void addToPin(drPin* in) override
  {
    owner_ = reinterpret_cast<drBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from drConnfig
   * hasNet
   * getNet
   * addToNet
   * removedFromNet
   */
  bool hasNet() const override
  {
    return (owner_) && (owner_->typeId() == drcNet);
  }

  drNet* getNet() const override { return reinterpret_cast<drNet*>(owner_); }

  void addToNet(drNet* in) override
  {
    owner_ = reinterpret_cast<drBlockObject*>(in);
  }

  void removeFromNet() override { owner_ = nullptr; }

  /* from drFig
   * getBBox
   * setBBox
   */
  Rect getBBox() const override
  {
    dbTransform xform(origin_);
    Rect box = offsetBox_;
    xform.apply(box);
    return box;
  }

  Rect getOffsetBox() const { return offsetBox_; }
  void setOffsetBox(const Rect& boxIn) { offsetBox_ = boxIn; }

  Point getOrigin() const { return origin_; }
  void setOrigin(const Point& in) { origin_ = in; }

 protected:
  Rect offsetBox_;
  Point origin_;
  frLayerNum layer_;
  drBlockObject* owner_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drShape>(*this);
    (ar) & offsetBox_;
    (ar) & origin_;
    (ar) & layer_;
    (ar) & owner_;
  }

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
