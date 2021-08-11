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

#include <vector>

#include "odb.h"

namespace odb {

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

class dbIStream;
class dbOStream;

class Point
{
  int _x;
  int _y;

 public:
  Point();
  Point(const Point& p);
  Point(int x, int y);
  ~Point() = default;
  Point& operator=(const Point& p);
  bool operator==(const Point& p) const;
  bool operator!=(const Point& p) const;
  bool operator<(const Point& p) const;

  int getX() const;
  int getY() const;
  void setX(int x);
  void setY(int y);

  void rotate90();
  void rotate180();
  void rotate270();

  int& x() { return _x; }
  int& y() { return _y; }
  const int& x() const { return _x; }
  const int& y() const { return _y; }

  // compute cross product of the vectors <p0,p1> and <p0,p2>
  //
  //      p2
  //      +
  //      ^
  //      |
  //      | crossProduct(p0,p1,p2) > 0
  //      |
  //      +------------>+
  //     p0            p1
  //
  //      p1
  //      +
  //      ^
  //      |
  //      | crossProduct(p0,p1,p2) < 0
  //      |
  //      +------------>+
  //     p0            p2
  //
  // Returns 0 if the vectors are colinear
  // Returns > 0 if the vectors rotate counter clockwise
  // Returns < 0 if the vectors rotate clockwise
  static int64 crossProduct(Point p0, Point p1, Point p2);

  // compute the rotation direction of the vectors <p0,p1> and <p0,p2>
  // Returns 0 if the vectors are colinear
  // Returns 1 if the vectors rotate counter clockwise
  // Returns -1 if the vectors rotate clockwise
  //
  enum Rotation
  {
    COLINEAR = 0,
    CW = -1,
    CCW = 1
  };
  static int rotation(Point p0, Point p1, Point p2);

  // compute the square distance between two points
  static uint64 squaredDistance(Point p0, Point p1);

  // compute the manhattan distance between two points
  static uint64 manhattanDistance(Point p0, Point p1);

  friend dbIStream& operator>>(dbIStream& stream, Point& p);
  friend dbOStream& operator<<(dbOStream& stream, const Point& p);
};

class GeomShape
{
 public:
  virtual uint dx() const = 0;
  virtual uint dy() const = 0;
  virtual int xMin() const = 0;
  virtual int yMin() const = 0;
  virtual int xMax() const = 0;
  virtual int yMax() const = 0;
  virtual std::vector<Point> getPoints() const = 0;
  virtual ~GeomShape() = default;
};

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
class Oct : public GeomShape
{
  Point center_high;  // the center of the higher octagon
  Point center_low;   // the center of the lower octagon
  int A;  // A=W/2 (the x distance from the center to the right or left edge)
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

