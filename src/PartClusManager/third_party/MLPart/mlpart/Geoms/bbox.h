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
//! CHANGES="bbox.h 990318  Yu add:  getBottomLeft() getTopRight()  getHeight()
// getWidth () intersect() bloat() setBottomLeft() setTopRight() setBottomLeft()
// setTopRight()"

// CHANGES
// 990318  Yu add:  getBottomLeft() getTopRight()  getHeight() getWidth ()
//                  intersect() bloat()
//                  setBottomLeft() setTopRight()  setBottomLeft()
//                  setTopRight()

#ifndef _BBOX_H_
#define _BBOX_H_

#include "ABKCommon/abkcommon.h"
#include "ABKCommon/abklimits.h"
#include "point.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cfloat>

class BBox;
typedef BBox Rectangle;
class SmartBBox;

//------------------------  INTERFACES START HERE  -------------------

//: Bounding box
class BBox {/* Bounding box */
       public:
        double xMin;
        double yMin;
        double xMax;  // xMax should be >= xMin
        double yMax;  // yMax should be >= xMax

        BBox() : xMin(DBL_MAX), yMin(DBL_MAX), xMax(-DBL_MAX), yMax(-DBL_MAX) {};
        // This constructs an "empty" BBox (thanks, Andy)
        BBox(double minx, double miny, double maxx, double maxy);
        BBox(const Point& pt);

        const Point getBottomLeft() const;
        const Point getTopRight() const;

        double getHeight() const { return (yMax - yMin); }
        double getWidth() const { return (xMax - xMin); }

        BBox bloat(double bloat) const;
        // Bloats the BBox by bloat

        BBox& setBottomLeft(double x, double y);
        BBox& setTopRight(double x, double y);

        BBox& setBottomLeft(Point pt);
        BBox& setTopRight(Point pt);

        bool isEmpty() const { return xMin > xMax || yMin > yMax; }
        void clear() { xMin = yMin = DBL_MAX, xMax = yMax = -DBL_MAX; }  // "empty" the BBox
        double getHalfPerim() const;
        double getArea() const;
        Point getGeomCenter() const;

        bool contains(const Point& pt) const;
        bool contains(double x, double y) const;  // for speed
        bool contains(const BBox& bbx) const;
        // the 3 below use < while the 3 above use  <=
        bool hasInside(const Point& pt) const;
        bool hasInside(double x, double y) const;  // for speed
        bool hasInside(const BBox& bbx) const;
        bool intersects(const BBox& bbx) const;
        // Tells whether two BBoxes intersect. Boundaries do not count!

        BBox intersect(const BBox& r) const;
        // Calculates the intersected component of two BBoxes

        double mdistTo(Point pt) const;
        // Calculates the Manhattan distance between point and BBox

        double mdistTo(const BBox& box) const;
        // Calculates the Manhattan distance between two BBoxes.

        Point& coerce(Point& pt) const;
        // Coerces point to the BBox

        bool operator==(const BBox& box) const;  // EXACT comparison
        bool operator!=(const BBox& box) const;  // EXACT comparison
        bool operator<(const BBox& box) const;
        BBox& operator+=(const Point& pt);  // extend the BBox by one point
        BBox& add(double xx, double yy);    // extend the BBox by one point
        BBox& centerAt(Point pt);           // Move the BBox to center at the point
        BBox& TranslateBy(Point pt);
        // Move the BBox according to the coordinate of the point
        BBox& expandToInclude(const BBox& inclBox);
        BBox& ShrinkTo(double percent = 0.9);  // can be negative or > 1.0
        // Shrinks the BBox to the percent

        friend std::ostream& operator<<(std::ostream& out, const BBox& arg);
        friend std::istream& operator>>(std::istream& in, BBox& arg);
};

inline double normalizeAngle(double phi) {
        if (phi > Pi) return phi - 2 * Pi;
        if (phi <= -Pi) return phi + 2 * Pi;
        abkfatal(0, " Internal error: this should never happen\n");  // iml 7-21-97
        return 0.0;                                                  // just to satisfy compiler mro 7-19-97
}

//: BBox that maintains a "center" at certain location
class SmartBBox : public BBox {
       protected:
        Point center;
        double _yup, _ydown, _xleft, _xright;
        double _phi1, _phi3, _phi5, _phi7;
        double _area1, _area2, _area3, _area4, _area5, _area6, _area7, _totalArea;
        void computeAux();

       public:
        SmartBBox(BBox bbx) : BBox(bbx), center(bbx.getGeomCenter()) {
                computeAux();
        };
        SmartBBox(BBox bbx, Point centre) : BBox(bbx), center(centre) {
                computeAux();
        };

        Point getCenter() const { return center; }

