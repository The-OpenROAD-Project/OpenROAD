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

// Created by Igor Markov on 12/02/98

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <algorithm>
#include "HGraph/hgFixed.h"
#include "Partitioning/partProb.h"
#include "PartEvals/partEvals.h"
#include "PartLegality/1balanceGen.h"
#include "bbPart.h"
#include "netStacks.h"

using std::cerr;
using std::cout;
using std::endl;
using std::flush;
using std::min;
using std::max;
using uofm::vector;

const unsigned MAXNUMMOVS = 100;
const unsigned MAXNUMNETS = 3 * MAXNUMMOVS;
const unsigned MAXNUMPINS = 5 * MAXNUMMOVS;

class CompareByDegree {
        const HGraphFixed& _hg;

       public:
        CompareByDegree(const HGraphFixed& hg) : _hg(hg) {}
        bool operator()(unsigned i, unsigned j) { return _hg.getNodeByIdx(i).getDegree() > _hg.getNodeByIdx(j).getDegree(); }
};

class CompareByWeight {
        const HGraphFixed& _hg;

       public:
        CompareByWeight(const HGraphFixed& hg) : _hg(hg) {}
        bool operator()(unsigned i, unsigned j) { return _hg.getWeight(i) > _hg.getWeight(j); }
};

BBPart::Parameters::Parameters(int argc, const char* argv[]) : lowerBound(0), upperBound(UINT_MAX), useGlobalEarlyBound(false), consolidateMultiEdges(true), assumePassedSolnLegal(false), reorderVertices(false), pushLimit(UINT_MAX), verb(argc, argv) {
        UnsignedParam puLim("bbPushLimit", argc, argv);
        BoolParam reorder("reorderVertices", argc, argv);
        BoolParam noreorder("noReorderVertices", argc, argv);

        abkfatal(!reorder.found() || !noreorder.found(),
                 "Can't specify "
                 "both -reorderVertices and -noReorderVertices ");

        if (reorder.found()) reorderVertices = true;

        if (noreorder.found()) reorderVertices = false;

        if (puLim.found()) pushLimit = puLim;
}

// BBPart::BBPart(const HGraphFixed &hgraph,
//               Partitioning &part,
//               const vector<unsigned>&movables,
//               double partMax0,
//               double partMax1,
//               Parameters params)
//               : _params(params),_totalTime(0),
//               _part(part),
//	           _hgraph(hgraph), _movables(movables),
//               _bestSeen(UINT_MAX), _bestMaxViol(0), _foundLegal(false),
//               _foundProvisionallyLegal(false),_bbFound(false),
//               _pushCounter(0), _timedOut(false)
//{
//    _partMax[0]=partMax0;
//    _partMax[1]=partMax1;
//    if (_params.assumePassedSolnLegal)
//    {
//        _foundProvisionallyLegal=true;
//        _computeCutPassedSoln();
//        _bestMaxViol=max(max(_area0-_partMax[0],_area1-_partMax[1]),
//                0.0);
//        if (_bestMaxViol==0.0)
//            _foundLegal=true;
//    }
//    else
//        _determEnginMethodMovablesOnly();
//
//    if (_params.verb.getForMajStats() > 2)
//    {
//        cout << "** Statistics for the hypergraph to be partitioned: " <<
// endl;
//        if(hgraph.getNumEdges() == 0)
//        {
//            abkwarn(0,"No Edges in Hypergraph to be partitioned. Not printing
// Statistics \n");
//        }
//        else
//        {
//            hgraph.printEdgeSizeStats();
//            hgraph.printNodeWtStats();
//            hgraph.printNodeDegreeStats();
//            cout << endl;
//        }
//    }
//#ifdef ABKDEBUG
//    for (unsigned l=0;l<movables.size();l++)
//    {
//        PartitionIds p=_part[movables[l]];
//        abkfatal(!p.isInPart(0) || !p.isInPart(1),
//                "attempt to refine incomplete solution");
//    }
//#endif
//
//
//    if (_params.verb.getForMajStats())
//    {
//        cout << "Initial cut used in patch ctor for B&B is "
//            << _bestSeen/2 << endl;
//    }
//
//    _setWeights();
//    _runBB();
//    _imposeSolution();
//}

