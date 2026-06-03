// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frFig.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class frGuide : public frConnFig
{
 public:
  frGuide(const odb::Point& begin, frLayerNum layer_num, const odb::Point& end)
      : begin_(begin), end_(end), layerNum_(layer_num)
  {
  }
  frGuide(const frGuide& in) = delete;
  // getters
  std::pair<odb::Point, odb::Point> getPoints() const { return {begin_, end_}; }

  const odb::Point& getBeginPoint() const { return begin_; }
  const odb::Point& getEndPoint() const { return end_; }

  frLayerNum getLayerNum() const { return layerNum_; }
  bool hasRoutes() const { return !routeObj_.empty(); }
  const std::vector<std::unique_ptr<frConnFig>>& getRoutes() const
  {
    return routeObj_;
  }
  int getIndexInOwner() const { return index_in_owner_; }
  // setters
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
  odb::Rect getBBox() const override { return odb::Rect(begin_, end_); }
  void move(const odb::dbTransform& xform) override { ; }
  bool intersects(const odb::Rect& box) const override { return false; }
  void setIndexInOwner(const int& val) { index_in_owner_ = val; }

 private:
  odb::Point begin_;
  odb::Point end_;
  frLayerNum layerNum_;
  std::vector<std::unique_ptr<frConnFig>> routeObj_;
  frNet* net_{nullptr};
  int index_in_owner_{0};
};
}  // namespace drt
