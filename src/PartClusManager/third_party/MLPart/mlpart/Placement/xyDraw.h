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

//! CHANGES="xyDraw.h 971130  ilm  to draw a line with one dot, xyDrawLine uses
// the beginning numDots is now optional with default value 2 971216  mro  moved
// xyDrawPoints to xyDraw.cxx "
/* CHANGES
  971130  ilm  to draw a line with one dot, xyDrawLine uses the beginning
          numDots is now optional with default value 2
  971216  mro  moved xyDrawPoints to xyDraw.cxx
*/

#ifndef _XYDRAW_H_
#define _XYDRAW_H_

#include <ABKCommon/abkcommon.h>
#include <Placement/placement.h>
#include <iostream>

// draws a line (into ostream) by plotting numDots on it
void xyDrawLine(std::ostream& xystream, Point begin, Point end, unsigned numDots = 2);

// draws a rectangle (into ostream) with numDots on each side
void xyDrawRectangle(std::ostream& xystream, Rectangle rect, unsigned numDots = 1);

// draws lines from heads[k] to tails[k] with numDots each
void xyDrawLines(std::ostream&, Placement heads, Placement tails, unsigned numDots = 2);

// draws points of Placement
void xyDrawPoints(std::ostream& xystream, Placement points);

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "971128, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
