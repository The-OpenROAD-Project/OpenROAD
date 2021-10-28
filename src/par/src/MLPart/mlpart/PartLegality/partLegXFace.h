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

// created by Igor Markov on 05/26/98

#ifndef __PART_LEG_XFACE_H__
#define __PART_LEG_XFACE_H__

#include "Partitioning/partProb.h"

// Note: derived classes should only be used when the precise class
//       is clear (not to compile any methods as virtual)

class PartLegalityXFace {
       protected:
        Partitioning* _part;  // maintained by caller, or is NULL

        const PartitioningProblem* _problem;  // use only to get info
                                              // not available otherwise
        const HGraphFixed& _hg;

       public:
        PartLegalityXFace(const PartitioningProblem& problem, Partitioning& part) : _part(&part), _problem(&problem), _hg(problem.getHGraph()) {};

        PartLegalityXFace(const PartitioningProblem& problem) : _part(NULL), _problem(&problem), _hg(problem.getHGraph()) {};

        virtual ~PartLegalityXFace() {};

        virtual bool isPartLegal() const = 0;  // check the state
        // may be slow for single-cell moves in k-way partitioning

        virtual double getViolation() const = 0;  // must be 0 if legal
        virtual double getDiffFromIdeal() const = 0;
        virtual unsigned getUnderfilledPartition() const = 0;

        // Reinitializes the leg-ty checker if the partition changed
        // "non-incrementally"
        // Must be redefined in derived classes by calling
        // same method in the immediate parent (with scopre resolution operator
        // ::)

        virtual void reinitialize();

        virtual void reinitBalances();  // reinitializes the min/max part
                                        // balances frm pbm

        virtual void resetTo(Partitioning& newPart) {
                _part = &newPart;
                reinitialize();
        }

        // these do not change the state and are fast
        // return true if the move does not make things worse
        virtual bool isLegalMove(unsigned moduleNumber, unsigned from, unsigned to) const = 0;
        virtual bool isLegalMove(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) const = 0;
        virtual bool isLegalMoveWithReplication(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) const = 0;

        // These represent incremental update by one move (with cost updates),
        // return true if the moved cell does not make things worse
        // when inlining, the compiler should detect when the value is not used

        virtual bool moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to) = 0;
        virtual bool moveModuleTo(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) = 0;
        virtual bool moveWithReplication(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) = 0;

        virtual unsigned balance(const Partitioning& fixedConstr) {
                abkfatal(0, " Not implemented ");
                (void)fixedConstr;
                return 0; /*#moves}*/
        }

        virtual unsigned enforce(const Partitioning& fixedConstr) {
                abkfatal(0, " Not implemented ");
                (void)fixedConstr;
                return 0; /*#moves*/
        }
};

#endif
