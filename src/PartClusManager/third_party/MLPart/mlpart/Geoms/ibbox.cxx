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

#include "ABKCommon/abkcommon.h"
#include "ibbox.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using std::ostream;
using std::istream;

class IBBox;

/* ============================ IMPLEMENTATIONS ========================== */

ostream& operator<<(ostream& out, const IBBox& arg) {
        if (arg.isEmpty()) out << "Empty IBBox: ";
        out << arg.xMin << "  " << arg.yMin << "   " << arg.xMax << "  " << arg.yMax << "\n";
        return out;
}

istream& operator>>(istream& in, IBBox& arg) {
        in >> my_isnumber() >> arg.xMin >> my_isnumber() >> arg.yMin >> my_isnumber() >> arg.xMax >> my_isnumber() >> arg.yMax;
        return in;
}

int IBBox::mdistTo(IPoint pt) const {
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

int IBBox::mdistTo(const IBBox& box) const {
        abkassert(!isEmpty(), "Can\'t compute distance to an empty bounding box");
        abkassert(!box.isEmpty(), "Can\'t compute distance to an empty bounding box");

        int dx, dy;

        if ((dx = box.xMin - xMax) < 0)
                if ((dx = xMin - box.xMax) < 0) dx = 0;
        if ((dy = box.yMin - yMax) < 0)
                if ((dy = yMin - box.yMax) < 0) dy = 0;

        return dx + dy;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif
