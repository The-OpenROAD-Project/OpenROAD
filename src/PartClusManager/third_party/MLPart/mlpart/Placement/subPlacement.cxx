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

//  Created:  Igor Markov, VLSI CAD ABKGROUP UCLA, June 15, 1997.
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include <Geoms/plGeom.h>
#include <Placement/subPlacement.h>
#include <iostream>

using std::ostream;
using std::endl;
using std::min;
using std::max;

Point SubPlacement::getCenterOfGravity() const {
        Point cog(0, 0);
        for (unsigned i = 0; i < getSize(); i++) {
                cog.x += points(i).x;
                cog.y += points(i).y;
        }
        cog.x /= getSize();
        cog.y /= getSize();
        return cog;
}

Point SubPlacement::getCenterOfGravity(double* weights) const {
        Point cog(0, 0);
        for (unsigned i = 0; i < getSize(); i++) {
                cog.x += weights[i] * points(i).x;
                cog.y += weights[i] * points(i).y;
        }
        cog.x /= getSize();
        cog.y /= getSize();
        return cog;
}

double SubPlacement::getPolygonArea() const {
        double area = 0.0;
        for (unsigned i = 0; i < getSize() - 1; i++) area += points(i).x * points(i + 1).y - points(i + 1).x * points(i).y;
        area += points(getSize() - 1).x * points(0).y - points(0).x * points(getSize() - 1).y;
        return 0.5 * area;
}

bool SubPlacement::isInsidePolygon(const Point& p) const {
        unsigned counter = 0;
        unsigned i;
        double xinters;
        Point p1, p2;

        p1 = points(0);
        for (i = 0; i < getSize(); i++) {
                p2 = points(i % getSize());
                if (p.y > min(p1.y, p2.y)) {
                        if (p.y <= max(p1.y, p2.y)) {
                                if (p.x <= max(p1.x, p2.x)) {
                                        if (p1.y != p2.y) {
                                                xinters = (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
                                                if (p1.x == p2.x || p.x <= xinters) counter++;
                                        }
                                }
                        }
                }
                p1 = p2;
        }

        if (counter % 2 == 0)
                return (false);  // outisde
        else
                return (true);  // inside
}

ostream& operator<<(ostream& out, const SubPlacement& arg) {
        TimeStamp tm;
        out << tm;
        out << " Total points in sub placement : " << arg.getSize() << endl;
        for (unsigned i = 0; i < arg.getSize(); i++) out << arg.points(i) << endl;
        return out;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

double operator-(const SubPlacement& arg1, const SubPlacement& arg2)
    /* returns the Linf distance between placements
       which you can compare then to epsilon     */
{
        abkfatal(arg1.getSize() == arg2.getSize(), " Comparing placements of different size");
        double dst, max = 0;
        for (unsigned i = 0; i < arg1.getSize(); i++) {
                dst = mDist(arg1[i], arg2[i]);
                if (max < dst) max = dst;
        }
        return max;
}
