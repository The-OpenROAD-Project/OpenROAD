// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cmath>
#include <limits>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace gtl = boost::polygon;

namespace odb {

namespace {

// A polygon is rectilinear (Manhattan) when every edge is either purely
// horizontal or purely vertical.  Only such polygons can be tiled exactly by
// Boost.Polygon's polygon_90 machinery.
bool isRectilinear(const std::vector<Point>& points)
{
  const size_t n = points.size();
  for (size_t i = 0; i < n; ++i) {
    const Point& a = points[i];
    const Point& b = points[(i + 1) % n];
    if (a.x() != b.x() && a.y() != b.y()) {
      return false;
    }
  }
  return true;
}

// Decompose a single horizontal trapezoid (a polygon whose top and bottom
// edges are horizontal, with up to two sloped sides) into a staircase of
// axis-aligned rectangles.  For each unit-tall band in Y the trapezoid's X
// extent is computed from the edge crossings and rounded outward so the
// resulting rectangles fully cover (never under-cover) the trapezoid.  This
// guarantees that obstruction/pin geometry derived from a 45-degree polygon
// does not leave gaps that would later read as missing metal (shorts) for
// routing/DRC consumers.
void staircaseTrapezoid(const std::vector<gtl::point_data<int>>& v,
                        std::vector<Rect>& rects)
{
  const int n = static_cast<int>(v.size());
  if (n < 3) {
    return;
  }

  int y_lo = std::numeric_limits<int>::max();
  int y_hi = std::numeric_limits<int>::min();
  for (const auto& p : v) {
    y_lo = std::min(y_lo, gtl::y(p));
    y_hi = std::max(y_hi, gtl::y(p));
  }

  for (int y = y_lo; y < y_hi; ++y) {
    const double y_mid = y + 0.5;
    double x_min = std::numeric_limits<double>::max();
    double x_max = std::numeric_limits<double>::lowest();

    for (int i = 0; i < n; ++i) {
      const auto& a = v[i];
      const auto& b = v[(i + 1) % n];
      const int ay = gtl::y(a);
      const int by = gtl::y(b);
      // Edge crosses the band's mid-line?
      if ((ay <= y_mid && by > y_mid) || (by <= y_mid && ay > y_mid)) {
        const double t = (y_mid - ay) / static_cast<double>(by - ay);
        const double x = gtl::x(a) + t * (gtl::x(b) - gtl::x(a));
        x_min = std::min(x_min, x);
        x_max = std::max(x_max, x);
      }
    }

    if (x_max <= x_min) {
      continue;
    }

    // Round outward so the staircase fully covers the sloped edges.
    const int x_lo = static_cast<int>(std::floor(x_min));
    const int x_hi = static_cast<int>(std::ceil(x_max));
    if (x_hi <= x_lo) {
      continue;
    }

    // Coalesce with the previous band when it has the same X extent and is
    // vertically contiguous, to keep the rectangle count down for vertical
    // runs.
    if (!rects.empty() && rects.back().xMin() == x_lo
        && rects.back().xMax() == x_hi && rects.back().yMax() == y) {
      rects.back().set_yhi(y + 1);
    } else {
      rects.emplace_back(x_lo, y, x_hi, y + 1);
    }
  }
}

}  // namespace

void decompose_polygon(const std::vector<Point>& points,
                       std::vector<Rect>& rects)
{
  using boost::polygon::operators::operator+=;

  // Fast, exact path for Manhattan polygons (the common case: obstructions,
  // pin ports, and die-area blockages are almost always rectilinear).  This
  // preserves the previous behaviour and output exactly.
  if (isRectilinear(points)) {
    gtl::polygon_90_data<int> polygon;
    polygon.set(points.begin(), points.end());

    gtl::polygon_90_set_data<int> polygon_set;
    polygon_set += polygon;

    polygon_set.get_rectangles(rects);
    return;
  }

  // Non-rectilinear (e.g. 45-degree / octagonal pad) polygons cannot be
  // expressed by polygon_90 without corrupting the shape (it fills corners
  // that should be empty and leaves real metal uncovered).  Fracture the
  // general polygon into horizontal trapezoids and staircase each one into
  // axis-aligned rectangles.
  gtl::polygon_data<int> polygon;
  polygon.set(points.begin(), points.end());

  gtl::polygon_set_data<int> polygon_set;
  polygon_set += polygon;

  std::vector<gtl::polygon_data<int>> trapezoids;
  polygon_set.get_trapezoids(trapezoids, gtl::HORIZONTAL);

  for (const auto& trapezoid : trapezoids) {
    std::vector<gtl::point_data<int>> v(trapezoid.begin(), trapezoid.end());
    // Boost may repeat the closing vertex; drop it so edge iteration is clean.
    if (v.size() > 1 && gtl::x(v.front()) == gtl::x(v.back())
        && gtl::y(v.front()) == gtl::y(v.back())) {
      v.pop_back();
    }
    staircaseTrapezoid(v, rects);
  }
}

// See "Orientation of a simple polygon" in
// https://en.wikipedia.org/wiki/Curve_orientation
// The a, b, c point names are used to match the wiki page
bool polygon_is_clockwise(const std::vector<Point>& P)
{
  const int n = P.size();

  if (n < 3) {
    return false;
  }

  // find a point on the convex hull of the polygon
  // Here we use the lowest-most in Y with lowest in X as a tie breaker
  int yMin = std::numeric_limits<int>::max();
  int xMin = std::numeric_limits<int>::max();
  int b = 0;  // the index of the point we are seeking
  for (int i = 0; i < n; ++i) {
    const int x = P[i].x();
    const int y = P[i].y();
    if (y < yMin || (y == yMin && x < xMin)) {
      b = i;
      yMin = y;
      xMin = x;
    }
  }

  const int a = b > 0 ? b - 1 : n - 1;  // previous pt to b
  const int c = b < n - 1 ? b + 1 : 0;  // next pt to b

  const double xa = P[a].getX();
  const double ya = P[a].getY();

  const double xb = P[b].getX();
  const double yb = P[b].getY();

  const double xc = P[c].getX();
  const double yc = P[c].getY();

  const double det
      = (xb * yc + xa * yb + ya * xc) - (ya * xb + yb * xc + xa * yc);
  return det < 0.0;
}

}  // namespace odb
