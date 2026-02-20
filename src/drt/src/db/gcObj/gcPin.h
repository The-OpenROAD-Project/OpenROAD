// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/gcObj/gcBlockObject.h"
#include "db/gcObj/gcShape.h"
#include "frBaseTypes.h"

namespace drt {
class gcNet;
class gcPin : public gcBlockObject
{
 public:
  // constructors
  gcPin() = default;
  gcPin(const gtl::polygon_90_with_holes_data<frCoord>& shapeIn,
        frLayerNum layerNumIn,
        gcNet* netIn)
      : polygon_(std::make_unique<gcPolygon>(shapeIn, layerNumIn, this, netIn)),
        net_(netIn)
  {
  }
  // setters
  void setNet(gcNet* in) { net_ = in; }
  void addPolygonEdges(std::vector<std::unique_ptr<gcSegment>>& in)
  {
    polygon_edges_.push_back(std::move(in));
  }
  void addPolygonCorners(std::vector<std::unique_ptr<gcCorner>>& in)
  {
    polygon_corners_.push_back(std::move(in));
  }
  void addMaxRectangle(std::unique_ptr<gcRect> in)
  {
    max_rectangles_.push_back(std::move(in));
  }

  // getters
  gcPolygon* getPolygon() const { return polygon_.get(); }
  const std::vector<std::vector<std::unique_ptr<gcSegment>>>& getPolygonEdges()
      const
  {
    return polygon_edges_;
  }
  const std::vector<std::vector<std::unique_ptr<gcCorner>>>& getPolygonCorners()
      const
  {
    return polygon_corners_;
  }
  const std::vector<std::unique_ptr<gcRect>>& getMaxRectangles() const
  {
    return max_rectangles_;
  }

  gcNet* getNet() { return net_; }
  // others
  frBlockObjectEnum typeId() const override { return gccPin; }

 private:
  std::unique_ptr<gcPolygon> polygon_;
  gcNet* net_{nullptr};
  // assisting structures
  std::vector<std::vector<std::unique_ptr<gcSegment>>> polygon_edges_;
  std::vector<std::vector<std::unique_ptr<gcCorner>>> polygon_corners_;
  std::vector<std::unique_ptr<gcRect>> max_rectangles_;
};
}  // namespace drt