        double getXLeft() const { return _xleft; }
        double getXRight() const { return _xright; }
        double getYUp() const { return _yup; }
        double getYDown() const { return _ydown; }

        Point& putIn(Point& pt) {
                pt.x += center.x;
                pt.y += center.y;
                return pt;
        }
        Point& takeOut(Point& pt) {
                pt.x -= center.x;
                pt.y -= center.y;
                return pt;
        }
        Point& coerce(Point& pt);

        double area(double phi);
        double invArea(double ar);
        double segLength(double phi);
        double spreadFunc(double phi) { return 2 * Pi * area(phi) / _totalArea - Pi; }
        double unSpreadFunc(double phi) {
                return invArea((phi + Pi) * _totalArea / (2 * Pi));
        };
};

/* ============================ IMPLEMENTATION ========================== */

inline const Point BBox::getBottomLeft() const {
        Point pt;
        pt.x = xMin;
        pt.y = yMin;
        return pt;
}

inline const Point BBox::getTopRight() const {
        Point pt;
        pt.x = xMax;
        pt.y = yMax;
        return pt;
}

inline BBox& BBox::setBottomLeft(double x, double y) {
        xMin = x;
        yMin = y;
        return *this;
}

inline BBox& BBox::setTopRight(double x, double y) {
        xMax = x;
        yMax = y;
        return *this;
}

inline BBox& BBox::setBottomLeft(Point pt) {
        xMin = pt.x;
        yMin = pt.y;
        return *this;
}

inline BBox& BBox::setTopRight(Point pt) {
        xMax = pt.x;
        yMax = pt.y;
        return *this;
}

inline BBox& BBox::operator+=(const Point& pt) {
        if (lessThanDouble(pt.x, xMin)) xMin = pt.x;
        if (greaterThanDouble(pt.x, xMax)) xMax = pt.x;
        if (lessThanDouble(pt.y, yMin)) yMin = pt.y;
        if (greaterThanDouble(pt.y, yMax)) yMax = pt.y;
        return *this;
}

inline BBox& BBox::add(double xx, double yy) {
        if (lessThanDouble(xx, xMin)) xMin = xx;
        if (greaterThanDouble(xx, xMax)) xMax = xx;
        if (lessThanDouble(yy, yMin)) yMin = yy;
        if (greaterThanDouble(yy, yMax)) yMax = yy;
        return *this;
}

inline BBox& BBox::centerAt(Point pt) {
        double dx = 0.5 * (xMax - xMin);
        double dy = 0.5 * (yMax - yMin);
        xMin = pt.x - dx;
        xMax = pt.x + dx;
        yMin = pt.y - dy;
        yMax = pt.y + dy;
        return *this;
}

inline BBox& BBox::TranslateBy(Point pt) {
        xMin += pt.x;
        yMin += pt.y;
        xMax += pt.x;
        yMax += pt.y;
        return *this;
}

inline BBox BBox::bloat(double bloat) const { return BBox(xMax - bloat, yMax - bloat, xMin + bloat, yMin + bloat); }

inline BBox& BBox::ShrinkTo(double percent)
    // can be negative or > 1.0
{
        double dx = 0.5 * (1 - percent) * (xMax - xMin), dy = 0.5 * (1 - percent) * (yMax - yMin);

        xMax -= dx;
        xMin += dx;

        yMax -= dy;
        yMin += dy;

        return (*this);
}

inline bool BBox::operator==(const BBox& box) const {
        // return(xMax == box.xMax && xMin == box.xMin &&
        //   yMax == box.yMax && yMin == box.yMin);
        return (equalDouble(xMax, box.xMax) && equalDouble(xMin, box.xMin) && equalDouble(yMax, box.yMax) && equalDouble(yMin, box.yMin));
}

inline bool BBox::operator!=(const BBox& box) const {
        // return(xMax != box.xMax || xMin != box.xMin ||
        //   yMax != box.yMax || yMin != box.yMin);
        return (!equalDouble(xMax, box.xMax) || !equalDouble(xMin, box.xMin) || !equalDouble(yMax, box.yMax) || !equalDouble(yMin, box.yMin));
}

inline bool BBox::operator<(const BBox& box) const { return getBottomLeft() < box.getBottomLeft() || (getBottomLeft() == box.getBottomLeft() && getTopRight() < box.getTopRight()); }

inline BBox::BBox(double minx, double miny, double maxx, double maxy) : xMin(minx), yMin(miny), xMax(maxx), yMax(maxy) {}

inline BBox::BBox(const Point& pt) : xMin(pt.x), yMin(pt.y), xMax(pt.x), yMax(pt.y) {}

inline double BBox::getHalfPerim() const {
        abkassert(!isEmpty(), "Can\'t get half-perimeter of an empty bounding box");
        return (xMax - xMin) + (yMax - yMin);
}

