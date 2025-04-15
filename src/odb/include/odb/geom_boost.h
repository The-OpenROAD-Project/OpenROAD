// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// This header adapts odb's Point and Rect to work with Boost Polygon.
// It is a separate header so clients uninterested can just include geom.h.

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/polygon/polygon.hpp>
#include <vector>

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

  static inline interval_type get(const odb::Rect& rectangle,
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
  static inline void set(odb::Rect& rectangle,
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
  static inline odb::Rect construct(const T2& interval_horizontal,
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
  static const std::vector<odb::Point> get(const odb::Oct& o)
  {
    return o.getPoints();
  }
};

template <>
struct interior_rings<odb::Oct>
{
  static std::vector<std::vector<odb::Point>> get(odb::Oct& o) { return {}; }
  static const std::vector<std::vector<odb::Point>> get(const odb::Oct& o)
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
  static const std::vector<odb::Point> get(const odb::Polygon& p)
  {
    return p.getPoints();
  }
};

template <>
struct interior_rings<odb::Polygon>
{
  static std::vector<std::vector<odb::Point>> get(odb::Polygon& p)
  {
    return {};
  }
  static const std::vector<std::vector<odb::Point>> get(const odb::Polygon& p)
  {
    return {};
  }
};

}  // namespace boost::geometry::traits
