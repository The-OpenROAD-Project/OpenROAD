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

  bool isValid() { return *this != PointT(); }

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
  IntervalT() { set(); }
  IntervalT(int val) { set(val); }
  IntervalT(int lo, int hi) { set(lo, hi); }

  int low() const { return low_; }
  int high() const { return high_; }

  // Setters
  void addToLow(const int increment) { low_ += increment; }
  void addToHigh(const int increment) { high_ += increment; }

  void set()
  {
    low_ = std::numeric_limits<int>::max();
    high_ = std::numeric_limits<int>::min();
  }
  void set(int val)
  {
    low_ = val;
    high_ = val;
  }
  void set(int lo, int hi)
  {
    low_ = lo;
    high_ = hi;
  }
  void setLow(int val) { low_ = val; }
  void setHigh(int val) { high_ = val; }

  // Getters
  int center() const { return (high_ + low_) / 2; }
  int range() const { return high_ - low_; }

  // Update
  // update() is always safe, fastUpdate() assumes existing values
  void update(int new_val)
  {
    low_ = std::min(new_val, low_);
    high_ = std::max(new_val, high_);
  }
  void fastUpdate(int new_val)
  {
    if (new_val < low_) {
      low_ = new_val;
    } else if (new_val > high_) {
      high_ = new_val;
    }
  }

  // Two types of intervals: 1. normal, 2. degenerated (i.e., point)
  // is valid interval (i.e., valid closed interval)
  bool isValid() const { return low_ <= high_; }
  // is strictly valid interval (excluding degenerated ones, i.e., valid open
  // interval)
  bool isStrictValid() const { return low_ < high_; }

  // Geometric Query/Update
  // interval/range of union (not union of intervals)
  IntervalT unionWith(const IntervalT& rhs) const
  {
    if (!isValid()) {
      return rhs;
    }

    if (!rhs.isValid()) {
      return *this;
    }

    return IntervalT(std::min(low_, rhs.low_), std::max(high_, rhs.high_));
  }
  // may return an invalid interval (as empty intersection)
  IntervalT intersectWith(const IntervalT& rhs) const
  {
    return IntervalT(std::max(low_, rhs.low_), std::min(high_, rhs.high_));
  }
  bool hasIntersectWith(const IntervalT& rhs) const
  {
    return intersectWith(rhs).isValid();
  }
  bool hasStrictIntersectWith(const IntervalT& rhs) const
  {
    return intersectWith(rhs).isStrictValid();
  }
  // contain a val
  bool contain(int val) const { return val >= low_ && val <= high_; }
  bool strictlyContain(int val) const { return val > low_ && val < high_; }
  // get nearest point(s) to val (assume valid intervals)
  int getNearestPointTo(int val) const
  {
    if (val <= low_) {
      return low_;
    }

    if (val >= high_) {
      return high_;
    }

    return val;
  }
  IntervalT getNearestPointsTo(IntervalT val) const
  {
    if (val.high_ <= low_) {
      return {low_};
    }

    if (val.low_ >= high_) {
      return {high_};
    }

    return intersectWith(val);
  }

  void shiftBy(const int& rhs)
  {
    low_ += rhs;
    high_ += rhs;
  }

  // Operators
  bool operator==(const IntervalT& rhs) const
  {
    return (!isValid() && !rhs.isValid())
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
  BoxT() { set(); }
  BoxT(int x_val, int y_val) { set(x_val, y_val); }
  BoxT(const PointT& pt) { set(pt); }
  BoxT(int lx, int ly, int hx, int hy) { set(lx, ly, hx, hy); }
  BoxT(const IntervalT& x_range, const IntervalT& y_range)
  {
    set(x_range, y_range);
  }
  BoxT(const PointT& low, const PointT& high) { set(low, high); }
  BoxT(const BoxT& box) { set(box); }

  IntervalT& operator[](int i)
  {
    assert(i == 0 || i == 1);
    return (i == 0) ? x_ : y_;
  }
  void set()
  {
    x_.set();
    y_.set();
  }
  void set(int x_val, int y_val)
  {
    x_.set(x_val);
    y_.set(y_val);
  }
  void set(const PointT& pt) { set(pt.x(), pt.y()); }
  void set(int lx, int ly, int hx, int hy)
  {
    x_.set(lx, hx);
    y_.set(ly, hy);
  }
  void set(const IntervalT& x_range, const IntervalT& y_range)
  {
    x_ = x_range;
    y_ = y_range;
  }
  void set(const PointT& low, const PointT& high)
  {
    set(low.x(), low.y(), high.x(), high.y());
  }
  void set(const BoxT& box) { set(box.x(), box.y()); }

  // Two types of boxes: normal & degenerated (line or point)
  // is valid box
  bool isValid() const { return x_.isValid() && y_.isValid(); }
  // is strictly valid box (excluding degenerated ones)
  bool isStrictValid() const
  {
    return x_.isStrictValid() && y_.isStrictValid();
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

  // update() is always safe, fastUpdate() assumes existing values
  void update(int x_val, int y_val)
  {
    x_.update(x_val);
    y_.update(y_val);
  }
  void fastUpdate(int x_val, int y_val)
  {
    x_.fastUpdate(x_val);
    y_.fastUpdate(y_val);
  }
  void update(const PointT& pt) { update(pt.x(), pt.y()); }
  void fastUpdate(const PointT& pt) { fastUpdate(pt.x(), pt.y()); }

  // Geometric Query/Update
  BoxT unionWith(const BoxT& rhs) const
  {
    return {x_.unionWith(rhs.x_), y_.unionWith(rhs.y_)};
  }
  BoxT intersectWith(const BoxT& rhs) const
  {
    return {x_.intersectWith(rhs.x_), y_.intersectWith(rhs.y_)};
  }
  bool hasIntersectWith(const BoxT& rhs) const
  {
    return intersectWith(rhs).isValid();
  }
  bool hasStrictIntersectWith(const BoxT& rhs) const
  {
    return intersectWith(rhs).isStrictValid();
  }  // tighter
  bool contain(const PointT& pt) const
  {
    return x_.contain(pt.x()) && y_.contain(pt.y());
  }
  bool strictlyContain(const PointT& pt) const
  {
    return x_.strictlyContain(pt.x()) && y_.strictlyContain(pt.y());
  }
  PointT getNearestPointTo(const PointT& pt)
  {
    return {x_.getNearestPointTo(pt.x()), y_.getNearestPointTo(pt.y())};
  }
  BoxT getNearestPointsTo(const BoxT& val) const
  {
    return {x_.getNearestPointsTo(val.x_), y_.getNearestPointsTo(val.y_)};
  }

  void shiftBy(const PointT& rhs)
  {
    x_.shiftBy(rhs.x());
    y_.shiftBy(rhs.y());
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
