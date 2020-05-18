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

//! author="Igor Markov on 08/27/98"

#ifndef __Accelerated_MOVE_MGR_XFACE_H__
#define __Accelerated_MOVE_MGR_XFACE_H__

#include "Partitioning/partitioning.h"

//: Interface for accelerated move manager
class AcceleratedMoveManagerInterface {
       protected:
        const PartitioningProblem& _problem;
        // use this only for info not available otherwise
        // Never const_cast _problem !
        PartitioningSolution* _partSol;
        Partitioning* _curPart;
        const Partitioning* _fixedConstr;

       public:
        bool randomized;

        AcceleratedMoveManagerInterface(const PartitioningProblem& problem) : _problem(problem), _partSol(NULL), _curPart(NULL), _fixedConstr(&problem.getFixedConstr()), randomized(false) {};

        virtual ~AcceleratedMoveManagerInterface() {}

        virtual unsigned doOnePass() = 0; /* returns #movesmade */

        virtual unsigned getCost() const = 0;
        virtual double getCostDouble() const = 0;

        virtual void resetTo(PartitioningSolution& newSol) = 0;

        PartitioningSolution& getSolution() { return *_partSol; }

        //-'status' information highly depends on the specific MoveManager,
        // thus we can't define it here.

        virtual double getViolation() const = 0;
        virtual double getImbalance() const = 0;
        typedef const uofm::vector<double>& rConstVecDbl;
        virtual rConstVecDbl getAreas() const = 0;
};

#endif
