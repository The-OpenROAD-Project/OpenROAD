// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// This header adapts odb's Point and Rect to work with Boost Polygon.
// It is a separate header so clients uninterested can just include geom.h.

#pragma once

#include <concepts>
#include <cstddef>
#include <vector>

#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometries/register/ring.hpp"
#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/geom.h"

// Make odb's Point work with boost polgyon

template <>
struct boost::polygon::geometry_concept<odb::Point>
{
  using type = point_concept;
};

template <>
struct boost::polygon::point_traits<odb::Point>
{
  using coordinate_type = int;

  static int get(const odb::Point& point, const orientation_2d& orient)
  {
    if (orient == HORIZONTAL) {
      return point.getX();
    }
    return point.getY();
  }
};

template <>
struct boost::polygon::point_mutable_traits<odb::Point>
{
  using coordinate_type = int;

  static void set(odb::Point& point,
                  const orientation_2d& orient,
                  const int value)
  {
    if (orient == HORIZONTAL) {
      point.setX(value);
    } else {
      point.setY(value);
    }
  }

  static odb::Point construct(int x_value, int y_value)
  {
    return odb::Point(x_value, y_value);
  }
};

// Make odb's Point work with boost geometry

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(odb::Point,
                                         int,
                                         boost::geometry::cs::cartesian,
                                         getX,
                                         getY,
                                         setX,
                                         setY);

// Register odb's Point vector as ring.

BOOST_GEOMETRY_REGISTER_RING(std::vector<odb::Point>);

// Make odb's Rect work with boost polgyon

template <>
struct boost::polygon::geometry_concept<odb::Rect>
{
  using type = rectangle_concept;
};

template <>
struct boost::polygon::rectangle_traits<odb::Rect>
{
  using coordinate_type = int;
  using interval_type = interval_data<int>;

  static interval_type get(const odb::Rect& rectangle,
                           const orientation_2d& orient)
  {
    if (orient == HORIZONTAL) {
      return {rectangle.xMin(), rectangle.xMax()};
    }
    return {rectangle.yMin(), rectangle.yMax()};
  }
};

template <>
struct boost::polygon::rectangle_mutable_traits<odb::Rect>
{
  template <typename T2>
  static void set(odb::Rect& rectangle,
                  const orientation_2d& orient,
                  const T2& interval)
  {
    if (orient == HORIZONTAL) {
      rectangle.set_xlo(low(interval));
      rectangle.set_xhi(high(interval));
    } else {
      rectangle.set_ylo(low(interval));
      rectangle.set_yhi(high(interval));
    }
  }

  template <typename T2, typename T3>
  static odb::Rect construct(const T2& interval_horizontal,
                             const T3& interval_vertical)
  {
    return odb::Rect(low(interval_horizontal),
                     low(interval_vertical),
                     high(interval_horizontal),
                     high(interval_vertical));
  }
};

// Make odb's Rect work with boost geometry.
//
// Unfortunately BOOST_GEOMETRY_REGISTER_BOX forces a bad API on the class
// and there is not _GET_SET version.  Instead we have to go lower to the
// traits to adapt.

namespace boost::geometry::traits {

template <>
struct tag<odb::Rect>
{
  using type = box_tag;
};

template <>
struct point_type<odb::Rect>
{
  using type = odb::Point;
};

template <std::size_t Dimension>
struct indexed_access<odb::Rect, min_corner, Dimension>
{
  using coordinate_type = int;

  static constexpr coordinate_type get(const odb::Rect& b)
  {
    return (Dimension == 0) ? b.xMin() : b.yMin();
  }

  static void set(odb::Rect& b, const int value)
  {
    if (Dimension == 0) {
      b.set_xlo(value);
    } else {
      b.set_ylo(value);
    }
  }
};

template <std::size_t Dimension>
struct indexed_access<odb::Rect, max_corner, Dimension>
{
  using coordinate_type = int;

  static constexpr coordinate_type get(const odb::Rect& b)
  {
    return (Dimension == 0) ? b.xMax() : b.yMax();
  }

