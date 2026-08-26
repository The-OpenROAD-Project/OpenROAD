// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/geom.h"

#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/geom_boost.h"

namespace odb {

void Polygon::setPoints(const std::vector<Point>& points)
{
  points_ = points;
  boost::geometry::correct(points_);
}

Polygon Polygon::bloat(int margin) const
{
  using boost::polygon::operators::operator+;

  // bloat polygon set
  const geom::BoostPolygonSet poly_out_set = geom::toPolygonSet(*this) + margin;

  // extract new polygon
  const auto polygons = geom::extractPolygons(poly_out_set);
  if (polygons.empty()) {
    return Polygon();
  }
  return polygons[0];
}

}  // namespace odb
