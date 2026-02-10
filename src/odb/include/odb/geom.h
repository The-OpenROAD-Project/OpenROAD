// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iosfwd>
#include <numbers>
#include <tuple>
#include <vector>

#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace odb {

class dbIStream;
class dbOStream;
class Rect;

class Point
{
 public:
  Point() = default;
  Point(const Point& p) = default;
  Point(int x, int y);
  ~Point() = default;
  Point& operator=(const Point& rhs) = default;

#ifndef SWIG
  auto operator<=>(const Point&) const = default;
#endif

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
  static int64_t squaredDistance(Point p0, Point p1);

  // compute the manhattan distance between two points
  static int64_t manhattanDistance(Point p0, Point p1);

  friend dbIStream& operator>>(dbIStream& stream, Point& p);
  friend dbOStream& operator<<(dbOStream& stream, const Point& p);

 protected:
  int x_ = 0;
  int y_ = 0;
};

std::ostream& operator<<(std::ostream& os, const Point& pIn);

class Point3D
{
 public:
  Point3D() = default;
  Point3D(int x, int y, int z) : x_(x), y_(y), z_(z) {}
  Point3D& operator=(const Point3D&) = default;
  Point3D(const Point3D& p) : x_(p.x()), y_(p.y()), z_(p.z()) {}
  Point3D(const Point& p, int z) : x_(p.x()), y_(p.y()), z_(z) {}

#ifndef SWIG
  auto operator<=>(const Point3D&) const = default;
#endif

  int z() const { return z_; }
  void setZ(int z) { z_ = z; }
  int x() const { return x_; }
  void setX(int x) { x_ = x; }
  int y() const { return y_; }
  void setY(int y) { y_ = y; }
  void set(const int x, const int y, const int z)
  {
    setX(x);
    setY(y);
    setZ(z);
  }

  friend dbIStream& operator>>(dbIStream& stream, Point3D& p);
  friend dbOStream& operator<<(dbOStream& stream, const Point3D& p);

 private:
  int x_ = 0;
  int y_ = 0;
  int z_ = 0;
};

std::ostream& operator<<(std::ostream& os, const Point3D& pIn);

class Cuboid
{
 public:
  Cuboid() = default;
  Cuboid(const Cuboid& other) = default;
  Cuboid(const Point3D& p1, const Point3D& p2);
  Cuboid(int x1, int y1, int z1, int x2, int y2, int z2);

  Cuboid& operator=(const Cuboid& b) = default;
#ifndef SWIG
  auto operator<=>(const Cuboid&) const = default;
#endif

  // Reinitialize the cuboid
  void init(int x1, int y1, int z1, int x2, int y2, int z2);

  // Reinitialize the cuboid without normalization
  void reset(int x1, int y1, int z1, int x2, int y2, int z2);

  // Moves the cuboid to the new point.
  void moveTo(int x, int y, int z);

  // Moves the cuboid by the offset amount
  void moveDelta(int dx, int dy, int dz);

  void mergeInit();

  // Indicates if the cuboid has a negative width, height or depth
  bool isInverted() const;

  void set_xlo(int x);
  void set_xhi(int x);
  void set_ylo(int y);
  void set_yhi(int y);
  void set_zlo(int z);
  void set_zhi(int z);

  int xMin() const { return xlo_; }
  int yMin() const { return ylo_; }
  int zMin() const { return zlo_; }
  int xMax() const { return xhi_; }
  int yMax() const { return yhi_; }
  int zMax() const { return zhi_; }
  int dx() const { return xhi_ - xlo_; }
  int dy() const { return yhi_ - ylo_; }
  int dz() const { return zhi_ - zlo_; }
  int xCenter() const { return (xlo_ + xhi_) / 2; }
  int yCenter() const { return (ylo_ + yhi_) / 2; }
  int zCenter() const { return (zlo_ + zhi_) / 2; }

  std::vector<Point3D> getPoints() const;
  Point3D lll() const;  // lower corner (xMin, yMin, zMin)
  Point3D uur() const;  // upper corner (xMax, yMax, zMax)
  Point3D center() const;

