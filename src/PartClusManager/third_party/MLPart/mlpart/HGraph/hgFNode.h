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

#ifndef _HGRAPHFIXEDNODE_H_
#define _HGRAPHFIXEDNODE_H_

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

// Hypergraph node
// Note that there is an important asymmetry between nodes and hyperedges
// to change the links between nodes and edges, the user must call HGFEdge::
// addSrc, etc.; similar methods in HGFNode are unavailable
//
// a low-level way to think about it is that all links are managed
// by the edges: HGFEdge methods call private methods inside the affected
// nodes to update the nodes; HGFNode methods assume they're called from
// HGFEdge, and so don't try to update the affected edges

struct HGN {
        HGN(const HGraphFixed& hg, const HGFNode& node) : _hg(hg), _node(node) {}
        const HGraphFixed& _hg;
        const HGFNode& _node;
};

class HGFNode {
        friend class HGraphFixed;
        friend class HGraphWDimensions;
        friend class HGraphFromSlice;
        friend class HGFEdge;
        friend class SubHGraph;

       protected:
        unsigned _index;
        ctainerHGFEdgesLocal _edges;

#ifdef SIGNAL_DIRECTIONS
        itHGFEdgeLocalMutable _snksBegin;
        itHGFEdgeLocalMutable _srcSnksBegin;
#endif

        HGFNode() : _index(UINT_MAX) {}

        HGFNode(unsigned id) : _index(id) {}

        void reserveEdges(unsigned degree) { _edges.reserve(degree); }

       public:
        unsigned getIndex() const { return _index; }

        unsigned getDegree() const { return _edges.size(); }
        unsigned getNumEdges() const { return _edges.size(); }

#ifdef SIGNAL_DIRECTIONS
        unsigned getNumSrcEdges() { return _srcSnksBegin - _edges.begin(); }
        unsigned getNumSrcSnkEdges() const { return _snksBegin - _srcSnksBegin; }
        unsigned getNumSnkEdges() const { return _edges.end() - _snksBegin; }
#endif

        itHGFEdgeLocal edgesBegin() const { return _edges.begin(); }
        itHGFEdgeLocal edgesEnd() const { return _edges.end(); }

#ifdef SIGNAL_DIRECTIONS
        itHGFEdgeLocal srcEdgesBegin() const { return edgesBegin(); }
        itHGFEdgeLocal srcEdgesEnd() const { return srcSnkEdgesBegin(); }
        itHGFEdgeLocal srcSnkEdgesBegin() const { return _srcSnksBegin; }
        itHGFEdgeLocal srcSnkEdgesEnd() const { return snkEdgesBegin(); }
        itHGFEdgeLocal snkEdgesBegin() const { return _snksBegin; }
        itHGFEdgeLocal snkEdgesEnd() const { return edgesEnd(); }
#endif

#ifdef SIGNAL_DIRECTIONS
        bool isEdgeSrc(const HGFEdge* edge) const;
        bool isEdgeSnk(const HGFEdge* edge) const;
        bool isEdgeSrcSnk(const HGFEdge* edge) const;
#endif
        bool isEdgeAdjacent(const HGFEdge* edge) const { return (find(_edges.begin(), _edges.end(), edge) != _edges.end()); }

        friend HGN operator<<(const HGraphFixed& hgraph, const HGFNode& node);
        friend std::ostream& operator<<(std::ostream& out, const HGN& hgn);
};

HGN operator<<(const HGraphFixed& hgraph, const HGFNode& node);
std::ostream& operator<<(std::ostream& out, const HGN& hgn);

#endif
