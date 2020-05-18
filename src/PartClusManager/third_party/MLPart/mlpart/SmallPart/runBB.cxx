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

// Created by Mike Oliver on 2 april 1999 (earlier work by Igor Markov)

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "bbPart.h"

using std::ostream;
using std::cerr;
using std::cout;
using std::endl;
using std::setw;
using std::max;
using uofm::vector;

void BBPart::_runBB() {
        if (_params.reorderVertices) _reorderMovables();
#ifdef BRUTEFORCECHECK
        _bruteForcePart(7);  // replace 7 by minimum cut you think exists
#endif
        _assignmentStack.reserve(_movables.size() + 1);
        _areaStacks[0].reserve(_movables.size() + 1);
        _areaStacks[1].reserve(_movables.size() + 1);
        _triedBoth.reserve(_movables.size() + 1);
        _areaStacks[0].push_back(_immovableArea[0]);
        _areaStacks[1].push_back(_immovableArea[1]);

        _netStacks.populate(_hgraph, _part, _movables);

        if (_params.verb.getForMajStats() >= 20) {
                cout << "INITIAL STATE of net stacks:" << endl << _netStacks << endl;
        }

        vector<unsigned> mapBack(_hgraph.getNumNodes(), UINT_MAX);
        for (unsigned ll = 0; ll < _movables.size(); ll++) {
                mapBack[_movables[ll]] = ll;
        }

        if (_params.verb.getForMajStats()) {
                cout << "Used edges: " << _netStacks.getUsedEdges() << endl;
                this->printTriangleStats();

                if (_params.verb.getForMajStats() >= 10) {
                        _netStacks.printState(_movables);
                }
        }

        if (_params.consolidateMultiEdges) {
                Timer *tm;

                if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) tm = new Timer;

                _netStacks.consolidate();

                if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
                        tm->stop();
                        cout << "Consolidation used " << *tm << endl;
                        delete tm;
                }

                if (_params.verb.getForMajStats()) {
                        cout << "Used edges after consolidation: " << _netStacks.getUsedEdges() << endl;

                        if (_params.verb.getForMajStats() >= 10) _netStacks.printState(_movables);
                }
        }

        if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
                _diagTimer.stop();
                _totalTime += _diagTimer.getUserTime();
                cout << " Initialization took " << _diagTimer << endl;
                _diagTimer.start();
        }
        // ===================== THE BRANCH AND BOUND LOOP
        // ========================
        // cout << " Part Max: " << _partMax[0] << " " << _partMax[1] << endl;

        _firstTry = true;  // first time assigning to this node with
                           // current assignment stack
        bool doLoop;
        if (_movables.size() > 0)
                doLoop = true;
        else
                doLoop = false;

        while (doLoop) {
                if (_firstTry) {
                        signed cutDif = _netStacks.cutDifference(_assignmentStack.size());
                        if (cutDif > 0) {
                                _to = 0;
                        } else if (cutDif < 0) {
                                _to = 1;
                        } else {
                                if (_partMax[0] - _areaStacks[0].back() > _partMax[1] - _areaStacks[1].back()) {
                                        _to = 0;
                                } else {
                                        _to = 1;
                                }
                        }
                } else {
                        _to = 1 - _to;
                }

                abkfatal(_netStacks.getSize() == _assignmentStack.size() + 1, " XX");
                abkfatal(_triedBoth.size() == _assignmentStack.size(), " YY");

                double newArea = _areaStacks[_to].back() + _weights[_assignmentStack.size()];

#ifdef BRUTEFORCECHECK
                if (_isInitSeg()) {
                        int a = 5;  // breakpoint here
                }
#endif
                if (_params.verb.getForMajStats() >= 20 && _assignmentStack.size() == _movables.size() - 1) {
                        cout << "All nodes assigned, viol = " << newArea - _partMax[_to] << endl;
                }
                if (newArea - _partMax[_to] <= _bestMaxViol)  // legal on area grounds
                {
                        unsigned newCut = _netStacks.assignNode(_assignmentStack.size(), _to);
                        _assignmentStack.push_back(_to);
                        _areaStacks[_to].push_back(newArea);
                        _triedBoth.push_back(!_firstTry);
                        if (++_pushCounter > _params.pushLimit) {
                                _timedOut = true;
                                goto end;
                        }
                        if (_params.verb.getForMajStats() >= 20 && _assignmentStack.size() == _movables.size()) {
                                cout << "Solution found, cut = " << newCut << endl;
                                cout << "Solution in original indices:" << endl;
                                for (unsigned kk = 0; kk < _hgraph.getNumNodes(); kk++) {
                                        unsigned origIndex = mapBack[kk];
                                        if (origIndex != UINT_MAX)
                                                cout << _assignmentStack[origIndex] << " ";
                                        else
                                                cout << "* ";
                                }
                                cout << endl << "Solution in _movables indices:" << endl;
                                for (unsigned jj = 0; jj < _movables.size(); jj++) {
                                        cout << _assignmentStack[jj] << " ";
                                }
                                cout << endl;
                        }

                        if (newCut > _bestSeen || (_foundProvisionallyLegal && newCut == _bestSeen)) {
                                if (_popAll()) goto end;
                                // goto backTrack2;
                                // from here we can't avoid getting to
                                // the backtracking based on tried branches
                                // at the end of the loop
                        } else {
                                if (_assignmentStack.size() < _movables.size()) {
                                        _firstTry = true;
                                        continue;
                                }
                                if (newCut % 2 != 0) {
                                        cerr << "Odd cut in complete solution" << endl;
                                        cout << "Odd cut in complete solution" << endl;
                                        cout << _netStacks << endl;
                                        abkfatal(0, "odd cut in complete solution");
                                }

                                abkfatal(newCut % 2 == 0, "odd cut in complete solution");
                                _bestSeen = newCut;
                                _bestSol = _assignmentStack;
                                _bbFound = true;
                                _foundProvisionallyLegal = true;
                                _area0 = _areaStacks[0].back();
                                _area1 = _areaStacks[1].back();
                                {
                                        double tempViol = max(max(_area0 - _partMax[0], _area1 - _partMax[1]), 0.0);
                                        abkfatal(tempViol <= _bestMaxViol,
                                                 "Unexpected increase "
                                                 "in violations");
                                        _bestMaxViol = tempViol;
                                }

                                if (_bestMaxViol == 0.0) _foundLegal = true;

                                if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
                                        abkassert(_bestSeen % 2 == 0, "_bestSeen is an odd number");
                                        _diagTimer.stop();
                                        _totalTime += _diagTimer.getUserTime();
                                        cout << " Descent to cut " << setw(3) << _bestSeen / 2 << " took " << setw(6) << _diagTimer.getUserTime() << " sec "
                                             << "(" << _totalTime << ")" << endl;
                                        _diagTimer.start();
                                }
                                if (_bestSeen <= 2 * _params.lowerBound) {
                                        if (_params.verb.getForMajStats())
                                                cout << " Reached the lower "
                                                        "bound of " << _params.lowerBound << endl;
                                        break;
                                }

                                // backtracking based on cut
                                do {
                                        if (_popAll()) goto end;
                                } while (_netStacks.getCut() == _bestSeen && !_assignmentStack.empty());
                        }
                }  // end of "legal area" if block
                //    else cout<<" Bounded on area at node
                // "<<_assignmentStack.size()-1<<endl;

                while (!_firstTry && !_assignmentStack.empty()) {
                        if (_popAll()) goto end;
                }
                _firstTry = false;
        }

