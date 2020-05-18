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

// created by Igor Markov on 06/07/98

#ifndef __NETTALLIES2WAY_H__
#define __NETTALLIES2WAY_H__

#include "Ctainers/umatrix.h"
#include "partEvalXFace.h"
#include <iostream>

class NetTallies2way : public PartEvalXFace
                       // not a functional eval
                       // only a maintenance facility w/o cost evaluation
                       {
        void reinitializeProper();

       protected:
        unsigned _terminalsCountAs;
        unsigned short* _tallies;

       public:
        // ctors call reinitializeProper()

        NetTallies2way(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = 0);
        // optional parameter to be used by derived classes
        NetTallies2way(const PartitioningProblem&, unsigned terminalsCountAs = 0);

        NetTallies2way(const HGraphFixed&, const Partitioning&, unsigned terminalsCountAs = 0);

        virtual ~NetTallies2way() {
                delete[] _tallies;
        };

        const unsigned short* getTallies() const { return _tallies; }

        // allows for replication/propagatoin of any module
        void reinitialize() { reinitializeProper(); }

        // no replication; calls updateNetForMovedModule() below
        virtual inline void moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to);

        virtual inline void moveModuleTo(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) { moveModuleTo(moduleNumber, oldVal.lowestNumPart(), newVal.lowestNumPart()); }

        // these methods update net tally,
        // in child classes must also update whatever else is maintained
        // (but not the cost of the net)

        virtual inline void recomputeCostOfOneNet(unsigned netIdx) {
                abkfatal(0,
                         "This function is purely virtual and should never be "
                         "called.");
        }

        // single-module move
        virtual inline void updateNetForMovedModule(unsigned netIdx, unsigned from, unsigned)  // to
                                                                                               // don't check for trivial moves as they aren't likely
        {
                unsigned short* pt = _tallies + 2 * netIdx;
                if (from == 0) {
                        (*pt)--, (*(pt + 1))++;
                } else {
                        (*pt)++, (*(pt + 1))--;
                }
        }

        friend std::ostream& operator<<(std::ostream& os, const NetTallies2way&);

        unsigned terminalsCountAs() const { return _terminalsCountAs; }

        virtual std::ostream& prettyPrint(std::ostream&) const;

        void moveWithReplication(unsigned, PartitionIds, PartitionIds) { abkfatal(0, "Not defined yet"); }
};

// this way, op<< can be ``inherited'' w/o pain
inline std::ostream& operator<<(std::ostream& os, const NetTallies2way& nt) { return nt.prettyPrint(os); }

inline void NetTallies2way::moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to)
    // no replication allowed
{
        const HGFNode& n = _hg.getNodeByIdx(moduleNumber);

        for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) {
                unsigned netIdx = (*e)->getIndex();
                updateNetForMovedModule(netIdx, from, to);
                recomputeCostOfOneNet(netIdx);
        }
}

#endif
