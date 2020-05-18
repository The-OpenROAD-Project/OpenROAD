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

//! author="Mike Oliver, 12/15/97"

#ifndef _ABKSEED_H_
#define _ABKSEED_H_

#include <cstdlib>
#include <cmath>
#include <fstream>

// Mateus@180515
#include <climits>
#include <cstring>
//--------------

#include <map>
#include "uofm_alloc.h"

//: Maintains all random seeds used by a process.
// Idea:  First time you create a random object, an initial nondeterministic
//       seed (also called the "external seed" is created from system
//       information and logged to seeds.out.
//       Any random object created with a seed of UINT_MAX ("nondeterministic"),
//       and that does not use the snazzy new "local independence" feature,
//       then uses seeds from a sequence starting with the initial value
//       and increasing by 1 with each new nondeterministic random object
//       created.
//
//       If the seed is overridden with a value other than UINT_MAX it is used,
//       and logged to seeds.out .
//
//       However if seeds.in exists, the initial nondeterministic value
//       is taken from the second line of seeds.in rather than the system
//       clock, and subsequent *deterministic* values are taken from successive
//       lines of seeds.in rather than from the value passed.
//
//       If seeds.out is in use or other errors occur trying to create it,
//       a warning is generated that the seeds will not be logged, and
//       the initial nondeterministic seed is printed (which should be
//       sufficient to recreate the run provided that all "deterministic"
//       seeds truly *are* deterministic (e.g. don't depend on external
//       devices).
//
//       You can disable the logging function by calling the static
//       function SeedHandler::turnOffLogging().  This will not disable
//       the ability to override seeding with seeds.in .
// NEW ULTRA-COOL FEATURE: (17 June 1999)
//       You now have the option, which I (Mike) highly recommend,
//       of specifying, instead of a seed, a "local identifier", which
//       is a string that should identify uniquely the random variable
//       (not necessarily the random object) being used.  The suggested
//       format of this string is as follows: If your
//       RNG is called _rng and is a member of MyClass, pass
//       the string "MyClass::_rng".  If it's a local variable
//       called rng in the method MyClass::myMethod(), pass
//       the string "MyClass::myMethod(),_rng".
//
//       SeedHandler will maintain a collection of counters associated
//       with each of these strings.  The sequence of random numbers
//       from a random object will be determined by three things:
//            1) the "external seed" (same as "initial nondeterm seed").
//            2) the "local identifier" (the string described above).
//            3) the counter saying how many RNG objects have previously
//                 been created with the same local identifier.
//       When any one of these changes, the output should be effectively
//       independent of the previous RNG.  Moreover you don't have
//       to worry about your regressions changing simply because someone
//       else's code which you call has changed the number of RNG objects
//       it uses.
// ANOTHER NEW FEATURE (21 February 2001)
//       At times you may wish to control the external seed yourself, e.g.
//       by specifying the seed on the command line, rather than
//       using either seeds.in or the nondeterministic chooser.
//       To enable this, a static method SeedHandler::overrideExternalSeed()
//       has been added.  If you call this method before any random
//       object has been created, you can set the external seed.
//       The file seeds.out will still be created unless you call
//       SeedHandler::turnOffLogging().  If seeds.in exists, it
//       will control -- your call to overrideExternalSeed() will
//       print a warning message and otherwise be ignored.

struct SeedHandlerCompareStrings {
        bool operator()(uofm::string firstString, uofm::string secondString) const { return strcmp(firstString.c_str(), secondString.c_str()) < 0; }
};
class SeedHandler {
       private:
        SeedHandler(unsigned seed);

        SeedHandler(const char *locIdent, unsigned counterOverride = UINT_MAX);

       public:
        ~SeedHandler();

       private:
        void _init();

        unsigned _seed;

        char *_locIdent;
        unsigned _counter;
        bool _isSeedMultipartite;

        static std::ofstream *_PseedOutFile;
        static std::ifstream *_PseedInFile;
        static unsigned _nextSeed;
        // slight misnomer,  This is the next nondeterministic seed.

        static unsigned _externalSeed;
        static unsigned _progOverrideExternalSeed;

        static std::map<uofm::string, unsigned, SeedHandlerCompareStrings> _counters;

        static bool _loggingOff;
        // don't log seeds
        static bool _haveRandObj;
        // starts false, true after first
        static bool _cleaned;
        // clean() has been called
        void _chooseInitNondetSeed();
        // first nondeterministic seed thereafter, increment.
        void _initializeOutFile();
        //  create log file and write first two lines.

        friend class RandomRoot;
        friend class SeedCleaner;

       public:
        static void turnOffLogging();
        static void overrideExternalSeed(unsigned extseed);
        static bool isInitialized() { return _haveRandObj; }
        static unsigned getExternalSeed() { return _externalSeed; }

       private:
        static void clean();
};

/* One global object of the following class, declared in seeds.cxx, will
make sure that SeedHandler::clean() is called at the end of execution*/
//: Makes it sure that SeedHandler will be cleaned at the end of execution
class SeedCleaner {
       public:
        SeedCleaner() {}

        ~SeedCleaner() { SeedHandler::clean(); }
};

#endif
