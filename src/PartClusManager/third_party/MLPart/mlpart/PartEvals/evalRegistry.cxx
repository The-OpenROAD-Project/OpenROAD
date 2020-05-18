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

// created by Igor Markov on 05/20/98
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "Partitioning/partitionData.h"
#include "Partitioning/partProb.h"
#include "evalRegistry.h"

using std::ostream;
using std::cout;
using std::endl;
using std::setw;

#define SEARCH_FOR(NAME) \
        if (!newstrcasecmp(evalName, #NAME)) _type = NAME;

PartEvalType::PartEvalType(int argc, const char* argv[]) {
        _type = NetCutWNetVec;  // static_cast<Type>(UINT_MAX);
        StringParam evalName("partEval", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);

        /* if (helpRequest1.found() || helpRequest2.found())
           {
              cout <<" -partEval     NetCutWBits  | NetCutWConfigIds |
           NetCutWNetVec\n"
               << "             | BBox1Dim     | BBox2Dim           |
           BBox1DimWCheng \n"
               << "             | BBox2DimWCheng | BBox2DimWRSMT    | HBBox \n"
               << "             | HBBoxWCheng  | HBBoxWRSMT         | HBBox0 \n"
               << "             | HBBox0wCheng | HBBox0wRSMT        | StrayNodes
           \n"
               << "             | NetCut2way   | NetCut2wayWWeights |
           StrayNodes2way"
               << endl;
              return;
           }
        */
        if (helpRequest1.found() || helpRequest2.found()) {
                cout << " -partEval  NetCutWBits  | NetCutWConfigIds   | "
                        "NetCutWNetVec\n"
                     << "             | BBox1Dim                          | "
                        "StrayNodes \n"
                     << "             | NetCut2way   | NetCut2wayWWeights | "
                        "StrayNodes2way" << endl;
                return;
        }

        if (evalName.found()) {
                SEARCH_FOR(NetCutWBits)
                else SEARCH_FOR(NetCutWConfigIds) else SEARCH_FOR(NetCutWNetVec) else SEARCH_FOR(BBox1Dim)
                    /*
                         else
                         SEARCH_FOR(BBox2Dim)
                         else
                         SEARCH_FOR(BBox1DimWCheng)
                         else
                         SEARCH_FOR(BBox2DimWCheng)
                         else
                         SEARCH_FOR(BBox2DimWRSMT)
                         else
                         SEARCH_FOR(HBBox)
                         else
                         SEARCH_FOR(HBBoxWCheng)
                         else
                         SEARCH_FOR(HBBoxWRSMT)
                         else
                         SEARCH_FOR(HBBox0)
                         else
                         SEARCH_FOR(HBBox0wCheng)
                         else
                         SEARCH_FOR(HBBox0wRSMT)
                    */
                    else SEARCH_FOR(StrayNodes) else SEARCH_FOR(NetCut2way) else SEARCH_FOR(NetCut2wayWWeights) else SEARCH_FOR(StrayNodes2way) else abkfatal3(0,
                                                                                                                                                               " -partEval followed by "
                                                                                                                                                               "an unkown evaluator "
                                                                                                                                                               "name \"",
                                                                                                                                                               evalName, "\" in command line\n");
        } else {
                //     cout << "No -partEval <name> in command line; assuming
                // NetCutWNetVec... "
                //          << endl;
                _type = NetCutWNetVec;
        }
}

char PartEvalType::strbuf[32];

PartEvalType::operator const char*() const {
        switch (_type) {
                case NetCutWBits:
                        strncpy(strbuf, "NetCutWBits     ", 30);
                        return strbuf;
                case NetCutWConfigIds:
                        strncpy(strbuf, "NetCutWConfigIds", 30);
                        return strbuf;
                case NetCutWNetVec:
                        strncpy(strbuf, "NetCutWNetVec   ", 30);
                        return strbuf;
                case BBox1Dim:
                        strncpy(strbuf, "BBox1Dim        ", 30);
                        return strbuf;
                /*
                 case BBox2Dim         : strncpy(strbuf,"BBox2Dim
                 ",30);return strbuf;
                 case BBox1DimWCheng   : strncpy(strbuf,"BBox1DimWCheng
                 ",30);return strbuf;
                 case BBox2DimWCheng   : strncpy(strbuf,"BBox2DimWCheng
                 ",30);return strbuf;
                 case BBox2DimWRSMT    : strncpy(strbuf,"BBox2DimWRSMT
                 ",30);return strbuf;
                 case HBBox            : strncpy(strbuf,"HBBox
                 ",30);return strbuf;
                 case HBBoxWCheng      : strncpy(strbuf,"HBBoxWCheng
                 ",30);return strbuf;
                 case HBBoxWRSMT       : strncpy(strbuf,"HBBoxWRSMT
                 ",30);return strbuf;
                 case HBBox0           : strncpy(strbuf,"HBBox0
                 ",30);return strbuf;
                 case HBBox0wCheng     : strncpy(strbuf,"HBBox0wCheng
                 ",30);return strbuf;
                 case HBBox0wRSMT      : strncpy(strbuf,"HBBox0wRSMT
                 ",30);return strbuf;
                */
                case StrayNodes:
                        strncpy(strbuf, "StrayNodes      ", 30);
                        return strbuf;
                case NetCut2way:
                        strncpy(strbuf, "NetCut2way      ", 30);
                        return strbuf;
                case NetCut2wayWWeights:
                        strncpy(strbuf, "NetCut2wayWWeights", 30);
                        return strbuf;
                case StrayNodes2way:
                        strncpy(strbuf, "StrayNodes2way  ", 30);
                        return strbuf;
                default:
                        abkfatal(0,
                                 " Evaluator requested unknown to PartEvalType "
                                 "class\n");
        }
        return strbuf;
}

ostream& operator<<(ostream& os, const PartEvalType& pet) { return os << static_cast<const char*>(pet); }