  // A point intersects any part of this cuboid.
  bool intersects(const Point3D& p) const;

  // A cuboid intersects any part of this cuboid.
  bool intersects(const Cuboid& b) const;

  // A cuboid intersects any part of this cuboid in XY plane
  bool xyIntersects(const Cuboid& b) const;

  // A point intersects the interior of this cuboid
  bool overlaps(const Point3D& p) const;

  // A cuboid intersects the interior of this cuboid
  bool overlaps(const Cuboid& b) const;

  // A cuboid intersects the interior of this cuboid in XY plane
  bool xyOverlaps(const Cuboid& b) const;

  //  A cuboid is contained in the interior of this cuboid
  bool contains(const Cuboid& b) const;

  //  A cuboid is completely contained in the interior of this cuboid,
  bool inside(const Cuboid& b) const;

  // Return the point inside cuboid that is closest to pt.
  Point3D closestPtInside(const Point3D& pt) const;

  // Compute the union of this cuboid and a point.
  void merge(const Point3D& p, Cuboid& result);

  // Compute the union of these two cuboides.
  void merge(const Cuboid& b, Cuboid& result);

  // Compute the union of this cuboid an point.
  // The result is stored in this cuboid.
  void merge(const Point3D& p);

  // Compute the union of these two cuboides. The result is stored in this
  // cuboid.
  void merge(const Cuboid& b);

  // Bloat each side of the cuboid by the margin.
  void bloat(int margin, Cuboid& result) const;

  // Compute the intersection of these two cuboides.
  Cuboid intersect(const Cuboid& b) const;

  int64_t volume() const;

  void printf(FILE* fp, const char* prefix = "");
  void print(const char* prefix = "");

 private:
  int xlo_ = 0;
  int ylo_ = 0;
  int zlo_ = 0;
  int xhi_ = 0;
  int yhi_ = 0;
  int zhi_ = 0;
};
std::ostream& operator<<(std::ostream& os, const Cuboid& cIn);

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
  Oct() = default;
  Oct(const Oct& r) = default;
  Oct(Point p1, Point p2, int width);
  Oct(int x1, int y1, int x2, int y2, int width);
  ~Oct() = default;
  Oct& operator=(const Oct& r) = default;
#ifndef SWIG
  auto operator<=>(const Oct&) const = default;
#endif
  void init(Point p1, Point p2, int width);
  OCT_DIR getDir() const;
  Point getCenterHigh() const;
  Point getCenterLow() const;
  int getWidth() const;

  int dx() const;
  int dy() const;
  int xMin() const;
  int yMin() const;
  int xMax() const;
  int yMax() const;
  std::vector<Point> getPoints() const;

  Oct bloat(int margin) const;
  Rect getEnclosingRect() const;

  friend dbIStream& operator>>(dbIStream& stream, Oct& o);
  friend dbOStream& operator<<(dbOStream& stream, const Oct& o);

 private:
  Point center_high_;  // the center of the higher octagon
  Point center_low_;   // the center of the lower octagon
  // A=W/2 (the x distance from the center to the right or left edge)
  int A_ = 0;
};

class Rect
{
 public:
  Rect() = default;
  Rect(const Rect& other) = default;
  Rect(Point p1, Point p2);
  Rect(int x1, int y1, int x2, int y2);

  Rect& operator=(const Rect& r) = default;
#ifndef SWIG
  auto operator<=>(const Rect&) const = default;
#endif

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

  int minDXDY() const;
  int maxDXDY() const;
  Orientation2D getDir() const;

  void set_xlo(int x);
  void set_xhi(int x);
  void set_ylo(int y);
  void set_yhi(int y);

  int xMin() const { return xlo_; }
  int yMin() const { return ylo_; }
  int xMax() const { return xhi_; }
  int yMax() const { return yhi_; }
  int dx() const { return xhi_ - xlo_; }
  int dy() const { return yhi_ - ylo_; }
  int xCenter() const { return (xlo_ + xhi_) / 2; }
  int yCenter() const { return (ylo_ + yhi_) / 2; }
  std::vector<Point> getPoints() const;
  Point ll() const;
  Point ul() const;
  Point ur() const;
  Point lr() const;
  Point center() const;

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

