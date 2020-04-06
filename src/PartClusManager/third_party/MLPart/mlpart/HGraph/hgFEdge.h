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

// 020811  ilm   ported to g++-3.0

#ifndef _HGRAPHFIXEDEDGE_H_
#define _HGRAPHFIXEDEDGE_H_

#include "ABKCommon/abkcommon.h"
#include "hgBase.h"
#include <ABKCommon/uofm_alloc.h>

class HGraphFixed;
class HGFNode;
class HGFEdge;

#ifndef ctainerHGFNodesLocal
typedef uofm::vector<HGFNode*> ctainerHGFNodesLocal;
typedef uofm::vector<HGFEdge*> ctainerHGFEdgesLocal;

typedef ctainerHGFNodesLocal::const_iterator itHGFNodeLocal;
typedef ctainerHGFEdgesLocal::const_iterator itHGFEdgeLocal;

typedef ctainerHGFNodesLocal::iterator itHGFNodeLocalMutable;
typedef ctainerHGFEdgesLocal::iterator itHGFEdgeLocalMutable;
#endif

class HGFEdge {
        friend class SubHGraph;
        friend class HGraphWDimensions;

       protected:
        unsigned _index;
        HGWeight _weight;
        ctainerHGFNodesLocal _nodes;

#ifdef SIGNAL_DIRECTIONS
        itHGFNodeLocalMutable _snksBegin;
        itHGFNodeLocalMutable _srcSnksBegin;
#endif

        HGFEdge(HGWeight weight = HGWeight(1.0)) : _weight(weight) {}

        friend class HGraphFixed;
        // only HGraphFixed can call our ctor

       public:
        unsigned getIndex() const { return _index; }

        HGWeight getWeight() const { return _weight; }
        void setWeight(HGWeight weight) { _weight = weight; }

        unsigned getDegree() const { return _nodes.size(); }
        unsigned getNumNodes() const { return _nodes.size(); }

#ifdef SIGNAL_DIRECTIONS
        unsigned getNumSrcs() { return _srcSnksBegin - _nodes.begin(); }
        unsigned getNumSrcSnks() const { return _snksBegin - _srcSnksBegin; }
        unsigned getNumSnks() const { return _nodes.end() - _snksBegin; }
#endif

        itHGFNodeLocal nodesBegin() const { return _nodes.begin(); }
        itHGFNodeLocal nodesEnd() const { return _nodes.end(); }

#ifdef SIGNAL_DIRECTIONS
        itHGFNodeLocal srcsBegin() const { return nodesBegin(); }
        itHGFNodeLocal srcsEnd() const { return srcSnksBegin(); }
        itHGFNodeLocal srcSnksBegin() const { return _srcSnksBegin; }
        itHGFNodeLocal srcSnksEnd() const { return snksBegin(); }
        itHGFNodeLocal snksBegin() const { return _snksBegin; }
        itHGFNodeLocal snksEnd() const { return nodesEnd(); }
#endif

#ifdef SIGNAL_DIRECTIONS
        bool isNodeSrc(const HGFNode* node) const;
        bool isNodeSnk(const HGFNode* node) const;
        bool isNodeSrcSnk(const HGFNode* node) const;
#endif

        bool isNodeAdjacent(const HGFNode* node) const { return (find(_nodes.begin(), _nodes.end(), node) != _nodes.end()); }

        friend std::ostream& operator<<(std::ostream& out, const HGFEdge& edge);
};

#endif
