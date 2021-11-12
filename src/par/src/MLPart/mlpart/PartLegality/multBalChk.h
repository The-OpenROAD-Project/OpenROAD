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

// created by Igor Markov on 05/26/99

#ifndef __MULTBALCHK_H__
#define __MULTBALCHK_H__

#include "partLegXFace.h"

// Note: derived classes should only be used when the precise class
//       is clear (not to compile any methods as virtual)

const unsigned MaxPartsForLegality = 32;
const unsigned MaxBalsForLegality = 32;

class MultBalanceLegality : public PartLegalityXFace {
       protected:
        unsigned _nParts;
        unsigned _nBals;
        double _maxPartBalances[MaxPartsForLegality][MaxBalsForLegality];
        double _minPartBalances[MaxPartsForLegality][MaxBalsForLegality];
        double _targetPartBalances[MaxPartsForLegality][MaxBalsForLegality];
        double _actualPartBalances[MaxPartsForLegality][MaxBalsForLegality];

       public:
        MultBalanceLegality(const PartitioningProblem& problem, const Partitioning& part);
        MultBalanceLegality(const PartitioningProblem& problem);

        unsigned getNumParts() const { return _nParts; }
        unsigned getNumBals() const { return _nBals; }
        double getActualBalances(unsigned part, unsigned bal) const { return _actualPartBalances[part][bal]; }

        bool isPartLegal() const;  // check current state; may be slow

        double getViolation() const { return getMaxViolation(); }
        double getDiffFromIdeal() const { return getMaxDiffFromIdeal(); }
        unsigned getUnderfilledPartition() const {
                abkfatal(0, " Unavailable");
                return UINT_MAX;
        }

        double getMaxViolation() const;
        double getMaxDiffFromIdeal() const;

        // Reinitializes the checker if the partition changed
        // "non-incrementally"
        // Must be redefined in derived classes by calling
        // same method in the immediate parent (with scope resolution operator
        // ::)
        void reinitialize();

        void reinitBalances();

        bool isLegalMove(unsigned moduleNumber, unsigned from, unsigned to, unsigned balance) const {
                double weight = _hg.getWeight(moduleNumber, balance);

                return (_actualPartBalances[from][balance] - weight >= _minPartBalances[from][balance] && _actualPartBalances[to][balance] + weight <= _maxPartBalances[to][balance]);
        }

        bool isLegalMove(unsigned moduleNumber, unsigned from, unsigned to) const {
                if (!isLegalMove(moduleNumber, from, to, 0)) return false;

                for (unsigned balance = 1; balance != _nBals; balance++)
                        if (!isLegalMove(moduleNumber, from, to, balance)) return false;

                return true;
        }

        bool isLegalMove(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) const { return isLegalMove(moduleNumber, old.lowestNumPart(), newIds.lowestNumPart()); }

        bool isLegalMoveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) const {
                (void)moduleNumber;
                (void)oldVal;
                (void)newVal;
                abkfatal(0, " Unavailable");
                return false;
        }

        // These represent incremental update by one move
        // return true if the moved cell does not make things worse
        // when inlining, the compiler should detect when the return value is
        // not used

        bool moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to) {
                double weight = _hg.getWeight(moduleNumber);
                _actualPartBalances[from][0] -= weight;
                _actualPartBalances[to][0] += weight;
                for (unsigned balance = 1; balance != _nBals; balance++) {
                        weight = _hg.getWeight(moduleNumber, balance);
                        _actualPartBalances[from][balance] -= weight;
                        _actualPartBalances[to][balance] += weight;
                }
                return false;  // the bool return value isn't functional
        }

        bool moveModuleTo(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) { return moveModuleTo(moduleNumber, old.lowestNumPart(), newIds.lowestNumPart()); }

        bool moveWithReplication(unsigned moduleNumber, PartitionIds oldVal, PartitionIds newVal) {
                (void)moduleNumber;
                (void)oldVal;
                (void)newVal;
                abkfatal(0, "Unavailable");
                return false;
        }

        // --------- Legality-specific interface

        // const vector<double>& getActualBalances() const { return
        // _actualPartBalances;}

        double getMaxLegalMoveArea(unsigned from, unsigned to) {
                double deltaFrom = _actualPartBalances[from][0] - _minPartBalances[from][0];
                double deltaTo = _maxPartBalances[to][0] - _actualPartBalances[to][0];
                double maxAr = std::min(deltaFrom, deltaTo);

                for (unsigned bal = 1; bal != _nBals; bal++) {
                        deltaFrom = _actualPartBalances[from][bal] - _minPartBalances[from][bal];
                        deltaTo = _maxPartBalances[to][bal] - _actualPartBalances[to][bal];
                        double tmp = std::min(deltaFrom, deltaTo);
                        if (maxAr > tmp) maxAr = tmp;
                }
                return std::max(0.0, maxAr);
        }

        // unsigned balance(const Partitioning& fixedConstr); // returns
        // moveCount
        // unsigned enforce(const Partitioning& fixedConstr);

        friend std::ostream& operator<<(std::ostream&, const MultBalanceLegality&);
};

std::ostream& operator<<(std::ostream&, const MultBalanceLegality&);

#endif