  // Compute the union of this rectangle and a point.
  void merge(const Point& p, Rect& result);

  // Compute the union of these two rectangles.
  void merge(const Rect& r, Rect& result);

  // Compute the union of this rectangle and an octagon.
  void merge(const Oct& o, Rect& result);

  // Compute the union of this rectangle an point.
  // The result is stored in this rectangle.
  void merge(const Point& p);

  // Compute the union of these two rectangles. The result is stored in this
  // rectangle.
  void merge(const Rect& r);

  // Compute the union of this rectangle an an octagon.
  // The result is stored in this rectangle.
  void merge(const Oct& o);

  // Bloat each side of the rectangle by the margin.
  void bloat(int margin, Rect& result) const;

  // Bloat the rectangle by the margin in the required orientation.
  Rect bloat(int margin, Orientation2D orient) const;

  // Compute the intersection of these two rectangles.
  void intersection(const Rect& r, Rect& result) const;

  // Compute the intersection of these two rectangles.
  Rect intersect(const Rect& r) const;

  int64_t area() const;
  int64_t margin() const;

  void printf(FILE* fp, const char* prefix = "");
  void print(const char* prefix = "");

  friend dbIStream& operator>>(dbIStream& stream, Rect& r);
  friend dbOStream& operator<<(dbOStream& stream, const Rect& r);

 private:
  int xlo_ = 0;
  int ylo_ = 0;
  int xhi_ = 0;
  int yhi_ = 0;
};

class Polygon
{
 public:
  Polygon() = default;
  Polygon& operator=(const Polygon&) = default;
  Polygon(const Polygon& r) = default;
  Polygon(const std::vector<Point>& points);
  Polygon(const Rect& rect);
  Polygon(const Oct& oct);

  std::vector<Point> getPoints() const;
  void setPoints(const std::vector<Point>& points);

#ifndef SWIG
  auto operator<=>(const Polygon&) const = default;
#endif

  bool isRect() const;
  Rect getEnclosingRect() const;
  int dx() const { return getEnclosingRect().dx(); }
  int dy() const { return getEnclosingRect().dy(); }

  // returns a corrected Polygon with a closed form and counter-clockwise points
  Polygon bloat(int margin) const;

  // Merge a collection of shapes
  static std::vector<Polygon> merge(const std::vector<Polygon>& polys);
  static std::vector<Polygon> merge(const std::vector<Rect>& rects);
  static std::vector<Polygon> merge(const std::vector<Oct>& octs);

  // Returns the geometric difference between this polygon "a" and polygon "b"
  // results in a vector of polygons.
  std::vector<Polygon> difference(Polygon b) const;

  friend dbIStream& operator>>(dbIStream& stream, Polygon& p);
  friend dbOStream& operator<<(dbOStream& stream, const Polygon& p);

 private:
  std::vector<Point> points_;
};

class Line
{
 public:
  Line() = default;
  Line(const Point& pt0, const Point& pt1);
  Line(int x0, int y0, int x1, int y1);

  Line& operator=(const Line& r) = default;
#ifndef SWIG
  auto operator<=>(const Line&) const = default;
#endif

  std::vector<Point> getPoints() const;
  Point pt0() const;
  Point pt1() const;

  void addX(int value);
  void addY(int value);

  friend dbIStream& operator>>(dbIStream& stream, Line& l);
  friend dbOStream& operator<<(dbOStream& stream, const Line& l);

 private:
  Point pt0_;
  Point pt1_;
};

std::ostream& operator<<(std::ostream& os, const Rect& box);

inline Point::Point(int x, int y)
{
  x_ = x;
  y_ = y;
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
  const int xp = -y_;
  const int yp = x_;
  x_ = xp;
  y_ = yp;
}

inline void Point::rotate180()
{
  const int xp = -x_;
  const int yp = -y_;
  x_ = xp;
  y_ = yp;
}

inline void Point::rotate270()
{
  const int xp = y_;
  const int yp = -x_;
  x_ = xp;
  y_ = yp;
}

