///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include <algorithm>
#include <iosfwd>
#include <vector>

#include "isotropy.h"
#include "odb.h"

namespace odb {

class dbIStream;
class dbOStream;

class Point
{
 public:
  Point();
  Point(const Point& p);
  Point(int x, int y);
  ~Point() = default;
  Point& operator=(const Point& p);
  bool operator==(const Point& p) const;
  bool operator!=(const Point& p) const;
  bool operator<(const Point& p) const;
  bool operator>=(const Point& p) const;

  int get(Orientation2D orient) const;
  int getX() const { return x_; }
  int getY() const { return y_; }
  void setX(int x) { x_ = x; }
  void setY(int y) { y_ = y; }
  void set(Orientation2D orient, int value);
  void addX(int x) { x_ += x; }
  void addY(int y) { y_ += y; }

  void rotate90();
  void rotate180();
  void rotate270();

  int x() const { return x_; }
  int y() const { return y_; }

  // compute the square distance between two points
  static uint64 squaredDistance(Point p0, Point p1);

  // compute the manhattan distance between two points
  static uint64 manhattanDistance(Point p0, Point p1);

  friend dbIStream& operator>>(dbIStream& stream, Point& p);
  friend dbOStream& operator<<(dbOStream& stream, const Point& p);

 private:
  int x_;
  int y_;
};

std::ostream& operator<<(std::ostream& os, const Point& pIn);

/*
an Oct represents a 45-degree routing segment as 2 connected octagons

DIR:RIGHT
                       ---------
                     /          \
                   /             \
                 /     high      |
               /                 |
             /                  /
           /                  /
         /                  /
       /                  /
     /                  /
   /                  /
  |                 /
  |     low       /
  \             /
   \          /
    ---------

DIR: LEFT
   ---------
  /         \
 /            \
|      high     \
|                 \
 \                  \
   \                  \
     \                  \
       \                  \
         \                  \
           \                  \
             \                 |
               \       low     |
                 \             /
                   \          /
                     ---------
each octagon follows the model:
      (-B,A) --------- (B,A)
            /         \
           /           \ (A,B)
    (-A,B)|<---width--->|
          |    center   |
          |             |
    (-A,-B)\           /(A,-B)
            \         /
      (-B,-A)---------(B,-A)

A = W/2
B = [ceiling(W/(sqrt(2) * M) ) * M] - A
where W is wire width and M is the manufacturing grid
*/
class Oct
{
 public:
  enum OCT_DIR  // The direction of the higher octagon relative to the lower
                // octagon ( / is right while  \ is left)
  {
    RIGHT,
    LEFT,
    UNKNOWN
  };
  Oct();
  Oct(const Oct& r) = default;
  Oct(const Point p1, const Point p2, int width);
  Oct(int x1, int y1, int x2, int y2, int width);
  ~Oct() = default;
  Oct& operator=(const Oct& r) = default;
  bool operator==(const Oct& r) const;
  bool operator!=(const Oct& r) const { return !(r == *this); };
  void init(const Point p1, const Point p2, int width);
  OCT_DIR getDir() const;
  Point getCenterHigh() const;
  Point getCenterLow() const;
  int getWidth() const;

  uint dx() const;
  uint dy() const;
  int xMin() const;
  int yMin() const;
  int xMax() const;
  int yMax() const;
  std::vector<Point> getPoints() const;

  friend dbIStream& operator>>(dbIStream& stream, Oct& o);
  friend dbOStream& operator<<(dbOStream& stream, const Oct& o);

 private:
  Point center_high_;  // the center of the higher octagon
  Point center_low_;   // the center of the lower octagon
  int A_;  // A=W/2 (the x distance from the center to the right or left edge)
};

class Rect
{
 public:
  Rect();
  Rect(const Rect& r) = default;
  Rect(const Point p1, const Point p2);
  Rect(int x1, int y1, int x2, int y2);
  ~Rect() = default;

