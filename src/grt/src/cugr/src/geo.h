//
// Some class templates for geometry primitives (point, interval, box)
//

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include "utl/Logger.h"

namespace grt {

using utl::format_as;

// Point template
class PointT
{
 public:
  PointT() = default;
  PointT(int x, int y) : x_(x), y_(y) {}

  bool IsValid() { return *this != PointT(); }

  int x() const { return x_; }
  int y() const { return y_; }

  // Operators
  const int& operator[](const int d) const
  {
    assert(d == 0 || d == 1);
    return (d == 0 ? x_ : y_);
  }
  int& operator[](const int d)
  {
    assert(d == 0 || d == 1);
    return (d == 0 ? x_ : y_);
  }
  PointT operator+(const PointT& rhs)
  {
    return PointT(x_ + rhs.x_, y_ + rhs.y_);
  }
  PointT operator/(int divisor) { return PointT(x_ / divisor, y_ / divisor); }
  PointT& operator+=(const PointT& rhs)
  {
    x_ += rhs.x_;
    y_ += rhs.y_;
    return *this;
  }
  PointT& operator-=(const PointT& rhs)
  {
    x_ -= rhs.x_;
    y_ -= rhs.y_;
    return *this;
  }
  bool operator==(const PointT& rhs) const
  {
    return x_ == rhs.x_ && y_ == rhs.y_;
  }
  bool operator!=(const PointT& rhs) const { return !(*this == rhs); }

  friend std::ostream& operator<<(std::ostream& os, const PointT& pt)
  {
    os << "(" << pt.x_ << ", " << pt.y_ << ")";
    return os;
  }

 private:
  int x_{std::numeric_limits<int>::max()};
  int y_{std::numeric_limits<int>::max()};
};

// Interval template
class IntervalT
{
 public:
  IntervalT() { Set(); }
  IntervalT(int val) { Set(val); }
  IntervalT(int lo, int hi) { Set(lo, hi); }

  int low() const { return low_; }
  int high() const { return high_; }

  // Setters
  void addToLow(const int increment) { low_ += increment; }
  void addToHigh(const int increment) { high_ += increment; }

  void Set()
  {
    low_ = std::numeric_limits<int>::max();
    high_ = std::numeric_limits<int>::min();
  }
  void Set(int val)
  {
    low_ = val;
    high_ = val;
  }
  void Set(int lo, int hi)
  {
    low_ = lo;
    high_ = hi;
  }
  void SetLow(int val) { low_ = val; }
  void SetHigh(int val) { high_ = val; }

  // Getters
  int center() const { return (high_ + low_) / 2; }
  int range() const { return high_ - low_; }

  // Update
  // Update() is always safe, FastUpdate() assumes existing values
  void Update(int newVal)
  {
    if (newVal < low_) {
      low_ = newVal;
    }
    if (newVal > high_) {
      high_ = newVal;
    }
  }
  void FastUpdate(int newVal)
  {
    if (newVal < low_) {
      low_ = newVal;
    } else if (newVal > high_) {
      high_ = newVal;
    }
  }

  // Two types of intervals: 1. normal, 2. degenerated (i.e., point)
  // is valid interval (i.e., valid closed interval)
  bool IsValid() const { return low_ <= high_; }
  // is strictly valid interval (excluding degenerated ones, i.e., valid open
  // interval)
  bool IsStrictValid() const { return low_ < high_; }

  // Geometric Query/Update
  // interval/range of union (not union of intervals)
  IntervalT UnionWith(const IntervalT& rhs) const
  {
    if (!IsValid()) {
      return rhs;
    }

    if (!rhs.IsValid()) {
      return *this;
    }

    return IntervalT(std::min(low_, rhs.low_), std::max(high_, rhs.high_));
  }
  // may return an invalid interval (as empty intersection)
  IntervalT IntersectWith(const IntervalT& rhs) const
  {
    return IntervalT(std::max(low_, rhs.low_), std::min(high_, rhs.high_));
  }
  bool HasIntersectWith(const IntervalT& rhs) const
  {
    return IntersectWith(rhs).IsValid();
  }
  bool HasStrictIntersectWith(const IntervalT& rhs) const
  {
    return IntersectWith(rhs).IsStrictValid();
  }
  // contain a val
  bool Contain(int val) const { return val >= low_ && val <= high_; }
  bool StrictlyContain(int val) const { return val > low_ && val < high_; }
  // get nearest point(s) to val (assume valid intervals)
  int GetNearestPointTo(int val) const
  {
    if (val <= low_) {
      return low_;
    }

    if (val >= high_) {
      return high_;
    }

    return val;
  }
  IntervalT GetNearestPointsTo(IntervalT val) const
  {
    if (val.high_ <= low_) {
      return {low_};
    }

    if (val.low_ >= high_) {
      return {high_};
    }

    return IntersectWith(val);
  }

  void ShiftBy(const int& rhs)
  {
    low_ += rhs;
    high_ += rhs;
  }

  // Operators
  bool operator==(const IntervalT& rhs) const
  {
    return (!IsValid() && !rhs.IsValid())
           || (low_ == rhs.low_ && high_ == rhs.high_);
  }
  bool operator!=(const IntervalT& rhs) const { return !(*this == rhs); }