inline int64_t Point::squaredDistance(Point p0, Point p1)
{
  const int64_t dx = p1.x_ - p0.x_;
  const int64_t dy = p1.y_ - p0.y_;
  return dx * dx + dy * dy;
}

inline int64_t Point::manhattanDistance(Point p0, Point p1)
{
  const int64_t dx = std::abs(p1.x_ - p0.x_);
  const int64_t dy = std::abs(p1.y_ - p0.y_);
  return dx + dy;
}

inline Rect::Rect(const int x1, const int y1, const int x2, const int y2)
{
  init(x1, y1, x2, y2);
}

inline Rect::Rect(const Point p1, const Point p2)
    : Rect(p1.getX(), p1.getY(), p2.getX(), p2.getY())
{
}

inline void Rect::set_xlo(int x)
{
  xlo_ = x;
}
inline void Rect::set_xhi(int x)
{
  xhi_ = x;
}
inline void Rect::set_ylo(int y)
{
  ylo_ = y;
}
inline void Rect::set_yhi(int y)
{
  yhi_ = y;
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
  std::tie(xlo_, xhi_) = std::minmax(x1, x2);
  std::tie(ylo_, yhi_) = std::minmax(y1, y2);
}

inline int Rect::minDXDY() const
{
  return std::min(dx(), dy());
}

inline int Rect::maxDXDY() const
{
  return std::max(dx(), dy());
}

inline Orientation2D Rect::getDir() const
{
  const int DX = dx();
  const int DY = dy();
  if (DX < DY) {
    return vertical;
  }
  return horizontal;
}

