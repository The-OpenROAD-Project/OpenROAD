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

// created by Andrew Caldwell and Igor Markov on 05/17/98

#ifndef __PART_EVAL_XFACE_H__
#define __PART_EVAL_XFACE_H__

#include "Partitioning/partProb.h"

// Note: derived classes should only be used when the precise class
//       is clear (not to compile any methods as virtual)

class PartEvalXFace {
       protected:
        const Partitioning* _part;  // maintained by caller

        const PartitioningProblem* _problem;  // use only to get info
                                              // not available otherwise
        const HGraphFixed& _hg;

       public:
        PartEvalXFace(const PartitioningProblem& problem, const Partitioning& part) : _part(&part), _problem(&problem), _hg(problem.getHGraph()) {};
        PartEvalXFace(const PartitioningProblem& problem) : _part(NULL), _problem(&problem), _hg(problem.getHGraph()) {};

        PartEvalXFace(const HGraphFixed& hg, const Partitioning& part) : _part(&part), _problem(NULL), _hg(hg) {};
        virtual ~PartEvalXFace() {};
        // This is "skimmed" ctor ;-)

        virtual unsigned computeCostOfOneNet(unsigned netIdx) const = 0;
        virtual double computeCostOfOneNetDouble(unsigned netIdx) const = 0;

        // "get cost" means "get precomputed cost"
        virtual unsigned getNetCost(unsigned netId) const = 0;
        virtual double getNetCostDouble(unsigned netId) const = 0;

        virtual unsigned getTotalCost() const = 0;      // one will be implemented
        virtual double getTotalCostDouble() const = 0;  // via the other

        // Reinitializes the evaluator if the partition changed
        // "non-incrementally"
        // Must be redefined in derived classes by calling
        // same method in the immediate parent (with scopre resolution operator
        // ::)
        virtual void reinitialize();
        virtual void resetTo(const Partitioning& newPart) {
                _part = &newPart;
                reinitialize();
        }

        // These represent incremental update by one move (with cost updates),
        virtual void moveModuleTo(unsigned moduleNumber, unsigned from, unsigned to) = 0;
        virtual void moveModuleTo(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) = 0;
        virtual void moveWithReplication(unsigned moduleNumber, PartitionIds old, PartitionIds newIds) = 0;

        // these are overwhelmingly typical implementations. override if
        // necessary.
        virtual unsigned getMinCostOfOneNet() const { return 0; }
        virtual double getMinCostOfOneNetDouble() const { return getMinCostOfOneNetDouble(); }

        // These highly depend on the evaluator and hypergraph
        virtual unsigned getMaxCostOfOneNet() const = 0;
        virtual double getMaxCostOfOneNetDouble() const { return getMaxCostOfOneNetDouble(); }

        virtual bool isNetCut() const { return false; }
        virtual bool isNetCut2way() const { return false; }
        virtual int getDeltaGainDueToNet(unsigned netId, unsigned movingFrom, unsigned movingTo, unsigned gainingFrom, unsigned gainingTo) const {
                (void)netId;
                (void)movingFrom;
                (void)movingTo;
                (void)gainingFrom;
                (void)gainingTo;
                abkfatal(0, "Not implemented");
                return 0;
        }
        virtual bool netCostNotAffectedByMove(unsigned netId, unsigned from, unsigned to) {
                (void)netId;
                (void)from;
                (void)to;
                return false;
        }
        // if not overriden, this will be detected at compile time
};

#endif
