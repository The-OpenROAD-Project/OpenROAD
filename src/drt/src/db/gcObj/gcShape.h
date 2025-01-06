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

#include <boost/polygon/polygon.hpp>

#include "db/gcObj/gcFig.h"

namespace gtl = boost::polygon;

namespace drt {
class gcSegment;
class gcRect;
class gcPolygon;
}  // namespace drt

template <>
struct gtl::geometry_concept<drt::gcSegment>
{
  using type = segment_concept;
};
template <>
struct gtl::geometry_concept<drt::gcRect>
{
  using type = gtl::rectangle_concept;
};
template <>
struct gtl::geometry_concept<drt::gcPolygon>
{
  using type = polygon_90_with_holes_concept;
};

namespace drt {

class gcShape : public gcPinFig
{
 public:
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
  virtual frLayerNum getLayerNum() const = 0;
};

class gcCorner : public gtl::point_data<frCoord>
{
 public:
  // getters
  gcCorner* getPrevCorner() const { return prevCorner_; }
  gcCorner* getNextCorner() const { return nextCorner_; }
  gcSegment* getPrevEdge() const { return prevEdge_; }
  gcSegment* getNextEdge() const { return nextEdge_; }
  frCornerTypeEnum getType() const { return cornerType_; }
  frCornerDirEnum getDir() const { return cornerDir_; }
  bool isFixed() const { return fixed_; }
  bool hasPin() const { return pin_; }
  gcPin* getPin() const { return pin_; }
  frLayerNum getLayerNum() const { return layer_num_; }

  // setters
  void setPrevCorner(gcCorner* in) { prevCorner_ = in; }
  void setNextCorner(gcCorner* in) { nextCorner_ = in; }
  void setPrevEdge(gcSegment* in) { prevEdge_ = in; }
  void setNextEdge(gcSegment* in) { nextEdge_ = in; }
  void setType(frCornerTypeEnum in) { cornerType_ = in; }
  void setDir(frCornerDirEnum in) { cornerDir_ = in; }
  void setFixed(bool in) { fixed_ = in; }
  void addToPin(gcPin* in) { pin_ = in; }
  void removeFromPin() { pin_ = nullptr; }
  void setLayerNum(const frLayerNum in) { layer_num_ = in; }

