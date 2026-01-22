// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definPolygon.h"

#include <algorithm>
#include <vector>

#include "odb/geom.h"
#include "odb/poly_decomp.h"

namespace odb {

definPolygon::definPolygon(const std::vector<Point>& points) : _points(points)
{
  if (_points.size() < 4) {
    return;
  }

  if (_points[0] == _points[_points.size() - 1]) {
    _points.pop_back();
  }

  if (_points.size() < 4) {
    return;
  }

  if (!polygon_is_clockwise(_points)) {
    std::ranges::reverse(_points);
  }
}

void definPolygon::decompose(std::vector<Rect>& rects)
{
  if (_points.size() < 4) {
    return;
  }

  decompose_polygon(_points, rects);
}

}  // namespace odb
