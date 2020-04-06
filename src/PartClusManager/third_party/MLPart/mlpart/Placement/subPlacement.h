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

//! author=" Igor Markov, June 15, 1997"

#ifndef _SUBPLACEMENT_H_
#define _SUBPLACEMENT_H_

#include <ABKCommon/abkcommon.h>
#include <Combi/mapping.h>
#include <Placement/placement.h>
#include <iostream>

class SubPlacement;

/*------------------------  INTERFACES START HERE  -------------------*/

//: A _SubPlacement_ is a part (subset) of a larger placement.
//    It is implemented by a mapping that is hardwired to a referenced
//    placement object.   ** I.e., this notion of partial placement
//    assumes the existence of the larger placement. **
//
//    The purpose of the partial placement is to keep track of
//    only some instances in a (larger) placement object.
//    The partial placement does not have any separate storage for
//    locations;  these are stored by the placement object.
//    The mapping part of the partial placement refers to specific
//    (x,y) points in the placement object.
//
//    There can be more than one partial placements that reference
//    a given placement object.
//
//    We remark that once created, a partial placement cannot be
//    modified.    In other words, both the mapping and the reference
//    to a placement object are constant.  However, the coordinates
//    of the referenced placement can be modified (see (2.) in
//    "typical operations", below).
//
//    Notice that a partial placement, since it contains the mapping
//    from {0,...,K-1} to {0,...,N-1}, in some sense has a "source placement"
//    (the locations of K objects), and a "destination placement" (the
//    referenced placement itself, with N objects).
//    -- Refer to "SPECS/placements1.1"
//    class SubPlacement implements almost all inline methods of class Placement
//    For everything else, you probably need to pullBack a Placement from
//    SubPlacement, work on it and then pushForward
class SubPlacement {

       protected:
        const Placement& _realPl;
        Subset _inj;
        // note: Sub Placement owns its mapping, but not its Placement
        Point& points(unsigned i) {
                return _realPl.points[_inj[i]];
        };
        /* these four are for easy compatibility  */
        const Point& points(unsigned i) const {
                return _realPl.points[_inj[i]];
        };

       public:
        SubPlacement(const Placement& rpl, const Subset&);
        // Creation from the destination placement and a mapping
        SubPlacement(const SubPlacement& ppl) : _realPl(ppl._realPl), _inj(ppl._inj) {};
        // Creation from other SubPlacement
        ~SubPlacement() {};
        // since the class has no remote ownerships

        const Mapping& getMapping() const { return _inj; }
        // synonyms
        const Subset& getSubset() const { return _inj; }

        const Placement& getPlacement() const { return _realPl; }

        const Placement& pushForward(const Placement& smallPl);
        //  Pushforward (i.e., "executing" a partial placement by
        //  using a smaller placement to set locations in a larger
        //  placement) , returns _realPl
        const Placement& pushForwardX(const Placement& smallPl);
        const Placement& pushForwardY(const Placement& smallPl);

        Placement& pullBack(Placement& smallPl);
        // Pullback (i.e., getting a [smaller] number of locations
        // from a larger placement.
        // empties smallPl and creates a new one, possibly of different size
        // returns smallPl.
        unsigned getSize() const {
                return _inj.getSourceSize();
        };
        unsigned getSourceSize() const {
                return _inj.getSourceSize();
        };
        unsigned getDestSize() const {
                return _inj.getDestSize();
        };

        Point& operator[](unsigned idx) { return points(idx); }
        const Point& operator[](unsigned idx) const { return points(idx); }

        SubPlacement& operator=(const SubPlacement&) {
                abkfatal(0, "Can\'t assign sub placements");
                return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const SubPlacement& arg);

        BBox getBBox() const;
        // get the BBox cover all point of subplacement
        Point getCenterOfGravity() const;
        // get the gravity center of the subplacement
        Point getCenterOfGravity(double* weights) const;
        void reorder(const Permutation& perm);
        // reorder the pints of _inj
        // assumes that the "big" placement has been reordered and adjusts the
        // indeces
        double getPolygonArea() const;
        // calculate the area of the polygon. Assumes a non-selfXsecting polygon
        // counter-clockwise is the positive orient.
        bool isInsidePolygon(const Point& pt) const;
        // Check whether pt is located in the polygon.
        // Counter-clockwise is the positive orient
};

std::ostream& operator<<(std::ostream& out, const SubPlacement& arg);

/* ============================ IMPLEMENTATION ========================== */

inline SubPlacement::SubPlacement(const Placement& rpl, const Subset& mp) : _realPl(rpl), _inj(mp) {
        abkfatal(_inj.getDestSize() <= _realPl.getSize(),
                 "Can\'t construct Sub Placement: mapping and placement "
                 "inconsistent\n");
}

inline BBox SubPlacement::getBBox() const {
        BBox box;
        for (unsigned i = 0; i < getSize(); i++) box += points(i);
        return box;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

// uses smallPl to set locations in the _realPl member; returns _realPl
inline const Placement& SubPlacement::pushForward(const Placement& smallPl) {
        for (unsigned i = 0; i < getSize(); i++) points(i) = smallPl[i];
        return _realPl;
}

// uses smallPl to set x locations in the _realPl member; returns _realPl
inline const Placement& SubPlacement::pushForwardX(const Placement& smallPl) {
        for (unsigned i = 0; i < getSize(); i++) points(i).x = smallPl[i].x;
        return _realPl;
}

// uses smallPl to set y locations in the _realPl member; returns _realPl
inline const Placement& SubPlacement::pushForwardY(const Placement& smallPl) {
        for (unsigned i = 0; i < getSize(); i++) points(i).y = smallPl[i].y;
        return _realPl;
}

// empties smallPl and creates a new one, possibly of different size
// returns smallPl
inline Placement& SubPlacement::pullBack(Placement& smallPl) {
        delete[] smallPl.points;
        smallPl.nPts = getSize();
        smallPl.points = new Point[smallPl.nPts];
        for (unsigned i = 0; i < getSize(); i++) smallPl.points[i] = points(i);
        return smallPl;
}

// reorder the pints of _inj
inline void SubPlacement::reorder(const Permutation& pm) {
        abkassert(pm.getSize() == getDestSize(), "Can't reorder a subplacement: wrong permutation size");
        for (unsigned k = 0; k < getSize(); k++) _inj[k] = pm[_inj[k]];
}

#endif