  uint dx() const override
  {
    OCT_DIR D = getDir();
    if (D == RIGHT)
      return abs(center_high.getX() + A - center_low.getX() + A);
    else if (D == LEFT)
      return abs(center_low.getX() + A - center_high.getX() + A);
    else
      return 0;
  };
  uint dy() const override
  {
    return abs(center_high.getY() + A - center_low.getY() + A);
  };
  int xMin() const override
  {
    OCT_DIR D = getDir();
    if (D == RIGHT)
      return center_low.getX() - A;
    else if (D == LEFT)
      return center_high.getX() - A;
    else
      return 0;
  };
  int yMin() const override { return center_low.getY() - A; };
  int xMax() const override
  {
    OCT_DIR D = getDir();
    if (D == RIGHT)
      return center_high.getX() + A;
    else if (D == LEFT)
      return center_low.getX() + A;
    else
      return 0;
  };
  int yMax() const override { return center_high.getY() + A; };
  std::vector<Point> getPoints() const override
  {
    OCT_DIR dir = getDir();
    int B = ceil((A * 2) / (sqrt(2))) - A;
    std::vector<Point> points(9);
    points[0] = points[8] = Point(center_low.getX() - B,
                                  center_low.getY() - A);  // low oct (-B,-A)
    points[1] = Point(center_low.getX() + B,
                      center_low.getY() - A);  // low oct (B,-A)
    points[4] = Point(center_high.getX() + B,
                      center_high.getY() + A);  // high oct (B,A)
    points[5] = Point(center_high.getX() - B,
                      center_high.getY() + A);  // high oct (-B,A)
    if (dir == RIGHT) {
      points[2] = Point(center_high.getX() + A,
                        center_high.getY() - B);  // high oct (A,-B)
      points[3] = Point(center_high.getX() + A,
                        center_high.getY() + B);  // high oct (A,B)
      points[6] = Point(center_low.getX() - A,
                        center_low.getY() + B);  // low oct  (-A,B)
      points[7] = Point(center_low.getX() - A,
                        center_low.getY() - B);  // low oct (-A,-B)
    } else {
      points[2] = Point(center_low.getX() + A,
                        center_low.getY() - B);  // low oct (A,-B)
      points[3] = Point(center_low.getX() + A,
                        center_low.getY() + B);  // low oct (A,B)
      points[6] = Point(center_high.getX() - A,
                        center_high.getY() + B);  // high oct  (-A,B)
      points[7] = Point(center_high.getX() - A,
                        center_high.getY() - B);  // high oct (-A,-B)
    }
    return points;
  };
  friend dbIStream& operator>>(dbIStream& stream, Oct& o);
  friend dbOStream& operator<<(dbOStream& stream, const Oct& o);
};

class Rect : public GeomShape
{
  int _xlo;
  int _ylo;
  int _xhi;
  int _yhi;

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
  bool isInverted();

  uint minDXDY();
  uint maxDXDY();
  int getDir();

  void set_xlo(int x1);
  void set_xhi(int x1);
  void set_ylo(int x1);
  void set_yhi(int x1);

  int xMin() const override { return _xlo; };
  int yMin() const override { return _ylo; };
  int xMax() const override { return _xhi; };
  int yMax() const override { return _yhi; };
  uint dx() const override { return (uint) (_xhi - _xlo); };
  uint dy() const override { return (uint) (_yhi - _ylo); };
  std::vector<Point> getPoints() const override
  {
    std::vector<Point> points(5);
    points[0] = points[4] = ll();
    points[1] = lr();
    points[2] = ur();
    points[3] = ul();
    return points;
  };
  Point ll() const;
  Point ul() const;
  Point ur() const;
  Point lr() const;

  // Returns the lower point (lower-left)
  Point low() const;

  // Returns the upper point (upper-right)
  Point high() const;

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

  // Compute the union of these two rectangles.
  void merge(const Rect& r, Rect& result);

  void merge(GeomShape* s, Rect& result);

  // Compute the union of these two rectangles. The result is stored in this
  // rectangle.
  void merge(const Rect& r);

  void merge(GeomShape* s);

  // Compute the intersection of these two rectangles.
  void intersection(const Rect& r, Rect& result);

  // Compute the intersection of these two rectangles.
  Rect intersect(const Rect& r);

  uint64 area();
  uint64 margin();

  void notice(const char* prefix = "");
  void printf(FILE* fp, const char* prefix = "");
  void print(const char* prefix = "");

