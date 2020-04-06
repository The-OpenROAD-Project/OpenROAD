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

#ifndef __HGBASE_H__
#define __HGBASE_H__

#include "hgParam.h"
#include "Combi/combi.h"
#include <utility>
#include <ABKCommon/uofm_alloc.h>

typedef double HGWeight;

typedef std::pair<unsigned, double> HGWeightPair;

struct CompareHGWeightPairs {
        bool operator()(const HGWeightPair *w1, const HGWeightPair *w2) { return (w1->first < w2->first); }
};

class HGraphBase {
       protected:
        unsigned _numPins;
        unsigned _numTerminals;

        // weights are stored as follows:
        //_multiWeights is a vector of size (numNodes * _numMultiWeights).
        // additionally, each node contains a begin-end pointer to a
        // variable number of weights, stored in _binWeights.

        unsigned _numMultiWeights;
        unsigned _numTotalWeights;
        uofm::vector<double> _multiWeights;
        // uofm::vector<HGWeightPair> _binWeights;

        unsigned _maxNodeDegree;
        unsigned _maxEdgeDegree;

        mutable Permutation _weightSort;
        mutable Permutation _degreeSort;

        unsigned _ignoredEdges;
        HGraphParameters _param;

        enum {
                ADDEDGE_NOT_USED,
                SLOW_ADDEDGE_USED,
                FAST_ADDEDGE_USED
        } _addEdgeStyle;

        virtual void computeMaxNodeDegree() const = 0;
        virtual void computeMaxEdgeDegree() const = 0;
        virtual void computeNumPins() const = 0;

        HGraphBase(HGraphParameters param = HGraphParameters()) : _numPins(0), _numTerminals(0), _numMultiWeights(0), _numTotalWeights(0), _maxNodeDegree(0), _maxEdgeDegree(0), _ignoredEdges(0), _param(param), _addEdgeStyle(ADDEDGE_NOT_USED) {}

       public:
        virtual ~HGraphBase() {}

        unsigned getNumTerminals() const { return _numTerminals; }
        void clearTerminals() { _numTerminals = 0; }
        bool isTerminal(unsigned nIdx) const { return nIdx < _numTerminals; }

        unsigned getNumPins() const {
                if (!_numPins) computeNumPins();
                return _numPins;
        }

        unsigned getMaxNodeDegree() const {
                if (!_maxNodeDegree) computeMaxNodeDegree();
                return _maxNodeDegree;
        }
        unsigned getMaxEdgeDegree() const {
                if (!_maxEdgeDegree) computeMaxEdgeDegree();
                return _maxEdgeDegree;
        }

        unsigned getNumMultiWeights() const { return _numMultiWeights; }
        unsigned getNumWeights() const { return _numTotalWeights; }

        void setNumTerminals(unsigned numT) { _numTerminals = numT; }
        HGWeight getWeight(unsigned nodeIdx, unsigned which = 0) const;

        void setWeight(unsigned nodeIdx, HGWeight weight, unsigned which = 0);
};

#endif
