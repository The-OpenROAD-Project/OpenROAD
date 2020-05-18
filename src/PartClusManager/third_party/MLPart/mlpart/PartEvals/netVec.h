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

// created by Igor Markov on 05/20/98

#ifndef __NETVEC_H__
#define __NETVEC_H__

#include "talliesWConfigIds.h"

const unsigned MaxPartitionsForGenericNetVector = 10;
const unsigned TwoToTheMaxPartitionsForGenericNetVector = 1024;

class NetVecGeneric : public TalliesWConfigIds {

       protected:
        unsigned _tableSize;  // 2**_nParts

       public:
        NetVecGeneric(const PartitioningProblem& problem, const Partitioning& part, unsigned terminalsCountAs = 0);

        NetVecGeneric(const PartitioningProblem& problem, unsigned terminalsCountAs = 0);

        NetVecGeneric(const HGraphFixed& hg, const Partitioning& part, unsigned nParts, unsigned terminalsCountAs = 0);

        virtual ~NetVecGeneric() {};

        virtual void reinitialize() {
                TalliesWConfigIds::reinitialize();
                updateAllCosts();
        }

        // can be slow -- only called at startup
        virtual unsigned computeCostToCache(unsigned config) = 0;

        // This method must be very fast. Will use _movbleConfigIds,
        // _fixedConfigIds
        // as well as  _costTable via PartitionIds::getUnsigned()

        // virtual inline unsigned computeCostOfOneNet(unsigned netIdx) = 0;
        // example:
        // { return _costTable[_movblConfigIds[netIdx].getUnsigned()]; }
};

#endif
