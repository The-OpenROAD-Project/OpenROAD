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

#ifndef __TALLIESWCOSTS2WAY_H__
#define __TALLIESWCOSTS2WAY_H__

#include "netTallies2way.h"
#include <iostream>

class TalliesWCosts2way : public NetTallies2way
                          // not a functional evaluator
                          // only a maintenance facility with generic cost evaluation
                          {

        void reinitializeProper() {};

       protected:
        unsigned _totalCost;
        uofm::vector<unsigned> _netCosts;  // at some point we can take this
                                           // away to make
                                           // a truly abstract class(children can use doubles)

       public:
        // ctors call reinitializeProper()
        TalliesWCosts2way(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = 0);
        TalliesWCosts2way(const PartitioningProblem&, unsigned terminalsCountAs = 0);
        TalliesWCosts2way(const HGraphFixed&, const Partitioning&, unsigned terminalsCountAs = 0);

        virtual ~TalliesWCosts2way() {};

        // allows for replication/propagation of any module
        void reinitialize() {
                NetTallies2way::reinitialize();
                reinitializeProper();
        }

        virtual void updateAllCosts();
        // derived classes need to call it in the end of reinitializeProper()

        // Net cost computation to be overriden in derived classes
        // reminder: partEvalXFace.h declares
        // virtual inline unsigned computeCostOfOneNet(unsigned netIdx) const =
        // 0;
        virtual inline double computeCostOfOneNetDouble(unsigned netIdx) const { return computeCostOfOneNet(netIdx); }

        // Re-evaluation from scratch. Sets the internal net cost
        // and updates total cost by delta with the prev. net cost.
        // Assumes the prev. total cost was consistent with stored net costs.
        // To be overriden in derived classes.
        virtual inline void recomputeCostOfOneNet(unsigned netIdx) {
                unsigned newCost, oldCost = _netCosts[netIdx];
                _netCosts[netIdx] = newCost = computeCostOfOneNet(netIdx);
                _totalCost += (newCost - oldCost);
        }

        // Incremental net cost update. Sets the internal net cost
        // and updates total cost by delta with the prev. net cost.
        // Assumes the prev. total cost was consistent with stored net costs.
        // To be overriden in derived classes.
        // Can be implemented with  recomputeCostOfOneNet() above.

        unsigned getTotalCost() const {
                abkassert2(_totalCost != UINT_MAX, "Total cost uninitialized: ", "check if tables, e.g. net-vectors, have to be built\n");
                return _totalCost;
        }
        double getTotalCostDouble() const { return getTotalCost(); }
        unsigned getNetCost(unsigned netId) const { return _netCosts[netId]; }
        double getNetCostDouble(unsigned netId) const { return getNetCost(netId); }

        virtual std::ostream& prettyPrint(std::ostream&) const;
};

// this way, op<< can be ``inherited'' w/o pain
inline std::ostream& operator<<(std::ostream& os, const TalliesWCosts2way& nt) { return nt.prettyPrint(os); }

#endif
