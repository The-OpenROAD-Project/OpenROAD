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

#ifndef __1BALANCECHKWPins_H__
#define __1BALANCECHKWPins_H__

#include "1balanceChk.h"

// Note: derived classes should only be used when the precise class
//       is clear (not to compile any methods as virtual)

class SingleBalanceLegalityWPins : public SingleBalanceLegality {
       protected:
        uofm::vector<unsigned> _actualPinBalances;
        unsigned _totalPins;
        double _maxPinDensity;

       public:
        SingleBalanceLegalityWPins(const PartitioningProblem& problem, Partitioning& part);
        SingleBalanceLegalityWPins(const PartitioningProblem& problem);

        bool isPartLegal() const;  // check current state
        // may be slow for single-cell moves in k-way part'ing

        // Reinitializes the checker if the partition changed
        // "non-incrementally"
        // Must be redefined in derived classes by calling
        // same method in the immediate parent (with scope resolution operator
        // ::)
        void reinitialize();

        bool isLegalMove(unsigned moduleNumber, unsigned from, unsigned to) const {
                double weight = _hg.getWeight(moduleNumber);
                if (!(_actualPartBalances[from] - weight >= _minPartBalances[from] && _actualPartBalances[to] + weight <= _maxPartBalances[to])) return false;
                unsigned deg = _hg.getNodeByIdx(moduleNumber).getDegree();
                return (_actualPinBalances[to] + deg <= _maxPinDensity * (_actualPartBalances[to] + weight) && _actualPinBalances[from] - deg <= _maxPinDensity * (_actualPartBalances[from] - weight));
        }

        bool isLegalMove(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) const { return isLegalMove(moduleNumber, old.lowestNumPart(), newIds.lowestNumPart()); }

        bool isLegalMoveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) const {
                (void)moduleNumber;
                (void)oldVal;
                (void)newVal;
                abkfatal(0, "Not implemented in this class yet");
                return true;
        }

        // These represent incremental update by one move
        // return true if the moved cell does not make things worse
        // when inlining, the compiler should detect when the return value is
        // not used

        bool moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to) {
                double weight = _hg.getWeight(moduleNumber);
                _actualPartBalances[from] -= weight;
                _actualPartBalances[to] += weight;
                unsigned deg = _hg.getNodeByIdx(moduleNumber).getDegree();
                _actualPinBalances[from] -= deg;
                _actualPinBalances[to] += deg;
                return (_actualPartBalances[from] >= _minPartBalances[from] && _actualPartBalances[to] <= _maxPartBalances[to]);
        }

        bool moveModuleTo(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) { return moveModuleTo(moduleNumber, old.lowestNumPart(), newIds.lowestNumPart()); }

        bool moveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) {
                (void)moduleNumber;
                (void)oldVal;
                (void)newVal;
                abkfatal(0, "Not implemented in this class yet");
                return isPartLegal();
        }

        // --------- Legality-specific interface

        friend std::ostream& operator<<(std::ostream&, const SingleBalanceLegalityWPins&);
};

std::ostream& operator<<(std::ostream&, const SingleBalanceLegalityWPins&);

#endif
