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

//! author="Igor Markov, Nov 16, 1997"

#ifndef _VERBOSITY_H_
#define _VERBOSITY_H_

#include <iostream>
#include <ABKCommon/uofm_alloc.h>

//: Click to see more comments for verbosity
//   The goal of standardization of verbosity options
//   is to be able to produce consistent diagnostics
//   when using multiple packages. For example, if you
//   run package B with option "silent" and it uses
//   package A, then package A should also understand
//   "silent".
//
//   Use model: constructors of complicated objects
//              can receive optional parameters.
//              A Verbosity object will be one of them
//              (with default "silent").
//              If an object wishes to write some
//              diagnostics, it first looks at
//              the verbosity level for this type
//              of diagnostics and decides whether
//              to write diags or not.
//
//              Note: ATM stderr/cerr output should
//                    not be affected by class Verbosity
//
//   Class Verbosity stores "verbosity levels"
//   for a variable number of "diagnostic types".
//
//   A verbosity level is an unsigned which tells
//   how much diagnostic output of a given type
//   should be printed to cout. Verbosity level
//   0 means "silent" and the bigger the level,
//   the more output should be printed (level
//   K contains all output of level N if K>=N).
//
//   At present, verbosity levels are required for
//   at least three diagnostic types (see comments
//   in the definition of clas Verbosity). Required
//   diag. types have additional support in
//   class Verbose.
//   Tentative general requirements:  for each type,
//   output with levels < 10^(k+1) should
//   be O(n^k) -  long (wrt the program input).
//
//   Usage: this class can be used as is or as a base class.
//
//   Note: if needed, we can add extensions to handle
//   output to different diagnostic streams
//   (e.g. for each type)
class Verbosity {
        uofm::vector<unsigned> _levels;

        void _ctructFromString(const char* levels);

       public:
        Verbosity();
        // the default is "silent"
        Verbosity(const char* levels);
        //  space or underscore-separated unsigneds
        //  can also be "silent" and "0" (same as "0 0 0", same as "0_0_0")
        Verbosity(int argc, const char* argv[]);
        //  catches -verb
        Verbosity(unsigned numArgs, unsigned forActions, unsigned forSysRes, unsigned forMajStats, ...);
        Verbosity(const uofm::vector<unsigned>&);

        Verbosity(const Verbosity& v) : _levels(v._levels) {}

        // Verbosity& operator=(const Verbosity&);

        unsigned getNumTypes() const { return _levels.size(); }

        unsigned& operator[](unsigned diagType);

        unsigned getForActions(void) const { return _levels[0]; }
        unsigned getForSysRes(void) const { return _levels[1]; }
        unsigned getForMajStats(void) const { return _levels[2]; }
        void setForActions(unsigned verb) { _levels[0] = verb; }
        void setForSysRes(unsigned verb) { _levels[1] = verb; }
        void setForMajStats(unsigned verb) { _levels[2] = verb; }

        // unsigned & forActions;
        //// "verbosity for actions" means writing "doing this, doing that"
        ////    with more or less detail, depending on the level

        // unsigned & forSysRes;
        //// "verbosity for system resources" means writing
        ////    how much memory/CPU time/etc was used
        ////    in more or fewer places depending on the level

        // unsigned & forMajStats;
        //// "verbosity for major stats" means writing
        ////    quantities/sizes of importants components,
        ////    on more or fewer occasions, depending on the level

        friend std::ostream& operator<<(std::ostream& os, const Verbosity& verbsty);
};

std::ostream& operator<<(std::ostream& os, const Verbosity& verbsty);

#endif
