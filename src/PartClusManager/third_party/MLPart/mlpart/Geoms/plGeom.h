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

//! author="Igor Markov, July 15, 1997"
//! CHANGES="plGeom.h 971113 mro unified unseeded, seeded ctors in RandomPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9) with percent>0; percent >
// 1.0 will expand the BBox"

// 971113 mro unified unseeded, seeded ctors in RandomPoint
// 971205 ilm added BBox::ShrinkTo(double percent=0.9)
//             with percent>0; percent > 1.0 will expand the BBox

#ifndef _PLGEOM_H_
#define _PLGEOM_H_

class Point;
class BBox;
#ifdef _MSC_VER
#define Rectangle uclaRectangle
#endif
typedef BBox Rectangle;
class RandomPoint;  //  in a given BBox
class SmartBBox;

#include "point.h"
#include "bbox.h"

//: Generates a poin randomly
class RandomPoint {
        RandomDouble _xGen;
        RandomDouble _yGen;

       public:
        RandomPoint(const BBox& bb, unsigned xSeed = UINT_MAX, unsigned ySeed = UINT_MAX) : _xGen(bb.xMin, bb.xMax, xSeed), _yGen(bb.yMin, bb.yMax, ySeed) {};
        unsigned getXSeed() { return _xGen.getSeed(); }
        unsigned getYSeed() { return _yGen.getSeed(); }

        operator Point() { return Point(_xGen, _yGen); }
};

#endif
