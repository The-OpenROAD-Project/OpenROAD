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

//////////////////////////////////////////////////////////////////////
//
// Created by Mike Oliver on May 13, 1999
// hgraphBreakTriangles.cxx: implementation of the HGraphBreakTriangles class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "hgraphBreakTriangles.h"

using uofm::vector;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HGraphBreakTriangles::HGraphBreakTriangles(const HGraphFixed &hgraph) {  // modeled after HGraph copy-ctor
        _numTerminals = hgraph.getNumTerminals();
        _numPins = hgraph.getNumPins();
        _finalized = false;
        getNodeNames() = vector<uofm::string>(hgraph.getNumNodes());

        init(hgraph.getNumNodes(), hgraph.getNumWeights());
        for (unsigned i = 0; i != hgraph.getNumNodes(); ++i) {
                setWeight(i, hgraph.getWeight(i));
                uofm::string tmpNames = hgraph.getNodeNameByIndex(i);
                getNodeNames()[i] = tmpNames;
                getNodeNamesMap()[tmpNames] = i;
        }

        itHGFEdgeGlobal edgeIt;
        for (edgeIt = hgraph.edgesBegin(); edgeIt != hgraph.edgesEnd(); ++edgeIt) {
                HGFEdge const &oldEdge = *(*edgeIt);
                if (oldEdge.getDegree() != 3) {
                        HGFEdge &newEdge = *addEdge(oldEdge.getWeight() * 2);
                        _copyNodes(newEdge, oldEdge);
                } else {
                        itHGFNodeLocal iN = oldEdge.nodesBegin();
                        for (; iN != oldEdge.nodesEnd(); iN++) {
                                HGFEdge &newEdge = *addEdge(oldEdge.getWeight());
                                _copyNodes(newEdge, oldEdge, (*iN)->getIndex());
                        }
                }
        }
        finalize();
}

void HGraphBreakTriangles::_copyNodes(HGFEdge &newEdge, HGFEdge const &oldEdge, unsigned excludeIdx) {
        itHGFNodeLocal adjIt;
#ifdef SIGNAL_DIRECTIONS
        for (adjIt = oldEdge.srcsBegin(); adjIt != oldEdge.srcsEnd(); ++adjIt) {
                unsigned idx = (*adjIt)->getIndex();
                if (excludeIdx != idx) addSrc(getNodeByIdx(idx), newEdge);
        }
        for (adjIt = oldEdge.snksBegin(); adjIt != oldEdge.snksEnd(); ++adjIt) {
                unsigned idx = (*adjIt)->getIndex();
                if (excludeIdx != idx) addSnk(getNodeByIdx(idx), newEdge);
        }
        for (adjIt = oldEdge.srcSnksBegin(); adjIt != oldEdge.srcSnksEnd(); ++adjIt) {
                unsigned idx = (*adjIt)->getIndex();
                if (excludeIdx != idx) addSrcSnk(getNodeByIdx(idx), newEdge);
        }
#else
        for (adjIt = oldEdge.nodesBegin(); adjIt != oldEdge.nodesEnd(); ++adjIt) {
                unsigned idx = (*adjIt)->getIndex();
                if (excludeIdx != idx) addSrcSnk(getNodeByIdx(idx), newEdge);
        }
#endif
}
