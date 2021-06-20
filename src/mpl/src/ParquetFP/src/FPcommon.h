/**************************************************************************
***
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

#ifndef FPCOMMON
#define FPCOMMON

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace parquetfp {
enum ORIENT
{
  N,
  E,
  S,
  W,
  FN,
  FE,
  FS,
  FW
};

std::ostream& operator<<(std::ostream&, const ORIENT&);

struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

class Point
{
 public:
  float x, y;
  Point() : x(0.f), y(0.f) {}
  Point(float x_, float y_) : x(x_), y(y_) {}
  Point(const Point& orig) : x(orig.x), y(orig.y) {}
};

bool operator<(const Point& a, const Point& b);
bool operator==(const Point& a, const Point& b);

class IntPoint
{
 public:
  int x, y;
  IntPoint() : x(0), y(0) {}
  IntPoint(int x_, int y_) : x(x_), y(y_) {}
  IntPoint(const IntPoint& orig) : x(orig.x), y(orig.y) {}
};

class BBox
{
 private:
  float _minX;
  float _maxX;
  float _minY;
  float _maxY;

  bool _valid;

 public:
  // ctor
  BBox();
  BBox(float minX, float minY, float maxX, float maxY)
      : _minX(minX),
        _maxX(maxX),
        _minY(minY),
        _maxY(maxY),
        _valid(maxX >= minX && maxY >= minY)
  {
  }

  void put(const Point& point);
  void clear(void);
  float getHPWL(void) const;
  float getXSize(void) const;
  float getYSize(void) const;
  float getMinX(void) const;
  float getMinY(void) const;
  bool isValid(void) const;
};

// global parsing functions
std::istream& eatblank(std::istream& i);

std::istream& skiptoeol(std::istream& i);

std::istream& eathash(std::istream& i);

bool needCaseChar(std::istream& i, char character);

// functions to manage the orientations
ORIENT toOrient(char* orient);
const char* toChar(ORIENT orient);

// Lifted from ABKcommon

inline bool equalFloat(const float a, const float b)
{
  // standard def of equality
  if (a == b)
    return true;

  float absdiff = std::abs(a - b);
  const float fraction = 1.e-6f;

  // special case for a = 0 or b = 0
  if (((a == 0.) || (b == 0.)) && (absdiff < fraction))
    return true;

  float absa = std::abs(a);
  float absb = std::abs(b);

  // difference insignificant compared to a and b
  return ((absdiff < fraction * absa) && (absdiff < fraction * absb));
}

inline bool lessOrEqualFloat(const float a, const float b)
{
  return (a <= b) || equalFloat(a, b);
}

inline bool lessThanFloat(const float a, const float b)
{
  return (a < b) && !equalFloat(a, b);
}

#define fpwarn(CONDITION, ERRMESSAGE)         \
  {                                           \
    if (!(CONDITION)) {                       \
      std::cerr << std::endl << (ERRMESSAGE); \
    }                                         \
  }

#define fpfatal(CONDITION, ERRMESSAGE)        \
  {                                           \
    if (!(CONDITION)) {                       \
      std::cerr << std::endl << (ERRMESSAGE); \
      abort();                                \
    }                                         \
  }

}  // namespace parquetfp

#endif
