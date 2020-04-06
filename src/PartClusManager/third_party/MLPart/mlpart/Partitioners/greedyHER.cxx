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

#include "greedyHER.h"
#include "PartLegality/1balanceChk.h"
#include "PartLegality/1balanceGen.h"
#include "PartEvals/netCut2way.h"
#include <iomanip>
#include <iostream>

using std::ostream;
using std::sort;
using std::set;
using std::cout;
using std::endl;
using uofm::vector;

GreedyHERPartitioner::GreedyHERPartitioner(PartitioningProblem& problem, const GreedyHERPartitioner::Parameters& params,
                                           //      const MultiStartPartitioner::Parameters& params,
                                           bool skipSolnGen, bool dontRunYet)
    : MultiStartPartitioner(problem, params.getMultiStartPartParams()),
      //: MultiStartPartitioner(problem, params),
      _herParams(params),
      _netTallies(2, vector<unsigned>(problem.getHGraph().getNumEdges(), 0)),
      _hgraph(problem.getHGraph()),
      _curPart(0),
      _skipSolnGen(skipSolnGen),
      _partMaxCapacities(2, 0) {
        const vector<vector<double> >& maxCaps = _problem.getMaxCapacities();
        // cout<<"Constructing greedy HER"<<endl;
        for (unsigned i = 0; i < maxCaps.size(); ++i) {
                _partMaxCapacities[i] = maxCaps[i][0];
                //    cout<<"_partMaxCapacities[i]
                // "<<_partMaxCapacities[i]<<endl;
        }

        if (!dontRunYet) {
                runMultiStart();
        }
}

void GreedyHERPartitioner::doOne(unsigned initSoln) { doOneGreedyHER(initSoln); }

