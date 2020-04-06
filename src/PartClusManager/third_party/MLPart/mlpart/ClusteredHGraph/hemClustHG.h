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

// created on 06/27/98 by Andrew Caldwell (caldwell@cs.ucla.edu)

#ifndef _CLUSTHGRAPH_HEM_CLUSTERTREE_H_
#define _CLUSTHGRAPH_HEM_CLUSTERTREE_H_

#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGTreeBase.h>
#include <HGraph/subHGraph.h>
#include <Placement/placeWOri.h>

class HEMClusteredHGraph : public virtual ClHG_ClusterTreeBase {
        uofm::vector<double> _edgeWeights;
        uofm::vector<ClHG_Cluster*> _adjNodes;
        uofm::vector<ClHG_Cluster*> _equalWtSmallNeighbors;
        uofm::vector<unsigned> _randMap;

        // by sadya+ramania for analytical clustering
        PlacementWOrient* _placement;

       public:
        HEMClusteredHGraph(const HGraphFixed& graph, const Parameters& params, const Partitioning* fixed = NULL, const Partitioning* curPart = NULL, PlacementWOrient* placement = NULL) : ClHG_ClusterTreeBase(graph, params, fixed, curPart), _placement(placement) {}

        virtual ~HEMClusteredHGraph() {}

       protected:
        void populateTree();
        void heavyEdgeMatchingLevel(double maxChildArea, double maxNewArea, unsigned targetNum);

        void matchDegree2Nodes(double maxChildArea, double maxNewArea);
        ClHG_Cluster* findBestNeighborExcept(ClHG_Cluster* cl0, ClHG_Cluster* except, double maxChildArea);

        bool isLarge(ClHG_Cluster* cl, double maxChildArea);
        void computeEqualWtSmallNeighbors(void);
        double min0tagNeighbor(ClHG_Cluster* cl0);
        void verify(unsigned numToCluster, unsigned origNumToCluster, double maxChildArea);
        void printAllReasonsForUnclusterable(ClHG_Cluster* cl0, double maxChildArea, double maxNewArea);
        double getConnectionWeight(const ClHG_ClNet& net, const ClHG_Cluster* cl0, const ClHG_Cluster* cl1);
        bool isCompatiblePair(ClHG_Cluster* cl0, ClHG_Cluster* cl1, double maxChildArea, double maxNewArea);
        bool isCompatiblePairRelaxed(ClHG_Cluster* cl0, ClHG_Cluster* cl1, double maxNewArea);
        bool isUnclusterable(ClHG_Cluster* cl, double maxChildArea);
        void mergeNodes(ClHG_Cluster* a, ClHG_Cluster* b);
        bool differentPartitions(ClHG_Cluster* a, ClHG_Cluster* b);
};

#endif