  static void set(odb::Rect& b, const int value)
  {
    if (Dimension == 0) {
      b.set_xhi(value);
    } else {
      b.set_yhi(value);
    }
  }
};

//
// Make odb's Oct work with boost geometry.
//

template <>
struct tag<odb::Oct>
{
  using type = polygon_tag;
};

template <>
struct ring_mutable_type<odb::Oct>
{
  using type = std::vector<odb::Point>;
};

template <>
struct ring_const_type<odb::Oct>
{
  using type = const std::vector<odb::Point>;
};

template <>
struct interior_const_type<odb::Oct>
{
  using type = const std::vector<std::vector<odb::Point>>;
};

template <>
struct interior_mutable_type<odb::Oct>
{
  using type = std::vector<std::vector<odb::Point>>;
};

template <>
struct exterior_ring<odb::Oct>
{
  static std::vector<odb::Point> get(odb::Oct& o) { return o.getPoints(); }
  static std::vector<odb::Point> get(const odb::Oct& o)
  {
    return o.getPoints();
  }
};

template <>
struct interior_rings<odb::Oct>
{
  static std::vector<std::vector<odb::Point>> get(odb::Oct&) { return {}; }
  static std::vector<std::vector<odb::Point>> get(const odb::Oct&)
  {
    return {};
  }
};

//
// Make odb's Polygon work with boost geometry.
//

template <>
struct tag<odb::Polygon>
{
  using type = polygon_tag;
};

template <>
struct ring_mutable_type<odb::Polygon>
{
  using type = std::vector<odb::Point>;
};

template <>
struct ring_const_type<odb::Polygon>
{
  using type = const std::vector<odb::Point>;
};

template <>
struct interior_const_type<odb::Polygon>
{
  using type = const std::vector<std::vector<odb::Point>>;
};

template <>
struct interior_mutable_type<odb::Polygon>
{
  using type = std::vector<std::vector<odb::Point>>;
};

template <>
struct exterior_ring<odb::Polygon>
{
  static std::vector<odb::Point> get(odb::Polygon& p) { return p.getPoints(); }
  static std::vector<odb::Point> get(const odb::Polygon& p)
  {
    return p.getPoints();
  }
};

template <>
struct interior_rings<odb::Polygon>
{
  static std::vector<std::vector<odb::Point>> get(odb::Polygon&) { return {}; }
  static std::vector<std::vector<odb::Point>> get(const odb::Polygon&)
  {
    return {};
  }
};

//
// Make odb's Line work with boost geometry.
//

template <>
struct tag<odb::Line>
{
  using type = segment_tag;
};

template <>
struct point_type<odb::Line>
{
  using type = odb::Point;
};

template <std::size_t Index, std::size_t Dimension>
struct indexed_access<odb::Line, Index, Dimension>
{
  using coordinate_type = int;

  static constexpr coordinate_type get(const odb::Line& line)
  {
    if (Index == 0) {
      return Dimension == 0 ? line.pt0().getX() : line.pt0().getY();
    }
    return Dimension == 0 ? line.pt1().getX() : line.pt1().getY();
  }

  static void set(odb::Line& line, const int value)
  {
    if (Index == 0) {
      odb::Point pt = line.pt0();
      if (Dimension == 0) {
        pt.setX(value);
      } else {
        pt.setY(value);
      }
      line.setPt0(pt);
    } else {
      odb::Point pt = line.pt1();
      if (Dimension == 0) {
        pt.setX(value);
      } else {
        pt.setY(value);
      }
      line.setPt1(pt);
    }
  }
};

}  // namespace boost::geometry::traits

