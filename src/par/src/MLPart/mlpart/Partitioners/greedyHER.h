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

#ifndef _GREEDYHER_H_
#define _GREEDYHER_H_

#include "multiStartPart.h"
#include <set>
#include <unordered_map>

// this partitioner assumes bisection

class GreedyHERPartitioner : virtual public MultiStartPartitioner {
       public:
        class Parameters {
               public:
                Parameters(int argc, const char** argv);
                Parameters(const MultiStartPartitioner::Parameters& params);
                Parameters(void);
                virtual ~Parameters(void) {};

                void initializeDefaults();

                bool acceptZeroGainMoves;

                void setMultiStartPartParams(const MultiStartPartitioner::Parameters& params) { _multiStartPartParams = params; }
                const MultiStartPartitioner::Parameters& getMultiStartPartParams(void) const { return _multiStartPartParams; }

                friend std::ostream& operator<<(std::ostream& os, const Parameters& p);

               private:
                MultiStartPartitioner::Parameters _multiStartPartParams;
                void setValues(int argc, const char** argv);
        };

       public:
        GreedyHERPartitioner(PartitioningProblem& problem, const Parameters& params, bool skipSolnGen = false, bool dontRunYet = false);

        Parameters getParameters(void) { return _herParams; }

       protected:  // functions
        void doOne(unsigned initSoln);
        void doOneGreedyHER(unsigned initSoln);

       protected:  // data
        Parameters _herParams;

       private:  // functions
        void generateInitialSolution(Partitioning& curPart);
        void moveEdge(unsigned edgeIdx, unsigned toPart);
        void moveNode(unsigned nodeIdx);
        void initializeTallies(const Partitioning& curPart);
        void initializeSizes(const Partitioning& curPart);
        void initializeNodesInPart(void);
        bool isCut(unsigned edgeIdx);
        double computeGainOfMove(unsigned edgeIdx, unsigned toPart);
        bool moveValid(unsigned edgeIdx, unsigned toPart);
        void computeNodesInPart(const std::vector<unsigned>& edgesAffected);

       private:  // data
        std::vector<std::vector<unsigned> > _netTallies;
        const HGraphFixed& _hgraph;
        Partitioning* _curPart;
        bool _skipSolnGen;
        std::vector<double> _partSizes;
        std::vector<double> _partMaxCapacities;

        //_nodesInPart[edge][part]=list
        std::unordered_map<unsigned, std::vector<std::set<unsigned>>*> _nodesInPart;
};

#endif
