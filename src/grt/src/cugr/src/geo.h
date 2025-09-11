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
  PointT(int x = std::numeric_limits<int>::max(),
         int y = std::numeric_limits<int>::max())
      : x_(x), y_(y)
  {
  }
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
  int x_;
  int y_;
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
  IntervalT x, y;

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
    return (i == 0) ? x : y;
  }
  void Set()
  {
    x.Set();
    y.Set();
  }
  void Set(int xVal, int yVal)
  {
    x.Set(xVal);
    y.Set(yVal);
  }
  void Set(const PointT& pt) { Set(pt.x(), pt.y()); }
  void Set(int lx, int ly, int hx, int hy)
  {
    x.Set(lx, hx);
    y.Set(ly, hy);
  }
  void Set(const IntervalT& xRange, const IntervalT& yRange)
  {
    x = xRange;
    y = yRange;
  }
  void Set(const PointT& low, const PointT& high)
  {
    Set(low.x(), low.y(), high.x(), high.y());
  }
  void Set(const BoxT& box) { Set(box.x, box.y); }

  // Two types of boxes: normal & degenerated (line or point)
  // is valid box
  bool IsValid() const { return x.IsValid() && y.IsValid(); }
  // is strictly valid box (excluding degenerated ones)
  bool IsStrictValid() const
  {
    return x.IsStrictValid() && y.IsStrictValid();
  }  // tighter

  // Getters
  int lx() const { return x.low(); }
  int ly() const { return y.low(); }
  int hy() const { return y.high(); }
  int hx() const { return x.high(); }
  int cx() const { return x.center(); }
  int cy() const { return y.center(); }
  int width() const { return x.range(); }
  int height() const { return y.range(); }
  int hp() const { return width() + height(); }  // half perimeter
  int area() const { return width() * height(); }
  const IntervalT& operator[](int i) const
  {
    assert(i == 0 || i == 1);
    return (i == 0) ? x : y;
  }

  // Update() is always safe, FastUpdate() assumes existing values
  void Update(int xVal, int yVal)
  {
    x.Update(xVal);
    y.Update(yVal);
  }
  void FastUpdate(int xVal, int yVal)
  {
    x.FastUpdate(xVal);
    y.FastUpdate(yVal);
  }
  void Update(const PointT& pt) { Update(pt.x(), pt.y()); }
  void FastUpdate(const PointT& pt) { FastUpdate(pt.x(), pt.y()); }

  // Geometric Query/Update
  BoxT UnionWith(const BoxT& rhs) const
  {
    return {x.UnionWith(rhs.x), y.UnionWith(rhs.y)};
  }
  BoxT IntersectWith(const BoxT& rhs) const
  {
    return {x.IntersectWith(rhs.x), y.IntersectWith(rhs.y)};
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
    return x.Contain(pt.x()) && y.Contain(pt.y());
  }
  bool StrictlyContain(const PointT& pt) const
  {
    return x.StrictlyContain(pt.x()) && y.StrictlyContain(pt.y());
  }
  PointT GetNearestPointTo(const PointT& pt)
  {
    return {x.GetNearestPointTo(pt.x()), y.GetNearestPointTo(pt.y())};
  }
  BoxT GetNearestPointsTo(BoxT val) const
  {
    return {x.GetNearestPointsTo(val.x), y.GetNearestPointsTo(val.y)};
  }

  void ShiftBy(const PointT& rhs)
  {
    x.ShiftBy(rhs.x());
    y.ShiftBy(rhs.y());
  }

  bool operator==(const BoxT& rhs) const
  {
    return (x == rhs.x) && (y == rhs.y);
  }
  bool operator!=(const BoxT& rhs) const { return !(*this == rhs); }

  friend std::ostream& operator<<(std::ostream& os, const BoxT& box)
  {
    os << "[x: " << box.x << ", y: " << box.y << "]";
    return os;
  }
};

}  // namespace grt
