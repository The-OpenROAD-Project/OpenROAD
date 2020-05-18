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

#ifndef __HGPARAM_H_
#define __HGPARAM_H_

#include "ABKCommon/abkcommon.h"

class HGraphParameters {
       public:
        unsigned netThreshold;
        // makes edges of degree > netThreshold have degree 0
        bool removeBigNets;
        // if true, these nets are not just emptied, but removed from the HGraph

        bool makeAllSrcSnk;
        // if true, all directions are set at SrcSnk when reading .netD

        enum NodeSortMethod {
                DONT_SORT_NODES,
                SORT_NODES_BY_INDEX,
                SORT_NODES_BY_ASCENDING_DEGREE,
                SORT_NODES_BY_DESCENDING_DEGREE
        };
        // determines sort order of edges on nodes
        NodeSortMethod nodeSortMethod;

        enum EdgeSortMethod {
                DONT_SORT_EDGES,
                SORT_EDGES_BY_INDEX,
                SORT_EDGES_BY_ASCENDING_DEGREE,
                SORT_EDGES_BY_DESCENDING_DEGREE
        };
        // determines sort order of nodes on edges
        EdgeSortMethod edgeSortMethod;

        enum ShredTopology {
                SINGLE_NET,
                GRID,
                STAR
        };
        // determines topology of nets connecting shreds when shredding
        ShredTopology shredTopology;

        Verbosity verb;

        double yScale;

        HGraphParameters() : netThreshold(UINT_MAX), removeBigNets(false), makeAllSrcSnk(true), nodeSortMethod(DONT_SORT_NODES), edgeSortMethod(DONT_SORT_EDGES), shredTopology(GRID), yScale(1.0) {}

        HGraphParameters(const HGraphParameters& hgP) : netThreshold(hgP.netThreshold), removeBigNets(hgP.removeBigNets), makeAllSrcSnk(hgP.makeAllSrcSnk), nodeSortMethod(hgP.nodeSortMethod), edgeSortMethod(hgP.edgeSortMethod), shredTopology(hgP.shredTopology), verb(hgP.verb), yScale(hgP.yScale) {}

        HGraphParameters& operator=(const HGraphParameters& hgP);

        HGraphParameters(int argc, const char** argv);

        friend std::ostream& operator<<(std::ostream&, const HGraphParameters&);
};

#endif