  Rect& operator=(const Rect& r) = default;
  bool operator==(const Rect& r) const;
  bool operator!=(const Rect& r) const;
  bool operator<(const Rect& r) const;
  bool operator>(const Rect& r) const { return r < *this; }
  bool operator<=(const Rect& r) const { return !(*this > r); }
  bool operator>=(const Rect& r) const { return !(*this < r); }

  // Reinitialize the rectangle
  void init(int x1, int y1, int x2, int y2);

  // Reinitialize the rectangle without normalization
  void reset(int x1, int y1, int x2, int y2);

  // Moves the rectangle to the new point.
  void moveTo(int x, int y);

  // Moves the rectangle by the offset amount
  void moveDelta(int dx, int dy);

  // Set the coordinates to: min(INT_MAX, INT_MAX) max(INT_MIN, INT_MIN)
  void mergeInit();

  // Indicates if the box has a negative width or height
  bool isInverted() const;

  uint minDXDY() const;
  uint maxDXDY() const;
  int getDir() const;

  void set_xlo(int x1);
  void set_xhi(int x1);
  void set_ylo(int x1);
  void set_yhi(int x1);

  int xMin() const { return xlo_; }
  int yMin() const { return ylo_; }
  int xMax() const { return xhi_; }
  int yMax() const { return yhi_; }
  uint dx() const { return (uint) (xhi_ - xlo_); }
  uint dy() const { return (uint) (yhi_ - ylo_); }
  int xCenter() const { return (xlo_ + xhi_) / 2; }
  int yCenter() const { return (ylo_ + yhi_) / 2; }
  std::vector<Point> getPoints() const;
  Point ll() const;
  Point ul() const;
  Point ur() const;
  Point lr() const;

  // Returns the low coordinate in the orientation
  int low(Orientation2D orient) const;

  // Returns the high coordinate in the orientation
  int high(Orientation2D orient) const;

  int get(Orientation2D orient, Direction1D dir) const;
  void set(Orientation2D orient, Direction1D dir, int value);

  // A point intersects any part of this rectangle.
  bool intersects(const Point& p) const;

  // A rectangle intersects any part of this rectangle.
  bool intersects(const Rect& r) const;

  // A point intersects the interior of this rectangle
  bool overlaps(const Point& p) const;

  // A rectangle intersects the interior of this rectangle
  bool overlaps(const Rect& r) const;

  //  A rectangle is contained in the interior of this rectangle
  bool contains(const Rect& r) const;

  //  A rectangle is completely contained in the interior of this rectangle,
  bool inside(const Rect& r) const;

  // Return the point inside rect that is closest to pt.
  Point closestPtInside(Point pt) const;

  // Compute the union of these two rectangles.
  void merge(const Rect& r, Rect& result);

  // Compute the union of this rectangle and an octagon.
  void merge(const Oct& s, Rect& result);

  // Compute the union of these two rectangles. The result is stored in this
  // rectangle.
  void merge(const Rect& r);

  // Compute the union of this rectangle an an octagon.
  // The result is stored in this rectangle.
  void merge(const Oct& s);

  // Bloat each side of the rectangle by the margin.
  void bloat(int margin, Rect& result) const;

  // Compute the intersection of these two rectangles.
  void intersection(const Rect& r, Rect& result) const;

  // Compute the intersection of these two rectangles.
  Rect intersect(const Rect& r) const;

  uint64 area() const;
  uint64 margin() const;

  void notice(const char* prefix = "");
  void printf(FILE* fp, const char* prefix = "");
  void print(const char* prefix = "");

  friend dbIStream& operator>>(dbIStream& stream, Rect& r);
  friend dbOStream& operator<<(dbOStream& stream, const Rect& r);

