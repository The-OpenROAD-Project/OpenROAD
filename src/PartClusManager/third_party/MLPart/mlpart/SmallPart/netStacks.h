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

// Created by Igor Markov on Feb 25, 1999

#ifndef _NETSTACKS_H_
#define _NETSTACKS_H_

#include "Ctainers/riskyStack.h"
#include "Partitioning/partitionData.h"
#include <iostream>

class NetStacks;
class NetStacksPartyStyle;

class HGraphFixed;
class Partitioning;

class NetStacks {
       public:
        struct edgeAndWeight {
                unsigned edge;
                unsigned weight;
        };

       protected:
        bool _populated;

        // each element of _netData represents one hyperedge
        // at some point in the recursion.  the meanings of
        // the values are:
        // 0        = no node on the edge is assigned
        // 1        = some node in partition 0
        // 2        = some node in partition 1
        // UINT_MAX = nodes in both partitions
        unsigned* _netData;

        // _netStacks[k] is a pointer to the element of _netData
        // describing edge k at the current moment in the recursion.
        // (or it could be NULL, which it will be if edge k is
        // not being considered)
        unsigned** _netStacks;

        // _netCheck[k] is equal to the original value of _netStacks[k].
        // used only for debug.
        unsigned** _netCheck;

        // the elements of _nets refer to
        // all nets that figure into the optimization; i.e. those
        // that are not already cut because of the assignments
        // of fixed nodes.
        edgeAndWeight* _nets;

        // _firstNets[k] is a pointer to the first element of
        // _nets that refers to a net touching movable node k
        edgeAndWeight** _firstNets;

        RiskyStack<unsigned> _cuts;
        unsigned _numMovables, _numNodes, _allEdges, _usedEdges, _usedPins;

        // never used outside populate()
        mutable unsigned* _pinCounter1;
        mutable edgeAndWeight* _pinCounter2;
        mutable uofm_bit_vector _traversed;
        mutable uofm::vector<unsigned> _mapBack;
        mutable Partitioning _netIds;

        virtual void _trivInit(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);

        virtual void _perMovableInit(unsigned k);

        virtual void _procEdge(const HGFEdge& edge, const Partitioning& part, unsigned movableIdx);

        virtual void _finalize(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);

        // called by assignNode
        inline void _assignNodeInner(unsigned idx, unsigned whereTo);

       public:
        const PartitionIds inNeither, inBoth;

        NetStacks();
        NetStacks(const HGraphFixed& hg, const Partitioning& part, const uofm::vector<unsigned>& movables);
        void populate(const HGraphFixed&, const Partitioning& part, const uofm::vector<unsigned>& movables);
        virtual ~NetStacks();
        unsigned getCut() const { return _cuts.back(); }
        unsigned getUsedEdges() const { return _usedEdges; }

        // return value is new cut
        inline unsigned assignNode(unsigned idx, unsigned whereTo);
        inline void unAssignNode(unsigned idx, unsigned whereFrom);

        // c1-c0, where cN is increased
        // cut for assigning movable
        // node idx to partition N.
        inline signed cutDifference(unsigned idx) const;

        unsigned getSize() const { return _cuts.size(); }

        friend std::ostream& operator<<(std::ostream&, const NetStacks&);
};

std::ostream& operator<<(std::ostream&, const NetStacks&);

inline void NetStacks::_assignNodeInner(unsigned idx, unsigned whereTo) {
        // cout << "Assigning node " << idx << " to " << whereTo << endl;
        _cuts.push_back(_cuts.back());
        unsigned other = 1 - whereTo;
        for (edgeAndWeight* pt = _firstNets[idx]; pt != _firstNets[idx + 1]; pt++) {
                typedef unsigned* up;
                up& netStackPtr = _netStacks[pt->edge];
                if (*netStackPtr == 1 + other)  // the net gets cut
                {
                        *(++netStackPtr) = UINT_MAX;
                        _cuts.back() += pt->weight;
                } else {
                        if (*netStackPtr == 0)
                                *++netStackPtr = 1 + whereTo;
                        else /* to or UINT_MAX */
                        {
                                netStackPtr[1] = netStackPtr[0];
                                ++netStackPtr;
                        }
                }
        }
}

inline unsigned NetStacks::assignNode(unsigned idx, unsigned whereTo) {
        _assignNodeInner(idx, whereTo);
        return _cuts.back();
}

inline void NetStacks::unAssignNode(unsigned idx, unsigned) {
        // cout << "Unassigning node " << idx << " from " << whereFrom << endl;
        for (edgeAndWeight* pt = _firstNets[idx]; pt != _firstNets[idx + 1]; pt++) --_netStacks[pt->edge];
        _cuts.pop_back();
}

inline signed NetStacks::cutDifference(unsigned idx) const {
        signed retval = 0;
        for (edgeAndWeight* pt = _firstNets[idx]; pt != _firstNets[idx + 1]; pt++) {
                switch (*_netStacks[pt->edge]) {
                        case 1:
                                retval += pt->weight;
                                break;
                        case 2:
                                retval -= pt->weight;
                }
        }
        return retval;
}

#endif