BBPart::BBPart(PartitioningProblem& problem, BBPartBitBoardContainer& bbBitBoards, Parameters params) : _bbBitBoards(bbBitBoards), _params(params), _totalTime(0), _part(problem.getSolnBuffers()[problem.getSolnBuffers().beginUsedSoln()]), _hgraph(problem.getHGraph()), _bestSeen(params.upperBound), _bestMaxViol(0), _foundLegal(false), _foundProvisionallyLegal(false), _bbFound(false), _pushCounter(0), _timedOut(false) {
        if (_params.verb.getForMajStats() > 2) {
                cout << "** Statistics for the hypergraph to be partitioned: " << endl;
                const HGraphFixed& hgraph = _hgraph;  // =problem.getHGraph();
                if (hgraph.getNumEdges() == 0) {
                        abkwarn(0,
                                "No Edges in Hypergraph to be partitioned. Not "
                                "printing Statistics \n");
                } else {
                        hgraph.printEdgeSizeStats();
                        hgraph.printNodeWtStats();
                        hgraph.printNodeDegreeStats();
                        cout << endl;
                }
        }
        NetCut2wayWWeights eval(problem);
        abkfatal(problem.getNumPartitions() == 2, " #parts != 2 not supported yet");
        unsigned numSol = problem.getSolnBuffers().endUsedSoln() - problem.getSolnBuffers().beginUsedSoln();
        abkfatal3(numSol == 1, "BBPart produces only one solution;", numSol, " requested\n");

        PartitionIds part0, part1, part01;
        part0.setToPart(0);
        part1.setToPart(1);
        part01.setToAll(2);

        _movables.reserve(100);
        const Partitioning& fixedConstr = problem.getFixedConstr();

        for (unsigned k = 0; k < _hgraph.getNumNodes(); ++k) {
                if (fixedConstr[k].isEmpty() || fixedConstr[k] == part01) {
                        _movables.push_back(k);
                }
                _part[k] = fixedConstr[k];
        }

// unnecessary assertion -- sometimes want to sent a problem
// through the flow without any freedom
// abkfatal(!_movables.empty(),
//	"Partitioning problem with everything fixed\n");
#if 0
    abkfatal3(_movables.size()<=MAXNUMMOVS,
        " Small part problem has too many movables (", nNodes,")\n");
    abkfatal3(_hgraph.getNumEdges()<=MAXNUMNETS,
        " Small part problem has too many nets (", _hgraph.getNumEdges(),")\n");
    abkfatal3(_hgraph.getNumPins()<=MAXNUMPINS,
        " Small part problem has too many pins (", _hgraph.getNumPins(),")\n");
#endif

        if (_params.verb.getForMajStats()) cout << " Found " << _movables.size() << " movables" << endl;

        if (_params.upperBound != UINT_MAX) {
                _bestSeen = 2 * _params.upperBound;
                if (_params.verb.getForMajStats()) cout << " Assumed that a legal solutions of cut at most " << _params.upperBound << " exist" << endl;
        }

        _partMax[0] = problem.getMaxCapacities()[0][0];
        _partMax[1] = problem.getMaxCapacities()[1][0];

        if (!_params.reorderVertices) _sort();
        _setWeights();
        _checkForLegalSol(problem, eval);
        _runBB();
        _finalize(problem, eval);
}

void BBPart::_computeCutPassedSoln() {
        _bbBitBoards.edges.reset(_hgraph.getNumEdges());  // make sure there's enough space
        _area0 = _area1 = 0.0;

        for (unsigned k = 0; k < _movables.size(); k++) {
                HGFNode const& node = _hgraph.getNodeByIdx(_movables[k]);
                for (itHGFEdgeLocal iE = node.edgesBegin(); iE != node.edgesEnd(); iE++) {
                        _bbBitBoards.edges.setBit((*iE)->getIndex());
                }
                if (_part[node.getIndex()].isInPart(0)) {
                        _area0 += _hgraph.getWeight(node.getIndex());
                } else {
                        _area1 += _hgraph.getWeight(node.getIndex());
                }
        }

        vector<unsigned> const& incidEdges = _bbBitBoards.edges.getIndicesOfSetBits();

        vector<unsigned>::const_iterator iIE;

        unsigned initCut = 0;

        for (iIE = incidEdges.begin(); iIE != incidEdges.end(); iIE++) {
                HGFEdge const& edge = _hgraph.getEdgeByIdx(*iIE);
                itHGFNodeLocal iN = edge.nodesBegin();
                unsigned whichPart = (_part[(*(iN++))->getIndex()].isInPart(0)) ? 0 : 1;
                if (!_part[(*(iN++))->getIndex()].isInPart(whichPart)) {
                        initCut += 2;
                } else
                        while (iN != edge.nodesEnd()) {
                                if (!_part[(*(iN++))->getIndex()].isInPart(whichPart)) {
                                        initCut += 2;
                                        break;
                                }
                        }
        }

        if (_params.upperBound == UINT_MAX) {
                _bestSeen = initCut;
        } else {
                _bestSeen = min(initCut, 2 * _params.upperBound);
        }
}
void BBPart::_sort() {
        //===================== SORT VERTICES BY DEGREE ========================

        CompareByDegree byDegree(_hgraph);
        std::stable_sort(_movables.begin(), _movables.end(), byDegree);
}

