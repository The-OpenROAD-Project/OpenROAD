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

// created by Igor Markov on 06/08/98

// This should be a parameter to partitioner, so that differently
// templated move managers can be instantiated.
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <iostream>
#include "ABKCommon/abkcommon.h"
#include "moveRegistry.h"
#include "newcasecmp.h"

using std::cout;
using std::endl;
using std::ostream;

#define SEARCH_FOR(NAME) \
        if (!newstrcasecmp(moveManName, #NAME)) _type = NAME;

char MoveManagerType::strbuf[32];

MoveManagerType::MoveManagerType(int argc, const char* argv[])
    // catches -moveMan
{
        _type = FM;  // static_cast<Type>(UINT_MAX);
        StringParam moveManName("part", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                cout << " -part FM | FMDD | |FMUCLA | FMwCutLineRef | FMHybrid "
                        "\n"
                     << "       | SA | HMetis | RandomGreedy | Greedy | AGreed" << endl;
                return;
        }

        if (moveManName.found()) {
                SEARCH_FOR(FMwCutLineRef)
                else if (!newstrcasecmp(moveManName, "FMwCLR")) _type = FMwCutLineRef;
                else SEARCH_FOR(FMDD) else SEARCH_FOR(FMUCLA) else SEARCH_FOR(FMHybrid) else SEARCH_FOR(FM) else SEARCH_FOR(SA) else SEARCH_FOR(RandomGreedy) else SEARCH_FOR(HMetis) else SEARCH_FOR(AGreed) else if (!newstrcasecmp(moveManName, "Greedy")) _type = FM;
                else abkfatal3(0, " -part ", moveManName, " (can't assign move manager)\n");
        }
        _type = FM;
}

MoveManagerType::operator const char*() const {
        switch (_type) {
                case FM:
                        strncpy(strbuf, "FM            ", 30);
                        return strbuf;
                case FMDD:
                        strncpy(strbuf, "FMDD          ", 30);
                        return strbuf;
                case FMUCLA:
                        strncpy(strbuf, "FMUCLA        ", 30);
                        return strbuf;
                case FMHybrid:
                        strncpy(strbuf, "FMHybrid      ", 30);
                        return strbuf;
                case FMwCutLineRef:
                        strncpy(strbuf, "FMwCutLineRef ", 30);
                        return strbuf;
                case SA:
                        strncpy(strbuf, "SA            ", 30);
                        return strbuf;
                case RandomGreedy:
                        strncpy(strbuf, "RandomGreedy  ", 30);
                        return strbuf;
                case AGreed:
                        strncpy(strbuf, "AGreed        ", 30);
                        return strbuf;
                case HMetis:
                        strncpy(strbuf, "HMetis        ", 30);
                        return strbuf;
                default:
                        abkfatal(0,
                                 " Move manager requested unknown to class "
                                 "MoveManagerType\n");
        }
        return strbuf;
}

ostream& operator<<(ostream& os, const MoveManagerType& mmt) { return os << static_cast<const char*>(mmt); }
