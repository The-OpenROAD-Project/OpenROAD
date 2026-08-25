// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/geom.h"

#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/geom_boost.h"

namespace odb {

namespace {

// native boost types are used to avoid needing a mutable access
// to odb::Polygon
using BoostPolygon = boost::polygon::polygon_data<int>;
using BoostPolygonSet = boost::polygon::polygon_set_data<int>;

// Collect the shape into a polygon set
BoostPolygonSet toPolygonSet(const Polygon& polygon)
{
  using boost::polygon::operators::operator+=;

  const std::vector<Point> points = polygon.getPoints();

  BoostPolygonSet polygon_set;
  polygon_set += BoostPolygon(points.begin(), points.end());

  return polygon_set;
}

// Collect the shapes into a single polygon set, which unions overlapping
// shapes together
BoostPolygonSet toPolygonSet(const std::vector<Polygon>& polygons)
{
  using boost::polygon::operators::operator+=;

  BoostPolygonSet polygon_set;
  for (const Polygon& polygon : polygons) {
    const std::vector<Point> points = polygon.getPoints();
    polygon_set += BoostPolygon(points.begin(), points.end());
  }

  return polygon_set;
}

// Extract the polygons held by a polygon set as odb polygons
std::vector<Polygon> extractPolygons(const BoostPolygonSet& polygon_set)
{
  std::vector<BoostPolygon> output_polygons;
  polygon_set.get(output_polygons);

  std::vector<Polygon> result;
  result.reserve(output_polygons.size());
  for (const BoostPolygon& boost_polygon : output_polygons) {
    std::vector<Point> new_coord;
    new_coord.reserve(boost_polygon.coords_.size());
    for (const auto& pt : boost_polygon.coords_) {
      new_coord.emplace_back(pt.x(), pt.y());
    }
    result.emplace_back(new_coord);
  }

  return result;
}

}  // namespace

void Polygon::setPoints(const std::vector<Point>& points)
{
  points_ = points;
  boost::geometry::correct(points_);
}

Polygon Polygon::bloat(int margin) const
{
  using boost::polygon::operators::operator+;

  // bloat polygon set
  const BoostPolygonSet poly_out_set = toPolygonSet(*this) + margin;

  // extract new polygon
  return extractPolygons(poly_out_set)[0];
}

std::vector<Polygon> Polygon::merge(const std::vector<Oct>& octs)
{
  std::vector<Polygon> polys(octs.begin(), octs.end());
  return Polygon::merge(polys);
}

std::vector<Polygon> Polygon::merge(const std::vector<Rect>& rects)
{
  std::vector<Polygon> polys(rects.begin(), rects.end());
  return Polygon::merge(polys);
}

std::vector<Polygon> Polygon::merge(const std::vector<Polygon>& polys)
{
  // extract the merged polygons
  return extractPolygons(toPolygonSet(polys));
}

std::vector<Polygon> Polygon::difference(const Polygon& b) const
{
  return difference(std::vector<Polygon>{b});
}

std::vector<Polygon> Polygon::difference(const std::vector<Oct>& b) const
{
  return difference(std::vector<Polygon>(b.begin(), b.end()));
}

std::vector<Polygon> Polygon::difference(const std::vector<Rect>& b) const
{
  return difference(std::vector<Polygon>(b.begin(), b.end()));
}

std::vector<Polygon> Polygon::difference(const std::vector<Polygon>& b) const
{
  using boost::polygon::operators::operator-;

  const BoostPolygonSet difference_set = toPolygonSet(*this) - toPolygonSet(b);

  // extract new polygons
  return extractPolygons(difference_set);
}

std::vector<Polygon> Polygon::intersection(const Polygon& b) const
{
  return intersection(std::vector<Polygon>{b});
}

std::vector<Polygon> Polygon::intersection(const std::vector<Oct>& b) const
{
  return intersection(std::vector<Polygon>(b.begin(), b.end()));
}

std::vector<Polygon> Polygon::intersection(const std::vector<Rect>& b) const
{
  return intersection(std::vector<Polygon>(b.begin(), b.end()));
}

std::vector<Polygon> Polygon::intersection(const std::vector<Polygon>& b) const
{
  using boost::polygon::operators::operator&;

  const BoostPolygonSet intersection_set
      = toPolygonSet(*this) & toPolygonSet(b);

  // extract new polygons
  return extractPolygons(intersection_set);
}

}  // namespace odb
