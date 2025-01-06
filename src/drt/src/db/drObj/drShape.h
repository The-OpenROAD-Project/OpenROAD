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

#pragma once

#include "db/drObj/drFig.h"
#include "db/infra/frSegStyle.h"
#include "dr/FlexMazeTypes.h"

namespace drt {

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
  drPathSeg() = default;
  drPathSeg(const drPathSeg& in) = default;
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
    if (isVertical()) {
      return begin_.y();
    }
    return begin_.x();
  }
  frCoord high() const
  {
    if (isVertical()) {
      return end_.y();
    }
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
  void setBeginStyle(const frEndStyle& bs, frUInt4 ext = 0)
  {
    style_.setBeginStyle(bs, ext);
  }
  void setEndStyle(const frEndStyle& es, frUInt4 ext = 0)
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
  Rect getBBox() const override;

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
  void setApPathSeg(Point pt)
  {
    is_ap_pathseg_ = true;
    ap_loc_ = pt;
  }
  bool isApPathSeg() const { return is_ap_pathseg_; }
  Point getApLoc() const { return ap_loc_; }

 protected:
  Point begin_;  // begin always smaller than end, assumed
  Point end_;
  frLayerNum layer_{0};
  frSegStyle style_;
  drBlockObject* owner_{nullptr};
  FlexMazeIdx beginMazeIdx_;
  FlexMazeIdx endMazeIdx_;
  bool patchSeg_{false};
  bool isTapered_{false};
  bool is_ap_pathseg_{false};
  Point ap_loc_;

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
    (ar) & is_ap_pathseg_;
    (ar) & ap_loc_;
  }

  friend class boost::serialization::access;
};

class drPatchWire : public drShape
{
 public:
  // constructors
  drPatchWire() = default;
  drPatchWire(const drPatchWire& in) = default;
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
  frLayerNum layer_{0};
  drBlockObject* owner_{nullptr};

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

}  // namespace drt
