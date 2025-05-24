// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/gcObj/gcShape.h"

namespace drt {
class gcNet;
class gcPin : public gcBlockObject
{
 public:
  // constructors
  gcPin(const gtl::polygon_90_with_holes_data<frCoord>& shapeIn,
        frLayerNum layerNumIn,
        gcNet* netIn)
      : polygon_(shapeIn, layerNumIn, this, netIn), net_(netIn)
  {
  }
  // setters
  void setNet(gcNet* in) { net_ = in; }
  void addPolygonEdges(std::vector<gcSegment*>& in)
  {
    polygon_edges_.push_back(in);
  }
  void addPolygonCorners(std::vector<gcCorner*>& in)
  {
    polygon_corners_.push_back(in);
  }
  void addMaxRectangle(gcRect* in) { max_rectangles_.push_back(in); }

  // getters
  gcPolygon* getPolygon() { return &polygon_; }
  const gcPolygon* getPolygon() const { return &polygon_; }
  const std::vector<std::vector<gcSegment*>>& getPolygonEdges() const
  {
    return polygon_edges_;
  }
  const std::vector<std::vector<gcCorner*>>& getPolygonCorners() const
  {
    return polygon_corners_;
  }
  const std::vector<gcRect*>& getMaxRectangles() const
  {
    return max_rectangles_;
  }

  gcNet* getNet() { return net_; }
  // others
  frBlockObjectEnum typeId() const override { return gccPin; }

 private:
  gcPolygon polygon_;
  gcNet* net_{nullptr};
  // assisting structures
  std::vector<std::vector<gcSegment*>> polygon_edges_;
  std::vector<std::vector<gcCorner*>> polygon_corners_;
  std::vector<gcRect*> max_rectangles_;
};
}  // namespace drt
