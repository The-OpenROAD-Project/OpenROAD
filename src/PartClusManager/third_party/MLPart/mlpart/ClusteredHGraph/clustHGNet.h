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

#ifndef _CLUSTHGRAPH_CLUST_CLNET_H_
#define _CLUSTHGRAPH_CLUST_CLNET_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>

class ClHG_Cluster;
class ClHG_ClusterTreeBase;
class HGFEdge;

typedef uofm::vector<ClHG_Cluster*>::iterator ClHG_NetClItr;
typedef uofm::vector<ClHG_Cluster*>::const_iterator ClHG_NetClConstItr;

class ClHG_ClNet {
        friend class ClHG_Cluster;
        friend class ClHG_ClusterTreeBase;
        friend class HEMClusteredHGraph;
        friend class PinHEMClusteredHGraph;
        friend class BestHEMClusteredHGraph;
        friend class GreedyHEMClusteredHGraph;
        friend class RandomClusteredHGraph;

       protected:
        unsigned _index;
        uofm::vector<ClHG_Cluster*> _clusters;
        unsigned _maxDegree;
        double _netWt;

       public:
        ClHG_ClNet(const HGFEdge& netSrc);
        ~ClHG_ClNet() {}

        unsigned getIndex() const { return _index; }

        ClHG_NetClItr clustersBegin() { return _clusters.begin(); }
        ClHG_NetClItr clustersEnd() { return _clusters.end(); }
        ClHG_NetClConstItr constClustersBegin() const { return _clusters.begin(); }
        ClHG_NetClConstItr constClustersEnd() const { return _clusters.end(); }

        unsigned getNumClusters() const { return _clusters.size(); }
        unsigned getDegree() const { return _clusters.size(); }
        unsigned getMaxDegree() const { return _maxDegree; }

        void setWeight(double newWt) { _netWt = newWt; }
        void incWeight(double inc) { _netWt += inc; }
        double getWeight() const { return _netWt; }

        friend std::ostream& operator<<(std::ostream& os, const ClHG_ClNet& net);

       protected:
        void addCluster(ClHG_Cluster& newCl) {
                abkassert(_clusters.size() < _maxDegree, "too many clusters on ClNet");
                _clusters.push_back(&newCl);
        }

        void removeCluster(ClHG_Cluster& oldCl);
};

#endif