void BBPart::_setWeights() {
        _weights = vector<double>(_movables.size());
        for (unsigned n = 0; n != _movables.size(); n++) _weights[n] = _hgraph.getWeight(_movables[n]);
        // cout << " Weights : " << _weights << endl;
}

void BBPart::_checkForLegalSol(PartitioningProblem& problem, NetCut2wayWWeights& eval) {
        // deterministic engineer's method checks for legal solns
        if (_bestSeen != UINT_MAX) return;

        _determEnginMethod();

        if (!_foundLegal) {
                // randomized engineer's method
                _bestMaxViol = DBL_MAX;
                Partitioning tmpPart(_part.size());

                unsigned k = 0;
                for (k = 1; k != 5; ++k) {
                        for (unsigned i = 0; i < tmpPart.size(); ++i) tmpPart[i].setToAll(2);
                        SingleBalanceGenerator gen(problem);
                        vector<double> balances = gen.generateSoln(tmpPart);
                        double curViol = max(balances[0] - _partMax[0], balances[1] - _partMax[1]);

                        if (curViol < _bestMaxViol) {
                                _bestMaxViol = max(curViol, 0.0);
                                for (vector<unsigned>::iterator j = _movables.begin(); j != _movables.end(); ++j) _part[*j] = tmpPart[*j];

                                _area0 = balances[0];
                                _area1 = balances[1];

                                if (_bestMaxViol == 0.0) {
                                        _foundLegal = true;
                                        _foundProvisionallyLegal = true;
                                        break;
                                }
                        }
                }

                if (_params.verb.getForMajStats())
                        if (_foundLegal)
                                cout << " Randomized engineers method found a "
                                        "legal solution in " << k << " tries " << endl;
                        else
                                cout << " Rand. engineer's method found no "
                                        "legal solution. Will do without." << endl;
        }

        // cout << result;
        eval.resetTo(_part);
        _bestSeen = 2 * eval.getTotalCost();
        if (_params.verb.getForMajStats()) cout << " Initial bound " << _bestSeen / 2.0 << endl;
}

void BBPart::_determEnginMethod() {
        const Permutation& sortAscWts_1 = _hgraph.getNodesSortedByWeights();
        double areaLeft[2] = {_partMax[0], _partMax[1]};
        _immovableArea[0] = 0;
        _immovableArea[1] = 0;
        for (unsigned n = _hgraph.getNumNodes() - 1; n != unsigned(-1); n--) {
                unsigned nodeIdx = sortAscWts_1[n];
                if (_part[nodeIdx].numberPartitions() == 1) {
                        unsigned part = _part[nodeIdx].lowestNumPart();
                        double wt = _hgraph.getWeight(nodeIdx);
                        if (!_part[nodeIdx].isInPart(part)) {
                                abkfatal3(_part[nodeIdx].isInPart(1 - part), " Node ", nodeIdx, " cannot be assigned to any partition");
                                part = 1 - part;
                        }
                        _immovableArea[part] += wt;
                        areaLeft[part] -= wt;
                }
        }
        for (unsigned n = _hgraph.getNumNodes() - 1; n != unsigned(-1); n--) {
                unsigned part = (areaLeft[0] > areaLeft[1] ? 0 : 1);
                unsigned nodeIdx = sortAscWts_1[n];
                if (_part[nodeIdx].numberPartitions() == 1) continue;
                double wt = _hgraph.getWeight(nodeIdx);
                if (!_part[nodeIdx].isInPart(part)) {
                        abkfatal3(_part[nodeIdx].isInPart(1 - part), " Node ", nodeIdx, " cannot be assigned to any partition");
                        part = 1 - part;
                }
                areaLeft[part] -= wt;
                _part[nodeIdx].setToPart(part);
        }
        _bestMaxViol = max(-min(areaLeft[0], areaLeft[1]), 0.0);
        _area0 = _partMax[0] - areaLeft[0];
        _area1 = _partMax[1] - areaLeft[1];
        if (_bestMaxViol == 0.0) {
                _foundLegal = true;
                _foundProvisionallyLegal = true;
                if (_params.verb.getForMajStats())
                        cout << " Deterministic engineer's method found a "
                                "legal solution " << endl;
        } else if (_params.verb.getForMajStats())
                cout << " Deterministic engineer's method did not find a legal "
                        "solution " << endl;
}