  friend std::ostream& operator<<(std::ostream& os, const IntervalT& interval)
  {
    os << "(" << interval.low_ << ", " << interval.high_ << ")";
    return os;
  }

 private:
  int low_;
  int high_;
};

// Box template
class BoxT
{
 public:
  BoxT() { Set(); }
  BoxT(int xVal, int yVal) { Set(xVal, yVal); }
  BoxT(const PointT& pt) { Set(pt); }
  BoxT(int lx, int ly, int hx, int hy) { Set(lx, ly, hx, hy); }
  BoxT(const IntervalT& xRange, const IntervalT& yRange)
  {
    Set(xRange, yRange);
  }
  BoxT(const PointT& low, const PointT& high) { Set(low, high); }
  BoxT(const BoxT& box) { Set(box); }

  IntervalT& operator[](int i)
  {
    assert(i == 0 || i == 1);
    return (i == 0) ? x_ : y_;
  }
  void Set()
  {
    x_.Set();
    y_.Set();
  }
  void Set(int xVal, int yVal)
  {
    x_.Set(xVal);
    y_.Set(yVal);
  }
  void Set(const PointT& pt) { Set(pt.x(), pt.y()); }
  void Set(int lx, int ly, int hx, int hy)
  {
    x_.Set(lx, hx);
    y_.Set(ly, hy);
  }
  void Set(const IntervalT& xRange, const IntervalT& yRange)
  {
    x_ = xRange;
    y_ = yRange;
  }
  void Set(const PointT& low, const PointT& high)
  {
    Set(low.x(), low.y(), high.x(), high.y());
  }
  void Set(const BoxT& box) { Set(box.x(), box.y()); }

  // Two types of boxes: normal & degenerated (line or point)
  // is valid box
  bool IsValid() const { return x_.IsValid() && y_.IsValid(); }
  // is strictly valid box (excluding degenerated ones)
  bool IsStrictValid() const
  {
    return x_.IsStrictValid() && y_.IsStrictValid();
  }  // tighter

  // Getters
  const IntervalT& x() const { return x_; }
  const IntervalT& y() const { return y_; }
  int lx() const { return x_.low(); }
  int ly() const { return y_.low(); }
  int hy() const { return y_.high(); }
  int hx() const { return x_.high(); }
  int cx() const { return x_.center(); }
  int cy() const { return y_.center(); }
  int width() const { return x_.range(); }
  int height() const { return y_.range(); }
  int hp() const { return width() + height(); }  // half perimeter
  int area() const { return width() * height(); }
  const IntervalT& operator[](int i) const
  {
    assert(i == 0 || i == 1);
    return (i == 0) ? x_ : y_;
  }

  // Update() is always safe, FastUpdate() assumes existing values
  void Update(int xVal, int yVal)
  {
    x_.Update(xVal);
    y_.Update(yVal);
  }
  void FastUpdate(int xVal, int yVal)
  {
    x_.FastUpdate(xVal);
    y_.FastUpdate(yVal);
  }
  void Update(const PointT& pt) { Update(pt.x(), pt.y()); }
  void FastUpdate(const PointT& pt) { FastUpdate(pt.x(), pt.y()); }

  // Geometric Query/Update
  BoxT UnionWith(const BoxT& rhs) const
  {
    return {x_.UnionWith(rhs.x_), y_.UnionWith(rhs.y_)};
  }
  BoxT IntersectWith(const BoxT& rhs) const
  {
    return {x_.IntersectWith(rhs.x_), y_.IntersectWith(rhs.y_)};
  }
  bool HasIntersectWith(const BoxT& rhs) const
  {
    return IntersectWith(rhs).IsValid();
  }
  bool HasStrictIntersectWith(const BoxT& rhs) const
  {
    return IntersectWith(rhs).IsStrictValid();
  }  // tighter
  bool Contain(const PointT& pt) const
  {
    return x_.Contain(pt.x()) && y_.Contain(pt.y());
  }
  bool StrictlyContain(const PointT& pt) const
  {
    return x_.StrictlyContain(pt.x()) && y_.StrictlyContain(pt.y());
  }
  PointT GetNearestPointTo(const PointT& pt)
  {
    return {x_.GetNearestPointTo(pt.x()), y_.GetNearestPointTo(pt.y())};
  }
  BoxT GetNearestPointsTo(const BoxT& val) const
  {
    return {x_.GetNearestPointsTo(val.x_), y_.GetNearestPointsTo(val.y_)};
  }

  void ShiftBy(const PointT& rhs)
  {
    x_.ShiftBy(rhs.x());
    y_.ShiftBy(rhs.y());
  }

  bool operator==(const BoxT& rhs) const
  {
    return (x_ == rhs.x_) && (y_ == rhs.y_);
  }
  bool operator!=(const BoxT& rhs) const { return !(*this == rhs); }

  friend std::ostream& operator<<(std::ostream& os, const BoxT& box)
  {
    os << "[x: " << box.x() << ", y: " << box.y() << "]";
    return os;
  }

 private:
  IntervalT x_;
  IntervalT y_;
};

}  // namespace grt
