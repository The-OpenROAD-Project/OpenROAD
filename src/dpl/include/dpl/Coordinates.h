// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <boost/operators.hpp>
#include <cmath>
#include <functional>
#include <ostream>

#include "Opendp.h"

namespace dpl {

// Strongly type the difference between pixel and DBU locations.
//
// Multiplication and division are intentionally not defined as they
// are often, but not always, used to convert between DBUs and pixels
// and such operations must be explicit about the resulting type.

template <typename T>
struct TypedCoordinate : public boost::totally_ordered<TypedCoordinate<T>>,
                         public boost::totally_ordered<TypedCoordinate<T>, int>,
                         public boost::additive<TypedCoordinate<T>>,
                         public boost::additive<TypedCoordinate<T>, int>,
                         public boost::modable<TypedCoordinate<T>>,
                         public boost::modable<TypedCoordinate<T>, int>,
                         public boost::incrementable<TypedCoordinate<T>>,
                         public boost::decrementable<TypedCoordinate<T>>,
                         public boost::dividable<TypedCoordinate<T>>,
                         public boost::multipliable<TypedCoordinate<T>>
{
  explicit TypedCoordinate(const int v = 0) : v(v) {}
  TypedCoordinate(const TypedCoordinate<T>& v) = default;

  // totally_ordered
  bool operator<(const TypedCoordinate& rhs) const { return v < rhs.v; }
  bool operator<(const int rhs) const { return v < rhs; }
  bool operator>(const int rhs) const { return v > rhs; }
  bool operator==(const TypedCoordinate& rhs) const { return v == rhs.v; }
  bool operator==(const int rhs) const { return v == rhs; }
  // additive
  TypedCoordinate& operator+=(const TypedCoordinate& rhs)
  {
    v += rhs.v;
    return *this;
  }
  TypedCoordinate& operator+=(const int rhs)
  {
    v += rhs;
    return *this;
  }
  TypedCoordinate& operator-=(const TypedCoordinate& rhs)
  {
    v -= rhs.v;
    return *this;
  }
  TypedCoordinate& operator-=(const int rhs)
  {
    v -= rhs;
    return *this;
  }
  // modable
  TypedCoordinate& operator%=(const TypedCoordinate& rhs)
  {
    v %= rhs.v;
    return *this;
  }

  // incrementable
  TypedCoordinate& operator++()
  {
    ++v;
    return *this;
  }

  // decrementable
  TypedCoordinate& operator--()
  {
    --v;
    return *this;
  }
  // dividable
  TypedCoordinate& operator/=(const TypedCoordinate& rhs)
  {
    v /= rhs.v;
    return *this;
  }
  // multipliable
  TypedCoordinate& operator*=(const TypedCoordinate& rhs)
  {
    v *= rhs.v;
    return *this;
  }

  int v;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& o, const TypedCoordinate<T>& tc)
{
  return o << tc.v;
}

template <typename T>
TypedCoordinate<T> abs(const TypedCoordinate<T>& val)
{
  return TypedCoordinate<T>{std::abs(val.v)};
}

struct GridPt
{
  GridPt() = default;
  GridPt(GridX x, GridY y) : x(x), y(y) {}
  bool operator==(const GridPt& p) const { return (x == p.x) && (y == p.y); }
  GridX x{0};
  GridY y{0};
};

struct GridRect
{
  GridX xlo;
  GridY ylo;
  GridX xhi;
  GridY yhi;
  GridRect intersect(const GridRect& r) const;
  GridPt closestPtInside(GridPt pt) const;
  Rect toRect() const;
};

inline Rect GridRect::toRect() const
{
  return Rect(xlo.v, ylo.v, xhi.v, yhi.v);
}

inline GridPt GridRect::closestPtInside(GridPt pt) const
{
  auto closest = toRect().closestPtInside({pt.x.v, pt.y.v});
  return {GridX{closest.x()}, GridY{closest.y()}};
}

inline GridRect GridRect::intersect(const GridRect& r) const
{
  GridRect result;
  result.xlo = std::max(xlo, r.xlo);
  result.ylo = std::max(ylo, r.ylo);
  result.xhi = std::min(xhi, r.xhi);
  result.yhi = std::min(yhi, r.yhi);
  return result;
}

struct DbuPt
{
  DbuPt() = default;
  DbuPt(DbuX x, DbuY y) : x(x), y(y) {}
  DbuX x{0};
  DbuY y{0};
};

struct DbuRect
{
  DbuRect(const Rect& rect)
      : xl(rect.xMin()), yl(rect.yMin()), xh(rect.xMax()), yh(rect.yMax())
  {
  }
  DbuX dx() const { return xh - xl; }
  DbuY dy() const { return yh - yl; }
  DbuX xl{0};
  DbuY yl{0};
  DbuX xh{0};
  DbuY yh{0};
};

inline bool operator==(const DbuPt& p1, const DbuPt& p2)
{
  return std::tie(p1.x, p1.y) == std::tie(p2.x, p2.y);
}

inline DbuX gridToDbu(GridX x, DbuX scale)
{
  return DbuX{x.v * scale.v};
}

inline DbuY gridToDbu(GridY y, DbuY scale)
{
  return DbuY{y.v * scale.v};
}

inline GridX dbuToGridCeil(DbuX x, DbuX divisor)
{
  return GridX{divCeil(x.v, divisor.v)};
}

inline GridX dbuToGridFloor(DbuX x, DbuX divisor)
{
  return GridX{divFloor(x.v, divisor.v)};
}

inline GridY dbuToGridCeil(DbuY y, DbuY divisor)
{
  return GridY{divCeil(y.v, divisor.v)};
}

inline GridY dbuToGridFloor(DbuY y, DbuY divisor)
{
  return GridY{divFloor(y.v, divisor.v)};
}

inline int sumXY(DbuX x, DbuY y)
{
  return x.v + y.v;
}

}  // namespace dpl

// Enable use with unordered_map/set
namespace std {

template <typename T>
struct hash<dpl::TypedCoordinate<T>>
{
  std::size_t operator()(const dpl::TypedCoordinate<T>& tc) const noexcept
  {
    return std::hash<int>()(tc.v);
  }
};

template <>
struct hash<dpl::GridPt>
{
  std::size_t operator()(const dpl::GridPt& p) const
  {
    size_t hashX = std::hash<dpl::GridX>{}(p.x);
    size_t hashY = std::hash<dpl::GridY>{}(p.y);
    return hashX ^ (hashY + 0x9e3779b9 + (hashX << 6) + (hashX >> 2));
  }
};

}  // namespace std
