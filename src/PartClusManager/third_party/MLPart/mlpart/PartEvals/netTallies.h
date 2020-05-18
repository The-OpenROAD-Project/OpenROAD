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

// created by Igor Markov on 05/17/98

#ifndef __NETTALLIES_H__
#define __NETTALLIES_H__

#include <iostream>
#include "Ctainers/umatrix.h"
#include "partEvalXFace.h"

class NetTallies : public PartEvalXFace
                   // not a functional partitioner,
                   // only a maintenance facility w/o cost evaluation
                   {
        void reinitializeProper();

       protected:
        unsigned _terminalsCountAs;
        unsigned _nParts;
        UDenseMatrix _tallies;

       public:
        // ctors call reinitializeProper()

        NetTallies(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = 0);
        // optional parameter to be used by derived classes
        NetTallies(const PartitioningProblem&, unsigned terminalsCountAs = 0);

        NetTallies(const HGraphFixed&, const Partitioning&, unsigned nParts, unsigned terminalsCountAs = 0);

        virtual ~NetTallies() {};

        const UDenseMatrix& getTallies() const { return _tallies; }

        // allows for replication/propagatoin of any module
        void reinitialize() { reinitializeProper(); }

        // no replication; calls updateNetForMovedModule() below
        virtual void moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to);
        virtual void moveModuleTo(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) { moveModuleTo(moduleNumber, oldVal.lowestNumPart(), newVal.lowestNumPart()); }

        // these methods update net tally,
        // in child classes must also update whatever else is maintained
        // (but not the cost of the net)

        virtual inline void recomputeCostOfOneNet(unsigned netIdx) {
                abkfatal(0,
                         "This function is purely virtual and should never be "
                         "called.");
        }

        virtual inline void netLostModule(unsigned netIdx, unsigned fromPartition) { _tallies(netIdx, fromPartition)--; }

        virtual inline void netGotModule(unsigned netIdx, unsigned inPartition) { _tallies(netIdx, inPartition)++; }

        // single-module move
        virtual inline void updateNetForMovedModule(unsigned netIdx, unsigned from, unsigned to)
            // don't check for trivial moves as they aren't likely
        {
                netLostModule(netIdx, from);
                netGotModule(netIdx, to);
        }

        // allows for replication both at source and destination parts
        // (calls two of the above methods)
        virtual void moveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal);

        friend std::ostream& operator<<(std::ostream& os, const NetTallies&);

        unsigned terminalsCountAs() const { return _terminalsCountAs; }

        virtual std::ostream& prettyPrint(std::ostream&) const;
};

// this way, op<< can be ``inherited'' w/o pain
inline std::ostream& operator<<(std::ostream& os, const NetTallies& nt) { return nt.prettyPrint(os); }

inline void NetTallies::moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to)
    // no replication allowed
{
        const HGFNode& n = _hg.getNodeByIdx(moduleNumber);

        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                unsigned netIdx = (*e)->getIndex();
                updateNetForMovedModule(netIdx, from, to);
                recomputeCostOfOneNet(netIdx);
        }
}

inline void NetTallies::moveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal)
    // replication is allowed both in source and destination partitions
{
        const HGFNode& n = _hg.getNodeByIdx(moduleNumber);

        for (unsigned k = 0; k != _nParts; k++) {
                if (oldVal.isInPart(k)) {
                        if (!newVal.isInPart(k))
                                for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                                        unsigned netIdx = (*e)->getIndex();
                                        netLostModule(netIdx, k);
                                        recomputeCostOfOneNet(netIdx);
                                }
                } else if (newVal.isInPart(k)) {
                        //     if (!oldVal.isInPart(k))
                        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                                unsigned netIdx = (*e)->getIndex();
                                netGotModule(netIdx, k);
                                recomputeCostOfOneNet(netIdx);
                        }
                }
        }
}

#endif
