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

// created by Andrew Caldwell and Igor Markov on 05/17/98

#ifndef __NETCUTWBITS_H__
#define __NETCUTWBITS_H__

#include "partEvalXFace.h"
#include <iostream>

class NetCutWBits : public PartEvalXFace {

        unsigned _totalCost;
        uofm::vector<unsigned> _netCosts;

        void reinitializeProper();
        unsigned computeNetCost(const HGFEdge&) const;

       public:
        // ctors call reinitializeProper()
        NetCutWBits(const PartitioningProblem&, const Partitioning&);
        NetCutWBits(const PartitioningProblem&);
        NetCutWBits(const HGraphFixed&, const Partitioning&);

        unsigned getNetCost(unsigned netId) const { return _netCosts[netId]; }
        double getNetCostDouble(unsigned netId) const { return getNetCost(netId); }

        unsigned computeCostOfOneNet(unsigned netId) const { return computeNetCost(_hg.getEdgeByIdx(netId)); }
        double computeCostOfOneNetDouble(unsigned netId) const { return computeCostOfOneNet(netId); }

        // "get cost" means "get precomputed cost"
        unsigned getTotalCost() const { return _totalCost; }
        double getTotalCostDouble() const { return getTotalCost(); }

        void reinitialize() {
                PartEvalXFace::reinitialize();
                reinitializeProper();
        }

        void moveModuleTo(unsigned moduleNumber);

        void moveModuleTo(unsigned moduleNumber, unsigned)  // "to"
        {
                moveModuleTo(moduleNumber);
        }
        void moveModuleTo(unsigned moduleNumber, PartitionIds)  // "new"
        {
                moveModuleTo(moduleNumber);
        }
        void moveModuleTo(unsigned moduleNumber, unsigned, unsigned)  // "from, to"
        {
                moveModuleTo(moduleNumber);
        }
        void moveModuleTo(unsigned moduleNumber, PartitionIds, PartitionIds) { moveModuleTo(moduleNumber); }         // "old, new"
        void moveWithReplication(unsigned moduleNumber, PartitionIds, PartitionIds) { moveModuleTo(moduleNumber); }  // "old, new"

        unsigned getMaxCostOfOneNet() const { return 1; }

        virtual bool isNetCut() const { return true; }

        friend std::ostream& operator<<(std::ostream&, const NetCutWBits&);
};

std::ostream& operator<<(std::ostream&, const NetCutWBits&);

inline unsigned NetCutWBits::computeNetCost(const HGFEdge& edge) const {
        itHGFNodeLocal n = edge.nodesBegin();
        PartitionIds commonPartitions = (*_part)[(*(n++))->getIndex()];

        for (; n != edge.nodesEnd(); n++) {
                commonPartitions &= (*_part)[(*n)->getIndex()];
                if (commonPartitions.isEmpty()) return 1;
        }

        // if there exists a partition which each partitionIds has
        // set, the bit-wise and of all of them will be != 0, and
        // the net is not cut. Otherwise, it is.

        return (commonPartitions.isEmpty() ? 1 : 0);
}

inline void NetCutWBits::moveModuleTo(unsigned moduleNumber) {
        HGFNode n = _hg.getNodeByIdx(moduleNumber);
        itHGFEdgeLocal e = n.edgesBegin();
        unsigned deltaCost = 0, netId;
        for (; e != n.edgesEnd(); e++) {
                unsigned tmp = computeNetCost(**e);
                netId = (*e)->getIndex();
                deltaCost += (tmp - _netCosts[netId]);
                _netCosts[netId] = tmp;
        }
        _totalCost += deltaCost;
}

#endif
