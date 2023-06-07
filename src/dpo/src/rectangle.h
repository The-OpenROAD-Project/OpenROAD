///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

#pragma once

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include <limits>

namespace dpo {

template <typename T>
class Rectangle_b
{
 public:
  Rectangle_b() { reset(); }
  Rectangle_b(T xmin, T ymin, T xmax, T ymax)
      : xmin_(xmin), ymin_(ymin), xmax_(xmax), ymax_(ymax)
  {
  }
  Rectangle_b(const Rectangle_b& rect)
      : xmin_(rect.xmin_),
        ymin_(rect.ymin_),
        xmax_(rect.xmax_),
        ymax_(rect.ymax_)
  {
  }
  Rectangle_b& operator=(const Rectangle_b& other)
  {
    if (this != &other) {
      xmin_ = other.xmin_;
      xmax_ = other.xmax_;
      ymin_ = other.ymin_;
      ymax_ = other.ymax_;
    }
    return *this;
  }

  void reset()
  {
    xmin_ = std::numeric_limits<T>::max();
    ymin_ = std::numeric_limits<T>::max();
    xmax_ = std::numeric_limits<T>::lowest();
    ymax_ = std::numeric_limits<T>::lowest();
  }

  void enlarge(const Rectangle_b& r)
  {
    xmin_ = (xmin_ > r.xmin_ ? r.xmin_ : xmin_);
    ymin_ = (ymin_ > r.ymin_ ? r.ymin_ : ymin_);
    xmax_ = (xmax_ < r.xmax_ ? r.xmax_ : xmax_);
    ymax_ = (ymax_ < r.ymax_ ? r.ymax_ : ymax_);
  }
  bool intersects(const Rectangle_b& r) const
  {
    return !(xmin_ > r.xmax_ || xmax_ < r.xmin_ || ymin_ > r.ymax_
             || ymax_ < r.ymin_);
  }
  bool is_overlap(T xmin, T ymin, T xmax, T ymax)
  {
    if (xmin >= xmax_) {
      return false;
    }
    if (xmax <= xmin_) {
      return false;
    }
    if (ymin >= ymax_) {
      return false;
    }
    if (ymax <= ymin_) {
      return false;
    }
    return true;
  }

  bool contains(const Rectangle_b& r)
  {
    if (r.xmin_ >= xmin_ && r.xmax_ <= xmax_ && r.ymin_ >= ymin_
        && r.ymax_ <= ymax_) {
      return true;
    }
    return false;
  }

  void addPt(T x, T y)
  {
    xmin_ = std::min(xmin_, x);
    xmax_ = std::max(xmax_, x);
    ymin_ = std::min(ymin_, y);
    ymax_ = std::max(ymax_, y);
  }
  T getCenterX() { return 0.5 * (xmax_ + xmin_); }
  T getCenterY() { return 0.5 * (ymax_ + ymin_); }
  T getWidth() { return xmax_ - xmin_; }
  T getHeight() { return ymax_ - ymin_; }
  void clear() { reset(); }

  T xmin() const { return xmin_; }
  T xmax() const { return xmax_; }
  T ymin() const { return ymin_; }
  T ymax() const { return ymax_; }

  T area() const { return (xmax_ - xmin_) * (ymax_ - ymin_); }

  void set_xmin(T val) { xmin_ = val; }
  void set_xmax(T val) { xmax_ = val; }
  void set_ymin(T val) { ymin_ = val; }
  void set_ymax(T val) { ymax_ = val; }

 private:
  T xmin_;
  T ymin_;
  T xmax_;
  T ymax_;
};

using Rectangle = Rectangle_b<double>;  // Legacy.

using Rectangle_d = Rectangle_b<double>;
using Rectangle_i = Rectangle_b<int>;

}  // namespace dpo