void GreedyHERPartitioner::doOneGreedyHER(unsigned initSoln) {
        if (_params.verb.getForActions() > 1) cout << "Starting Greedy Hyperedge Removal" << endl;

        const unsigned max_edge_degree_threshold = 100;

        if (!_skipSolnGen) {
                PartitioningSolution& soln = *_solutions[initSoln];
                _curPart = &soln.part;
                generateInitialSolution(*_curPart);
                // cout<<"DPDEBUG generating an initial solution"<<endl;
        } else {
                PartitioningSolution& soln = *_solutions[initSoln];
                _curPart = &soln.part;
                // cout<<"DPDEBUG using a supplied solution"<<endl;
        }

        initializeTallies(*_curPart);
        initializeSizes(*_curPart);
        initializeNodesInPart();

        // NetCut2way nc2wa(_problem, *_curPart);
        // double new_cost= nc2wa.getTotalCostDouble();
        // double old_cost = new_cost;
        for (itHGFEdgeGlobal edge = _hgraph.edgesBegin(); edge != _hgraph.edgesEnd(); ++edge) {
                if ((*edge)->getDegree() > max_edge_degree_threshold) continue;  // ignore large edges
                if ((*edge)->getDegree() == 2) continue;                         // ignore 2-pin edges
                if (!isCut((*edge)->getIndex())) continue;                       // ignore uncut
                                                                                 // edges

                // find the move with greatest postive gain
                double maxGain = -std::numeric_limits<double>::max();
                const int DONT_MOVE = -1;
                int bestPart = DONT_MOVE;
                for (unsigned part = 0; part < 2; ++part) {
                        if (moveValid((*edge)->getIndex(), part)) {
                                // cout<<"Testing a valid move
                                // "<<(*edge)->getIndex()<<endl;
                                double gain = computeGainOfMove((*edge)->getIndex(), part);
                                // move must have positive gain
                                if (gain > 0 && gain > maxGain) {
                                        maxGain = gain;
                                        bestPart = part;
                                }
                                // move must have non-negative gain (accept zero
                                // gains)
                                else if (gain >= 0 && gain > maxGain && _herParams.acceptZeroGainMoves) {
                                        maxGain = gain;
                                        bestPart = part;
                                }
                        }
                }

                // NetCut2way nc2w(_problem, *_curPart);
                // double old_cost = nc2w.getTotalCostDouble();
                // cout<<"Net Tallies:
                // 0:"<<_netTallies[0][(*edge)->getIndex()]<<"
                // 1:"<<_netTallies[1][(*edge)->getIndex()]<<endl;

                if (bestPart != DONT_MOVE) {
                        moveEdge((*edge)->getIndex(), bestPart);
                        if (_params.verb.getForActions() > 3) cout << "Accepting a move! Moving edge: " << (*edge)->getIndex() << " going to " << bestPart << " with gain " << maxGain << endl;
                        // SingleBalanceLegality sbl(_problem, *_curPart);
                        // if(!sbl.isPartLegal())
                        //{
                        //   cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
                        //   cout<<"Last move violated balance
                        // constraints"<<endl;
                        //   cout<<"Edge : "<<(*edge)->getIndex()<<endl;
                        //   cout<<"PartSizes 0: "<<_partSizes[0]<<" 1:
                        // "<<_partSizes[1]<<endl;
                        //   cout<<"Part Capacities 0:
                        // "<<_partMaxCapacities[0]<<" 1:
                        // "<<_partMaxCapacities[1]<<endl;
                        //   for(itHGFNodeLocal it =
                        // _hgraph.getEdgeByIdx((*edge)->getIndex()).nodesBegin();
                        // it !=
                        // _hgraph.getEdgeByIdx((*edge)->getIndex()).nodesEnd();
                        // ++it)
                        //   {
                        //      cout<<"Node : "<<(*it)->getIndex()<<" has
                        // weight:
                        // "<<_hgraph.getWeight((*it)->getIndex())<<endl;
                        //   }
                        //
                        //   exit(1);
                        //}
                }

                // NetCut2way nc2wa(_problem, *_curPart);
                // double new_cost = nc2wa.getTotalCostDouble();

                // if(maxGain!=-std::numeric_limits<double>::max() && maxGain !=
                // old_cost - new_cost)
                //{
                //    cout<<"MaxGain: "<<maxGain<<endl;
                //    cout<<"old_cost: "<<old_cost<<endl;
                //    cout<<"new_cost: "<<new_cost<<endl;
                //    cout<<"Net Degree: "<<(*edge)->getDegree()<<endl;
                //    cout<<"Net Tallies:
                // 0:"<<_netTallies[0][(*edge)->getIndex()]<<"
                // 1:"<<_netTallies[1][(*edge)->getIndex()]<<endl;
                //    exit(1);
                //    abkassert(maxGain==-std::numeric_limits<double>::max() ||
                //              maxGain == old_cost - new_cost, "Gain must be
                // the difference in costs");
                //}
        }

        NetCut2way nc2wb(_problem, *_curPart);
        _solutions[initSoln]->cost = static_cast<int>(nc2wb.getTotalCostDouble());
}

void GreedyHERPartitioner::generateInitialSolution(Partitioning& curPart) {

        if (_params.verb.getForActions() > 1)
                cout << "Generating an initial solution with Randomized "
                        "Engineers" << endl;
        SingleBalanceGenerator sGen(_problem);
        sGen.generateSoln(curPart);
}

void GreedyHERPartitioner::moveEdge(unsigned edgeIdx, unsigned toPart) {
        for (itHGFNodeLocal node = _hgraph.getEdgeByIdx(edgeIdx).nodesBegin(); node != _hgraph.getEdgeByIdx(edgeIdx).nodesEnd(); ++node) {
                unsigned part = (*_curPart)[(*node)->getIndex()].isInPart(0) ? 0 : 1;
                if (part != toPart) {
                        if (_params.verb.getForActions() > 6) cout << "Moving an edge: moving node: " << (*node)->getIndex() << " on edge " << edgeIdx << endl;
                        moveNode((*node)->getIndex());
                }
        }
}

