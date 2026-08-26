// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/geom.h"

#include <vector>

#include "boost/geometry/geometry.hpp"
#include "odb/geom_boost.h"

namespace odb {

void Polygon::setPoints(const std::vector<Point>& points)
{
  points_ = points;
  boost::geometry::correct(points_);
}

}  // namespace odb
