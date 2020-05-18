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

//! author="Andrew Caldwell and Igor Markov on 05/14/98 "
//! CONTACTS=" Igor Andy ABK"

#ifndef __MultiStart_PARTITIONER_H__
#define __MultiStart_PARTITIONER_H__

#include "ABKCommon/uofm_alloc.h"
#include "Partitioning/partitioning.h"
#include "moveRegistry.h"
#include "PartEvals/evalRegistry.h"
#include "PartLegality/solnGenRegistry.h"

#include <iostream>

//: Partitioner with multiple start
class MultiStartPartitioner {
       public:
        //: generic parameters for multi-start partitioners
        class Parameters {
               public:
                Verbosity verb;

                MoveManagerType moveMan;  // all have default values
                PartEvalType eval;
                SolnGenType solnGen;

                bool legalizeOnly;  // stop when a legal soln is reached
                unsigned useClip;   // ignored by non-FM type partitioners
                // 0=don't, 1=do, 2="halfClip"
                bool useEarlyStop;  // for FM type partitioners,
                // if useEarlyStop == true, they may
                // terminate passes early
                double maxHillHeightFactor;  // e.g. for FM, SA etc
                // stops when cur cost > maxH.. times curBestCost
                // (0-1] means greedy; 0 means climb till death
                double maxHillWidth;  // e.g. for FM, SA etc; measured in %%
                                      // #modules
                // stops when # of nonimproving moves > maxH..
                // times #modules
                // meaningful values 0-100
                bool printHillStats;   // i.e. spend valuable CPU to compute
                                       // stats
                unsigned maxNumMoves;  // 0 == unlimited

                double minPassImprovement;  // if pass improved less, no more
                                            // passes
                unsigned maxNumPasses;      // 0 == unlimited
                unsigned relaxedTolerancePasses;
                int unCorking;  // 0 -- check all, +k -- invalidate bucket
                // after k failures; -k -- same, but
                // if it wasn't a failure, put the offending
                // buckets into the end of the list
                unsigned skipNetsLargerThan;
                double skipLargestNets;  // 0 to 100 percent

                bool randomized;

                bool doFirstLIFOPass;

                bool allowCorkingNodes;  // skip nodes of area >= the balance
                                         // tol

                bool allowIllegalMoves;

                bool tieBreakOnPins;

                bool wiggleTerms;

                bool saveMoveLog;

                bool useWts;
                bool mapBasedGC;

                // as of 051206 hashBasedGC vs array based is dynamically
                // selected if
                // neither of these parameters are set
                bool hashBasedGC;    // force hashBasedGC always
                bool noHashBasedGC;  // never use hashBasedGC

                // this is a MetaPlacer meta-parameter, which forces hashBasedGC
                // if
                // noHashBasedGC is not present.
                bool saveMem;

                Parameters(Verbosity verbosity = Verbosity("1 1 1"));

                Parameters(int argc, const char* argv[]);
                Parameters(const Parameters& params);
                void initializeDefaults();
        };

       public:
       protected:
        Parameters _params;
        static RandomRawUnsigned _rng;
        PartitioningProblem& _problem;

        uofm::vector<PartitioningSolution*> _solutions;
        unsigned _bestSolnNum;
        unsigned _beginUsedSoln;
        unsigned _endUsedSoln;

        // DATA CACHED FOR INSPECTORS
        double _userTime;
        // for randomized multi-start methods
        double _aveCost;
        unsigned _bestCost;
        double _stdDev;
        unsigned _numLegalSolns;
        // for hill-climbing methods
        double _maxHillHeightSeen;
        double _maxGoodHillHeightSeen;
        double _maxHillHeightSeenThisPass;

        double _maxHillWidthSeen;
        double _maxGoodHillWidthSeen;
        double _maxHillWidthSeenThisPass;

       protected:
        void setBestSolAndAverageCost();

        virtual void doOne(unsigned initSolN) = 0;
        // It is recommended that this method be implemented in one line
        // e.g., "{ doOneFM(initSoln); }", this facilitates fancier multistarts

       public:
        MultiStartPartitioner(PartitioningProblem& problem, const Parameters& params);

        virtual ~MultiStartPartitioner();

        virtual void runMultiStart();  // in terms of doOne()

        const PartitioningSolution& getResult(unsigned solnNumber) const { return *_solutions[solnNumber]; }
        const PartitioningSolution& getBestResult() const { return *_solutions[_bestSolnNum]; }

        unsigned getNumSolns() const { return _endUsedSoln - _beginUsedSoln; }
        unsigned getBestSolnNum() const { return _bestSolnNum; }
        unsigned getBestSolnCost() const { return _bestCost; }
        double getAveSolnCost() const { return _aveCost; }
        double getStdDevForCost() const { return _stdDev; }
        unsigned getNumLegalSolns() const { return _numLegalSolns; }
        double getUserTime() const { return _userTime; }
        double getMaxHillHeight() const { return _maxHillHeightSeen; }
        double getMaxGoodHill() const { return _maxGoodHillHeightSeen; }
        double getMaxHillWidth() const { return _maxHillWidthSeen; }
        double getMaxGoodHillWidth() const { return _maxGoodHillWidthSeen; }

        virtual unsigned getBestSolnPinImbalance() const { return _solutions[_bestSolnNum]->pinImbalance; }

        virtual double getAveNumPassesMade() const { return DBL_MAX; }
        virtual double getAveNumMovesAttempted() const { return DBL_MAX; }
        virtual double getAveNumMovesMade() const { return DBL_MAX; }
};

#ifndef _MSC_VER
typedef MultiStartPartitioner::Parameters PartitionerParams;
#else
#define PartitionerParams MultiStartPartitioner::Parameters
#endif

std::ostream& operator<<(std::ostream& os, const PartitionerParams& parms);

#endif
