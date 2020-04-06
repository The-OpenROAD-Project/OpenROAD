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

// CHANGES

// 980606 ilm added ~300 ESLOC in enforce() and balance()
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "1balanceChk.h"
#include "Stats/stats.h"
#include <algorithm>

using std::fill;
using std::max;
using std::min;
using std::ostream;
using std::cout;
using std::endl;
using std::setw;
using uofm::vector;

SingleBalanceLegality::SingleBalanceLegality(const PartitioningProblem& problem) : PartLegalityXFace(problem), _nParts(problem.getNumPartitions()), _maxPartBalances(problem.getNumPartitions()), _minPartBalances(problem.getNumPartitions()), _targetPartBalances(problem.getNumPartitions()), _actualPartBalances(problem.getNumPartitions()) {
        for (unsigned k = 0; k != _nParts; k++) {
                _maxPartBalances[k] = _problem->getMaxCapacities()[k][0];
                _minPartBalances[k] = _problem->getMinCapacities()[k][0];
                _targetPartBalances[k] = _problem->getCapacities()[k][0];
        }
}

SingleBalanceLegality::SingleBalanceLegality(const PartitioningProblem& problem, Partitioning& part) : PartLegalityXFace(problem, part), _nParts(problem.getNumPartitions()), _maxPartBalances(problem.getNumPartitions()), _minPartBalances(problem.getNumPartitions()), _targetPartBalances(problem.getNumPartitions()), _actualPartBalances(problem.getNumPartitions()) {
        for (unsigned k = 0; k != _nParts; k++) {
                _maxPartBalances[k] = _problem->getMaxCapacities()[k][0];
                _minPartBalances[k] = _problem->getMinCapacities()[k][0];
                _targetPartBalances[k] = _problem->getCapacities()[k][0];
        }
        reinitialize();
}

void SingleBalanceLegality::reinitialize() {
        PartLegalityXFace::reinitialize();

        std::fill(_actualPartBalances.begin(), _actualPartBalances.end(), 0);
        for (unsigned n = 0; n != _hg.getNumNodes(); n++) {
                double weight = _hg.getWeight(n);
                for (unsigned k = 0; k != _nParts; k++)
                        if ((*_part)[n].isInPart(k)) _actualPartBalances[k] += weight;
        }
}

void SingleBalanceLegality::reinitBalances() {
        for (unsigned k = 0; k != _nParts; k++) {
                _maxPartBalances[k] = _problem->getMaxCapacities()[k][0];
                _minPartBalances[k] = _problem->getMinCapacities()[k][0];
                _targetPartBalances[k] = _problem->getCapacities()[k][0];
        }
}

bool SingleBalanceLegality::isPartLegal() const  // check current state
                                                 // may be slow for single-cell moves in k-way part'ing
{
        if (_actualPartBalances[0] < _minPartBalances[0] || _actualPartBalances[0] > _maxPartBalances[0]) return false;
        if (_actualPartBalances[1] < _minPartBalances[1] || _actualPartBalances[1] > _maxPartBalances[1]) return false;
        for (unsigned k = 2; k != _nParts; k++)
                if (_actualPartBalances[k] < _minPartBalances[k] || _actualPartBalances[k] > _maxPartBalances[k]) return false;
        return true;
}

double SingleBalanceLegality::getMaxViolation() const {
        double maxViolation = max(max(_minPartBalances[0] - _actualPartBalances[0], _actualPartBalances[0] - _maxPartBalances[0]), max(_minPartBalances[1] - _actualPartBalances[1], _actualPartBalances[1] - _maxPartBalances[1]));
        for (unsigned k = 2; k != _nParts; k++) {
                double curViol = max(_minPartBalances[k] - _actualPartBalances[k], _actualPartBalances[k] - _maxPartBalances[k]);
                if (curViol > maxViolation) maxViolation = curViol;
        }
        if (maxViolation < 0)
                return 0;
        else
                return maxViolation;
}

