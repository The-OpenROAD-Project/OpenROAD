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

#ifndef _GC_SHAPE_H_
#define _GC_SHAPE_H_

#include <boost/polygon/polygon.hpp>

#include "db/gcObj/gcFig.h"

namespace gtl = boost::polygon;

namespace fr {
class gcCorner;
class gcSegment;
class gcRect;
class gcPolygon;
}  // namespace fr

template <>
struct gtl::geometry_concept<fr::gcSegment>
{
  typedef segment_concept type;
};
template <>
struct gtl::geometry_concept<fr::gcRect>
{
  typedef gtl::rectangle_concept type;
};
template <>
struct gtl::geometry_concept<fr::gcPolygon>
{
  typedef polygon_90_with_holes_concept type;
};

namespace fr {
// class gcEdge;
class gcShape : public gcPinFig
{
 public:
  // setters
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
  // getters
  virtual frLayerNum getLayerNum() const = 0;
  // others
 protected:
  // constructors
  gcShape() : gcPinFig() {}
  gcShape(const gcShape& in) : gcPinFig(in) {}

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcPinFig>(*this);
  }

  friend class boost::serialization::access;
};

class gcCorner : public gtl::point_data<frCoord>
{
 public:
  // constructors
  gcCorner()
      : prevCorner_(nullptr),
        nextCorner_(nullptr),
        prevEdge_(nullptr),
        nextEdge_(nullptr),
        cornerType_(frCornerTypeEnum::UNKNOWN),
        cornerDir_(frCornerDirEnum::UNKNOWN),
        fixed_(false)
  {
  }
  gcCorner(const gcCorner& in) = default;

  // getters
  gcCorner* getPrevCorner() const { return prevCorner_; }
  gcCorner* getNextCorner() const { return nextCorner_; }
  gcSegment* getPrevEdge() const { return prevEdge_; }
  gcSegment* getNextEdge() const { return nextEdge_; }
  frCornerTypeEnum getType() const { return cornerType_; }
  frCornerDirEnum getDir() const { return cornerDir_; }
  bool isFixed() const { return fixed_; }

  // setters
  void setPrevCorner(gcCorner* in) { prevCorner_ = in; }
  void setNextCorner(gcCorner* in) { nextCorner_ = in; }
  void setPrevEdge(gcSegment* in) { prevEdge_ = in; }
  void setNextEdge(gcSegment* in) { nextEdge_ = in; }
  void setType(frCornerTypeEnum in) { cornerType_ = in; }
  void setDir(frCornerDirEnum in) { cornerDir_ = in; }
  void setFixed(bool in) { fixed_ = in; }

 private:
  gcCorner* prevCorner_;
  gcCorner* nextCorner_;
  gcSegment* prevEdge_;
  gcSegment* nextEdge_;
  frCornerTypeEnum cornerType_;
  frCornerDirEnum cornerDir_;  // points away from poly for convex and concave
  bool fixed_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gtl::point_data<frCoord>>(*this);
    (ar) & prevCorner_;
    (ar) & nextCorner_;
    (ar) & prevEdge_;
    (ar) & nextEdge_;
    (ar) & cornerType_;
    (ar) & cornerDir_;
    (ar) & fixed_;
  }

  friend class boost::serialization::access;
};

class gcSegment : public gtl::segment_data<frCoord>, public gcShape
{
 public:
  // constructors
  gcSegment()
      : gtl::segment_data<frCoord>(),
        gcShape(),
        layer_(-1),
        pin_(nullptr),
        net_(nullptr),
        prev_edge_(nullptr),
        next_edge_(nullptr),
        lowCorner_(nullptr),
        highCorner_(nullptr),
        fixed_(false)
  {
  }
  gcSegment(const gcSegment& in)
      : gtl::segment_data<frCoord>(in),
        gcShape(in),
        layer_(in.layer_),
        pin_(in.pin_),
        net_(in.net_),
        prev_edge_(in.prev_edge_),
        next_edge_(in.next_edge_),
        lowCorner_(in.lowCorner_),
        highCorner_(in.highCorner_),
        fixed_(in.fixed_)
  {
  }
  // getters
  gcSegment* getPrevEdge() const { return prev_edge_; }
  gcSegment* getNextEdge() const { return next_edge_; }
  gcCorner* getLowCorner() const { return lowCorner_; }
  gcCorner* getHighCorner() const { return highCorner_; }
  bool isFixed() const { return fixed_; }
  // direction always from bp to ep, not orthogonal!!
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