 private:
  gcPin* pin_{nullptr};
  gcCorner* prevCorner_{nullptr};
  gcCorner* nextCorner_{nullptr};
  gcSegment* prevEdge_{nullptr};
  gcSegment* nextEdge_{nullptr};
  frCornerTypeEnum cornerType_{frCornerTypeEnum::UNKNOWN};
  // points away from poly for convex and concave
  frCornerDirEnum cornerDir_{frCornerDirEnum::UNKNOWN};
  bool fixed_{false};
  frLayerNum layer_num_{-1};
};

class gcSegment : public gtl::segment_data<frCoord>, public gcShape
{
 public:
  // getters
  gcSegment* getPrevEdge() const { return prev_edge_; }
  gcSegment* getNextEdge() const { return next_edge_; }
  gcCorner* getLowCorner() const { return lowCorner_; }
  gcCorner* getHighCorner() const { return highCorner_; }
  bool isFixed() const { return fixed_; }
  // direction always from bp to ep
  frDirEnum getDir() const
  {
    frDirEnum dir = frDirEnum::UNKNOWN;
    // SOUTH / NORTH
    if (gtl::segment_data<frCoord>::low().x()
        == gtl::segment_data<frCoord>::high().x()) {
      if (gtl::segment_data<frCoord>::low().y()
          < gtl::segment_data<frCoord>::high().y()) {
        dir = frDirEnum::N;
      } else {
        dir = frDirEnum::S;
      }
      // WEST / EAST
    } else {
      if (gtl::segment_data<frCoord>::low().x()
          < gtl::segment_data<frCoord>::high().x()) {
        dir = frDirEnum::E;
      } else {
        dir = frDirEnum::W;
      }
    }
    return dir;
  }
  // returns the direction to the inner side of the polygon
  frDirEnum getInnerDir()
  {
    switch (getDir()) {
      case frDirEnum::N:
        return frDirEnum::W;
      case frDirEnum::S:
        return frDirEnum::E;
      case frDirEnum::E:
        return frDirEnum::N;
      case frDirEnum::W:
        return frDirEnum::S;
      default:
        return frDirEnum::UNKNOWN;
    }
  }
  // returns the direction to the outer side of the polygon
  frDirEnum getOuterDir()
  {
    switch (getDir()) {
      case frDirEnum::N:
        return frDirEnum::E;
      case frDirEnum::S:
        return frDirEnum::W;
      case frDirEnum::E:
        return frDirEnum::S;
      case frDirEnum::W:
        return frDirEnum::N;
      default:
        return frDirEnum::UNKNOWN;
    }
  }
  gtl::orientation_2d getOrientation() const
  {
    const frDirEnum dir = getDir();
    return (dir == frDirEnum::W || dir == frDirEnum::E) ? gtl::HORIZONTAL
                                                        : gtl::VERTICAL;
  }
  // setters
  void setSegment(const gtl::segment_data<frCoord>& in)
  {
    gtl::segment_data<frCoord>::operator=(in);
  }
  void setSegment(const gtl::point_data<frCoord>& bp,
                  const gtl::point_data<frCoord>& ep)
  {
    gtl::segment_data<frCoord>::low(bp);
    gtl::segment_data<frCoord>::high(ep);
  }
  void setPrevEdge(gcSegment* in) { prev_edge_ = in; }
  void setNextEdge(gcSegment* in) { next_edge_ = in; }
  void setLowCorner(gcCorner* in) { lowCorner_ = in; }
  void setHighCorner(gcCorner* in) { highCorner_ = in; }
  int isVertical() { return low().x() == high().x(); }
  void setFixed(bool in) { fixed_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return gccEdge; }
  /* from gcShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from gcPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override { return pin_; }

  gcPin* getPin() const override { return pin_; }

  void addToPin(gcPin* in) override { pin_ = in; }

  void removeFromPin() override { pin_ = nullptr; }

  /* from gcConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override { return net_; }

  gcNet* getNet() const override { return net_; }

  void addToNet(gcNet* in) override { net_ = in; }

  void removeFromNet() override { net_ = nullptr; }
  int length() { return gtl::length(*this); }

 private:
  frLayerNum layer_{-1};
  gcPin* pin_{nullptr};
  gcNet* net_{nullptr};
  gcSegment* prev_edge_{nullptr};
  gcSegment* next_edge_{nullptr};
  gcCorner* lowCorner_{nullptr};
  gcCorner* highCorner_{nullptr};
  bool fixed_{false};
};

class gcRect : public gtl::rectangle_data<frCoord>, public gcShape
{
 public:
  // constructors
  gcRect() = default;
  gcRect(const gcRect& in) = default;
  gcRect(const gtl::rectangle_data<frCoord>& shapeIn,
         frLayerNum layerIn,
         gcPin* pinIn,
         gcNet* netIn,
         bool fixedIn)
      : gtl::rectangle_data<frCoord>(shapeIn),
        layer_(layerIn),
        pin_(pinIn),
        net_(netIn),
        fixed_(fixedIn)
  {
  }
  // setters
  void setRect(const gtl::rectangle_data<frCoord>& in)
  {
    gtl::rectangle_data<frCoord>::operator=(in);
  }
  void setRect(frCoord xl, frCoord yl, frCoord xh, frCoord yh)
  {
    gtl::xl((*this), xl);
    gtl::xh((*this), xh);
    gtl::yl((*this), yl);
    gtl::yh((*this), yh);
  }
  void setRect(const Point& bp, const Point& ep)
  {
    gtl::xl((*this), bp.x());
    gtl::xh((*this), ep.x());
    gtl::yl((*this), bp.y());
    gtl::yh((*this), ep.y());
  }
  void setRect(const Rect& in)
  {
    gtl::xl((*this), in.xMin());
    gtl::xh((*this), in.xMax());
    gtl::yl((*this), in.yMin());
    gtl::yh((*this), in.yMax());
  }
  void setFixed(bool in) { fixed_ = in; }
  // getters
  frCoord length() const
  {
    return std::max(gtl::xh(*this) - gtl::xl(*this),
                    gtl::yh(*this) - gtl::yl(*this));
  }
  frCoord width() const
  {
    return std::min(gtl::xh(*this) - gtl::xl(*this),
                    gtl::yh(*this) - gtl::yl(*this));
  }
  bool isFixed() const { return fixed_; }

  // others
  frBlockObjectEnum typeId() const override { return gccRect; }
  /* from gcShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from gcPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override { return pin_; }

  gcPin* getPin() const override { return pin_; }

  void addToPin(gcPin* in) override { pin_ = in; }

  void removeFromPin() override { pin_ = nullptr; }

  /* from gcConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override { return net_; }

  gcNet* getNet() const override { return net_; }

  void addToNet(gcNet* in) override { net_ = in; }

  void removeFromNet() override { net_ = nullptr; }

  bool isTapered() { return tapered_; }

  void setTapered(bool t) { tapered_ = t; }

  bool intersects(const Rect& bx)
  {
    return gtl::xl(*this) <= bx.xMax() && gtl::xh(*this) >= bx.xMin()
           && gtl::yl(*this) <= bx.yMax() && gtl::yh(*this) >= bx.yMin();
  }

 protected:
  frLayerNum layer_{-1};
  gcPin* pin_{nullptr};
  gcNet* net_{nullptr};
  bool fixed_{false};
  bool tapered_{false};
};

class gcPolygon : public gtl::polygon_90_with_holes_data<frCoord>,
                  public gcShape
{
 public:
  // constructors
  gcPolygon() = default;
  gcPolygon(const gcPolygon& in) = default;
  gcPolygon(const gtl::polygon_90_with_holes_data<frCoord>& shapeIn,
            frLayerNum layerIn,
            gcPin* pinIn,
            gcNet* netIn)
      : gtl::polygon_90_with_holes_data<frCoord>(shapeIn),
        layer_(layerIn),
        pin_(pinIn),
        net_(netIn)
  {
  }
  // setters
  void setPolygon(const gtl::polygon_90_with_holes_data<frCoord>& in)
  {
    gtl::polygon_90_with_holes_data<frCoord>::operator=(in);
  }
  // ensure gtl assumption: counterclockwise for outer and clockwise for holes
  void setPolygon(const std::vector<Point>& in)
  {
    std::vector<gtl::point_data<frCoord>> points;
    gtl::point_data<frCoord> tmp;
    for (auto& pt : in) {
      tmp.x(pt.x());
      tmp.y(pt.y());
      points.push_back(tmp);
    }
    gtl::polygon_90_data<frCoord> poly;
    poly.set(points.begin(), points.end());
    gtl::polygon_90_set_data<frCoord> ps;
    using gtl::operators::operator+=;
    ps += poly;
    std::vector<gtl::polygon_90_with_holes_data<frCoord>> polys;
    ps.get(polys);
    setPolygon(polys[0]);
  }
  // getters
  frBlockObjectEnum typeId() const override { return gccPolygon; }
  /* from gcShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from gcPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override { return pin_; }

  gcPin* getPin() const override { return pin_; }

  void addToPin(gcPin* in) override { pin_ = in; }

  void removeFromPin() override { pin_ = nullptr; }

  /* from gcConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override { return net_; }

  gcNet* getNet() const override { return net_; }

  void addToNet(gcNet* in) override { net_ = in; }

  void removeFromNet() override { net_ = nullptr; }

  // edge iterator
 protected:
  frLayerNum layer_{-1};
  gcPin* pin_{nullptr};
  gcNet* net_{nullptr};
};

}  // namespace drt
