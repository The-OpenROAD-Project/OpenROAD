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
//! CHANGES="ipoint.h 971113 mro unified unseeded, seeded ctors in RandomIPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9) with percent>0; percent >
// 1.0 will expand the BBox"

// 971113 mro unified unseeded, seeded ctors in RandomIPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9)
//             with percent>0; percent > 1.0 will expand the BBox

#ifndef _IPOINT_H_
#define _IPOINT_H_

#include <ABKCommon/abkcommon.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cfloat>

class IPoint;

//------------------------  INTERFACES START HERE  -------------------

//: Point with integer coordinates
class IPoint {
       public:
        int x;  // units are determined by the Units member of dbLayout
        int y;  // in DB

        IPoint() {};  // no initialization for high performance
        IPoint(int xx, int yy) : x(xx), y(yy) {};
        IPoint operator-(IPoint arg) const { return IPoint(x - arg.x, y - arg.y); }
        IPoint& operator-=(IPoint arg) {
                x -= arg.x, y -= arg.y;
                return *this;
        }

        IPoint operator+(const IPoint arg) const { return IPoint(x + arg.x, y + arg.y); }
        IPoint& operator+=(IPoint arg) {
                x += arg.x, y += arg.y;
                return *this;
        }

        bool operator==(const IPoint arg) const { return x == arg.x && y == arg.y; }
        // these two are rarely used and should often be superceded
        // by comparing mDist(IPoint,IPoint) to epsilon (e.g. 1e-6)
        bool operator!=(const IPoint arg) const { return x != arg.x || y != arg.y; }
        bool operator<(const IPoint arg) const {
                if (x < arg.x) return true;
                if (x > arg.x) return false;
                if (y < arg.y) return true;
                return false;
        }

        IPoint& moveBy(const IPoint& arg) {
                x += arg.x, y += arg.y;
                return *this;
        }
        IPoint& scaleBy(int xsc, int ysc) {
                x *= xsc, y *= ysc;
                return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const IPoint& arg);
        friend std::istream& operator>>(std::istream& in, IPoint& arg);
};

inline double mDist(const IPoint arg1, const IPoint arg2) { return abs(arg1.x - arg2.x) + abs(arg1.y - arg2.y); }

std::ostream& operator<<(std::ostream& out, const IPoint& arg);
std::istream& operator>>(std::istream& in, IPoint& arg);

/* ============================ IMPLEMENTATION ========================== */

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
