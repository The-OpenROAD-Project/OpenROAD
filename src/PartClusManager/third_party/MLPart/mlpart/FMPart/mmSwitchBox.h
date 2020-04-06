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

// Created by Igor Markov, Sept 22, 1998
// 980923 aec - changed behavior of 'conservative' switchbox, not to exit
//		if the first pass fails

#ifndef _MMSWITCHBOX_H_
#define _MMSWITCHBOX_H_

class mmSwitchBoxInterface;
typedef mmSwitchBoxInterface MoveManagerSwitchBoxInterface;

// a move manager switchbox is a small state machine
// that's updated after a pass with information whether the pass improved cost
// the state machine then tells whether to stop passes
// and/or whether to swap MMs in the next pass

class mmSwitchBoxInterface {
       protected:
        unsigned _passNumber;
        bool _quitNow;
        bool _swapMMs;
        bool _secondMMinUse;

       public:
        mmSwitchBoxInterface() : _passNumber(0), _quitNow(false), _swapMMs(false), _secondMMinUse(false) {}
        virtual ~mmSwitchBoxInterface() {}
        // state changes
        virtual void passImprovedCost() = 0;
        virtual void passFailed() = 0;
        virtual void reinitialize() {
                _passNumber = 0;
                _quitNow = false;
                _swapMMs = false;
                _secondMMinUse = false;
        }
        // state queries
        unsigned getPassNumber() const { return _passNumber; }
        bool quitNow() const { return _quitNow; }
        bool swapMMs() const {
                abkassert(!_quitNow, "No next pass");
                return _swapMMs;
        }
        bool isSecondMMinUse() const { return _secondMMinUse; }
};

class mmSwitchBoxNoSwaps : public mmSwitchBoxInterface
                           // never swap MMs, quit after the first failure
                           {
       public:
        mmSwitchBoxNoSwaps() {}
        virtual void passImprovedCost() { _passNumber++; }
        virtual void passFailed() {
                _passNumber++;
                _quitNow = true;
        }
};

class mmSwitchBoxSwapOnce : public mmSwitchBoxInterface {
       public:
        mmSwitchBoxSwapOnce() {
                _swapMMs = true;
                _secondMMinUse = true;
        }  // start w/ the 2nd MM
        virtual void reinitialize() {
                _passNumber = 0;
                _quitNow = false;
                _swapMMs = true;
                _secondMMinUse = true;
        }
        virtual void passImprovedCost() {
                _swapMMs = false;
                _passNumber++;
        }
        virtual void passFailed() {
                if (_secondMMinUse) {
                        _swapMMs = true;
                        _secondMMinUse = false;
                } else
                        _quitNow = true;
                _passNumber++;
        }
};

class mmSwitchBoxConservative : public mmSwitchBoxInterface
                                // if the first pass didn't improve, swap MMs
                                // if the second also does not improve, quit.
                                // otherwise continue until no improvement - swap MMs
                                // continue until no improvement, then quit
                                {
        bool _firstFailed;

       public:
        mmSwitchBoxConservative() : _firstFailed(false) {}
        virtual void reinitialize() {
                _passNumber = 0;
                _quitNow = false;
                _swapMMs = false;
                _secondMMinUse = false;
                _firstFailed = false;
        }

        virtual void passImprovedCost() {
                if (_passNumber == 0) {
                        _firstFailed = false;
                        _swapMMs = true;
                        _secondMMinUse = true;
                } else
                        _swapMMs = false;
                _passNumber++;
        }

        virtual void passFailed() {
                if (_passNumber == 0) {
                        _firstFailed = true;
                        _swapMMs = true;
                        _secondMMinUse = true;
                } else if (_passNumber == 1 && _firstFailed)
                        // both failed the firt time out..quit
                        _quitNow = true;
                else if (_secondMMinUse) {
                        _swapMMs = true;
                        _secondMMinUse = false;
                } else
                        _quitNow = true;
                _passNumber++;
        }
};

#endif
