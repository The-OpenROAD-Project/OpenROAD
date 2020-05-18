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

//! author=" Andrew Caldwell and Igor Markov "

// Adapted by Andrew Caldwell and Igor Markov from an earlier
// prototype by Andrew Caldwell,  March 04, 1998
// version 4.0 by Andrew Caldwell 090998

#ifndef __BASEMLPART_H__
#define __BASEMLPART_H__

#include "ABKCommon/abkcommon.h"
#include "Geoms/plGeom.h"
#include "Partitioners/multiStartPart.h"
#include "Partitioners/partRegistry.h"
#include "ClusteredHGraph/clustHGraph.h"
#include "FilledHier/fillHier.h"
#include "SmallPart/bbPart.h"
#include <iostream>

class PartitionIds;
class Partitioning;
class PartitioningBuffer;
class PartitioningDoubleBuffer;
class PartitioningProblem;

class BaseMLPart {
       public:
        class Parameters : public PartitionerParams {
               public:
                enum SavePartProb {
                        NeverSave,
                        AtAllLastLevels,
                        AtAllLevels,
                        AtFirstLevelOfFirst,
                        AtLastLevelOfFirst,
                        AtAllLevelsOfFirst
                };

                enum VcyclType {
                        NoVcycles,
                        Exclusive,
                        Relaxed,
                        Relaxed2,
                        Comprehensive,
                        Initial
                };

                enum TwoPartCalls {
                        ALL,
                        TOPONLY,
                        ALLBUTLAST,
                        NEVER
                };

                SavePartProb savePartProb;

                PartitionerType flatPartitioner;
                bool useBBonTop;

                double partFuzziness;  // measured in %% of the mdist from
                // the terminal to closest partition
                // 0 means can propagate only to
                // propagate to partitions with min. dist
                unsigned runsPerClTree;
                unsigned solnPoolOnTopLevel;

                double toleranceMultiple;  // multiple of the largest
                // node at the top level

                double toleranceAlpha;  // used to weight 2nd and
                // subsequent largest
                // cells in computing top
                // level tol. A value of 1
                // means they are given full
                // weight, 0 means only the
                // largest node is considered

                TwoPartCalls useTwoPartCalls;  // on which levels should
                // a partitioner first be
                // called with the relaxed
                // tol, then the orig.

                unsigned netThreshold;  // remove nets of size > netThreshold.
                VcyclType Vcycling;
                unsigned timeLimit;
                bool expPrint2Costs;  // experimental
                bool clusterToTerminals;
                bool seedTopLvlSoln;
                // if an initial partitioning is passed to
                // ML, it will build the ClusteredHGraphs
                // to respect it.  If seedTopLvlSoln is
                // true, the top-level soln will also
                // respect this partitioning, otherwise it
                // will be randomly generated.

                unsigned pruningPercent;
                double pruningPoint;  // in percent. The costs are compared
                // for pruning at #Leaves*pruningPt%

                unsigned maxNumPassesAtBottom;
                unsigned maxPassesAfterTopLevels;  // added by royj

                /*Vcycling parameters.	These are used for the trees/ml runs
                 *done during Vcycling.  There are 2 sets of parameters.*/
                unsigned vcNumFailures;  // Vcycling stops after numFailures
                // consecutive failures.
                double vcImproveRatio;  // soln quality must improve by
                // improveRatio in order to be
                // considered not a fail.

                double vc1ClusterRatio;  // Vcycling patern#1
                unsigned vc1FirstLevel;
                double vc1LevelGrowth;

                double vc2ClusterRatio;  // Vcycling patern#2
                unsigned vc2FirstLevel;
                double vc2LevelGrowth;

                // added by DP
                bool useFMPartPlus;

                // added by royj
                double lastVCycleStartThreshold;  // only do last vcycling if
                                                  // solution cost
                // is as good as this
                double lastVCycleImproveThreshold;  // when to stop the last
                                                    // vcycling

                ClustHGraphParameters clParams;

                Parameters(int argc, const char* argv[]);

