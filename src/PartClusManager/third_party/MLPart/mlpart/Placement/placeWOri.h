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

//! author="Andrew Caldwell, August 11, 1999"

#ifndef _PLACEWORI_H_
#define _PLACEWORI_H_

#include <ABKCommon/uofm_alloc.h>
#include <Placement/placement.h>
#include <Placement/symOri.h>

class PlacementWOrient : public Placement {
        friend class SubPlacement;
        friend class dbSpatial;
        friend class Capo;

        Orientation* orients;

        PlacementWOrient() { orients = 0; }
        // noop constructor for friends and relatives ;-)

       public:
        PlacementWOrient(unsigned n);
        PlacementWOrient(unsigned n, Point pt, Orientation ori = Orientation());
        PlacementWOrient(const PlacementWOrient& pl);
        PlacementWOrient(const Placement& pl, const uofm::vector<Orientation>&);
        PlacementWOrient(const uofm::vector<Point>&);
        PlacementWOrient(const uofm::vector<Point>&, const uofm::vector<Orientation>&);

        PlacementWOrient(const Mapping& pullBackMap, const PlacementWOrient& from);

        PlacementWOrient(const char* plFileName, unsigned nTerms, unsigned nCore);

        virtual ~PlacementWOrient() {
                if (orients) delete[] orients;
        };

        Orientation& getOrient(unsigned idx) {
                abkassert3(idx < getSize(), idx, " Index out of range ", getSize());
                return orients[idx];
        }
        const Orientation& getOrient(unsigned idx) const {
                abkassert3(idx < getSize(), idx, " Index out of range ", getSize());
                return orients[idx];
        }

        void setOrient(unsigned idx, Orient ori) {
                abkassert3(idx < getSize(), idx, " Index out of range ", getSize());
                orients[idx] = ori;
        }

        PlacementWOrient& operator=(const PlacementWOrient& from);

        friend std::ostream& operator<<(std::ostream& out, const PlacementWOrient& arg);

        virtual void reorder(const Permutation& perm);
        // Reorder the points in current placement according to pm
        // points[k] goes to points[perm[k]]

        virtual void reorderBack(const Permutation& perm);
        // return from reordered state to original order of points
        // points[perm[k]] goes to points[k]

        void save(const char* plFileName, unsigned nTerms) const;

        const Orientation* getOrients() const { return orients; }
};

std::ostream& operator<<(std::ostream& out, const PlacementWOrient& arg);

//: A functional object used for sorting vectors of points by
//  their X coordinates
class CompareXWOri {
        const PlacementWOrient& _pl;

       public:
        CompareXWOri(const PlacementWOrient& pl) : _pl(pl) {}
        int operator()(unsigned i, unsigned j) { return _pl[i].x < _pl[j].x; }
};

//: A functional object used for sorting vectors of points by
//  their Y coordinates
class CompareYWOri {
        const PlacementWOrient& _pl;

       public:
        CompareYWOri(const PlacementWOrient& pl) : _pl(pl) {}
        int operator()(unsigned i, unsigned j) { return _pl[i].y < _pl[j].y; }
};

#endif