 private:
  frLayerNum layer_;
  gcPin* pin_;
  gcNet* net_;
  gcSegment* prev_edge_;
  gcSegment* next_edge_;
  gcCorner* lowCorner_;
  gcCorner* highCorner_;
  bool fixed_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gtl::segment_data<frCoord>>(*this);
    (ar) & boost::serialization::base_object<gcShape>(*this);
    (ar) & layer_;
    (ar) & pin_;
    (ar) & net_;
    (ar) & prev_edge_;
    (ar) & next_edge_;
    (ar) & lowCorner_;
    (ar) & highCorner_;
    (ar) & fixed_;
  }

  friend class boost::serialization::access;
};

class gcRect : public gtl::rectangle_data<frCoord>, public gcShape
{
 public:
  // constructors
  gcRect()
      : gtl::rectangle_data<frCoord>(),
        gcShape(),
        layer_(-1),
        pin_(nullptr),
        net_(nullptr),
        fixed_(false),
        tapered_(false)
  {
  }
  gcRect(const gcRect& in)
      : gtl::rectangle_data<frCoord>(in),
        gcShape(in),
        layer_(in.layer_),
        pin_(in.pin_),
        net_(in.net_),
        fixed_(in.fixed_),
        tapered_(in.tapered_)
  {
  }
  gcRect(const gtl::rectangle_data<frCoord>& shapeIn,
         frLayerNum layerIn,
         gcPin* pinIn,
         gcNet* netIn,
         bool fixedIn)
      : gtl::rectangle_data<frCoord>(shapeIn),
        layer_(layerIn),
        pin_(pinIn),
        net_(netIn),
        fixed_(fixedIn),
        tapered_(false)
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
  void setRect(const frPoint& bp, const frPoint& ep)
  {
    gtl::xl((*this), bp.x());
    gtl::xh((*this), ep.x());
    gtl::yl((*this), bp.y());
    gtl::yh((*this), ep.y());
  }
  void setRect(const frBox& in)
  {
    gtl::xl((*this), in.left());
    gtl::xh((*this), in.right());
    gtl::yl((*this), in.bottom());
    gtl::yh((*this), in.top());
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

  bool intersects(const frBox& bx)
  {
    return gtl::xl(*this) <= bx.right() && gtl::xh(*this) >= bx.left()
           && gtl::yl(*this) <= bx.top() && gtl::yh(*this) >= bx.bottom();
  }

 private:
  frLayerNum layer_;
  gcPin* pin_;
  gcNet* net_;
  bool fixed_;
  bool tapered_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar)
        & boost::serialization::base_object<gtl::rectangle_data<frCoord>>(
            *this);
    (ar) & boost::serialization::base_object<gcShape>(*this);
    (ar) & layer_;
    (ar) & pin_;
    (ar) & net_;
    (ar) & fixed_;
    (ar) & tapered_;
  }

  friend class boost::serialization::access;
};

class gcPolygon : public gtl::polygon_90_with_holes_data<frCoord>,
                  public gcShape
{
 public:
  // constructors
  gcPolygon()
      : gtl::polygon_90_with_holes_data<frCoord>(),
        gcShape(),
        layer_(-1),
        pin_(nullptr),
        net_(nullptr)
  {
  }
  gcPolygon(const gcPolygon& in)
      : gtl::polygon_90_with_holes_data<frCoord>(in),
        gcShape(in),
        layer_(in.layer_),
        pin_(in.pin_),
        net_(in.net_)
  {
  }
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
  void setPolygon(const std::vector<frPoint>& in)
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
    using namespace gtl::operators;
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
 private:
  frLayerNum layer_;
  gcPin* pin_;
  gcNet* net_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar)
        & boost::serialization::base_object<
            gtl::polygon_90_with_holes_data<frCoord>>(*this);
    (ar) & boost::serialization::base_object<gcShape>(*this);
    (ar) & layer_;
    (ar) & pin_;
    (ar) & net_;
  }

  friend class boost::serialization::access;
};

}  // namespace fr

#endif
