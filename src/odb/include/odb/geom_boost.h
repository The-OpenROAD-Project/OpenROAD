///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// This header adapts odb's Point and Rect to work with Boost Polygon.
// It is a separate header so clients uninterested can just include geom.h.

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/polygon/polygon.hpp>

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
