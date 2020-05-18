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

#ifndef __TALLIESWCONFIGIDS_H__
#define __TALLIESWCONFIGIDS_H__

#include "talliesWCosts.h"
#include <iostream>

class TalliesWConfigIds : public TalliesWCosts {
        // derived classes should call updateAllCosts()
        // in the end of reinitializeProper()
        void reinitializeProper();

       protected:
        Partitioning _fixedConfigIds;  // intersections (for each net)
        Partitioning _movblConfigIds;  // consistent at all times

       public:
        // ctors call reinitializeProper()
        TalliesWConfigIds(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = 0);
        TalliesWConfigIds(const PartitioningProblem&, unsigned terminalsCountAs = 0);
        TalliesWConfigIds(const HGraphFixed&, const Partitioning&, unsigned nParts, unsigned terminalsCountAs = 0);

        virtual ~TalliesWConfigIds() {};

        void reinitialize() {
                TalliesWCosts::reinitialize();
                reinitializeProper();
        }

        // these will be overriden only if something else is maintained
        virtual inline void netLostModule(unsigned netIdx, unsigned fromPartition) {
                if ((--_tallies(netIdx, fromPartition)) == 0) _movblConfigIds[netIdx].removeFromPart(fromPartition);
        }

        virtual inline void netGotModule(unsigned netIdx, unsigned inPartition) {
                _tallies(netIdx, inPartition)++;
                _movblConfigIds[netIdx].addToPart(inPartition);
        }

        virtual std::ostream& prettyPrint(std::ostream&) const;
};

// this way, op<< can be ``inherited'' w/o pain
inline std::ostream& operator<<(std::ostream& os, const TalliesWConfigIds& t) { return t.prettyPrint(os); }

#endif
