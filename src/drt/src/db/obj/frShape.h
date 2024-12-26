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

#include "db/infra/frSegStyle.h"
#include "db/obj/frFig.h"

namespace drt {
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
   * intersects
   */

  virtual void setIter(frListIter<std::unique_ptr<frShape>>& in) = 0;
  virtual frListIter<std::unique_ptr<frShape>> getIter() const = 0;
  void setIndexInOwner(int idx) { index_in_owner_ = idx; }
  int getIndexInOwner() const { return index_in_owner_; }

  bool hasPin() const override
  {
    return (owner_)
           && ((owner_->typeId() == frcBPin) || owner_->typeId() == frcMPin);
  }

 protected:
  // constructors
  frShape() = default;
  frShape(frBlockObject* owner) : owner_(owner) {}

  frBlockObject* owner_{nullptr};  // general back pointer 0
  int index_in_owner_{0};

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};

class frRect : public frShape
{
 public:
  // constructors
  frRect() = default;
  frRect(const frRect& in) : frShape(in), box_(in.box_), layer_(in.layer_) {}
  frRect(int xl, int yl, int xh, int yh, frLayerNum lNum, frBlockObject* owner)
      : frShape(owner), box_(xl, yl, xh, yh), layer_(lNum)
  {
  }

  // setters
  void setBBox(const Rect& boxIn) { box_ = boxIn; }
  // getters
  // others
  bool isHor() const
  {
    frCoord xSpan = box_.dx();
    frCoord ySpan = box_.dy();
    return (xSpan >= ySpan) ? true : false;
  }
  frCoord width() const
  {
    frCoord xSpan = box_.dx();
    frCoord ySpan = box_.dy();
    return (xSpan > ySpan) ? ySpan : xSpan;
  }
  frCoord length() const
  {
    frCoord xSpan = box_.dx();
    frCoord ySpan = box_.dy();
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
   * getPin
   * addToPin
   * removeFromPin
   */
  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void addToPin(frBPin* in) override
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
   * intersects in .cpp
   */
  Rect getBBox() const override { return box_; }
  void move(const dbTransform& xform) override { xform.apply(box_); }
  bool intersects(const Rect& box) const override
  {
    return getBBox().intersects(box);
  }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }
  void shift(int x, int y) { box_.moveDelta(x, y); }
  void setLeft(frCoord v) { box_.set_xlo(v); }
  void setRight(frCoord v) { box_.set_xhi(v); }
  void setTop(frCoord v) { box_.set_yhi(v); }
  void setBottom(frCoord v) { box_.set_ylo(v); }

 protected:
  Rect box_;
  frLayerNum layer_{0};
  frListIter<std::unique_ptr<frShape>> iter_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frShape>(*this);
    (ar) & box_;
    (ar) & layer_;
    // iter is handled by the owner
  }

  friend class boost::serialization::access;
};

class frPatchWire : public frShape
{
 public:
  // constructors
  frPatchWire() = default;
  frPatchWire(const frPatchWire& in)
      : frShape(in),
        offsetBox_(in.offsetBox_),
        origin_(in.origin_),
        layer_(in.layer_)
  {
  }
  frPatchWire(const drPatchWire& in);
  // setters
  void setOffsetBox(const Rect& in) { offsetBox_ = in; }
  void setOrigin(const Point& in) { origin_ = in; }
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
   * getPin
   * addToPin
   * removeFromPin
   */
  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void addToPin(frBPin* in) override
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
   * intersects in .cpp
   */
  Rect getBBox() const override
  {
    dbTransform xform(origin_);
    Rect box = offsetBox_;
    xform.apply(box);
    return box;
  }
  Rect getOffsetBox() const { return offsetBox_; }
  Point getOrigin() const { return origin_; }
  void move(const dbTransform& xform) override {}
  bool intersects(const Rect& box) const override
  {
    return getBBox().intersects(box);
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
  Rect offsetBox_;
  Point origin_;
  frLayerNum layer_{0};
  frListIter<std::unique_ptr<frShape>> iter_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frShape>(*this);
    (ar) & offsetBox_;
    (ar) & origin_;
    (ar) & layer_;
    // iter is handled by the owner
  }

  friend class boost::serialization::access;
};

