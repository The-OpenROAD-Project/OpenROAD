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

// created by Andrew Caldwell and Igor Markov on 05/24/98

#ifndef __NBYK_GAIN_REPOSITORY_H__
#define __NBYK_GAIN_REPOSITORY_H__

#include "ABKCommon/abkcommon.h"
#include "svGainElmt.h"

class NbyKRepository {  // an nXk block of GainElements
        SVGainElement* _blockOfElements;
        SVGainElement* _lastPtr;  // end of the block of elements
        unsigned _n;
        unsigned _k;

       public:
        NbyKRepository(unsigned n, unsigned k) : _n(n), _k(k) {
                _blockOfElements = new SVGainElement[n * k];
                _lastPtr = _blockOfElements + (n * k);
        }

        ~NbyKRepository() { delete[] _blockOfElements; }

        unsigned getElementId(const SVGainElement& elmt) const;
        unsigned getElementPartition(const SVGainElement& elmt) const;
        SVGainElement& getElement(unsigned node, unsigned part) const;
        SVGainElement& getNextPartSameNode(SVGainElement& elmt, unsigned part) const;

        /*
            friend ostream& operator() (ostream &os,
                                const NbyKRepository<GainElement>& rep);
        */
};

inline unsigned NbyKRepository::getElementId(const SVGainElement& elmt) const {
        abkassert(&elmt >= _blockOfElements, "element ptr invalid: too small");
        abkassert(&elmt <= _lastPtr, "element ptr invalid: too large");

        return (&elmt - _blockOfElements) / _k;
}

inline unsigned NbyKRepository::getElementPartition(const SVGainElement& elmt) const {
        abkassert(&elmt >= _blockOfElements, "element ptr invalid: too small");
        abkassert(&elmt <= _lastPtr, "element ptr invalid: too large");

        return (&elmt - _blockOfElements) % _k;
}

inline SVGainElement& NbyKRepository::getElement(unsigned node, unsigned part) const {
        abkassert(node < _n, "node too large for getElement");
        abkassert(part < _k, "part too large for getElement");

        return *(_blockOfElements + (node * _k) + part);
}

inline SVGainElement& NbyKRepository::getNextPartSameNode(SVGainElement& elmt, unsigned part) const
    // NOTE: assumes part is the partition elmt is for.
    // NOTE: asssumes part <= k-1
{
#ifdef ABKDEBUG
        unsigned actualPart = getElementPartition(elmt);
        abkfatal(actualPart == part, "incorrect part in NextPartSameNode");
        abkfatal(part < _k - 1, "part is too large in NextPartSameNode");

        SVGainElement& newElmt = *(&elmt + 1);
        abkfatal(getElementId(newElmt) == getElementId(elmt), "element Ids to not match in nextPartSameNode");
#endif
        // part;

        return *(&elmt + 1);
}

#endif
