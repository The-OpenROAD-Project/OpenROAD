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

//! author=" Igor Markov June 24, 1997"

// Created:    Igor Markov,  VLSI CAD ABKGROUP UCLA  June 24, 1997

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "placement.h"
#include "math.h"

class Transform;
class SwapXY;
class ToPolar;
class BBoxToBBox;
class ToXY;
class ClumpInCenter;
class Compression;
class DeCompression;
class PolarRotation;
class RectRotation;

/*------------------------  INTERFACES START HERE  -------------------*/

//: Base class for typically continuous geometric tranformation
class Transform {
       public:
        virtual Point& apply(Point& pt) = 0;
        virtual Placement& operator()(Placement& pl) {
                for (unsigned i = 0; i < pl.getSize(); i++) apply(pl[i]);
                return pl;
        }
        virtual BBox& operator()(BBox& bbx) {
                Point minPt(bbx.xMin, bbx.yMin);
                Point maxPt(bbx.xMax, bbx.yMax);
                apply(minPt);
                apply(maxPt);
                bbx.xMin = minPt.x;
                bbx.yMin = minPt.y;
                bbx.xMax = maxPt.x;
                bbx.yMax = maxPt.y;
                return bbx;
        }
        virtual ~Transform() {}
};

//: Swaps the value of x and y coordinate of one point
class SwapXY : public Transform {
       public:
        Point& apply(Point& pt) {
                std::swap(pt.x, pt.y);
                return pt;
        }
};

//: Transforms to polar coordinate
class ToPolar : public Transform {
       public:
        Point& apply(Point& pt) {
                double alpha = atan2(pt.x, pt.y);
                pt.x = sqrt(pt.x * pt.x + pt.y * pt.y);
                pt.y = alpha;
                return pt;
        }
};

//: Transforms from source BBox to destination BBox
class BBoxToBBox : public Transform {
        Point srcBotLeftCorner;
        Point destBotLeftCorner;
        double xScale, yScale;

       public:
        BBoxToBBox(const BBox& source, const BBox& dest) : srcBotLeftCorner(source.xMin, source.yMin), destBotLeftCorner(dest.xMin, dest.yMin), xScale((dest.xMax - dest.xMin) / (source.xMax - source.xMin)), yScale((dest.yMax - dest.yMin) / (source.yMax - source.yMin)) {}
        Point& apply(Point& pt) {
                pt -= srcBotLeftCorner;
                pt.scaleBy(xScale, yScale);
                return pt += destBotLeftCorner;
        }
};

//: Transforms from Polar coordinate to XY coordinate
class ToXY : public Transform {
       public:
        Point& apply(Point& pt) {
                double x1 = pt.x * sin(pt.y);
                pt.y = pt.x * cos(pt.y);
                pt.x = x1;
                return pt;
        }
};

//: Clumps the BBox according to center point
class ClumpInCenter : public Transform, protected SmartBBox {
        double _alpha;

       public:
        ClumpInCenter(BBox bbx, double alpha) : SmartBBox(bbx), _alpha(alpha) { abkfatal(alpha > 0, " ClumpInCenter: negative alpha not allowed "); }
        ClumpInCenter(BBox bbx, Point centr, double alpha) : SmartBBox(bbx, centr), _alpha(alpha) { abkfatal(alpha > 0, " ClumpInCenter: negative alpha not allowed "); }

        Point& apply(Point& pt) {
                double dx = pt.x - center.x;
                if (dx < 0)
                        pt.x = center.x - _xleft * pow(-dx / _xleft, _alpha);
                else
                        pt.x = center.x + _xright * pow(dx / _xright, _alpha);

                double dy = pt.y - center.y;
                if (dy < 0)
                        pt.y = center.y - _ydown * pow(-dy / _ydown, _alpha);
                else
                        pt.y = center.y + _yup * pow(dy / _yup, _alpha);

                return pt;
        };
};

//: Compresses the coordinate of point with the segment length by point
//  and spread value by point. Point should be in polar coordinate.
class Compression : public Transform {
        SmartBBox _sbbox;

       public:
        Compression(BBox bbx) : _sbbox(bbx) {};
        Compression(BBox bbx, Point center) : _sbbox(bbx, center) {};
        Point& apply(Point& pt) {
                pt.x /= _sbbox.segLength(pt.y);
                pt.y = _sbbox.spreadFunc(pt.y);
                return pt;
        };
};

//: the Inverse transform for Compression
class DeCompression : public Transform {
        SmartBBox _sbbox;

       public:
        DeCompression(BBox bbx) : _sbbox(bbx) {};
        DeCompression(BBox bbx, Point center) : _sbbox(bbx, center) {};
        Point& apply(Point& pt) {
                pt.y = _sbbox.unSpreadFunc(pt.y);
                pt.x *= _sbbox.segLength(pt.y);
                return pt;
        };
};

//: Rotates around the point
class PolarRotation : public Transform {
        double _a;

       public:
        PolarRotation(double alpha) : _a(alpha) {};
        Point& apply(Point& pt) {
                pt.y += _a;
                if (pt.y > Pi) pt.y -= 2 * Pi;
                if (pt.y <= -Pi) pt.y += 2 * Pi;
                return pt;
        };
};

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "071197, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

//: a continuous transform that maps a rectangle
//  into itself while "rotating" its sides
class RectRotation : public Transform {
        SmartBBox _sbb;
        Compression _compress;
        DeCompression _decompress;
        PolarRotation _rotate;
        ToPolar _topolar;
        ToXY _toxy;

       public:
        RectRotation(double alpha, const BBox& bbx) : _sbb(bbx), _compress(bbx), _decompress(bbx), _rotate(alpha) {};
        RectRotation(double alpha, const BBox& bbx, const Point& center) : _sbb(bbx, center), _compress(bbx, center), _decompress(bbx, center), _rotate(alpha) {
                abkfatal(bbx.contains(center), " Rotation center is not in the BBox");
        };
        Point& apply(Point& pt) {
                _sbb.takeOut(pt);
                _topolar.apply(pt);
                _compress.apply(pt);
                _rotate.apply(pt);
                _decompress.apply(pt);
                _toxy.apply(pt);
                _sbb.putIn(pt);
                return pt;
        };
        /*
           Placement&    operator()(Placement& pl)
                                {
                                        // sbb.takeOut(pt);
                                  topolar(pl);
                                  cout << pl;
                                  compress(pl);

                                  toxy(pl); return pl;
                                  rotate(pl);
                                  decompress(pl);
                                  toxy(pl);
                                  return pl;
                                         // sbb.putIn(pt);
                               }
        */
};

#endif
