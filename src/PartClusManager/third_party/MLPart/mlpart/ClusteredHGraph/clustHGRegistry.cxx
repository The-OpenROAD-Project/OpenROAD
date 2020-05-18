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

// created by Andrew Caldwell on 06/25/98

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ClusteredHGraph/clustHGRegistry.h>
#include "newcasecmp.h"

using std::ostream;
using std::cout;
using std::endl;

#define SEARCH_FOR(NAME) \
        if (!newstrcasecmp(clustName, #NAME)) _type = NAME;

ClHG_ClusteringType::ClHG_ClusteringType(int argc, const char* argv[]) {
        _type = static_cast<Type>(UINT_MAX);
        StringParam clustName("clust", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                _type = HEM;
                cout << " -clust     {HEM, PHEM, BHEM, GRHEM, CutOpt, Random} " << endl;
                return;
        }

        if (clustName.found()) {
                SEARCH_FOR(PHEM)
                else SEARCH_FOR(HEM) else SEARCH_FOR(BHEM) else SEARCH_FOR(GRHEM) else SEARCH_FOR(CutOpt) else SEARCH_FOR(Random) else abkfatal3(0,
                                                                                                                                                 " -clust "
                                                                                                                                                 "followed "
                                                                                                                                                 "by an "
                                                                                                                                                 "unknown "
                                                                                                                                                 "clustering "
                                                                                                                                                 "algorithm "
                                                                                                                                                 "\"",
                                                                                                                                                 clustName,
                                                                                                                                                 "\" in "
                                                                                                                                                 "command "
                                                                                                                                                 "line\n");
        } else
                _type = HEM;
}

char ClHG_ClusteringType::strbuf[32];

ClHG_ClusteringType::operator const char*() const {
        switch (_type) {
                case HEM:
                        strncpy(strbuf, "HEM   ", 10);
                        return strbuf;
                        break;
                case PHEM:
                        strncpy(strbuf, "PHEM  ", 10);
                        return strbuf;
                        break;
                case BHEM:
                        strncpy(strbuf, "BHEM  ", 10);
                        return strbuf;
                        break;
                case GRHEM:
                        strncpy(strbuf, "GRHEM ", 10);
                        return strbuf;
                        break;
                case CutOpt:
                        strncpy(strbuf, "CutOpt", 10);
                        return strbuf;
                        break;
                case Random:
                        strncpy(strbuf, "Random", 10);
                        return strbuf;
                        break;

                default:
                        abkfatal(0,
                                 " Evaluator requested unknown to "
                                 "ClusteringType class\n");
        }
        return strbuf;
}

ostream& operator<<(ostream& os, const ClHG_ClusteringType& cl) { return os << static_cast<const char*>(cl); }
