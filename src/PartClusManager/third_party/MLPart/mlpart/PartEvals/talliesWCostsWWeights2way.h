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

#ifndef __TALLIESWCOSTS_WWEIGHTS_2WAY_H__
#define __TALLIESWCOSTS_WWEIGHTS_2WAY_H__

#include <iostream>
#include "talliesWCosts2way.h"

class TalliesWCostsWWeights2way : public TalliesWCosts2way
                                  // not a functional evaluator
                                  // only a maintenance facility with generic cost evaluation
                                  {
        void reinitializeProper() {};

       protected:
        uofm::vector<unsigned> _netWeights;
        unsigned _maxNetWeight;
        void setNetWeights();

       public:
        // ctors call reinitializeProper()
        TalliesWCostsWWeights2way(const PartitioningProblem&, const Partitioning&, unsigned terminalsCountAs = 0);
        TalliesWCostsWWeights2way(const PartitioningProblem&, unsigned terminalsCountAs = 0);
        TalliesWCostsWWeights2way(const HGraphFixed&, const Partitioning&, unsigned terminalsCountAs = 0);

        unsigned getNetWeight(unsigned netIdx) const { return _netWeights[netIdx]; }

        virtual ~TalliesWCostsWWeights2way() {};

        void reinitialize() { TalliesWCosts2way::reinitialize(); }

        //   derived classes need to call
        // virtual void TalliesWCosts2way::updateAllCosts();
        //   in the end of reinitializeProper()

        virtual std::ostream& prettyPrint(std::ostream&) const;
};

// this way, op<< can be ``inherited'' w/o pain
inline std::ostream& operator<<(std::ostream& os, const TalliesWCostsWWeights2way& nt) { return nt.prettyPrint(os); }

#endif
