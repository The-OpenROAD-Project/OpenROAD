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
}

template <>
struct gtl::geometry_concept<fr::gcSegment> { typedef segment_concept type; };
template <>
struct gtl::geometry_concept<fr::gcRect> { typedef gtl::rectangle_concept type; };
template <>
struct gtl::geometry_concept<fr::gcPolygon> { typedef polygon_90_with_holes_concept type; };

namespace fr {
  //class gcEdge;
  class gcShape: public gcPinFig {
  public:
    // setters
    virtual void setLayerNum (frLayerNum tmpLayerNum) = 0;
    // getters
    virtual frLayerNum getLayerNum() const = 0;
    // others
  protected:
    // constructors
    gcShape(): gcPinFig() {}
    gcShape(const gcShape &in): gcPinFig(in) {}
  };

  class gcCorner: public gtl::point_data<frCoord> {
  public:
    // constructors
    gcCorner(): prevCorner(nullptr), nextCorner(nullptr), prevEdge(nullptr), nextEdge(nullptr), cornerType(frCornerTypeEnum::UNKNOWN), cornerDir(frCornerDirEnum::UNKNOWN), fixed(false) {}
    gcCorner(const gcCorner &in) = default;

    // getters
    gcCorner* getPrevCorner() const {
      return prevCorner;
    }
    gcCorner* getNextCorner() const {
      return nextCorner;
    }
    gcSegment* getPrevEdge() const {
      return prevEdge;
    }
    gcSegment* getNextEdge() const {
      return nextEdge;
    }
    frCornerTypeEnum getType() const {
      return cornerType;
    }
    frCornerDirEnum getDir() const {
      return cornerDir;
    }
    bool isFixed() const {
      return fixed;
    }

    // setters
    void setPrevCorner(gcCorner* in) {
      prevCorner = in;
    }
    void setNextCorner(gcCorner* in) {
      nextCorner = in;
    }
    void setPrevEdge(gcSegment* in) {
      prevEdge = in;
    }
    void setNextEdge(gcSegment* in) {
      nextEdge = in;
    }
    void setType(frCornerTypeEnum in) {
      cornerType = in;
    }
    void setDir(frCornerDirEnum in) {
      cornerDir = in;
    }
    void setFixed(bool in) {
      fixed = in;
    }

  private:
    gcCorner* prevCorner;
    gcCorner* nextCorner;
    gcSegment* prevEdge;
    gcSegment* nextEdge;
    frCornerTypeEnum cornerType;
    frCornerDirEnum cornerDir; // points away from poly for convex and concave
    bool fixed;
  };

