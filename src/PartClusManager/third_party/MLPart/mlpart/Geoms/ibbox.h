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

//! author="Igor Markov, April 15, 1997"

#ifndef _IBBOX_H_
#define _IBBOX_H_

#include "ipoint.h"
#include "ABKCommon/abkcommon.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cfloat>

class IBBox;
typedef IBBox IRectangle;

//------------------------  INTERFACES START HERE  -------------------

//: Bounding box with integer coordinates
class IBBox {/* Bounding box */
       public:
        int xMin;
        int yMin;
        int xMax;  // xMax should be >= xMin
        int yMax;  // yMax should be >= xMax

        IBBox() : xMin(INT_MAX), yMin(INT_MAX), xMax(-INT_MAX), yMax(-INT_MAX) {};
        // This constructs an "empty" IBBox (thanks, Andy)
        IBBox(int minx, int miny, int maxx, int maxy);
        IBBox(const IPoint& pt);
        bool isEmpty() const { return xMin > xMax || yMin > yMax; }
        void clear() { xMin = yMin = INT_MAX, xMax = yMax = -INT_MAX; }  //"empty" the IBBox
        int getHalfPerim() const;
        int getArea() const;
        IPoint getGeomCenter() const;

        bool contains(const IPoint& pt) const;
        bool contains(int x, int y) const;  // for speed
        bool contains(const IBBox& bbx) const;
        // the 3 below use < while the 3 above use  <=
        bool hasInside(const IPoint& pt) const;
        bool hasInside(int x, int y) const;  // for speed
        bool hasInside(const IBBox& bbx) const;
        bool intersects(const IBBox& bbx) const;  // boundaries do not count!

        int mdistTo(IPoint pt) const;
        int mdistTo(const IBBox& box) const;
        IPoint& coerce(IPoint& pt) const;
        IBBox& operator+=(const IPoint& pt);  // extend the IBBox by one point
        IBBox& add(int xx, int yy);           // extend the IBBox by one point
        IBBox& centerAt(IPoint pt);
        IBBox& TranslateBy(IPoint pt);
        IBBox& ShrinkTo(double percent = 0.9);  // can be negative; percent <1.0

        friend std::ostream& operator<<(std::ostream& out, const IBBox& arg);
        friend std::istream& operator>>(std::istream& in, IBBox& arg);
};

/* ============================ IMPLEMENTATION ========================== */

inline IBBox& IBBox::operator+=(const IPoint& pt) {
        if (pt.x < xMin) xMin = pt.x;
        if (pt.x > xMax) xMax = pt.x;
        if (pt.y < yMin) yMin = pt.y;
        if (pt.y > yMax) yMax = pt.y;
        return *this;
}

inline IBBox& IBBox::add(int xx, int yy) {
        if (xx < xMin) xMin = xx;
        if (xx > xMax) xMax = xx;
        if (yy < yMin) yMin = yy;
        if (yy > yMax) yMax = yy;
        return *this;
}

inline IBBox& IBBox::centerAt(IPoint pt) {
        int dx = static_cast<int>(floor(0.5 * (xMax - xMin)));
        int dy = static_cast<int>(floor(0.5 * (yMax - yMin)));
        xMin = pt.x - dx;
        xMax = pt.x + dx;
        yMin = pt.y - dy;
        yMax = pt.y + dy;
        return *this;
}

inline IBBox& IBBox::TranslateBy(IPoint pt) {
        xMin += pt.x;
        yMin += pt.y;
        xMax += pt.x;
        yMax += pt.y;
        return *this;
}

inline IBBox& IBBox::ShrinkTo(double percent)
    // can be negative; percent < 1.0
{
        xMax = static_cast<int>(floor(percent * xMax + (1 - percent) * xMin));
        xMin = static_cast<int>(floor(percent * xMin + (1 - percent) * xMax));
        yMax = static_cast<int>(floor(percent * yMax + (1 - percent) * yMin));
        yMin = static_cast<int>(floor(percent * yMin + (1 - percent) * yMax));
        return (*this);
}

inline IBBox::IBBox(int minx, int miny, int maxx, int maxy) : xMin(minx), yMin(miny), xMax(maxx), yMax(maxy) {}

inline IBBox::IBBox(const IPoint& pt) : xMin(pt.x), yMin(pt.y), xMax(pt.x), yMax(pt.y) {}

inline int IBBox::getHalfPerim() const {
        abkassert(!isEmpty(), "Can\'t get half-perimeter of an empty bounding box");
        return abs(xMax - xMin) + abs(yMax - yMin);
}

inline int IBBox::getArea() const {
        if (isEmpty()) return 0;
        return abs(xMax - xMin) * abs(yMax - yMin);
}

inline IPoint IBBox::getGeomCenter() const {
        abkassert(!isEmpty(), "Empty bounding box does not have a center");
        return IPoint((xMin + xMax) / 2, (yMin + yMax) / 2);
}

inline bool IBBox::contains(const IPoint& pt) const { return xMin <= pt.x && pt.x <= xMax && yMin <= pt.y && pt.y <= yMax; }

inline bool IBBox::contains(int x, int y) const { return xMin <= x && x <= xMax && yMin <= y && y <= yMax; }

inline bool IBBox::contains(const IBBox& bbx) const { return bbx.xMin >= xMin && bbx.xMax <= xMax && bbx.yMin >= yMin && bbx.yMax <= yMax; }

inline bool IBBox::hasInside(const IPoint& pt) const { return xMin < pt.x && pt.x < xMax && yMin < pt.y && pt.y < yMax; }

inline bool IBBox::hasInside(int x, int y) const { return xMin < x && x < xMax && yMin < y && y < yMax; }

inline bool IBBox::hasInside(const IBBox& bbx) const { return bbx.xMin > xMin && bbx.xMax < xMax && bbx.yMin > yMin && bbx.yMax < yMax; }

inline IPoint& IBBox::coerce(IPoint& pt) const {
        abkassert(!isEmpty(), "Can\'t coerce to an empty bounding box");
        if (pt.x < xMin)
                pt.x = xMin;
        else if (pt.x > xMax)
                pt.x = xMax;
        if (pt.y < yMin)
                pt.y = yMin;
        else if (pt.y > yMax)
                pt.y = yMax;
        return pt;
}

std::ostream& operator<<(std::ostream& out, const IBBox& arg);

std::istream& operator>>(std::istream& in, IBBox& arg);

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

inline bool IBBox::intersects(const IBBox& box) const
    /* True iff the interior regions of the two boxes share
       any points.
    */
{
        if ((xMax <= box.xMin) || (xMin >= box.xMax) || (yMax <= box.yMin) || (yMin >= box.yMax))
                return false;
        else
                return true;
}

#endif
