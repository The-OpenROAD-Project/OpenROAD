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

//! author="Andrew Caldwell and Igor Markov on 05/14/98"

#ifndef __MOVE_MGR_TBASE_H__
#define __MOVE_MGR_TBASE_H__

#include "Partitioning/partitioning.h"
#include "moveMgrXFace.h"
#include "PartLegality/1balanceChk.h"
#include <iostream>

//: Base template for move manager
template <class Evaluator, class GainContainer>
class MoveManagerTemplateBase : public MoveManagerInterface {
       protected:
        Evaluator _eval;
        GainContainer _gainCo;

        SingleBalanceLegality _partLeg;

       public:
        MoveManagerTemplateBase(const PartitioningProblem& problem) : MoveManagerInterface(problem), _eval(problem), _gainCo(problem.getHGraph().getNumNodes(), problem.getNumPartitions()), _partLeg(problem) {}
        /*
           Need to resetTo(PartitioningSolution&) befor every pass
           The follwoing interface is getting obsolete as it entails duplication
        */
        MoveManagerTemplateBase(const PartitioningProblem& problem, PartitioningSolution& partSol) : MoveManagerInterface(problem, partSol), _eval(problem, partSol.part), _gainCo(problem.getHGraph().getNumNodes(), problem.getNumPartitions()), _partLeg(problem, partSol.part) {}

        virtual ~MoveManagerTemplateBase() {}

        /*  as defined in base class:
            virtual void pickMoveApplyIt()        = 0;
        */
        virtual unsigned getCost() const { return _eval.getTotalCost(); }
        virtual double getCostDouble() const { return getCost(); }

        virtual void reinitialize() {
                _partLeg.reinitialize();
                _eval.reinitialize();
        }
        // cause the evaluator to reinitialize it's cached data

        virtual void reinitTolerances() {};  // reinitialize tolerances

        /*
          virtual void     resetTo(PartitioningSolution& newSol)
            { _partSol = &newSol;
              _curPart = &(newSol.part);
              _partLeg.resetTo(newSol.part);
              _eval.resetTo(newSol.part);
            }
        */

        /* -'status' information highly depends on the specific MoveManager,
           thus we can't define it here

           KL-type move managers will define
           virtual void undoMoves(unsigned k) = 0; */

        virtual void prettyPrint(std::ostream& out) const { out << "MoveManagerTemplateBase:\n Evaluator " << _eval << " Gain Container " << _gainCo << std::endl; }

        virtual double getImbalance() const { return _partLeg.getDiffFromIdeal(); }
        virtual double getViolation() const { return _partLeg.getViolation(); }
        virtual rConstVecDbl getAreas() const { return _partLeg.getActualBalances(); }
};

template <class Evaluator, class GainContainer>
inline std::ostream& operator<<(std::ostream& out, const MoveManagerTemplateBase<Evaluator, GainContainer>& mm) {
        mm.prettyPrint(out);
        return out;
}

#endif
