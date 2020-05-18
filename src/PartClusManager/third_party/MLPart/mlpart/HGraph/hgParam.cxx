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

#include <ABKCommon/abkcommon.h>
#include <HGraph/hgBase.h>

#include <iostream>

using std::ostream;
using std::cout;
using std::endl;

HGraphParameters::HGraphParameters(int argc, const char** argv) : netThreshold(UINT_MAX), removeBigNets(false), makeAllSrcSnk(true), nodeSortMethod(DONT_SORT_NODES), edgeSortMethod(DONT_SORT_EDGES), shredTopology(GRID), verb(argc, argv), yScale(1.0) {
        BoolParam removeBigNetsFlag("hgRmBigNets", argc, argv);
        UnsignedParam netThresholdNum("hgNetThreshold", argc, argv);
        UnsignedParam skipNetsLargerThanNum("skipNetsLargerThan", argc, argv);
        BoolParam makeAllSrcSnkFlag("hgMakeAllSrcSnk", argc, argv);
        BoolParam helpRequest1("h", argc, argv);
        BoolParam helpRequest2("help", argc, argv);
        StringParam nodeSortStr("hgNodeSort", argc, argv);
        StringParam edgeSortStr("hgEdgeSort", argc, argv);
        StringParam shredTopologyStr("shredTopology", argc, argv);

        if (helpRequest1.found() || helpRequest2.found()) {
                cout << "== HGraph parameters ==" << endl << " -hgRmBigNets \n"
                     << " -hgNetThreshold <unsigned> \n"
                     << " -skipNetsLargerThan <unsigned> \n"
                     << " -hgMakeAllSrcSnk \n"
                     << " -hgNodeSort <ascdeg | descdeg | index | none> \n"
                     << " -hgEdgeSort <ascdeg | descdeg | index | none> \n"
                     << " -shredTopology <singleNet | grid | star> \n";
                return;
        }

        removeBigNets = removeBigNetsFlag.found();
        if (makeAllSrcSnkFlag.found()) makeAllSrcSnk = makeAllSrcSnkFlag.on();
        if (netThresholdNum.found()) netThreshold = netThresholdNum;
        if (skipNetsLargerThanNum.found()) {
                removeBigNets = true;
                netThreshold = skipNetsLargerThanNum;
        }

        if (nodeSortStr.found()) {
                if (!strcmp(nodeSortStr, "ascdeg"))
                        nodeSortMethod = SORT_NODES_BY_ASCENDING_DEGREE;
                else if (!strcmp(nodeSortStr, "descdeg"))
                        nodeSortMethod = SORT_NODES_BY_DESCENDING_DEGREE;
                else if (!strcmp(nodeSortStr, "index"))
                        nodeSortMethod = SORT_NODES_BY_INDEX;
                else if (strcmp(nodeSortStr, "none"))
                        abkfatal(0, "Incorrect value for hgNodeSort param");
        }
        if (edgeSortStr.found()) {
                if (!strcmp(edgeSortStr, "ascdeg"))
                        edgeSortMethod = SORT_EDGES_BY_ASCENDING_DEGREE;
                else if (!strcmp(edgeSortStr, "descdeg"))
                        edgeSortMethod = SORT_EDGES_BY_DESCENDING_DEGREE;
                else if (!strcmp(edgeSortStr, "index"))
                        edgeSortMethod = SORT_EDGES_BY_INDEX;
                else if (strcmp(edgeSortStr, "none"))
                        abkfatal(0, "Incorrect value for hgEdgeSort param");
        }
        if (shredTopologyStr.found()) {
                if (!strcmp(shredTopologyStr, "singleNet"))
                        shredTopology = SINGLE_NET;
                else if (!strcmp(shredTopologyStr, "grid"))
                        shredTopology = GRID;
                else if (!strcmp(shredTopologyStr, "star"))
                        shredTopology = STAR;
                else
                        abkfatal(0, "Incorrect value for shredTopology param");
        }

        DoubleParam yScale_("yScale", argc, argv);
        if (yScale_.found()) {
                yScale = yScale_;
        }
}

HGraphParameters& HGraphParameters::operator=(const HGraphParameters& hgP) {
        netThreshold = hgP.netThreshold;
        removeBigNets = hgP.removeBigNets;
        makeAllSrcSnk = hgP.makeAllSrcSnk;
        nodeSortMethod = hgP.nodeSortMethod;
        edgeSortMethod = hgP.edgeSortMethod;
        shredTopology = hgP.shredTopology;
        verb = hgP.verb;
        yScale = hgP.yScale;

        return *this;
}

ostream& operator<<(ostream& out, const HGraphParameters& param) {
        const char* sortName[4] = {"no sort", "by index", "ascending degree", "descending degree"};
        const char* shredName[3] = {"Single Net", "Grid", "Star"};

        out << "HGraph Parameters: \n"
            << "   makeAllSrcSnk : " << param.makeAllSrcSnk << "\n"
            << "   removeBigNets : " << param.removeBigNets << "\n"
            << "   netThreshold  : " << param.netThreshold << "\n"
            << "   nodeSortMethod: " << sortName[param.nodeSortMethod] << "\n"
            << "   edgeSortMethod: " << sortName[param.edgeSortMethod] << "\n"
            << "   shredTopology: " << shredName[param.shredTopology] << "\n";
        return out;
}
