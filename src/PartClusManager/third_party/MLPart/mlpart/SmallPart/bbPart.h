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
#ifndef _BBPART_H_
#define _BBPART_H_

#include "ABKCommon/abkcommon.h"
#include "Ctainers/bitBoard.h"
#include "Ctainers/discretePrioritizer.h"
#include "Ctainers/riskyStack.h"
#include "PartEvals/netCut2wayWWeights.h"
#include "hgraphBreakTriangles.h"
#include "netStacks.h"
#include "netStacksInevCuts.h"

class BBPart;
typedef BBPart BBPartitioner;

class PartitioningProblem;

class BBPartBitBoardContainer {
       public:
        BitBoard edges;

        BBPartBitBoardContainer(void) : edges(1000) {}
};

// const unsigned bbpart_leafGoalWeight;
const unsigned bbpart_twoHopGoalWeight = 100;
const unsigned bbpart_tailOrderingWeight = 100;
const unsigned bbpart_degreeWeight = 10;
const unsigned bbpart_nodeSizeWeight = 1;
// const unsigned bbpart_allNodesGoalWeight;

class BBPart  // : BaseSmallPartitioner
    {
       public:
        class Parameters {
               public:
                unsigned lowerBound;
                unsigned upperBound;
                bool useGlobalEarlyBound;
                bool consolidateMultiEdges;
                bool assumePassedSolnLegal;
                bool reorderVertices;
                unsigned pushLimit;
                Verbosity verb;

                Parameters() : lowerBound(0), upperBound(UINT_MAX), useGlobalEarlyBound(false), consolidateMultiEdges(true), assumePassedSolnLegal(false), reorderVertices(false), pushLimit(UINT_MAX), verb("silent") {}

                Parameters(int argc, const char *argv[]);
        };

       protected:
        class ReorderMovables {
               private:
                Verbosity _verb;
                uofm::vector<unsigned> &_movables;

                double _maxNodeSize, _minNodeSize;

                // indices in _movables of marked nodes
                uofm::vector<unsigned> _numberedNodes;

                const HGraphBreakTriangles _noTriGraph;

                // index is index in _movables
                uofm_bit_vector _isNumbered;

                // index is index in _movables
                BitBoard _changedPriority;

                // index is original index
                // bit set true when a node is marked and the
                // two-hop priorities of its second-order nbrs
                // are computed
                uofm_bit_vector _twoHopsComputed;

                uofm::vector<unsigned> _mapBack;

                // indexed by index for _movables
                uofm::vector<unsigned> _priorities;
                DiscretePrioritizer _prior;
                DiscretePrioritizer _edgePrior;
                uofm_bit_vector _edgeDequeued;

                static unsigned _totalEdgeWeight(HGFNode const &node);

                void _numberHairyNodes();  // nodes of high degree

                void _numberOddNodes();  // nodes with an odd no. of 2-pin nbrs

                void _dom2PinEdges();

                void _twoHops();

                // use index in graph, not in _movables
                void _updateTwoHopPriors(unsigned markedOrigIdx);

                // nbrIdx is index in _movables
                // _movables[nbrIdx] is a neighbor of markedOrigIdx
                // via a 2-pin edge, and its current priority may include
                // two-hop paths via markedOrigIdx.  When markedOrigIdx
                // is marked, this component must be removed from the
                // priority of the neighbor node.
                void _reduceNbrPriors(unsigned nbrIdx, HGFNode const &markedNode, double firstEdgeWeight);

                void _tailOrdering();

                void _numberLooseNodes();  // nodes with no edges

                void _prioritizeNodesForTailOrdering();

               public:
                ReorderMovables(uofm::vector<unsigned> &movables, HGraphFixed const &hgraph, Verbosity verb = Verbosity("silent"));
        };

       protected:
        BBPartBitBoardContainer &_bbBitBoards;

        Parameters _params;
        double _totalTime;
        Timer _diagTimer, _tm;
        Partitioning &_part;

        const HGraphFixed &_hgraph;

        uofm::vector<unsigned> _movables;
        uofm::vector<double> _weights;
        double _partMax[2];
        unsigned _bestSeen;
        double _bestMaxViol;
        bool _foundLegal;

        // i.e. legal after redefining limits
        bool _foundProvisionallyLegal;

        bool _bbFound;  // Did b&b change the engineer's solution?

        mutable bool _firstTry;
        mutable unsigned _to;

        //      NetStacks                _netStacks;
        NetStacksInevCuts _netStacks;
        RiskyStack<double> _areaStacks[2];
        RiskyStack<unsigned> _assignmentStack, _bestSol;
        RiskyStack<bool> _triedBoth;

        unsigned _savedSoln;  // used only when BRUTEFORCECHECK
        // is defined

        double _area0, _area1;  // areas of partitions in best
        // solution seen
        // NOTE!  Their current
        // application relates to the
        //"patch" ctor.  Therefore
        // they are assumed to be the
        // sums of the areas of the movables.
        // Some initializations may not be
        // correct if there are terminals
        // with nonzero area.
        double _immovableArea[2];
        unsigned _pushCounter;  // used to timeout if run is too long
        bool _timedOut;

        void _sort();
        void _setWeights();

        void _checkForLegalSol(PartitioningProblem &problem, NetCut2wayWWeights &eval);

        void _determEnginMethod();
        // void _determEnginMethodMovablesOnly();
        void _computeCutPassedSoln();
        void _reorderMovables();

        void _runBB();
        void _finalize(PartitioningProblem &problem, NetCut2wayWWeights &eval);

        void _imposeSolution();

        inline bool _popAll();

        void _bruteForcePart(PartitioningProblem &problem, unsigned cut = UINT_MAX);  // provide place to set
                                                                                      // a breakpoint
        // finding all legal solns with a
        // given cut.  Used only when
        // BRUTEFORCECHECK is defined

        bool _isInitSeg();  // current assignment is init seg
                            // of saved solution from brute
                            // force.  Used only when
                            // BRUTEFORCECHECK is defined

       public:
        BBPart(PartitioningProblem &problem, BBPartBitBoardContainer &bbBitBoards, Parameters params = Parameters());

        //  BBPart(const HGraphFixed &hgraph,
        //      Partitioning &part,
        //      const uofm::vector<unsigned>&movables,
        //      double partMax0,
        //      double partMax1,
        //      Parameters params=Parameters());

        void printTriangleStats() const;
        double getArea0() const { return _area0; }
        double getArea1() const { return _area1; }
        bool timedOut() const { return _timedOut; }
        bool foundLegal() const { return _foundLegal; }
        bool foundProvisionallyLegal() const { return _foundProvisionallyLegal; }

        void printMovablesGraph(std::ostream &os = std::cout) const;
};

inline bool BBPart::_popAll() {
        _to = _assignmentStack.back();
        _firstTry = !_triedBoth.back();
        _triedBoth.pop_back();
        _areaStacks[_to].pop_back();
        _assignmentStack.pop_back();
        _netStacks.unAssignNode(_assignmentStack.size(), _to);
        return _assignmentStack.empty() && !_firstTry;
}

#endif