inline void Rect::moveTo(int x, int y)
{
  const int DX = dx();
  const int DY = dy();
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

inline Point Rect::center() const
{
  return Point(xCenter(), yCenter());
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

inline Point Rect::closestPtInside(const Point pt) const
{
  return Point(std::min(std::max(pt.getX(), xMin()), xMax()),
               std::min(std::max(pt.getY(), yMin()), yMax()));
}

inline void Rect::merge(const Point& p, Rect& result)
{
  result.xlo_ = std::min(xlo_, p.getX());
  result.ylo_ = std::min(ylo_, p.getY());
  result.xhi_ = std::max(xhi_, p.getX());
  result.yhi_ = std::max(yhi_, p.getY());
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

inline void Rect::merge(const Point& p)
{
  xlo_ = std::min(xlo_, p.getX());
  ylo_ = std::min(ylo_, p.getY());
  xhi_ = std::max(xhi_, p.getX());
  yhi_ = std::max(yhi_, p.getY());
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

inline Rect Rect::bloat(int margin, Orientation2D orient) const
{
  Rect result;
  if (orient == horizontal) {
    result.xlo_ = xlo_ - margin;
    result.xhi_ = xhi_ + margin;
    result.ylo_ = ylo_;
    result.yhi_ = yhi_;
  } else {
    result.xlo_ = xlo_;
    result.xhi_ = xhi_;
    result.ylo_ = ylo_ - margin;
    result.yhi_ = yhi_ + margin;
  }
  return result;
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

inline int64_t Rect::area() const
{
  return dx() * static_cast<int64_t>(dy());
}

inline int64_t Rect::margin() const
{
  const int64_t DX = dx();
  const int64_t DY = dy();
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
  return {ll(), lr(), ur(), ul(), ll()};
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
  if (center_low_ == center_high_) {
    return UNKNOWN;
  }
  if (center_high_.getX() > center_low_.getX()) {
    return RIGHT;
  }
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

inline int Oct::dx() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT) {
    return abs(center_high_.getX() + A_ - center_low_.getX() + A_);
  }
  if (D == LEFT) {
    return abs(center_low_.getX() + A_ - center_high_.getX() + A_);
  }
  return 0;
}

inline int Oct::dy() const
{
  return abs(center_high_.getY() + A_ - center_low_.getY() + A_);
}

inline int Oct::xMin() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT) {
    return center_low_.getX() - A_;
  }
  if (D == LEFT) {
    return center_high_.getX() - A_;
  }
  return 0;
}

inline int Oct::yMin() const
{
  return center_low_.getY() - A_;
}

inline int Oct::xMax() const
{
  OCT_DIR D = getDir();
  if (D == RIGHT) {
    return center_high_.getX() + A_;
  }
  if (D == LEFT) {
    return center_low_.getX() + A_;
  }
  return 0;
}

inline int Oct::yMax() const
{
  return center_high_.getY() + A_;
}

inline std::vector<Point> Oct::getPoints() const
{
  OCT_DIR dir = getDir();
  int B = ceil((A_ * 2) / std::numbers::sqrt2) - A_;
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

inline Oct Oct::bloat(int margin) const
{
  return Oct(center_low_, center_high_, 2 * (A_ + margin));
}

inline Rect Oct::getEnclosingRect() const
{
  return Rect(xMin(), yMin(), xMax(), yMax());
}

inline Polygon::Polygon(const std::vector<Point>& points)
{
  setPoints(points);
}

inline Polygon::Polygon(const Rect& rect)
{
  setPoints(rect.getPoints());
}

inline Polygon::Polygon(const Oct& oct)
{
  setPoints(oct.getPoints());
}

inline std::vector<Point> Polygon::getPoints() const
{
  return points_;
}

inline Rect Polygon::getEnclosingRect() const
{
  Rect rect;
  rect.mergeInit();
  for (const Point& pt : points_) {
    rect.merge(Rect(pt, pt));
  }
  return rect;
}

inline bool Polygon::isRect() const
{
  // A polygon is a rect if and only if the polygon
  // of its bounding box is equal to itself.
  return *this == Polygon(getEnclosingRect());
}

inline Line::Line(const Point& pt0, const Point& pt1) : pt0_(pt0), pt1_(pt1)
{
}

inline Line::Line(int x0, int y0, int x1, int y1)
    : pt0_(Point(x0, y0)), pt1_(Point(x1, y1))
{
}

inline Point Line::pt0() const
{
  return pt0_;
}

inline Point Line::pt1() const
{
  return pt1_;
}

inline void Line::addX(int value)
{
  pt0_.setX(pt0_.getX() + value);
  pt1_.setX(pt1_.getX() + value);
}

inline void Line::addY(int value)
{
  pt0_.setY(pt0_.getY() + value);
  pt1_.setY(pt1_.getY() + value);
}

inline std::vector<Point> Line::getPoints() const
{
  std::vector<Point> pts{pt0_, pt1_};
  return pts;
}

// Returns the manhattan distance from Point p to Rect r
inline int manhattanDistance(const Rect& r, const Point& p)
{
  const int x = p.getX();
  const int y = p.getY();
  const int dx = std::abs(x - std::clamp(x, r.xMin(), r.xMax()));
  const int dy = std::abs(y - std::clamp(y, r.yMin(), r.yMax()));
  return dx + dy;
}

inline Cuboid::Cuboid(const int x1,
                      const int y1,
                      const int z1,
                      const int x2,
                      const int y2,
                      const int z2)
{
  init(x1, y1, z1, x2, y2, z2);
}

inline Cuboid::Cuboid(const Point3D& p1, const Point3D& p2)
    : Cuboid(p1.x(), p1.y(), p1.z(), p2.x(), p2.y(), p2.z())
{
}

inline void Cuboid::set_xlo(int x)
{
  xlo_ = x;
}

inline void Cuboid::set_xhi(int x)
{
  xhi_ = x;
}

inline void Cuboid::set_ylo(int y)
{
  ylo_ = y;
}

inline void Cuboid::set_yhi(int y)
{
  yhi_ = y;
}

inline void Cuboid::set_zlo(int z)
{
  zlo_ = z;
}

inline void Cuboid::set_zhi(int z)
{
  zhi_ = z;
}

inline void Cuboid::reset(int x1, int y1, int z1, int x2, int y2, int z2)
{
  xlo_ = x1;
  xhi_ = x2;
  ylo_ = y1;
  yhi_ = y2;
  zlo_ = z1;
  zhi_ = z2;
}

inline void Cuboid::init(int x1, int y1, int z1, int x2, int y2, int z2)
{
  std::tie(xlo_, xhi_) = std::minmax(x1, x2);
  std::tie(ylo_, yhi_) = std::minmax(y1, y2);
  std::tie(zlo_, zhi_) = std::minmax(z1, z2);
}

inline void Cuboid::moveTo(int x, int y, int z)
{
  const int DX = dx();
  const int DY = dy();
  const int DZ = dz();
  xlo_ = x;
  ylo_ = y;
  zlo_ = z;
  xhi_ = x + DX;
  yhi_ = y + DY;
  zhi_ = z + DZ;
}

inline void Cuboid::moveDelta(int dx, int dy, int dz)
{
  xlo_ += dx;
  ylo_ += dy;
  zlo_ += dz;
  xhi_ += dx;
  yhi_ += dy;
  zhi_ += dz;
}

inline std::vector<Point3D> Cuboid::getPoints() const
{
  return {Point3D(xlo_, ylo_, zlo_),   // min x, min y, min z
          Point3D(xhi_, ylo_, zlo_),   // max x, min y, min z
          Point3D(xhi_, yhi_, zlo_),   // max x, max y, min z
          Point3D(xlo_, yhi_, zlo_),   // min x, max y, min z
          Point3D(xlo_, ylo_, zhi_),   // min x, min y, max z
          Point3D(xhi_, ylo_, zhi_),   // max x, min y, max z
          Point3D(xhi_, yhi_, zhi_),   // max x, max y, max z
          Point3D(xlo_, yhi_, zhi_)};  // min x, max y, max z
}

inline Point3D Cuboid::lll() const
{
  return Point3D(xlo_, ylo_, zlo_);
}

inline Point3D Cuboid::uur() const
{
  return Point3D(xhi_, yhi_, zhi_);
}

inline Point3D Cuboid::center() const
{
  return Point3D(xCenter(), yCenter(), zCenter());
}

inline bool Cuboid::intersects(const Point3D& p) const
{
  return (p.x() >= xlo_) && (p.x() <= xhi_) && (p.y() >= ylo_)
         && (p.y() <= yhi_) && (p.z() >= zlo_) && (p.z() <= zhi_);
}

inline bool Cuboid::intersects(const Cuboid& b) const
{
  return (b.xhi_ >= xlo_) && (b.xlo_ <= xhi_) && (b.yhi_ >= ylo_)
         && (b.ylo_ <= yhi_) && (b.zhi_ >= zlo_) && (b.zlo_ <= zhi_);
}

inline bool Cuboid::xyIntersects(const Cuboid& b) const
{
  return (b.xhi_ >= xlo_) && (b.xlo_ <= xhi_) && (b.yhi_ >= ylo_)
         && (b.ylo_ <= yhi_);
}

inline bool Cuboid::overlaps(const Point3D& p) const
{
  return (p.x() > xlo_) && (p.x() < xhi_) && (p.y() > ylo_) && (p.y() < yhi_)
         && (p.z() > zlo_) && (p.z() < zhi_);
}

inline bool Cuboid::overlaps(const Cuboid& b) const
{
  return (b.xhi_ > xlo_) && (b.xlo_ < xhi_) && (b.yhi_ > ylo_)
         && (b.ylo_ < yhi_) && (b.zhi_ > zlo_) && (b.zlo_ < zhi_);
}

inline bool Cuboid::xyOverlaps(const Cuboid& b) const
{
  return (b.xhi_ > xlo_) && (b.xlo_ < xhi_) && (b.yhi_ > ylo_)
         && (b.ylo_ < yhi_);
}

inline bool Cuboid::contains(const Cuboid& b) const
{
  return (xlo_ <= b.xlo_) && (ylo_ <= b.ylo_) && (zlo_ <= b.zlo_)
         && (xhi_ >= b.xhi_) && (yhi_ >= b.yhi_) && (zhi_ >= b.zhi_);
}

inline bool Cuboid::inside(const Cuboid& b) const
{
  return (xlo_ < b.xlo_) && (ylo_ < b.ylo_) && (zlo_ < b.zlo_)
         && (xhi_ > b.xhi_) && (yhi_ > b.yhi_) && (zhi_ > b.zhi_);
}

inline Point3D Cuboid::closestPtInside(const Point3D& pt) const
{
  return Point3D(std::min(std::max(pt.x(), xMin()), xMax()),
                 std::min(std::max(pt.y(), yMin()), yMax()),
                 std::min(std::max(pt.z(), zMin()), zMax()));
}

inline void Cuboid::merge(const Point3D& p, Cuboid& result)
{
  result.xlo_ = std::min(xlo_, p.x());
  result.ylo_ = std::min(ylo_, p.y());
  result.zlo_ = std::min(zlo_, p.z());
  result.xhi_ = std::max(xhi_, p.x());
  result.yhi_ = std::max(yhi_, p.y());
  result.zhi_ = std::max(zhi_, p.z());
}

inline void Cuboid::merge(const Cuboid& b, Cuboid& result)
{
  result.xlo_ = std::min(xlo_, b.xlo_);
  result.ylo_ = std::min(ylo_, b.ylo_);
  result.zlo_ = std::min(zlo_, b.zlo_);
  result.xhi_ = std::max(xhi_, b.xhi_);
  result.yhi_ = std::max(yhi_, b.yhi_);
  result.zhi_ = std::max(zhi_, b.zhi_);
}

inline void Cuboid::merge(const Point3D& p)
{
  xlo_ = std::min(xlo_, p.x());
  ylo_ = std::min(ylo_, p.y());
  zlo_ = std::min(zlo_, p.z());
  xhi_ = std::max(xhi_, p.x());
  yhi_ = std::max(yhi_, p.y());
  zhi_ = std::max(zhi_, p.z());
}

inline void Cuboid::merge(const Cuboid& b)
{
  xlo_ = std::min(xlo_, b.xlo_);
  ylo_ = std::min(ylo_, b.ylo_);
  zlo_ = std::min(zlo_, b.zlo_);
  xhi_ = std::max(xhi_, b.xhi_);
  yhi_ = std::max(yhi_, b.yhi_);
  zhi_ = std::max(zhi_, b.zhi_);
}

inline void Cuboid::bloat(int margin, Cuboid& result) const
{
  result.xlo_ = xlo_ - margin;
  result.ylo_ = ylo_ - margin;
  result.zlo_ = zlo_ - margin;
  result.xhi_ = xhi_ + margin;
  result.yhi_ = yhi_ + margin;
  result.zhi_ = zhi_ + margin;
}

inline Cuboid Cuboid::intersect(const Cuboid& b) const
{
  assert(intersects(b));
  Cuboid result;
  result.xlo_ = std::max(xlo_, b.xlo_);
  result.ylo_ = std::max(ylo_, b.ylo_);
  result.zlo_ = std::max(zlo_, b.zlo_);
  result.xhi_ = std::min(xhi_, b.xhi_);
  result.yhi_ = std::min(yhi_, b.yhi_);
  result.zhi_ = std::min(zhi_, b.zhi_);
  return result;
}

inline int64_t Cuboid::volume() const
{
  return static_cast<int64_t>(dx()) * dy() * dz();
}

inline void Cuboid::mergeInit()
{
  xlo_ = INT_MAX;
  ylo_ = INT_MAX;
  zlo_ = INT_MAX;
  xhi_ = INT_MIN;
  yhi_ = INT_MIN;
  zhi_ = INT_MIN;
}

inline bool Cuboid::isInverted() const
{
  return xlo_ > xhi_ || ylo_ > yhi_ || zlo_ > zhi_;
}

inline void Cuboid::printf(FILE* fp, const char* prefix)
{
  fprintf(fp,
          "%s%12d %12d %12d - %12d %12d %12d\n",
          prefix,
          xlo_,
          ylo_,
          zlo_,
          dx(),
          dy(),
          dz());
}

inline void Cuboid::print(const char* prefix)
{
  fprintf(stdout,
          "%s%12d %12d %12d - %12d %12d %12d\n",
          prefix,
          xlo_,
          ylo_,
          zlo_,
          dx(),
          dy(),
          dz());
}

#ifndef SWIG
using utl::format_as;
#endif

}  // namespace odb
