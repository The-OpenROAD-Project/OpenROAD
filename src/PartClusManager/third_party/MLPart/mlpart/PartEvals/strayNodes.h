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

// created by Igor Markov on 06/02/98

#ifndef __MODIFIEDNETCUT_H_
#define __MODIFIEDNETCUT_H_

#include <iostream>
#include "talliesWCosts.h"

static const unsigned forDefaultStrayNodesTerminalsCountAs = 2;

class StrayNodes : public TalliesWCosts {
        void reinitializeProper();
        void findBiggestPart(unsigned netIndex) {
                unsigned maxPartSize = _tallies(netIndex, 0);
                unsigned maxPart = 0;
                unsigned partSize = _tallies(netIndex, 1);
                if (partSize > maxPartSize) {
                        maxPartSize = partSize;
                        maxPart = 1;
                }

                for (unsigned i = 2; i < _nParts; i++) {
                        unsigned pSize = _tallies(netIndex, i);
                        if (pSize > maxPartSize) {
                                maxPartSize = pSize;
                                maxPart = i;
                        }
                }
                _maxPartSize[netIndex] = maxPartSize;
                _maxPart[netIndex] = maxPart;
        }

       protected:
        uofm::vector<unsigned> _netDegree;
        uofm::vector<unsigned> _maxPartSize;
        uofm::vector<unsigned> _maxPart;
        unsigned _maxNetCost;

       public:
        // ctors call reinitializeProper()
        StrayNodes(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = forDefaultStrayNodesTerminalsCountAs);

        StrayNodes(const PartitioningProblem&, unsigned terminalsCountAs = forDefaultStrayNodesTerminalsCountAs);

        void reinitialize() {
                TalliesWCosts::reinitialize();
                reinitializeProper();
        }

        virtual inline unsigned computeCostOfOneNet(unsigned netIdx) const { return _netDegree[netIdx] - _maxPartSize[netIdx]; }

        void updateNetForMovedModule(unsigned netIdx, unsigned from, unsigned to)
            // don't check for trivial moves as they aren't likely
        {
                netLostModule(netIdx, from);
                netGotModule(netIdx, to);
                if (to == _maxPart[netIdx])
                        _maxPartSize[netIdx]++;
                else
                        findBiggestPart(netIdx);
        }

        virtual unsigned getMaxCostOfOneNet() const { return _maxNetCost; }

        virtual std::ostream& prettyPrint(std::ostream&) const;
};

inline std::ostream& operator<<(std::ostream& os, const StrayNodes& mcut) { return mcut.prettyPrint(os); }

#endif
