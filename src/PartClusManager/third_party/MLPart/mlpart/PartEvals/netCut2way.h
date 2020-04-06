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

#ifndef __NETCUT2WAY_H__
#define __NETCUT2WAY_H__

#include <iostream>
#include "talliesWCosts2way.h"

class NetCut2way : public TalliesWCosts2way {

       public:
        NetCut2way(const PartitioningProblem& problem, const Partitioning& part) : TalliesWCosts2way(problem, part, 1) { updateAllCosts(); }

        NetCut2way(const PartitioningProblem& problem) : TalliesWCosts2way(problem, 1) {}

        NetCut2way(const HGraphFixed& hg, const Partitioning& part) : TalliesWCosts2way(hg, part, 1) { updateAllCosts(); }

        void reinitialize() {
                TalliesWCosts2way::reinitialize();
                updateAllCosts();
        }

        unsigned getMaxCostOfOneNet() const { return 1; }

        virtual inline unsigned computeCostOfOneNet(unsigned netIdx) const {
                unsigned short* pt = _tallies + 2 * netIdx;
                if ((*pt) == 0 || (*(pt + 1)) == 0)
                        return 0;
                else
                        return 1;
        }

        virtual inline void moveModuleOnNet(unsigned netIdx, unsigned from, unsigned to) {
                unsigned short* pt = _tallies + 2 * netIdx;
                if (pt[to] == 0) {
                        _netCosts[netIdx] = 1;
                        _totalCost++;
                } else if (pt[from] == 1) {
                        _netCosts[netIdx] = 0;
                        _totalCost--;
                }
                pt[to]++, pt[from]--;
        }

        virtual inline void moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to)
            // no replication allowed
        {
                const HGFNode& n = _hg.getNodeByIdx(moduleNumber);

                for (itHGFEdgeLocal e = n.edgesBegin(); e != n.edgesEnd(); e++) moveModuleOnNet((*e)->getIndex(), from, to);
        }

        virtual void moveModuleTo(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) { moveModuleTo(moduleNumber, oldVal.lowestNumPart(), newVal.lowestNumPart()); }

        virtual bool isNetCut() const { return true; }
        virtual bool isNetCut2way() const { return true; }

        virtual int getDeltaGainDueToNet(unsigned netIdx, unsigned movingFrom, unsigned /*movingTo*/, unsigned gainingFrom, unsigned /*gainingTo*/) const {
                unsigned short* tally = _tallies + 2 * netIdx;
                switch (tally[1 - movingFrom]) {
                        case 0:
                                return ((tally[movingFrom] == 2) ? 2 : 1);
                        case 1:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? 1 : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -2 : -1);
                        default:
                                if (movingFrom == gainingFrom)
                                        return ((tally[movingFrom] == 2) ? 1 : 0);
                                else
                                        return ((tally[movingFrom] == 1) ? -1 : 0);
                }
        }

        virtual bool netCostNotAffectedByMove(unsigned netIdx, unsigned from, unsigned to)
            // should be called before the move is applied
        {
                unsigned short* tally = _tallies + 2 * netIdx;
                return tally[from] >= 3 && tally[to] >= 2;
        }
};
#endif
