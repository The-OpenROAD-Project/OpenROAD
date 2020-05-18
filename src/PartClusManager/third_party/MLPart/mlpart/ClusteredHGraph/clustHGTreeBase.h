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

#ifndef _CLHGRAPH_CLTREEBASE_H_
#define _CLHGRAPH_CLTREEBASE_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGNet.h>
#include <ClusteredHGraph/clustHGCluster.h>
#include <ClusteredHGraph/baseClustHGraph.h>

class HGraphFixed;
class PartitionIds;

class ClHG_ClusterTreeBase : public BaseClusteredHGraph {
        friend class ClHG_Cluster;

       protected:
        static RandomRawUnsigned _rng;

        unsigned _nextId;  // Index the next cluster added will get

        unsigned _numLeafClusters;   // num Clusters at bottom level
        unsigned _numLeafTerminals;  // num Terminals at bottom level

        uofm::vector<ClHG_Cluster*> _toCluster;  // nodes to cluster.
        //(clusterable) terminals come first in this vector
        uofm::vector<ClHG_Cluster*> _terminals;  // unclusterable terminals
        unsigned _numClusterableTerminals;

        uofm::vector<ClHG_ClNet> _nets;  // one net for each HGraph net.

        double _totalArea;
        double _minArea;  // smallest non-zero area of a (leaf)cell

        uofm::vector<unsigned> _numContained;  // indexed by net id.  Used in
                                               // creating
        // a new cluster's set of cut nets

        ClHG_ClusterTreeBase(const HGraphFixed& leafLevel, const Parameters& params, const Partitioning* fixed = NULL, const Partitioning* origPart = NULL);

        virtual ~ClHG_ClusterTreeBase();

        ClHG_ClNet& getNetByIdx(unsigned idx) { return _nets[idx]; }

        ClHG_Cluster* addClust(const HGraphFixed& hgraph, const HGFNode& node, PartitionIds fixed, PartitionIds part);

        // adds a 'snap-shot' hgraph to BaseClusteredHGraph
        void addHGraph();
        void addTopLevelPartitioning();

        void cleanupOldNodes(uofm::vector<ClHG_Cluster*>& oldNodes);
        void cleanupContainedEdges();

        void removeDuplicateEdges();

        void printStatsHeader();
        void printStats();

        void clusterDegree1Nodes();
        void clusterDegree2Nodes();

        unsigned numClusterable() const { return _toCluster.size() - _numClusterableTerminals; }

        double evalCurrentClustering() const;

       public:
        static unsigned deg2Removed;
        static unsigned deg3Removed;

        // <aaronnn>
        static void spinRandomNumber(unsigned numToWaste);
};

inline ClHG_Cluster* ClHG_ClusterTreeBase::addClust(const HGraphFixed& hgraph, const HGFNode& node, PartitionIds fixed, PartitionIds part) {
        _nextId = std::max(_nextId, node.getIndex() + 1);
        ClHG_Cluster* newCl = new ClHG_Cluster(hgraph, node, *this, fixed, part);
        PartitionIds intersect = fixed;
        intersect |= part;
        abkassert(!intersect.isEmpty(), "fixed and part have null intersection");

        return newCl;
}

inline void ClHG_ClusterTreeBase::cleanupOldNodes(uofm::vector<ClHG_Cluster*>& oldNodes) {
        for (unsigned i = 0; i < oldNodes.size(); i++) delete oldNodes[i];
        oldNodes.clear();
}

struct RemoveClusteredNodes {
        RemoveClusteredNodes() {}
        bool operator()(ClHG_Cluster* cl) {
                if (cl->getTag() == 1) {
                        delete cl;
                        return true;
                }
                return false;
        }
};

#endif