inline double BBox::getArea() const {
        if (isEmpty() || equalDouble(xMin, xMax) || equalDouble(yMin, yMax))
                return 0.;
        else
                return (xMax - xMin) * (yMax - yMin);
}

inline Point BBox::getGeomCenter() const {
        abkassert(!isEmpty(), "Empty bounding box does not have a center");
        return Point(0.5 * (xMin + xMax), 0.5 * (yMin + yMax));
}

inline bool BBox::contains(const Point& pt) const {
        // return xMin<=pt.x && pt.x <= xMax && yMin <= pt.y && pt.y <= yMax;
        return lessOrEqualDouble(xMin, pt.x) && lessOrEqualDouble(pt.x, xMax) && lessOrEqualDouble(yMin, pt.y) && lessOrEqualDouble(pt.y, yMax);
}

inline bool BBox::contains(double x, double y) const {
        // return xMin<=x && x <= xMax && yMin <= y && y <= yMax;
        return lessOrEqualDouble(xMin, x) && lessOrEqualDouble(x, xMax) && lessOrEqualDouble(yMin, y) && lessOrEqualDouble(y, yMax);
}

inline bool BBox::contains(const BBox& bbx) const {
        // return  bbx.xMin>=xMin && bbx.xMax<=xMax
        //     && bbx.yMin>=yMin && bbx.yMax<=yMax;
        return greaterOrEqualDouble(bbx.xMin, xMin) && lessOrEqualDouble(bbx.xMax, xMax) && greaterOrEqualDouble(bbx.yMin, yMin) && lessOrEqualDouble(bbx.yMax, yMax);
}

inline bool BBox::hasInside(const Point& pt) const {
        // return xMin < pt.x && pt.x < xMax && yMin < pt.y && pt.y < yMax;
        return lessThanDouble(xMin, pt.x) && lessThanDouble(pt.x, xMax) && lessThanDouble(yMin, pt.y) && lessThanDouble(pt.y, yMax);
}

inline bool BBox::hasInside(double x, double y) const {
        // return xMin<x && x < xMax && yMin < y && y < yMax;
        return lessThanDouble(xMin, x) && lessThanDouble(x, xMax) && lessThanDouble(yMin, y) && lessThanDouble(y, yMax);
}

inline bool BBox::hasInside(const BBox& bbx) const {
        // return  bbx.xMin>xMin && bbx.xMax<xMax
        //     && bbx.yMin>yMin && bbx.yMax<yMax;
        return greaterThanDouble(bbx.xMin, xMin) && lessThanDouble(bbx.xMax, xMax) && greaterThanDouble(bbx.yMin, yMin) && lessThanDouble(bbx.yMax, yMax);
}

inline Point& BBox::coerce(Point& pt) const {
        abkassert(!isEmpty(), "Can\'t coerce to an empty bounding box");
        if (lessThanDouble(pt.x, xMin))
                pt.x = xMin;
        else if (greaterThanDouble(pt.x, xMax))
                pt.x = xMax;
        if (lessThanDouble(pt.y, yMin))
                pt.y = yMin;
        else if (greaterThanDouble(pt.y, yMax))
                pt.y = yMax;
        return pt;
}

std::ostream& operator<<(std::ostream& out, const BBox& arg);

std::istream& operator>>(std::istream& in, BBox& arg);

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

inline bool BBox::intersects(const BBox& box) const
    /* True iff the interior regions of the two boxes share
       any points.
    */
{
        if (lessOrEqualDouble(xMax, box.xMin) || greaterOrEqualDouble(xMin, box.xMax) || lessOrEqualDouble(yMax, box.yMin) || greaterOrEqualDouble(yMin, box.yMax))
                return false;
        else
                return true;
}

inline BBox& BBox::expandToInclude(const BBox& inclBox)
    // DV added this (aaronnn fixed this)
    // Expand the current bounding box to include the BBox inclBox
{
        if (greaterThanDouble(xMin, inclBox.xMin)) xMin = inclBox.xMin;
        if (lessThanDouble(xMax, inclBox.xMax)) xMax = inclBox.xMax;
        if (greaterThanDouble(yMin, inclBox.yMin)) yMin = inclBox.yMin;
        if (lessThanDouble(yMax, inclBox.yMax)) yMax = inclBox.yMax;

        // if (xMin > inclBox.xMax) xMin = inclBox.xMax;
        // if (xMax < inclBox.xMin) xMax = inclBox.xMin;
        // if (yMin > inclBox.yMax) yMin = inclBox.yMax;
        // if (yMax < inclBox.yMin) yMax = inclBox.yMin;

        return (*this);
}

#endif