void GreedyHERPartitioner::initializeTallies(const Partitioning& curPart) {
        _netTallies = vector<vector<unsigned> >(2, vector<unsigned>(_hgraph.getNumEdges(), 0));
        for (itHGFEdgeGlobal edge = _hgraph.edgesBegin(); edge != _hgraph.edgesEnd(); ++edge) {
                for (itHGFNodeLocal node = (*edge)->nodesBegin(); node != (*edge)->nodesEnd(); ++node) {
                        unsigned part = (*_curPart)[(*node)->getIndex()].isInPart(0) ? 0 : 1;
                        ++_netTallies[part][(*edge)->getIndex()];
                }
        }
}

void GreedyHERPartitioner::initializeSizes(const Partitioning& curPart) {
        _partSizes = vector<double>(2, 0);
        for (itHGFNodeGlobal node = _hgraph.nodesBegin(); node != _hgraph.nodesEnd(); ++node) {
                unsigned part = (*_curPart)[(*node)->getIndex()].isInPart(0) ? 0 : 1;
                _partSizes[part] += _hgraph.getWeight((*node)->getIndex());
        }
}

void GreedyHERPartitioner::initializeNodesInPart(void) {
        hash_map<unsigned, vector<std::set<unsigned> >*, hash<unsigned>, std::equal_to<unsigned> >::iterator it;

        for (it = _nodesInPart.begin(); it != _nodesInPart.end(); ++it) {
                delete it->second;
                it->second = 0;
        }

        _nodesInPart.clear();
}

bool GreedyHERPartitioner::isCut(unsigned edgeIdx) { return ((_netTallies[0][edgeIdx] > 0) && (_netTallies[1][edgeIdx] > 0)); }

void GreedyHERPartitioner::computeNodesInPart(const vector<unsigned>& edgesAffected) {
        for (unsigned affectedIt = 0; affectedIt < edgesAffected.size(); ++affectedIt) {
                unsigned currEdgeIdx = edgesAffected[affectedIt];
                if (_nodesInPart.find(currEdgeIdx) != _nodesInPart.end()) continue;  // we already know this one
                vector<set<unsigned> >* thisEdge = new vector<set<unsigned> >(2);
                _nodesInPart[currEdgeIdx] = thisEdge;
                for (itHGFNodeLocal node = _hgraph.getEdgeByIdx(currEdgeIdx).nodesBegin(); node != _hgraph.getEdgeByIdx(currEdgeIdx).nodesEnd(); ++node) {
                        unsigned part = (*_curPart)[(*node)->getIndex()].isInPart(0) ? 0 : 1;
                        // if(part != toPart)
                        //    edgeNodesInFromPart[affectedIt].push_back((*node)->getIndex());
                        (*thisEdge)[part].insert((*node)->getIndex());
                }
        }
}