class frPolygon : public frShape
{
 public:
  // constructors
  frPolygon() = default;
  frPolygon(const frPolygon& in)
      : frShape(in), points_(in.points_), layer_(in.layer_)
  {
  }
  // setters
  void setPoints(const std::vector<Point>& pointsIn) { points_ = pointsIn; }
  // getters
  const std::vector<Point>& getPoints() const { return points_; }
  // others
  frBlockObjectEnum typeId() const override { return frcPolygon; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * getPin
   * addToPin
   * removeFromPin
   */
  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void addToPin(frBPin* in) override
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
   * intersects, in .cpp
   */
  Rect getBBox() const override
  {
    frCoord llx = 0;
    frCoord lly = 0;
    frCoord urx = 0;
    frCoord ury = 0;
    if (!points_.empty()) {
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
    return Rect(llx, lly, urx, ury);
  }
  void move(const dbTransform& xform) override
  {
    for (auto& point : points_) {
      xform.apply(point);
    }
  }
  bool intersects(const Rect& box) const override { return false; }

  void setIter(frListIter<std::unique_ptr<frShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<frShape>> getIter() const override
  {
    return iter_;
  }

 protected:
  std::vector<Point> points_;
  frLayerNum layer_{0};
  frListIter<std::unique_ptr<frShape>> iter_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frShape>(*this);
    (ar) & points_;
    (ar) & layer_;
    // iter is handled by the owner
  }

  friend class boost::serialization::access;
};

class frPathSeg : public frShape
{
 public:
  // constructors
  frPathSeg() = default;
  frPathSeg(const frPathSeg& in)
      : frShape(in),
        begin_(in.begin_),
        end_(in.end_),
        layer_(in.layer_),
        style_(in.style_),
        tapered_(in.tapered_)
  {
  }
  frPathSeg(const drPathSeg& in);
  frPathSeg(const taPathSeg& in);
  // getters
  std::pair<Point, Point> getPoints() const { return {begin_, end_}; }
  const Point& getBeginPoint() const { return begin_; }
  const Point& getEndPoint() const { return end_; }
  const frSegStyle& getStyle() const { return style_; }
  frEndStyle getBeginStyle() const { return style_.getBeginStyle(); }
  frEndStyle getEndStyle() const { return style_.getEndStyle(); }
  frUInt4 getEndExt() const { return style_.getEndExt(); }
  frUInt4 getBeginExt() const { return style_.getBeginExt(); }
  bool isVertical() const { return begin_.x() == end_.x(); }
  frCoord high() const { return isVertical() ? end_.y() : end_.x(); }
  frCoord low() const { return isVertical() ? begin_.y() : begin_.x(); }
  void setHigh(frCoord v)
  {
    if (isVertical()) {
      end_.setY(v);
    } else {
      end_.setX(v);
    }
  }
  void setLow(frCoord v)
  {
    if (isVertical()) {
      begin_.setY(v);
    } else {
      begin_.setX(v);
    }
  }
  bool isBeginTruncated()
  {
    return style_.getBeginStyle() == frcTruncateEndStyle;
  }
  bool isEndTruncated() { return style_.getEndStyle() == frcTruncateEndStyle; }
  // setters
  void setPoints(const Point& beginIn, const Point& endIn)
  {
    begin_ = beginIn;
    end_ = endIn;
  }
  void setPoints_safe(const Point& beginIn, const Point& endIn)
  {
    if (endIn < beginIn) {
      setPoints(endIn, beginIn);
    } else {
      setPoints(beginIn, endIn);
    }
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
  // others
  frBlockObjectEnum typeId() const override { return frcPathSeg; }

  /* from frShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from frPinFig
   * getPin
   * addToPin
   * removeFromPin
   */
  frPin* getPin() const override { return reinterpret_cast<frPin*>(owner_); }

  void addToPin(frPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void addToPin(frBPin* in) override
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
   * intersects, in .cpp
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
  bool intersects(const Rect& box) const override { return false; }

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

  bool intersectsCenterLine(const Point& pt)
  {
    return pt.x() >= begin_.x() && pt.x() <= end_.x() && pt.y() >= begin_.y()
           && pt.y() <= end_.y();
  }
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
  bool tapered_{false};
  bool is_ap_pathseg_{false};
  Point ap_loc_;
  frListIter<std::unique_ptr<frShape>> iter_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frShape>(*this);
    (ar) & begin_;
    (ar) & end_;
    (ar) & layer_;
    (ar) & style_;
    (ar) & tapered_;
    (ar) & is_ap_pathseg_;
    (ar) & ap_loc_;
    // iter is handled by the owner
  }

  friend class boost::serialization::access;
};
}  // namespace drt
