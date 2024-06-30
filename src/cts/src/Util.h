/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cmath>
#include <iomanip>
#include <ostream>

#include "utl/Logger.h"

namespace cts {

inline bool fuzzyEqual(double x1, double x2, double epsilon = 1e-6)
{
  if (fabs(x1 - x2) < epsilon) {
    return true;
  }
  return false;
}

// x1 >= x2
inline bool fuzzyEqualOrGreater(double x1, double x2, double epsilon = 1e-6)
{
  if (fabs(x1 - x2) < epsilon) {
    return true;
  }
  return (x1 > x2);
}

// x1 <= x2
inline bool fuzzyEqualOrSmaller(double x1, double x2, double epsilon = 1e-6)
{
  if (fabs(x1 - x2) < epsilon) {
    return true;
  }
  return (x1 < x2);
}

template <class T>
class Point
{
 public:
  Point(T x, T y) : x_(x), y_(y) {}

  T getX() const { return x_; }
  T getY() const { return y_; }

  void setX(T x) { x_ = x; }
  void setY(T y) { y_ = y; }

  T computeDist(const Point<T>& other) const
  {
    T dx = (getX() > other.getX()) ? (getX() - other.getX())
                                   : (other.getX() - getX());
    T dy = (getY() > other.getY()) ? (getY() - other.getY())
                                   : (other.getY() - getY());

    return dx + dy;
  }

  T computeDistX(const Point<T>& other) const
  {
    T dx = (getX() > other.getX()) ? (getX() - other.getX())
                                   : (other.getX() - getX());
    return dx;
  }

  T computeDistY(const Point<T>& other) const
  {
    T dy = (getY() > other.getY()) ? (getY() - other.getY())
                                   : (other.getY() - getY());
    return dy;
  }

  bool equal(const Point<T>& other, double epsilon = 1e-6) const
  {
    if ((fabs(getX() - other.getX()) < epsilon)
        && (fabs(getY() - other.getY()) < epsilon)) {
      return true;
    }
    return false;
  }

  bool operator<(const Point<T>& other) const
  {
    if (getX() != other.getX()) {
      return getX() < other.getX();
    }

    return getY() < other.getY();
  }

  bool operator==(const Point<T>& other) const
  {
    if (equal(other)) {
      return true;
    }

    return false;
  }

  bool operator!=(const Point<T>& other) const
  {
    if (equal(other)) {
      return false;
    }

    return true;
  }

  friend std::ostream& operator<<(std::ostream& out, const Point<T>& point)
  {
    out << "[(" << std::fixed << std::setprecision(3) << point.getX() << ", "
        << std::fixed << std::setprecision(3) << point.getY() << ")]";
    return out;
  }

 private:
  T x_;
  T y_;
};

template <class T>
class Box
{
 public:
  T getMinX() const { return xMin_; }
  T getMinY() const { return yMin_; }
  T getMaxX() const { return xMin_ + width_; }
  T getMaxY() const { return yMin_ + height_; }
  T getWidth() const { return width_; }
  T getHeight() const { return height_; }

  Point<T> getCenter()
  {
    if (centerSet_) {
      Point<T> center(centerX_, centerY_);
      return center;
    }

    centerX_ = xMin_ + width_ / 2;
    centerY_ = yMin_ + height_ / 2;
    Point<T> center(centerX_, centerY_);
    centerSet_ = true;
    return center;
  }

  void setCenter(Point<T> point)
  {
    centerX_ = point.getX();
    centerY_ = point.getY();
    centerSet_ = true;
  }

  Box<double> normalize(double factor)
  {
    return Box<double>(getMinX() * factor,
                       getMinY() * factor,
                       getMaxX() * factor,
                       getMaxY() * factor);
  }

  Box() : xMin_(0), yMin_(0), width_(0), height_(0) {}
  Box(T xMin, T yMin, T xMax, T yMax)
      : xMin_(xMin), yMin_(yMin), width_(xMax - xMin), height_(yMax - yMin)
  {
  }

  friend std::ostream& operator<<(std::ostream& out, const Box& box)
  {
    out << "[(" << box.getMinX() << ", " << box.getMinY() << "), ("
        << box.getMaxX() << ", " << box.getMaxY() << ")]";
    return out;
  }

 private:
  T xMin_;
  T yMin_;
  T width_;
  T height_;
  T centerX_ = 0;
  T centerY_ = 0;
  bool centerSet_ = false;
};

using utl::format_as;

}  // namespace cts