  class gcSegment: public gtl::segment_data<frCoord>, public gcShape {
  public:
    // constructors
    gcSegment(): gtl::segment_data<frCoord>(), gcShape(), layer(-1), pin(nullptr), net(nullptr), prev_edge(nullptr),
                                               next_edge(nullptr), lowCorner(nullptr), highCorner(nullptr), fixed(false) {}
    gcSegment(const gcSegment &in): gtl::segment_data<frCoord>(in), gcShape(in), layer(in.layer), pin(in.pin), net(in.net),
                              prev_edge(in.prev_edge), next_edge(in.next_edge),
                              lowCorner(in.lowCorner), highCorner(in.highCorner), fixed(in.fixed) {}
    // getters
    gcSegment* getPrevEdge() const {
      return prev_edge;
    }
    gcSegment* getNextEdge() const {
      return next_edge;
    }
    gcCorner* getLowCorner() const {
      return lowCorner;
    }
    gcCorner* getHighCorner() const {
      return highCorner;
    }
    bool isFixed() const {
      return fixed;
    }
    // direction always from bp to ep, not orthogonal!!
    frDirEnum getDir() const {
      frDirEnum dir = frDirEnum::UNKNOWN;
      // SOUTH / NORTH
      if (gtl::segment_data<frCoord>::low().x() == gtl::segment_data<frCoord>::high().x()) {
        if (gtl::segment_data<frCoord>::low().y() < gtl::segment_data<frCoord>::high().y()) {
          dir = frDirEnum::N;
        } else {
          dir = frDirEnum::S;
        }
      // WEST / EAST
      } else {
        if (gtl::segment_data<frCoord>::low().x() < gtl::segment_data<frCoord>::high().x()) {
          dir = frDirEnum::E;
        } else {
          dir = frDirEnum::W;
        }
      }
      return dir;
    }
    // setters
    void setSegment(const gtl::segment_data<frCoord> &in) {
      gtl::segment_data<frCoord>::operator =(in);
    }
    void setSegment(const gtl::point_data<frCoord> &bp, const gtl::point_data<frCoord> &ep) {
      gtl::segment_data<frCoord>::low(bp);
      gtl::segment_data<frCoord>::high(ep);
    }
    void setPrevEdge(gcSegment* in) {
      prev_edge = in;
    }
    void setNextEdge(gcSegment* in) {
      next_edge = in;
    }
    void setLowCorner(gcCorner* in) {
      lowCorner = in;
    }
    void setHighCorner(gcCorner* in) {
      highCorner = in;
    }
    void setFixed(bool in) {
      fixed = in;
    }
    // others
    frBlockObjectEnum typeId() const override {
      return gccEdge;
    }
    /* from gcShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
    /* from gcPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (pin);
    }
    
    gcPin* getPin() const override {
      return pin;
    }
    
    void addToPin(gcPin* in) override {
      pin = in;
    }
    
    void removeFromPin() override {
      pin = nullptr;
    }

    /* from gcConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (net);
    }
    
    gcNet* getNet() const override {
      return net;
    }
    
    void addToNet(gcNet* in) override {
      net = in;
    }
    
    void removeFromNet() override {
      net = nullptr;
    }
  protected:
    frLayerNum  layer;
    gcPin*      pin;
    gcNet*      net;
    gcSegment*  prev_edge;
    gcSegment*  next_edge;
    gcCorner*   lowCorner;
    gcCorner*   highCorner;
    bool        fixed;
  };

  class gcRect: public gtl::rectangle_data<frCoord>, public gcShape {
  public:
    // constructors
    gcRect() : gtl::rectangle_data<frCoord>(), gcShape(), layer(-1), pin(nullptr), net(nullptr), fixed(false) {}
    gcRect(const gcRect& in): gtl::rectangle_data<frCoord>(in), gcShape(in), layer(in.layer), pin(in.pin), net(in.net), fixed(in.fixed) {}
    gcRect(const gtl::rectangle_data<frCoord> &shapeIn, frLayerNum layerIn, gcPin* pinIn, gcNet* netIn, bool fixedIn):
      gtl::rectangle_data<frCoord>(shapeIn), layer(layerIn), pin(pinIn), net(netIn), fixed(fixedIn) {}
    // setters
    void setRect(const gtl::rectangle_data<frCoord> &in) {
      gtl::rectangle_data<frCoord>::operator =(in);
    }
    void setRect(frCoord xl, frCoord yl, frCoord xh, frCoord yh) {
      gtl::xl((*this), xl);
      gtl::xh((*this), xh);
      gtl::yl((*this), yl);
      gtl::yh((*this), yh);
    }
    void setRect(const frPoint &bp, const frPoint &ep) {
      gtl::xl((*this), bp.x());
      gtl::xh((*this), ep.x());
      gtl::yl((*this), bp.y());
      gtl::yh((*this), ep.y());
    }
    void setRect(const frBox &in) {
      gtl::xl((*this), in.left());
      gtl::xh((*this), in.right());
      gtl::yl((*this), in.bottom());
      gtl::yh((*this), in.top());
    }
    void setFixed(bool in) {
      fixed = in;
    }
    // getters
    frCoord length() const {
      return std::max(gtl::xh(*this) - gtl::xl(*this),
                      gtl::yh(*this) - gtl::yl(*this));
    }
    frCoord width() const {
      return std::min(gtl::xh(*this) - gtl::xl(*this),
                      gtl::yh(*this) - gtl::yl(*this));
    }
    bool isFixed() const {
      return fixed;
    }

    // others
    frBlockObjectEnum typeId() const override {
      return gccRect;
    }
    /* from gcShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
    /* from gcPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (pin);
    }
    
    gcPin* getPin() const override {
      return pin;
    }
    
    void addToPin(gcPin* in) override {
      pin = in;
    }
    
    void removeFromPin() override {
      pin = nullptr;
    }

    /* from gcConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (net);
    }
    
    gcNet* getNet() const override {
      return net;
    }
    
    void addToNet(gcNet* in) override {
      net = in;
    }
    
    void removeFromNet() override {
      net = nullptr;
    }

  protected:
    frLayerNum                   layer;
    gcPin*                       pin;
    gcNet*                       net;
    bool                         fixed;
  };

  class gcPolygon: public gtl::polygon_90_with_holes_data<frCoord>, public gcShape {
  public:
    // constructors
    gcPolygon(): gtl::polygon_90_with_holes_data<frCoord>(), gcShape(), layer(-1), pin(nullptr), net(nullptr) {}
    gcPolygon(const gcPolygon& in): gtl::polygon_90_with_holes_data<frCoord>(in), gcShape(in), layer(in.layer), pin(in.pin), net(in.net) {}
    gcPolygon(const gtl::polygon_90_with_holes_data<frCoord> &shapeIn, frLayerNum layerIn, gcPin* pinIn, gcNet* netIn):
              gtl::polygon_90_with_holes_data<frCoord>(shapeIn), layer(layerIn), pin(pinIn), net(netIn) {
    }
    // setters
    void setPolygon(const gtl::polygon_90_with_holes_data<frCoord> &in) {
      gtl::polygon_90_with_holes_data<frCoord>::operator =(in);
    }
    // ensure gtl assumption: counterclockwise for outer and clockwise for holes
    void setPolygon(const std::vector<frPoint> &in) {
      std::vector<gtl::point_data<frCoord> > points;
      gtl::point_data<frCoord> tmp;
      for (auto &pt: in) {
        tmp.x(pt.x());
        tmp.y(pt.y());
        points.push_back(tmp);
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      gtl::polygon_90_set_data<frCoord> ps;
      using namespace gtl::operators;
      ps += poly;
      std::vector<gtl::polygon_90_with_holes_data<frCoord> > polys;
      ps.get(polys);
      setPolygon(polys[0]);
    }
    // getters
    frBlockObjectEnum typeId() const override {
      return gccPolygon;
    }
    /* from gcShape
     * setLayerNum
     * getLayerNum
     */
    void setLayerNum (frLayerNum numIn) override {
      layer = numIn;
    }
    frLayerNum getLayerNum() const override {
      return layer;
    }
    
    /* from gcPinFig
     * hasPin
     * getPin
     * addToPin
     * removeFromPin
     */
    bool hasPin() const override {
      return (pin);
    }
    
    gcPin* getPin() const override {
      return pin;
    }
    
    void addToPin(gcPin* in) override {
      pin = in;
    }
    
    void removeFromPin() override {
      pin = nullptr;
    }

    /* from gcConnFig
     * hasNet
     * getNet
     * addToNet
     * removeFromNet
     */
    bool hasNet() const override {
      return (net);
    }
    
    gcNet* getNet() const override {
      return net;
    }
    
    void addToNet(gcNet* in) override {
      net = in;
    }
    
    void removeFromNet() override {
      net = nullptr;
    }
    
    // edge iterator
  protected:
    frLayerNum layer;
    gcPin*     pin;
    gcNet*     net;
  };

}

#endif
