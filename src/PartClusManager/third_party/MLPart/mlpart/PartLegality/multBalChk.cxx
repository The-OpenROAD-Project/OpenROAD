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

// Created by Igor Markov, May 17, 1999

// CHANGES
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "multBalChk.h"
#include "Stats/stats.h"

using std::max;
using std::ostream;
using std::endl;
using std::setw;
using uofm::vector;

MultBalanceLegality::MultBalanceLegality(const PartitioningProblem& problem) : PartLegalityXFace(problem), _nParts(problem.getNumPartitions()), _nBals(_hg.getNumWeights()) {
        for (unsigned bal = 0; bal != _nBals; bal++)
                for (unsigned k = 0; k != _nParts; k++) {
                        _maxPartBalances[k][bal] = _problem->getMaxCapacities()[k][bal];
                        _minPartBalances[k][bal] = _problem->getMinCapacities()[k][bal];
                        _targetPartBalances[k][bal] = _problem->getCapacities()[k][bal];
                        _actualPartBalances[k][bal] = DBL_MAX;
                }
}

MultBalanceLegality::MultBalanceLegality(const PartitioningProblem& problem, const Partitioning& part) : PartLegalityXFace(problem, const_cast<Partitioning&>(part)), _nParts(problem.getNumPartitions()), _nBals(_hg.getNumWeights()) {
        for (unsigned bal = 0; bal != _nBals; bal++)
                for (unsigned k = 0; k != _nParts; k++) {
                        _maxPartBalances[k][bal] = _problem->getMaxCapacities()[k][bal];
                        _minPartBalances[k][bal] = _problem->getMinCapacities()[k][bal];
                        _targetPartBalances[k][bal] = _problem->getCapacities()[k][bal];
                        _actualPartBalances[k][bal] = DBL_MAX;
                }
        reinitialize();
}

void MultBalanceLegality::reinitialize() {
        PartLegalityXFace::reinitialize();

        for (unsigned bal = 0; bal != _nBals; bal++) {
                unsigned n;
                for (n = 0; n != _nParts; n++) _actualPartBalances[n][bal] = 0.0;
                for (n = 0; n != _hg.getNumNodes(); n++) {
                        double weight = _hg.getWeight(n, bal);
                        for (unsigned k = 0; k != _nParts; k++)
                                if ((*_part)[n].isInPart(k)) _actualPartBalances[k][bal] += weight;
                }
        }
}

void MultBalanceLegality::reinitBalances() {
        for (unsigned bal = 0; bal != _nBals; bal++)
                for (unsigned k = 0; k != _nParts; k++) {
                        _maxPartBalances[k][bal] = _problem->getMaxCapacities()[k][bal];
                        _minPartBalances[k][bal] = _problem->getMinCapacities()[k][bal];
                        _targetPartBalances[k][bal] = _problem->getCapacities()[k][bal];
                        _actualPartBalances[k][bal] = DBL_MAX;
                }
}

bool MultBalanceLegality::isPartLegal() const  // check current state
                                               // may be slow for single-cell moves in k-way part'ing
{
        for (unsigned bal = 0; bal != _nBals; bal++) {
                if (_actualPartBalances[0][bal] < _minPartBalances[0][bal] || _actualPartBalances[0][bal] > _maxPartBalances[0][bal]) return false;
                if (_actualPartBalances[1][bal] < _minPartBalances[1][bal] || _actualPartBalances[1][bal] > _maxPartBalances[1][bal]) return false;
                for (unsigned k = 2; k != _nParts; k++)
                        if (_actualPartBalances[k][bal] < _minPartBalances[k][bal] || _actualPartBalances[k][bal] > _maxPartBalances[k][bal]) return false;
        }
        return true;
}

double MultBalanceLegality::getMaxViolation() const {
        double maxViol = 0.0;
        for (unsigned bal = 0; bal != _nBals; bal++) {
                double curViol = max(max(_minPartBalances[0][bal] - _actualPartBalances[0][bal], _actualPartBalances[0][bal] - _maxPartBalances[0][bal]), max(_minPartBalances[1][bal] - _actualPartBalances[1][bal], _actualPartBalances[1][bal] - _maxPartBalances[1][bal]));

                if (curViol > maxViol) maxViol = curViol;
                for (unsigned k = 2; k != _nParts; k++) {
                        curViol = max(_minPartBalances[k][bal] - _actualPartBalances[k][bal], _actualPartBalances[k][bal] - _maxPartBalances[k][bal]);
                        if (curViol > maxViol) maxViol = curViol;
                }
        }
        return maxViol;
}

double MultBalanceLegality::getMaxDiffFromIdeal() const {
        double maxDiff = 0.0;
        for (unsigned bal = 0; bal != _nBals; bal++) {
                double currDiff = max(fabs(_targetPartBalances[0][bal] - _actualPartBalances[0][bal]), fabs(_targetPartBalances[1][bal] - _actualPartBalances[1][bal]));
                if (currDiff > maxDiff) maxDiff = currDiff;
                for (unsigned k = 2; k != _nParts; k++) {
                        currDiff = fabs(_targetPartBalances[k][bal] - _actualPartBalances[k][bal]);
                        if (currDiff > maxDiff) maxDiff = currDiff;
                }
        }
        return maxDiff;
}

// unsigned MultBalanceLegality::enforce(const Partitioning& fixedConstr)
// { abkfatal(0,"Unavailable"); return UINT_MAX; }

// unsigned MultBalanceLegality::balance(const Partitioning& fixedConstr)
// { abkfatal(0,"Unavailable"); return UINT_MAX; }

ostream& operator<<(ostream& os, const MultBalanceLegality& mbl) {
        unsigned i, oldPrec = os.precision();
        os << std::setprecision(4) << " Mult Balance Legality Checker \n Part# ";

        unsigned bal;
        for (bal = 0; bal != mbl._nBals; bal++) {
                os << "  balance" << bal;
        }
        os << endl;
        vector<double> totalBalances;
        for (bal = 0; bal != mbl._nBals; bal++) {
                double total = 0.0;
                for (i = 0; i != mbl._nParts; i++) total += mbl._actualPartBalances[i][bal];
                totalBalances.push_back(total);
        }
        for (i = 0; i != mbl._nParts; i++) {
                os << setw(4) << i << "    ";
                for (bal = 0; bal != mbl._nBals; bal++) {
                        bool under = mbl._minPartBalances[i][bal] > mbl._actualPartBalances[i][bal];
                        bool over = mbl._actualPartBalances[i][bal] > mbl._maxPartBalances[i][bal];

                        os << " " << setw(6) << 100 * mbl._actualPartBalances[i][bal] / totalBalances[bal] << "%";
                        if (under)
                                os << "-";
                        else if (over)
                                os << "+";
                        else
                                os << " ";
                        os << " ";
                }
                os << endl;
        }
        return os << std::setprecision(oldPrec) << endl;
}
