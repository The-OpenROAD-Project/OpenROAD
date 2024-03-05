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

#include "db/obj/frFig.h"
#include "frBaseTypes.h"

namespace drt {
class frNet;
class frGuide : public frConnFig
{
 public:
  frGuide() = default;
  frGuide(const frGuide& in) = delete;
  // getters
  std::pair<Point, Point> getPoints() const { return {begin_, end_}; }

  const Point& getBeginPoint() const { return begin_; }
  const Point& getEndPoint() const { return end_; }

  frLayerNum getBeginLayerNum() const { return beginLayer_; }
  frLayerNum getEndLayerNum() const { return endLayer_; }
  bool hasRoutes() const { return routeObj_.empty() ? false : true; }
  const std::vector<std::unique_ptr<frConnFig>>& getRoutes() const
  {
    return routeObj_;
  }
  int getIndexInOwner() const { return index_in_owner_; }
  // setters
  void setPoints(const Point& beginIn, const Point& endIn)
  {
    begin_ = beginIn;
    end_ = endIn;
  }
  void setBeginLayerNum(frLayerNum numIn) { beginLayer_ = numIn; }
  void setEndLayerNum(frLayerNum numIn) { endLayer_ = numIn; }
  void addRoute(std::unique_ptr<frConnFig> cfgIn)
  {
    routeObj_.push_back(std::move(cfgIn));
  }
  void setRoutes(std::vector<std::unique_ptr<frConnFig>>& routesIn)
  {
    routeObj_ = std::move(routesIn);
  }
  // others
  frBlockObjectEnum typeId() const override { return frcGuide; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */
  bool hasNet() const override { return (net_); }
  frNet* getNet() const override { return net_; }
  void addToNet(frNet* in) override { net_ = in; }
  void removeFromNet() override { net_ = nullptr; }

  /* from frFig
   * getBBox
   * move, in .cpp
   * intersects, incomplete
   */
  // needs to be updated
  Rect getBBox() const override { return Rect(begin_, end_); }
  void move(const dbTransform& xform) override { ; }
  bool intersects(const Rect& box) const override { return false; }
  void setIndexInOwner(const int& val) { index_in_owner_ = val; }

 private:
  Point begin_;
  Point end_;
  frLayerNum beginLayer_{0};
  frLayerNum endLayer_{0};
  std::vector<std::unique_ptr<frConnFig>> routeObj_;
  frNet* net_{nullptr};
  int index_in_owner_{0};
};
}  // namespace drt
