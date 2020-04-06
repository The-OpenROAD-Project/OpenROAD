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

// Created, June 1997, Igor Markov, VLSICAD  ABKGroup UCLA/CS

// Last revision::    Igor Markov, July 5, 1997
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <iostream>
#include <iomanip>
#include <limits.h>
#include <math.h>
#include "ABKCommon/abkcommon.h"
#include "bbox.h"
using std::ostream;
using std::istream;
using std::endl;
using std::max;
using std::min;

class BBox;
class SmartBBox;

/* ============================ IMPLEMENTATIONS ========================== */

ostream& operator<<(ostream& out, const BBox& arg) {
        if (arg.isEmpty()) out << "Empty BBox: ";
        out << arg.xMin << "  " << arg.yMin << "   " << arg.xMax << "  " << arg.yMax;
        return out;
}

istream& operator>>(istream& in, BBox& arg) {
        in >> my_isnumber() >> arg.xMin >> my_isnumber() >> arg.yMin >> my_isnumber() >> arg.xMax >> my_isnumber() >> arg.yMax;
        return in;
}

double BBox::mdistTo(Point pt) const {
        abkassert(!isEmpty(), "Can\'t compute distance to an empty bounding box");
        if (pt.x < xMin)
                pt.x = xMin - pt.x;
        else if (pt.x > xMax)
                pt.x -= xMax;
        else
                pt.x = 0;
        if (pt.y < yMin)
                pt.y = yMin - pt.y;
        else if (pt.y > yMax)
                pt.y -= yMax;
        else
                pt.y = 0;
        return pt.x + pt.y;
}
double BBox::mdistTo(const BBox& box) const {
        abkassert(!isEmpty(), "Can\'t compute distance to an empty bounding box");
        abkassert(!box.isEmpty(), "Can\'t compute distance to an empty bounding box");

        double dx, dy;

        if ((dx = box.xMin - xMax) < 0)
                if ((dx = xMin - box.xMax) < 0) dx = 0;
        if ((dy = box.yMin - yMax) < 0)
                if ((dy = yMin - box.yMax) < 0) dy = 0;

        return dx + dy;
}

BBox BBox::intersect(const BBox& r) const {
        double x1 = max(xMin, r.xMin);
        double y1 = max(yMin, r.yMin);
        double x2 = min(xMax, r.xMax);
        double y2 = min(yMax, r.yMax);

        // prevent the upper right being below lower left
        // (which is possible if the intersection is empty.)
        x2 = max(x1, x2);
        y2 = max(y1, y2);

        return BBox(x1, y1, x2, y2);
}

Point& SmartBBox::coerce(Point& pt) {
        abkassert(!isEmpty(), "Can\'t coerce to an empty SmartBBox");
        if (contains(pt)) return pt;
        double dx = pt.x - center.x;
        double dy = pt.y - center.y;
        double dist = sqrt(square(dx) + square(dy));
        double segLen = segLength(atan2(dx, dy));
        double tau = segLen / dist;
        pt.x = center.x + tau * dx;
        pt.y = center.y + tau * dy;
        return pt;
}

double SmartBBox::area(double phi) {
        if (phi < _phi1) return 0.5 * square(_ydown) * tan(phi + Pi);  // == tan(phi), I know ;-)
        if (phi < -Pi / 2) return _area2 - 0.5 * square(_xleft) * tan(-Pi / 2 - phi);
        if (phi < _phi3) return _area2 + 0.5 * square(_xleft) * tan(phi + Pi / 2);
        if (phi < 0) return _area4 - 0.5 * square(_yup) * tan(-phi);
        if (phi < _phi5) return _area4 + 0.5 * square(_yup) * tan(phi);
        if (phi < Pi / 2) return _area6 - 0.5 * square(_xright) * tan(Pi / 2 - phi);
        if (phi < _phi7) return _area6 + 0.5 * square(_xright) * tan(phi - Pi / 2);
        return _totalArea - 0.5 * square(_ydown) * tan(Pi - phi);
}

double SmartBBox::invArea(double ar) {
        if (ar < _area1) return -Pi + atan(2 * ar / square(_ydown));
        if (ar < _area2) return -Pi / 2 - atan(2 * (_area2 - ar) / square(_xleft));
        if (ar < _area3) return -Pi / 2 + atan(2 * (ar - _area2) / square(_xleft));
        if (ar < _area4) return -atan(2 * (_area4 - ar) / square(_yup));
        if (ar < _area5) return atan(2 * (ar - _area4) / square(_yup));
        if (ar < _area6) return Pi / 2 - atan(2 * (_area6 - ar) / square(_xright));
        if (ar < _area7) return Pi / 2 + atan(2 * (ar - _area6) / square(_xright));
        return Pi - atan(2 * (_totalArea - ar) / square(_ydown));
}

void SmartBBox::computeAux() {
        abkassert(!isEmpty(), "Can\'t have empty SmartBBox");
        _xleft = center.x - xMin;
        _xright = xMax - center.x;
        _yup = yMax - center.y;
        _ydown = center.y - yMin;
        _phi1 = atan2(-_xleft, -_ydown);
        _phi3 = atan2(-_xleft, _yup);
        _phi5 = atan2(_xright, _yup);
        _phi7 = atan2(_xright, -_ydown);
        _area1 = 0.5 * _ydown * _xleft;
        _area2 = _area1 + 0.5 * _ydown * _xleft;
        _area3 = _area2 + 0.5 * _yup * _xleft;
        _area4 = _area3 + 0.5 * _yup * _xleft;
        _area5 = _area4 + 0.5 * _yup * _xright;
        _area6 = _area5 + 0.5 * _yup * _xright;
        _area7 = _area6 + 0.5 * _ydown * _xright;
        _totalArea = _area7 + 0.5 * _ydown * _xright;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

double SmartBBox::segLength(double phi) {
        if (phi < _phi1) return -_ydown / cos(phi);
        if (phi < _phi3) return -_xleft / sin(phi);
        if (phi < _phi5) return _yup / cos(phi);
        if (phi < _phi7) return _xright / sin(phi);
        return -_ydown / cos(phi);
}
