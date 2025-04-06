// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// This is adapted from Boost Polygon's isotropy.hpp.  The original
// includes many extraneous includes and unrelated code to the
// orientation and direction classes that are of use.  The code was
// modified to fit our coding standard and made more usable.  Below is
// a copy of the Boost license notice:

/*
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/

#pragma once

#include "utl/Logger.h"

namespace odb {

// Note that the use of the word 'orientation' here is distinct from
// the usage in LEF/DEF where it combines with the idea of flipping to
// give a different meaning (e.g. Flipped North or 'MY').

// You will rarely need to instantiate these classes directly as
// covenience objects are provided (near the end of the file).  All
// instances of these classes are immutable.

class Direction2D;
class Direction3D;

// Models direction along a one dimensional axis.  The two values are
// low and high.
class Direction1D
{
 public:
  enum Value
  {
    Low = 0,
    High = 1
  };

  constexpr Direction1D() = default;
  constexpr Direction1D(const Direction1D& other) = default;
  constexpr Direction1D(const Value value) : value_(value) {}
  // [East/North -> High] [West/South -> Low]
  constexpr explicit Direction1D(const Direction2D& other);
  // [East/North/Up -> High] [West/South/Down -> Low]
  constexpr explicit Direction1D(const Direction3D& other);

  Direction1D& operator=(const Direction1D& other) = default;

  constexpr operator Value() const { return static_cast<Value>(value_); }
  Direction1D flipped() const { return Value(value_ ^ 1); }

  bool operator==(const Direction1D& d) const { return value_ == d.value_; }
  bool operator!=(const Direction1D& d) const { return value_ != d.value_; }
  bool operator<(const Direction1D& d) const { return value_ < d.value_; }
  bool operator<=(const Direction1D& d) const { return value_ <= d.value_; }
  bool operator>(const Direction1D& d) const { return value_ > d.value_; }
  bool operator>=(const Direction1D& d) const { return value_ >= d.value_; }

 private:
  unsigned char value_ = Low;
};

// Models the axis orientation in two dimensional space.  The
// two axes are termed horizontal and vertical.
class Orientation2D
{
 public:
  enum Value
  {
    Horizontal = 0,
    Vertical = 1
  };

  constexpr Orientation2D() = default;
  constexpr Orientation2D(const Orientation2D& ori) = default;
  constexpr Orientation2D(const Value value) : value_(value) {}
  // [West / East -> Horizontal] [South / North -> Vertical]
  constexpr explicit Orientation2D(const Direction2D& other);

  Orientation2D& operator=(const Orientation2D& other) = default;

  constexpr operator Value() const { return static_cast<Value>(value_); }

  // [Horizontal -> Vertical] [Vertical -> Horizontal]
  Orientation2D turn_90() const { return Value(value_ ^ 1); }

  // [Horizontal: Low->West, High->East] [Vertical: Low->South, High->North]
  constexpr Direction2D getDirection(const Direction1D& dir) const;

  bool operator==(const Orientation2D& d) const { return value_ == d.value_; }
  bool operator!=(const Orientation2D& d) const { return value_ != d.value_; }
  bool operator<(const Orientation2D& d) const { return value_ < d.value_; }
  bool operator<=(const Orientation2D& d) const { return value_ <= d.value_; }
  bool operator>(const Orientation2D& d) const { return value_ > d.value_; }
  bool operator>=(const Orientation2D& d) const { return value_ >= d.value_; }

 private:
  unsigned char value_ = Horizontal;
};

// Models a direction in two dimensional space.  The possible values
// are west, east, north, and south.
class Direction2D
{
 public:
  enum Value
  {
    West = 0,
    East = 1,
    South = 2,
    North = 3
  };

  constexpr Direction2D() = default;
  constexpr Direction2D(const Direction2D& other) = default;
  constexpr Direction2D(const Value value) : value_(value) {}

  Direction2D& operator=(const Direction2D& other) = default;

  bool operator==(const Direction2D& d) const { return value_ == d.value_; }
  bool operator!=(const Direction2D& d) const { return value_ != d.value_; }
  bool operator<(const Direction2D& d) const { return value_ < d.value_; }
  bool operator<=(const Direction2D& d) const { return value_ <= d.value_; }
  bool operator>(const Direction2D& d) const { return value_ > d.value_; }
  bool operator>=(const Direction2D& d) const { return value_ >= d.value_; }

  // Casting to Value
  constexpr operator Value() const { return static_cast<Value>(value_); }

  // [East <-> West] [ North <-> South]
  Direction2D flipped() const { return Value(value_ ^ 1); }

  // Returns a direction 90 degree left (High) or right (Low) to this one
  Direction2D turn(const Direction1D& dir) const
  {
    return Value(value_ ^ 3 ^ (value_ >> 1) ^ dir);
  }

  // Returns a direction 90 degree left to this one
  Direction2D left() const { return turn(Direction1D::High); }

  // Returns a direction 90 degree right to this one
  Direction2D right() const { return turn(Direction1D::Low); }

  // North, East are positive; South, West are negative
  bool is_positive() const { return value_ & 1; }
  bool is_negative() const { return !is_positive(); }

 private:
  unsigned char value_ = West;
};

// This is axis orientation in three dimensional space.  This implicitly
// includes the Orientation2D values of vertical and horizontal and adds
// proximal perpendicular to those two.  The enum values are coordinated.
class Orientation3D
{
 public:
  enum Value
  {
    Proximal = 2
  };

  constexpr Orientation3D() = default;
  constexpr Orientation3D(const Orientation3D& ori) = default;
  constexpr Orientation3D(const Orientation2D& ori) : value_(ori) {}
  constexpr Orientation3D(const Value value) : value_(value) {}
  constexpr explicit Orientation3D(const Direction2D& other);
  constexpr explicit Orientation3D(const Direction3D& other);

  Orientation3D& operator=(const Orientation3D& other) = default;

  constexpr operator Value() const { return static_cast<Value>(value_); }

  // [Proximal: Low->down, High->Up] [others as Orientation2D]
  constexpr Direction3D getDirection(const Direction1D& dir) const;

  bool operator==(const Orientation3D&& d) const { return value_ == d.value_; }
  bool operator!=(const Orientation3D&& d) const { return value_ != d.value_; }
  bool operator<(const Orientation3D&& d) const { return value_ < d.value_; }
  bool operator<=(const Orientation3D&& d) const { return value_ <= d.value_; }
  bool operator>(const Orientation3D&& d) const { return value_ > d.value_; }
  bool operator>=(const Orientation3D&& d) const { return value_ >= d.value_; }

 private:
  unsigned char value_ = Orientation2D::Horizontal;
};

// Models a direction in three dimensional space.  This implicitly
// includes the Direction2D values of west, east, south, and north and adds
// up and down perpendicular to those four.  The enum values are coordinated.
class Direction3D
{
 public:
  enum Value
  {
    Down = 4,
    Up = 5
  };

  constexpr Direction3D() = default;
  constexpr Direction3D(Direction2D other) : value_(other) {}
  constexpr Direction3D(const Direction3D& other) = default;
  constexpr Direction3D(const Direction2D::Value value) : value_(value) {}
  constexpr Direction3D(const Value value) : value_(value) {}

  Direction3D& operator=(const Direction3D& other) = default;

  bool operator==(const Direction3D& d) const { return value_ == d.value_; }
  bool operator!=(const Direction3D& d) const { return value_ != d.value_; }
  bool operator<(const Direction3D& d) const { return value_ < d.value_; }
  bool operator<=(const Direction3D& d) const { return value_ <= d.value_; }
  bool operator>(const Direction3D& d) const { return value_ > d.value_; }
  bool operator>=(const Direction3D& d) const { return value_ >= d.value_; }

  // Casting to int
  constexpr operator Value() const { return static_cast<Value>(value_); }

  // [Up <-> Down] [others as Direction2D]
  Direction3D flipped() const { return Value(value_ ^ 1); }

  // North, East, Up are positive; South, West, Down are negative
  bool is_positive() const { return (value_ & 1); }
  bool is_negative() const { return !is_positive(); }

 private:
  unsigned char value_ = Direction2D::West;
};

constexpr Direction1D::Direction1D(const Direction2D& other) : value_(other & 1)
{
}

constexpr Direction1D::Direction1D(const Direction3D& other) : value_(other & 1)
{
}

constexpr Orientation2D::Orientation2D(const Direction2D& other)
    : value_(other >> 1)
{
}

constexpr Direction2D Orientation2D::getDirection(const Direction1D& dir) const
{
  return Direction2D(Direction2D::Value((value_ << 1) + dir));
}

constexpr Orientation3D::Orientation3D(const Direction2D& other)
    : value_(other >> 1)
{
}

constexpr Orientation3D::Orientation3D(const Direction3D& other)
    : value_(other >> 1)
{
}

constexpr Direction3D Orientation3D::getDirection(const Direction1D& dir) const
{
  return Direction3D(Direction3D::Value((value_ << 1) + dir));
}

std::ostream& operator<<(std::ostream& os, const Orientation2D& ori);
std::ostream& operator<<(std::ostream& os, const Orientation3D& ori);
std::ostream& operator<<(std::ostream& os, const Direction1D& dir);
std::ostream& operator<<(std::ostream& os, const Direction2D& dir);
std::ostream& operator<<(std::ostream& os, const Direction3D& dir);

// Convenience objects that will be commonly used.
constexpr Orientation2D horizontal{Orientation2D::Horizontal};
constexpr Orientation2D vertical{Orientation2D::Vertical};
constexpr Orientation3D proximal{Orientation3D::Proximal};

constexpr Direction1D low{Direction1D::Low};
constexpr Direction1D high{Direction1D::High};

constexpr Direction2D west{Direction2D::West};
constexpr Direction2D east{Direction2D::East};
constexpr Direction2D south{Direction2D::South};
constexpr Direction2D north{Direction2D::North};

constexpr Direction3D down{Direction3D::Down};
constexpr Direction3D up{Direction3D::Up};

using utl::format_as;

}  // namespace odb
