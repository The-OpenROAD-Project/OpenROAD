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

// CHANGES
//  971130  ilm  to draw a line with one dot, xyDrawLine uses the beginning
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ABKCommon/abkcommon.h>
#include <Placement/placement.h>
#include <Placement/xyDraw.h>
#include <iostream>

using std::ostream;
using std::endl;

void xyDrawLine(ostream& xystream, Point begin, Point end, unsigned numDots)
    // draws a line (into ostream) by plotting numDots on it
{
        abkfatal(numDots, "Need at least one dot to draw a line");

        Point dot = begin;
        xystream << dot << endl;

        if (numDots == 1) return;

        Point delta = end - begin;
        delta.x /= (numDots - 1);
        delta.y /= (numDots - 1);
        for (unsigned k = 1; k < numDots; k++) xystream << dot.moveBy(delta) << endl;
}

void xyDrawRectangle(ostream& xystream, Rectangle rect, unsigned numDots)
    // draws a rectangle (into ostream) with numDots on each side
{
        if (rect.isEmpty()) {
                abkwarn(0, "Drawing an empty rectangle");
                return;
        }
        xyDrawLine(xystream, Point(rect.xMin, rect.yMin), Point(rect.xMin, rect.yMax), numDots);
        xyDrawLine(xystream, Point(rect.xMin, rect.yMax), Point(rect.xMax, rect.yMax), numDots);
        xyDrawLine(xystream, Point(rect.xMax, rect.yMax), Point(rect.xMax, rect.yMin), numDots);
        xyDrawLine(xystream, Point(rect.xMax, rect.yMin), Point(rect.xMin, rect.yMin), numDots);
        if (numDots == 1) xystream << Point(rect.xMin, rect.yMin) << endl;
}

void xyDrawLines(ostream& xystream, Placement heads, Placement tails, unsigned numDots)
    // draws lines heads[k] to tails[k] with numDots each
{
        abkfatal(heads.getSize() == tails.getSize(), "Size mismatch in line draw data");
        for (unsigned k = 0; k < heads.getSize(); k++) {
                xyDrawLine(xystream, heads[k], tails[k], numDots);
                xystream << endl;
        }
}

void xyDrawPoints(ostream& xystream, Placement points) { xyDrawLines(xystream, points, points, 1); }

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "971128, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif
