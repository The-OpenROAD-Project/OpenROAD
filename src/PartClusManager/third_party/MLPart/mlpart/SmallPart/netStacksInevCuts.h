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

// Created by Mike Oliver on 20 mar 1999
// (earlier work by Igor Markov)

//////////////////////////////////////////////////////////////////////
//
// netStacksInevCuts.h: interface for the NetStacksInevCuts class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NETSTACKSINEVCUTS_H_INCLUDED_
#define _NETSTACKSINEVCUTS_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000

#include "netStacks.h"

// WARNING!
// WARNING!
// WARNING!

// do *not* use objects of this class in any context
// where the exact class is not known at compile time.
// I.e. do not do this:
//
// NetStacks *pStack = new NetStacksInevCuts;
//
// The reason is that assignNode() and unassignNode()
// are not virtual (for performance reasons).  In the
// above code, when you call pStack->assignNode(), it
// will call the wrong version, and you will get wrong
// answers.

class NetStacksInevCuts : public NetStacks {
        friend class CompareByNode;
        friend std::ostream& operator<<(std::ostream&, const NetStacksInevCuts&);

       private:
        struct nodeAndWeight {
                unsigned node;  // index in movables
                unsigned weight;
        };

       protected:
        // for movable node k, _firstNodes[k] is a
        // pointer to the element of _nodes representing
        // the first node to which movable k has a forward edge
        nodeAndWeight** _firstNodes;

        // _nodeTallies[0][k] is the (weighted) number of smaller-indexed
        // nodes with which movable k has an edge, and which
        // are currently assigned to partition 0. mutatis
        // mutandis for _nodeTallies[1]
        unsigned* _nodeTallies[2];

        // elements of _nodes are the "other" nodes
        // in a degree-2 edge, with the multiplicities
        // represented by the "weight" member.  Each edge is represented
        // only once, from the smaller-indexed to the greater-indexed
        // node.  Therefore edges involving a fixed node are not
        // represented at all; they're taken care of by _nodeTallies.

        // Note that the "node" members of the elements of _nodes are indices
        // in movables, not in the original hypergraph.  That's
        // OK because fixed nodes will never show up here (as
        // fixed nodes always come *before* movable ones, and
        // we only represent the "forward" edges (smaller to
        // greater node index).
        nodeAndWeight* _nodes;

        mutable nodeAndWeight* _nodeCounter;

        virtual void _trivInit(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);

        virtual void _perMovableInit(unsigned k);

        virtual void _procEdge(const HGFEdge& edge, const Partitioning& part, unsigned movableIdx);

        virtual void _finalize(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);

       public:
        NetStacksInevCuts(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);

        NetStacksInevCuts();
        // return value is new cut
        inline unsigned assignNode(unsigned idx, unsigned whereTo);
        inline void unAssignNode(unsigned idx, unsigned whereFrom);

        // c1-c0, where cN is increased
        // cut for assigning movable
        // node idx to partition N.
        inline signed int cutDifference(unsigned idx) const;
        virtual ~NetStacksInevCuts();
        void printState(const uofm::vector<unsigned>& movables) const;
        void consolidate();  // collapse multiedges to edges w/weights
};

std::ostream& operator<<(std::ostream&, const NetStacksInevCuts&);

inline unsigned NetStacksInevCuts::assignNode(unsigned idx, unsigned whereTo) {
        NetStacks::_assignNodeInner(idx, whereTo);

        unsigned& cut = _cuts.back();
        unsigned* n0 = _nodeTallies[0], *n1 = _nodeTallies[1];
        const nodeAndWeight* pt;

        int c0 = n0[idx], c1 = n1[idx];
        unsigned other = 1 - whereTo;

        // the min of c0,c1 has already been added in as an inev cut.
        // need add difference for actual partition to which node assigned
        cut += ((other == 0) ? c0 : c1) - std::min(c0, c1);

        for (pt = _firstNodes[idx]; pt != _firstNodes[idx + 1]; pt++) {
                const nodeAndWeight& nw = *pt;
                unsigned ptdTo = nw.node;
                unsigned oldMinCut = std::min(n0[ptdTo], n1[ptdTo]);
                _nodeTallies[whereTo][ptdTo] += nw.weight;
                cut += std::min(n0[ptdTo], n1[ptdTo]) - oldMinCut;
        }
        return cut;
}

inline void NetStacksInevCuts::unAssignNode(unsigned idx, unsigned whereFrom) {
        nodeAndWeight* pt;
        for (pt = _firstNodes[idx]; pt != _firstNodes[idx + 1]; pt++) {
                nodeAndWeight& nw = *pt;
                unsigned ptdTo = nw.node;
                _nodeTallies[whereFrom][ptdTo] -= nw.weight;
        }
        NetStacks::unAssignNode(idx, whereFrom);
}

inline signed NetStacksInevCuts::cutDifference(unsigned idx) const {
        unsigned* n0 = _nodeTallies[0], *n1 = _nodeTallies[1];

        signed retval = signed(n0[idx]) - signed(n1[idx]);

        for (nodeAndWeight* pt = _firstNodes[idx]; pt != _firstNodes[idx + 1]; pt++) {
                unsigned ptdTo = pt->node;
                signed wt = pt->weight;
                signed talDif = signed(n0[ptdTo]) - signed(n1[ptdTo]);
                if (talDif >= wt) {
                        retval += wt;
                } else if (talDif >= -wt) {
                        retval += talDif;
                } else {
                        retval -= wt;
                }
        }
        return retval + NetStacks::cutDifference(idx);
}
#endif  // !defined(_NETSTACKSINEVCUTS_H_INCLUDED_)
