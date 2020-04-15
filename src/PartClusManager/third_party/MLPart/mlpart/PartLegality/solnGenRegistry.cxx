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

// created by Igor Markov on 07/10/99
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ABKCommon/abkcommon.h"
#include "solnGenRegistry.h"
#include <iostream>
#include "newcasecmp.h"

using std::ostream;
using std::cout;
using std::endl;

#define SEARCH_FOR(NAME) \
        if (!newstrcasecmp(solnGen, #NAME)) _type = NAME;

char SolnGenType::strbuf[32];

SolnGenType::SolnGenType(int argc, const char* argv[])
    // catches -moveMan
{
        _type = RandomizedEngineers;  // static_cast<Type>(UINT_MAX);
        StringParam solnGen("solnGen", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                cout << " -solnGen RandomizedEngineers | AllToOne | Bfs" << endl;
                return;
        }

        if (solnGen.found()) {
                SEARCH_FOR(RandomizedEngineers)
                else SEARCH_FOR(AllToOne) else SEARCH_FOR(Bfs) else abkfatal3(0, " -solnGen ", solnGen, " (unknown solution generator)\n");
        }
        // else
        //  cout << "No -solnGen <name> in command line; "
        //          "assuming RandomizedEngineers... "
        //       << endl;
}

SolnGenType::operator const char*() const {
        switch (_type) {
                case RandomizedEngineers:
                        strncpy(strbuf, "RandomizedEngineers", 30);
                        return strbuf;
                case AllToOne:
                        strncpy(strbuf, "AllToOne       ", 30);
                        return strbuf;
                case Bfs:
                        strncpy(strbuf, "Bfs            ", 30);
                        return strbuf;
                default:
                        abkfatal(0, " Unknown solution generator \n");
        }
        return strbuf;
}

ostream& operator<<(ostream& os, const SolnGenType& mmt) { return os << static_cast<const char*>(mmt); }
