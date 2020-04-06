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

// created by Andrew Caldwell and Igor Markov on 06/06/98

#ifndef __FM_PARTITIONER_H__
#define __FM_PARTITIONER_H__

#include "Partitioners/multiStartPart.h"
#include "Partitioners/moveMgrXFace.h"
#include "mmSwitchBox.h"
#include "Partitioners/svMoveLog.h"

class FMPartitioner : virtual public MultiStartPartitioner {
       protected:
        // active move manager is either LIFO or moveMgr2
        // no remote ownership, so don't delete
        MoveManagerInterface* _activeMoveMgr, *_moveMgrLIFO, *_moveMgr2;
        MoveManagerSwitchBoxInterface* _switchBox;

        // these are used for accounting & calculating the moves/second
        unsigned _totalMovesMade;
        double _totalTime;

        double _peakMemUsage;

        bool _skipSolnGen;

        // these are stored so that we don't have to reset the MM, etc
        // after the last pass (ie, MM keeps track of current cost, but
        // it would need to be reset after undo's to be accurate)

        double _bestPassCost;
        double _bestPassImbalance;
        double _bestPassViolation;
        uofm::vector<double> _bestPassAreas;

        SVMoveLog _moveLog;

        unsigned _numPasses;

        MaxMem* _maxMem;

        virtual void doOne(unsigned initSoln) { doOneFM(initSoln); }

        void doOneFM(unsigned initSoln);
        void doOneFMRelaxedTol(unsigned initSoln, unsigned relTolPasses, uofm::vector<double>& maxCellSize);  // by sadya.
                                                                                                              // does a
                                                                                                              // relaxed
                                                                                                              // Tol FM

        unsigned doOneFMPass(bool useClip);
        void setupPass();
        void cleanupPass();
        void setMoveManagerAndSwitchBox();
        void swapMoveManagers();

       public:
        FMPartitioner(PartitioningProblem& problem, MaxMem* maxMem, const Parameters& params, bool skipSolnGen = false, bool dontRunYet = false);

        virtual ~FMPartitioner() {
                if (_moveMgr2 && _moveMgr2 != _moveMgrLIFO) delete _moveMgr2;
                if (_moveMgrLIFO) delete _moveMgrLIFO;
                if (_switchBox) delete _switchBox;
        }
        const SVMoveLog& getMoveLog() const { return _moveLog; }
        PartitionerParams& getParams(void) { return _params; }
        double peakMemUsage() const { return _peakMemUsage; }
        double getAveNumPassesMade() const { return _numPasses; }
};

#endif