// The computeGainOfMove function is the bottleneck because of the many calls to
// computeEdgeNodesInFromPart, which is slow.  There is definitely a lot of work
// being duplicated in that function, and the following container can remember
// the results.  Given an edge index, we can store in a hash table, a pointer to
// a vector<set> v.  Each entry contains the indices of the nodes in that
// partition, e.g., v[0] is the set of nodes in partition 0.  If the edge index
// is not represented in the hash table, then we have never computed the
// edgeNodesInParts lists.  If it is there, then it has been computed.  Whenever
// we call moveNode (which is few times when compared to computeGainOfMove), we
// have to go over all of its incident edges, and if it is in the hash table
// then remove it from its current partition set, and put it into its new
// partition set.  this way, once the sets are computed, they will be maintained
// and not need to be recomputed.
double GreedyHERPartitioner::computeGainOfMove(unsigned edgeIdx, unsigned toPart) {
        double gain = 0.0;

        // cout<<"Computing gain of moving edge "<<edgeIdx<<" to part
        // "<<toPart<<endl;
        // cout<<"Edge "<<edgeIdx<<" incident to nodes: "<<endl;
        // for(itHGFNodeLocal node = _hgraph.getEdgeByIdx(edgeIdx).nodesBegin();
        // node != _hgraph.getEdgeByIdx(edgeIdx).nodesEnd(); ++node)
        //{
        //   cout<<"   "<<(*node)->getIndex();
        //}
        // cout<<endl;
        // for(itHGFNodeLocal node = _hgraph.getEdgeByIdx(edgeIdx).nodesBegin();
        // node != _hgraph.getEdgeByIdx(edgeIdx).nodesEnd(); ++node)
        //{
        //  cout<<" Node "<<(*node)->getIndex()<<" incident to edges:"<<endl;
        //  for(itHGFEdgeLocal edge = (*node)->edgesBegin(); edge !=
        // (*node)->edgesEnd(); ++edge)
        //  {
        //    cout<<"   "<<(*edge)->getIndex();
        //  }
        //  cout<<endl;
        //}

        computeNodesInPart(vector<unsigned>(1, edgeIdx));
        const set<unsigned>& nodesMoving = (*_nodesInPart[edgeIdx])[!toPart];

        // list the edges connected to those nodes
        vector<unsigned> edgesAffected;
        edgesAffected.reserve(_hgraph.getEdgeByIdx(edgeIdx).getDegree() * 4);  // assume: 4=avg node degree
        for (set<unsigned>::const_iterator i = nodesMoving.begin(); i != nodesMoving.end(); ++i)
                for (itHGFEdgeLocal edge = _hgraph.getNodeByIdx(*i).edgesBegin(); edge != _hgraph.getNodeByIdx(*i).edgesEnd(); ++edge) {
                        edgesAffected.push_back((*edge)->getIndex());
                }

        sort(edgesAffected.begin(), edgesAffected.end());
        vector<unsigned>::iterator new_end = std::unique(edgesAffected.begin(), edgesAffected.end());
        edgesAffected.erase(new_end, edgesAffected.end());

        computeNodesInPart(edgesAffected);

        for (unsigned affectedIt = 0; affectedIt < edgesAffected.size(); ++affectedIt) {
                unsigned currEdgeIdx = edgesAffected[affectedIt];
                // if any of those edges were cut, and all of the nodes in
                // fromPart, are going to toPart
                if (!isCut(currEdgeIdx)) continue;
                if (includes(nodesMoving.begin(), nodesMoving.end(), (*_nodesInPart[edgesAffected[affectedIt]])[!toPart].begin(), (*_nodesInPart[edgesAffected[affectedIt]])[!toPart].end())) {

                        gain += _hgraph.getEdgeByIdx(currEdgeIdx).getWeight();
                        // cout<<"Net just got uncut, adding edge weight to
                        // gain"<<endl;
                }
        }

        for (unsigned affectedIt = 0; affectedIt < edgesAffected.size(); ++affectedIt) {
                // if any of those edges were uncut in fromPart, but not all are
                // going

                unsigned currEdgeIdx = edgesAffected[affectedIt];

                bool all_moving = includes(nodesMoving.begin(), nodesMoving.end(), (*_nodesInPart[edgesAffected[affectedIt]])[!toPart].begin(), (*_nodesInPart[edgesAffected[affectedIt]])[!toPart].end());
                // if(!all_moving && !isCut(*it) && part!=toPart)
                if (!all_moving && !isCut(currEdgeIdx) && (*_nodesInPart[edgesAffected[affectedIt]])[!toPart].size() > 0) {
                        gain -= _hgraph.getEdgeByIdx(currEdgeIdx).getWeight();
                        // cout<<"Net just got cut, subtracting edge weight from
                        // gain"<<endl;
                }
        }

        return gain;
}

