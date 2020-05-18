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

// Created by Igor Markov

#ifndef _1BAL_CHK_H_
#define _1BAL_CHK_H_

#include "Partitioning/partProb.h"
#include "Combi/partitionIds.h"

class AllToOneGen;
class SBGWeightRand;
class SBGRandDutt;
class SBGRandVeryIllegal;

// typedef SBGRandVeryIllegal   SingleBalanceGenerator;
// typedef SBGRandDutt   SingleBalanceGenerator;
typedef SBGWeightRand SingleBalanceGenerator;  // orig. UCLA rand generator
// typedef AllToOneGen   SingleBalanceGenerator;  // Vile

class AllToOneGen {
        const PartitioningProblem& _problem;
        static unsigned counter;

       public:
        AllToOneGen(const PartitioningProblem& problem) : _problem(problem) {}
        uofm::vector<double> generateSoln(Partitioning& curPart);
};

class SBGWeightRand {
        const PartitioningProblem& _problem;
        static RandomRawUnsigned _ru;

       public:
        SBGWeightRand(const PartitioningProblem& problem) : _problem(problem) {}
        uofm::vector<double> generateSoln(Partitioning& curPart);

       protected:
        bool assignNodeToPartition(Partitioning& curPart, unsigned nodeIdx, unsigned nonZeroChances, double minNonZero, uofm::vector<double>& areaToMin, uofm::vector<double>& areaToFill, uofm::vector<double>& availableArea, uofm::vector<double>& chancesOfGoingToPart);
};

class SBGRandDutt {
        const PartitioningProblem& _problem;
        static RandomRawUnsigned _ru;

       public:
        SBGRandDutt(const PartitioningProblem& problem) : _problem(problem) {}
        uofm::vector<double> generateSoln(Partitioning& curPart);
};

class SBGRandVeryIllegal {
        const PartitioningProblem& _problem;
        static RandomRawUnsigned _ru;

       public:
        SBGRandVeryIllegal(const PartitioningProblem& problem) : _problem(problem) {}
        uofm::vector<double> generateSoln(Partitioning& curPart);
};

#endif
