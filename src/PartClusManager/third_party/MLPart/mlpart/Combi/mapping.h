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

//! author="Igor Markov June 28, 1997"

#ifndef _MAPPING_H_
#define _MAPPING_H_

#include "ABKCommon/abkassert.h"
#include <iostream>
#include <algorithm>
#include <ABKCommon/uofm_alloc.h>

class Mapping;

typedef Mapping OrderedSubset;
typedef OrderedSubset Subset;

/*------------------------  INTERFACES START HERE  -------------------*/

//: An injective mapping from the finite set
//  {0,...,K-1} to the finite set {0,...,N-1}, K <= N.
//  (only linear time check possible, see getPartialInverse)
class Mapping {
       public:
        enum Example {
                Identity,
                _Reverse,
                _Random,
                _RandomOrdered
        };
        // Identity:  like 0, 1, 2, 3, 4, ..., n
        // _Reverse:  like n, n-1, ..., 2, 1, 0
        // _Random, _RandomOrdered: get a random number sequence.
        // reverse is not the inverse ;-) it's rather like 4321, but not
        // necessarily a permutation
        // _RandomOrdered is faster than _Random (elements are ordered)
        enum BinaryOp {
                COMPOSE,
                UNION,
                XSECT,
                MINUS
        };
        //  UNION :  given two mappings, determine which items in the
        //  destination set are hit by either mapping  (e.g., used in merging
        //  two partial placements)
        //  COMPLEMENT:  given a mapping, determine which items in the range
        //  are not hit (implemented using partial inverse)
        enum UnaryOp {
                COMPLEMENT
        };

       protected:
        unsigned _destSize;
        uofm::vector<unsigned> _meat;

       public:
        Mapping() : _destSize(0), _meat(0) {}
        // don't overuse/overlook
        inline Mapping(unsigned sourceSize, unsigned targetSize);
        inline Mapping(unsigned sourceSize, unsigned targetSize, const unsigned data[]);
        inline Mapping(unsigned sourceSize, unsigned targetSize, const uofm::vector<unsigned>& data);
        Mapping(unsigned sourceSize, unsigned targetSize, Example ex);
        inline Mapping(const Mapping& arg1, const Mapping& arg2);
        Mapping(const Mapping& arg1, BinaryOp, const Mapping& arg2);
        // composition arg1(arg2())
        Mapping(UnaryOp, const Mapping& arg);
        Mapping(const Mapping& mp) : _destSize(mp._destSize), _meat(mp._meat) {}
        inline Mapping& operator=(const Mapping& mp);

        unsigned getSourceSize() const { return _meat.size(); }
        unsigned getDestSize() const { return _destSize; }
        inline unsigned operator[](unsigned idx) const;
        unsigned& operator[](unsigned idx) { return _meat[idx]; }

        const uofm::vector<unsigned>& getVector() const { return _meat; }

        uofm::vector<unsigned>& getPartialInverse(uofm::vector<unsigned>& result) const;
        //  Takes a vector, stretches it to getDestSize() if it is shorter;
        //  puts preimage of k into place k ("marks" the range of the mapping)
        //  Sets all other places to -1; checks injectivity (this is the only
        // way)

        friend std::ostream& operator<<(std::ostream& out, const Mapping& mp);

        operator const uofm::vector<unsigned>&() const { return _meat; }

        void swapValues(unsigned i, unsigned j) {
                unsigned tmp = _meat[i];
                _meat[i] = _meat[j];
                _meat[j] = tmp;
        }
};

/*------------------------      INTERFACES END HERE    -------------------
------------------------  IMPLEMENTATIONS START HERE -------------------*/

inline Mapping::Mapping(unsigned sourceSize, unsigned targetSize)
    : _destSize(targetSize),
#ifdef ABKDEBUG
      /* initialize with "bad" value */
      _meat(sourceSize, (unsigned)-1)
#else
      _meat(sourceSize)
#endif
{
        abkassert(sourceSize <= targetSize, " Map not injective: source set bigger than target set");
}

inline Mapping::Mapping(unsigned sourceSize, unsigned targetSize, const unsigned data[]) : _destSize(targetSize), _meat(sourceSize) {
        abkfatal(data, " Can\'t init mapping: NULL data");
        abkassert(sourceSize <= targetSize, " Can\'t init mappping: source set bigger than target set");
        for (unsigned i = 0; i < sourceSize; i++) {
                abkassert(data[i] < _destSize, " Can\'t init mapping: data out of range");
                _meat[i] = data[i];
        }
}

inline Mapping::Mapping(unsigned sourceSize, unsigned targetSize, const uofm::vector<unsigned>& data) : _destSize(targetSize), _meat(sourceSize) {
        abkassert(data.size() >= sourceSize, "Can\'t init mapping: vector too short");
        abkassert(sourceSize <= targetSize, " Can\'t init mappping: source set bigger than target set");
        for (unsigned i = 0; i < sourceSize; i++) {
                abkassert(data[i] < _destSize, " Can\'t init mapping: data out of range");
                _meat[i] = data[i];
        }
}

inline Mapping& Mapping::operator=(const Mapping& mp) {
        _destSize = mp._destSize;
        _meat = mp._meat;
        return *this;
}

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "062897, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

inline unsigned Mapping::operator[](unsigned idx) const {
        abkassert(idx < _meat.size(), " Mapping argument out of range");
        abkassert(_meat[idx] < _destSize, " Mapping image out of range");
        return _meat[idx];
}

inline Mapping::Mapping(const Mapping& arg1, const Mapping& arg2)
    :
      /* compose arg1(arg2()) */
      _destSize(arg1.getDestSize()),
      _meat(arg2.getSourceSize()) {
        abkassert(arg1.getSourceSize() >= arg2.getDestSize(), "Can\'t compose mappings");
        for (unsigned i = 0; i < _meat.size(); i++) _meat[i] = arg1[arg2[i]];
}

#endif