bool GreedyHERPartitioner::moveValid(unsigned edgeIdx, unsigned toPart) {
        unsigned from = _netTallies[!toPart][edgeIdx];
        // cout<<"Checking if move of edge "<<edgeIdx<<" to part "<<toPart<<" is
        // valid"<<endl;

        if (from == 1) {
                // cout<<"\tOnly one vertex in from part, no"<<endl;
                return false;
        }

        double availableSpaceInToPart = _partMaxCapacities[toPart] - _partSizes[toPart];

        double weight = 0;
        for (itHGFNodeLocal it = _hgraph.getEdgeByIdx(edgeIdx).nodesBegin(); it != _hgraph.getEdgeByIdx(edgeIdx).nodesEnd(); ++it) {
                unsigned part = (*_curPart)[(*it)->getIndex()].isInPart(0) ? 0 : 1;
                if (part == !toPart) {
                        const Partitioning& fixedConstr = _problem.getFixedConstr();
                        if (!(fixedConstr[(*it)->getIndex()].isInPart(toPart))) return false;  // dont move this edge if any
                                                                                               // nodes cannot move to toPart
                        weight += _hgraph.getWeight((*it)->getIndex());
                }
                if (weight > availableSpaceInToPart) {
                        // cout<<"\tWeight in from part "<<weight<<" is greater
                        // than "<<availableSpaceInToPart<<", no"<<endl;
                        return false;
                }
        }

        // cout<<"\tMove is valid! "<<endl;
        return true;
}

void GreedyHERPartitioner::moveNode(unsigned nodeIdx) {
        unsigned part = (*_curPart)[nodeIdx].isInPart(0) ? 0 : 1;

        // update tallies
        for (itHGFEdgeLocal edge = _hgraph.getNodeByIdx(nodeIdx).edgesBegin(); edge != _hgraph.getNodeByIdx(nodeIdx).edgesEnd(); ++edge) {
                --_netTallies[part][(*edge)->getIndex()];
                ++_netTallies[!part][(*edge)->getIndex()];
        }

        // update balance constraint tracking
        _partSizes[part] -= _hgraph.getWeight(nodeIdx);
        _partSizes[!part] += _hgraph.getWeight(nodeIdx);

        // update nodesInPart
        for (itHGFEdgeLocal edge = _hgraph.getNodeByIdx(nodeIdx).edgesBegin(); edge != _hgraph.getNodeByIdx(nodeIdx).edgesEnd(); ++edge) {
                unsigned edgeIdx = (*edge)->getIndex();
                if (_nodesInPart.find(edgeIdx) != _nodesInPart.end()) {
                        (*_nodesInPart[edgeIdx])[part].erase(nodeIdx);
                        (*_nodesInPart[edgeIdx])[!part].insert(nodeIdx);
                }
        }

        // cout<<"Setting partition of node "<<nodeIdx<<" to "<<(!part)<<endl;
        (*_curPart)[nodeIdx].setToPart(!part);
}

GreedyHERPartitioner::Parameters::Parameters(int argc, const char** argv) : acceptZeroGainMoves(false), _multiStartPartParams(argc, argv) {
        // redundant if everyone puts things in the initializer list, might
        // catch an error someday tho
        initializeDefaults();
        setValues(argc, argv);
}

GreedyHERPartitioner::Parameters::Parameters(void) : acceptZeroGainMoves(false) {
        // redundant if everyone puts things in the initializer list, might
        // catch an error someday tho
        initializeDefaults();
}

GreedyHERPartitioner::Parameters::Parameters(const MultiStartPartitioner::Parameters& params) : acceptZeroGainMoves(false) {
        // redundant if everyone puts things in the initializer list, might
        // catch an error someday tho
        initializeDefaults();
        setMultiStartPartParams(params);
}

void GreedyHERPartitioner::Parameters::initializeDefaults() { acceptZeroGainMoves = false; }

void GreedyHERPartitioner::Parameters::setValues(int argc, const char** argv) {
        BoolParam acceptZero("acceptZero", argc, argv);
        acceptZeroGainMoves = acceptZero.found();
}

ostream& operator<<(ostream& os, const GreedyHERPartitioner::Parameters& p) {
        const char* tf[2] = {"false", "true"};
        os << p.getMultiStartPartParams();
        os << "   Accept Zero Gain Moves          : " << tf[p.acceptZeroGainMoves] << endl;
        return os;
}