// void BBPart::_determEnginMethodMovablesOnly()
//{
//    double areaLeft[2]={_partMax[0], _partMax[1]};
//    vector<unsigned> temp=_movables;
//    CompareByWeight c(_hgraph);
//    std::sort(temp.begin(),temp.end(),c);
//    for(unsigned n=temp.size()-1; n!=unsigned(-1); n-- )
//    {
//        unsigned part=(areaLeft[0]>areaLeft[1] ? 0 : 1);
//        unsigned nodeIdx=temp[n];
//        double wt=_hgraph.getNodeByIdx(nodeIdx).getWeight();
//        if (!_part[nodeIdx].isInPart(part))
//        {
//            abkfatal3(_part[nodeIdx].isInPart(1-part),
//                    " Node ", nodeIdx, " cannot be assigned to any
// partition");
//            part=1-part;
//        }
//        areaLeft[part]-=wt;
//        _part[nodeIdx].setToPart(part);
//    }
//    _bestMaxViol=max(-min(areaLeft[0],areaLeft[1]),0.0);
//    _area0=_partMax[0]-areaLeft[0];
//    _area1=_partMax[1]-areaLeft[1];
//    if (_bestMaxViol==0.0)
//    {
//        _foundLegal=true;
//        _foundProvisionallyLegal=true;
//        if ( _params.verb.getForMajStats() )
//            cout<<" Deterministic engineer's method found a legal solution "
// << endl;
//    }
//    else
//    {
//        if ( _params.verb.getForMajStats() )
//            cout
//                <<" Deterministic engineer's method did not find a legal
// solution "<<endl;
//    }
//}

void BBPart::_finalize(PartitioningProblem& problem, NetCut2wayWWeights& eval) {
        PartitioningBuffer& buf = problem.getSolnBuffers();
        problem.setBestSolnNum(buf.beginUsedSoln());

        _imposeSolution();

        vector<double>& viols = const_cast<vector<double>&>(problem.getViolations());
        //  vector<double>&
        // imbals=const_cast<vector<double>&>(problem.getImbalances());
        // the latter won't be set
        vector<double>& costs = const_cast<vector<double>&>(problem.getCosts());
        viols[buf.beginUsedSoln()] = _bestMaxViol;
        abkassert(_bestSeen % 2 == 0, "_bestSeen is an odd number");
        costs[buf.beginUsedSoln()] = _bestSeen / 2;

        if (_params.verb.getForMajStats()) {
                cout << flush << " Resulting cut is " << _bestSeen / 2 << endl;
                eval.resetTo(_part);
                cerr << flush;
                cout << flush << " Max violations   " << _bestMaxViol << endl;
                cerr << flush;
                cout << " Checked cost  is " << eval.getTotalCost() << endl;
        }

        _tm.stop();
        if (_params.verb.getForSysRes()) cout << " Branch-and-bound search took " << _tm << endl;
}

void BBPart::_imposeSolution() {
        if (_bbFound) {
                for (unsigned k = 0; k != _movables.size(); k++) {
                        _part[_movables[k]].setToPart(_bestSol[k]);
                }
        }
}

void BBPart::printTriangleStats() const {
        unsigned numThrees = 0, threesNoTerms = 0, threesOneTerm = 0, threesTwoTerms = 0, threesThreeTerms = 0;
        vector<unsigned> mapBack(_hgraph.getNumNodes(), UINT_MAX);

        for (unsigned k = 0; k < _movables.size(); k++) {
                mapBack[_movables[k]] = k;
        }
        for (unsigned eIdx = 0; eIdx != _hgraph.getNumEdges(); eIdx++) {
                HGFEdge const& edge = _hgraph.getEdgeByIdx(eIdx);
                if (edge.getDegree() == 3) {
                        numThrees++;
                        unsigned movIndices[3];
                        for (unsigned l = 0; l < 3; l++) {
                                movIndices[l] = mapBack[(*(edge.nodesBegin() + l))->getIndex()];
                        }
                        std::sort(movIndices, movIndices + 3);
                        if (movIndices[0] == UINT_MAX) {
                                threesThreeTerms++;
                        } else if (movIndices[1] == UINT_MAX) {
                                threesTwoTerms++;
                        } else if (movIndices[2] == UINT_MAX) {
                                threesOneTerm++;
                        } else {
                                threesNoTerms++;
                        }
                }
        }
        cout << "Three-pin nets: " << numThrees << endl << "\tno terms: " << threesNoTerms << endl << "\tone term: " << threesOneTerm << endl << "\ttwo terms: " << threesTwoTerms << endl << "\tthree terms: " << threesThreeTerms << endl;
}
