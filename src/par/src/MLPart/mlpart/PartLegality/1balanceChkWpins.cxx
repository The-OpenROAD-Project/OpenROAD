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

// Created by Igor Markov, May 27, 1998

// CHANGES

// 980606 ilm added ~300 ESLOC in enforce() and balance()

#include "1balanceChkWpins.h"
#include "Stats/stats.h"

using std::ostream;
using std::cout;
using std::endl;
using std::setw;

SingleBalanceLegalityWPins::SingleBalanceLegalityWPins(const PartitioningProblem& problem) : SingleBalanceLegality(problem), _actualPinBalances(problem.getNumPartitions()) {}

SingleBalanceLegalityWPins::SingleBalanceLegalityWPins(const PartitioningProblem& problem, Partitioning& part) : SingleBalanceLegality(problem, part), _actualPinBalances(problem.getNumPartitions()) { reinitialize(); }

void SingleBalanceLegalityWPins::reinitialize() {
        SingleBalanceLegality::reinitialize();

        std::fill(_actualPinBalances.begin(), _actualPinBalances.end(), 0);
        _totalPins = 0;
        for (unsigned n = 0; n != _hg.getNumNodes(); n++) {
                unsigned deg = _hg.getNodeByIdx(n).getDegree();
                _totalPins += deg;
                for (unsigned k = 0; k != _nParts; k++)
                        if ((*_part)[n].isInPart(k)) _actualPinBalances[k] += deg;
        }
        _maxPinDensity = 1.2 * _totalPins / _problem->getTotalWeight()[0];
}

bool SingleBalanceLegalityWPins::isPartLegal() const  // check current state
                                                      // may be slow for single-cell moves in k-way part'ing
{
        if (!SingleBalanceLegality::isPartLegal()) return false;

        if (_actualPinBalances[0] > _actualPartBalances[0] * _maxPinDensity || _actualPinBalances[1] > _actualPartBalances[1] * _maxPinDensity) return false;
        for (unsigned k = 2; k != _nParts; k++)
                if (_actualPinBalances[k] > _actualPartBalances[k] * _maxPinDensity) return false;
        return true;
}

ostream& operator<<(ostream& os, const SingleBalanceLegalityWPins& sbl) {
        os << static_cast<SingleBalanceLegality>(sbl);
        os << " Pins:   ";
        for (unsigned i = 0; i != sbl._nParts; i++) {
                os << setw(5) << sbl._actualPinBalances[i];
                if (sbl._actualPinBalances[i] > sbl._maxPinDensity * sbl._actualPartBalances[i]) cout << "(+)";
                cout << "  ";
        }
        return os << endl << endl;
}
