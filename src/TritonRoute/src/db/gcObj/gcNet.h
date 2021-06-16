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

#ifndef _GC_NET_H_
#define _GC_NET_H_

#include <memory>

#include "db/gcObj/gcBlockObject.h"
#include "db/gcObj/gcPin.h"
#include "db/obj/frNet.h"

namespace fr {
class frNet;
using namespace std;
class gcNet : public gcBlockObject
{
 public:
  // constructors
  gcNet(int numLayers = 0)  // = 0 for serialization
      : gcBlockObject(),
        fixedPolygons_(numLayers),
        routePolygons_(numLayers),
        fixedRectangles_(numLayers),
        routeRectangles_(numLayers),
        pins_(numLayers),
        owner_(nullptr),
        taperedRects(numLayers),
        nonTaperedRects(numLayers)
  {
  }
  // setters
  void addPolygon(const frBox& box, frLayerNum layerNum, bool isFixed = false)
  {
    gtl::rectangle_data<frCoord> rect(
        box.left(), box.bottom(), box.right(), box.top());
    using namespace gtl::operators;
    if (isFixed) {
      fixedPolygons_[layerNum] += rect;
    } else {
      routePolygons_[layerNum] += rect;
    }
  }
  void addRectangle(const frBox& box, frLayerNum layerNum, bool isFixed = false)
  {
    gtl::rectangle_data<frCoord> rect(
        box.left(), box.bottom(), box.right(), box.top());
    if (isFixed) {
      fixedRectangles_[layerNum].push_back(rect);
    } else {
      routeRectangles_[layerNum].push_back(rect);
    }
  }
  void addPin(const gtl::polygon_90_with_holes_data<frCoord>& shape,
              frLayerNum layerNum)
  {
    auto pin = std::make_unique<gcPin>(shape, layerNum, this);
    pin->setId(pins_[layerNum].size());
    pins_[layerNum].push_back(std::move(pin));
  }
  void addPin(const gtl::rectangle_data<frCoord>& rect, frLayerNum layerNum)
  {
    gtl::polygon_90_with_holes_data<frCoord> shape;
    std::vector<frCoord> coords
        = {gtl::xl(rect), gtl::yl(rect), gtl::xh(rect), gtl::yh(rect)};
    shape.set_compact(coords.begin(), coords.end());
    auto pin = std::make_unique<gcPin>(shape, layerNum, this);
    pin->setId(pins_[layerNum].size());
    pins_[layerNum].push_back(std::move(pin));
  }
  void setOwner(frBlockObject* in) { owner_ = in; }
  void clear()
  {
    auto size = routePolygons_.size();
    routePolygons_.clear();
    routePolygons_.resize(size);
    routeRectangles_.clear();
    routeRectangles_.resize(size);
    taperedRects.clear();
    taperedRects.resize(size);
    nonTaperedRects.clear();
    nonTaperedRects.resize(size);
    specialSpacingRects.clear();
    for (auto& layerPins : pins_) {
      layerPins.clear();
    }
  }
  // getters
  const std::vector<gtl::polygon_90_set_data<frCoord>>& getPolygons(
      bool isFixed = false) const
  {
    if (isFixed) {
      return fixedPolygons_;
    } else {
      return routePolygons_;
    }
  }
  const gtl::polygon_90_set_data<frCoord>& getPolygons(frLayerNum layerNum,
                                                       bool isFixed
                                                       = false) const
  {
    if (isFixed) {
      return fixedPolygons_[layerNum];
    } else {
      return routePolygons_[layerNum];
    }
  }
  const std::vector<std::vector<gtl::rectangle_data<frCoord>>>& getRectangles(
      bool isFixed = false) const
  {
    if (isFixed) {
      return fixedRectangles_;
    } else {
      return routeRectangles_;
    }
  }
  const std::vector<gtl::rectangle_data<frCoord>>& getRectangles(
      frLayerNum layerNum,
      bool isFixed = false) const
  {
    if (isFixed) {
      return fixedRectangles_[layerNum];
    } else {
      return routeRectangles_[layerNum];
    }
  }
  const std::vector<std::vector<std::unique_ptr<gcPin>>>& getPins() const
  {
    return pins_;
  }
  const std::vector<std::unique_ptr<gcPin>>& getPins(frLayerNum layerNum) const
  {
    return pins_[layerNum];
  }
  bool hasOwner() const { return owner_; }
  frBlockObject* getOwner() const { return owner_; }
  // others
  frBlockObjectEnum typeId() const override { return gccNet; }

  frNet* getFrNet() const
  {
    if (owner_->typeId() == frcNet) {
      return static_cast<frNet*>(owner_);
    }
    return nullptr;
  }
  bool isNondefault() const
  {
    return getFrNet() && getFrNet()->getNondefaultRule();
  }
  void addTaperedRect(const frBox& bx, int zIdx)
  {
    taperedRects[zIdx].push_back(bx);
  }
  const vector<frBox>& getTaperedRects(int z) const { return taperedRects[z]; }
  void addNonTaperedRect(const frBox& bx, int zIdx)
  {
    nonTaperedRects[zIdx].push_back(bx);
  }
  const vector<frBox>& getNonTaperedRects(int z) const
  {
    return nonTaperedRects[z];
  }
  void addSpecialSpcRect(const frBox& bx,
                         frLayerNum lNum,
                         gcPin* pin,
                         gcNet* net)
  {
    unique_ptr<gcRect> sp = make_unique<gcRect>();
    sp->setLayerNum(lNum);
    sp->addToNet(net);
    sp->addToPin(pin);
    sp->setRect(bx);
    specialSpacingRects.push_back(std::move(sp));
  }
  const vector<unique_ptr<gcRect>>& getSpecialSpcRects() const
  {
    return specialSpacingRects;
  }
  bool hasPolyCornerAt(frCoord x, frCoord y, frLayerNum ln) const
  {
    for (auto& pin : pins_[ln]) {
      for (auto& corners : pin->getPolygonCorners()) {
        for (auto& corner : corners) {
          if (corner->x() == x && corner->y() == y) {
            return true;
          }
        }
      }
    }
    return false;
  }

 private:
  std::vector<gtl::polygon_90_set_data<frCoord>>
      fixedPolygons_;  // only routing layer
  std::vector<gtl::polygon_90_set_data<frCoord>>
      routePolygons_;  // only routing layer
  std::vector<std::vector<gtl::rectangle_data<frCoord>>>
      fixedRectangles_;  // only cut layer
  std::vector<std::vector<gtl::rectangle_data<frCoord>>>
      routeRectangles_;  // only cut layer
  std::vector<std::vector<std::unique_ptr<gcPin>>> pins_;
  frBlockObject* owner_;
  vector<vector<frBox>> taperedRects;     //(only routing layer)
  vector<vector<frBox>> nonTaperedRects;  //(only routing layer)
  // A non-tapered rect within a tapered max rectangle still require nondefault
  // spacing. This list hold these rectangles
  vector<unique_ptr<gcRect>> specialSpacingRects;

  void init();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcBlockObject>(*this);
    (ar) & fixedPolygons_;
    (ar) & routePolygons_;
    (ar) & fixedRectangles_;
    (ar) & routeRectangles_;
    (ar) & pins_;
    (ar) & owner_;
    (ar) & taperedRects;
    (ar) & nonTaperedRects;
    (ar) & specialSpacingRects;
  }

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