 private:
  int xlo_;
  int ylo_;
  int xhi_;
  int yhi_;
};

std::ostream& operator<<(std::ostream& os, const Rect& box);

inline Point::Point()
{
  x_ = 0;
  y_ = 0;
}

inline Point::Point(const Point& p)
{
  x_ = p.x_;
  y_ = p.y_;
}

inline Point::Point(int x, int y)
{
  x_ = x;
  y_ = y;
}

inline Point& Point::operator=(const Point& p)
{
  x_ = p.x_;
  y_ = p.y_;
  return *this;
}

inline bool Point::operator==(const Point& p) const
{
  return (x_ == p.x_) && (y_ == p.y_);
}

inline bool Point::operator!=(const Point& p) const
{
  return (x_ != p.x_) || (y_ != p.y_);
}

inline int Point::get(Orientation2D orient) const
{
  return orient == horizontal ? x_ : y_;
}

inline void Point::set(Orientation2D orient, int value)
{
  if (orient == horizontal) {
    x_ = value;
  } else {
    y_ = value;
  }
}

inline void Point::rotate90()
{
  int xp = -y_;
  int yp = x_;
  x_ = xp;
  y_ = yp;
}

inline void Point::rotate180()
{
  int xp = -x_;
  int yp = -y_;
  x_ = xp;
  y_ = yp;
}

inline void Point::rotate270()
{
  int xp = y_;
  int yp = -x_;
  x_ = xp;
  y_ = yp;
}

inline uint64 Point::squaredDistance(Point p0, Point p1)
{
  int64 x0 = p0.x_;
  int64 x1 = p1.x_;
  int64 dx = x1 - x0;
  int64 y0 = p0.y_;
  int64 y1 = p1.y_;
  int64 dy = y1 - y0;
  return (uint64) (dx * dx + dy * dy);
}

inline uint64 Point::manhattanDistance(Point p0, Point p1)
{
  int64 x0 = p0.x_;
  int64 x1 = p1.x_;
  int64 dx = x1 - x0;
  if (dx < 0)
    dx = -dx;
  int64 y0 = p0.y_;
  int64 y1 = p1.y_;
  int64 dy = y1 - y0;
  if (dy < 0)
    dy = -dy;
  return (uint64) (dx + dy);
}

inline bool Point::operator<(const Point& rhs) const
{
  if (x_ < rhs.x_)
    return true;

  if (x_ > rhs.x_)
    return false;

  return y_ < rhs.y_;
}

inline bool Point::operator>=(const Point& rhs) const
{
  return !(*this < rhs);
}

inline bool Rect::operator<(const Rect& rhs) const
{
  if (xlo_ < rhs.xlo_)
    return true;

  if (xlo_ > rhs.xlo_)
    return false;

  if (ylo_ < rhs.ylo_)
    return true;

  if (ylo_ > rhs.ylo_)
    return false;

  if (xhi_ < rhs.xhi_)
    return true;

  if (xhi_ > rhs.xhi_)
    return false;

  return yhi_ < rhs.yhi_;
}

inline Rect::Rect()
{
  xlo_ = ylo_ = xhi_ = yhi_ = 0;
}

inline Rect::Rect(int x1, int y1, int x2, int y2)
{
  if (x1 < x2) {
    xlo_ = x1;
    xhi_ = x2;
  } else {
    xlo_ = x2;
    xhi_ = x1;
  }

  if (y1 < y2) {
    ylo_ = y1;
    yhi_ = y2;
  } else {
    ylo_ = y2;
    yhi_ = y1;
  }
}

inline Rect::Rect(const Point p1, const Point p2)
{
  int x1 = p1.getX();
  int y1 = p1.getY();
  int x2 = p2.getX();
  int y2 = p2.getY();

  if (x1 < x2) {
    xlo_ = x1;
    xhi_ = x2;
  } else {
    xlo_ = x2;
    xhi_ = x1;
  }

  if (y1 < y2) {
    ylo_ = y1;
    yhi_ = y2;
  } else {
    ylo_ = y2;
    yhi_ = y1;
  }
}

inline void Rect::set_xlo(int x1)
{
  xlo_ = x1;
}
inline void Rect::set_xhi(int x2)
{
  xhi_ = x2;
}
inline void Rect::set_ylo(int y1)
{
  ylo_ = y1;
}
inline void Rect::set_yhi(int y2)
{
  yhi_ = y2;
}
inline void Rect::reset(int x1, int y1, int x2, int y2)
{
  xlo_ = x1;
  xhi_ = x2;
  ylo_ = y1;
  yhi_ = y2;
}

inline void Rect::init(int x1, int y1, int x2, int y2)
{
  if (x1 < x2) {
    xlo_ = x1;
    xhi_ = x2;
  } else {
    xlo_ = x2;
    xhi_ = x1;
  }

  if (y1 < y2) {
    ylo_ = y1;
    yhi_ = y2;
  } else {
    ylo_ = y2;
    yhi_ = y1;
  }
}

inline bool Rect::operator==(const Rect& r) const
{
  return (xlo_ == r.xlo_) && (ylo_ == r.ylo_) && (xhi_ == r.xhi_)
         && (yhi_ == r.yhi_);
}

inline bool Rect::operator!=(const Rect& r) const
{
  return (xlo_ != r.xlo_) || (ylo_ != r.ylo_) || (xhi_ != r.xhi_)
         || (yhi_ != r.yhi_);
}

inline uint Rect::minDXDY() const
{
  uint DX = dx();
  uint DY = dy();
  if (DX < DY)
    return DX;
  else
    return DY;
}
inline uint Rect::maxDXDY() const
{
  uint DX = dx();
  uint DY = dy();
  if (DX > DY)
    return DX;
  else
    return DY;
}
inline int Rect::getDir() const
{
  uint DX = dx();
  uint DY = dy();
  if (DX < DY)
    return 0;
  else if (DX > DY)
    return 1;
  else
    return -1;
}
inline void Rect::moveTo(int x, int y)
{
  uint DX = dx();
  uint DY = dy();
  xlo_ = x;
  ylo_ = y;
  xhi_ = x + DX;
  yhi_ = y + DY;
}

inline void Rect::moveDelta(int dx, int dy)
{
  xlo_ += dx;
  ylo_ += dy;
  xhi_ += dx;
  yhi_ += dy;
}

inline Point Rect::ll() const
{
  return Point(xlo_, ylo_);
}
inline Point Rect::ul() const
{
  return Point(xlo_, yhi_);
}
inline Point Rect::ur() const
{
  return Point(xhi_, yhi_);
}
inline Point Rect::lr() const
{
  return Point(xhi_, ylo_);
}
inline void Rect::set(Orientation2D orient, Direction1D dir, int value)
{
  if (dir == odb::low) {
    if (orient == horizontal) {
      set_xlo(value);
    } else {
      set_ylo(value);
    }
  } else {
    if (orient == horizontal) {
      set_xhi(value);
    } else {
      set_yhi(value);
    }
  }
}
inline int Rect::get(Orientation2D orient, Direction1D dir) const
{
  return dir == odb::low ? low(orient) : high(orient);
}
inline int Rect::low(Orientation2D orient) const
{
  return orient == horizontal ? xlo_ : ylo_;
}
inline int Rect::high(Orientation2D orient) const
{
  return orient == horizontal ? xhi_ : yhi_;
}

inline bool Rect::intersects(const Point& p) const
{
  return (p.getX() >= xlo_) && (p.getX() <= xhi_) && (p.getY() >= ylo_)
         && (p.getY() <= yhi_);
}

inline bool Rect::intersects(const Rect& r) const
{
  return (r.xhi_ >= xlo_) && (r.xlo_ <= xhi_) && (r.yhi_ >= ylo_)
         && (r.ylo_ <= yhi_);
}

inline bool Rect::overlaps(const Point& p) const
{
  return (p.getX() > xlo_) && (p.getX() < xhi_) && (p.getY() > ylo_)
         && (p.getY() < yhi_);
}

inline bool Rect::overlaps(const Rect& r) const
{
  return (r.xhi_ > xlo_) && (r.xlo_ < xhi_) && (r.yhi_ > ylo_)
         && (r.ylo_ < yhi_);
}

inline bool Rect::contains(const Rect& r) const
{
  return (xlo_ <= r.xlo_) && (ylo_ <= r.ylo_) && (xhi_ >= r.xhi_)
         && (yhi_ >= r.yhi_);
}

inline bool Rect::inside(const Rect& r) const
{
  return (xlo_ < r.xlo_) && (ylo_ < r.ylo_) && (xhi_ > r.xhi_)
         && (yhi_ > r.yhi_);
}

inline Point Rect::closestPtInside(Point pt) const
{
  return Point(std::min(std::max(pt.getX(), xMin()), xMax()),
               std::min(std::max(pt.getY(), yMin()), yMax()));
}

// Compute the union of these two rectangles.
inline void Rect::merge(const Rect& r, Rect& result)
{
  result.xlo_ = std::min(xlo_, r.xlo_);
  result.ylo_ = std::min(ylo_, r.ylo_);
  result.xhi_ = std::max(xhi_, r.xhi_);
  result.yhi_ = std::max(yhi_, r.yhi_);
}

inline void Rect::merge(const Oct& o, Rect& result)
{
  result.xlo_ = std::min(xlo_, o.xMin());
  result.ylo_ = std::min(ylo_, o.yMin());
  result.xhi_ = std::max(xhi_, o.xMax());
  result.yhi_ = std::max(yhi_, o.yMax());
}

// Compute the union of these two rectangles.
inline void Rect::merge(const Rect& r)
{
  xlo_ = std::min(xlo_, r.xlo_);
  ylo_ = std::min(ylo_, r.ylo_);
  xhi_ = std::max(xhi_, r.xhi_);
  yhi_ = std::max(yhi_, r.yhi_);
}

inline void Rect::merge(const Oct& o)
{
  xlo_ = std::min(xlo_, o.xMin());
  ylo_ = std::min(ylo_, o.yMin());
  xhi_ = std::max(xhi_, o.xMax());
  yhi_ = std::max(yhi_, o.yMax());
}

// Bloat each side of the rectangle by the margin.
inline void Rect::bloat(int margin, Rect& result) const
{
  result.xlo_ = xlo_ - margin;
  result.ylo_ = ylo_ - margin;
  result.xhi_ = xhi_ + margin;
  result.yhi_ = yhi_ + margin;
}

// Compute the intersection of these two rectangles.
inline void Rect::intersection(const Rect& r, Rect& result) const
{
  if (!intersects(r)) {
    result.xlo_ = 0;
    result.ylo_ = 0;
    result.xhi_ = 0;
    result.yhi_ = 0;
  } else {
    result.xlo_ = std::max(xlo_, r.xlo_);
    result.ylo_ = std::max(ylo_, r.ylo_);
    result.xhi_ = std::min(xhi_, r.xhi_);
    result.yhi_ = std::min(yhi_, r.yhi_);
  }
}

// Compute the intersection of these two rectangles.
inline Rect Rect::intersect(const Rect& r) const
{
  assert(intersects(r));
  Rect result;
  result.xlo_ = std::max(xlo_, r.xlo_);
  result.ylo_ = std::max(ylo_, r.ylo_);
  result.xhi_ = std::min(xhi_, r.xhi_);
  result.yhi_ = std::min(yhi_, r.yhi_);
  return result;
}

inline uint64 Rect::area() const
{
  uint64 a = dx();
  uint64 b = dy();
  return a * b;
}

inline uint64 Rect::margin() const
{
  uint64 DX = dx();
  uint64 DY = dy();
  return DX + DX + DY + DY;
}

inline void Rect::mergeInit()
{
  xlo_ = INT_MAX;
  ylo_ = INT_MAX;
  xhi_ = INT_MIN;
  yhi_ = INT_MIN;
}

inline bool Rect::isInverted() const
{
  return xlo_ > xhi_ || ylo_ > yhi_;
}

inline void Rect::notice(const char*)
{
  ;  // notice(0, "%s%12d %12d - %12d %12d\n", prefix, xlo_, ylo_, dx, dy);
}
inline void Rect::printf(FILE* fp, const char* prefix)
{
  fprintf(fp, "%s%12d %12d - %12d %12d\n", prefix, xlo_, ylo_, dx(), dy());
}
inline void Rect::print(const char* prefix)
{
  fprintf(stdout, "%s%12d %12d - %12d %12d\n", prefix, xlo_, ylo_, dx(), dy());
}

inline std::vector<Point> Rect::getPoints() const
{
  std::vector<Point> points(5);
  points[0] = points[4] = ll();
  points[1] = lr();
  points[2] = ur();
  points[3] = ul();
  return points;
}

inline Oct::Oct()
{
  A_ = 0;
}

inline Oct::Oct(const Point p1, const Point p2, int width)
{
  init(p1, p2, width);
}

inline Oct::Oct(int x1, int y1, int x2, int y2, int width)
{
  Point p1(x1, y1);
  Point p2(x2, y2);
  init(p1, p2, width);
}

inline bool Oct::operator==(const Oct& r) const
{
  if (center_low_ != r.center_low_)
    return false;
  if (center_high_ != r.center_high_)
    return false;
  if (A_ != r.A_)
    return false;
  return true;
}

inline void Oct::init(const Point p1, const Point p2, int width)
{
  if (p1.getY() > p2.getY()) {
    center_high_ = p1;
    center_low_ = p2;
  } else {
    center_high_ = p2;
    center_low_ = p1;
  }
  A_ = width / 2;
}

inline Oct::OCT_DIR Oct::getDir() const
{
  if (center_low_ == center_high_)
    return UNKNOWN;
  if (center_high_.getX() > center_low_.getX())
    return RIGHT;
  return LEFT;
}

inline Point Oct::getCenterHigh() const
{
  return center_high_;
}
inline Point Oct::getCenterLow() const
{
  return center_low_;
}
inline int Oct::getWidth() const
{
  return A_ * 2;
}

inline uint Oct::dx() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT)
    return abs(center_high_.getX() + A_ - center_low_.getX() + A_);
  else if (D == LEFT)
    return abs(center_low_.getX() + A_ - center_high_.getX() + A_);
  else
    return 0;
}