end:
        abkwarn(_foundProvisionallyLegal || _timedOut, " Main B&B loop found no solutions with small enough violations\n");
        abkwarn(_foundProvisionallyLegal, " Main B&B loop timed out without finding legal solution");

        if (_params.verb.getForSysRes() || _params.verb.getForMajStats()) {
                _diagTimer.stop();
                _totalTime += _diagTimer.getUserTime();
                cout << " Final proof took " << _diagTimer.getUserTime() << " sec "
                     << " (" << _totalTime << ")" << endl;
                _diagTimer.start();
                cout << " BB performed a total of " << _pushCounter << " pushes" << endl;
        }
}

void BBPart::printMovablesGraph(ostream &os) const {
        vector<unsigned> mapBack(_hgraph.getNumNodes(), UINT_MAX);
        unsigned k;
        for (k = 0; k < _movables.size(); k++) {
                mapBack[_movables[k]] = k;
        }
        for (k = 0; k < _movables.size(); k++) {
                os << "Movable node " << k << " original index " << _movables[k] << endl;
                HGFNode const &node = _hgraph.getNodeByIdx(_movables[k]);
                itHGFEdgeLocal iE = node.edgesBegin();
                for (; iE != node.edgesEnd(); iE++) {
                        HGFEdge const &edge = *(*iE);
                        os << "\tEdge " << edge.getIndex() << " degree " << edge.getDegree() << " movables: ";
                        itHGFNodeLocal iN = edge.nodesBegin();
                        for (; iN != edge.nodesEnd(); iN++) {
                                HGFNode const &nbrNode = *(*iN);
                                unsigned origIdx = nbrNode.getIndex();
                                unsigned nbrIdx = mapBack[origIdx];
                                if (nbrIdx != UINT_MAX) os << nbrIdx << " ";
                        }
                        os << endl;
                }
        }
}
