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

#include <ostream>
#include <spdlog/fmt/fmt.h>

namespace cts {

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

  bool operator<(const Point<T>& other) const
  {
    if (getX() != other.getX()) {
      return getX() < other.getX();
    } else {
      return getY() < other.getY();
    }
  }

  friend std::ostream& operator<<(std::ostream& out, const Point<T>& point)
  {
    out << "[(" << point.getX() << ", " << point.getY() << ")]";
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

  Point<T> computeCenter() const
  {
    return Point<T>(xMin_ + width_ / 2, yMin_ + height_ / 2);
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

 protected:
  T xMin_;
  T yMin_;
  T width_;
  T height_;
};

}  // namespace cts

#if defined(FMT_VERSION) && FMT_VERSION >= 90000
#include <fmt/ostream.h>
template <typename T> struct fmt::formatter<cts::Box<T>> : fmt::ostream_formatter {};
#endif // FMT_VERSION >= 90000
