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

#ifndef _CLUSTHGRAPH_CLUST_CLUSTER_H_
#define _CLUSTHGRAPH_CLUST_CLUSTER_H_

// CHANGES

// 980917 aec -ver 1.2
//	removed 'induced nets'. We only need 'cutNets'.

#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGNet.h>
#include <Combi/partitionIds.h>

class HGraphFixed;
class HGFNode;
class ClHG_ClusterTreeBase;

struct ClHG_CutNet {
        unsigned netId;
        ClHG_ClNet* net;
        short unsigned numLeaves;
        // num descendents of this cluster on this net

        ClHG_CutNet(unsigned idx, ClHG_ClNet& netRef, unsigned leaves) : netId(idx), net(&netRef), numLeaves(leaves) {}
};

typedef uofm::vector<ClHG_CutNet> ClHG_CutNets;
typedef uofm::vector<ClHG_CutNet>::iterator ClHG_CutNetItr;

typedef uofm::vector<ClHG_Cluster*> ClHG_Children;
typedef uofm::vector<ClHG_Cluster*>::iterator ClHG_ChildItr;

class ClHG_Cluster {
        friend class ClHG_ClusterTreeBase;
        friend class HEMClusteredHGraph;
        friend class PinHEMClusteredHGraph;
        friend class BestHEMClusteredHGraph;
        friend class GreedyHEMClusteredHGraph;
        friend class RandomClusteredHGraph;

       protected:
        const unsigned _index;
        double _area;
        unsigned _leafDesc;  // number of leaf desc. of this cluster

        ClHG_CutNets _cutNets;

        short int _tag;

        PartitionIds _part;
        PartitionIds _fixed;
        unsigned char _terminal : 1;
        unsigned char _clusterable : 1;

       public:
        uofm::vector<unsigned> hgraphIds;
        // the list of hgraphIds, in the most recient 'snapshot' of the
        // clusters in that hgraph, which are descendents of this cluster.
        // this is used for constructing the mapping between clusters

        // leaf node ctor
        ClHG_Cluster(const HGraphFixed& hgraph, const HGFNode& node, ClHG_ClusterTreeBase& tree, PartitionIds fixed, PartitionIds part);

        ~ClHG_Cluster() {}

        void mergeWith(ClHG_Cluster& newChild, ClHG_ClusterTreeBase& tree);

        unsigned getIndex() const { return _index; }
        double getArea() const { return _area; }

        void setTag(unsigned tag) { _tag = tag; }
        unsigned getTag() const { return _tag; }

        bool isTerminal() const { return _terminal; }
        bool isClusterable() const { return _clusterable; }

        unsigned getNumLeafDesc() const { return _leafDesc; }
        bool isLeaf() const { return _leafDesc == 1; }

        ClHG_CutNetItr cutNetsBegin() { return _cutNets.begin(); }
        ClHG_CutNetItr cutNetsEnd() { return _cutNets.end(); }
        unsigned getDegree() const { return _cutNets.size(); }

        PartitionIds getFixed() const { return _fixed; }
        PartitionIds getPart() const { return _part; }
        PartitionIds getAllowableParts() {
                PartitionIds okparts = _fixed;
                okparts &= _part;
                return okparts;
        }

        friend std::ostream& operator<<(std::ostream& out, const ClHG_Cluster& cl);

       protected:
        void removeIndNets() {
                size_t n = _cutNets.size();
                while (n > 0) {
                        --n;
                        _cutNets[n].net->removeCluster(*this);
                }
        }

        void induceNets() {
                size_t n = _cutNets.size();
                while (n > 0) {
                        --n;
                        _cutNets[n].net->addCluster(*this);
                }
        }
};

class ClHG_CompareCutNetsByNetIndex {
       public:
        ClHG_CompareCutNetsByNetIndex() {}

        bool operator()(const ClHG_CutNet& n1, const ClHG_CutNet& n2) { return (n1.netId < n2.netId); }

        bool operator()(const unsigned nIdx, const ClHG_CutNet& n2) { return (nIdx < n2.netId); }

        bool operator()(const ClHG_CutNet& n1, const unsigned nIdx) { return (n1.netId < nIdx); }
};

struct RemoveContainedNets {
        uofm::vector<unsigned>& numCont;
        RemoveContainedNets(uofm::vector<unsigned>& numCont_) : numCont(numCont_) {}
        bool operator()(ClHG_CutNet& cutN) {
                cutN.numLeaves = numCont[cutN.netId];
                numCont[cutN.netId] = 0;
                return (cutN.numLeaves == cutN.net->getMaxDegree());
        }
};

class CompareCutNetsByNetId {
       public:
        CompareCutNetsByNetId();

        bool operator()(const ClHG_CutNet& cn1, const ClHG_CutNet& cn2) { return cn1.net->getIndex() < cn2.net->getIndex(); }
};

#endif