double SingleBalanceLegality::getMaxDiffFromIdeal() const {
        double maxDiff = max(fabs(_targetPartBalances[0] - _actualPartBalances[0]), fabs(_targetPartBalances[1] - _actualPartBalances[1]));
        for (unsigned k = 2; k != _nParts; k++) {
                double curDiff = fabs(_targetPartBalances[k] - _actualPartBalances[k]);
                if (curDiff > maxDiff) maxDiff = curDiff;
        }
        return maxDiff;
}

unsigned SingleBalanceLegality::enforce(const Partitioning& fixedConstr) {
        RandomRawUnsigned ru;
        unsigned numNodes = _hg.getNumNodes();
        unsigned tryCount = 0, moveCount = 0;
        unsigned tryLimit = numNodes * _nParts;
        if (numNodes < 200) tryLimit = 200 * _nParts;

        {  // first cure overfilled partitions

                // find max overfilled partition
                unsigned maxPart = 0;
                double maxDelta = _actualPartBalances[0] - _maxPartBalances[0];
                if (maxDelta < 0) maxDelta = 0;
                double minDelta = maxDelta;
                for (unsigned i = 1; i != _nParts; i++) {
                        double deltaI = _actualPartBalances[i] - _maxPartBalances[i];
                        if (deltaI < 0) deltaI = 0;
                        if (deltaI > maxDelta) {
                                maxDelta = deltaI;
                                maxPart = i;
                        }
                        if (deltaI < minDelta) {
                                minDelta = deltaI;
                        }
                }

                if (minDelta != maxDelta) {
                        vector<double> chancesOfGoingToPart(_nParts, 0);
                        while (maxDelta > 0 && minDelta == 0.0 && tryCount <= tryLimit)
                            // while overfilled parts exist, but some are not
                        {
                                while (_actualPartBalances[maxPart] > _maxPartBalances[maxPart] && tryCount <= tryLimit) {  // part loop
                                                                                                                            // assumes there
                                                                                                                            // is no
                                                                                                                            // replication
                                        unsigned movedIdx = ru % numNodes;
                                        tryCount++;
                                        if (!(*_part)[movedIdx].isInPart(maxPart)) continue;

                                        fill(chancesOfGoingToPart.begin(), chancesOfGoingToPart.end(), 0.0);
                                        double minNonZero = DBL_MAX;
                                        unsigned foundGoodPlace = 0;
                                        double nodeWt = _hg.getWeight(movedIdx);

                                        //        an older version: unbiased
                                        // random selection
                                        //        unsigned newPart=((ru %
                                        // (_nParts-1))+maxPart+1) % _nParts;

                                        unsigned k;
                                        for (k = 0; k < _nParts; k++) {
                                                if (!fixedConstr[movedIdx].isInPart(k)) continue;
                                                double slack = _minPartBalances[k] - _actualPartBalances[k];
                                                if (nodeWt > slack) continue;
                                                foundGoodPlace = 1;
                                                chancesOfGoingToPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection;

                                        for (k = 0; k < _nParts; k++) {
                                                if (!fixedConstr[movedIdx].isInPart(k)) continue;
                                                double slack = _targetPartBalances[k] - _actualPartBalances[k];
                                                if (nodeWt > slack) continue;
                                                foundGoodPlace = 2;
                                                chancesOfGoingToPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection;

                                        for (k = 0; k < _nParts; k++) {
                                                if (!fixedConstr[movedIdx].isInPart(k)) continue;
                                                double slack = _maxPartBalances[k] - _actualPartBalances[k];
                                                if (nodeWt > slack) continue;
                                                foundGoodPlace = 3;
                                                chancesOfGoingToPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection;

                                        break;  // This means movedIdx can not
                                                // be moved to a reasonable part
                                                /*
                          for(k = 0; k < _nParts; k++)
                          {
                            if ( !
                   fixedConstr[movedIdx].isInPart(k) ) continue;
                            double badness=
                   square(_actualPartBalances[k]-_maxPartBalances[k]);
                            double chances= 1.0/badness;
                            chancesOfGoingToPart[k] = chances;
                            if (chances < minNonZero)
                   minNonZero=chances;
                         }
                */

                                selection:

                                        for (k = 0; k < _nParts; k++) chancesOfGoingToPart[k] /= minNonZero;

                                        unsigned newPart = BiasedRandomSelection(chancesOfGoingToPart, ru);

                                        (*_part)[movedIdx].setToPart(newPart);
                                        moveModuleTo(movedIdx, maxPart, newPart);
                                        moveCount++;
                                }
                                // find the new max overfilled partition
                                maxPart = 0;
                                maxDelta = _actualPartBalances[0] - _maxPartBalances[0];
                                if (maxDelta < 0) maxDelta = 0;
                                minDelta = maxDelta;
                                for (unsigned j = 1; j != _nParts; j++) {
                                        double deltaI = _actualPartBalances[j] - _maxPartBalances[j];
                                        if (deltaI < 0) deltaI = 0;
                                        if (deltaI > maxDelta) {
                                                maxDelta = deltaI;
                                                maxPart = j;
                                        }
                                        if (deltaI < minDelta) {
                                                minDelta = deltaI;
                                        }
                                }
                        }
                }
        }  // no overfilled partitions

        {  // take care of underfilled partitions
                unsigned minPart = 0;
                double maxDelta = _minPartBalances[0] - _actualPartBalances[0];
                if (maxDelta < 0) maxDelta = 0;
                double minDelta = maxDelta;
                for (unsigned i = 1; i != _nParts; i++) {
                        double deltaI = _maxPartBalances[i] - _actualPartBalances[i];
                        if (deltaI < 0) deltaI = 0;
                        if (deltaI > maxDelta) {
                                maxDelta = deltaI;
                                minPart = i;
                        }
                        if (deltaI < minDelta) {
                                minDelta = deltaI;
                        }
                }

                if (minDelta != maxDelta) {
                        tryCount = 0;
                        vector<double> chancesOfComingFromPart(_nParts, 0.0);
                        while (maxDelta > 0 && minDelta == 0.0 && tryCount <= tryLimit)
                            // while underfilled parts exist, but some parts are
                            // not
                        {
                                while (_actualPartBalances[minPart] < _minPartBalances[minPart] && tryCount <= tryLimit) {  //  loop assumes
                                                                                                                            // there is no
                                                                                                                            // replication
                                                                                                                            // ----
                                        fill(chancesOfComingFromPart.begin(), chancesOfComingFromPart.end(), 0);
                                        double minNonZero = DBL_MAX;
                                        unsigned foundGoodPlace = 0;

                                        unsigned k;
                                        for (k = 0; k < _nParts; k++) {
                                                double slack = _actualPartBalances[k] - _maxPartBalances[k];
                                                if (slack < 0) continue;
                                                foundGoodPlace = 1;
                                                chancesOfComingFromPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection1;

                                        for (k = 0; k < _nParts; k++) {
                                                double slack = _actualPartBalances[k] - _targetPartBalances[k];
                                                if (slack < 0) continue;
                                                foundGoodPlace = 2;
                                                chancesOfComingFromPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection1;

                                        for (k = 0; k < _nParts; k++) {
                                                double slack = _actualPartBalances[k] - _minPartBalances[k];
                                                if (slack < 0) continue;
                                                foundGoodPlace = 3;
                                                chancesOfComingFromPart[k] = slack;
                                                if (slack < minNonZero) minNonZero = slack;
                                        }
                                        if (foundGoodPlace) goto selection1;

                                        break;  // This means movedIdx can not
                                                // be moved to a reasonable part

                                /*
                                          for(k = 0; k < _nParts; k++)
                                          {
                                            double
                                   badness=square(_minPartBalances[k]
                                   -_actualPartBalances[k]);
                                            double chances=1.0/badness;
                                            chancesOfComingFromPart[k] =
                                   chances;
                                            if (chances < minNonZero)
                                   minNonZero=chances;
                                         }
                                */

                                selection1:

                                        for (k = 0; k < _nParts; k++) chancesOfComingFromPart[k] /= minNonZero;

                                        BiasedRandomSelection rouletteWheel(chancesOfComingFromPart, ru);
                                        bool foundModule = false;
                                        unsigned fromPart;
                                        unsigned movedIdx;
                                        while (!foundModule && tryCount <= tryLimit) {
                                                fromPart = rouletteWheel;
                                                tryCount++;
                                                for (unsigned n = ru % numNodes; n < numNodes; n++) {
                                                        if (!(*_part)[n].isInPart(fromPart)) continue;
                                                        double nodeWt = _hg.getWeight(n);
                                                        double curViol = _minPartBalances[minPart] - _actualPartBalances[minPart];
                                                        double curViolFrom = _minPartBalances[fromPart] - _actualPartBalances[fromPart];
                                                        if (curViol - nodeWt <= curViolFrom + nodeWt) continue;
                                                        foundModule = true;
                                                        movedIdx = n;
                                                        break;
                                                }
                                        }

                                        if (foundModule) {
                                                (*_part)[movedIdx].setToPart(minPart);
                                                moveModuleTo(movedIdx, fromPart, minPart);
                                                moveCount++;
                                        }
                                }

                                minPart = 0;
                                maxDelta = _minPartBalances[0] - _actualPartBalances[0];
                                if (maxDelta < 0) maxDelta = 0;
                                minDelta = maxDelta;
                                for (unsigned j = 1; j != _nParts; j++) {
                                        double deltaI = _minPartBalances[j] - _actualPartBalances[j];
                                        if (deltaI < 0) deltaI = 0;
                                        if (deltaI > maxDelta) {
                                                maxDelta = deltaI;
                                                minPart = j;
                                        }
                                        if (deltaI < minDelta) {
                                                minDelta = deltaI;
                                        }
                                }
                        }
                }
        }
        return moveCount;
}

unsigned SingleBalanceLegality::balance(const Partitioning& fixedConstr) {
        // try bringing all parts to target caps
        vector<double> tempMaxPartBalances(_maxPartBalances);
        vector<double> tempMinPartBalances(_minPartBalances);

        for (unsigned k = 0; k != _nParts; k++) {
                _maxPartBalances[k] = (1 / 3.0) * _maxPartBalances[k] + (2 / 3.0) * _targetPartBalances[k];
                _minPartBalances[k] = (1 / 3.0) * _minPartBalances[k] + (2 / 3.0) * _targetPartBalances[k];
        }

        unsigned moves = enforce(fixedConstr);

        _maxPartBalances = tempMaxPartBalances;
        _minPartBalances = tempMinPartBalances;

        return moves;
}

ostream& operator<<(ostream& os, const SingleBalanceLegality& sbl) {
        os << " Single Balance Legality Checker \n"
           << " Part#    Min capacity   Actual capacity   Max capacity  "
              "Violation\n";
        for (unsigned i = 0; i != sbl._nParts; i++) {
                bool underfilled = sbl._minPartBalances[i] > sbl._actualPartBalances[i];
                bool overfilled = sbl._actualPartBalances[i] > sbl._maxPartBalances[i];

                os << " " << setw(4) << i << " : "
                   << " " << setw(13) << sbl._minPartBalances[i] << " " << setw(15) << sbl._actualPartBalances[i] << " " << setw(16) << sbl._maxPartBalances[i] << "      ";
                if (underfilled) os << "-";
                if (overfilled) os << "+";
                cout << "\n";
        }
        return os << endl;
}
