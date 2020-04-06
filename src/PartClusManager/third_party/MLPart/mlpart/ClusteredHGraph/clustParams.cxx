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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "baseClustHGraph.h"
#include "Combi/combi.h"
#include "HGraph/hgFixed.h"

using std::cout;
using std::endl;
using std::ostream;

BaseClusteredHGraph::Parameters::Parameters() : clType(), clusterRatio(1.3), maxNewClRatio(9.0), maxChildClRatio(4.5), maxClArea(0), sizeOfTop(200), levelGrowth(2.5), removeDup(0), weightOption(3), dontClusterTerms(false), verb("1 1 1") {}

BaseClusteredHGraph::Parameters::Parameters(ClHG_ClusteringType cltype) : clType(cltype), clusterRatio(1.3), maxNewClRatio(9.0), maxChildClRatio(4.5), maxClArea(0), sizeOfTop(200), levelGrowth(2.5), removeDup(0), weightOption(3), dontClusterTerms(false), verb("1 1 1") {}

BaseClusteredHGraph::Parameters::Parameters(int argc, const char* argv[]) : clType(argc, argv), clusterRatio(1.3), maxNewClRatio(9.0), maxChildClRatio(4.5), maxClArea(0), sizeOfTop(200), levelGrowth(2.5), removeDup(0), weightOption(3), dontClusterTerms(false), verb("1 1 1") {
        DoubleParam clusterRatio_("clClusterRatio", argc, argv);
        DoubleParam levelGrowth_("levelGrowth", argc, argv);

        DoubleParam maxNewClRatio_("clMaxNewClRatio", argc, argv);
        DoubleParam maxChildClRatio_("clMaxChildClRatio", argc, argv);
        DoubleParam maxClArea_("clMaxClArea", argc, argv);

        UnsignedParam sizeOfTop_("clSizeOfTop", argc, argv);
        UnsignedParam removeDup_("clRemoveDup", argc, argv);

        UnsignedParam weightOption_("weightOption", argc, argv);

        BoolParam dontClusterTerms_("dontClusterTerms", argc, argv);

        BoolParam help1("h", argc, argv);
        BoolParam help2("help", argc, argv);

        if (help1 || help2) {
                cout << " --Clustering Options: \n"
                     << "  -clClusterRatio    <double> [1.3] (> 1.0)  \n"
                     << "  -levelGrowth       <double> [2.5]  \n"
                     << "  -clMaxNewClRatio   <double> [9.0](multiple of "
                        "average)"
                     << "(0=no limit)\n"
                     << "  -clMaxChildClRatio <double> [4.5](multiple of "
                        "average)"
                     << "(0=no limit) \n"
                     << "  -clMaxClArea       <double> [0.0](percent of total)"
                     << "(0=no limit) \n"
                     << "  -clSizeOfTop       <unsigned>[200] "
                     << "  -clRemoveDup       <0,2,3>[0] \n"
                     << "  -weightOption      <0-7> [3] \n"
                     << "  -dontClusterTerms  [false] \n" << endl;
        }

        if (clusterRatio_.found()) clusterRatio = clusterRatio_;
        if (levelGrowth_.found()) levelGrowth = levelGrowth_;

        if (maxNewClRatio_.found()) maxNewClRatio = maxNewClRatio_;
        if (maxChildClRatio_.found()) maxChildClRatio = maxChildClRatio_;
        if (maxClArea_.found()) maxClArea = maxClArea_;

        if (sizeOfTop_.found()) sizeOfTop = sizeOfTop_;
        if (removeDup_.found()) removeDup = removeDup_;

        if (weightOption_.found()) weightOption = weightOption_;

        if (dontClusterTerms_.found()) dontClusterTerms = true;
}

BaseClusteredHGraph::Parameters::Parameters(const Parameters& p) : clType(p.clType), clusterRatio(p.clusterRatio), maxNewClRatio(p.maxNewClRatio), maxChildClRatio(p.maxChildClRatio), maxClArea(p.maxClArea), sizeOfTop(p.sizeOfTop), levelGrowth(p.levelGrowth), removeDup(p.removeDup), weightOption(p.weightOption), dontClusterTerms(p.dontClusterTerms), verb(p.verb) {}
ostream& operator<<(ostream& os, const BaseClusteredHGraph::Parameters& p) {
        os << "Clustering Parameters" << endl;
        os << " Clustering Type: " << p.clType << endl;
        os << " ClusterRatio:    " << p.clusterRatio << endl;
        os << " LevelGrowth:     " << p.levelGrowth << endl;
        os << " MaxNewClRatio:   " << p.maxNewClRatio << endl;
        os << " MaxChildClRatio: " << p.maxChildClRatio << endl;
        os << " MaxClArea:       " << p.maxClArea << endl;
        os << " SizeOfTop:       " << p.sizeOfTop << endl;
        os << " RemoveDup:       " << p.removeDup << endl;
        os << " WeightOption:    " << p.weightOption << endl;
        os << " dontClusterToTerms " << p.dontClusterTerms << endl;

        return os;
}
