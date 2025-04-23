// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

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
