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

//! author="Igor Markov, June 15, 1997"
//! CHANGES="infolines.h 971130  ilm  reworked clas MemUsage() to give readings
// of both peak memory and estimated memory usage cout << MemUsage() now works
// similar to cout << TimeStamp;"

/*
 This file to be included into all projects in the group

 CHANGES

 971130  ilm  reworked clas MemUsage() to give readings
              of both peak memory and estimated memory usage
              cout << MemUsage() now works similar to cout << TimeStamp;
*/
#ifndef _INFOLINES_H_
#define _INFOLINES_H_

#include "abkcpunorm.h"
#include "abkassert.h"
#include "uofm_alloc.h"

#include <cstdlib>
#include <time.h>
#include <iostream>
#include <iomanip>

//: Baseclass for TimeStamp, CmdLine, Platform, MemUsage, MemChange, User.
//  it will be used, e.g., when writing commented info into files. The class
//  is printable.
class InfoLine {
       protected:
        char* _infoLine;
        InfoLine() : _infoLine(NULL) {}

       public:
        virtual ~InfoLine() {
                if (_infoLine) delete[] _infoLine;
        }
        virtual operator const char*() const { return _infoLine; }
};

//: Prints a command line.
struct CmdLine : public InfoLine {
        CmdLine(int argc, const char* argv[]);
};

//: Prints starting execution info
class ExecInfo : public InfoLine {
       public:
        ExecInfo(); /* defined in platfDepend.cxx */
};

//: Prints user's home directory
struct UserHomeDir : public InfoLine {
        UserHomeDir(); /* defined in platfDepend.cxx */
};

//: Prints the location of the executable
struct ExecLocation : public InfoLine {
        ExecLocation(); /* defined in platfDepend.cxx */
};

//: Prints current's time
struct TimeStamp : public InfoLine {
        TimeStamp(bool justTime = false);
};

//: Prints information about the platform you are using
struct Platform : public InfoLine {
        /* defined in platfDepend.cxx */
        Platform();
};

//: Prints information about currnt user
struct User : public InfoLine {
        /* defined in platfDepend.cxx */
        User();
};

//: Prints information about the memory usage of the process
class MemUsage {
        double _peak;
        // in Mbytes
        double _estimate;

       public:
        /* defined in platfDepend.cxx */
        MemUsage();
        double getPeakMem() const { return _peak; }
        double getEstimate() const { return _estimate; }
        operator double() const { return _estimate; }
};

class VMemUsage  // virtual memory usage -- aaronnn
    {
        double _minflt;
        double _majflt;
        // in MB:
        double _rss;  // resident set size
        // in MB:
        double _phys;               // physical size available
        static double _phys_total;  // total physical size

       public:
        /* defined in platfDepend.cxx */
        VMemUsage();
        double getMinPageFaults() const { return _minflt; }
        static double measureMinPageFaults();
        double getMajPageFaults() const { return _majflt; }
        static double measureMajPageFaults();
        double getRSS() const { return _rss; }
        static double measureRSS();
        double getPhysAvailable() { return _phys; }
        static double measurePhysAvailable();
        static double getPhysTotal() { return _phys_total; }
};

std::ostream& operator<<(std::ostream&, const MemUsage&);

class MaxMem {
        bool _printExtra;
        bool _noticedThrashing;
        double _peak;
        double _initialMinFaults;
        double _initialMajFaults;
        uofm::string _message;

       public:
        MaxMem(void);
        void update(const char* message);
        bool noticedThrashing() const;
        bool printExtra() const;
        void setPrintExtra(bool print);
        double getPeak() const;
        const uofm::string& getMessage() const;
};

std::ostream& operator<<(std::ostream&, const MaxMem&);

class MemChange {
        double _memUsageMark;
        // in Mbytes
       public:
        /*  defined in platfDepend.cxx */
        MemChange();
        void resetMark();
        // does the same as ctor
        operator double() const;
        // returns the change wrt the stored value
};

//: For system information
struct SysInfo {
        TimeStamp tm;
        Platform pl;
        CPUNormalizer cpunorm;
        User us;
        MemUsage mu;

        SysInfo() {};
};

std::ostream& operator<<(std::ostream&, const SysInfo&);

#endif
