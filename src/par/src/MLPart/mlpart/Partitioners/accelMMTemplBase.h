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

#ifndef __ACCELERATED_MOVE_MGR_TBASE_H__
#define __ACCELERATED_MOVE_MGR_TBASE_H__

#include <iostream>
#include "Partitioning/partitioning.h"
#include "accelMMXFace.h"
#include "PartLegality/1balanceChk.h"

//: Base template for accelerated move manager
template <class Evaluator>
class AcceleratedMoveManagerTemplateBase : public AcceleratedMoveManagerInterface {
       protected:
        Evaluator _eval;
        SingleBalanceLegality _partLeg;

       public:
        AcceleratedMoveManagerTemplateBase(const PartitioningProblem& problem) : AcceleratedMoveManagerInterface(problem), _eval(problem), _partLeg(problem) {}

        virtual ~AcceleratedMoveManagerTemplateBase() {}

        virtual unsigned getCost() const { return _eval.getTotalCost(); }
        virtual double getCostDouble() const { return getCost(); }

        virtual void resetTo(PartitioningSolution& newSol) {
                _partSol = &newSol;
                _curPart = &(newSol.part);
                _partLeg.resetTo(newSol.part);
                _eval.resetTo(newSol.part);
        }

        virtual void prettyPrint(std::ostream& out) const { out << "AcceleratedMoveManagerTemplateBase:\n Evaluator " << _eval; }

        virtual double getImbalance() const { return _partLeg.getDiffFromIdeal(); }
        virtual double getViolation() const { return _partLeg.getViolation(); }
        virtual rConstVecDbl getAreas() const { return _partLeg.getActualBalances(); }
};

template <class Evaluator>
inline std::ostream& operator<<(std::ostream& out, const AcceleratedMoveManagerTemplateBase<Evaluator>& mm) {
        mm.prettyPrint(out);
        return out;
}

#endif
