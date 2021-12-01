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

// created by Igor Markov on 09/16/98

#ifndef _FM_MOVE_MANAGER_NC2w_H_
#define _FM_MOVE_MANAGER_NC2w_H_

#include "fmMMbaseNC2w.h"

class FMMoveManagerNC2w : public FMMMbaseNC2w {
       protected:
        inline bool isMoveLegal() const /* verifies all PartLegalities */
        {
                return _partLeg.isLegalMove(_movingModuleIdx, _moveFromPart, _moveToPart);
        }

        bool pickMove();

        void updateAffectedModules();
        void updateAffectedModulesWW();

        void updateGains() {
                lockModule(_movingModuleIdx);
                if (_useWts)
                        updateAffectedModulesWW();
                else
                        updateAffectedModules();
        }

        bool wiggleNow;

       public:
        FMMoveManagerNC2w(const PartitioningProblem& problem, const PartitionerParams& params, bool allowCorkingNodes) : FMMMbaseNC2w(problem, params, allowCorkingNodes) {
                wiggleNow = wiggleTerms;
        };
        virtual void resetTo(PartitioningSolution& newSol) {
                FMMMbaseNC2w::resetTo(newSol);
                wiggleNow = wiggleTerms;
        }
        virtual ~FMMoveManagerNC2w() {}

        bool pickMoveApplyIt();
        void computeGains();
        virtual void uncutNets();
};

#endif