inline uint Oct::dy() const
{
  return abs(center_high_.getY() + A_ - center_low_.getY() + A_);
}

inline int Oct::xMin() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT)
    return center_low_.getX() - A_;
  else if (D == LEFT)
    return center_high_.getX() - A_;
  else
    return 0;
}

inline int Oct::yMin() const
{
  return center_low_.getY() - A_;
}

inline int Oct::xMax() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT)
    return center_high_.getX() + A_;
  else if (D == LEFT)
    return center_low_.getX() + A_;
  else
    return 0;
}

inline int Oct::yMax() const
{
  return center_high_.getY() + A_;
}

inline std::vector<Point> Oct::getPoints() const
{
  OCT_DIR dir = getDir();
  int B = ceil((A_ * 2) / (sqrt(2))) - A_;
  std::vector<Point> points(9);
  points[0] = points[8] = Point(center_low_.getX() - B,
                                center_low_.getY() - A_);  // low oct (-B,-A)
  points[1] = Point(center_low_.getX() + B,
                    center_low_.getY() - A_);  // low oct (B,-A)
  points[4] = Point(center_high_.getX() + B,
                    center_high_.getY() + A_);  // high oct (B,A)
  points[5] = Point(center_high_.getX() - B,
                    center_high_.getY() + A_);  // high oct (-B,A)
  if (dir == RIGHT) {
    points[2] = Point(center_high_.getX() + A_,
                      center_high_.getY() - B);  // high oct (A,-B)
    points[3] = Point(center_high_.getX() + A_,
                      center_high_.getY() + B);  // high oct (A,B)
    points[6] = Point(center_low_.getX() - A_,
                      center_low_.getY() + B);  // low oct  (-A,B)
    points[7] = Point(center_low_.getX() - A_,
                      center_low_.getY() - B);  // low oct (-A,-B)
  } else {
    points[2] = Point(center_low_.getX() + A_,
                      center_low_.getY() - B);  // low oct (A,-B)
    points[3] = Point(center_low_.getX() + A_,
                      center_low_.getY() + B);  // low oct (A,B)
    points[6] = Point(center_high_.getX() - A_,
                      center_high_.getY() + B);  // high oct  (-A,B)
    points[7] = Point(center_high_.getX() - A_,
                      center_high_.getY() - B);  // high oct (-A,-B)
  }
  return points;
}

}  // namespace odb
