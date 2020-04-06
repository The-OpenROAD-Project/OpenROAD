/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
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

//! author=" Igor Markov, July 15, 1997"
//! CHANGES ="point.h 971113 mro unified unseeded, seeded ctors in RandomPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9) with percent>0; percent >
// 1.0 will expand the BBox"
//! CONTACTS="Igor ABK"
// 971113 mro unified unseeded, seeded ctors in RandomPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9)
//             with percent>0; percent > 1.0 will expand the BBox

#ifndef _POINT_H_
#define _POINT_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <iostream>
#include <iomanip>

class Point;
class BBox;
typedef BBox Rectangle;
class RandomPoint;  //  in a given BBox
class SmartBBox;

//------------------------  INTERFACES START HERE  -------------------

//: Point with double coordinates
class Point {
       public:
        double x;  // units are determined by the Units member of dbLayout
        double y;  // in DB

        Point() {};  // no initialization for high performance
        Point(double xx, double yy) : x(xx), y(yy) {};
        Point operator-(const Point& arg) const { return Point(x - arg.x, y - arg.y); }
        Point& operator-=(const Point& arg) {
                x -= arg.x, y -= arg.y;
                return *this;
        }

        Point operator+(const Point& arg) const { return Point(x + arg.x, y + arg.y); }
        Point& operator+=(const Point& arg) {
                x += arg.x, y += arg.y;
                return *this;
        }

        bool operator==(const Point& arg) const { return x == arg.x && y == arg.y; }
        // these two are rarely used and should often be superceded
        // by comparing mDist(Point,Point) to epsilon (e.g. 1e-6)
        bool operator!=(const Point& arg) const { return x != arg.x || y != arg.y; }
        bool operator<(const Point& arg) const {
                if (x < arg.x) return true;
                if (x > arg.x) return false;
                if (y < arg.y) return true;
                return false;
        }

        Point& moveBy(const Point& arg) {
                x += arg.x, y += arg.y;
                return *this;
        }
        Point& scaleBy(double xsc, double ysc) {
                x *= xsc, y *= ysc;
                return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const Point& arg);
        friend std::istream& operator>>(std::istream& in, Point& arg);
};

inline double mDist(const Point arg1, const Point arg2) { return fabs(arg1.x - arg2.x) + fabs(arg1.y - arg2.y); }

std::ostream& operator<<(std::ostream& out, const Point& arg);
std::istream& operator>>(std::istream& in, Point& arg);

/* ============================ IMPLEMENTATION ========================== */

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