  friend dbIStream& operator>>(dbIStream& stream, Rect& r);
  friend dbOStream& operator<<(dbOStream& stream, const Rect& r);
};

inline Point::Point()
{
  _x = 0;
  _y = 0;
}

inline Point::Point(const Point& p)
{
  _x = p._x;
  _y = p._y;
}

inline Point::Point(int x, int y)
{
  _x = x;
  _y = y;
}

inline Point& Point::operator=(const Point& p)
{
  _x = p._x;
  _y = p._y;
  return *this;
}

inline bool Point::operator==(const Point& p) const
{
  return (_x == p._x) && (_y == p._y);
}

inline bool Point::operator!=(const Point& p) const
{
  return (_x != p._x) || (_y != p._y);
}

inline int Point::getX() const
{
  return _x;
}

inline int Point::getY() const
{
  return _y;
}

inline void Point::setX(int x)
{
  _x = x;
}

inline void Point::setY(int y)
{
  _y = y;
}

inline void Point::rotate90()
{
  int xp = -_y;
  int yp = _x;
  _x = xp;
  _y = yp;
}

inline void Point::rotate180()
{
  int xp = -_x;
  int yp = -_y;
  _x = xp;
  _y = yp;
}

inline void Point::rotate270()
{
  int xp = _y;
  int yp = -_x;
  _x = xp;
  _y = yp;
}

inline int64 Point::crossProduct(Point p0, Point p1, Point p2)
{
  // because the cross-product might overflow in an "int"
  // 64-bit arithmetic is used here
  int64 x0 = p0._x;
  int64 x1 = p1._x;
  int64 x2 = p2._x;
  int64 y0 = p0._y;
  int64 y1 = p1._y;
  int64 y2 = p2._y;
  return (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
}

inline int Point::rotation(Point p0, Point p1, Point p2)
{
  int64 cp = crossProduct(p0, p1, p2);
  return (cp == 0 ? 0 : cp < 0 ? -1 : 1);
}

inline uint64 Point::squaredDistance(Point p0, Point p1)
{
  int64 x0 = p0._x;
  int64 x1 = p1._x;
  int64 dx = x1 - x0;
  int64 y0 = p0._y;
  int64 y1 = p1._y;
  int64 dy = y1 - y0;
  return (uint64) (dx * dx + dy * dy);
}

inline uint64 Point::manhattanDistance(Point p0, Point p1)
{
  int64 x0 = p0._x;
  int64 x1 = p1._x;
  int64 dx = x1 - x0;
  if (dx < 0)
    dx = -dx;
  int64 y0 = p0._y;
  int64 y1 = p1._y;
  int64 dy = y1 - y0;
  if (dy < 0)
    dy = -dy;
  return (uint64) (dx + dy);
}

inline bool Point::operator<(const Point& rhs) const
{
  if (_x < rhs._x)
    return true;

  if (_x > rhs._x)
    return false;

  return _y < rhs._y;
}

inline bool Rect::operator<(const Rect& rhs) const
{
  if (_xlo < rhs._xlo)
    return true;

  if (_xlo > rhs._xlo)
    return false;

  if (_ylo < rhs._ylo)
    return true;

  if (_ylo > rhs._ylo)
    return false;

  if (_xhi < rhs._xhi)
    return true;

  if (_xhi > rhs._xhi)
    return false;

  return _yhi < rhs._yhi;
}

inline Rect::Rect()
{
  _xlo = _ylo = _xhi = _yhi = 0;
}

inline Rect::Rect(int x1, int y1, int x2, int y2)
{
  if (x1 < x2) {
    _xlo = x1;
    _xhi = x2;
  } else {
    _xlo = x2;
    _xhi = x1;
  }

  if (y1 < y2) {
    _ylo = y1;
    _yhi = y2;
  } else {
    _ylo = y2;
    _yhi = y1;
  }
}

inline Rect::Rect(const Point p1, const Point p2)
{
  int x1 = p1.getX();
  int y1 = p1.getY();
  int x2 = p2.getX();
  int y2 = p2.getY();

  if (x1 < x2) {
    _xlo = x1;
    _xhi = x2;
  } else {
    _xlo = x2;
    _xhi = x1;
  }

  if (y1 < y2) {
    _ylo = y1;
    _yhi = y2;
  } else {
    _ylo = y2;
    _yhi = y1;
  }
}

inline void Rect::set_xlo(int x1)
{
  _xlo = x1;
}
inline void Rect::set_xhi(int x2)
{
  _xhi = x2;
}
inline void Rect::set_ylo(int y1)
{
  _ylo = y1;
}
inline void Rect::set_yhi(int y2)
{
  _yhi = y2;
}
inline void Rect::reset(int x1, int y1, int x2, int y2)
{
  _xlo = x1;
  _xhi = x2;
  _ylo = y1;
  _yhi = y2;
}

inline void Rect::init(int x1, int y1, int x2, int y2)
{
  if (x1 < x2) {
    _xlo = x1;
    _xhi = x2;
  } else {
    _xlo = x2;
    _xhi = x1;
  }

  if (y1 < y2) {
    _ylo = y1;
    _yhi = y2;
  } else {
    _ylo = y2;
    _yhi = y1;
  }
}

inline bool Rect::operator==(const Rect& r) const
{
  return (_xlo == r._xlo) && (_ylo == r._ylo) && (_xhi == r._xhi)
         && (_yhi == r._yhi);
}

inline bool Rect::operator!=(const Rect& r) const
{
  return (_xlo != r._xlo) || (_ylo != r._ylo) || (_xhi != r._xhi)
         || (_yhi != r._yhi);
}

inline uint Rect::minDXDY()
{
  uint DX = dx();
  uint DY = dy();
  if (DX < DY)
    return DX;
  else
    return DY;
}
inline uint Rect::maxDXDY()
{
  uint DX = dx();
  uint DY = dy();
  if (DX > DY)
    return DX;
  else
    return DY;
}
inline int Rect::getDir()
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
  _xlo = x;
  _ylo = y;
  _xhi = x + DX;
  _yhi = y + DY;
}

inline void Rect::moveDelta(int dx, int dy)
{
  _xlo += dx;
  _ylo += dy;
  _xhi += dx;
  _yhi += dy;
}

inline Point Rect::ll() const
{
  return Point(_xlo, _ylo);
}
inline Point Rect::ul() const
{
  return Point(_xlo, _yhi);
}
inline Point Rect::ur() const
{
  return Point(_xhi, _yhi);
}
inline Point Rect::lr() const
{
  return Point(_xhi, _ylo);
}
inline Point Rect::low() const
{
  return Point(_xlo, _ylo);
}
inline Point Rect::high() const
{
  return Point(_xhi, _yhi);
}

inline bool Rect::intersects(const Point& p) const
{
  return !((p.getX() < _xlo) || (p.getX() > _xhi) || (p.getY() < _ylo)
           || (p.getY() > _yhi));
}

inline bool Rect::intersects(const Rect& r) const
{
  return !((r._xhi < _xlo) || (r._xlo > _xhi) || (r._yhi < _ylo)
           || (r._ylo > _yhi));
}

inline bool Rect::overlaps(const Point& p) const
{
  return !((p.getX() <= _xlo) || (p.getX() >= _xhi) || (p.getY() <= _ylo)
           || (p.getY() >= _yhi));
}

inline bool Rect::overlaps(const Rect& r) const
{
  return !((r._xhi <= _xlo) || (r._xlo >= _xhi) || (r._yhi <= _ylo)
           || (r._ylo >= _yhi));
}

inline bool Rect::contains(const Rect& r) const
{
  return (_xlo <= r._xlo) && (_ylo <= r._ylo) && (_xhi >= r._xhi)
         && (_yhi >= r._yhi);
}

inline bool Rect::inside(const Rect& r) const
{
  return (_xlo < r._xlo) && (_ylo < r._ylo) && (_xhi > r._xhi)
         && (_yhi > r._yhi);
}

// Compute the union of these two rectangles.
inline void Rect::merge(const Rect& r, Rect& result)
{
  result._xlo = MIN(_xlo, r._xlo);
  result._ylo = MIN(_ylo, r._ylo);
  result._xhi = MAX(_xhi, r._xhi);
  result._yhi = MAX(_yhi, r._yhi);
}
inline void Rect::merge(GeomShape* s, Rect& result)
{
  result._xlo = MIN(_xlo, s->xMin());
  result._ylo = MIN(_ylo, s->yMin());
  result._xhi = MAX(_xhi, s->xMax());
  result._yhi = MAX(_yhi, s->yMax());
}

// Compute the union of these two rectangles.
inline void Rect::merge(const Rect& r)
{
  _xlo = MIN(_xlo, r._xlo);
  _ylo = MIN(_ylo, r._ylo);
  _xhi = MAX(_xhi, r._xhi);
  _yhi = MAX(_yhi, r._yhi);
}
inline void Rect::merge(GeomShape* s)
{
  _xlo = MIN(_xlo, s->xMin());
  _ylo = MIN(_ylo, s->yMin());
  _xhi = MAX(_xhi, s->xMax());
  _yhi = MAX(_yhi, s->yMax());
}

// Compute the intersection of these two rectangles.
inline void Rect::intersection(const Rect& r, Rect& result)
{
  if (!intersects(r)) {
    result._xlo = 0;
    result._ylo = 0;
    result._xhi = 0;
    result._yhi = 0;
  } else {
    result._xlo = MAX(_xlo, r._xlo);
    result._ylo = MAX(_ylo, r._ylo);
    result._xhi = MIN(_xhi, r._xhi);
    result._yhi = MIN(_yhi, r._yhi);
  }
}

// Compute the intersection of these two rectangles.
inline Rect Rect::intersect(const Rect& r)
{
  assert(intersects(r));
  Rect result;
  result._xlo = MAX(_xlo, r._xlo);
  result._ylo = MAX(_ylo, r._ylo);
  result._xhi = MIN(_xhi, r._xhi);
  result._yhi = MIN(_yhi, r._yhi);
  return result;
}

inline uint64 Rect::area()
{
  uint64 a = dx();
  uint64 b = dy();
  return a * b;
}

inline uint64 Rect::margin()
{
  uint64 DX = dx();
  uint64 DY = dy();
  return DX + DX + DY + DY;
}

inline void Rect::mergeInit()
{
  _xlo = INT_MAX;
  _ylo = INT_MAX;
  _xhi = INT_MIN;
  _yhi = INT_MIN;
}

inline bool Rect::isInverted()
{
  return _xlo > _xhi || _ylo > _yhi;
}

inline void Rect::notice(const char*)
{
  ;  // notice(0, "%s%12d %12d - %12d %12d\n", prefix, _xlo, _ylo, dx, dy);
}
inline void Rect::printf(FILE* fp, const char* prefix)
{
  fprintf(fp, "%s%12d %12d - %12d %12d\n", prefix, _xlo, _ylo, dx(), dy());
}
inline void Rect::print(const char* prefix)
{
  fprintf(stdout, "%s%12d %12d - %12d %12d\n", prefix, _xlo, _ylo, dx(), dy());
}

inline Oct::Oct()
{
  A = 0;
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
  if (center_low != r.center_low)
    return false;
  if (center_high != r.center_high)
    return false;
  if (A != r.A)
    return false;
  return true;
}

inline void Oct::init(const Point p1, const Point p2, int width)
{
  if (p1.getY() > p2.getY()) {
    center_high = p1;
    center_low = p2;
  } else {
    center_high = p2;
    center_low = p1;
  }
  A = width / 2;
}

inline Oct::OCT_DIR Oct::getDir() const
{
  if (center_low == center_high)
    return UNKNOWN;
  if (center_high.getX() > center_low.getX())
    return RIGHT;
  return LEFT;
}

inline Point Oct::getCenterHigh() const
{
  return center_high;
}
inline Point Oct::getCenterLow() const
{
  return center_low;
}
inline int Oct::getWidth() const
{
  return A * 2;
}
}  // namespace odb
