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

//! author="Igor Markov, June 15, 1997"
//! CHANGES="placement.h 971206 ilm  Placement::_Grid2 now constructs a regular
// grid size*size filling the given BBox"

/* Created:  Igor Markov, VLSI CAD ABKGROUP UCLA, June 15, 1997.
  971206 ilm  Placement::_Grid2 now constructs a regular grid size*size
             filling the given BBox
*/

#ifndef _PLACEMENT_H_
#define _PLACEMENT_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <Combi/combi.h>
#include <Geoms/plGeom.h>
#include <iostream>

/*
  You need to include other headers yourself
 'cause they are too big and you may not need them at all
*/

class Placement;
class CompareX;
class CompareY;

/*------------------------  INTERFACES START HERE  -------------------*/

//: A 'generic pointset' class that knows nothing about
//  placement algorithms.  A \_placement\_ is an array of (x,y) pairs (points),
//  < (x0,y0), (x1,y1), ..., (xN-1,yN-1) >.
class Placement {

       public:
        enum Examples {
                _Random,
                _Grid1,
                _Grid2
        };
        // The elements in Examples are used for exampled constructor.
        // _Random is to get the random placement.
        // _Grid1 is to get even placement include the four sides of layout
        // _Grid2 is to get even placement except the four sides of layout

       protected:
        unsigned nPts;
        Point* points;

       protected:
        Placement() {
                nPts = 0;
                points = 0;
        }
        // noop constructor for friends and relatives ;-)

        friend class SubPlacement;
        friend class dbSpatial;

       public:
        Placement(unsigned n);
        // Just allocate a array with size n
        Placement(unsigned n, Point pt);
        Placement(const Placement& pl);
        // Construct placement object with an existed one
        Placement(const uofm::vector<Point>&);
        // Construct placement object with a vector of points
        explicit Placement(std::istream& in);
        Placement(const Mapping& pullBackMap, const Placement& from);  // pullback
        Placement(Examples ex, unsigned size, BBox bb = BBox(0, 0, 1, 1));
        virtual ~Placement() {
                if (points) delete[] points;
        };

        unsigned getSize() const {
                return nPts;
        };
        unsigned size() const {
                return nPts;
        };

        Point& operator[](unsigned idx) {
                abkassert3(idx < getSize(), idx, " Index out of range ", getSize());
                return points[idx];
        }
        const Point& operator[](unsigned idx) const {
                abkassert3(idx < getSize(), idx, " Index out of range ", getSize());
                return points[idx];
        }

        Placement& operator=(const Placement& from);
        // destroys the placement and creates a new one, possibly of different
        // size

        friend std::ostream& operator<<(std::ostream& out, const Placement& arg);

        friend double operator-(const Placement& arg1, const Placement& arg2);
        // returns the Linf distance between placements

        BBox getBBox() const;
        // Get the BBox cover all of the points of placement
        Point getCenterOfGravity() const;
        Point getCenterOfGravity(double* weights) const;

        void flipXY();

        virtual void reorder(const Permutation& perm);
        // Reorder the points in current placement according to pm
        // points[k] goes to points[perm[k]]
        virtual void reorderBack(const Permutation& perm);
        // return from reordered state to original order of points
        // points[perm[k]] goes to points[k]

        double getPolygonArea() const;
        // assumes a non-selfXsecting polygon
        // counter-clockwise is the positive orient.
        bool isInsidePolygon(const Point& pt) const;
        // counter-clockwise is the positive orient

        friend double getMSTCost(const Placement& pl);
        friend double getMSTCost(const Placement& pl, unsigned* pairs, unsigned nPairs);

        //  double getMSTCost() const;
        // uses Lou Scheffer's MST8 code
        //  double getMSTCost(unsigned *pairs,unsigned nPairs) const;
        // allows for "equivalent points", e.g., pins within a net which belong
        // to
        // the same cell. The pairs array should ne 2*nPairs long
        // pairs[2*nPairs]
        // is equivalent to pairs[2*nPairs+1]
};

std::ostream& operator<<(std::ostream& out, const Placement& arg);

double operator-(const Placement& arg1, const Placement& arg2);
// returns the Linf distance between placements
// which you can compare then to epsilon

//: A functional object used for sorting vectors of points by
//  their X coordinates
class CompareX {
        const Placement& _pl;

       public:
        CompareX(const Placement& pl) : _pl(pl) {}
        int operator()(unsigned i, unsigned j) { return _pl[i].x < _pl[j].x; }
};

//: A functional object used for sorting vectors of points by
//  their Y coordinates
class CompareY {
        const Placement& _pl;

       public:
        CompareY(const Placement& pl) : _pl(pl) {}
        int operator()(unsigned i, unsigned j) { return _pl[i].y < _pl[j].y; }
};

/* ============================ IMPLEMENTATION ========================== */

inline Placement::Placement(unsigned n) : nPts(n) {
        // abkfatal(n>0,"Empty placement");
        if (nPts)
                points = new Point[n];
        else
                points = NULL;
}

inline Placement::Placement(const Placement& pl) : nPts(pl.getSize()) {
        points = new Point[nPts];
        for (unsigned i = 0; i < nPts; i++) points[i] = pl[i];
}

inline Placement::Placement(const uofm::vector<Point>& vec) : nPts(vec.size()) {
        points = new Point[nPts];
        for (unsigned i = 0; i < nPts; i++) points[i] = vec[i];
}

inline BBox Placement::getBBox() const {
        BBox box;  // (points[0]);
        for (unsigned i = 0; i < nPts; i++) box += points[i];
        return box;
}

inline void Placement::reorder(const Permutation& pm) {
        abkassert(pm.getSize() == getSize(), "Can't reorder placement: wrong size of permutation\n");
        Point* newPoints = new Point[getSize()];
        unsigned k;
        for (k = 0; k < getSize(); k++) newPoints[pm[k]] = points[k];
        for (k = 0; k < getSize(); k++) points[k] = newPoints[k];
        delete[] newPoints;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

inline void Placement::reorderBack(const Permutation& pm) {
        abkassert(pm.getSize() == getSize(), "Can't reorder placement: wrong size of permutation\n");
        Point* newPoints = new Point[getSize()];
        unsigned k;
        for (k = 0; k < getSize(); k++) newPoints[k] = points[pm[k]];
        for (k = 0; k < getSize(); k++) points[k] = newPoints[k];
        delete[] newPoints;
}

#endif