namespace odb::geom {

//
// Arbitrary angle boost polygon types.  These can hold a 45 degree edge, and
// so are the only safe choice for anything derived from an Oct.  Native boost
// types are used to avoid needing mutable access to odb::Polygon.
//
using BoostPolygon = boost::polygon::polygon_data<int>;
using BoostPolygonSet = boost::polygon::polygon_set_data<int>;

//
// Manhattan only boost polygon types.  Considerably cheaper than the
// arbitrary angle set, but every edge must be axis aligned.
//
using BoostRectangle = boost::polygon::rectangle_data<int>;
using BoostPolygon90 = boost::polygon::polygon_90_data<int>;
using BoostPolygon90WithHoles = boost::polygon::polygon_90_with_holes_data<int>;
using BoostPolygon90Set = boost::polygon::polygon_90_set_data<int>;

// The odb shapes that enclose an area, and so can seed a polygon set.  This
// is spelled out rather than deduced from getPoints() because odb::Line also
// has getPoints() but bounds no area.
template <typename T>
concept AreaShape
    = std::same_as<T, Rect> || std::same_as<T, Oct> || std::same_as<T, Polygon>;

// The odb shapes that may be handed to the polygon_90 flavor of boost
// polygon.  Oct is excluded: polygon_90_data stores only a compressed
// alternating x/y coordinate list, so a 45 degree edge cannot be represented
// and boost would silently build a different shape.  Polygon is accepted, but
// it is the caller's responsibility to know it holds no 45 degree edges - one
// constructed from an Oct does.
template <typename T>
concept ManhattanShape = std::same_as<T, Rect> || std::same_as<T, Polygon>;

namespace detail {

// Copy the vertices of a boost polygon into an odb Polygon.
template <typename BoostPolygonType>
Polygon toPolygon(const BoostPolygonType& boost_polygon)
{
  std::vector<Point> points;
  points.reserve(boost_polygon.size());
  for (const auto& pt : boost_polygon) {
    points.emplace_back(pt.x(), pt.y());
  }
  return Polygon(points);
}

// Copy the polygons held by a polygon set into odb Polygons.  BoostPolygonType
// selects which boost polygon flavor the set is asked to hand back.
template <typename BoostPolygonType, typename BoostSetType>
std::vector<Polygon> extractPolygons(const BoostSetType& polygon_set)
{
  std::vector<BoostPolygonType> output_polygons;
  polygon_set.get(output_polygons);

  std::vector<Polygon> result;
  result.reserve(output_polygons.size());
  for (const BoostPolygonType& boost_polygon : output_polygons) {
    result.push_back(toPolygon(boost_polygon));
  }

  return result;
}

}  // namespace detail

//
// odb shapes -> boost
//

// Convert a Manhattan shape into a single boost polygon
template <ManhattanShape T>
BoostPolygon90 toPolygon90(const T& shape)
{
  const std::vector<Point> points = shape.getPoints();

  BoostPolygon90 polygon;
  polygon.set(points.begin(), points.end());

  return polygon;
}

// Collect the shape into a polygon set
template <AreaShape T>
BoostPolygonSet toPolygonSet(const T& shape)
{
  using boost::polygon::operators::operator+=;

  const std::vector<Point> points = shape.getPoints();

  BoostPolygonSet polygon_set;
  polygon_set += BoostPolygon(points.begin(), points.end());

  return polygon_set;
}

// Collect the shapes into a single polygon set, which unions overlapping
// shapes together
template <AreaShape T>
BoostPolygonSet toPolygonSet(const std::vector<T>& shapes)
{
  using boost::polygon::operators::operator+=;

  BoostPolygonSet polygon_set;
  for (const T& shape : shapes) {
    const std::vector<Point> points = shape.getPoints();
    polygon_set += BoostPolygon(points.begin(), points.end());
  }

  return polygon_set;
}

// Collect the Manhattan shape into a polygon set
template <ManhattanShape T>
BoostPolygon90Set toPolygonSet90(const T& shape)
{
  using boost::polygon::operators::operator+=;

  BoostPolygon90Set polygon_set;
  polygon_set += toPolygon90(shape);

  return polygon_set;
}

// Collect the Manhattan shapes into a single polygon set, which unions
// overlapping shapes together
template <ManhattanShape T>
BoostPolygon90Set toPolygonSet90(const std::vector<T>& shapes)
{
  using boost::polygon::operators::operator+=;

  BoostPolygon90Set polygon_set;
  for (const T& shape : shapes) {
    polygon_set += toPolygon90(shape);
  }

  return polygon_set;
}

//
// boost -> odb shapes
//

// Convert a boost rectangle into an odb Rect
inline Rect toRect(const BoostRectangle& rectangle)
{
  return Rect(boost::polygon::xl(rectangle),
              boost::polygon::yl(rectangle),
              boost::polygon::xh(rectangle),
              boost::polygon::yh(rectangle));
}

// Extract the polygons held by a polygon set as odb polygons
inline std::vector<Polygon> extractPolygons(const BoostPolygonSet& polygon_set)
{
  return detail::extractPolygons<BoostPolygon>(polygon_set);
}

inline std::vector<Polygon> extractPolygons(
    const BoostPolygon90Set& polygon_set)
{
  return detail::extractPolygons<BoostPolygon90>(polygon_set);
}

// Decompose a polygon set into rectangles
inline std::vector<Rect> extractRectangles(const BoostPolygon90Set& polygon_set)
{
  std::vector<Rect> rects;
  polygon_set.get_rectangles(rects);

  return rects;
}

// Decompose a polygon set into rectangles, slicing along the given
// orientation.  Pass the non-preferred direction of the layer to get
// rectangles that run in its preferred direction.
inline std::vector<Rect> extractRectangles(
    const BoostPolygon90Set& polygon_set,
    const boost::polygon::orientation_2d& slicing_orientation)
{
  std::vector<Rect> rects;
  polygon_set.get_rectangles(rects, slicing_orientation);

  return rects;
}

// Bounding box of a boost polygon or polygon set.  An empty shape yields a
// default constructed (zero area, origin) Rect.
template <typename BoostShapeType>
Rect getEnclosingRect(const BoostShapeType& shape)
{
  Rect rect;
  boost::polygon::extents(rect, shape);

  return rect;
}

// Merge a collection of shapes into a single set of polygons.  Overlapping
// shapes are unioned together, and the result is decomposed back into polygons.
template <AreaShape T>
std::vector<Polygon> mergePolygons(const std::vector<T>& shapes)
{
  return extractPolygons(toPolygonSet(shapes));
}

}  // namespace odb::geom
