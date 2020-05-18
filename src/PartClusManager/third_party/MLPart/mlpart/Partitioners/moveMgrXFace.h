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

//!author="Andrew Caldwell and Igor Markov on 05/14/98"

#ifndef __MOVE_MGR_XFACE_H__
#define __MOVE_MGR_XFACE_H__

#include "Partitioning/partitioning.h"
#include "svMoveLog.h"

//: Interface for move manager
class MoveManagerInterface {

       protected:
        const PartitioningProblem& _problem;
        // use this only for info not available otherwise
        // Never const_cast _problem !
        PartitioningSolution* _partSol;
        Partitioning* _curPart;
        const Partitioning* _fixedConstr;

       public:
        bool randomized;
        unsigned skipNetsLargerThan;
        int unCorking;  // a given number of nodes; default 1; 0 means infty

        bool allowIllegalMoves;
        bool wiggleTerms;

        MoveManagerInterface(const PartitioningProblem& problem) : _problem(problem), _partSol(NULL), _curPart(NULL), _fixedConstr(&problem.getFixedConstr()), randomized(false), skipNetsLargerThan(UINT_MAX), unCorking(1), wiggleTerms(false) {};
        /*
           Need to resetTo(PartitioningSolution&) befor every pass
           The follwoing interface is getting obsolete as it entails duplication
        */

        MoveManagerInterface(const PartitioningProblem& problem, PartitioningSolution& partSol) : _problem(problem), _partSol(&partSol), _curPart(&partSol.part), _fixedConstr(&problem.getFixedConstr()), randomized(false) {};

        virtual ~MoveManagerInterface() {}

        /*virtual bool haveMovesLeft() const    = 0; */
        virtual bool pickMoveApplyIt() = 0;
        // haveMovesLeft was removed because it is often just as costly
        // to decide if there are any legal moves left as it is to pick one.
        // Rather, pickMoveApplyIt returns a bool.  It has value true if
        // a legal move was found (even if it was not taken, ie, in SA),
        // and value false if no legal moves were found.

        virtual unsigned getCost() const = 0;
        virtual double getCostDouble() const = 0;

        virtual void reinitialize() = 0;  // reinitialize the evaluators
        // and everything

        virtual void reinitTolerances() = 0;  // reinitialize data fields
        // connected with tolerances

        virtual void resetTo(PartitioningSolution& newSol) = 0;
        // there is no general implementation of resetTo..it needs to
        // be overloaded in the derived movemanagers.  It should reset
        // evaluators, etc, and copy code from the MM's reinitialize
        // function, rather than calling it (otherwise, it will reinitialize
        // some things twice).

        PartitioningSolution& getSolution() { return *_partSol; }

        //-'status' information highly depends on the specific MoveManager,
        // thus we can't define it here.

        /* KL-type move managers will define */
        virtual void undo(unsigned) {}
        virtual const SVMoveLog& getMoveLog() const = 0;
        virtual void computeGains() {}
        virtual void uncutNets() {}
        virtual void setupClip() {}

        virtual double getViolation() const = 0;
        virtual double getImbalance() const = 0;

        typedef const uofm::vector<double>& rConstVecDbl;
        virtual rConstVecDbl getAreas() const = 0;

        /* SA-type move managers will define */
        virtual void setTemp(double newTemp) { static_cast<void>(newTemp); }
        virtual void clearAcceptStats() {}
        virtual unsigned getNumAccepted() const { return 0; }
        virtual unsigned getNumRejected() const { return 0; }
};

#endif
