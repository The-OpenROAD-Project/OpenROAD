// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <utility>

#include "db/infra/frSegStyle.h"
#include "db/taObj/taFig.h"

namespace drt {
class frNet;
class taPin;
class frPathSeg;
class taShape : public taPinFig
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
 protected:
  // constructors
  taShape() = default;
};

class taPathSeg : public taShape
{
 public:
  // constructors
  taPathSeg() = default;
  taPathSeg(const taPathSeg& in) = delete;
  taPathSeg& operator=(const taPathSeg&) = delete;
  taPathSeg(const frPathSeg& in);
  // getters
  std::pair<Point, Point> getPoints() const { return {begin_, end_}; }
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
  // others
  frBlockObjectEnum typeId() const override { return tacPathSeg; }

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
  bool hasPin() const override { return owner_ && owner_->typeId() == tacPin; }

  taPin* getPin() const override { return reinterpret_cast<taPin*>(owner_); }

  void addToPin(taPin* in) override
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
    }
    return Rect(begin_.x() - width / 2,
                begin_.y() - beginExt,
                end_.x() + width / 2,
                end_.y() + endExt);
  }
  void move(const dbTransform& xform) override
  {
    xform.apply(begin_);
    xform.apply(end_);
  }
  bool overlaps(const Rect& box) const override { return false; }

 protected:
  Point begin_;  // begin always smaller than end, assumed
  Point end_;
  frLayerNum layer_{0};
  frSegStyle style_;
  frBlockObject* owner_{nullptr};
};

}  // namespace drt
