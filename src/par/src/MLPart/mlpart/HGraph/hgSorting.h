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

#ifndef _HGSORTING_H_
#define _HGSORTING_H_

struct HGNodeSortByIndex {
        bool operator()(const HGFNode* ptr1, const HGFNode* ptr2) const { return ptr1->getIndex() < ptr2->getIndex(); }
};

struct HGNodeSortByWeight {
        const HGraphFixed& _hgraph;

        HGNodeSortByWeight(const HGraphFixed& hgraph) : _hgraph(hgraph) {}

        bool operator()(const HGFNode* ptr1, const HGFNode* ptr2) const { return _hgraph.getWeight(ptr1->getIndex()) < _hgraph.getWeight(ptr2->getIndex()); }
};

struct HGNodeIdSortByWeights {
        const HGraphFixed& _hgraph;

        HGNodeIdSortByWeights(const HGraphFixed& hgraph) : _hgraph(hgraph) {}

        bool operator()(unsigned n1, unsigned n2) { return _hgraph.getWeight(n1) < _hgraph.getWeight(n2); }
};

struct HGNodeSortByAscendingDegree {
        bool operator()(const HGFNode* ptr1, const HGFNode* ptr2) const { return ptr1->getDegree() < ptr2->getDegree(); }
};

struct HGNodeSortByDescendingDegree {
        bool operator()(const HGFNode* ptr1, const HGFNode* ptr2) const { return ptr1->getDegree() > ptr2->getDegree(); }
};

struct HGNodeIdSortByDegrees {
        const HGraphFixed& _graph;
        HGNodeIdSortByDegrees(const HGraphFixed& graph) : _graph(graph) {}

        bool operator()(unsigned lhs, unsigned rhs) const { return _graph.getNodeByIdx(lhs).getDegree() < _graph.getNodeByIdx(rhs).getDegree(); }
};

struct HGEdgeSortByIndex {
        bool operator()(const HGFEdge* ptr1, const HGFEdge* ptr2) const { return ptr1->getIndex() < ptr2->getIndex(); }
};

struct HGEdgeSortByAscendingDegree {
        bool operator()(const HGFEdge* ptr1, const HGFEdge* ptr2) const { return ptr1->getDegree() < ptr2->getDegree(); }
};

struct HGEdgeSortByDescendingDegree {
        bool operator()(const HGFEdge* ptr1, const HGFEdge* ptr2) const { return ptr1->getDegree() > ptr2->getDegree(); }
};

#endif