                Parameters()
                    : PartitionerParams(Verbosity("1 1 1")),
                      savePartProb(NeverSave),
                      useBBonTop(false),
                      partFuzziness(0),
                      runsPerClTree(1),
                      solnPoolOnTopLevel(3),
                      toleranceMultiple(2),
                      toleranceAlpha(0),
                      useTwoPartCalls(TOPONLY),
                      netThreshold(0),  // don't threshold
                      Vcycling(Exclusive),
                      timeLimit(0),
                      expPrint2Costs(false),
                      clusterToTerminals(false),
                      seedTopLvlSoln(false),
                      pruningPercent(10000),
                      pruningPoint(30),
                      maxNumPassesAtBottom(0),
                      maxPassesAfterTopLevels(2),
                      vcNumFailures(1),
                      vcImproveRatio(1),
                      vc1ClusterRatio(2),
                      vc1FirstLevel(200),
                      vc1LevelGrowth(2),
                      vc2ClusterRatio(1.3),
                      vc2FirstLevel(200),
                      vc2LevelGrowth(2),
                      useFMPartPlus(false),
                      lastVCycleStartThreshold(DBL_MAX),
                      lastVCycleImproveThreshold(2. / 300.),
                      clParams() {
                        maxHillHeightFactor = 2;
                        useEarlyStop = true;
                        useClip = 0;
                }

                //	Parameters(const Parameters &params);
                //	Parameters &operator=(const Parameters &params);
        };

       protected:
        double _userTime;
        double _bestCost;
        double _aveCost;
        unsigned _numLegalSolns;

        double _totalClusteringTime;
        double _aveClusteringTime;
        double _totalUnClusteringTime;
        double _aveUnClusteringTime;

        uofm::vector<uofm::vector<double> > _bestSolnPartWeights;  // weights
                                                                   // for
        // each partition in the cur best soln
        uofm::vector<unsigned> _bestSolnPartNumNodes;  // NumNodes for
        // each partition in the cur best soln

        // these are used for the pruning of later starts
        unsigned _bestCostSoFar;
        unsigned _bestCostsMidWayCost;
        // cost at the mid-way point of the start
        // which produced the best final cost yet seen
        unsigned _midWayPoint;
        // check the cost of starts after the first call
        // to FM with <= _midWayPoint nodes

        PartitioningProblem& _problem;
        ClusteredHGraph* _hgraphs;

        Parameters _params;
        bool _doingCycling;
        PartitioningDoubleBuffer* _soln2Buffers;
        unsigned _bestSolnNum;
        Partitioning* _fixedConstraints;

        const uofm::vector<unsigned>* _terminalBlocksPtr;
        // for each Node, this == block idx it's in.
        // UINT_MAX == not a terminal

        const uofm::vector<BBox>* _termBlockBBoxesPtr;
        // bbox for each block w/ a terminal in it.

        unsigned _levelsToGo;

        uofm::vector<double> _prevLevelTolerances;    // one per weight
        uofm::vector<double> _bottomLevelTolerances;  // one per weight

        double _callPartTime;

        MultiStartPartitioner* _partitioner;

        BBPartBitBoardContainer& _bbBitBoards;
        MaxMem* _maxMem;

        FillableHierarchy* _hierarchy;  // if there is a hierarchy,
        // we'll use it. Otherwise this
        // will be NULL

        void _resetTerminals();

        void _callPartitionerWoML(PartitioningProblem& problem);
        void _callPartitioner(const HGraphFixed&, const Partitioning& fixed);

        void _computeNewMinAndMaxCap(const HGraphFixed& hgraph, uofm::vector<uofm::vector<double> >& newMinCaps, uofm::vector<uofm::vector<double> >& newMaxCaps);

        void _runPartitioner(PartitioningProblem& newProblem, const PartitionerParams& par, bool skipSolnGen);

        void _resetMinAndMaxCap(PartitioningProblem& newProblem);

        static unsigned _majCallCounter;
        static unsigned _minCallCounter;

        PartitionIds _calcTerminalPartitions(Point termLoc);

        void populate(ClusteredHGraph& hgraphs, const uofm::vector<unsigned>& terminalBlocks, const uofm::vector<BBox>& termBlockBBoxes);

       public:
        BaseMLPart(PartitioningProblem& problem, const Parameters& params, BBPartBitBoardContainer& bbBitBoards, MaxMem* maxMem, FillableHierarchy* hier = NULL);

        virtual ~BaseMLPart();

        // ------------- various public inspectors --------------

        double getPartitionArea(unsigned partNumber) const;
        unsigned getPartitionNumNodes(unsigned partNumber) const;
        double getPartitionRatio(unsigned partNumber) const;
        double getBestSolnCost() const { return _bestCost; }
        double getAveSolnCost() const { return _aveCost; }
        unsigned getNumLegalSolns() const { return _numLegalSolns; }
        double getUserTime() const { return _userTime; }
};

#ifndef _MSC_VER
typedef BaseMLPart::Parameters MLPartParams;
#else
#define MLPartParams BaseMLPart::Parameters
#endif

std::ostream& operator<<(std::ostream&, const MLPartParams&);

#endif
