// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/geom.h"

#include <vector>

#include "odb/geom_boost.h"

namespace odb {

void Polygon::setPoints(const std::vector<Point>& points)
{
  points_ = points;
  boost::geometry::correct(points_);
}

Polygon Polygon::bloat(int margin) const
{
  // convert to native boost types to avoid needed a mutable access
  // to odb::Polygon
  using BoostPolygon = boost::polygon::polygon_data<int>;
  using BoostPolygonSet = boost::polygon::polygon_set_data<int>;
  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator+;

  // convert to boost polygon
  const BoostPolygon polygon_in(points_.begin(), points_.end());

  // add to polygon set
  BoostPolygonSet poly_in_set;
  poly_in_set += polygon_in;

  // bloat polygon set
  const BoostPolygonSet poly_out_set = poly_in_set + margin;

  // extract new polygon
  std::vector<BoostPolygon> output_polygons;
  poly_out_set.get(output_polygons);
  const BoostPolygon& polygon_out = output_polygons[0];

  std::vector<odb::Point> new_coord;
  new_coord.reserve(polygon_out.coords_.size());
  for (const auto& pt : polygon_out.coords_) {
    new_coord.emplace_back(pt.x(), pt.y());
  }

  return Polygon(new_coord);
}

std::vector<Polygon> Polygon::difference(Polygon b) const
{
  // convert to native boost types to avoid needed a mutable access
  // to odb::Polygon
  using BoostPolygon = boost::polygon::polygon_data<int>;
  using BoostPolygonSet = boost::polygon::polygon_set_data<int>;
  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator-;

  // convert to boost polygon
  const BoostPolygon polygon_a(points_.begin(), points_.end());
  const BoostPolygon polygon_b(b.points_.begin(), b.points_.end());

  // add to polygon set
  BoostPolygonSet poly_a_set;
  BoostPolygonSet poly_b_set;
  poly_a_set += polygon_a;
  poly_b_set += polygon_b;

  const BoostPolygonSet difference_set = poly_a_set - poly_b_set;

  // extract new polygon
  std::vector<BoostPolygon> output_polygons;
  difference_set.get(output_polygons);

  std::vector<Polygon> result;
  result.reserve(output_polygons.size());
  for (const BoostPolygon& boost_polygon : output_polygons) {
    std::vector<odb::Point> new_coord;
    new_coord.reserve(boost_polygon.coords_.size());
    for (const auto& pt : boost_polygon.coords_) {
      new_coord.emplace_back(pt.x(), pt.y());
    }
    result.emplace_back(new_coord);
  }

  return result;
}

}  // namespace odb
